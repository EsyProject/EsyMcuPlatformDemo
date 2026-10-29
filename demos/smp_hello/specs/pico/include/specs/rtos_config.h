/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMP_HELLO_RTOS_CONFIG_H
#define SMP_HELLO_RTOS_CONFIG_H

#include "esy_common/defs/base/esy_common_defs.h"

#define DEMO_TASK_STACK_BYTES 4096u
#define DEMO_TASK_PRIORITY 1u
#define DEMO_SCHEDULER_CORE_COUNT 2u
#define DEMO_WORKER_0_AFFINITY ESY_CORE_0
#define DEMO_WORKER_1_AFFINITY ESY_CORE_1
#define DEMO_WORKER_0_START_DELAY_MS 0u
#define DEMO_WORKER_1_START_DELAY_MS 500u

#endif
