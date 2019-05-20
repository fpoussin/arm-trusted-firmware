/*
 * Copyright (c) 2017-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>

#include <libfdt.h>

#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <dt-bindings/power/stm32mp1-power.h>

#include <stm32mp_dt.h>
#include <stm32mp1_power_config.h>

#define SYSTEM_SUSPEND_SUPPORTED_MODES	"system_suspend_supported_soc_modes"
#define SYSTEM_OFF_MODE			"system_off_soc_mode"

static uint32_t deepest_system_suspend_mode;
static uint32_t system_off_mode;
static uint8_t stm32mp1_supported_soc_modes[STM32_PM_MAX_SOC_MODE];

static int dt_get_pwr_node(void *fdt)
{
	return fdt_node_offset_by_compatible(fdt, -1, DT_PWR_COMPAT);
}

static void save_supported_mode(void *fdt, int pwr_node)
{
	int len;
	uint32_t count;
	unsigned int i;
	uint32_t supported[ARRAY_SIZE(stm32mp1_supported_soc_modes)];
	const void *prop;

	prop = fdt_getprop(fdt, pwr_node, SYSTEM_SUSPEND_SUPPORTED_MODES, &len);
	if (prop == NULL) {
		panic();
	}

	count = (uint32_t)len / sizeof(uint32_t);
	if (count > STM32_PM_MAX_SOC_MODE) {
		panic();
	}

	if (fdt_read_uint32_array(fdt, pwr_node, SYSTEM_SUSPEND_SUPPORTED_MODES,
				  count, &supported[0]) < 0) {
		ERROR("PWR DT\n");
		panic();
	}

	for (i = 0; i < count; i++) {
		if (supported[i] >= STM32_PM_MAX_SOC_MODE) {
			ERROR("Invalid mode\n");
			panic();
		}
		stm32mp1_supported_soc_modes[supported[i]] = 1U;
	}

	/* Initialize to deepest possible mode */
	for (i = STM32_PM_MAX_SOC_MODE - 1U; i != STM32_PM_CSLEEP_RUN; i--) {
		if (stm32mp1_supported_soc_modes[i] == 1U) {
			deepest_system_suspend_mode = i;
			break;
		}
	}
}

static int dt_fill_lp_state(uint32_t *lp_state_config, const char *lp_state)
{
	int pwr_node;
	void *fdt;
	const fdt32_t *cuint;

	if (fdt_get_address(&fdt) == 0) {
		return -ENOENT;
	}

	pwr_node = dt_get_pwr_node(fdt);
	if (pwr_node < 0) {
		return -FDT_ERR_NOTFOUND;
	}

	cuint = fdt_getprop(fdt, pwr_node, lp_state, NULL);
	if (cuint == NULL) {
		return -FDT_ERR_NOTFOUND;
	}

	*lp_state_config = fdt32_to_cpu(*cuint);

	save_supported_mode(fdt, pwr_node);

	return 0;
}

void stm32mp1_init_lp_states(void)
{
	if (dt_fill_lp_state(&system_off_mode, SYSTEM_OFF_MODE) < 0) {
		ERROR("Node %s not found\n", SYSTEM_OFF_MODE);
		panic();
	}
}

static bool is_allowed_mode(uint32_t soc_mode)
{
	assert(soc_mode < ARRAY_SIZE(stm32mp1_supported_soc_modes));

	return stm32mp1_supported_soc_modes[soc_mode] == 1U;
}

uint32_t stm32mp1_get_lp_soc_mode(uint32_t psci_mode)
{
	uint32_t mode;

	if (psci_mode == PSCI_MODE_SYSTEM_OFF) {
		return system_off_mode;
	}

	mode = deepest_system_suspend_mode;

	while ((mode > STM32_PM_CSLEEP_RUN) && !is_allowed_mode(mode)) {
		mode--;
	}

	return mode;
}
