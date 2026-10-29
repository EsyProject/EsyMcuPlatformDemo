<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# Verification record

The maintainer has tested UART OTA on Pico 2 and STM32F407ZGT6, including reboot
and new-image confirmation.

The current public platform pin is
`395bffe5f1a8b732a3564dc17648d2b622dfaed0` (`v0.1.0`). It differs from the
historical baseline below only in `CHANGELOG.md`; firmware source and build
files are identical.

The cross-build and host results below were recorded on 2026-10-28 against
EsyMcuPlatform `81412da7b46fbc87c6cc62c78369ade2621c4d7a` and are retained as
the automated baseline. Other pinned dependencies are listed in
[DEPENDENCIES.md](DEPENDENCIES.md).
The results below replace the earlier firmware sizes and migration evidence.

## Public release dependency verification

On 2026-10-29, the pinned `v0.1.0` commit was fetched successfully from the public
EsyMcuPlatform repository. All eight Release firmware configurations listed
below were configured and built again against that commit with the same Arm
GCC 14.2.1, Pico SDK/picotool 2.1.1 and pinned board/kernel providers. All eight
builds passed; their Flash and main-RAM reports match the table below.

The native host executable was also rebuilt with GCC 15.2.0, OpenSSL 3.5.5 and
`DEMO_TEST_STATIC_FORMATTER=ON`. Both CTests, `signing_roundtrip` and
`formatter_contract`, passed. No demo-owned source warnings were reported;
the provider diagnostics described below remain.

The existing repository-root `build` cache still points at the host entry with
an Arm compiler and has no `build.ninja`; `cmake --build build` failed before
compilation. The successful embedded builds and native host tests used fresh,
separate build directories. These repeated automated checks do not replace the
maintainer's hardware OTA testing reported above.

## Native firmware builds

All eight Release configurations compiled and linked using GNU Arm Embedded
14.2.1, the actual board SDK/CMSIS/HAL providers, and the pinned FreeRTOS kernel.
Pico builds used Pico SDK and picotool 2.1.1. These are embedded cross-builds.

| Configuration, Release | Linked Flash | Main RAM report | Output |
| --- | --- | --- | --- |
| Pico 2 `smp_hello` | 34,996 B | 21,936 B | ELF, HEX, BIN, 70,656 B UF2 |
| STM32F407ZGT6 `smp_hello` | 23,344 B | 18,480 B | ELF, HEX, BIN |
| Pico 2 `bootloader` | 13,528 B | 1,600 B | ELF, HEX, BIN, UF2 |
| STM32F407ZGT6 `bootloader` | 11,672 B | 4,240 B | ELF, HEX, BIN |
| Pico 2 `ota_update`, slot A, version 1 | 31,316 B | 14,148 B | ELF, HEX, BIN, UF2 |
| Pico 2 `ota_update`, slot B, version 2 | 31,316 B | 14,148 B | ELF, HEX, BIN, UF2 |
| STM32F407ZGT6 `ota_update`, slot A, version 1 | 39,092 B | 17,616 B | ELF, HEX, slot A BIN |
| STM32F407ZGT6 `ota_update`, slot B, version 2 | 39,092 B | 17,616 B | ELF, HEX, slot B BIN |

STM32 RAM reports include the reserved 4 KiB main stack. The OTA task's 6 KiB
static stack and hello's two 4 KiB task stacks are included in static RAM.
Pico hello also reserves a 2 KiB core 1 stack in SCRATCH_X and a 2 KiB core 0
startup stack in SCRATCH_Y. Pico boot/update reserve a 4 KiB startup stack in
SCRATCH_Y. These SDK dummy stack sections are outside the main-RAM reports.
The figures report linker section sizes and reserved stack budgets.

The example READMEs contain the complete configure/build commands. Use a fresh
build directory per device and slot. The old repository-root `build` cache was
an incomplete host configuration with an Arm compiler and no `build.ninja`;
`cmake --build build` failed before compilation. The eight successful firmware
checks used independent configurations instead.

## Integration and artifact checks

- The three feature files use `ESY_RTOS` and `ENABLE_ESY_*`. Script assertions
  confirmed that opposite cache inputs cannot override their ordinary `set()`
  selections. Two real bootloader configurations in the same directory, the
  second with conflicting cache values, produced identical feature snapshots,
  31 compilation units and 10 target graph entries, with no OS, logger or OTA
  target/source assembled.
- Preprocessing actual kernel `tasks.c` and wrapper `esy_os_task.c` confirmed
  Pico hello native SMP, two cores and affinity; STM32 hello one core; and Pico
  update one scheduled core with affinity disabled. All four task-bearing
  configurations enable static allocation and disable dynamic allocation.
- Pico update ELFs contain no core 1 launch symbol. Their native
  `flash_range_erase` and `flash_range_program` functions are linked in RAM.
- All four application BINs have aligned SRAM initial stack pointers and Thumb
  reset vectors inside their linked slots. Pico slot bases are
  `0x10040000`/`0x100A0000`, with MSP `0x20082000`; STM32 slot bases are
  `0x08040000`/`0x080A0000`, with MSP `0x20020000`.
  STM32 application vectors link strong SVC, PendSV and SysTick handlers.
- All eight ELFs were scanned for retained libc and RTOS allocation symbols;
  none were found. This describes the selected linked firmware. It does not
  establish heap-free behavior for every SDK feature or runtime stack margins.
- Both board layouts explicitly initialize both `meta_size[]` entries. Pico
  reserves 4 KiB per metadata sector; STM32 uses 16 KiB sectors 2 and 3. The
  serialized metadata record and signature message remain unchanged.

## Host and owner checks

The host executable built with GCC 15.2.0 and OpenSSL 3.5.5. Both shipped CTests
passed: OpenSSL-to-micro-ecc signing and corrupt digest/signature rejection,
and the static STM32 formatter contract. Reproduce them with:

```sh
cmake -S host -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DDEMO_TEST_STATIC_FORMATTER=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

A temporary package check completed 60 assertions across the four newly built
A/B application images. It generated and verified signed packages and checked
header length/version, image SHA-256, exact payload, 256-byte CONFIRMED factory
metadata, reserved bytes, metadata CRC32 and vectors. Corrupt hash/signature/
reserved fields and payload were rejected, as were invalid version inputs.
The 112-byte `ESYU` transport header and canonical 40-byte signature message
remain compatible with the host utility.

A temporary C harness executes the actual OTA application's owner logic with
the actual new OTA core, native Flash provider and SHA-256 implementation.
UART, Flash, boot state and signature admission use fakes; provider cleanup
failure is injected through its public descriptor. It compiles with
`-Wall -Wextra -Werror` and UndefinedBehaviorSanitizer. All 16 scenarios pass
under each board's application configuration, for 32 completed scenarios.
It checks admission rejection, first-block vector rejection, readback failure before activation,
finish/activate/publication ordering, separate core/provider cleanup, UART drain
before reset, activation and metadata-publication failure, timeout cleanup and
session restart, cleanup failure, session-ID exhaustion, payload hash mismatch,
failed startup confirmation, and receive/drain waits across tick wrap. These
checks exercise the owner's control flow using the fakes described above;
cryptographic verification is checked separately above.

The earlier 80 pseudo-terminal transport checks and old owner harness ran
against the previous platform revision. They have not been repeated as that
same matrix and are not current migration evidence.

## Diagnostics

No demo-owned firmware source warning was found. Existing provider diagnostics
remain: ST HAL and Arm micro-ecc unused parameters, the one-core RP2350 kernel's
unused doorbell variable, and newlib-nano `nosys` system-call stubs. The native
micro-ecc build also reports its `__int128` extension under `-Wpedantic` and a
`-Wmaybe-uninitialized` diagnostic. All recorded builds and CTests succeeded.

Picotool 2.1.1 recognizes the base-address boot ELF but does not classify the
slot-linked application ELF through its binary-start heuristic. Provision
application BIN data and metadata at the explicit addresses in the bootloader
guide; a generated slot UF2 alone is not complete provisioning. The SDK linker
source remains unchanged; the demo generates its Flash layout in the build
directory.
