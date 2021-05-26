/*
 * Copyright (C) 2016 Cisco Systems, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "mkimage.h"
#include <stdio.h>
#include <string.h>
#include <image.h>
#include <u-boot/ecdsa.h>

int ecdsa_sign(struct image_sign_info *info,
	       const struct image_region region[], int region_count,
	       uint8_t **sigp, uint *sig_len)
{
	return -ENXIO;
}

int ecdsa_add_verify_data(struct image_sign_info *info, void *keydest)
{
	return -ENXIO;
}
