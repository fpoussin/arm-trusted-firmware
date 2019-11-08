/*
 * Copyright (c) 2017-2018, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ETZPC_H__
#define __ETZPC_H__

/* Define security level for each peripheral (DECPROT) */
enum etzpc_decprot_attributes {
	TZPC_DECPROT_S_RW = 0,
	TZPC_DECPROT_NS_R_S_W = 1,
	TZPC_DECPROT_MCU_ISOLATION = 2,
	TZPC_DECPROT_NS_RW = 3,
	TZPC_DECPROT_MAX = 4,
};

void etzpc_configure_decprot(uint32_t decprot_id,
			     enum etzpc_decprot_attributes decprot_attr);
enum etzpc_decprot_attributes etzpc_get_decprot(uint32_t decprot_id);
void etzpc_lock_decprot(uint32_t decprot_id);
void etzpc_configure_tzma(uint32_t tzma_id, uint16_t tzma_value);
uint16_t etzpc_get_tzma(uint32_t tzma_id);
void etzpc_lock_tzma(uint32_t tzma_id);
bool etzpc_get_lock_tzma(uint32_t tzma_id);
uint8_t etzpc_get_num_per_sec(void);
uint8_t etzpc_get_revision(void);
uintptr_t etzpc_get_base_address(void);
int etzpc_init(void);

#endif /* __ETZPC_H__ */