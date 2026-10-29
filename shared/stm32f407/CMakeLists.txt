# SPDX-FileCopyrightText: 2026 EsyProject contributors
#
# SPDX-License-Identifier: Apache-2.0

set(demo_dependency_root "${CMAKE_CURRENT_LIST_DIR}/../../external")
set(demo_hal_root "${demo_dependency_root}/stm32f4xx-hal-driver")
set(demo_device_root "${demo_dependency_root}/cmsis-device-f4")
set(demo_cmsis_root "${demo_dependency_root}/CMSIS_5")

foreach(demo_dependency_file IN ITEMS
        "${demo_hal_root}/Src/stm32f4xx_hal.c"
        "${demo_device_root}/Include/stm32f407xx.h"
        "${demo_cmsis_root}/CMSIS/Core/Include/core_cm4.h")
    if(NOT EXISTS "${demo_dependency_file}")
        message(FATAL_ERROR "Missing public dependency: ${demo_dependency_file}. Initialize the repository submodules.")
    endif()
endforeach()

add_library(STM32_Drivers STATIC
    "${demo_hal_root}/Src/stm32f4xx_hal.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_cortex.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_dma.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_flash.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_flash_ex.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_gpio.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_pwr.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_pwr_ex.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_rcc.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_rcc_ex.c"
    "${demo_hal_root}/Src/stm32f4xx_hal_uart.c")
target_include_directories(STM32_Drivers PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${demo_cmsis_root}/CMSIS/Core/Include"
    "${demo_device_root}/Include"
    "${demo_hal_root}/Inc")
target_compile_definitions(STM32_Drivers PUBLIC STM32F407xx USE_HAL_DRIVER)
target_link_libraries(STM32_Drivers PUBLIC esy_opt_default)

add_library(esy_board STATIC src/esy_board.c)
target_include_directories(esy_board PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(esy_board PUBLIC esy_common_defs PRIVATE STM32_Drivers)
