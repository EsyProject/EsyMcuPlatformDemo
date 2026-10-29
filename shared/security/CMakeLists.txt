# SPDX-FileCopyrightText: 2026 EsyProject contributors
# SPDX-License-Identifier: Apache-2.0
add_library(demo_security STATIC demo_security.c
    "${DEMO_ROOT}/external/micro-ecc/uECC.c")
target_include_directories(demo_security PUBLIC "${CMAKE_CURRENT_LIST_DIR}"
    PRIVATE "${DEMO_ROOT}/external/micro-ecc")
target_compile_definitions(demo_security PRIVATE
    uECC_SUPPORTS_secp160r1=0 uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0 uECC_SUPPORTS_secp256k1=0
    uECC_SUPPORT_COMPRESSED_POINT=0 uECC_OPTIMIZATION_LEVEL=2)
target_link_libraries(demo_security PRIVATE esy_opt_default)
