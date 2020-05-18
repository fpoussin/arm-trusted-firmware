/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018, Icenowy Zheng <icenowy@aosc.io>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <platform_def.h>

#include <common/debug.h>
#include <drivers/allwinner/axp.h>
#include <drivers/allwinner/sunxi_rsb.h>

#include <sunxi_def.h>
#include <sunxi_mmap.h>
#include <sunxi_private.h>

#define AXP803_HW_ADDR	0x3a3
#define AXP803_RT_ADDR	0x2d

int axp_read(uint8_t reg)
{
	return rsb_read(AXP803_RT_ADDR, reg);
}

int axp_write(uint8_t reg, uint8_t val)
{
	return rsb_write(AXP803_RT_ADDR, reg, val);
}

static int rsb_init(void)
{
	int ret;

	ret = rsb_init_controller();
	if (ret)
		return ret;

	/* Start with 400 KHz to issue the I2C->RSB switch command. */
	ret = rsb_set_bus_speed(SUNXI_OSC24M_CLK_IN_HZ, 400000);
	if (ret)
		return ret;

	/*
	 * Initiate an I2C transaction to write 0x7c into register 0x3e,
	 * switching the PMIC to RSB mode.
	 */
	ret = rsb_set_device_mode(0x7c3e00);
	if (ret)
		return ret;

	/* Now in RSB mode, switch to the recommended 3 MHz. */
	ret = rsb_set_bus_speed(SUNXI_OSC24M_CLK_IN_HZ, 3000000);
	if (ret)
		return ret;

	/* Associate the 8-bit runtime address with the 12-bit bus address. */
	ret = rsb_assign_runtime_address(AXP803_HW_ADDR,
					 AXP803_RT_ADDR);
	if (ret)
		return ret;

	return axp_check_id();
}

int sunxi_pmic_setup(uint16_t socid, const void *fdt)
{
	int ret;

	if (socid != SUNXI_SOC_A64)
		return 0;

	INFO("PMIC: Probing AXP803 on RSB\n");

	ret = sunxi_init_platform_r_twi(socid, true);
	if (ret)
		return ret;

	ret = rsb_init();
	if (ret)
		return ret;

	axp_setup_regulators(fdt);

	return 0;
}
