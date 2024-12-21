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
#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/vmalloc.h>
#include "onic_common.h"

void print_raw_data(const u8 *data, u32 len)
{
	const unsigned int buf_len = 2048;
	char *buf;
	int i, offset;

	buf = vmalloc(buf_len);
	if (!buf)
		return;

	pr_info("data length = %d\n", len);
	offset = 0;
	for (i = 0; i < len; ++i) {
		offset += snprintf(buf + offset, buf_len - offset,
				   " %02x", data[i]);
		if (i % 16 == 15) {
			pr_info("%s\n", buf);
			offset = 0;
		}
	}
	if (i % 16 != 0)
		pr_info("%s\n", buf);

	vfree(buf);
}

void print_skb(const struct sk_buff *skb)
{
	print_raw_data((u8 *)skb->data, skb->len);
}
