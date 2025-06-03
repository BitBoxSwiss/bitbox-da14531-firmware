/**
 ****************************************************************************************
 *
 * @file user_config.h
 *
 * @brief User configuration file.
 *
 * Copyright (C) 2015-2023 Renesas Electronics Corporation and/or its
 *affiliates. All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation
 *and/or its affiliates ("Renesas"). Renesas grants you a personal,
 *non-exclusive, non-transferable, revocable, non-sub-licensable right and
 *license to use the Software, solely if used in or together with Renesas
 *products. You may make copies of this Software, provided this copyright notice
 *and disclaimer ("Notice") is included in all such copies. Renesas reserves the
 *right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY
 *KIND, WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE
 *WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NON-INFRINGEMENT. TO THE MAXIMUM EXTENT PERMITTED UNDER LAW, IN NO EVENT SHALL
 *RENESAS BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR
 *CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE,
 *EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. USE OF
 *THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS CONTAINED IN AN
 *ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. IN CASE OF CONFLICT BETWEEN THE
 *TERMS OF THIS NOTICE AND ANY SUCH ADDITIONAL LICENSE AGREEMENT, THE TERMS OF
 *THE AGREEMENT SHALL TAKE PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU
 *AGREE TO THE TERMS OF THIS NOTICE.IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE
 *NOT PERMITTED TO USE THIS SOFTWARE.
 *
 ****************************************************************************************
 */

#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_adv_data.h"
#include "app_default_handlers.h"
#include "app_user_config.h"
#include "arch_api.h"
#include "co_bt.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define USER_CFG_ADDRESS_MODE APP_CFG_CNTL_PRIV_RPA_RAND
#define USER_CFG_CNTL_PRIV_MODE APP_CFG_CNTL_PRIV_MODE_NETWORK
#define USER_CFG_FEAT_IO_CAP GAP_IO_CAP_DISPLAY_YES_NO
#define USER_CFG_FEAT_OOB GAP_OOB_AUTH_DATA_NOT_PRESENT
#define USER_CFG_FEAT_AUTH_REQ (GAP_AUTH_BOND | GAP_AUTH_MITM | GAP_AUTH_SEC)
#define USER_CFG_FEAT_KEY_SIZE KEY_LEN
#define USER_CFG_FEAT_SEC_REQ GAP_SEC1_SEC_PAIR_ENC
#define USER_CFG_FEAT_INIT_KDIST                                               \
  (GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_SIGNKEY)
#define USER_CFG_FEAT_RESP_KDIST                                               \
  (GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_SIGNKEY)
static const sleep_state_t app_default_sleep_mode = ARCH_SLEEP_OFF;

extern uint8_t irk[KEY_LEN];

/*
 ****************************************************************************************
 *
 * Advertising configuration
 *
 ****************************************************************************************
 */
static const struct advertise_configuration user_adv_conf = {
    .addr_src = APP_CFG_ADDR_SRC(USER_CFG_ADDRESS_MODE),
    .intv_min = MS_TO_BLESLOTS(20),  // default: 687.5ms
    .intv_max = MS_TO_BLESLOTS(100), // default: 687.5ms
    .channel_map = ADV_ALL_CHNLS_EN,
    .mode = GAP_GEN_DISCOVERABLE,
    .adv_filt_policy = ADV_ALLOW_SCAN_ANY_CON_ANY,

    /// Address of peer device
    /// NOTE: Meant for directed advertising (ADV_DIRECT_IND)
    .peer_addr = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6},

    /// Address type of peer device (0=public/1=random)
    /// NOTE: Meant for directed advertising (ADV_DIRECT_IND)
    .peer_addr_type = 0,
};

#define USER_ADVERTISE_DATA                                                    \
  ("\x11" ADV_TYPE_COMPLETE_LIST_128BIT_SERVICE_IDS                            \
   "\xF1\xD1\x90\x07\x88\x6C\xB8\x82\xC0\x44\xDB\xF3\x45\x1A\x51\xE1")

/// Advertising data length - maximum 28 bytes, 3 bytes are reserved to set
#define USER_ADVERTISE_DATA_LEN (sizeof(USER_ADVERTISE_DATA) - 1)

/// Scan response data
#define USER_ADVERTISE_SCAN_RESPONSE_DATA ""

/// Scan response data length- maximum 31 bytes
#define USER_ADVERTISE_SCAN_RESPONSE_DATA_LEN                                  \
  (sizeof(USER_ADVERTISE_SCAN_RESPONSE_DATA) - 1)

/*
 ****************************************************************************************
 *
 * Device name.
 *
 * - If there is space left in the advertising or scan response data the device
 *name is copied there. The device name can be anytime read by a connected peer,
 *if the application supports it.
 * - The Bluetooth device name can be up to 248 bytes.
 *
 ****************************************************************************************
 */
/// Device name
// #define USER_DEVICE_NAME "BitBox02+"
#define USER_DEVICE_NAME ""

/// Device name length
// #define USER_DEVICE_NAME_LEN (sizeof(USER_DEVICE_NAME) - 1)
#define USER_DEVICE_NAME_LEN 0

/*
 ****************************************************************************************
 *
 * GAPM configuration
 *
 ****************************************************************************************
 */
static const struct gapm_configuration user_gapm_conf = {
    .role = GAP_ROLE_PERIPHERAL,
    .max_mtu = 509,
    .addr_type = APP_CFG_ADDR_TYPE(USER_CFG_ADDRESS_MODE),
    .renew_dur = 60000, // 60000 * 10ms = 600s (150s is the minimum value)
    .addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .irk = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
            0x0b, 0x0c, 0x0d, 0x0e, 0x0f},

    /****************************
     * ATT database configuration
     ****************************
     */

    /// Attribute database configuration (@see enum gapm_att_cfg_flag)
    ///    7     6    5     4     3    2    1    0
    /// +-----+-----+----+-----+-----+----+----+----+
    /// | DBG | RFU | SC | PCP | APP_PERM |NAME_PERM|
    /// +-----+-----+----+-----+-----+----+----+----+
    /// - Bit [0-1]: Device Name write permission requirements for peer
    /// device
    /// (@see device_name_write_perm)
    /// - Bit [2-3]: Device Appearance write permission requirements for
    /// peer
    /// device (@see device_appearance_write_perm)
    /// - Bit [4]  : Slave Preferred Connection Parameters present
    /// - Bit [5]  : Service change feature present in GATT attribute
    /// database.
    /// - Bit [6]  : Reserved
    /// - Bit [7]  : Enable Debug Mode
    // TODO(nd): REMOVE SC? We will never change the services
    .att_cfg = GAPM_MASK_ATT_SVC_CHG_EN,
    .gap_start_hdl = 0,
    .gatt_start_hdl = 0,
    .max_mps = 0,
    .max_txoctets = 251,
    .max_txtime = 2120,
};

/*
 ****************************************************************************************
 *
 * Parameter update configuration
 *
 ****************************************************************************************
 */
static const struct connection_param_configuration user_connection_param_conf =
    {
        .intv_min = MS_TO_DOUBLESLOTS(7.5),
        .intv_max = MS_TO_DOUBLESLOTS(15),
        .latency = 0,
        .time_out = MS_TO_TIMERUNITS(300),
        .ce_len_min = MS_TO_DOUBLESLOTS(0),
        .ce_len_max = MS_TO_DOUBLESLOTS(0),
};

/*
 ****************************************************************************************
 *
 * Default handlers configuration (applies only for @app_default_handlers.c)
 *
 ****************************************************************************************
 */
static const struct default_handlers_configuration user_default_hnd_conf = {
    .adv_scenario = DEF_ADV_FOREVER,
    .advertise_period = MS_TO_TIMERUNITS(180000),
    .security_request_scenario = DEF_SEC_REQ_NEVER,
    //.security_request_scenario = DEF_SEC_REQ_ON_CONNECT,
};

/*
 ****************************************************************************************
 *
 * Security related configuration
 *
 ****************************************************************************************
 */
#if defined(BLE_APP_SEC)
static const struct security_configuration user_security_conf = {
    .iocap = USER_CFG_FEAT_IO_CAP,
    .oob = USER_CFG_FEAT_OOB,
    .auth = USER_CFG_FEAT_AUTH_REQ,
    .key_size = USER_CFG_FEAT_KEY_SIZE,
    .ikey_dist = USER_CFG_FEAT_INIT_KDIST,
    .rkey_dist = USER_CFG_FEAT_RESP_KDIST,
    .sec_req = USER_CFG_FEAT_SEC_REQ,
};
#endif

#endif // _USER_CONFIG_H_
