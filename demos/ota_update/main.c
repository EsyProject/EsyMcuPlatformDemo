/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "demo_config.h"
#include "demo_security.h"
#include "esy_common/utils/integrity/esy_sha256.h"
#include "esy_hal/esy_hal_system.h"
#include "esy_hal/esy_hal_uart.h"
#include "esy_middleware/bootloader/esy_bootloader.h"
#include "esy_middleware/os/esy_os.h"
#include "esy_middleware/os/esy_os_task.h"
#include "esy_middleware/ota/esy_ota_flash.h"

enum {
  REPLY_READY = 'R',
  REPLY_ACK = 'A',
  REPLY_DONE = 'D',
  REPLY_ERROR = 'E',
};

typedef struct {
  esy_uart_t uart;
  esy_ota_flash_t writer;
  esy_ota_t ota;
  esy_os_task_t *task;
  esy_os_task_static_storage_t task_control_storage;
  _Alignas(ESY_OS_TASK_STACK_ALIGN)
      uint8_t task_stack_buffer[DEMO_OTA_TASK_STACK_BYTES];
  uint8_t cache_buffer[DEMO_OTA_CACHE_BYTES];
  uint8_t header[DEMO_OTA_HEADER_BYTES];
  uint8_t chunk[DEMO_OTA_CHUNK_BYTES];
  uint8_t last_chunk[DEMO_OTA_CHUNK_BYTES];
  esy_image_manifest_t candidate;
  uint32_t session_sequence;
  esy_err_t confirmation_result;
  int running_slot;
} app_ctx_t;

/* Single owner after scheduler start. The OTA task keeps running during an
 * update because it owns UART, the writer and all Flash/metadata operations.
 * There are no other application tasks, DMA users, outputs or watchdogs.
 * The sender waits after every 256-byte chunk, including Flash erase stalls. */
static app_ctx_t s_ctx;

static uint32_t read_le32(const uint8_t *bytes) {
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void write_le32(uint8_t *bytes, uint32_t value) {
  bytes[0] = (uint8_t)value;
  bytes[1] = (uint8_t)(value >> 8);
  bytes[2] = (uint8_t)(value >> 16);
  bytes[3] = (uint8_t)(value >> 24);
}

static bool send_reply(uint8_t reply) {
  return esy_hal_uart_send_blocking(&s_ctx.uart, &reply, 1U) == 1;
}

static void drain_uart_before_reset(void) {
  const uint32_t started = esy_hal_system_get_tick();
  /* Some providers return after queuing the last byte in the UART FIFO.
   * Give DONE time to leave the wire; a failed drain still requires reboot
   * because activation may already have published the trial image. */
  while (esy_hal_uart_is_busy(&s_ctx.uart, true) &&
         (uint32_t)(esy_hal_system_get_tick() - started) <
             DEMO_UART_TX_DRAIN_TIMEOUT_MS) {
  }
}

/* The public UART receive API has no timeout argument. Polling before
 * each one-byte receive avoids indefinite waits during an incomplete frame.
 * Do not insert a task delay while receiving: STM32 USART1 has no RX FIFO.
 * Only this task reads UART and it performs no Flash work until the complete
 * chunk has arrived. The inter-byte deadline does not include Flash stalls. */
static bool receive_bytes(uint8_t *data, size_t length, bool wait_first_byte) {
  uint32_t last_progress = esy_hal_system_get_tick();
  size_t offset = 0U;
  while (offset < length) {
    if (esy_hal_uart_is_readable(&s_ctx.uart)) {
      if (esy_hal_uart_receive_blocking(&s_ctx.uart, &data[offset], 1U) != 1) {
        return false;
      }
      ++offset;
      last_progress = esy_hal_system_get_tick();
    } else if ((!wait_first_byte || offset != 0U) &&
               (uint32_t)(esy_hal_system_get_tick() - last_progress) >=
                   DEMO_OTA_RX_GAP_TIMEOUT_MS) {
      return false;
    }
  }
  return true;
}

static void resynchronize_uart(void) {
  uint8_t discarded;
  const uint32_t started = esy_hal_system_get_tick();
  uint32_t last_rx = started;
  while ((uint32_t)(esy_hal_system_get_tick() - last_rx) <
             DEMO_OTA_RESYNC_QUIET_MS &&
         (uint32_t)(esy_hal_system_get_tick() - started) <
             DEMO_OTA_RX_GAP_TIMEOUT_MS) {
    if (esy_hal_uart_is_readable(&s_ctx.uart)) {
      if (esy_hal_uart_receive_blocking(&s_ctx.uart, &discarded, 1U) != 1) {
        /* A native overrun/framing error needs a fresh peripheral lifecycle. */
        break;
      }
      last_rx = esy_hal_system_get_tick();
    }
  }
}

static bool configure_uart(void) {
  esy_uart_config_t config;
  esy_hal_uart_default_cfg(&config);
  config.phys_index = DEMO_UART_PHYS_INDEX;
  config.baudrate = DEMO_UART_BAUDRATE;
  config.data_bits = 8U;
  config.stop_bits = 1U;
  config.parity = ESY_UART_PARITY_NONE;
  return esy_hal_uart_init(&s_ctx.uart, &config) == ESY_OK;
}

static esy_err_t new_session_id(void *context, uint32_t *session_id) {
  uint32_t *sequence = context;
  if (sequence == NULL || session_id == NULL) return ESY_ERR_INVALID_ARG;
  /* IDs are internal to this single-owner transport, never taken from input.
   * Refuse exhaustion rather than reuse an ID during this boot. */
  if (*sequence == UINT32_MAX) return ESY_ERR_STATE;
  *session_id = ++*sequence;
  return ESY_OK;
}

static esy_err_t initialize_update(void) {
  const esy_ota_flash_static_resources_t flash_resources = {
      .cache_buffer = s_ctx.cache_buffer,
      .cache_size = sizeof(s_ctx.cache_buffer),
  };
  const esy_ota_flash_config_t flash_config = {
      .static_resources = &flash_resources,
  };
  esy_err_t result = esy_ota_flash_init(&s_ctx.writer, &flash_config);
  if (result != ESY_OK) return result;

  esy_ota_provider_t provider;
  result = esy_ota_flash_get_provider(&s_ctx.writer, &provider);
  if (result != ESY_OK) return result;
  const esy_ota_static_resources_t resources = {
      .last_chunk_buffer = s_ctx.last_chunk,
      .last_chunk_capacity = sizeof(s_ctx.last_chunk),
  };
  const esy_ota_config_t config = {
      .provider = provider,
      .max_image_bytes = esy_bootloader_get_flash_slot_size(),
      .max_chunk_bytes = sizeof(s_ctx.chunk),
      .static_resources = &resources,
      .new_session_id = new_session_id,
      .session_id_context = &s_ctx.session_sequence,
      /* Transport gap deadlines exclude Flash stalls; no core deadline. */
      .progress_timeout_ms = 0U,
  };
  return esy_ota_init(&s_ctx.ota, &config);
}

static bool deinitialize_update(void) {
  /* Core cleanup must release the provider before its cache is detached. */
  return esy_ota_deinit(&s_ctx.ota) == ESY_OK &&
         esy_ota_flash_deinit(&s_ctx.writer) == ESY_OK;
}

static void abort_update(void) {
  if (!deinitialize_update()) {
    esy_hal_system_reset();
  }
  resynchronize_uart();
  /* No IRQ/DMA/registry references exist. Reinit clears native UART errors.
   * Keep the handle intact if cleanup fails; a reset is then required. */
  if (esy_hal_uart_deinit(&s_ctx.uart) != ESY_OK || !configure_uart()) {
    esy_hal_system_reset();
  }
  (void)send_reply(REPLY_ERROR);
}

static bool candidate_is_acceptable(void) {
  uint8_t digest[ESY_FIRMWARE_SHA256_BYTES];
  s_ctx.candidate.version = read_le32(&s_ctx.header[4]);
  s_ctx.candidate.image_size = read_le32(&s_ctx.header[8]);
  memcpy(s_ctx.candidate.image_sha256, &s_ctx.header[12],
         sizeof(s_ctx.candidate.image_sha256));
  memcpy(s_ctx.candidate.signature, &s_ctx.header[44],
         sizeof(s_ctx.candidate.signature));
  /* Four final bytes are reserved and must be zero. */
  if (read_le32(&s_ctx.header[108]) != 0U ||
      s_ctx.candidate.version == 0U || s_ctx.candidate.image_size < 8U ||
      s_ctx.candidate.image_size > esy_bootloader_get_flash_slot_size()) {
    return false;
  }

  esy_sha256(&s_ctx.header[4], ESY_FIRMWARE_SIGNED_MESSAGE_BYTES, digest);
  return demo_verify_signature(NULL, digest, s_ctx.candidate.signature);
}

static bool candidate_vector_is_acceptable(size_t length) {
  const uint32_t slot_start = esy_bootloader_get_flash_slot_addr();
  const uint32_t stack = read_le32(&s_ctx.chunk[0]);
  const uint32_t reset_vector = read_le32(&s_ctx.chunk[4]);
  const uint32_t reset_address = reset_vector & ~1UL;
  return length >= 8U && slot_start != 0U &&
         stack > DEMO_RAM_START && stack <= DEMO_RAM_END &&
         (stack & 7U) == 0U && (reset_vector & 1U) != 0U &&
         reset_address >= slot_start &&
         (reset_address - slot_start) < s_ctx.candidate.image_size;
}

static void perform_update(void) {
  esy_image_status_t status;
  uint32_t received = 0U;

  if (!receive_bytes(&s_ctx.header[4], sizeof(s_ctx.header) - 4U, false) ||
      s_ctx.confirmation_result != ESY_OK || !candidate_is_acceptable() ||
      initialize_update() != ESY_OK ||
      esy_ota_begin(&s_ctx.ota, &s_ctx.candidate, &status) != ESY_OK ||
      !send_reply(REPLY_READY)) {
    abort_update();
    return;
  }
  const uint32_t session_id = status.session_id;

  while (received < s_ctx.candidate.image_size) {
    const uint32_t remaining = s_ctx.candidate.image_size - received;
    const size_t length = remaining < sizeof(s_ctx.chunk)
                              ? (size_t)remaining : sizeof(s_ctx.chunk);
    if (!receive_bytes(s_ctx.chunk, length, false) ||
        (received == 0U && !candidate_vector_is_acceptable(length)) ||
        esy_ota_write(&s_ctx.ota, session_id, received, s_ctx.chunk, length,
                      &status) != ESY_OK ||
        !send_reply(REPLY_ACK)) {
      abort_update();
      return;
    }
    received += (uint32_t)length;
  }

  if (esy_ota_finish(&s_ctx.ota, session_id, &status) != ESY_OK) {
    abort_update();
    return;
  }
  if (esy_ota_activate(&s_ctx.ota, session_id, &status) != ESY_OK) {
    /* Publication may already have changed metadata. Reboot to reconcile it;
     * do not clear the provider's publication latch and admit another image. */
    (void)send_reply(REPLY_ERROR);
    drain_uart_before_reset();
    esy_hal_system_reset();
  }
  if (!deinitialize_update()) {
    esy_hal_system_reset();
  }
  if (!send_reply(REPLY_DONE)) {
    /* The new trial metadata is already committed. Do not report a rollback. */
    esy_hal_system_reset();
  }
  drain_uart_before_reset();
  esy_hal_system_reset();
}

static void send_info(void) {
  uint8_t reply[16] = {'E', 'S', 'Y', 'I'};
  write_le32(&reply[4], (uint32_t)s_ctx.running_slot);
  write_le32(&reply[8], DEMO_FIRMWARE_VERSION);
  write_le32(&reply[12], (uint32_t)s_ctx.confirmation_result);
  (void)esy_hal_uart_send_blocking(&s_ctx.uart, reply, sizeof(reply));
}

static esy_err_t confirm_startup(void) {
  esy_firmware_header_t metadata;
  s_ctx.running_slot = esy_bootloader_get_running_slot();
  if (esy_os_core_count() != 1U || esy_hal_system_get_core_id() != 0U ||
      s_ctx.running_slot < 0 ||
      esy_bootloader_read_meta_data(s_ctx.running_slot, &metadata) != ESY_OK ||
      metadata.version != DEMO_FIRMWARE_VERSION) {
    return ESY_ERR_STATE;
  }
  /* Consumer health check: hardware init, UART init, scheduler task entry,
   * running-slot identity and the expected firmware version all succeeded.
   * This demo has no sensors, actuators, storage mounts or network to qualify. */
  return esy_bootloader_mark_success();
}

static void ota_task(void *argument) {
  (void)argument;
  s_ctx.confirmation_result = confirm_startup();

  for (;;) {
    if (!receive_bytes(s_ctx.header, 4U, true)) {
      abort_update();
    } else if (memcmp(s_ctx.header, "ESYI", 4U) == 0) {
      send_info();
    } else if (memcmp(s_ctx.header, "ESYU", 4U) == 0) {
      perform_update();
    } else {
      abort_update();
    }
  }
}

int main(void) {
  s_ctx.confirmation_result = ESY_ERR_STATE;
  s_ctx.running_slot = -1;
  if (esy_hal_system_init() == ESY_OK && configure_uart()) {
    esy_os_init();
    if (esy_os_task_create_static("ota", ota_task, NULL,
                                  sizeof(s_ctx.task_stack_buffer),
                                  DEMO_OTA_TASK_PRIORITY,
                                  s_ctx.task_stack_buffer,
                                  &s_ctx.task_control_storage,
                                  &s_ctx.task) == ESY_OK) {
      esy_os_start_scheduler();
    }
  }
  for (;;) {
    esy_hal_system_wait_for_event();
  }
}
