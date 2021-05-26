/**
 * \file sha512.h
 *
 * \brief SHA-384 and SHA-512 cryptographic hash function
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: GPL-2.0
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  This file was modified from mbed TLS (https://tls.mbed.org)
 */
#ifndef _SHA512_H
#define _SHA512_H

#define SHA384_SUM_LEN 48
#define SHA512_SUM_LEN 64

#define SHA384_DER_LEN 19
#define SHA512_DER_LEN 19

extern const uint8_t sha384_der_prefix[];
extern const uint8_t sha512_der_prefix[];

/* Reset watchdog each time we process this many bytes */
#define CHUNKSZ_SHA512 (64 * 1024)

/**
 * \brief          SHA-512 context structure
 */
typedef struct
{
    uint64_t total[2];          /*!< number of bytes processed  */
    uint64_t state[8];          /*!< intermediate digest state  */
    unsigned char buffer[128];  /*!< data block being processed */
    int is384;                  /*!< 0 => SHA-512, else SHA-384 */
}
sha512_context;

/**
 * \brief          SHA-512 context setup
 *
 * \param ctx      context to be initialized
 * \param is384    0 = use SHA512, 1 = use SHA384
 */
void sha512_starts( sha512_context *ctx, int is384 );

/**
 * \brief          SHA-512 process buffer
 *
 * \param ctx      SHA-512 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void sha512_update( sha512_context *ctx, const unsigned char *input,
                    unsigned int ilen );

/**
 * \brief          SHA-512 final digest
 *
 * \param ctx      SHA-512 context
 * \param output   SHA-384/512 checksum result
 */
void sha512_finish( sha512_context *ctx, unsigned char output[64] );

/**
 * \brief          Output = SHA-384( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   SHA-384 checksum result
 */
void sha384_csum_wd( const unsigned char *input, unsigned int ilen,
                     unsigned char *output, unsigned int chunk_sz );

/**
 * \brief          Output = SHA-512( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   SHA-384/512 checksum result
 */
void sha512_csum_wd( const unsigned char *input, unsigned int ilen,
                     unsigned char *output, unsigned int chunk_sz );

#endif /* sha512.h */
