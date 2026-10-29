<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# EsyMcuPlatform Demo

Three small examples of [EsyMcuPlatform](https://github.com/EsyProject/EsyMcuPlatform),
each supporting **Raspberry Pi Pico 2 (Arm)** and **STM32F407ZGT6 (1 MiB Flash)**.
Select the board with one CMake input: `ESY_TARGET_DEVICE`.

## Build

**Start with `smp_hello` using the quick-build commands below.** There is no
root `CMakeLists.txt`: each project has its own CMake entry. Run all commands
from the repository root and select the project with `cmake -S`.

| Project | CMake source (`-S`) | What it builds | Full build and usage instructions |
| --- | --- | --- | --- |
| `smp_hello` | `demos/smp_hello` | Two tasks with RTT logs; two cores on Pico 2, one on STM32. | [Pico 2](demos/smp_hello/README.md#pico-2-build-and-load) / [STM32F407](demos/smp_hello/README.md#stm32f407zgt6-build-and-load) |
| `bootloader` | `demos/bootloader` | Signed dual-slot boot. | [Pico 2](demos/bootloader/README.md#pico-2-arm) / [STM32F407](demos/bootloader/README.md#stm32f407zgt6) |
| `ota_update` | `demos/ota_update` | UART firmware update, trial boot and confirmation. | [Pico 2](demos/ota_update/README.md#pico-2-arm) / [STM32F407](demos/ota_update/README.md#stm32f407zgt6) |
| `demo_host` | `host` | Native signing, packaging and UART transfer tool. | [Linux/macOS](host/README.md#integration) |

### Requirements

Firmware builds require **Arm GCC, CMake 3.24 or later, and Ninja**. Pico 2
also requires **Pico SDK 2.1.1** and matching **picotool 2.1.1**.
Replace `/path/to/arm-toolchain` below with the installation directory containing
`bin/arm-none-eabi-gcc`, and `/path/to/pico-sdk` with your SDK directory.

The native host tool requires a C compiler, CMake, Ninja, OpenSSL 3 development
files, and a Linux or macOS host.

### Get the sources

```sh
git clone https://github.com/EsyProject/EsyMcuPlatformDemo.git
cd EsyMcuPlatformDemo
```

If you already have the repository, start with one of the builds below.

### Quick build: Pico 2 Hello

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel

cmake -S demos/smp_hello -B build/hello-pico2 -G Ninja \
  -DESY_TARGET_DEVICE=PICO2 -DCMAKE_BUILD_TYPE=Release \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DPICO_TOOLCHAIN_PATH=/path/to/arm-toolchain
cmake --build build/hello-pico2
```

Outputs: `build/hello-pico2/smp_hello.uf2` and `smp_hello.elf`.
See the [Pico 2 guide](demos/smp_hello/README.md#pico-2-build-and-load) for
BOOTSEL loading and offline picotool setup; the SDK may fetch/build picotool
during configuration.

### Quick build: STM32F407ZGT6 Hello

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/CMSIS_5 external/cmsis-device-f4 \
  external/stm32f4xx-hal-driver external/SEGGER_RTT external/printf

cmake -S demos/smp_hello -B build/hello-stm32 -G Ninja \
  -DESY_TARGET_DEVICE=STM32F407ZGT6 -DCMAKE_BUILD_TYPE=Release \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain
cmake --build build/hello-stm32
```

Outputs: `build/hello-stm32/smp_hello.elf`, `smp_hello.hex` and `smp_hello.bin`.
This is a standalone firmware at `0x08000000`; flashing it replaces any
bootloader there. See the [STM32 guide](demos/smp_hello/README.md#stm32f407zgt6-build-and-load)
for SWD/OpenOCD loading. `STM32F407ZG` is also accepted as the device ID.

### Build the native host tool

```sh
git submodule update --init external/micro-ecc
cmake -S host -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Output: `build/host/demo_host`. See the [host guide](host/README.md#integration)
for signing, packaging and serial commands.

### Rebuild and run the update demo

After the initial configuration, rerun just the matching `cmake --build`
command to compile changes. Use a separate build directory for each project,
board and application slot.

For the full update demonstration, follow the **bootloader** and **ota_update**
guides linked in the table above. Provision the bootloader, version 1 application
and metadata, then upload version 2 and query the confirmed version.

Hello logs require an RTT-capable SWD probe/client. STM32 provisioning requires
an SWD probe and OpenOCD. Updates use an external **3.3 V USB-to-UART adapter**;
the firmware USB connection does not provide a serial console.

There are **no project-owned Shell or Python scripts**. Commands are documented
directly. [host/](host/README.md) contains a small native C executable for signing,
packaging and serial transfer. Upstream submodules retain their own files and
build systems.

## Layout

```text
EsyMcuPlatformDemo/
├── demos/
│   ├── smp_hello/       Portable static tasks; Pico 2 SMP and STM32 single core
│   ├── bootloader/      Signature verification and application handoff
│   └── ota_update/      Signed serial update and startup confirmation
├── shared/
│   ├── pico2/           Update-pair board mapping and Flash layout
│   ├── stm32f407/       Board mapping, startup and linker layouts
│   ├── security/        Shared P-256 verifier and sample public key
│   └── format/          Static buffer formatter for STM32 hello
├── host/                Native C package and serial utility
├── keys/                Public demonstration signing key
└── external/            Pinned public dependencies
```

Application sources are shared across boards. CMake selects the board adapter
and configuration; application code uses public EsyMcuPlatform HAL, OS and
middleware interfaces. Startup, features, task storage and resource ownership
remain visible. No project generation step is required.

Each example's `esy_feature_import.cmake` owns its `ESY_RTOS` and
`ENABLE_ESY_*` selections. The update example explicitly links the generic
`esy_ota_core` and native `esy_ota_flash` provider. It keeps its documented UART
protocol while mapping a complete upload to begin, write, finish and activate
operations; the application owns signature admission and reset.

## Memory and update scope

The update pair deliberately uses **one scheduled core on both boards**.
Pico 2 keeps core 1 parked during Flash access. Enabling SMP in an updater
requires a separate, validated policy for coordinating Flash access across cores.

Firmware tasks, handles and update buffers use static or caller-owned memory.
FreeRTOS dynamic allocation is disabled. The STM32 hello build selects an
allocation-free buffer formatter; Pico hello uses its SDK formatter. These
statements describe the selected example configurations, not every optional
SDK or middleware component. The desktop tool uses OpenSSL and host allocation.

The update flow checks SHA-256 and ECDSA P-256 signatures, stages the inactive
image, trial-boots it, and confirms after the example's startup checks.
The included private key is **public demonstration material** and provides no
trusted signing identity; see [key replacement](host/README.md#integration).

UART is a small stop-and-wait example transport. It is not wireless OTA.
There is no transfer resumption, protected signing-key storage, anti-rollback
counter or power-loss qualification. Images must be linked for the inactive
slot. Keep the full board-specific bootloader/application/metadata provisioning
procedure intact; a slot image alone is insufficient.

## Verification status

[VALIDATION.md](VALIDATION.md) records dependency versions, completed native
cross-builds and host checks. The maintainer has tested UART OTA, reboot and
new-image confirmation on Pico 2 and STM32F407ZGT6.
Expected output in the guides is illustrative.

## License

Demo-owned source is Apache-2.0; see [LICENSE](LICENSE). Dependencies retain their
own licenses; see [DEPENDENCIES.md](DEPENDENCIES.md).
