/*
 * Copyright (c) 2017-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP1_DDR_HELPERS_H
#define STM32MP1_DDR_HELPERS_H

#include <stdint.h>

void ddr_enable_clock(void);
int ddr_sw_self_refresh_exit(void);
uint32_t ddr_get_io_calibration_val(void);
int ddr_standby_sr_entry(void);
void ddr_sr_mode_ssr(void);
void ddr_sr_mode_asr(void);
void ddr_sr_mode_hsr(void);

#endif /* STM32MP1_DDR_HELPERS_H */
