/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_STM32F4XX_HAL_CONF_H
#define DEMO_STM32F4XX_HAL_CONF_H

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

#define HSE_VALUE 8000000u
#define HSE_STARTUP_TIMEOUT 100u
#define HSI_VALUE 16000000u
#define LSI_VALUE 32000u
#define LSE_VALUE 32768u
#define LSE_STARTUP_TIMEOUT 5000u
#define EXTERNAL_CLOCK_VALUE 12288000u
#define VDD_VALUE 3300u
#define TICK_INT_PRIORITY 15u
#define USE_RTOS 0u
#define PREFETCH_ENABLE 1u
#define INSTRUCTION_CACHE_ENABLE 1u
#define DATA_CACHE_ENABLE 1u

#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_uart.h"

#define assert_param(expression) ((void)0u)

#endif
