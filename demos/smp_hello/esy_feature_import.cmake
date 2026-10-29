# SPDX-FileCopyrightText: 2026 EsyProject contributors
# SPDX-License-Identifier: Apache-2.0

# This is the only feature source for the demo; SDK stdio stays disabled.
set(ESY_RTOS FreeRTOS)
set(ENABLE_ESY_SYSTEM ON)
set(ENABLE_ESY_LOGGER ON)
set(ENABLE_ESY_LOGGER_BACKEND_RTT ON)
set(ENABLE_ESY_STDIO_UART OFF)
set(ENABLE_ESY_STDIO_RTT OFF)
