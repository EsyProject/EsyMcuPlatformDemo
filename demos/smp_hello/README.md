<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# Two static tasks with RTT logging

Build the same EsyMcuPlatform application for **Raspberry Pi Pico 2** or
**STM32F407ZGT6**. Each task logs its actual core ID through the public HAL.

| Device | Scheduling | Expected task core IDs |
| --- | --- | --- |
| Pico 2, Arm | Native FreeRTOS SMP with two cores | `hello0`: 0; `hello1`: 1 |
| STM32F407ZGT6 | Single-core FreeRTOS | Both tasks: 0 |

The example keeps its `smp_hello` directory name on both targets. **STM32F407 is
a single-core MCU and does not demonstrate SMP.** It demonstrates reuse of the
same public task, HAL and logger APIs on a different architecture.

## Purpose

Show explicit task placement, caller-owned task memory, and concurrent task
logging in one small, independently buildable firmware project. Pico 2 adds
native SMP; STM32F407 runs the same two workers on its single core. No project
generation step or application framework is required.

## Public Surface

Application code uses `esy_hal_system_*`, `esy_os_*`, `esy_os_task_*`, and
`esy_logger_*` only. It neither includes hardware SDK/FreeRTOS headers nor calls
their functions directly. Device-specific FreeRTOS/task settings and board
selection are visible in the example's `specs/` and `CMakeLists.txt`.

## Integration

### Hardware and tools

Both targets need an Arm GCC toolchain, CMake 3.24 or later, Ninja, and an
**RTT-capable SWD debug probe and host RTT client**. The USB cable used for
power or UF2 loading does not create a serial log port. The demo uses RTT
up-channel **0**; UART and USB stdio are disabled.

For Pico 2, use its Arm cores and install Pico SDK **2.1.1** plus matching
picotool **2.1.1**. Connect the Pico 2 SWDIO, SWCLK and GND pads to an RP2350
capable probe. For STM32F407ZGT6, connect SWDIO/SWCLK to PA13/PA14 and common
ground, and set BOOT0 for internal Flash. In both cases, follow your probe's
target-voltage connection requirements. The STM32 board uses its internal
**16 MHz HSI** oscillator; no external crystal or LED wiring is required.

Run these commands from the repository root. Replace tool paths with your
installation directories and keep a separate build directory per device.

### Pico 2 build and load

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel
cmake -S demos/smp_hello -B build/hello-pico2 -G Ninja \
  -DESY_TARGET_DEVICE=PICO2 -DCMAKE_BUILD_TYPE=Release \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DPICO_TOOLCHAIN_PATH=/path/to/arm-toolchain
cmake --build build/hello-pico2
```

The SDK may download/build picotool during configuration. For an offline build,
install picotool 2.1.1 first and append
`-Dpicotool_DIR=/path/to/picotool/cmake-package`; that directory must contain
`picotoolConfig.cmake`.

Hold BOOTSEL while connecting USB, then copy
`build/hello-pico2/smp_hello.uf2` to the board's mounted BOOTSEL drive. The board
reboots into the demo. The build also produces `smp_hello.elf`.

### STM32F407ZGT6 build and load

```sh
git submodule update --init external/EsyMcuPlatform external/FreeRTOS-Kernel \
  external/CMSIS_5 external/cmsis-device-f4 \
  external/stm32f4xx-hal-driver external/SEGGER_RTT external/printf
cmake -S demos/smp_hello -B build/hello-stm32 -G Ninja \
  -DESY_TARGET_DEVICE=STM32F407ZGT6 -DCMAKE_BUILD_TYPE=Release \
  -DESY_TOOLCHAIN_ROOT=/path/to/arm-toolchain
cmake --build build/hello-stm32
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/hello-stm32/smp_hello.elf verify reset exit"
```

Use your probe's interface configuration if it differs from ST-Link. Outputs
are `smp_hello.elf`, `smp_hello.hex` and `smp_hello.bin`. This is a **standalone
image at `0x08000000`** using the 1 MiB Flash/128 KiB SRAM layout. Loading it
replaces the bootloader at that address; provision the bootloader/update pair
again before running those separate examples. No bootloader or image metadata
is required for this example.

### Read RTT output

Attach the SWD probe, select the matching RP2350 Arm or STM32F407 target in
your RTT-capable debug server/client, leave the target running, and open
up-channel **0**. Use automatic control-block discovery, or find `_SEGGER_RTT`
in the ELF if your client requests an address. No breakpoint is needed.

Illustrative expected Pico 2 output:

```text
[boot][I] Two static tasks; scheduler cores=2
[hello0][I] Hello from core 0, sequence 0
[hello1][I] Hello from core 1, sequence 0
```

Expected STM32F407 content:

```text
[boot][I] Two static tasks; scheduler cores=1
[hello0][I] Hello from core 0, sequence 0
[hello1][I] Hello from core 0, sequence 0
```

The boot line may have passed before the viewer attaches. Reset with the RTT
viewer connected to capture startup. Each task increments its own sequence
counter once per logging attempt. Missing lines do not necessarily mean a task
stopped.

## Contracts

- Startup is system HAL, RTT backend/logger binding, OS initialization, task
  creation, affinity assignment, then scheduler start.
- Pico's native `RP2350_ARM_NTZ` port supplies `FREE_RTOS_KERNEL_SMP=1`.
  Its kernel config selects two cores and core affinity. `hello0` is assigned
  `ESY_CORE_0` and `hello1` `ESY_CORE_1` before scheduling starts.
- STM32 uses the native `GCC_ARM_CM4F` single-core port. Both task configs
  select core 0; the port requires no SMP affinity operation. The runtime
  startup check expects one core on STM32 and two on Pico 2. Printed IDs come
  from the HAL's execution-context query rather than echoing configuration.
- Logger ownership belongs to the startup context until reset. Both tasks use
  its frontend and never call backend callbacks directly. The context owns the
  RTT backend and one persistent `esy_logger_backend_binding_t`; initialization
  passes that binding through `esy_logger_config_t`. Whole lines are
  serialized; concurrent contention can drop a line, and RTT buffer exhaustion
  may drop or shorten output. A 500 ms initial offset reduces collisions;
  strict alternation and exact wall-clock cadence are not guaranteed.
- STM32 links the explicit [static formatter provider](../../shared/format/README.md)
  for logger `snprintf`/`vsnprintf`. It supports the integer/string formats used
  here; floating-point and `long long` formatting are disabled. Pico uses the
  SDK's existing formatting provider.
- If task creation partially fails, created tasks are deleted before the
  scheduler starts. Other startup failures enter a stopped startup loop;
  initialized static storage remains valid until reset.
- Workers share no mutable application state. Each owns its stack, task
  control storage and local sequence value. There are no application IRQ/DMA
  handlers or watchdogs. Library/OS code owns synchronization and native ticks.
- **OTA policy:** this standalone firmware enables neither OTA nor a
  bootloader, and neither worker performs Flash operations. OTA quiescence is
  not applicable. Adding an updater requires a new task stop/acknowledgment
  policy appropriate to the target's Flash behavior.

## Resource Model

One zero-initialized static `app_ctx_t` owns two independent, 8-byte-aligned
**4096-byte** task stacks, two opaque static task storage objects, task handles,
a **256-byte** RTT formatting buffer, and the logger handle/resources. Their
lifetime is the firmware lifetime. FreeRTOS dynamic allocation is disabled on
both targets; kernel idle/task resources are statically supplied.

Pico 2 additionally enables the kernel timer task, with static memory. STM32
disables timers; its selected kernel still retains an unused static timer
storage callback.

The selected ELFs contain no libc/RTOS allocator symbols. The STM32 formatter
uses caller buffers and stack-local conversion storage, so adding logging does
not introduce a libc heap. This describes the selected demo builds; changing
providers or features requires a separate memory audit.

Task parameters and kernel settings live in `specs/pico/` and `specs/stm32f4/`.
Shared buffer/timing budgets live in `specs/common/`. Feature selection has one
entry, `esy_feature_import.cmake`.

## Verification

Native Release builds completed using GCC **14.2.1** and the pinned public
dependencies; the Pico build additionally used SDK/picotool 2.1.1.

| Target | Flash bytes | RAM bytes | Additional memory | Output size |
| --- | ---: | ---: | --- | ---: |
| Pico 2 | 34,996 | 21,936 | SCRATCH_X: 2,048 B | UF2: 70,656 B |
| STM32F407ZGT6 | 23,344 | 18,480 | Includes 4 KiB main-stack reservation | BIN: 23,344 B |

These linker/file sizes depend on the selected build configuration.
Preprocessing the native kernel and OS wrapper confirms Pico SMP/two-core
affinity, STM32 single-core configuration, and static allocation enabled with
dynamic allocation disabled on both targets. STM32's linked vector table is
at `0x08000000`, with strong SVC, PendSV and SysTick handlers.

STM32 emits upstream HAL unused-parameter warnings and newlib-nano
unimplemented stdio-syscall warnings from discarded sections. No demo-source
warnings were emitted. Current revision and test results are recorded in
[VALIDATION.md](../../VALIDATION.md).

## Known Limits

This is an API/integration example, not a throughput benchmark or proof of
real-time behavior. STM32F407 does not support SMP. Supported device choices
are `PICO2`, `STM32F407ZG` and `STM32F407ZGT6`; other Pico variants, STM32
densities and architectures are outside the example. The local Pico board
adapter covers only system initialization. The example has no update transport,
Flash writer, networking, GUI or runtime shutdown.
