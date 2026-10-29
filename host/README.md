<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# Native package and serial utility

## Purpose

Build a small C host program to sign application images, produce factory
metadata, inspect the running firmware and transfer a candidate over UART.
It uses OpenSSL 3 for hashing/signing and the same micro-ecc public-key verifier
as the MCU to check signature compatibility before sending anything.

## Public Surface

```text
demo_host pack KEY.pem IMAGE.bin VERSION OUTPUT_PREFIX
demo_host verify PACKAGE.esy
demo_host upload SERIAL_PORT PACKAGE.esy
demo_host info SERIAL_PORT
demo_host export-key KEY.pem OUTPUT.h
demo_host selftest
```

`pack` creates `OUTPUT_PREFIX.esy` and `OUTPUT_PREFIX.meta.bin`. The first is a
112-byte transport header followed by the raw image. The second is a 256-byte
Esy boot metadata record in CONFIRMED state, for initial factory provisioning.
Normal uploads publish TRY_BOOT metadata through the library; they never use
the factory metadata file. Create the output directory before packing.

The version passed to `pack` must equal the application's
`DEMO_FIRMWARE_VERSION` build input. Packaging does not rewrite the application.

## Integration

First build the A/version 1 and B/version 2 firmware images using the
[OTA example instructions](../demos/ota_update/README.md#integration). Then run
from the repository root on Linux/macOS. This first snippet uses the STM32
paths from that guide:

```sh
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
build/host/demo_host info /dev/ttyUSB0
build/host/demo_host upload /dev/ttyUSB0 out/version2.esy
```

For the Pico 2 images from the same guide, use their Pico-specific BIN paths:

```sh
mkdir -p out/pico
build/host/demo_host pack keys/demo-signing-key.pem \
  build/pico-ota-a/ota_update.bin 1 out/pico/version1
build/host/demo_host pack keys/demo-signing-key.pem \
  build/pico-ota-b/ota_update.bin 2 out/pico/version2
build/host/demo_host verify out/pico/version2.esy
build/host/demo_host upload /dev/ttyUSB0 out/pico/version2.esy
```

The packer recognizes the supported board and slot address from the image's
vectors. The same header and uploader serve both boards. Provision the matching
bootloader, baseline and signed metadata before using `info` or `upload`.

The serial port requires read/write permission and exclusive ownership; close
terminal emulators before using it. Linux ports commonly appear as
`/dev/ttyUSB0`; macOS commonly uses `/dev/cu.usbserial-*`. Native Windows serial
support is not implemented. An external sender can implement the documented
protocol without using this utility.

The sample signing key is intentionally included and publicly known. To use a
different demonstration identity, generate a P-256 key and export its public key:

```sh
openssl ecparam -name prime256v1 -genkey -noout -out my-key.pem
build/host/demo_host export-key my-key.pem shared/security/demo_public_key.h
cmake --build build/host
```

Rebuild **both** bootloader and applications after changing the public key.
Reprovision the bootloader and baseline image with matching signed metadata.
The host packer rejects keys that do not match its compiled verifier.
The built-in self-test uses the included sample key; when intentionally replacing
the public key, update the test-key path in the host CMake file to match.
Never store your own private signing key in the public repository.

## Contracts

The tool requires a nonempty image no larger than 384 KiB, an 8-byte-aligned
initial SRAM stack pointer, and a Thumb reset handler inside the linked image in
slot A or B. It accepts STM32 slot bases `0x08040000`/`0x080A0000` with SRAM
through `0x20020000`, or Pico 2 slot bases `0x10040000`/`0x100A0000` with SRAM
through `0x20082000`. The target independently checks the destination slot before any
image erase. It verifies the signature before accepting data and verifies the
full image SHA-256 from Flash before activation.

Each command is synchronous. The serial utility waits up to 30 seconds for each
I/O phase; the MCU allows a 2-second receive gap while a frame is pending. An
error aborts the entire upload. It does not retry an ambiguously acknowledged
chunk or resume a partial image. Wait for the target's error recovery or reset
the board, then restart with the complete package. A lost final reply can mean
the image has already been activated; query the running version after reboot.

## Resource Model

The desktop tool loads at most one 384 KiB image/package in host memory.
OpenSSL owns its normal host allocations. Firmware allocation policy does not
apply to the desktop process. The MCU verifier uses the compiled-in 64-byte
P-256 public key and synchronous micro-ecc operations without heap allocation.

## Verification

`ctest` signs a canonical metadata message with OpenSSL, verifies it using the
MCU's micro-ecc verifier, rejects modified digests/signatures and checks the
standard CRC32 check vector. To also run the STM32 hello formatter's native
contract test, initialize `external/printf`, configure the host build with
`-DDEMO_TEST_STATIC_FORMATTER=ON`, and rerun build/CTest. That test checks the
integer/string formats used by hello, length reporting, buffer truncation and
zero-capacity formatting. Additional completed checks are recorded in
[VALIDATION.md](../VALIDATION.md).

## Known Limits

The protocol is designed for this demonstration pair. It has no session IDs,
per-chunk CRC, retransmission, authentication of a live sender, encryption,
transfer resumption or discovery of the correct build slot. The signed complete
image hash protects acceptance of image contents. The host must select an image
linked for the inactive slot and a version matching the compiled application.
