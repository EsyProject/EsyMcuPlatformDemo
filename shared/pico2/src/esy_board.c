/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esy_board/esy_board.h"

#include "hardware/gpio.h"
#include "pico.h"

#define DEMO_SYSTEM_CLOCK_KHZ 150000u
#define DEMO_UART_TX_GPIO 0u
#define DEMO_UART_RX_GPIO 1u
#define DEMO_UART_IRQ_PRIORITY 0x80u
#define DEMO_SLOT_A_ADDRESS 0x10040000u
#define DEMO_SLOT_B_ADDRESS 0x100a0000u
#define DEMO_SLOT_BYTES (384u * 1024u)
#define DEMO_META_A_ADDRESS 0x10020000u
#define DEMO_META_B_ADDRESS 0x10021000u
#define DEMO_META_BYTES 4096u

_Static_assert(PICO_FLASH_SIZE_BYTES == (4u * 1024u * 1024u),
               "This layout requires the Pico 2 4 MiB Flash board definition");

void esy_board_init(void) {
  /* SDK reset startup establishes this image's RAM vector table. Preserve it
   * so the native RTOS port can install handlers through the SDK IRQ API. */
}

bool esy_board_get_system_config(esy_system_config_t *config) {
  if (config == NULL) return false;
  *config = (esy_system_config_t){
      .sys_clock_khz = DEMO_SYSTEM_CLOCK_KHZ,
      .enable_us_tick = true,
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
  if (index != 0u || inner == NULL) return;
  gpio_set_function(DEMO_UART_TX_GPIO, GPIO_FUNC_UART);
  gpio_set_function(DEMO_UART_RX_GPIO, GPIO_FUNC_UART);
  gpio_pull_up(DEMO_UART_RX_GPIO);
  inner->irq_priority = DEMO_UART_IRQ_PRIORITY;
}

void esy_board_uart_reset_pins(uint8_t index) {
  if (index != 0u) return;
  gpio_set_function(DEMO_UART_TX_GPIO, GPIO_FUNC_NULL);
  gpio_set_function(DEMO_UART_RX_GPIO, GPIO_FUNC_NULL);
  gpio_disable_pulls(DEMO_UART_TX_GPIO);
  gpio_disable_pulls(DEMO_UART_RX_GPIO);
}

esy_err_t esy_board_uart_select_pin_state(
    uint8_t index, esy_board_controller_pin_state_t state) {
  if (index != 0u) return ESY_ERR_NOT_SUPPORTED;
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
  return DEMO_UART_IRQ_PRIORITY;
}
