/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x080a0000, LENGTH = 384K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

INCLUDE image_sections.ld
