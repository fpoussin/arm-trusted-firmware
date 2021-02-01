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
#include <drivers/st/stm32mp_pmic.h>
#include <lib/spinlock.h>
#include <lib/utils.h>
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
static uint16_t boot_itf_selected;
static uint32_t boot_action_saved;

void stm32mp_save_boot_ctx_address(uintptr_t address)
{
	boot_api_context_t *boot_context = (boot_api_context_t *)address;

	boot_ctx_address = address;
	boot_itf_selected = boot_context->boot_interface_selected;
	boot_action_saved = boot_context->boot_action;
}

uintptr_t stm32mp_get_boot_ctx_address(void)
{
	return boot_ctx_address;
}

uint16_t stm32mp_get_boot_itf_selected(void)
{
	return boot_itf_selected;
}

uint32_t stm32mp_get_boot_action(void)
{
	return boot_action_saved;
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

#if STM32MP_USE_STM32IMAGE
int stm32mp_check_header(boot_api_image_header_t *header, uintptr_t buffer)
{
	/*
	 * Check header/payload validity:
	 *	- Header magic
	 *	- Header version
	 *	- Payload checksum if no signature verification
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

	if (header->option_flags == 1U) {
		uint32_t i;
		uint32_t img_checksum = 0U;

		for (i = 0U; i < header->image_length; i++) {
			img_checksum += *(uint8_t *)(buffer + i);
		}

		if (header->payload_checksum != img_checksum) {
			ERROR("Checksum: 0x%x (awaited: 0x%x)\n", img_checksum,
			      header->payload_checksum);
			return -EINVAL;
		}
	}

	return 0;
}
#endif

/* Return CPU supply name */
const char *stm32mp_get_cpu_supply_name(void)
{
	const char *regulator;
	const char *supply = NULL;

	regulator = dt_get_cpu_regulator_name();
	if (regulator == NULL) {
		return NULL;
	}

	if (dt_pmic_status() > 0) {
		if (dt_pmic_find_supply(&supply, regulator) != 0) {
			return NULL;
		}
	}

	return supply;
}

/* Return VDD supply name */
const char *stm32mp_get_vdd_supply_name(void)
{
	const char *supply = NULL;

	if (dt_pmic_status() > 0) {
		const char *regulator = dt_get_vdd_regulator_name();

		if (regulator != NULL) {
			dt_pmic_find_supply(&supply, regulator);
		}
	}

	return supply;
}

/* Return USB phy supply name */
const char *stm32mp_get_usb_phy_supply_name(void)
{
	const char *supply = NULL;

	if (dt_pmic_status() > 0) {
		const char *regulator = dt_get_usb_phy_regulator_name();

		if (regulator != NULL) {
			dt_pmic_find_supply(&supply, regulator);
		}
	}

	return supply;
}

#if TRUSTED_BOARD_BOOT && STM32MP_USE_STM32IMAGE
/* Save pointer to last loaded header */
static boot_api_image_header_t *latest_stm32_header;

/* Save last loaded header */
void stm32mp_save_loaded_header(void *header)
{
	assert(latest_stm32_header == NULL);

	latest_stm32_header = header;
}

/* Discard last loaded header */
void stm32mp_delete_loaded_header(void)
{
	if (latest_stm32_header == NULL) {
		return;
	}

	zeromem(latest_stm32_header, sizeof(boot_api_image_header_t));
	latest_stm32_header = NULL;
}

/* Get last loaded header */
boot_api_image_header_t *stm32mp_get_loaded_header(void)
{
	assert(latest_stm32_header != NULL);

	return latest_stm32_header;
}
#endif /* TRUSTED_BOARD_BOOT */

int stm32mp_map_ddr_non_cacheable(void)
{
	return  mmap_add_dynamic_region(STM32MP_DDR_BASE, STM32MP_DDR_BASE,
					STM32MP_DDR_MAX_SIZE,
					MT_NON_CACHEABLE | MT_RW | MT_SECURE);
}

int stm32mp_unmap_ddr(void)
{
	return  mmap_remove_dynamic_region(STM32MP_DDR_BASE,
					   STM32MP_DDR_MAX_SIZE);
}
