/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DEMO_SECURITY_H
#define DEMO_SECURITY_H
#include <stdbool.h>
#include <stdint.h>
/** Verify a SHA-256 digest with the compiled-in demo P-256 public key.
 * Synchronous caller context; no allocation or retained pointers. The optional
 * context is unused. Returns false for invalid arguments or invalid signatures.
 */
bool demo_verify_signature(void *context, const uint8_t digest[32],
                           const uint8_t signature[64]);
#endif
