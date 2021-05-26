/*
 * Copyright (c) 2013, Andreas Oetken.
 *
 * SPDX-License-Identifier:    GPL-2.0+
*/

#ifndef _RSA_CHECKSUM_H
#define _RSA_CHECKSUM_H

#include <errno.h>
#include <image.h>
#include <sha1.h>
#include <sha256.h>
#include <u-boot/sha512.h>

/**
 * hash_calculate() - Calculate hash over the data
 *
 * @name:  Name of algorithm to be used for hash calculation
 * @region: Array having info of regions over which hash needs to be calculated
 * @region_count: Number of regions in the region array
 * @checksum: Buffer contanining the output hash
 *
 * @return 0 if OK, < 0 if error
 */
int hash_calculate(const char *name,
		   const struct image_region region[], int region_count,
		   uint8_t *checksum);

#endif
