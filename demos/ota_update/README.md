<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# Signed firmware update: Pico 2 and STM32F407

## Purpose

Update a running Pico 2 (Arm) or STM32F407ZGT6 application over UART using the public
EsyMcuPlatform OTA Flash writer. The example authenticates candidate metadata,
writes the inactive slot, verifies Flash readback with SHA-256, requests trial
boot, and confirms the new image after its startup checks succeed.

This example and [bootloader](../bootloader/README.md) share one board and Flash
layout. Both must use the same verification key. The transport is wired UART;
the OTA module itself does not require this transport.

## Public Surface

The application uses `esy_hal_system_*`, `esy_hal_uart_*`, `esy_os_*`,
`esy_os_task_create_static`, `esy_ota_*`, `esy_ota_flash_*`, and application-side
`esy_bootloader_*` slot/confirmation APIs. It does not repeat bootloader
initialization or image selection inside the application.

| Build input | Default | Meaning |
| --- | --- | --- |
| `ESY_TARGET_DEVICE` | `PICO2` | Also accepts `STM32F407ZG` or `STM32F407ZGT6`. |
| `DEMO_APP_SLOT` | `A` | Link the complete image for slot `A` or `B`. |
| `DEMO_FIRMWARE_VERSION` | `1` | Positive unsigned 32-bit version; use the same value when packaging. |
| `ESY_TOOLCHAIN_ROOT` | Compiler discovery | GNU Arm Embedded installation root. |

Features are selected in [esy_feature_import.cmake](esy_feature_import.cmake),
including `ESY_RTOS=FreeRTOS`, `ENABLE_ESY_OTA` and `ENABLE_ESY_OTA_FLASH`.
Task and buffer settings are in [config/demo_config.h](config/demo_config.h).
CMake selects the UART/RAM and static-only kernel configuration from
[config/pico2/](config/pico2/) or [config/stm32f407/](config/stm32f407/).

## Integration

Both targets use a **3.3 V** UART adapter at **115200, 8N1**, without flow
control. Connect the adapter's TX to the board's RX, its RX to board TX, and
grounds together. The application does not mix text logs into this UART.

| Hardware | UART TX | UART RX | Initial provisioning |
| --- | --- | --- | --- |
| Pico 2 (Arm) | GPIO0 | GPIO1 | USB BOOTSEL and picotool |
| STM32F407ZGT6, 1 MiB Flash | PA9 | PA10 | SWD and OpenOCD |

Run the commands from the repository root. Use a separate build directory for
every device and slot. The host utility requires a native compiler and OpenSSL 3
development files on Linux or macOS.

### Pico 2 (Arm)

Install Arm GCC, CMake, Ninja, Pico SDK 2.1.1 and picotool 2.1.1. Replace the
SDK and toolchain paths below with your installations.

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/micro-ecc

cmake -S demos/bootloader -B build/pico-bootloader -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=PICO2 \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain
cmake --build build/pico-bootloader

cmake -S demos/ota_update -B build/pico-ota-a -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=PICO2 \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain \
  -DDEMO_APP_SLOT=A -DDEMO_FIRMWARE_VERSION=1
cmake --build build/pico-ota-a

cmake -S demos/ota_update -B build/pico-ota-b -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=PICO2 \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain \
  -DDEMO_APP_SLOT=B -DDEMO_FIRMWARE_VERSION=2
cmake --build build/pico-ota-b

cmake -S host -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
ctest --test-dir build/host --output-on-failure
mkdir -p out/pico
build/host/demo_host pack keys/demo-signing-key.pem \
  build/pico-ota-a/ota_update.bin 1 out/pico/version1
build/host/demo_host pack keys/demo-signing-key.pem \
  build/pico-ota-b/ota_update.bin 2 out/pico/version2
build/host/demo_host verify out/pico/version1.esy
build/host/demo_host verify out/pico/version2.esy
```

Provision bootloader, slot A/version 1 and metadata A using the complete
[Pico BOOTSEL procedure](../bootloader/README.md#pico-2-arm). Do not preload B.
Then run the following, allowing the target to reboot before the final query:

```sh
build/host/demo_host info /dev/ttyUSB0
build/host/demo_host upload /dev/ttyUSB0 out/pico/version2.esy
build/host/demo_host info /dev/ttyUSB0
```

The expected transition is `Slot A, version 1, confirmation 0` to
`Slot B, version 2, confirmation 0`. These sample responses are illustrative.
Unlike the SMP introduction, this Flash-writing example deliberately
runs **only core 0** and never launches core 1; see the concurrency contract below.

### STM32F407ZGT6

Use an STM32F407ZGT6 with **1 MiB Flash**, a compatible SWD probe for initial
provisioning, and a **3.3 V UART adapter**. Connect adapter TX to **PA10**,
adapter RX to **PA9**, and grounds together. USART1 uses **115200, 8N1**, with
no flow control. No external crystal is required: the board uses HSI at 16 MHz.
The application does not use the UART pins for logs.

Run these commands from the repository root after installing Arm GCC, CMake,
Ninja, a native C compiler, and OpenSSL 3 development files. Replace the example
toolchain path with your installation. Each firmware has its own build directory.

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/CMSIS_5 external/cmsis-device-f4 \
  external/stm32f4xx-hal-driver external/micro-ecc

cmake -S demos/bootloader -B build/bootloader -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=STM32F407ZGT6 \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain
cmake --build build/bootloader

cmake -S demos/ota_update -B build/ota_a -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=STM32F407ZGT6 \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain \
  -DDEMO_APP_SLOT=A -DDEMO_FIRMWARE_VERSION=1
cmake --build build/ota_a

cmake -S demos/ota_update -B build/ota_b -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DESY_TARGET_DEVICE=STM32F407ZGT6 \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain \
  -DDEMO_APP_SLOT=B -DDEMO_FIRMWARE_VERSION=2
cmake --build build/ota_b

cmake -S host -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
ctest --test-dir build/host --output-on-failure
mkdir -p out

build/host/demo_host pack keys/demo-signing-key.pem \
  build/ota_a/ota_update_slot_a.bin 1 out/version1
build/host/demo_host pack keys/demo-signing-key.pem \
  build/ota_b/ota_update_slot_b.bin 2 out/version2
build/host/demo_host verify out/version1.esy
build/host/demo_host verify out/version2.esy
```

Each pack command writes an `.esy` update package and a `.meta.bin` metadata
record. Provision **only bootloader + slot A/version 1 + metadata A**, following
the complete [STM32 provisioning procedure](../bootloader/README.md#stm32f407zgt6).
The `.meta.bin` generated for version 2 is not written during a UART update;
the OTA writer creates the trial metadata itself. Do not preload slot B or
its factory-confirmed metadata when demonstrating the A-to-B update.

After resetting into version 1, run the following. Replace `/dev/ttyUSB0` with
your UART adapter; macOS commonly uses `/dev/cu.usbserial-*`.

```sh
build/host/demo_host info /dev/ttyUSB0
build/host/demo_host upload /dev/ttyUSB0 out/version2.esy
build/host/demo_host info /dev/ttyUSB0
```

Allow the target to reboot before the second `info` request. The following
expected output is illustrative:

```text
Slot A, version 1, confirmation 0
...
Image accepted. Target will reboot and confirm after startup checks.
Slot B, version 2, confirmation 0
```

To update from B back to A, build another A-linked image and package its version
accordingly. Use increasing versions for repeated A-to-B-to-A demonstrations:
when both slots are confirmed, a later reset selects the higher-version image.
An image linked for the running slot is rejected. The native
[host utility](../../host/README.md) checks package integrity and vector placement
before sending; the MCU independently checks the inactive slot's vector range.

## Contracts

One static task owns UART, the OTA handle, Flash writes, metadata changes and
confirmation. It continues running during the update because it performs the
transfer. The only other task is the kernel idle task. There are no application
DMA users, timers, hardware outputs, watchdogs or interrupt-driven receive
callbacks requiring a stop handshake.

On Pico, the native RP2350 FreeRTOS port is configured with
`configNUMBER_OF_CORES=1`; the kernel's native SMP compile definition is retained.
The board, bootloader and application never launch core 1. This is required
because the selected default Flash safe-execute hook masks only core 0's local
interrupts before the RAM-resident operation. Do not enable a second core or add
XIP readers/DMA without implementing and verifying complete Flash coordination.

STM32F407 uses single-bank Flash. Erase/program operations can stall execution
and SysTick; this example does not promise real-time task scheduling during
those operations. The host sends at most **256 bytes**, then waits for `A` before
sending another chunk. It must not stream payload while Flash is being erased.
The application polls UART without inserting task delays, because STM32 USART1 has
no RX FIFO. It deliberately keeps the CPU busy while waiting for input.

Before any erase, the target validates size and a P-256 signature over
`SHA256(version_le32 || image_size_le32 || image_sha256)`. It checks the first
chunk's initial MSP and Thumb reset vector against the supported RAM and inactive
image region. After all bytes arrive, the writer verifies the image SHA-256 from
Flash in `esy_ota_finish()`. Only a successful finish is followed by
`esy_ota_activate()`, which publishes `TRY_BOOT` metadata and selects the next
boot target. A corrupted payload fails verification before activation even if
its metadata signature was valid. Neither middleware call resets the MCU;
the application sends the final reply, drains UART and resets.

The task owns a zero-initialized OTA core and native Flash provider. It binds
separate caller-owned write-cache and last-block buffers before beginning a
session. Session IDs are internal to this stop-and-wait transport, increase
within a boot, and are never accepted from the serial input; exhaustion rejects
a new session. The core's progress timeout is disabled because the transport
already bounds receive gaps and deliberately excludes synchronous Flash stalls.

The bootloader authenticates the image again and promotes its trial state before
handoff. The OTA task confirms only after HAL/UART initialization, scheduler task
entry, running-slot identification and the compiled firmware version matching
the signed metadata all succeed. These are this demonstration's finite startup
checks; a real application defines its own health criteria.

The byte protocol is deliberately small. All integers are little-endian.

| Update header offset | Bytes | Value |
| --- | --- | --- |
| 0 | 4 | ASCII `ESYU` |
| 4 | 4 | Positive firmware version |
| 8 | 4 | Exact image size, excluding the header |
| 12 | 32 | Image SHA-256 |
| 44 | 64 | Raw ECDSA P-256 signature, big-endian `r || s` |
| 108 | 4 | Reserved; all bytes must be zero |

Send the complete **112-byte header**, wait for `R`, then send fixed 256-byte
chunks, with only the last chunk shorter. Wait for `A` after every chunk,
including the last. Then wait for `D`: finish/activation succeeded and the target
will reset. Before resetting, it waits up to 100 ms for physical UART transmission
to drain. A drain failure can lose this final response even though activation
already succeeded; query the target after reboot to determine its state.
`E` rejects or aborts the session. The sender must stop immediately
on rejection. There is no chunk retry, sequence number or resume operation.

An `ESYI` four-byte request receives a 16-byte response: `ESYI`, running slot
(`0` for A, `1` for B), compiled firmware version, and signed 32-bit confirmation
result (`0` is success). It is accepted only while no update session is active.

A frame that stops making progress for two seconds is aborted. Cleanup drains
input until 100 ms of quiet, capped at two seconds, reinitializes UART and sends
`E`. A stopped transfer leaves its partial inactive image uncommitted. Reconnect
and restart the complete package; reset the board if the link cannot resynchronize.
Cleanup deinitializes the core before releasing the Flash provider's cache. A
cleanup failure requires reset. An activation failure can already have changed
persistent boot state: the task sends `E`, drains UART and resets so boot
selection can reconcile it. It never clears the publication latch to accept
another upload in that boot. After either final reply, query the version after
reconnection to resolve the resulting boot state.

## Resource Model

All application runtime state is in one static `s_ctx`: a UART handle, OTA core
and Flash provider handles, the internal session counter,
task/control storage, an aligned **6144-byte task stack**, a **256-byte write
cache**, a separate **256-byte last-block buffer**, a **256-byte receive chunk**,
the **112-byte header** and metadata.
The kernel supplies static idle-task storage. The selected STM32 kernel build
also retains an unused 128-word static timer buffer even though timers are
disabled; Pico's section garbage collection removes that unused storage.

FreeRTOS dynamic allocation is disabled, and the selected firmware ELFs retain
no heap allocation functions. The
shared verifier uses the compiled public key and fixed storage. The native host
utility uses desktop allocation and OpenSSL; this firmware resource statement
does not apply to the host utility.

## Verification

Native Arm builds of bootloader, slot A/version 1 and slot B/version 2 are
checked separately for both devices. Pico vector tables are linked at
`0x10040000`/`0x100A0000` and STM32 at `0x08040000`/`0x080A0000`.
Each provider supplies the matching scheduler tick/exception handlers.
Host checks cover signing, corrupted digest/signature rejection and package
format/integrity. See [validation details](../../VALIDATION.md).

The maintainer has tested UART OTA, reboot and new-image confirmation on Pico 2
and STM32F407ZGT6.

## Known Limits

- The included signing key is published demonstration material and provides no
  trusted signing identity. Replace the private key and compiled public key
  together before evaluating a separate trust policy.
- Positive lower or higher version values can enter a trial. After confirmation,
  later resets prefer the higher version among confirmed images, so a lower
  candidate may not remain selected. There is no anti-rollback counter, confidential transport, protected key storage or signature key rotation.
- The binary must be linked for the inactive slot. There is no relocation,
  slot copying or one-binary deployment to both addresses.
- Serial recovery requires the current application to run. There is no bootloader
  serial downloader; if no valid application can run, provision through Pico
  BOOTSEL or STM32 SWD again.
- There is no watchdog-driven automatic reset on failed application health.
  Unconfirmed-image fallback requires another reset; this example does not claim
  recovery from every application fault.
- Flash metadata updates are not transactional across every possible power cut.
- The native uploader currently supports POSIX hosts with OpenSSL 3. A Windows
  serial implementation is not included.
