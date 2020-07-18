/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>

#include <sunxi_def.h>
#include <sunxi_mmap.h>
#include <sunxi_private.h>

static const mmap_region_t sunxi_mmap[PLATFORM_MMAP_REGIONS + 1] = {
	MAP_REGION_FLAT(SUNXI_SRAM_BASE, SUNXI_SRAM_SIZE,
			MT_RW_DATA | MT_SECURE),
	MAP_REGION_FLAT(SUNXI_SCP_BASE, SUNXI_SCP_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE | MT_EXECUTE_NEVER),
	MAP_REGION_FLAT(SUNXI_DEV_BASE, SUNXI_DEV_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE | MT_EXECUTE_NEVER),
	MAP_REGION(SUNXI_DRAM_BASE, SUNXI_DRAM_VIRT_BASE, SUNXI_DRAM_SEC_SIZE,
		   MT_RW_DATA | MT_SECURE),
	MAP_REGION(PLAT_SUNXI_NS_IMAGE_OFFSET,
		   SUNXI_DRAM_VIRT_BASE + SUNXI_DRAM_SEC_SIZE,
		   SUNXI_DRAM_MAP_SIZE,
		   MT_RO_DATA | MT_NS),
	{},
};

unsigned int plat_get_syscnt_freq2(void)
{
	return SUNXI_OSC24M_CLK_IN_HZ;
}

uintptr_t plat_get_ns_image_entrypoint(void)
{
#ifdef PRELOADED_BL33_BASE
	return PRELOADED_BL33_BASE;
#else
	return PLAT_SUNXI_NS_IMAGE_OFFSET;
#endif
}

void sunxi_configure_mmu_el3(int flags)
{
	mmap_add_region(BL_CODE_BASE, BL_CODE_BASE,
			BL_CODE_END - BL_CODE_BASE,
			MT_CODE | MT_SECURE);
	mmap_add_region(BL_RO_DATA_BASE, BL_RO_DATA_BASE,
			BL_RO_DATA_END - BL_RO_DATA_BASE,
			MT_RO_DATA | MT_SECURE);
	mmap_add_region(BL_COHERENT_RAM_BASE, BL_COHERENT_RAM_BASE,
			BL_COHERENT_RAM_END - BL_COHERENT_RAM_BASE,
			MT_DEVICE | MT_RW | MT_SECURE | MT_EXECUTE_NEVER);

	mmap_add(sunxi_mmap);
	init_xlat_tables();

	enable_mmu_el3(0);
}

#define SRAM_VER_REG (SUNXI_SYSCON_BASE + 0x24)
uint16_t sunxi_read_soc_id(void)
{
	uint32_t reg = mmio_read_32(SRAM_VER_REG);

	/* Set bit 15 to prepare for the SOCID read. */
	mmio_write_32(SRAM_VER_REG, reg | BIT(15));

	reg = mmio_read_32(SRAM_VER_REG);

	/* deactivate the SOCID access again */
	mmio_write_32(SRAM_VER_REG, reg & ~BIT(15));

	return reg >> 16;
}

int sunxi_init_platform_r_twi(uint16_t socid, bool use_rsb)
{
	uint32_t pin_func = 0x77;
	uint32_t device_bit;
	unsigned int reset_offset = 0xb0;

	switch (socid) {
	case SUNXI_SOC_H5:
		if (use_rsb)
			return -ENODEV;
		pin_func = 0x22;
		device_bit = BIT(6);
		break;
	case SUNXI_SOC_H6:
		if (use_rsb)
			return -ENODEV;
		pin_func = 0x33;
		device_bit = BIT(16);
		reset_offset = 0x19c;
		break;
	case SUNXI_SOC_A64:
		pin_func = use_rsb ? 0x22 : 0x33;
		device_bit = use_rsb ? BIT(3) : BIT(6);
		break;
	default:
		INFO("R_I2C/RSB on Allwinner 0x%x SoC not supported\n", socid);
		return -ENODEV;
	}

	/* un-gate R_PIO clock */
	if (socid != SUNXI_SOC_H6)
		mmio_setbits_32(SUNXI_R_PRCM_BASE + 0x28, BIT(0));

	/* switch pins PL0 and PL1 to the desired function */
	mmio_clrsetbits_32(SUNXI_R_PIO_BASE + 0x00, 0xffU, pin_func);

	/* level 2 drive strength */
	mmio_clrsetbits_32(SUNXI_R_PIO_BASE + 0x14, 0x0fU, 0xaU);

	/* set both pins to pull-up */
	mmio_clrsetbits_32(SUNXI_R_PIO_BASE + 0x1c, 0x0fU, 0x5U);

	/* un-gate clock */
	if (socid != SUNXI_SOC_H6)
		mmio_setbits_32(SUNXI_R_PRCM_BASE + 0x28, device_bit);
	else
		mmio_setbits_32(SUNXI_R_PRCM_BASE + 0x19c, device_bit | BIT(0));

	/* assert, then de-assert reset of I2C/RSB controller */
	mmio_clrbits_32(SUNXI_R_PRCM_BASE + reset_offset, device_bit);
	mmio_setbits_32(SUNXI_R_PRCM_BASE + reset_offset, device_bit);

	return 0;
}
