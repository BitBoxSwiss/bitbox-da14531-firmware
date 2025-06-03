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

#ifndef SERIAL_LINK_H
#define SERIAL_LINK_H

// Control commands
#define SL_CTRL_CMD_DEVICE_NAME 1
#define SL_CTRL_CMD_BOND_DB_GET 2
#define SL_CTRL_CMD_BOND_DB_SET 3
#define SL_CTRL_CMD_PAIRING_CODE 4
#define SL_CTRL_CMD_BLE_STATUS 5
#define SL_CTRL_CMD_IRK 6
#define SL_CTRL_CMD_PRODUCT_STRING 7
#define SL_CTRL_CMD_BLE_CHIP_RESET 8
#define SL_CTRL_CMD_IDENTITY_ADDRESS 9
#define SL_CTRL_CMD_PAIRING_SUCCESSFUL 10
#define SL_CTRL_CMD_TK_CONFIRM 11
#define SL_CTRL_CMD_BLE_ENABLED 12
#define SL_CTRL_CMD_BLE_PWR_LEVEL 13
#define SL_CTRL_CMD_DEBUG_STR 254

#define BLE_STATUS_ADVERTISING 0
#define BLE_STATUS_CONNECTED 1
#define BLE_STATUS_CONNECTED_SECURE 2

// Packet types
enum packet_type {
  SL_PT_ACK = 0b00101101,
  SL_PT_NAK = 0b01011010,
  SL_PT_BLE_DATA = 0b00111100,
  SL_PT_CTRL_DATA = 0b10110100,
  SL_PT_PING = 0b01001011,
};

/// Formats a packet into buf for sending over serial
/// Returns number of bytes formatted
uint16_t serial_link_format(uint8_t *buf, uint16_t buf_len, uint8_t typ,
                            const uint8_t *payload, uint16_t payload_len);

// Result type for serial_link_parse_packet
enum sl_status {
  SL_NONE,
  SL_ERR,
  SL_PACKET_TYPE_ACK,
  SL_PACKET_TYPE_NAK,
  SL_PACKET_TYPE_BLE_DATA,
  SL_PACKET_TYPE_CTRL_DATA,
  SL_PACKET_TYPE_PING,
};

/// Will read out the next frame from the stream. Returns true if there is
/// a complete frame in the output, or false in case it needs more bytes.
///
/// buf - Buffer to read from, there might be unread bytes in case there was
///       more than one frame in the buffer
/// buf_len - length of content in buffer. Will be updated in case there are
///           unread bytes
/// frame - Buffer to write to
/// frame_len - how many bytes were written to `frame` buffer
/// frame_cap - capacity of `frame`. how many bytes long the buffer is
///
/// if it returns PACKET_TYPE_ERR send a NAK back
///
enum sl_status serial_link_parse_packet(uint8_t *buf, uint16_t *buf_len,
                                        uint8_t *frame, uint16_t *frame_len,
                                        uint16_t frame_cap);

// Blocking load of bond_db
uint16_t sl_bond_db_load(uint8_t *bdb, uint16_t bdb_len);
// Blocking load of IRK
uint16_t sl_irk_load(uint8_t *irk, uint16_t irk_len);
// Blocking load of identity address
uint16_t sl_identity_address_load(uint8_t *addr, uint16_t addr_len);
// Blocking load of device name
uint16_t sl_device_name_load(uint8_t *device_name, uint16_t device_name_len);

#endif
