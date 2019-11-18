/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/st/stm32mp_clkfunc.h>
#include <lib/spinlock.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>

#define HEADER_VERSION_MAJOR_MASK	GENMASK(23, 16)

static struct spinlock lock;

uintptr_t plat_get_ns_image_entrypoint(void)
{
	return BL33_BASE;
}

unsigned int plat_get_syscnt_freq2(void)
{
	return read_cntfrq_el0();
}

#pragma weak stm32mp_plat_reset
void __dead2 stm32mp_plat_reset(int cpu)
{
	panic();
}

static uintptr_t boot_ctx_address;

void stm32mp_save_boot_ctx_address(uintptr_t address)
{
	boot_ctx_address = address;
}

uintptr_t stm32mp_get_boot_ctx_address(void)
{
	return boot_ctx_address;
}

uintptr_t stm32mp_ddrctrl_base(void)
{
	return DDRCTRL_BASE;
}

uintptr_t stm32mp_ddrphyc_base(void)
{
	return DDRPHYC_BASE;
}

uintptr_t stm32mp_pwr_base(void)
{
	return PWR_BASE;
}

uintptr_t stm32mp_rcc_base(void)
{
	return RCC_BASE;
}

bool stm32mp_lock_available(void)
{
	const uint32_t c_m_bits = SCTLR_M_BIT | SCTLR_C_BIT;

	/* The spinlocks are used only when MMU and data cache are enabled */
	return (read_sctlr() & c_m_bits) == c_m_bits;
}

void stm32mp_pwr_regs_lock(void)
{
	if (stm32mp_lock_available()) {
		spin_lock(&lock);
	}
}

void stm32mp_pwr_regs_unlock(void)
{
	if (stm32mp_lock_available()) {
		spin_unlock(&lock);
	}
}

int stm32mp_check_header(boot_api_image_header_t *header, uintptr_t buffer)
{
	uint32_t i;
	uint32_t img_checksum = 0U;

	/*
	 * Check header/payload validity:
	 *	- Header magic
	 *	- Header version
	 *	- Payload checksum
	 */
	if (header->magic != BOOT_API_IMAGE_HEADER_MAGIC_NB) {
		ERROR("Header magic\n");
		return -EINVAL;
	}

	if ((header->header_version & HEADER_VERSION_MAJOR_MASK) !=
	    (BOOT_API_HEADER_VERSION & HEADER_VERSION_MAJOR_MASK)) {
		ERROR("Header version\n");
		return -EINVAL;
	}

	for (i = 0U; i < header->image_length; i++) {
		img_checksum += *(uint8_t *)(buffer + i);
	}

	if (header->payload_checksum != img_checksum) {
		ERROR("Checksum: 0x%x (awaited: 0x%x)\n", img_checksum,
		      header->payload_checksum);
		return -EINVAL;
	}

	return 0;
}

int stm32mp_map_ddr_non_cacheable(void)
{
	return  mmap_add_dynamic_region(STM32MP_DDR_BASE, STM32MP_DDR_BASE,
					STM32MP_DDR_MAX_SIZE,
					MT_NON_CACHEABLE | MT_RW | MT_NS);
}

int stm32mp_unmap_ddr(void)
{
	return  mmap_remove_dynamic_region(STM32MP_DDR_BASE,
					   STM32MP_DDR_MAX_SIZE);
}
