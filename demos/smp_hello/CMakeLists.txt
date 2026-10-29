# SPDX-FileCopyrightText: 2026 EsyProject contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.24)

get_filename_component(DEMO_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(ESY_MCU_PLATFORM_DIR "${DEMO_REPOSITORY_ROOT}/external/EsyMcuPlatform")
set(ESY_TARGET_DEVICE PICO2 CACHE STRING "PICO2 or STM32F407ZGT6")
include("${ESY_MCU_PLATFORM_DIR}/cmake/esy_target_device.cmake")
esy_configure_target_device()
if(ESY_TARGET_DEVICE STREQUAL "PICO2")
    set(demo_specs_platform pico)
    set(ESY_BOARD_PATH "${CMAKE_CURRENT_LIST_DIR}/esy_board")
elseif(ESY_TARGET_DEVICE MATCHES "^STM32F407ZG(T6)?$")
    set(demo_specs_platform stm32f4)
    set(ESY_BOARD_PATH "${DEMO_REPOSITORY_ROOT}/shared/stm32f407")
    set(ESY_STM32_SUPPORT_ROOT "")
    set(ESY_SEGGER_RTT_ROOT "${DEMO_REPOSITORY_ROOT}/external/SEGGER_RTT")
    set(ESY_LINKER_SCRIPT_DEFAULT "${CMAKE_CURRENT_LIST_DIR}/linker/standalone.ld")
else()
    message(FATAL_ERROR "smp_hello supports PICO2 or STM32F407ZG/STM32F407ZGT6 only.")
endif()
if(DEFINED ESY_APP_SLOT)
    message(FATAL_ERROR "smp_hello is a standalone image; do not set ESY_APP_SLOT.")
endif()

# Every consumer layout input is visible here, before importing the SDK.
set(ESY_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(ESY_BUILD_CONFIG_FILE "")
set(ESY_FEATURE_IMPORT_FILE "${CMAKE_CURRENT_LIST_DIR}/esy_feature_import.cmake")
set(ESY_SPECS_DIR "${CMAKE_CURRENT_LIST_DIR}/specs/${demo_specs_platform}")
set(ESY_RTOS_CONFIG_DIR "${ESY_SPECS_DIR}/include/specs")
set(ESY_VERSION_TEMPLATE "")
set(GENERATED_INCLUDE_DIR "")
set(FREERTOS_KERNEL_PATH "${DEMO_REPOSITORY_ROOT}/external/FreeRTOS-Kernel")
include("${ESY_MCU_PLATFORM_DIR}/cmake/esy_early_import.cmake")
if(ESY_TARGET_DEVICE STREQUAL "PICO2")
    set(ESY_LINKER_SCRIPT_DEFAULT
        "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2350/memmap_default.ld")
endif()

project(smp_hello VERSION 1.0.0 LANGUAGES C CXX ASM)
include("${ESY_MCU_PLATFORM_DIR}/cmake/esy_project_import.cmake")
add_subdirectory("${ESY_MCU_PLATFORM_DIR}" "${CMAKE_BINARY_DIR}/esy_platform")

add_executable(smp_hello src/main.c)
target_include_directories(smp_hello PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/specs/common/include")
target_link_libraries(smp_hello PRIVATE esy_hal esy_os esy_logger)
if(ESY_TARGET_PLATFORM STREQUAL "STM32F4")
    add_subdirectory("${DEMO_REPOSITORY_ROOT}/shared/format"
        "${CMAKE_BINARY_DIR}/demo_static_format")
    target_link_libraries(smp_hello PRIVATE demo_static_format)
    include("${ESY_BOARD_PATH}/firmware.cmake")
    demo_stm32_add_startup(smp_hello standalone)
endif()
set(ESY_FIRMWARE_TARGET smp_hello)
include("${ESY_MCU_PLATFORM_DIR}/cmake/esy_firmware.cmake")
