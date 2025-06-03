/**
 ****************************************************************************************
 *
 * @file user_custs1_def.h
 *
 * @brief Custom Server 1 (CUSTS1) profile database definitions.
 *
 * Copyright (c) 2016-2019 Dialog Semiconductor. All rights reserved.
 *
 * This software ("Software") is owned by Dialog Semiconductor.
 *
 * By using this Software you agree that Dialog Semiconductor retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Dialog Semiconductor is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Dialog Semiconductor products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * DIALOG SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
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

#ifndef _USER_CUSTS1_DEF_H_
#define _USER_CUSTS1_DEF_H_

/**
 ****************************************************************************************
 * @defgroup USER_CONFIG
 * @ingroup USER
 * @brief Custom Server 1 (CUSTS1) profile database definitions.
 *
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

// Service 1 of the custom server 1
// E1511A45-F3DB-44C0-82B8-6C880790D1F1
#define DEF_SVC1_UUID_128                                                      \
  {0xF1, 0xD1, 0x90, 0x07, 0x88, 0x6C, 0xB8, 0x82,                             \
   0xC0, 0x44, 0xDB, 0xF3, 0x45, 0x1A, 0x51, 0xE1}

// Characteristic RX of Service 1
// 799d485c-d354-4ed0-b577-f8ee79ec275a
#define DEF_SVC1_RX_UUID_128                                                   \
  {0x5a, 0x27, 0xec, 0x79, 0xee, 0xf8, 0x77, 0xb5,                             \
   0xd0, 0x4e, 0x54, 0xd3, 0x5c, 0x48, 0x9d, 0x79}
#define DEF_SVC1_RX_CHAR_LEN 320
#define DEF_SVC1_RX_USER_DESC "RX"

// Characteristic TX of Service 1
// 419572a5-9f53-4eb1-8db7-61bcab928867
#define DEF_SVC1_TX_UUID_128                                                   \
  {0x67, 0x88, 0x92, 0xab, 0xbc, 0x61, 0xb7, 0x8d,                             \
   0xb1, 0x4e, 0x53, 0x9f, 0xa5, 0x72, 0x95, 0x41}
#define DEF_SVC1_TX_CHAR_LEN 64
#define DEF_SVC1_TX_USER_DESC "TX"

// Characteristic PRODUCT of Service 1
// 9d1c9a77-8b03-4e49-8053-3955cda7da93
#define DEF_SVC1_PRODUCT_UUID_128                                              \
  {0x93, 0xda, 0xa7, 0xcd, 0x55, 0x39, 0x53, 0x80,                             \
   0x49, 0x4e, 0x03, 0x8b, 0x77, 0x9a, 0x1c, 0x9d}
#define DEF_SVC1_PRODUCT_CHAR_LEN 0
#define DEF_SVC1_PRODUCT_USER_DESC "PRODUCT"

/// Custom1 Service Data Base Characteristic enum
enum {
  // Custom Service 1
  SVC1_IDX_SVC = 0,

  SVC1_IDX_RX_CHAR,
  SVC1_IDX_RX_VAL,
  SVC1_IDX_RX_USER_DESC,

  SVC1_IDX_TX_CHAR,
  SVC1_IDX_TX_VAL,
  SVC1_IDX_TX_IND_CFG,
  SVC1_IDX_TX_USER_DESC,

  SVC1_IDX_PRODUCT_CHAR,
  SVC1_IDX_PRODUCT_VAL,
  SVC1_IDX_PRODUCT_IND_CFG,
  SVC1_IDX_PRODUCT_USER_DESC,

  CUSTS1_IDX_NB
};

/// @} USER_CONFIG

#endif // _USER_CUSTS1_DEF_H_
