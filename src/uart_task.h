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

#ifndef UART_TASK_H
#define UART_TASK_H

#include <ke_msg.h>
#include <rwip_config.h>

#include "serial_link.h"

// Let's steal "RFU_3"
#define TASK_UART TASK_RFU_3

// There can be only one
#define UART_COUNT_MAX 1

extern ke_state_t uart_state[UART_COUNT_MAX];

enum uart_state {
  // UART initial state, disabled
  UART_DISABLED,
  // UART enabled
  UART_TX_READY,
  // UART busy
  UART_TX_BUSY,
  // Number of defined states
  UART_STATE_MAX,
};

enum uart_msg_id {
  UART_TX = KE_FIRST_MSG(TASK_UART), // There is data to send
  UART_TX_DONE,                      // TX is done
  UART_RX,                           // There is data to be received
};

struct uart_rx_req {
  enum packet_type type;
  uint16_t length;
  uint8_t value[__ARRAY_EMPTY];
};

struct uart_tx_req {
  enum packet_type type;
  uint16_t length;
  uint8_t value[__ARRAY_EMPTY];
};

void uart_task_init(void);
void uart_task_enable(void);
#if !defined(NDEBUG)
void debug_uart(const char *msg);
#else
#define debug_uart(...) (void)0
#endif

void uart_task_notify_connection_status(uint8_t status);

#endif
