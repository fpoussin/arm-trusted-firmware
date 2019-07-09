/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <libfdt.h>

#include <platform_def.h>

#include <drivers/st/stm32_iwdg.h>
#include <drivers/st/stm32mp_dummy_regulator.h>
#include <drivers/st/stm32mp_pmic.h>
#include <drivers/st/stm32mp_regulator.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

/* Internal layout of the 32bit OTP word board_id */
#define BOARD_ID_BOARD_NB_MASK		GENMASK(31, 16)
#define BOARD_ID_BOARD_NB_SHIFT		16
#define BOARD_ID_VARCPN_MASK		GENMASK(15, 12)
#define BOARD_ID_VARCPN_SHIFT		12
#define BOARD_ID_REVISION_MASK		GENMASK(11, 8)
#define BOARD_ID_REVISION_SHIFT		8
#define BOARD_ID_VARFG_MASK		GENMASK(7, 4)
#define BOARD_ID_VARFG_SHIFT		4
#define BOARD_ID_BOM_MASK		GENMASK(3, 0)

#define BOARD_ID2NB(_id)		(((_id) & BOARD_ID_BOARD_NB_MASK) >> \
					 BOARD_ID_BOARD_NB_SHIFT)
#define BOARD_ID2VARCPN(_id)		(((_id) & BOARD_ID_VARCPN_MASK) >> \
					 BOARD_ID_VARCPN_SHIFT)
#define BOARD_ID2REV(_id)		(((_id) & BOARD_ID_REVISION_MASK) >> \
					 BOARD_ID_REVISION_SHIFT)
#define BOARD_ID2VARFG(_id)		(((_id) & BOARD_ID_VARFG_MASK) >> \
					 BOARD_ID_VARFG_SHIFT)
#define BOARD_ID2BOM(_id)		((_id) & BOARD_ID_BOM_MASK)

#if defined(IMAGE_BL2)
#define MAP_SEC_SYSRAM	MAP_REGION_FLAT(STM32MP_SYSRAM_BASE, \
					STM32MP_SYSRAM_SIZE, \
					MT_MEMORY | \
					MT_RW | \
					MT_SECURE | \
					MT_EXECUTE_NEVER)
#elif defined(IMAGE_BL32)
#define MAP_SEC_SYSRAM	MAP_REGION_FLAT(STM32MP_SEC_SYSRAM_BASE, \
					STM32MP_SEC_SYSRAM_SIZE, \
					MT_MEMORY | \
					MT_RW | \
					MT_SECURE | \
					MT_EXECUTE_NEVER)

/* Non-secure SYSRAM is used a uncached memory for SCMI message transfer */
#define MAP_NS_SYSRAM	MAP_REGION_FLAT(STM32MP_NS_SYSRAM_BASE, \
					STM32MP_NS_SYSRAM_SIZE, \
					MT_DEVICE | \
					MT_RW | \
					MT_NS | \
					MT_EXECUTE_NEVER)
#endif

#define MAP_DEVICE1	MAP_REGION_FLAT(STM32MP1_DEVICE1_BASE, \
					STM32MP1_DEVICE1_SIZE, \
					MT_DEVICE | \
					MT_RW | \
					MT_SECURE | \
					MT_EXECUTE_NEVER)

#define MAP_DEVICE2	MAP_REGION_FLAT(STM32MP1_DEVICE2_BASE, \
					STM32MP1_DEVICE2_SIZE, \
					MT_DEVICE | \
					MT_RW | \
					MT_SECURE | \
					MT_EXECUTE_NEVER)

#if defined(IMAGE_BL2)
static const mmap_region_t stm32mp1_mmap[] = {
	MAP_SEC_SYSRAM,
	MAP_DEVICE1,
	MAP_DEVICE2,
	{0}
};
#endif
#if defined(IMAGE_BL32)
static const mmap_region_t stm32mp1_mmap[] = {
	MAP_SEC_SYSRAM,
	MAP_NS_SYSRAM,
	MAP_DEVICE1,
	MAP_DEVICE2,
	{0}
};
#endif

void configure_mmu(void)
{
	mmap_add(stm32mp1_mmap);
	init_xlat_tables();

	enable_mmu_svc_mon(0);
}

uintptr_t stm32_get_gpio_bank_base(unsigned int bank)
{
	if (bank == GPIO_BANK_Z) {
		return GPIOZ_BASE;
	}

	assert(GPIO_BANK_A == 0 && bank <= GPIO_BANK_K);

	return GPIOA_BASE + (bank * GPIO_BANK_OFFSET);
}

uint32_t stm32_get_gpio_bank_offset(unsigned int bank)
{
	if (bank == GPIO_BANK_Z) {
		return 0;
	}

	assert(GPIO_BANK_A == 0 && bank <= GPIO_BANK_K);

	return bank * GPIO_BANK_OFFSET;
}

#if STM32MP_UART_PROGRAMMER
/*
 * UART Management
 */
static const uintptr_t stm32mp1_uart_addresses[8] = {
	USART1_BASE,
	USART2_BASE,
	USART3_BASE,
	UART4_BASE,
	UART5_BASE,
	USART6_BASE,
	UART7_BASE,
	UART8_BASE,
};

uintptr_t get_uart_address(uint32_t instance_nb)
{
	if ((instance_nb == 0U) ||
	    (instance_nb > ARRAY_SIZE(stm32mp1_uart_addresses))) {
		return 0U;
	}

	return stm32mp1_uart_addresses[instance_nb - 1U];
}
#endif

unsigned long stm32_get_gpio_bank_clock(unsigned int bank)
{
	if (bank == GPIO_BANK_Z) {
		return GPIOZ;
	}

	assert(GPIO_BANK_A == 0 && bank <= GPIO_BANK_K);

	return GPIOA + (bank - GPIO_BANK_A);
}

int stm32_get_gpio_bank_pinctrl_node(void *fdt, unsigned int bank)
{
	switch (bank) {
	case GPIO_BANK_A:
	case GPIO_BANK_B:
	case GPIO_BANK_C:
	case GPIO_BANK_D:
	case GPIO_BANK_E:
	case GPIO_BANK_F:
	case GPIO_BANK_G:
	case GPIO_BANK_H:
	case GPIO_BANK_I:
	case GPIO_BANK_J:
	case GPIO_BANK_K:
		return fdt_path_offset(fdt, "/soc/pin-controller");
	case GPIO_BANK_Z:
		return fdt_path_offset(fdt, "/soc/pin-controller-z");
	default:
		panic();
	}
}

int stm32_get_otp_index(const char *otp_name, uint32_t *otp_idx,
			uint32_t *otp_len)
{
	assert(otp_name != NULL);
	assert(otp_idx != NULL);

	if (bsec_find_otp_name_in_dt(otp_name, otp_idx, otp_len) != BSEC_OK) {
		return -1;
	}

	return 0;
}

int stm32_get_otp_value(const char *otp_name, uint32_t *otp_val)
{
	uint32_t otp;

	assert(otp_name != NULL);
	assert(otp_val != NULL);

	if (stm32_get_otp_index(otp_name, &otp, NULL) != 0) {
		return -1;
	}

#if defined(IMAGE_BL2)
	if (bsec_shadow_read_otp(otp_val, otp) != BSEC_OK) {
		ERROR("BSEC: %s Read Error\n", otp_name);
		return -1;
	}
#elif defined(IMAGE_BL32)
	if (bsec_read_otp(otp_val, otp) != BSEC_OK) {
		ERROR("BSEC: %s Read Error\n", otp_name);
		return -1;
	}
#endif

	return 0;
}

int stm32mp_get_chip_version(uint32_t *chip_version)
{
	return stm32mp1_dbgmcu_get_chip_version(chip_version);
}

static int get_part_number(uint32_t *part_nb)
{
	uint32_t part_number;
	uint32_t dev_id;

	assert(part_nb != NULL);

	if (stm32mp1_dbgmcu_get_chip_dev_id(&dev_id) < 0) {
		return -1;
	}

	if (stm32_get_otp_value(PART_NUMBER_OTP, &part_number) != 0) {
		return -1;
	}

	part_number = (part_number & PART_NUMBER_OTP_PART_MASK) >>
		PART_NUMBER_OTP_PART_SHIFT;

	*part_nb = part_number | (dev_id << 16);

	return 0;
}

static int get_cpu_package(uint32_t *cpu_package)
{
	uint32_t package;

	assert(cpu_package != NULL);

	if (stm32_get_otp_value(PACKAGE_OTP, &package) != 0) {
		return -1;
	}

	*cpu_package = (package & PACKAGE_OTP_PKG_MASK) >>
		PACKAGE_OTP_PKG_SHIFT;

	return 0;
}

void stm32mp_get_soc_name(char name[STM32_SOC_NAME_SIZE])
{
	char *cpu_s, *cpu_r, *pkg;
	uint32_t part_number;
	uint32_t cpu_package;
	uint32_t chip_dev_id;
	int ret;

	/* MPUs Part Numbers */
	ret = get_part_number(&part_number);
	if (ret < 0) {
		WARN("Cannot get part number\n");
		return;
	}

	switch (part_number) {
	case STM32MP157C_PART_NB:
		cpu_s = "157C";
		break;
	case STM32MP157A_PART_NB:
		cpu_s = "157A";
		break;
	case STM32MP153C_PART_NB:
		cpu_s = "153C";
		break;
	case STM32MP153A_PART_NB:
		cpu_s = "153A";
		break;
	case STM32MP151C_PART_NB:
		cpu_s = "151C";
		break;
	case STM32MP151A_PART_NB:
		cpu_s = "151A";
		break;
	case STM32MP157F_PART_NB:
		cpu_s = "157F";
		break;
	case STM32MP157D_PART_NB:
		cpu_s = "157D";
		break;
	case STM32MP153F_PART_NB:
		cpu_s = "153F";
		break;
	case STM32MP153D_PART_NB:
		cpu_s = "153D";
		break;
	case STM32MP151F_PART_NB:
		cpu_s = "151F";
		break;
	case STM32MP151D_PART_NB:
		cpu_s = "151D";
		break;
	default:
		cpu_s = "????";
		break;
	}

	/* Package */
	ret = get_cpu_package(&cpu_package);
	if (ret < 0) {
		WARN("Cannot get CPU package\n");
		return;
	}

	switch (cpu_package) {
	case PKG_AA_LFBGA448:
		pkg = "AA";
		break;
	case PKG_AB_LFBGA354:
		pkg = "AB";
		break;
	case PKG_AC_TFBGA361:
		pkg = "AC";
		break;
	case PKG_AD_TFBGA257:
		pkg = "AD";
		break;
	default:
		pkg = "??";
		break;
	}

	/* REVISION */
	ret = stm32mp_get_chip_version(&chip_dev_id);
	if (ret < 0) {
		WARN("Cannot get CPU version\n");
		return;
	}

	switch (chip_dev_id) {
	case STM32MP1_REV_B:
		cpu_r = "B";
		break;
	case STM32MP1_REV_Z:
		cpu_r = "Z";
		break;
	default:
		cpu_r = "?";
		break;
	}

	snprintf(name, STM32_SOC_NAME_SIZE,
		 "STM32MP%s%s Rev.%s", cpu_s, pkg, cpu_r);
}

void stm32mp_print_cpuinfo(void)
{
	char name[STM32_SOC_NAME_SIZE];

	stm32mp_get_soc_name(name);
	NOTICE("CPU: %s\n", name);
}

void stm32mp_print_boardinfo(void)
{
	uint32_t board_id = 0;

	if (stm32_get_otp_value(BOARD_ID_OTP, &board_id) != 0) {
		return;
	}

	if (board_id != 0U) {
		char rev[2];

		rev[0] = BOARD_ID2REV(board_id) - 1 + 'A';
		rev[1] = '\0';
		NOTICE("Board: MB%04x Var%u.%u Rev.%s-%02u\n",
		       BOARD_ID2NB(board_id),
		       BOARD_ID2VARCPN(board_id),
		       BOARD_ID2VARFG(board_id),
		       rev,
		       BOARD_ID2BOM(board_id));
	}
}

/* Return true when SoC provides a single Cortex-A7 core, and false otherwise */
bool stm32mp_is_single_core(void)
{
	uint32_t part_number;

	if (get_part_number(&part_number) < 0) {
		ERROR("Invalid part number, assume single core chip");
		return true;
	}

	switch (part_number) {
	case STM32MP151A_PART_NB:
	case STM32MP151C_PART_NB:
	case STM32MP151D_PART_NB:
	case STM32MP151F_PART_NB:
		return true;

	default:
		return false;
	}
}

/* Return true when device is in closed state */
bool stm32mp_is_closed_device(void)
{
	uint32_t value;

	if (stm32_get_otp_value(CFG0_OTP, &value) != 0) {
		return true;
	}

	return (value & CFG0_CLOSED_DEVICE) == CFG0_CLOSED_DEVICE;
}

uint32_t stm32_iwdg_get_instance(uintptr_t base)
{
	switch (base) {
	case IWDG1_BASE:
		return IWDG1_INST;
	case IWDG2_BASE:
		return IWDG2_INST;
	default:
		panic();
	}
}

uint32_t stm32_iwdg_get_otp_config(uint32_t iwdg_inst)
{
	uint32_t iwdg_cfg = 0U;
	uint32_t otp_value;

	if (stm32_get_otp_value(HW2_OTP, &otp_value) != 0) {
		panic();
	}

	if ((otp_value & BIT(iwdg_inst + HW2_OTP_IWDG_HW_POS)) != 0U) {
		iwdg_cfg |= IWDG_HW_ENABLED;
	}

	if ((otp_value & BIT(iwdg_inst + HW2_OTP_IWDG_FZ_STOP_POS)) != 0U) {
		iwdg_cfg |= IWDG_DISABLE_ON_STOP;
	}

	if ((otp_value & BIT(iwdg_inst + HW2_OTP_IWDG_FZ_STANDBY_POS)) != 0U) {
		iwdg_cfg |= IWDG_DISABLE_ON_STANDBY;
	}

	return iwdg_cfg;
}

#if defined(IMAGE_BL2)
uint32_t stm32_iwdg_shadow_update(uint32_t iwdg_inst, uint32_t flags)
{
	uint32_t otp_value;
	uint32_t otp;
	uint32_t result;

	if (stm32_get_otp_index(HW2_OTP, &otp, NULL) != 0) {
		panic();
	}

	if (stm32_get_otp_value(HW2_OTP, &otp_value) != 0) {
		panic();
	}

	if ((flags & IWDG_DISABLE_ON_STOP) != 0) {
		otp_value |= BIT(iwdg_inst + HW2_OTP_IWDG_FZ_STOP_POS);
	}

	if ((flags & IWDG_DISABLE_ON_STANDBY) != 0) {
		otp_value |= BIT(iwdg_inst + HW2_OTP_IWDG_FZ_STANDBY_POS);
	}

	result = bsec_write_otp(otp_value, otp);
	if (result != BSEC_OK) {
		return result;
	}

	/* Sticky lock OTP_IWDG (read and write) */
	if ((bsec_set_sr_lock(otp) != BSEC_OK) ||
	    (bsec_set_sw_lock(otp) != BSEC_OK)) {
		return BSEC_LOCK_FAIL;
	}

	return BSEC_OK;
}
#endif

int plat_bind_regulator(struct stm32mp_regulator *regu)
{
	if ((dt_pmic_status() > 0) && is_pmic_regulator(regu)) {
		bind_pmic_regulator(regu);
	} else {
		bind_dummy_regulator(regu);
	}

	return 0;
}

/* Get the non-secure DDR size */
uint32_t stm32mp_get_ddr_ns_size(void)
{
	static uint32_t ddr_ns_size;
	uint32_t ddr_size;

	if (ddr_ns_size != 0U) {
		return ddr_ns_size;
	}

	ddr_size = dt_get_ddr_size();
	if ((ddr_size <= (STM32MP_DDR_S_SIZE + STM32MP_DDR_SHMEM_SIZE)) ||
	    (ddr_size > STM32MP_DDR_MAX_SIZE)) {
		panic();
	}

	ddr_ns_size = ddr_size - (STM32MP_DDR_S_SIZE + STM32MP_DDR_SHMEM_SIZE);

	return ddr_ns_size;
}
