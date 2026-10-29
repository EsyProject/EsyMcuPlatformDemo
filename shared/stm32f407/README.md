<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors

SPDX-License-Identifier: Apache-2.0
-->

# STM32F407 shared board support

## Purpose

This board support serves the bootloader and OTA application examples on an
STM32F407ZGT6 with 1 MiB internal Flash. It uses the internal 16 MHz HSI clock,
so neither example assumes an external crystal frequency or a board LED.

## Public Surface

The `esy_board` target implements the system, boot-layout and polling-UART
mapping needed by the public EsyMcuPlatform HAL. Applications call the public
HAL APIs. The `STM32_Drivers` target owns the official ST HAL provider.

`firmware.cmake` provides `demo_stm32_add_startup(target layout)`, which attaches
the official CMSIS device startup and `SystemInit` to an executable. `layout`
is `bootloader`, `slot_a` or `slot_b`. The public platform firmware helper owns
the final linker-script selection and ELF, HEX and BIN outputs.

## Integration

Set `ESY_BOARD_PATH` to this directory before the platform's early import.
Initialize the public dependency submodules before configuring CMake. These
examples use the official `CMSIS_5/CMSIS/Core/Include`, `cmsis-device-f4` and
`stm32f4xx-hal-driver` checkouts under the repository's `external` directory.

The UART transport is USART1, with PA9 as TX and PA10 as RX. Cross TX/RX to a
3.3 V USB-to-UART adapter and connect grounds. Do not connect RS-232 voltage
levels or use a 5 V serial signal. The examples select 115200 bit/s, 8N1; the
board support only owns the pin mapping. Programming uses SWD (PA13/PA14).

## Contracts

Call `esy_hal_system_init()` once before peripheral initialization. The board
restores the current image's vector table and unmasks IRQs, which the direct
boot handoff masks before entering an application's reset handler. Both images
leave the reset HSI clock selected. The application FreeRTOS configuration uses
the same 16 MHz processor clock.

UART ownership and all transfer operations remain with the caller. No DMA
route is assigned by this board. Continuous RX and DMA requests return
unsupported; the examples use polling and a request/response protocol to avoid
receiving an unbounded byte stream while Flash stalls instruction fetches.

## Resource Model

| Region | Address | Capacity | Physical sectors |
| --- | --- | --- | --- |
| Bootloader | `0x08000000` | 32 KiB | 0–1 |
| Slot A metadata | `0x08008000` | 16 KiB | 2 |
| Slot B metadata | `0x0800c000` | 16 KiB | 3 |
| Reserved | `0x08010000` | 192 KiB | 4–5 |
| Application slot A | `0x08040000` | 384 KiB | 6–8 |
| Application slot B | `0x080a0000` | 384 KiB | 9–11 |

Each writable region follows full physical-sector boundaries. The layouts
reserve 4 KiB for the main stack and use the contiguous 128 KiB SRAM region.
CCM SRAM is unused. No heap arena is reserved. Each application must be linked
for its actual slot: a slot A binary cannot be written to slot B and executed.

The boot-layout descriptor supplies both `meta_size[]` entries as 16 KiB erase
capacities for sectors 2 and 3. The serialized metadata record remains 256 bytes.

## Verification

Build the bootloader and both application slots using the commands in their
example READMEs. Linker bounds reject an image or static RAM that exceeds its
region.

## Known Limits

These files do not support other STM32F4 Flash densities or assign arbitrary
board peripherals. UART accuracy depends on HSI tolerance, supply voltage and
temperature. The fixed Flash layout is a demonstration layout, not a general
partition manager.
