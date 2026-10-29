/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1M
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

/* The shared startup helper supplies the linker search path. */
INCLUDE image_sections.ld
