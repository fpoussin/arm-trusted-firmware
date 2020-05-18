/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SUNXI_PRIVATE_H
#define SUNXI_PRIVATE_H

void sunxi_configure_mmu_el3(int flags);

int sunxi_pmic_setup(uint16_t socid, const void *fdt);
void sunxi_security_setup(void);

uint16_t sunxi_read_soc_id(void);
int sunxi_init_platform_r_twi(uint16_t socid, bool use_rsb);

#endif /* SUNXI_PRIVATE_H */
