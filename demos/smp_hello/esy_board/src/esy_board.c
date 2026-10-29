/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esy_board/esy_board.h"
#include <stddef.h>

void esy_board_init(void) {
  /* The Pico SDK startup owns the minimal hardware initialization. */
}

bool esy_board_get_system_config(esy_system_config_t *out_config) {
  if (out_config == NULL) return false;
  *out_config = (esy_system_config_t){.sys_clock_khz = 150000u,
                                     .enable_us_tick = true};
  return true;
}
