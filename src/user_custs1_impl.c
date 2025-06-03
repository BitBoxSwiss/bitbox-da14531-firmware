/**
 ****************************************************************************************
 *
 * @file user_custs1_impl.c
 *
 * @brief Peripheral project Custom1 Server implementation source code.
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_custs1_impl.h"
#include "app.h"
#include "custs1_task.h"
#include "uart_task.h"
#include <debug.h>

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

ke_msg_id_t
    timer_used __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

// Handler for when BL Central writes to the RX char, forward the
// packet to UART
void user_svc1_rx_val_ind_handler(ke_msg_id_t const msgid,
                                  struct custs1_val_write_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
  // Only accept multiples of 64
  if (param->length % 64 != 0) {
    return;
  }
  struct uart_tx_req *req = KE_MSG_ALLOC_DYN(
      UART_TX, KE_BUILD_ID(TASK_UART, 0), src_id, uart_tx_req, param->length);
  req->type = SL_PT_BLE_DATA;
  req->length = param->length;
  memcpy(&req->value[0], &param->value[0], param->length);
  KE_MSG_SEND(req);
}

// Handler for when BL Central subscribes to PRODUCT char, request product
// string from MCU over UART
void user_svc1_product_val_cfg_ind_handler(
    ke_msg_id_t const msgid, struct custs1_val_write_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  // Generate indication when the central subscribes to it
  if (param->value[0]) {
    struct uart_tx_req *req = KE_MSG_ALLOC_DYN(
        UART_TX, KE_BUILD_ID(TASK_UART, 0), TASK_APP, uart_tx_req, 1);
    req->type = SL_PT_CTRL_DATA;
    req->length = 1;
    req->value[0] = SL_CTRL_CMD_PRODUCT_STRING;
    KE_MSG_SEND(req);
  }
}

// Catch-all handler?
void user_svc1_rest_att_info_req_handler(
    ke_msg_id_t const msgid, struct custs1_att_info_req const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  struct custs1_att_info_rsp *rsp =
      KE_MSG_ALLOC(CUSTS1_ATT_INFO_RSP, src_id, dest_id, custs1_att_info_rsp);
  // Provide the connection index.
  rsp->conidx = app_env[param->conidx].conidx;
  // Provide the attribute index.
  rsp->att_idx = param->att_idx;
  // Force current length to zero.
  rsp->length = 0;
  // Provide the ATT error code.
  rsp->status = ATT_ERR_WRITE_NOT_PERMITTED;

  KE_MSG_SEND(rsp);
}
