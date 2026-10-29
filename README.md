<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# EsyMcuPlatform Demo

Three small examples of [EsyMcuPlatform](https://github.com/EsyProject/EsyMcuPlatform),
each supporting **Raspberry Pi Pico 2 (Arm)** and **STM32F407ZGT6 (1 MiB Flash)**.
Select the board with one CMake input: `ESY_TARGET_DEVICE`.

| Example | Pico 2 | STM32F407ZGT6 |
| --- | --- | --- |
| [smp_hello](demos/smp_hello/README.md) | Two static tasks on separate cores; RTT logs actual core IDs. | The same task/logger API on one core; both tasks report core 0. |
| [bootloader](demos/bootloader/README.md) | Signed dual-slot boot in external Flash. | Signed dual-slot boot in internal Flash. |
| [ota_update](demos/ota_update/README.md) | UART update, Flash readback, trial boot and confirmation. | The same update flow through the same public APIs. |

**Start with `smp_hello`** to evaluate static tasks, logging and Pico 2 SMP.
STM32F407 has one CPU core; its hello build demonstrates portable task code.
For the full update demonstration, provision `bootloader` and version 1 of
`ota_update`, then upload version 2 and query the confirmed version.

The update pair deliberately uses **one scheduled core on both boards**.
Pico 2 keeps core 1 parked during Flash access. Enabling SMP in an updater
requires a separate, validated policy for coordinating Flash access across cores.

## Get the sources

```sh
git clone https://github.com/EsyProject/EsyMcuPlatformDemo.git
cd EsyMcuPlatformDemo
```

Initialize the public dependencies you need:

```sh
# Pico 2 hello.
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel

# Either board's bootloader and update pair.
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/micro-ecc

# Additional dependencies for STM32F407 examples.
git submodule update --init external/CMSIS_5 external/cmsis-device-f4 \
  external/stm32f4xx-hal-driver

# Additional providers for STM32F407 hello logging.
git submodule update --init external/SEGGER_RTT external/printf
```

Each example has an independent CMake entry, feature file, configuration and
complete build/flash instructions. Choose `-DESY_TARGET_DEVICE=PICO2` or
`-DESY_TARGET_DEVICE=STM32F407ZGT6`, using a fresh build directory for each
board/slot combination. `STM32F407ZG` is accepted as the short part ID.

Install Arm GCC, CMake and Ninja. Pico 2 needs Pico SDK **2.1.1** and matching
picotool. STM32 provisioning uses an SWD probe and OpenOCD. Hello logs require
an RTT-capable SWD probe/client. Updates use an external **3.3 V USB-to-UART
adapter**; the firmware USB connection does not provide a serial console.
The native packer/uploader requires a C compiler, OpenSSL 3 development files
and a Linux or macOS host.

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
