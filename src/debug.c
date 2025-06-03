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

#include <debug.h>

#include <da1458x_config_advanced.h>
#include <da1458x_config_basic.h>
#include <da1458x_scatter_config.h>
#include <ke_mem.h>
#include <rwip_config.h>

static void printMemory(const char *name, uint8_t type, uint16_t size) {
#if !defined(NDEBUG)
  uint16_t usage = ke_get_mem_usage(type);
#endif
  LOG("%s = %d / %d (%d%%)\n", name, usage, size,
      (unsigned int)(usage * 100) / size);
}

void Debug_PrintMemoryUsage(void) {
  LOG_S("Mem usage:\n");
  printMemory("ENV", KE_MEM_ENV, __SCT_HEAP_ENV_SIZE);
  printMemory("ATT_DB", KE_MEM_ATT_DB, __SCT_HEAP_DB_SIZE);
  printMemory("KE_MSG", KE_MEM_KE_MSG, __SCT_HEAP_MSG_SIZE);
  printMemory("NON_RET", KE_MEM_NON_RETENTION, __SCT_HEAP_NON_RET_SIZE);
  LOG("MAX = %u\n", (unsigned int)ke_get_max_mem_usage());
}

void log_x(const char *prefix, const uint8_t *buf, uint16_t buf_len) {
  LOG_S(prefix);
  for (int i = 0; i < buf_len; i++) {
    LOG("%02x", buf[i]);
  }
  LOG_S("\n");
  LOG_S(prefix);
  for (int i = 0; i < buf_len; i++) {
    if (buf[i] >= ' ') {
      // print like this to align with byte above
      LOG(" %c", buf[i]);
    } else {
      LOG_S("??");
    }
  }
  LOG_S("\n");
}
