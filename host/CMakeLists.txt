# SPDX-FileCopyrightText: 2026 EsyProject contributors
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20)
project(EsyMcuDemoHost VERSION 0.1.0 LANGUAGES C)
if(NOT UNIX)
    message(FATAL_ERROR "The serial uploader currently requires a POSIX host (Linux/macOS).")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
add_executable(demo_host demo_host.c
    ../shared/security/demo_security.c ../external/micro-ecc/uECC.c)
target_include_directories(demo_host PRIVATE ../shared/security ../external/micro-ecc)
target_compile_definitions(demo_host PRIVATE _POSIX_C_SOURCE=200809L
    uECC_SUPPORTS_secp160r1=0 uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0 uECC_SUPPORTS_secp256k1=0
    uECC_SUPPORT_COMPRESSED_POINT=0
    DEMO_TEST_KEY="${CMAKE_CURRENT_LIST_DIR}/../keys/demo-signing-key.pem")
target_compile_options(demo_host PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(demo_host PRIVATE OpenSSL::Crypto)
enable_testing()
add_test(NAME signing_roundtrip COMMAND demo_host selftest)

option(DEMO_TEST_STATIC_FORMATTER "Test the STM32 hello buffer formatter" OFF)
if(DEMO_TEST_STATIC_FORMATTER)
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../external/printf/printf.c")
        message(FATAL_ERROR "Initialize external/printf to test the static formatter.")
    endif()
    add_subdirectory(../shared/format "${CMAKE_BINARY_DIR}/static_format")
    add_executable(formatter_contract ../shared/format/tests/formatter_contract.c)
    target_link_libraries(formatter_contract PRIVATE demo_static_format)
    target_compile_options(formatter_contract PRIVATE
        -fno-builtin-snprintf -fno-builtin-vsnprintf -U_FORTIFY_SOURCE)
    if(APPLE)
        target_link_options(formatter_contract PRIVATE -Wl,-dead_strip)
    else()
        target_link_options(formatter_contract PRIVATE -Wl,--gc-sections)
    endif()
    add_test(NAME formatter_contract COMMAND formatter_contract)
endif()
