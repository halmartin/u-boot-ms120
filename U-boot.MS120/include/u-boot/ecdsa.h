/*
 * Copyright (C) 2016 Cisco Systems, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ECDSA_H
#define _ECDSA_H

#include <errno.h>
#include <image.h>

#if IMAGE_ENABLE_SIGN
/**
 * sign() - calculate and return signature for given input data
 *
 * @info:	Specifies key and FIT information
 * @data:	Pointer to the input data
 * @data_len:	Data length
 * @sigp:	Set to an allocated buffer holding the signature
 * @sig_len:	Set to length of the calculated hash
 *
 * This computes input data signature according to selected algorithm.
 * Resulting signature value is placed in an allocated buffer, the
 * pointer is returned as *sigp. The length of the calculated
 * signature is returned via the sig_len pointer argument. The caller
 * should free *sigp.
 *
 * @return: 0, on success, -ve on error
 */
int ecdsa_sign(struct image_sign_info *info,
	       const struct image_region region[],
	       int region_count, uint8_t **sigp, uint *sig_len);

/**
 * add_verify_data() - Add verification information to FDT
 *
 * Add public key information to the FDT node, suitable for
 * verification at run-time. The information added depends on the
 * algorithm being used.
 *
 * @info:	Specifies key and FIT information
 * @keydest:	Destination FDT blob for public key data
 * @return: 0, on success, -ENOSPC if the keydest FDT blob ran out of space,
		other -ve value on error
*/
int ecdsa_add_verify_data(struct image_sign_info *info, void *keydest);
#else
static inline
int ecdsa_sign(struct image_sign_info *info,
	       const struct image_region region[], int region_count,
	       uint8_t **sigp, uint *sig_len)
{
	return -ENXIO;
}

static inline
int ecdsa_add_verify_data(struct image_sign_info *info, void *keydest)
{
	return -ENXIO;
}
#endif

#if IMAGE_ENABLE_VERIFY
/**
 * ecdsa_verify() - Verify a signature against some data
 *
 * Verify a ECDSA signature against an expected hash.
 *
 * @info:	Specifies key and FIT information
 * @data:	Pointer to the input data
 * @data_len:	Data length
 * @sig:	Signature
 * @sig_len:	Number of bytes in signature
 * @return 0 if verified, -ve on error
 */
int ecdsa_verify(struct image_sign_info *info,
		 const struct image_region region[], int region_count,
		 uint8_t *sig, uint sig_len);
#else
static inline
int ecdsa_verify(struct image_sign_info *info,
		 const struct image_region region[], int region_count,
		 uint8_t *sig, uint sig_len)
{
	return -ENXIO;
}
#endif

#define SWIMS_MAX_CMDLINE_LEN	1024
#define SWIMS_MAX_TMPFILE_LEN	64

#define ECP256_BYTES	(256 / 8)
#define ECP384_BYTES	(384 / 8)

#endif
