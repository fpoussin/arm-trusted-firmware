/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <platform_def.h>

#include <drivers/st/stm32mp1_clk.h>
#include <dt-bindings/clock/stm32mp1-clks.h>
#include <lib/mmio.h>

#include <stm32mp1_context.h>

#define TAMP_BOOT_ITF_BACKUP_REG_ID	U(20)
#define TAMP_BOOT_ITF_MASK		U(0x0000FF00)
#define TAMP_BOOT_ITF_SHIFT		8

int stm32_save_boot_interface(uint32_t interface, uint32_t instance)
{
	uint32_t bkpr_itf_idx = tamp_bkpr(TAMP_BOOT_ITF_BACKUP_REG_ID);

	stm32mp_clk_enable(RTCAPB);

	mmio_clrsetbits_32(bkpr_itf_idx,
			   TAMP_BOOT_ITF_MASK,
			   ((interface << 4) | (instance & 0xFU)) <<
			   TAMP_BOOT_ITF_SHIFT);

	stm32mp_clk_disable(RTCAPB);

	return 0;
}

int stm32_get_boot_interface(uint32_t *interface, uint32_t *instance)
{
	uint32_t itf;
	uint32_t bkpr = tamp_bkpr(TAMP_BOOT_ITF_BACKUP_REG_ID);

	stm32mp_clk_enable(RTCAPB);

	itf = (mmio_read_32(bkpr) & TAMP_BOOT_ITF_MASK) >> TAMP_BOOT_ITF_SHIFT;

	stm32mp_clk_disable(RTCAPB);

	*interface = itf >> 4;
	*instance = itf & 0xFU;

	return 0;
}
