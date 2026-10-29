# SPDX-FileCopyrightText: 2026 EsyProject contributors
# SPDX-License-Identifier: Apache-2.0

add_library(demo_static_format OBJECT stdio_bridge.c
    "${CMAKE_CURRENT_LIST_DIR}/../../external/printf/printf.c")
target_include_directories(demo_static_format PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../external/printf")
target_compile_definitions(demo_static_format PRIVATE
    PRINTF_DISABLE_SUPPORT_FLOAT PRINTF_DISABLE_SUPPORT_EXPONENTIAL
    PRINTF_DISABLE_SUPPORT_LONG_LONG)
target_compile_options(demo_static_format PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-ffunction-sections>
    $<$<C_COMPILER_ID:GNU,Clang>:-fdata-sections>)
if(TARGET esy_opt_default)
    target_link_libraries(demo_static_format PRIVATE esy_opt_default)
endif()
