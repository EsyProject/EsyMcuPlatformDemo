/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include "demo_security.h"
#include "demo_public_key.h"
#include "uECC.h"
#include <stddef.h>
bool demo_verify_signature(void *context, const uint8_t digest[32],
                           const uint8_t signature[64]) {
  (void)context;
  if (digest == NULL || signature == NULL) return false;
  return uECC_verify(k_demo_public_key, digest, 32u, signature,
                     uECC_secp256r1()) == 1;
}
