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

#ifndef USER_CUSTS1_IMPL
#define USER_CUSTS1_IMPL
#include <custs1.h>
#include <custs1_task.h>
void user_svc1_rx_val_ind_handler(ke_msg_id_t const msgid,
                                  struct custs1_val_write_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

void user_svc1_product_val_cfg_ind_handler(
    ke_msg_id_t const msgid, struct custs1_val_write_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id);

void user_svc1_rest_att_info_req_handler(
    ke_msg_id_t const msgid, struct custs1_att_info_req const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id);
#endif
