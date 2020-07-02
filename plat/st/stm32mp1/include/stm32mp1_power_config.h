/*
 * Copyright (c) 2017-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP1_POWER_CONFIG_H
#define STM32MP1_POWER_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define PSCI_MODE_SYSTEM_SUSPEND	0
#define PSCI_MODE_SYSTEM_OFF		1

void stm32mp1_init_lp_states(void);
uint32_t stm32mp1_get_lp_soc_mode(uint32_t psci_mode);
bool stm32mp1_get_retram_enabled(void);

#endif /* STM32MP1_POWER_CONFIG_H */
