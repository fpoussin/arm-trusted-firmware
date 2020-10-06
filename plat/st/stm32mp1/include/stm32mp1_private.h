/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP1_PRIVATE_H
#define STM32MP1_PRIVATE_H

#include <stdint.h>

enum boot_device_e {
	BOOT_DEVICE_USB,
	BOOT_DEVICE_BOARD
};

void configure_mmu(void);

void stm32mp1_arch_security_setup(void);
void stm32mp1_security_setup(void);
#if STM32MP_UART_PROGRAMMER
uintptr_t get_uart_address(uint32_t instance_nb);
#endif

enum boot_device_e get_boot_device(void);

void stm32mp1_gic_pcpu_init(void);
void stm32mp1_gic_init(void);

void stm32mp1_syscfg_init(void);
void stm32mp1_syscfg_enable_io_compensation(void);
void stm32mp1_syscfg_disable_io_compensation(void);

uint32_t stm32mp_get_ddr_ns_size(void);

void stm32mp1_init_scmi_server(void);
#endif /* STM32MP1_PRIVATE_H */
