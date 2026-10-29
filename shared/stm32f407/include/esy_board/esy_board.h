/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_STM32F407_ESY_BOARD_H
#define DEMO_STM32F407_ESY_BOARD_H

#include "esy_common/defs/board/esy_board_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Restore this image's vector table and unmask IRQs before HAL startup.
 * Startup and both demo images keep the internal 16 MHz HSI clock. Call only
 * through esy_hal_system_init(), before starting the scheduler or peripherals.
 */
void esy_board_init(void);

/** @brief Copy the fixed HSI clock description; return false for a null output. */
bool esy_board_get_system_config(esy_system_config_t *config);

/** @brief Copy the two-slot 1 MiB Flash layout; return false for a null output. */
bool esy_board_get_bootloader_layout(esy_bootloader_layout_t *layout);

/** @brief Configure USART1 PA9/PA10 AF7 and complete its IRQ priority.
 * Only controller 1 is wired by this example board. The caller serializes its
 * lifecycle; this function is not an ISR entry and does not enable an IRQ.
 */
void esy_board_uart_configure_pins(uint8_t index,
                                 esy_uart_inner_config_t *inner);

/** @brief Return USART1 pins to analog state after native UART teardown. */
void esy_board_uart_reset_pins(uint8_t index);

/** @brief Select active pins or an analog idle/sleep state for USART1.
 * Return ESY_ERR_NOT_SUPPORTED for another controller and INVALID_ARG for an
 * unknown state. The peripheral owner must suspend hardware before idle/sleep.
 */
esy_err_t esy_board_uart_select_pin_state(
    uint8_t index, esy_board_controller_pin_state_t state);

/** @brief Report no DMA routing: these demonstrations use polling UART only. */
bool esy_board_uart_get_dma_info(uint8_t index, esy_dma_inner_config_t *inner,
                               bool is_tx);

/** @brief Return IRQ metadata required by the HAL seam; no DMA route is owned. */
uint8_t esy_board_dma_get_irq_index(uint32_t dreq, uint8_t channel);
uint8_t esy_board_dma_get_irq_priority(uint32_t dreq, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
