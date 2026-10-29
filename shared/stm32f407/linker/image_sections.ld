/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Startup copies .data and clears .bss using the symbols below. No allocator
 * arena is reserved. CCM SRAM is intentionally unused by these demonstrations.
 */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Stack_Size = 0x1000;
__StackTop = _estack;
__StackLimit = _estack - _Min_Stack_Size;

PHDRS
{
  code PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(512);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH :code

  .text :
  {
    . = ALIGN(4);
    *(.text .text.*)
    *(.glue_7 .glue_7t)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
  } > FLASH :code

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } > FLASH :code

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH :code

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH :code

  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH :code

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH :code

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
    _etext = .;
  } > FLASH :code

  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data .data.*)
    *(.RamFunc .RamFunc.* .time_critical.*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH :data

  .bss (NOLOAD) :
  {
    . = ALIGN(8);
    _sbss = .;
    __bss_start__ = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(8);
    _ebss = .;
    __bss_end__ = .;
  } > RAM :data

  .noinit (NOLOAD) :
  {
    . = ALIGN(8);
    *(.noinit .noinit.*)
    . = ALIGN(8);
    _end = .;
    PROVIDE(end = .);
  } > RAM :data

  .stack_budget (NOLOAD) :
  {
    . += _Min_Stack_Size;
  } > RAM :NONE

  .ARM.attributes 0 : { *(.ARM.attributes) }
  /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}

ASSERT(_end <= __StackLimit, "Static RAM overlaps the reserved main stack")
