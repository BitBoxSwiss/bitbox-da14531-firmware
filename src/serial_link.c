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

#include <crc.h>
#include <uart.h>

#include "debug.h"
#include "serial_link.h"
#include "util.h"

#define SL_SOF 0x7E
#define SL_ESCAPE 0x7D
#define SL_XOR 0x20
#define STX 0x02

enum serial_link_state {
  SERIAL_LINK_WAIT,
  SERIAL_LINK_ACCEPT,
  SERIAL_LINK_ESCAPE,
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
bool serial_link_parse_frame(uint8_t *buf, uint16_t *buf_len, uint8_t *frame,
                             uint16_t *frame_len, uint16_t frame_cap) {
  static enum serial_link_state state = SERIAL_LINK_WAIT;

  for (int i = 0; i < *buf_len && *frame_len < frame_cap; i++) {
    // LOG("i:%d,b:%02x,f:%d\n", i, buf[i], *frame_len);
    switch (state) {
    case SERIAL_LINK_WAIT:
      if (buf[i] == SL_SOF) {
        state = SERIAL_LINK_ACCEPT;
      }
      break;
    case SERIAL_LINK_ACCEPT:
      if (buf[i] == SL_SOF) {
        if (*frame_len >= 3) {

          // Move any bytes we didn't use to the beginning
          memmove(&buf[0], &buf[i + 1], *buf_len - i - 1);
          *buf_len = *buf_len - i - 1;

          return true;
        }
        *frame_len = 0;
      } else if (buf[i] == SL_ESCAPE) {
        state = SERIAL_LINK_ESCAPE;
      } else {
        frame[(*frame_len)++] = buf[i];
      }
      break;
    case SERIAL_LINK_ESCAPE:
      frame[(*frame_len)++] = buf[i] ^ SL_XOR;
      state = SERIAL_LINK_ACCEPT;
      break;
    }
  }
  *buf_len = 0;
  return false;
}

///
enum sl_status serial_link_parse_packet(uint8_t *buf, uint16_t *buf_len,
                                        uint8_t *frame, uint16_t *frame_len,
                                        uint16_t frame_cap) {

  bool frame_complete =
      serial_link_parse_frame(buf, buf_len, frame, frame_len, frame_cap);

  // We ran out of space in frame buffer while reading bytes
  if (*frame_len == frame_cap) {
    ASSERT_ERROR(false);
    return SL_ERR;
  }

  if (frame_complete) {
    // LOG("serial_link_parse_packet, frame len: %d\n", *frame_len);
    uint8_t type = frame[0];
    uint16_t len = frame[1] | frame[2] << 8;
    if (*frame_len != len + 5 || *frame_len < 5) {
      // Invalid length
      return SL_NONE;
    }
    uint16_t crc_frame = frame[3 + len] | frame[3 + len + 1] << 8;

    crc_t crc = crc_init();
    crc = crc_update(crc, &frame[0], 3 + len);
    crc = crc_finalize(crc);

    // LOG("len %d, type %d, crc_frame: %04x, crc: %04x\n", len, cmd,
    //              crc_frame, crc);

    if (crc_frame != crc) {
      LOG("INVALID CRC\n");
      return SL_ERR;
    }

    // TODO: do error correction, bits 7:4 and 3:0 in type are the same but
    // complemented
    switch (type) {
    case SL_PT_ACK:
      return SL_PACKET_TYPE_ACK;
    case SL_PT_NAK:
      return SL_PACKET_TYPE_NAK;
    case SL_PT_BLE_DATA:
      return SL_PACKET_TYPE_BLE_DATA;
    case SL_PT_CTRL_DATA:
      return SL_PACKET_TYPE_CTRL_DATA;
    case SL_PT_PING:
      return SL_PACKET_TYPE_PING;
    }
  }
  return SL_NONE;
}

// We also escape STX so that the MCU can detect if the BLE chip has been
// reset.
static void _serial_link_format_byte(uint8_t data, uint8_t *buf,
                                     uint16_t buf_len, uint16_t *idx) {
  ASSERT_ERROR(*idx + 2 < buf_len);
  switch (data) {
  case SL_SOF:
  case SL_ESCAPE:
  case STX:
    buf[(*idx)++] = SL_ESCAPE;
    buf[(*idx)++] = data ^ SL_XOR;
    break;
  default:
    buf[(*idx)++] = data;
    break;
  }
}

/// Formats a packet into buf for sending over serial
/// Returns number of bytes formatted
uint16_t serial_link_format(uint8_t *buf, uint16_t buf_len, uint8_t typ,
                            const uint8_t *payload, uint16_t payload_len) {
  uint16_t idx = 0;
  crc_t crc = crc_init();

  ASSERT_ERROR(idx + 1 < buf_len);
  buf[idx++] = SL_SOF;

  crc = crc_update(crc, &typ, 1);
  _serial_link_format_byte(typ, buf, buf_len, &idx);

  uint8_t len = payload_len & 0xff;
  crc = crc_update(crc, &len, 1);
  _serial_link_format_byte(len, buf, buf_len, &idx);

  len = (payload_len >> 8) & 0xff;
  crc = crc_update(crc, &len, 1);
  _serial_link_format_byte(len, buf, buf_len, &idx);

  for (int i = 0; i < payload_len; i++) {
    _serial_link_format_byte(payload[i], buf, buf_len, &idx);
  }

  crc = crc_update(crc, &payload[0], payload_len);
  crc = crc_finalize(crc);

  // crc_t is the "fastest" type that holds u16, so can be longer than 2
  // bytes
  for (int i = 0; i < sizeof(uint16_t); i++) {
    _serial_link_format_byte(crc & 0xff, buf, buf_len, &idx);
    crc >>= 8;
  }
  ASSERT_ERROR(idx + 1 < buf_len);
  buf[idx++] = SL_SOF;
  return idx;
}

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

// Read next frame into `buf`.
// At most `buf_len` bytes
// Blocks until complete frame has been received.
static uint16_t sl_next_frame(uint8_t *buf, uint16_t buf_len) {
  uint8_t frame[700];
  uint16_t frame_len = 0;
  uint8_t buf_rd[32];
  uint16_t buf_rd_len = 0;

  while (true) {
    buf_rd_len = _read(&buf_rd[0], sizeof(buf_rd));
    if (buf_rd_len > 0) {
      // LOG("read %d bytes\n", buf_rd_len);
      enum sl_status res = serial_link_parse_packet(
          &buf_rd[0], &buf_rd_len, &frame[0], &frame_len, sizeof(frame));

      switch (res) {
      case SL_PACKET_TYPE_CTRL_DATA: {
        uint16_t len = MIN(buf_len, frame_len - 6);
        memcpy(buf, &frame[4], len);
        return len;
      } break;
      case SL_NONE:
        break;
      default:
        ASSERT_ERROR(false);
      }
    }
  }
  return 0;
}

// Blocking write is meant for small payloads
static void sl_write(uint8_t cmd, uint8_t *payload, uint16_t payload_len) {
  uint8_t buf[10] = {0};
  uint8_t buf_out[20] = {0};

  ASSERT_ERROR(payload_len < sizeof(buf) - 1);
  buf[0] = cmd;
  memcpy(&buf[1], payload, payload_len);
  uint16_t len = serial_link_format(&buf_out[0], sizeof(buf_out),
                                    SL_PT_CTRL_DATA, &buf[0], 1 + payload_len);
  uart_send(UART1, &buf_out[0], len, UART_OP_BLOCKING);
}

uint16_t sl_bond_db_load(uint8_t *bdb, uint16_t bdb_len) {
  sl_write(SL_CTRL_CMD_BOND_DB_GET, NULL, 0);
  return sl_next_frame(bdb, bdb_len);
}

uint16_t sl_identity_address_load(uint8_t *addr, uint16_t addr_len) {
  sl_write(SL_CTRL_CMD_IDENTITY_ADDRESS, NULL, 0);
  return sl_next_frame(addr, addr_len);
}

uint16_t sl_irk_load(uint8_t *irk, uint16_t irk_len) {
  sl_write(SL_CTRL_CMD_IRK, NULL, 0);
  return sl_next_frame(irk, irk_len);
}

uint16_t sl_device_name_load(uint8_t *device_name, uint16_t device_name_len) {
  sl_write(SL_CTRL_CMD_DEVICE_NAME, NULL, 0);
  return sl_next_frame(device_name, device_name_len);
}

/// sl_init registers a simpler rx callback that only handles the objects we
/// fetch during initialization
// void sl_init(void) { uart_register_rx_cb(UART1, sl_rx_cb); }
