/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include "esy_hal/esy_hal_system.h"
#include "esy_middleware/logger/esy_logger.h"
#include "esy_middleware/logger/esy_logger_backend_rtt.h"
#include "esy_middleware/os/esy_os.h"
#include "esy_middleware/os/esy_os_task.h"
#include "specs/app_constants.h"
#include "specs/rtos_config.h"

typedef struct {
  _Alignas(ESY_OS_TASK_STACK_ALIGN) uint8_t stack[DEMO_TASK_STACK_BYTES];
  esy_os_task_static_storage_t control;
  esy_os_task_t *task;
} demo_worker_storage_t;

typedef struct {
  esy_logger_backend_t logger;
  uint8_t logger_buffer[DEMO_LOGGER_BUFFER_BYTES];
  esy_logger_backend_rtt_static_resources_t logger_resources;
  esy_logger_backend_binding_t logger_binding;
  demo_worker_storage_t workers[DEMO_WORKER_COUNT];
} app_ctx_t;

typedef struct {
  const char *name;
  uint8_t affinity;
  uint32_t start_delay_ms;
} demo_worker_config_t;

static app_ctx_t s_ctx;
static const demo_worker_config_t k_workers[DEMO_WORKER_COUNT] = {
    {.name = "hello0", .affinity = DEMO_WORKER_0_AFFINITY,
     .start_delay_ms = DEMO_WORKER_0_START_DELAY_MS},
    {.name = "hello1", .affinity = DEMO_WORKER_1_AFFINITY,
     .start_delay_ms = DEMO_WORKER_1_START_DELAY_MS},
};

static void wait_after_failure(void) {
  for (;;) esy_hal_system_delay_ms(DEMO_BOOT_ERROR_WAIT_MS);
}

static void worker_main(void *arg) {
  const demo_worker_config_t *config = arg;
  uint32_t sequence = 0u;
  if (config == NULL) {
    for (;;) esy_os_task_delay_ms(DEMO_LOG_PERIOD_MS);
  }
  if (config->start_delay_ms != 0u)
    esy_os_task_delay_ms(config->start_delay_ms);
  for (;;) {
    ESY_LOG_I(config->name, "Hello from core %u, sequence %lu",
              (unsigned)esy_hal_system_get_core_id(), (unsigned long)sequence++);
    esy_os_task_delay_ms(DEMO_LOG_PERIOD_MS);
  }
}

static esy_err_t app_start(void) {
  esy_err_t result = esy_hal_system_init();
  if (result != ESY_OK) return result;

  s_ctx.logger_resources.buffer = s_ctx.logger_buffer;
  s_ctx.logger_resources.buffer_size = sizeof(s_ctx.logger_buffer);
  esy_logger_backend_rtt_config_t logger_config;
  esy_logger_backend_rtt_default_cfg(&logger_config);
  logger_config.up_buffer_index = DEMO_LOGGER_RTT_CHANNEL;
  logger_config.static_resources = &s_ctx.logger_resources;
  result = esy_logger_backend_rtt_init(&s_ctx.logger, &logger_config);
  if (result != ESY_OK) return result;
  s_ctx.logger_binding = (esy_logger_backend_binding_t){
      .backend = &s_ctx.logger,
      .filter_level = ESY_LOG_LEVEL_INFO,
  };
  const esy_logger_static_resources_t logger_resources = {
      .bindings = &s_ctx.logger_binding,
      .binding_count = 1u,
  };
  const esy_logger_config_t config = {
      .static_resources = &logger_resources,
      .filter_level = ESY_LOG_LEVEL_INFO,
  };
  result = esy_logger_init(&config);
  if (result != ESY_OK) {
    (void)esy_logger_backend_rtt_deinit(&s_ctx.logger);
    return result;
  }

  esy_os_init();
  if (esy_os_core_count() != DEMO_SCHEDULER_CORE_COUNT)
    return ESY_ERR_NOT_SUPPORTED;
  for (size_t i = 0u; i < DEMO_WORKER_COUNT; ++i) {
    result = esy_os_task_create_static(
        k_workers[i].name, worker_main, (void *)&k_workers[i],
        sizeof(s_ctx.workers[i].stack), DEMO_TASK_PRIORITY,
        s_ctx.workers[i].stack, &s_ctx.workers[i].control, &s_ctx.workers[i].task);
    if (result != ESY_OK) {
      /* The scheduler has not started, so no worker can still be running. */
      for (size_t j = 0u; j < i; ++j) esy_os_task_delete(s_ctx.workers[j].task);
      return result;
    }
    esy_os_task_set_core_affinity(s_ctx.workers[i].task, k_workers[i].affinity);
  }
  ESY_LOG_I("boot", "Two static tasks; scheduler cores=%u",
            (unsigned)esy_os_core_count());
  return ESY_OK;
}

int main(void) {
  esy_err_t result = app_start();
  if (result != ESY_OK) {
    ESY_LOG_E("boot", "Startup failed: %ld", (long)result);
    wait_after_failure();
  }
  esy_os_start_scheduler();
  ESY_LOG_E("boot", "Scheduler returned unexpectedly");
  wait_after_failure();
  return 0;
}
