# SPDX-FileCopyrightText: 2026 EsyProject contributors
#
# SPDX-License-Identifier: Apache-2.0

# Attach vendor reset startup directly to the executable so weak vector symbols
# cannot hide strong HAL/OS vector objects in archives.
function(demo_stm32_add_startup target layout)
    set(demo_board_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    set(demo_device_root "${demo_board_root}/../../external/cmsis-device-f4")
    if(NOT layout MATCHES "^(bootloader|slot_a|slot_b|standalone)$")
        message(FATAL_ERROR "Unknown STM32F407 image layout: ${layout}")
    endif()
    target_sources(${target} PRIVATE
        "${demo_device_root}/Source/Templates/system_stm32f4xx.c"
        "${demo_device_root}/Source/Templates/gcc/startup_stm32f407xx.s")
    # The board restores VTOR from the linked g_pfnVectors symbol. Keep the
    # vendor startup unmodified; its fixed-offset relocation is not enabled.
    target_link_libraries(${target} PRIVATE STM32_Drivers)
    target_link_options(${target} PRIVATE
        --specs=nosys.specs "-L${demo_board_root}/linker")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
        "${demo_board_root}/linker/image_sections.ld")
endfunction()
