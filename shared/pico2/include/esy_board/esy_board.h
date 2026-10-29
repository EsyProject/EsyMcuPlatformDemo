/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_PICO2_ESY_BOARD_H
#define DEMO_PICO2_ESY_BOARD_H

#include "esy_common/defs/board/esy_board_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Keep the vector table established by the Pico SDK runtime.
 * Call through esy_hal_system_init() before starting the scheduler. The board
 * does not launch core 1 or install an interrupt, timer, DMA or stdio owner.
 */
void esy_board_init(void);

/** @brief Copy the Pico 2 150 MHz system-clock request; false for a null output. */
bool esy_board_get_system_config(esy_system_config_t *config);

/** @brief Copy the demonstration's two-slot layout within Pico 2's 4 MiB Flash. */
bool esy_board_get_bootloader_layout(esy_bootloader_layout_t *layout);

/** @brief Configure UART0 TX GPIO0 and RX GPIO1 and complete IRQ metadata.
 * Only physical controller 0 is wired. The caller serializes lifecycle and
 * owns polling transfers; this function does not retain inner or enable IRQs.
 */
void esy_board_uart_configure_pins(uint8_t index,
                                 esy_uart_inner_config_t *inner);

/** @brief Disconnect UART0 pin functions and pulls after UART teardown. */
void esy_board_uart_reset_pins(uint8_t index);

/** @brief Select active or disconnected idle/sleep UART0 pads.
 * The caller suspends hardware before changing to idle/sleep. Invalid states
 * return INVALID_ARG; another controller returns NOT_SUPPORTED. Errors do not
 * change the previous pin state. This function is not an ISR entry.
 */
esy_err_t esy_board_uart_select_pin_state(
    uint8_t index, esy_board_controller_pin_state_t state);

/** @brief Report no DMA route: both update examples use polling UART. */
bool esy_board_uart_get_dma_info(uint8_t index, esy_dma_inner_config_t *inner,
                               bool is_tx);

/** @brief Return metadata for the board seam; no DMA channel is assigned. */
uint8_t esy_board_dma_get_irq_index(uint32_t dreq, uint8_t channel);
uint8_t esy_board_dma_get_irq_priority(uint32_t dreq, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
