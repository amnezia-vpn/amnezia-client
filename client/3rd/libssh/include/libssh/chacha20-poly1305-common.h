/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2020 Red Hat, Inc.
 *
 * Author: Jakub Jelen <jjelen@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * chacha20-poly1305.h file
 * This file includes definitions needed for Chacha20-poly1305 AEAD cipher
 * using different crypto backends.
 */

#ifndef CHACHA20_POLY1305_H
#define CHACHA20_POLY1305_H

#define CHACHA20_BLOCKSIZE 64
#define CHACHA20_KEYLEN 32

#define POLY1305_TAGLEN 16
/* size of the keys k1 and k2 as defined in specs */
#define POLY1305_KEYLEN 32

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
struct ssh_packet_header {
    uint32_t length;
    uint8_t payload[];
}
#if defined(__GNUC__)
__attribute__ ((packed))
#endif
#ifdef _MSC_VER
#pragma pack(pop)
#endif
;

#endif /* CHACHA20_POLY1305_H */
