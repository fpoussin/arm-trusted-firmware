/*
 * Copyright (c) 2017-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <stm32mp1_smc.h>

#include "rcc_svc.h"

uint32_t rcc_cal_scv_handler(uint32_t x1)
{
	uint32_t ret = STM32_SMC_FAILED;

	switch (x1) {
	case CK_CSI:
		if (stm32mp1_calib_start_csi_cal() ==  0) {
			ret = STM32_SMC_OK;
		}
		break;

	case CK_HSI:
		if (stm32mp1_calib_start_hsi_cal() == 0) {
			ret = STM32_SMC_OK;
		}
		break;

	default:
		ret = STM32_SMC_INVALID_PARAMS;
		break;
	}

	return ret;
}

uint32_t rcc_opp_scv_handler(uint32_t x1, uint32_t x2, uint32_t *res)
{
	uint32_t cmd = x1;
	uint32_t opp = x2 / 1000U; /* KHz */

	switch (cmd) {
	case STM32_SMC_RCC_OPP_SET:
		if (stm32mp1_set_opp_khz(opp) != 0) {
			return STM32_SMC_FAILED;
		}
		break;

	case STM32_SMC_RCC_OPP_ROUND:
		if (stm32mp1_round_opp_khz(&opp) != 0) {
			return STM32_SMC_FAILED;
		}

		if (opp > (UINT32_MAX / 1000U)) {
			return STM32_SMC_FAILED;
		}

		*res = opp * 1000U;
		break;

	default:
		return STM32_SMC_INVALID_PARAMS;
	}

	return STM32_SMC_OK;
}
