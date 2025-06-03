// Copyright 2025 Shift Crypto AG
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <da1458x_config_advanced.h>
#include <da1458x_config_basic.h>
#include <rwip_config.h>
#include <user_config.h>

#include "app.h"
#include "app_easy_security.h"
#include "app_easy_timer.h"
#include "app_prf_perm_types.h"
#include "gap.h"
#include "gattc_task.h"
#include "uart_task.h"
#include "util.h"
#include "version.h"
#include <app_bond_db.h>
#include <arch.h>
#include <custs1.h>
#include <custs1_task.h>
#include <debug.h>
#include <gpio.h>
#include <hw_otpc.h>
#include <otp_cs.h>
#include <rf_531.h>
#include <uart.h>
#include <user_app.h>
#include <user_custs1_def.h>
#include <user_custs1_impl.h>
#include <user_periph_setup.h>

///
/// This file implements various callbacks called by the SDK
///

// This struct holds the firmware version and the git hash.
struct version {
  uint16_t version;
  uint8_t hash[20];
} __attribute__((packed));

// This struct holds metadata about the firmware
struct metadata {
  uint8_t metadata_version; // The version of the format of this struct.
  struct version version;
} __attribute__((packed));

// The metadata about the firmware is placed on a fixed offset in the output by
// the linker script so that other tools knows where to find it
struct metadata metadata __attribute__((section(".metadata"), used)) = {
    .metadata_version = 1,
    .version = {.version = GIT_VERSION, .hash = GIT_HASH}};

// Set to one to create firmware that locks chip. Locking chip means that the
// built-in bootloader doesn't enable the debug interface.
#define LOCK_CHIP 1

// In debug builds we enable the debug interface when the firmware boots
#if defined(NDEBUG)
#define SWD_ENABLED 0
#else
#define SWD_ENABLED 1
#endif

uint8_t app_connection_idx __SECTION_ZERO("retention_mem_area0");
timer_hnd
    app_param_update_request_timer_used __SECTION_ZERO("retention_mem_area0");
uint8_t app_connection_status __SECTION_ZERO("retention_mem_area0");

// If we receive shut down command over UART we store it here to avoid
// restarting advertising.
bool shutting_down = false;

// The scan response data is stored here. The SDK has been modified to read this
// instead of reading the definition in the config file.
uint8_t scan_rsp_data[SCAN_RSP_DATA_LEN] __SECTION_ZERO("retention_mem_area0");
uint8_t scan_rsp_data_len __SECTION_ZERO("retention_mem_area0");

// The IRK is defined as a global here and the SDK has been modified to copy
// from it instead of copying from the definition in the config file.
uint8_t irk[KEY_LEN] __SECTION_ZERO("retention_mem_area0");

// Helper function to update the scan response data
void user_app_set_scan_rsp_data(uint8_t *data, uint8_t data_len) {
  LOG_X("rsp: ", data, data_len);
  uint8_t len = MIN(SCAN_RSP_DATA_LEN, data_len);
  memcpy(&scan_rsp_data[0], data, len);
  scan_rsp_data_len = len;
  app_easy_gap_update_adv_data((uint8_t *)USER_ADVERTISE_DATA,
                               USER_ADVERTISE_DATA_LEN, scan_rsp_data,
                               scan_rsp_data_len);
}

// offset - offset in bytes from 0x07f80000 (MEMORY_OTP_BASE)
// data - pointer to data
// count - number of words to read
static int _otp_read(uint32_t offset, uint32_t *data, uint32_t count) {
  int ret = count;

  uint32_t addr = MEMORY_OTP_BASE + offset;
  hw_otpc_init();
  hw_otpc_enter_mode(HW_OTPC_MODE_READ);

  while (count--) {
    *data++ = GetWord32(addr);
    addr += sizeof(uint32_t);
  }
  hw_otpc_close();
  return ret;
}

// offset - offset in bytes from 0x07f80000 (MEMORY_OTP_BASE)
// data - pointer to data
// count - number of words to write
static void _otp_write(uint32_t offset, uint32_t *data, uint32_t count) {
  offset = offset / sizeof(uint32_t);
  hw_otpc_init();
  hw_otpc_prog(data, offset, count);
  hw_otpc_close();
}

// This function reads the configuration space (CS) in the one time programmable
// memory (OTP) and sets a flag to disable the bootloader from enabling the
// debug interface. It must write the flag to the first unused slot, because the
// bootloader will only read until it reaches the that.
static bool _factory_setup(void) {
  // READ CS
  uint32_t cs_buf[OTP_CS_MAX_ENTRIES];
  _otp_read(OTP_CS_BASE_OFFSET, &cs_buf[0], OTP_CS_MAX_ENTRIES);

  // Check "start command" is correct
  if (cs_buf[0] != 0xA5A5A5A5) {
    LOG("factory-setup: Invalid magic word");
    return false;
  }

  // Search for first unused word
  int i = 0;
  for (i = 0; i < sizeof(cs_buf) / sizeof(uint32_t); i++) {
    if (cs_buf[i] == OTP_CS_CMD_SWD_MODE) {
      // Already configured
      LOG("factory-setup: Already configured");
      return true;
    }
    if (cs_buf[i] == OTP_CS_EMPTY_VAL) {
      // Found first empty word
      break;
    }
  }

  if (i == sizeof(cs_buf) / sizeof(uint32_t)) {
    // No room left in configuration space
    LOG("factory-setup: No room for configuration");
    return false;
  }

  LOG("empty word %d: 0x%08x", i, (unsigned int)cs_buf[i]);

  // Write "disable bootloader from enabling debug interface" instruction
  if (LOCK_CHIP) {
    uint32_t buf = OTP_CS_CMD_SWD_MODE;
    _otp_write(OTP_CS_BASE_OFFSET + i * 4, &buf, 1);
  }

  // For good measure, disable the debug interface. see datasheet p.198.
  SetBits16(SYS_CTRL_REG, DEBUGGER_ENABLE, 0);

  return true;
}

/// This callback is called once after the TASK_APP has been created and
/// initialized to DISABLED from main() in arch_system.c.
void user_app_on_init_cb(void) {
  LOG_S("user_app_on_init()\n");

  // Initialize globals
  app_connection_idx = GAP_INVALID_CONIDX;
  app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;

  rf_pa_pwr_adv_set(RF_TX_PWR_LVL_MINUS_19d5);

  if (!_factory_setup()) {
    LOG_S("Failed running factory setup\n");
  }

  // Ensure that debugging is on in DEBUG builds.
  if (SWD_ENABLED) {
    SetBits16(SYS_CTRL_REG, DEBUGGER_ENABLE,
              3 /* Enable SWD in P0_2/P0_10, see datasheet p.198 */);
  }

  // Create uart task
  uart_task_init();

  //  To keep compatibility call default handler
  default_app_on_init();

#if (BLE_APP_SEC)
  // Set service security requirements
  app_set_prf_srv_perm(TASK_ID_CUSTS1, APP_CUSTS1_SEC_REQ);
  // Fetch bond data from the external memory
  app_easy_security_bdb_init();
  LOG_S("bond db loaded\n");
#endif

  uint16_t read = sl_irk_load(&irk[0], sizeof(irk));
  ASSERT_ERROR(read == sizeof(irk));
  LOG_S("irk loaded\n");

  // Where the SDK stores the "Identity address".
  extern struct bd_addr dev_bdaddr;
  read = sl_identity_address_load(&dev_bdaddr.addr[0], sizeof(dev_bdaddr.addr));
  ASSERT_ERROR(read == sizeof(dev_bdaddr.addr));
  LOG_S("address loaded\n");

  read = sl_device_name_load(&device_info.dev_name.name[0],
                             sizeof(device_info.dev_name.name));
  ASSERT_ERROR(read > 0);
  device_info.dev_name.length = read;
  uint8_t buf[SCAN_RSP_DATA_LEN];
  buf[0] = read + 1;
  buf[1] = GAP_AD_TYPE_COMPLETE_NAME;
  int len = MIN(SCAN_RSP_DATA_LEN - 2, read);
  memcpy(&buf[2], &device_info.dev_name.name[0], len);
  LOG_S("device name loaded\n");

  user_app_set_scan_rsp_data(&buf[0], read + 2);
  uart_task_enable();
}

/// The purpose of this callback is to start advertising
///
/// This callback is called in the default implementations of some callbacks:
/// * default_app_on_connection (when no device was connected)
/// * default_app_on_disconnect
/// * default_app_on_set_dev_config_complete (which is called when GAP
///   configuration is updated)
/// * default_app_on_db_init_complete (which is called when TASK_APP enter
///   CONNECTABLE)
void user_default_operation_adv_cb(void) {
  app_easy_gap_undirected_advertise_start();

  // Inform MCU that we are advertising again
  app_connection_status = BLE_STATUS_ADVERTISING;
  uart_task_notify_connection_status(app_connection_status);
}

/// When a connection is established we check if packet intervals, latency and
/// timeout is matching between us on the central. If not this callback is
/// scheduled to request an update of those parameters.
static void param_update_request_timer_cb() {
  app_easy_gap_param_update_start(app_connection_idx);
  app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
}

/// This callback is called on every new connection. Since we only allow a
/// single connected device we store the connection id in `app_connection_idx`.
void user_app_on_connection_cb(uint8_t conidx,
                               struct gapc_connection_req_ind const *param) {
  if (app_env[conidx].conidx != GAP_INVALID_CONIDX) {
    rf_pa_pwr_conn_set(RF_TX_PWR_LVL_PLUS_2d5, conidx);
    LOG("new connection %d\n", conidx);
    app_connection_idx = conidx;

    app_connection_status = BLE_STATUS_CONNECTED;
    uart_task_notify_connection_status(app_connection_status);

    default_app_on_connection(conidx, param);

    // Check if the parameters of the established connection are the
    // preferred ones. If not then schedule a connection parameter update
    // request.
    if ((param->con_interval < user_connection_param_conf.intv_min) ||
        (param->con_interval > user_connection_param_conf.intv_max) ||
        (param->con_latency != user_connection_param_conf.latency) ||
        (param->sup_to != user_connection_param_conf.time_out)) {
      // Connection params are not these that we expect
      // LOG("would like to udpate params\n");
      app_param_update_request_timer_used =
          app_easy_timer(30, param_update_request_timer_cb);
    }
  } else {
    // No connection has been established, restart advertising
    user_default_operation_adv_cb();
  }
}

// TODO: Document why this is needed
void user_app_adv_undirect_complete_cb(uint8_t status) {
  LOG("user_app_adv_undirect_complete_cb %d\n", status);
  // If advertising was canceled then update advertising data and start
  // advertising again
  if (shutting_down) {
    debug_uart("RF power down");
    rf_power_down();
    return;
  }
  if (status == GAP_ERR_CANCELED) {
    user_default_operation_adv_cb();
  }
}

/// Callback called when disconnect happens, advertisting is restarted
void user_app_on_disconnect_cb(struct gapc_disconnect_ind const *param) {
  LOG("app_on_disconnectio_cb\n");
  app_connection_idx = GAP_INVALID_CONIDX;
  //  Cancel the parameter update request timer
  if (app_param_update_request_timer_used != EASY_TIMER_INVALID_TIMER) {
    app_easy_timer_cancel(app_param_update_request_timer_used);
    app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
  }
  //  Restart Advertising
  if (shutting_down) {
    debug_uart("RF power down");
    rf_power_down();
    return;
  }
  user_default_operation_adv_cb();
}

#if (BLE_APP_SEC)
/// Callback called when exchanging temporary key during pairing. The key is
/// sent over UART to the MCU.
void user_app_on_tk_exch_cb(uint8_t conidx,
                            struct gapc_bond_req_ind const *param) {
  if (param->data.tk_type == GAP_TK_KEY_CONFIRM) {
    struct uart_tx_req *req = KE_MSG_ALLOC_DYN(
        UART_TX, KE_BUILD_ID(TASK_UART, 0), TASK_APP, uart_tx_req, KEY_LEN + 1);
    req->type = SL_PT_CTRL_DATA;
    req->length = KEY_LEN + 1;
    req->value[0] = SL_CTRL_CMD_PAIRING_CODE;
    memcpy(&req->value[1], &param->tk.key[0], KEY_LEN);
    KE_MSG_SEND(req);
#if 0
    uint32_t passkey;
    // Print the 6 Least Significant Digits of Passkey
    char buf[6];
    passkey = (param->tk.key[0] << 0) | (param->tk.key[1] << 8) |
              (param->tk.key[2] << 16) | (param->tk.key[3] << 24);
    LOG("\n Confirmation Value: ");
    for (uint8_t i = 0; i < 6; i++) {
      buf[5 - i] = passkey % 10;
      passkey /= 10;
    }
    for (uint8_t i = 0; i < 6; i++) {
      LOG("%u", buf[i]);
    }
    LOG("\n");
#endif
  }
}

#endif

/// This callback is called in gapm_cmp_evt_handler when the "operation" was
/// GAPM_CANCEL or any operation not covered in the gapm_cmp_evt_handler
void user_app_process_catch_rest_cb(ke_msg_id_t const msgid, void const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id) {
  switch (msgid) {
  case CUSTS1_VAL_WRITE_IND: {
    // LOG("CUSTS1_VAL_WRITE_IND");
    struct custs1_val_write_ind *msg_param =
        (struct custs1_val_write_ind *)param;
    switch (msg_param->handle) {
    case SVC1_IDX_RX_VAL:
      user_svc1_rx_val_ind_handler(msgid, msg_param, dest_id, src_id);
      break;
    case SVC1_IDX_PRODUCT_IND_CFG:
      user_svc1_product_val_cfg_ind_handler(msgid, msg_param, dest_id, src_id);
      break;
    }
  } break;
  case CUSTS1_VAL_NTF_CFM:
    // LOG("CUSTS1_VAL_NTF_CFM");
    break;

  case CUSTS1_VAL_IND_CFM: {
    // LOG("CUSTS1_VAL_IND_CFM");
    struct custs1_val_ind_cfm const *msg_param =
        (struct custs1_val_ind_cfm const *)(param);

    switch (msg_param->handle) {
    case SVC1_IDX_RX_VAL:
      break;

    default:
      break;
    }
  } break;

  case CUSTS1_VALUE_REQ_IND: {
    // LOG("CUSTS1_VALUE_REQ_IND");
  } break;
  // TODO: Why is this needed?
  case CUSTS1_ATT_INFO_REQ: {
    // LOG("CUSTS1_ATT_INFO_REQ");
    struct custs1_att_info_req const *msg_param =
        (struct custs1_att_info_req const *)param;

    user_svc1_rest_att_info_req_handler(msgid, msg_param, dest_id, src_id);
  } break;

  case GAPC_PARAM_UPDATED_IND: {
    // Cast the "param" pointer to the appropriate message structure
    struct gapc_param_updated_ind const *msg_param =
        (struct gapc_param_updated_ind const *)(param);

    // Check if updated Conn Params filled to preferred ones
    if ((msg_param->con_interval >= user_connection_param_conf.intv_min) &&
        (msg_param->con_interval <= user_connection_param_conf.intv_max) &&
        (msg_param->con_latency == user_connection_param_conf.latency) &&
        (msg_param->sup_to == user_connection_param_conf.time_out)) {
      LOG("UPDATED PARAMS interval: %dms\n",
          (msg_param->con_interval * 1250) / 1000);
    }
  } break;

  case GATTC_EVENT_REQ_IND: {
    // Confirm unhandled indication to avoid GATT timeout
    struct gattc_event_ind const *ind = (struct gattc_event_ind const *)param;
    struct gattc_event_cfm *cfm =
        KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
    cfm->handle = ind->handle;
    KE_MSG_SEND(cfm);
  } break;

  default:
    break;
  }
}

// Callback called when bonding is succeeded.
void user_app_on_pairing_succeeded_cb(uint8_t conidx) {
  LOG("pairing succeeded\n");
  default_app_on_pairing_succeeded(conidx);
  struct uart_tx_req *req = KE_MSG_ALLOC_DYN(UART_TX, KE_BUILD_ID(TASK_UART, 0),
                                             TASK_APP, uart_tx_req, 1);
  req->type = SL_PT_CTRL_DATA;
  req->length = 1;
  req->value[0] = SL_CTRL_CMD_PAIRING_SUCCESSFUL;
  KE_MSG_SEND(req);
}

// This callback is called after encrypted connection has been established.
// Inform MCU that connection is secure.
void user_app_on_encrypt_ind_cb(uint8_t conidx, uint8_t auth) {
  app_connection_status = BLE_STATUS_CONNECTED_SECURE;
  uart_task_notify_connection_status(app_connection_status);
}
