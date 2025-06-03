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

#ifndef USER_APP_H_
#define USER_APP_H_

#include <stdint.h>

#include "gapc_task.h" // gap functions and messages
#include <arch.h>
#include <arch_api.h>
#include <ke_msg.h>

void user_app_on_init_cb(void);
void user_default_operation_adv_cb(void);
void user_app_on_connection_cb(uint8_t conidx,
                               struct gapc_connection_req_ind const *param);
void user_app_adv_undirect_complete_cb(uint8_t status);
void user_app_on_disconnect_cb(struct gapc_disconnect_ind const *param);
void user_app_on_tk_exch_cb(uint8_t conidx,
                            struct gapc_bond_req_ind const *param);
void user_app_on_pairing_request_cb(uint8_t conidx,
                                    struct gapc_bond_req_ind const *param);
void user_app_on_pairing_succeeded_cb(uint8_t conidx);

// Set the scan response data, data_len can be at most 31 bytes
void user_app_set_scan_rsp_data(uint8_t *data, uint8_t data_len);

void user_app_process_catch_rest_cb(ke_msg_id_t const msgid, void const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id);

void user_app_on_encrypt_ind_cb(uint8_t conidx, uint8_t auth);
void user_app_on_get_dev_name(struct app_device_name *device_name);

extern uint8_t app_connection_idx;
extern uint8_t app_connection_status;

extern uint8_t scan_rsp_data[SCAN_RSP_DATA_LEN];
extern uint8_t scan_rsp_data_len;
extern bool shutting_down;

extern struct app_device_name private_device_name;

#endif // USER_APP_H_
