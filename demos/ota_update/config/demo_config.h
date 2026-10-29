/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

#include "demo_device_config.h"

#define DEMO_UART_BAUDRATE 115200U
#define DEMO_OTA_TASK_STACK_BYTES 6144U
#define DEMO_OTA_TASK_PRIORITY 1U
#define DEMO_OTA_CACHE_BYTES 256U
#define DEMO_OTA_CHUNK_BYTES 256U
#define DEMO_OTA_HEADER_BYTES 112U
#define DEMO_OTA_RX_GAP_TIMEOUT_MS 2000U
#define DEMO_OTA_RESYNC_QUIET_MS 100U
#define DEMO_UART_TX_DRAIN_TIMEOUT_MS 100U
#endif
