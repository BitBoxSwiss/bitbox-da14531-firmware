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

#include <app_easy_security.h>
#include <crc.h>
#include <custs1_task.h>
#include <da1458x_scatter_config.h>
#include <ke_msg.h>
#include <ke_task.h>
#include <prf.h>
#include <rf_531.h>
#include <uart.h>

#include "debug.h"
#include "serial_link.h"
#include "uart_task.h"
#include "user_app.h"
#include "user_custs1_def.h"
#include "util.h"

// Where the Kernel stores some state about our task
ke_state_t uart_state[UART_COUNT_MAX] = {0};

// UART out buffer
// bond_db is 644 long
#define UART_TX_BUF_LEN 700
static uint8_t tx_buf[UART_TX_BUF_LEN] __SECTION_ZERO("retention_mem_area0");

// Read as many bytes as possible from UART1
static uint16_t _read(uint8_t *buf, uint16_t buf_len) {
  uint16_t idx = 0;
  while (idx < buf_len) {
    if (uart_data_ready_getf(UART1)) {
      buf[idx++] = uart_read_rbr(UART1);
    }
    // fifo empty
    else {
      return idx;
    }
  }
  return idx;
}

volatile timer_hnd turn_on_rx_cb_handle = EASY_TIMER_INVALID_TIMER;

// Timer callback to attempt turning on RX interrupts again
void turn_on_rx_cb(void) {
  turn_on_rx_cb_handle = EASY_TIMER_INVALID_TIMER;
  uart_rxdata_intr_setf(UART1, UART_BIT_EN);
}

// UART Receive callback
// As many bytes as possible are read out. If we have a whole frame we forward
// it over bluetooth or to TASK_UART
static void uart_task_rx_cb(uint16_t _data_cnt) {
  static uint8_t buf[64];
  static uint16_t buf_len = 0;
  static uint8_t frame[100];
  static uint16_t frame_len = 0;

  uint16_t read = _read(&buf[buf_len], sizeof(buf) - buf_len);
  buf_len += read;

  ASSERT_ERROR(buf_len <= sizeof(buf));

  if (ke_get_mem_usage(KE_MEM_KE_MSG) > ((__SCT_HEAP_MSG_SIZE * 90) / 100)) {
    // Disable RX interrupts
    uart_rxdata_intr_setf(UART1, UART_BIT_DIS);
    LOG("OOM warning\n");
    if (turn_on_rx_cb_handle == EASY_TIMER_INVALID_TIMER) {
      turn_on_rx_cb_handle = app_easy_timer(10, turn_on_rx_cb);
    }
    return;
  }

  enum sl_status res = serial_link_parse_packet(&buf[0], &buf_len, &frame[0],
                                                &frame_len, sizeof(frame));

  switch (res) {
  case SL_PACKET_TYPE_BLE_DATA: {
    struct custs1_val_ind_req *req = KE_MSG_ALLOC_DYN(
        CUSTS1_VAL_IND_REQ, prf_get_task_from_id(TASK_ID_CUSTS1), TASK_APP,
        custs1_val_ind_req, frame_len - 5);
    req->conidx = app_connection_idx;
    req->handle = SVC1_IDX_TX_VAL;
    req->length = frame_len - 5;
    memcpy(req->value, &frame[3], frame_len - 5);
    KE_MSG_SEND(req);
    frame_len = 0;
  } break;
  case SL_ERR:
    // TODO: Respond with NAK
    break;
  default: {
    struct uart_rx_req *req =
        KE_MSG_ALLOC_DYN(UART_RX, KE_BUILD_ID(TASK_UART, 0), TASK_APP,
                         uart_rx_req, frame_len - 5);
    req->type = frame[0];
    req->length = frame_len - 5;
    memcpy(req->value, &frame[3], frame_len - 5);
    KE_MSG_SEND(req);
    frame_len = 0;
  } break;
  case SL_NONE:
    // Wait for more bytes
    break;
  }
}

// UART Transmit callback
// TX Buffer can be used again
static void uart_task_tx_cb(uint16_t data_cnt) {
  // Changing task state will trigger kernel to serve all saved messages
  KE_MSG_SEND_BASIC(UART_TX_DONE, TASK_UART, TASK_APP);
}

void uart_task_notify_connection_status(uint8_t status) {
  struct uart_tx_req *req = KE_MSG_ALLOC_DYN(UART_TX, KE_BUILD_ID(TASK_UART, 0),
                                             TASK_APP, uart_tx_req, 2);
  req->type = SL_PT_CTRL_DATA;
  req->length = 2;
  req->value[0] = SL_CTRL_CMD_BLE_STATUS;
  req->value[1] = status;
  KE_MSG_SEND(req);
}

// Handle the UART_RX msg for TASK_UART
int uart_task_handler_rx(ke_msg_id_t const msgid, void const *param,
                         ke_task_id_t const dest_id,
                         ke_task_id_t const src_id) {
  // LOG("uart_task_handler_rx_cb\n");
  struct uart_rx_req *msg = (struct uart_rx_req *)param;
  switch (msg->type) {
  case SL_PT_ACK:
    // Handle ACK
    break;
  case SL_PT_NAK:
    // Handle NAK
    break;
  case SL_PT_CTRL_DATA: {
    uint8_t cmd = msg->value[0];
    switch (cmd) {
    case SL_CTRL_CMD_DEVICE_NAME: {
      const uint8_t *device_name = &msg->value[1];
      // In the device_info field we can fit GAP_MAX_NAME_SIZE characters
      uint16_t device_name_len = MIN(msg->length - 1, GAP_MAX_NAME_SIZE);
      //  Update the device name
      memcpy(device_info.dev_name.name, device_name, device_name_len);
      device_info.dev_name.length = device_name_len;

      // In the scan response field we can only fit SCAN_RSP_DATA_LEN-2
      // characters Update the scan response field
      uint8_t scan_rsp_device_name_len =
          MIN(msg->length - 1, SCAN_RSP_DATA_LEN - 2);
      uint8_t scan_rsp_data_len = scan_rsp_device_name_len + 2;
      uint8_t scan_rsp_data[SCAN_RSP_DATA_LEN];
      scan_rsp_data[0] = device_name_len + 1;       // Length
      scan_rsp_data[1] = GAP_AD_TYPE_COMPLETE_NAME; // Type
      memcpy(&scan_rsp_data[2], device_name,
             MIN(device_name_len, SCAN_RSP_DATA_LEN - 2));
      user_app_set_scan_rsp_data(&scan_rsp_data[0], scan_rsp_data_len);

    } break;
    case SL_CTRL_CMD_PRODUCT_STRING: {
      const uint8_t *product = &msg->value[1];
      uint16_t product_len = msg->length - 1;
      if (app_connection_idx != GAP_INVALID_CONIDX) {
        struct custs1_val_ind_req *req = KE_MSG_ALLOC_DYN(
            CUSTS1_VAL_IND_REQ, prf_get_task_from_id(TASK_ID_CUSTS1), TASK_APP,
            custs1_val_ind_req, product_len);
        req->conidx = app_connection_idx;
        req->handle = SVC1_IDX_PRODUCT_VAL;
        req->length = product_len;
        memcpy(req->value, product, product_len);
        KE_MSG_SEND(req);
      }
    } break;
    case SL_CTRL_CMD_BLE_CHIP_RESET: {
      // Remap address 0 to ROM and reset
      SetWord16(SYS_CTRL_REG,
                (GetWord16(SYS_CTRL_REG) & ~REMAP_ADR0) | SW_RESET);
    } break;
    case SL_CTRL_CMD_TK_CONFIRM: {
      if (msg->length != KEY_LEN + 2) {
        LOG("invalid length %d\n", msg->length);
        break;
      }
      uint8_t *key = &msg->value[1];
      uint8_t accept = msg->value[1 + KEY_LEN];
      LOG("accept %d\n", accept);
      app_easy_security_tk_exch(app_connection_idx, key, KEY_LEN, accept);
    } break;
    case SL_CTRL_CMD_BLE_STATUS: {
      if (msg->length != 1) {
        LOG("invalid length");
        break;
      }
      uart_task_notify_connection_status(app_connection_status);
    } break;
    case SL_CTRL_CMD_BLE_ENABLED: {
      if (msg->length != 2) {
        LOG("invalid length");
        break;
      }
      if (msg->value[1] == 0) {
        debug_uart("ble disabled");
        shutting_down = true;
        if (app_connection_idx != GAP_INVALID_CONIDX) {
          debug_uart("app_easy_gap_disconnect");
          app_easy_gap_disconnect(app_connection_idx);
        } else {
          app_easy_gap_advertise_stop();
        }
      }
      // BLE Enabled isn't implemented, would need to wake up the chip
    } break;
    case SL_CTRL_CMD_BLE_PWR_LEVEL: {
      if (msg->length != 2) {
        LOG("invalid length");
        break;
      }
      if (msg->value[1] > 0 && msg->value[1] <= 12) {
        debug_uart("ble level update");
        rf_pa_pwr_adv_set(msg->value[1]);
      }
    } break;
    default:
      break;
    }
  } break;
  case SL_PT_PING:
    // Handle ping
    break;
  case SL_PT_BLE_DATA:
    // BLE data should've been sent to TASK_APP from the rx callback
    ASSERT_ERROR(false);
  default:
    break;
  }
  return (KE_MSG_CONSUMED);
}

// Handle the UART_TX msg for TASK_UART
int uart_task_handler_tx(ke_msg_id_t const msgid, void const *param,
                         ke_task_id_t const dest_id,
                         ke_task_id_t const src_id) {

  ke_state_set(TASK_UART, UART_TX_BUSY);

  struct uart_tx_req const *req = (struct uart_tx_req const *)param;

  int16_t write_offset = 0;

  switch (req->type) {
  case SL_PT_CTRL_DATA: {
    write_offset +=
        serial_link_format(&tx_buf[write_offset], sizeof(tx_buf) - write_offset,
                           req->type, &req->value[0], req->length);

  } break;
  case SL_PT_BLE_DATA: {
    // We only generate messages that are multiples of 64 bytes long (see
    // user_custs1_impl.c). Format every 64 byte packet as separate frames.
    uint16_t read_offset = 0;
    while (read_offset < req->length) {
      write_offset += serial_link_format(
          &tx_buf[write_offset], sizeof(tx_buf) - write_offset, req->type,
          &req->value[read_offset], 64);
      read_offset += 64;
    }
  } break;
  default:
    LOG("uart_task: Error unexpected req->type");
    break;
  }

  uart_send(UART1, &tx_buf[0], write_offset, UART_OP_INTR);

  return (KE_MSG_CONSUMED);
};

int uart_task_handler_tx_done(ke_msg_id_t const msgid, void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id) {
  ke_state_set(TASK_UART, UART_TX_READY);
  return KE_MSG_CONSUMED;
}
const struct ke_msg_handler uart_default_state[] = {
    {UART_TX, uart_task_handler_tx},
    {UART_TX_DONE, uart_task_handler_tx_done},
    {UART_RX, uart_task_handler_rx},
};

const struct ke_msg_handler uart_tx_busy_state[] = {
    {UART_TX, ke_msg_save},
    {UART_TX_DONE, uart_task_handler_tx_done},
    {UART_RX, uart_task_handler_rx},
};

const struct ke_state_handler uart_default_handler =
    KE_STATE_HANDLER(uart_default_state);

const struct ke_state_handler uart_state_handler[UART_STATE_MAX] = {
    [UART_DISABLED] = KE_STATE_HANDLER_NONE,
    [UART_TX_READY] = KE_STATE_HANDLER_NONE,
    [UART_TX_BUSY] = KE_STATE_HANDLER(uart_tx_busy_state),
};

static const struct ke_task_desc TASK_DESC_UART = {
    uart_state_handler, &uart_default_handler, uart_state, UART_STATE_MAX,
    UART_COUNT_MAX};

void uart_task_init(void) {
  ke_task_create(TASK_UART, &TASK_DESC_UART);
  ke_state_set(TASK_UART, UART_DISABLED);
}

void uart_task_enable(void) {
  uart_register_rx_cb(UART1, uart_task_rx_cb);
  uart_register_tx_cb(UART1, uart_task_tx_cb);

  // Enable uart receive interrupts
  uart_receive(UART1, NULL, 1, UART_OP_INTR);

  ke_state_set(TASK_UART, UART_TX_READY);
}

#if !NDEBUG
void debug_uart(const char *msg) {
  int len = strlen(msg);
  struct uart_tx_req *req = KE_MSG_ALLOC_DYN(UART_TX, KE_BUILD_ID(TASK_UART, 0),
                                             TASK_APP, uart_tx_req, len + 1);
  req->type = SL_PT_CTRL_DATA;
  req->length = len + 1;
  req->value[0] = SL_CTRL_CMD_DEBUG_STR;
  memcpy(&req->value[1], msg, len);
  KE_MSG_SEND(req);
}
#endif
