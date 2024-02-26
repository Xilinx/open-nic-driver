/*
 * Copyright (c) 2020 Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */
#ifndef __ONIC_COMMON_H__
#define __ONIC_COMMON_H__

#include <linux/types.h>
#include <linux/skbuff.h>

/**
 * get_trailing_zeros - Get the number of trailing zeros
 * @x: input number
 *
 * Returns the number of trailing 0s in x, starting at LSB.  For GCC compiler,
 * it simply calls the __builtin_ffsll function.
 **/
static inline u8 get_trailing_zeros(u64 x)
{
#ifdef GCC_COMPILER
	return (__builtin_ffsll(x) - 1);
#else
	u8 n = 1;

	if ((x & 0xFFFFFFFF) == 0) {
		n = n + 32;
		x = x >> 32;
	}
	if ((x & 0x0000FFFF) == 0) {
		n = n + 16;
		x = x >> 16;
	}
	if ((x & 0x000000FF) == 0) {
		n = n + 8;
		x = x >> 8;
	}
	if ((x & 0x0000000F) == 0) {
		n = n + 4;
		x = x >> 4;
	}
	if ((x & 0x00000003) == 0) {
		n = n + 2;
		x = x >> 2;
	}

	return n - (x & 1);
#endif
}

#define FIELD_SHIFT(mask)	get_trailing_zeros(mask)
#define FIELD_SET(mask, val)	(((u64)(val) << FIELD_SHIFT(mask)) & mask)
#define BITFIELD_GET(mask, reg)	((reg & mask) >> FIELD_SHIFT(mask))

/**
 * print_raw_data - print raw data to kernel log
 * @data: raw data to be printed
 * @len: length of the data
 */
void print_raw_data(const u8 *data, u32 len);

/**
 * print_skb - print skb content to kernel log
 * @skb: skb to be printed
 */
void print_skb(const struct sk_buff *skb);

#endif
