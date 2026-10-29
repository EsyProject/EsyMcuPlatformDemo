/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esy_board/esy_board.h"

#include "stm32f4xx_hal.h"

#define DEMO_HSI_KHZ 16000u
#define DEMO_UART_PRIORITY 6u
#define DEMO_SLOT_A_ADDRESS 0x08040000u
#define DEMO_SLOT_B_ADDRESS 0x080a0000u
#define DEMO_SLOT_BYTES (384u * 1024u)
#define DEMO_META_A_ADDRESS 0x08008000u
#define DEMO_META_B_ADDRESS 0x0800c000u
#define DEMO_META_BYTES (16u * 1024u)

extern const uint32_t g_pfnVectors[];

void esy_board_init(void) {
  /* Direct boot enters the reset handler with PRIMASK set. Both startup paths
   * reach this function before creating any interrupt-driven resource. */
  SCB->VTOR = (uint32_t)(uintptr_t)g_pfnVectors;
  __DSB();
  __ISB();
  SystemCoreClockUpdate();
  __enable_irq();
}

bool esy_board_get_system_config(esy_system_config_t *config) {
  if (config == NULL) return false;
  *config = (esy_system_config_t){
      .sys_clock_khz = DEMO_HSI_KHZ,
      .enable_us_tick = false,
  };
  return true;
}

bool esy_board_get_bootloader_layout(esy_bootloader_layout_t *layout) {
  if (layout == NULL) return false;
  *layout = (esy_bootloader_layout_t){
      .slot_addr = {DEMO_SLOT_A_ADDRESS, DEMO_SLOT_B_ADDRESS},
      .meta_addr = {DEMO_META_A_ADDRESS, DEMO_META_B_ADDRESS},
      .slot_size = DEMO_SLOT_BYTES,
      .meta_size = {DEMO_META_BYTES, DEMO_META_BYTES},
  };
  return true;
}

void esy_board_uart_configure_pins(uint8_t index,
                                 esy_uart_inner_config_t *inner) {
  if (index != 1u || inner == NULL) return;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  GPIO_InitTypeDef config = {
      .Pin = GPIO_PIN_9 | GPIO_PIN_10,
      .Mode = GPIO_MODE_AF_PP,
      .Pull = GPIO_PULLUP,
      .Speed = GPIO_SPEED_FREQ_HIGH,
      .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &config);
  inner->irq_priority = DEMO_UART_PRIORITY;
}

void esy_board_uart_reset_pins(uint8_t index) {
  if (index != 1u) return;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef config = {
      .Pin = GPIO_PIN_9 | GPIO_PIN_10,
      .Mode = GPIO_MODE_ANALOG,
      .Pull = GPIO_NOPULL,
  };
  HAL_GPIO_Init(GPIOA, &config);
}

esy_err_t esy_board_uart_select_pin_state(
    uint8_t index, esy_board_controller_pin_state_t state) {
  if (index != 1u) return ESY_ERR_NOT_SUPPORTED;
  if (state == ESY_BOARD_CONTROLLER_PIN_STATE_DEFAULT) {
    esy_uart_inner_config_t inner;
    esy_board_uart_configure_pins(index, &inner);
  } else if (state == ESY_BOARD_CONTROLLER_PIN_STATE_IDLE ||
             state == ESY_BOARD_CONTROLLER_PIN_STATE_SLEEP) {
    esy_board_uart_reset_pins(index);
  } else {
    return ESY_ERR_INVALID_ARG;
  }
  return ESY_OK;
}

bool esy_board_uart_get_dma_info(uint8_t index, esy_dma_inner_config_t *inner,
                               bool is_tx) {
  (void)index;
  (void)inner;
  (void)is_tx;
  return false;
}

uint8_t esy_board_dma_get_irq_index(uint32_t dreq, uint8_t channel) {
  (void)dreq;
  (void)channel;
  return 0u;
}

uint8_t esy_board_dma_get_irq_priority(uint32_t dreq, uint8_t channel) {
  (void)dreq;
  (void)channel;
  return DEMO_UART_PRIORITY;
}
