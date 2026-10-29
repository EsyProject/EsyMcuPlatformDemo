# SPDX-FileCopyrightText: 2026 EsyProject contributors
#
# SPDX-License-Identifier: Apache-2.0

if(NOT TARGET pico_time)
    message(FATAL_ERROR "The Pico 2 board requires an initialized Pico SDK.")
endif()

# Leave the SDK default alarm pool uninitialized so its callbacks and IRQ
# ownership cannot cross the direct transfer into another image's SDK startup.
target_compile_definitions(pico_time INTERFACE PICO_TIME_DEFAULT_ALARM_POOL_DISABLED=1)

# Reserve all of SCRATCH_Y for the startup/boot stack. Verification call depth
# still needs measurement on hardware; the SDK default stack is only 2 KiB.
target_compile_definitions(pico_crt0 INTERFACE PICO_STACK_SIZE=4096)

add_library(esy_board STATIC src/esy_board.c)
target_include_directories(esy_board PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(esy_board PUBLIC esy_common_defs
    PRIVATE esy_opt_default hardware_gpio hardware_sync)
