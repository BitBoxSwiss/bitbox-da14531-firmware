/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
 *
 * Copyright (c) 2015-2019 Dialog Semiconductor. All rights reserved.
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

#include <datasheet.h>
#include <gpio.h>
#include <rwip_config.h>
#include <syscntl.h>
#include <system_library.h>
#include <uart.h>
#include <user_periph_setup.h>

#include "debug.h"

// Configuration struct for UART
static const uart_cfg_t uart_cfg = {
    .baud_rate = UART_BAUDRATE_115200,
    .data_bits = UART_DATABITS_8,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOPBITS_1,
    .auto_flow_control = UART_AFCE_EN,
    .use_fifo = UART_FIFO_EN,
    .tx_fifo_tr_lvl = UART_TX_FIFO_LEVEL_0,
    .rx_fifo_tr_lvl = UART_RX_FIFO_LEVEL_3,
    .intr_priority = 2,
#if defined(CFG_UART_DMA_SUPPORT)
    // Set UART DMA Channel Pair Configuration
    .uart_dma_channel = UART_DMA_CHANNEL_01,
    // Set UART DMA Priority
    .uart_dma_priority = DMA_PRIO_0,
#endif
};

#if DEVELOPMENT_DEBUG
void GPIO_reservations(void) {}
#endif

static void set_pad_functions(void) {
  GPIO_ConfigurePin(UART1_TX_PORT, UART1_TX_PIN, OUTPUT, PID_UART1_TX, false);
  GPIO_ConfigurePin(UART1_RX_PORT, UART1_RX_PIN, INPUT_PULLUP, PID_UART1_RX,
                    false);

  GPIO_ConfigurePin(UART1_CTS_PORT, UART1_CTS_PIN, INPUT_PULLUP, PID_UART1_CTSN,
                    false);
  GPIO_ConfigurePin(UART1_RTS_PORT, UART1_RTS_PIN, OUTPUT, PID_UART1_RTSN,
                    true);
}

void periph_init(void) {
  // Disable HW RST on P0_0
  GPIO_Disable_HW_Reset();

  // TODO(nd): Figure out if this is true?
  // In Boost mode enable the DCDC converter to supply VBAT_HIGH for the used
  // GPIOs Assumption: The connected external peripheral is powered by 3V
  syscntl_dcdc_turn_on_in_boost(SYSCNTL_DCDC_LEVEL_3V0);

  // ROM patch
  patch_func();

  // Set pad functionality
  set_pad_functions();

  // Initialize uart
  uart_initialize(UART1, &uart_cfg);

  // Enable the pads
  GPIO_set_pad_latch_en(true);

#ifdef DEBUG_SEGGER
  // Set up JLink RTT
  SEGGER_RTT_Init();
  LOG("periph_init()\n");
#endif
}
