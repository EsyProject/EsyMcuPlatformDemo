<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors

SPDX-License-Identifier: Apache-2.0
-->

# Authenticated bootloader: Pico 2 and STM32F407

## Purpose

Boot a signed application on **Raspberry Pi Pico 2 (Arm)** or
**STM32F407ZGT6 with 1 MiB internal Flash**.
The example uses the public EsyMcuPlatform bootloader to check image SHA-256,
verify an ECDSA P-256 signature, select an application slot and enter its reset
handler. Pair it with [ota_update](../ota_update/README.md) to provision version
1, upload version 2 into the inactive slot, trial-boot it and confirm startup.

This boot stage is synchronous and runs before any scheduler. The application
provides the FreeRTOS task and serial update transport.

## Public Surface

The example calls `esy_hal_system_init()`, `esy_bootloader_init()` and
`esy_bootloader_jump_to_app()`. The shared verifier supplies the required
signature callback; application code does not access vendor registers or drivers.

Its CMake entry is `demos/bootloader/CMakeLists.txt`. `ESY_TARGET_DEVICE` defaults
to `PICO2` and also accepts `STM32F407ZG` or `STM32F407ZGT6`. Each device uses
its own public board adapter and fixed Flash layout. There is no application-slot
choice for the bootloader. Both builds produce `bootloader.elf` and
`bootloader.bin`; Pico also produces UF2 and STM32 also produces HEX.

## Integration

Run the commands below from the repository root. Install CMake 3.24 or later,
Ninja and a GNU Arm Embedded toolchain.
The native package utility additionally needs a host C compiler and OpenSSL 3
development files. Use a Linux or macOS host for its serial commands. Never use
one build directory for different devices or slot layouts.

### Pico 2 (Arm)

Install Pico SDK 2.1.1 and picotool 2.1.1. Connect a **3.3 V** USB-to-UART adapter:
Pico GPIO0/TX to adapter RX, GPIO1/RX to adapter TX, and common ground. UART0 uses
115200 bit/s, 8N1. USB BOOTSEL is used only for initial provisioning; the running
application's update transport is UART.

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/micro-ecc

cmake -S demos/bootloader -B build/pico-bootloader -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=PICO2 \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DESY_TOOLCHAIN_ROOT=/path/to/gnu-arm
cmake --build build/pico-bootloader

cmake -S demos/ota_update -B build/pico-ota-a -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=PICO2 \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DESY_TOOLCHAIN_ROOT=/path/to/gnu-arm \
  -DDEMO_APP_SLOT=A -DDEMO_FIRMWARE_VERSION=1
cmake --build build/pico-ota-a

cmake -S host -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
ctest --test-dir build/host --output-on-failure
mkdir -p out/pico
build/host/demo_host pack keys/demo-signing-key.pem \
  build/pico-ota-a/ota_update.bin 1 out/pico/version1
build/host/demo_host verify out/pico/version1.esy
```

Hold BOOTSEL while attaching the Pico's USB cable, then provision all three
assets. The erase command removes **all existing Flash contents** on the Pico.
Use it only when preparing or deliberately resetting this demonstration board.

```sh
picotool erase -a
picotool load --ignore-partitions -v build/pico-bootloader/bootloader.elf
picotool load --ignore-partitions -v build/pico-ota-a/ota_update.bin -t bin -o 0x10040000
picotool load --ignore-partitions -v out/pico/version1.meta.bin -t bin -o 0x10020000
picotool reboot
build/host/demo_host info /dev/ttyUSB0
```

The bootloader ELF carries its address. Load the slot-linked application as a raw
binary at its explicit slot A address; picotool does not classify this image as
a standalone RP2 executable. The metadata is also a raw binary and requires its
explicit metadata A address. An application UF2 alone is not a supported
provisioning procedure for this layout. The expected response is
`Slot A, version 1, confirmation 0`; this sample response is illustrative.
Continue with [Pico OTA instructions](../ota_update/README.md#pico-2-arm).

The Pico bootloader runs on core 0 and never launches core 1. Its default SDK
alarm pool is disabled, so there is no application-owned timer/IRQ activity to
carry across the direct handoff. The OTA application also uses only core 0;
this is separate from the two-core [smp_hello](../smp_hello/README.md) example.

### STM32F407ZGT6

Install OpenOCD and an ST-Link-compatible SWD probe.

Connect the probe's SWDIO/SWCLK to PA13/PA14, and connect ground and the probe's
target-voltage reference according to your board. Set BOOT0 for booting internal
Flash. For the application transport, connect a **3.3 V** USB-to-UART adapter:
board PA9 TX to adapter RX, board PA10 RX to adapter TX, and common ground.
The application uses USART1 at 115200 bit/s, 8N1. Both images use the internal
16 MHz HSI oscillator; no external-crystal frequency or LED wiring is assumed.

Initialize the public dependencies:

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/CMSIS_5 external/cmsis-device-f4 \
  external/stm32f4xx-hal-driver external/micro-ecc
```

Build the bootloader and the slot A baseline. Replace `/path/to/gnu-arm` with
the toolchain directory containing `bin/arm-none-eabi-gcc`:

```sh
cmake -S demos/bootloader -B build/bootloader -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=STM32F407ZGT6 \
  -DESY_TOOLCHAIN_ROOT=/path/to/gnu-arm
cmake --build build/bootloader

cmake -S demos/ota_update -B build/ota_a -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=STM32F407ZGT6 \
  -DESY_TOOLCHAIN_ROOT=/path/to/gnu-arm \
  -DDEMO_APP_SLOT=A -DDEMO_FIRMWARE_VERSION=1
cmake --build build/ota_a
```

Build the native utility and create signed metadata for that exact baseline
binary. The version passed to `pack` must match `DEMO_FIRMWARE_VERSION`:

```sh
cmake -S host -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
ctest --test-dir build/host --output-on-failure
mkdir -p out
build/host/demo_host pack keys/demo-signing-key.pem \
  build/ota_a/ota_update_slot_a.bin 1 out/version1
build/host/demo_host verify out/version1.esy
```

The following command performs **initial provisioning**: it erases the complete
1 MiB application Flash, then installs the bootloader, baseline application and
its confirmed metadata. Existing firmware, both metadata sectors and any other
data in that Flash are erased. Run it only when preparing or deliberately
resetting this demonstration board:

```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "init; reset init; flash erase_address 0x08000000 0x100000; program build/bootloader/bootloader.elf verify; program build/ota_a/ota_update.elf verify; program out/version1.meta.bin verify 0x08008000; reset run; shutdown"
```

ELF images carry their load addresses. The metadata is a raw binary, so its
destination address is explicit, following the official
[OpenOCD programming syntax](https://openocd.org/doc/html/Flash-Commands.html).
If your probe uses a different interface configuration, replace only the
interface argument with the configuration appropriate to that probe.

Provision **all three files**. An application binary alone has no valid signed
metadata, and this bootloader does not invent a baseline on blank Flash.
`out/version1.esy` is a serial-update package; do not program the package file
directly into an application slot.

Once the board restarts, query the application, replacing the serial port with
your adapter's actual device:

```sh
build/host/demo_host info /dev/ttyUSB0
```

Expect slot A, version 1 and a successful confirmation result. This response
comes from the application; the bootloader itself has no UART console. Continue
with the [OTA update instructions](../ota_update/README.md) for version 2.
Subsequent serial updates do not use the full-erase provisioning command.

## Contracts

The bootloader validates metadata, image bounds, the Flash image digest and the
signature before selection. The signature binds the firmware version, length
and image hash to the compiled public key. Both bootloader and application
must use the same key and shared Flash layout.

The factory metadata generated by `pack` marks the initial image `CONFIRMED`.
An OTA candidate uses `TRY_BOOT`; the bootloader persists `UNCONFIRMED` before
entering that candidate. The application confirms after its own startup checks.
On a later reset, a valid confirmed baseline takes precedence over an
unconfirmed candidate. This example has no watchdog that forces a hung trial
to reset; a reset must occur before another selection can take place.

Each application must be linked for its actual slot. The boot stage transfers
to that slot's vector table without moving or relocating image contents. The
STM32 board restores the application's vector table and unmasks interrupts
before HAL initialization; Pico uses its SDK's linked vector table and startup.

If no valid image is selectable, initialization fails or a handoff returns,
the example waits without jumping to an unchecked address. Recover through the
debug probe or Pico BOOTSEL by provisioning a matching baseline and metadata.

## Resource Model

Both layouts provide two 384 KiB application slots and separate metadata regions.

| Region | Pico 2 | STM32F407ZGT6 |
| --- | --- | --- |
| Bootloader | `0x10000000`, 128 KiB | `0x08000000`, 32 KiB |
| Metadata A | `0x10020000`, 4 KiB | `0x08008000`, 16 KiB |
| Metadata B | `0x10021000`, 4 KiB | `0x0800C000`, 16 KiB |
| Application A | `0x10040000`, 384 KiB | `0x08040000`, 384 KiB |
| Application B | `0x100A0000`, 384 KiB | `0x080A0000`, 384 KiB |

See the [Pico board layout](../../shared/pico2/README.md) and
[STM32 board layout](../../shared/stm32f407/README.md#resource-model) before
changing addresses. Pico linker files are derived from the installed SDK's
RP2350 linker layout at build time; the shared helper changes the Flash region
for the selected image while retaining SDK startup and RAM sections.

The selected firmware uses static state and stack storage; its ELF retains no
heap allocation functions. STM32 reserves 4 KiB for its main stack within 128 KiB contiguous SRAM;
CCM SRAM is unused. Pico uses the SDK's RAM and stack layout. Signature
verification runs synchronously. There are
no bootloader tasks, DMA channels or transport users to stop before selection.

## Verification

Cross-builds, image sizes and completed host checks are recorded in
[VALIDATION.md](../../VALIDATION.md). Linker limits enforce each target's boot
region and static-RAM budget. The host self-test checks OpenSSL signing against
the same micro-ecc verifier compiled for the target.

The maintainer has tested the paired UART OTA flow, reboot and new-image
confirmation on Pico 2 and STM32F407ZGT6.
Sample responses above are illustrative.

## Known Limits

The included signing key is intentionally public demonstration material. Anyone
can sign with it; it establishes no trusted publisher identity. See the
[key replacement procedure](../../host/README.md#integration) and rebuild the
bootloader, both applications and host verifier together when changing it.

There is no UART recovery downloader, protected key storage, anti-rollback
counter, hardware-enforced boot trust or power-loss qualification. Software
version selection does not prevent a debug probe or a compromised application
from rewriting Flash. Trial boot and fallback require a remaining valid image
and intact metadata; they do not establish a general recovery guarantee.
