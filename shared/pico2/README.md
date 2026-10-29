<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors

SPDX-License-Identifier: Apache-2.0
-->

# Pico 2 shared board support

## Purpose

Provide the Pico 2 board mapping and fixed two-slot layout used by the signed
bootloader and serial-update demonstrations. These builds use RP2350 in Arm
mode, the standard Pico 2 board's 4 MiB external Flash, and a 150 MHz system
clock. No wireless module or USB console is involved.

## Public Surface

`esy_board` supplies the public HAL's system, boot-layout and UART board hooks.
Applications use the EsyMcuPlatform HAL rather than Pico SDK hardware calls.

`firmware.cmake` supplies `demo_pico_add_firmware(target layout)`. Call it after
creating the executable and before including the public `esy_firmware.cmake`.
Supported layout names are `bootloader`, `slot_a` and `slot_b`.

## Integration

Set `ESY_TARGET_DEVICE=PICO2` and select this directory as `ESY_BOARD_PATH`
before the platform's early import. Supply the supported Pico SDK and Arm
toolchain paths as shown in the example READMEs.

The firmware helper reads the selected SDK's RP2350 default linker layout and
generates build-directory linker files with only their Flash origin and length
changed. This keeps the SDK's reset startup, embedded image block, RAM vectors,
RAM functions and stack layout intact. It neither changes the SDK checkout nor
publishes a copied SDK linker file in this repository.

The transport is UART0, physical controller index 0. GPIO0 is TX (Pico 2 header
pin 1), and GPIO1 is RX (header pin 2). Cross TX/RX to a 3.3 V USB-to-UART adapter
and connect ground. The application selects 115200 bit/s, 8N1. Use the board's
SWD pads for a debug probe; provisioning details are in the
[bootloader guide](../../demos/bootloader/README.md).

## Contracts

The SDK initializes this image's RAM vector table before `main`; the board
preserves that table so the native FreeRTOS port can install its handlers.
Board initialization does not launch core 1, install an IRQ handler or create a
background callback. The shared board disables the SDK default alarm pool for
these builds, preventing default-pool initialization and callback/IRQ ownership
across the bootloader-to-application transfer. USB, UART and RTT stdio are
disabled in the update pair; the public UART handle exclusively owns the serial
pins.

The update application runs on **core 0 only** and keeps core 1 parked. Its
FreeRTOS configuration uses one scheduled core. The default public Flash safe
execution hook masks local interrupts while the SDK executes each Flash
operation from RAM. That local interrupt mask does not protect another active
core: do not enable the second core in this update example without adding and
validating a suitable multicore Flash-quiescence policy.

Polling UART is the only route assigned here. DMA and continuous RX requests
are unsupported. The stop-and-wait transport must finish receiving a request
before starting a Flash operation, then acknowledge it after that operation;
an unrestricted incoming stream cannot be received while XIP Flash is busy.

## Resource Model

| Region | Address | Capacity |
| --- | --- | --- |
| Bootloader | `0x10000000` | 128 KiB |
| Slot A metadata | `0x10020000` | 4 KiB |
| Slot B metadata | `0x10021000` | 4 KiB |
| Reserved | `0x10022000` | 120 KiB |
| Application slot A | `0x10040000` | 384 KiB |
| Application slot B | `0x100a0000` | 384 KiB |
| Remaining Flash | `0x10100000` | 3 MiB |

Each writable region follows 4 KiB erase-sector boundaries. The Flash program
unit is 256 bytes. Metadata records fit one program page; the update writer
handles application-page buffering. Each application must be linked for its
actual slot. The rest of the 4 MiB device is outside this demonstration's image
and metadata regions.

The boot-layout descriptor supplies both `meta_size[]` entries as 4 KiB erase
capacities. The serialized metadata record remains 256 bytes.

The generated linker keeps the SDK's 512 KiB main SRAM plus two 4 KiB scratch
regions. The core 0 startup/boot stack has an explicit 4 KiB reservation in
SCRATCH_Y. Application task buffers and control storage are statically provided.
The selected boot/update builds do not use the SDK default alarm pool or a
FreeRTOS heap provider.

## Verification

See [VALIDATION.md](../../VALIDATION.md) for completed cross-builds and host
checks. Inspect each ELF's initial vector words, executable reset address,
Flash region and native scheduler configuration before provisioning it.

## Known Limits

The fixed layout supports the standard 4 MiB Pico 2 Arm build. It does not
configure the RP2350 boot ROM's partition tables, OTP trust settings or secure
boot features. The library's software signature check is a separate mechanism.
There is no general multicore Flash-safety implementation or UART recovery
downloader in this board support.

The SDK may generate UF2 outputs for individual application slots. Such a file
does not provision the required bootloader and signed metadata; follow the
complete initial-provisioning procedure rather than treating a slot UF2 as a
standalone demonstration.
