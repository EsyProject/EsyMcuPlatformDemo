/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_PICO_FREERTOS_CONFIG_H
#define DEMO_PICO_FREERTOS_CONFIG_H

/* The native RP2350 port supplies FREE_RTOS_KERNEL_SMP. This Flash-writing
 * example intentionally schedules one core and never launches core 1. The
 * default Flash safe-execute hook masks only the calling core's interrupts. */
#define configNUMBER_OF_CORES 1
#define configNUM_CORES configNUMBER_OF_CORES
#define configTICK_CORE 0
#define configUSE_CORE_AFFINITY 0
#define configRUN_MULTIPLE_PRIORITIES 0
#define configCPU_CLOCK_HZ 150000000UL
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 0
#define configUSE_TICKLESS_IDLE 0
#define configTICK_RATE_HZ 1000
#define configMAX_PRIORITIES 4
#define configMINIMAL_STACK_SIZE 256
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configSTACK_DEPTH_TYPE uint32_t
#define configSUPPORT_STATIC_ALLOCATION 1
#define configKERNEL_PROVIDED_STATIC_MEMORY 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define configUSE_NEWLIB_REENTRANT 0
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_TIMERS 0
/* The pinned kernel still compiles its unused static timer-storage callback. */
#define configTIMER_TASK_STACK_DEPTH 128
#define configUSE_TRACE_FACILITY 0
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configUSE_IDLE_HOOK 0
#define configUSE_PASSIVE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 0
/* This polling owner needs neither cross-core SDK wakeups nor SDK sleep
 * translation. Disabling both avoids a timer-daemon dependency in this port. */
#define configSUPPORT_PICO_SYNC_INTEROP 0
#define configSUPPORT_PICO_TIME_INTEROP 0
#define configENABLE_MPU 0
#define configENABLE_TRUSTZONE 0
#define configRUN_FREERTOS_SECURE_ONLY 1
#define configENABLE_FPU 1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16
#define configASSERT(condition) do { if (!(condition)) __builtin_trap(); } while (0)

#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_eTaskGetState 1

#endif
