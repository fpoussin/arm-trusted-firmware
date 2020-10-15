/*
 * Copyright (c) 2020-2021, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <endian.h>
#include <errno.h>
#include <limits.h>

#include <platform_def.h>

#include <common/debug.h>
#include <plat/common/platform.h>

static uint32_t root_pk_hash[BOOT_API_SHA256_DIGEST_SIZE_IN_BYTES /
			     sizeof(uint32_t)];

int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
	uint32_t otp_idx;
	uint32_t otp_val;
	uint32_t len;
	size_t i;

	if (cookie != NULL) {
		return -EINVAL;
	}

	if (stm32_get_otp_index(PKH_OTP, &otp_idx, &len) != 0) {
		VERBOSE("get_rot_pk_hash: get index error\n");
		return -EINVAL;
	}
	if (len != (CHAR_BIT * BOOT_API_SHA256_DIGEST_SIZE_IN_BYTES)) {
		VERBOSE("get_rot_pk_hash: length Error\n");
		return -EINVAL;
	}

	for (i = 0U; i < ARRAY_SIZE(root_pk_hash); i++) {
		if (stm32_get_otp_value_from_idx(otp_idx + i, &otp_val) != 0) {
			return -EINVAL;
		}

		root_pk_hash[i] = bswap32(otp_val);
	}

	*key_ptr = &root_pk_hash;
	*key_len = BOOT_API_SHA256_DIGEST_SIZE_IN_BYTES;
	*flags = ROTPK_IS_HASH;

	if (!stm32mp_is_closed_device()) {
		/* Check if key hash values in OTP are 0 or 0xFFFFFFFFF programmed : Invalid Key */
		uint32_t res;
		uint32_t rootpk;
		uint8_t *proot_pk = root_pk_hash;
		uint8_t idx = sizeof(uint32_t);

#if !STM32MP_USE_STM32IMAGE
		idx += sizeof(der_sha256_header);
		proot_pk = root_pk_hash + sizeof(der_sha256_header);
#endif
		memcpy(&res, proot_pk, sizeof(uint32_t));
		if ((res == 0U) || (res == 0xFFFFFFFFU)) {
			while (idx < ARRAY_SIZE(root_pk_hash)) {
				memcpy(&rootpk, (proot_pk + idx), sizeof(uint32_t));
				if (res != rootpk) {
					return 0;
				}

				idx += sizeof(uint32_t);
			}

			*flags |= ROTPK_NOT_DEPLOYED;
		}
	}

	return 0;
}

int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
	/*
	 * This monotonic counter is the counter used by ROM code
	 * to identify BL2.
	 */
	if ((cookie == NULL) &&
	    (stm32_get_otp_value(MONOTONIC_OTP, nv_ctr) == 0)) {
		return 0;
	}

	return -EINVAL;
}

int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
	return -EINVAL;
}

#if !STM32MP_USE_STM32IMAGE
int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}
#endif
