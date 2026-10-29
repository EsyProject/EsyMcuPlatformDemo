/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "demo_security.h"
#include "esy_hal/esy_hal_system.h"
#include "esy_middleware/bootloader/esy_bootloader.h"

/* Startup has one synchronous owner. There are no tasks, DMA or UART users to
 * stop before the public bootloader transfers to the selected application. */
int main(void) {
  const esy_bootloader_config_t config = {
      .verify_signature = demo_verify_signature,
      .verify_context = NULL,
  };

  if (esy_hal_system_init() == ESY_OK &&
      esy_bootloader_init(&config) == ESY_OK) {
    (void)esy_bootloader_jump_to_app();
  }

  /* An absent/invalid image requires provisioning over the debug probe. */
  for (;;) {
    esy_hal_system_wait_for_event();
  }
}
