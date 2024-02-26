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
#include "onic_common.h"
#include "qdma_export.h"

void qdma_pack_h2c_st_desc(u8 *data, struct qdma_h2c_st_desc *desc)
{
	u64 *dw0, *dw1;

	if (!data || !desc)
		return;

	dw0 = (u64 *)data;
	dw1 = (u64 *)data + 1;

	*dw0 = 0;
	*dw0 = (FIELD_SET(QDMA_H2C_ST_DESC_DW0_METADATA_MASK, desc->metadata) |
		FIELD_SET(QDMA_H2C_ST_DESC_DW0_LEN_MASK, desc->len));
	*dw1 = desc->src_addr;
}

void qdma_pack_c2h_st_desc(u8 *data, struct qdma_c2h_st_desc *desc)
{
	u64 *dw;

	if (!data || !desc)
		return;

	dw = (u64 *)data;

	*dw = desc->dst_addr;
}

void qdma_unpack_wb_stat(struct qdma_wb_stat *stat, u8 *data)
{
	u64 *dw;

	if (!stat || !data)
		return;

	dw = (u64 *)data;

	stat->pidx = BITFIELD_GET(QDMA_WB_STAT_DW_PIDX_MASK, *dw);
	stat->cidx = BITFIELD_GET(QDMA_WB_STAT_DW_CIDX_MASK, *dw);
}

void qdma_unpack_c2h_cmpl(struct qdma_c2h_cmpl *cmpl, u8 *data)
{
	u64 *dw;

	if (!cmpl || !data)
		return;

	dw = (u64 *)data;

	cmpl->color = BITFIELD_GET(QDMA_C2H_CMPL_DW_COLOR_MASK, *dw);
	cmpl->err = BITFIELD_GET(QDMA_C2H_CMPL_DW_ERR_MASK, *dw);
	cmpl->pkt_len = BITFIELD_GET(QDMA_C2H_CMPL_DW_PKT_LEN_MASK, *dw);
	cmpl->pkt_id = BITFIELD_GET(QDMA_C2H_CMPL_DW_PKT_ID_MASK, *dw);
}

void qdma_unpack_c2h_cmpl_stat(struct qdma_c2h_cmpl_stat *stat, u8 *data)
{
	u64 *dw;

	if (!stat || !data)
		return;

	dw = (u64 *)data;

	stat->pidx = BITFIELD_GET(QDMA_C2H_CMPL_STAT_DW_PIDX_MASK, *dw);
	stat->cidx = BITFIELD_GET(QDMA_C2H_CMPL_STAT_DW_CIDX_MASK, *dw);
	stat->color = BITFIELD_GET(QDMA_C2H_CMPL_STAT_DW_COLOR_MASK, *dw);
	stat->intr_state =
		BITFIELD_GET(QDMA_C2H_CMPL_STAT_DW_INTR_STATE_MASK, *dw);
}
