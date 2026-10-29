/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_ESY_BOARD_H
#define DEMO_ESY_BOARD_H

#include <stdbool.h>
#include "esy_common/defs/system/esy_system_defs.h"

/** @brief Board hook called by the public HAL before system configuration. */
void esy_board_init(void);

/** @brief Supply the Pico 2 nominal 150 MHz system clock; reject NULL output. */
bool esy_board_get_system_config(esy_system_config_t *out_config);

#endif
