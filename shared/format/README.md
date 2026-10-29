<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# Static buffer-formatting provider

## Purpose

Keep the STM32 `smp_hello` logger independent of newlib's allocating formatting
implementation. This adapter links the pinned, MIT-licensed mpaland/printf
provider for the two bounded formatting functions used by EsyMcuPlatform's
logger. The Pico build uses its SDK formatter instead.

## Public Surface

The `demo_static_format` CMake object target supplies strong standard-signature
`snprintf` and `vsnprintf` functions. They delegate to the provider's
`vsnprintf_` implementation. No application-facing API or console output
function is introduced.

## Integration

Initialize `external/printf`, add this directory after project configuration,
and link `demo_static_format` to the final executable. Object linking ensures
the two strong formatting definitions precede libc archive extraction.
Keep function/data sections and `--gc-sections` enabled; unused provider
console-output sections are discarded, so `_putchar` is not required.

The provider disables floating-point, exponential and `long long` formats.
Use the integer/string formats exercised by the hello logger: `%s`, `%c`,
`%u`, `%lu`, `%ld` and `%%`. This is a scoped firmware provider rather than
a complete implementation of every C formatted-output extension. The host
package utility continues to use its native libc/OpenSSL providers.

## Contracts

Calls are synchronous and reentrant, with no retained buffer or argument-list
references. Each caller supplies its own output storage. The return value is
the number of characters that would have been written, excluding the terminal
NUL; a value at least equal to capacity reports truncation. Nonzero-capacity
calls NUL-terminate their output. A zero-capacity call may pass a null buffer.
Format/argument types must match, as for standard formatted output.

The adapter does not add a lock, shared mutable state, allocation, I/O, or
task. It must not be used as an ISR logging capability: the logger's existing
task-context restrictions remain in force.

## Resource Model

Output uses only the caller's buffer. Conversion workspace is stack-local;
the provider's integer-conversion buffer budget is 32 bytes per active
conversion. Total stack includes the provider call chain and the caller's
logger formatting buffer.

## Verification

The STM32 native hello ELF was checked for the two adapter symbols and the
absence of libc/RTOS allocator symbols. Native host coverage in
`tests/formatter_contract.c` checks actual logger-style strings, unsigned
32-bit maximum and signed 32-bit minimum values, literal percent signs,
truncation, single-byte/zero-capacity output, and the variadic entry point.

## Known Limits

Floating-point and `long long` formatting are disabled. Unsupported format
specifiers are outside this provider's contract. Linking another strong
`snprintf`/`vsnprintf` implementation causes a conflict and must be resolved
explicitly. Adding other libc I/O APIs or firmware modules needs a fresh
allocation and link audit.
