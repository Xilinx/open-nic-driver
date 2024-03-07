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
#ifndef __QDMA_EXPORT_H__
#define __QDMA_EXPORT_H__

#include <linux/types.h>
#include <linux/bitops.h>

#define QDMA_NUM_DESC_RNGCNT		16
#define QDMA_NUM_C2H_BUFSZ		16
#define QDMA_NUM_C2H_TIMERS		16
#define QDMA_NUM_C2H_COUNTERS		16

enum qdma_dir {
	QDMA_H2C = 0,
	QDMA_C2H = 1
};

enum qdma_intr_rngsz {
	QDMA_INTR_RNGSZ_4KB = 0,
	QDMA_INTR_RNGSZ_8KB,
	QDMA_INTR_RNGSZ_12KB,
	QDMA_INTR_RNGSZ_16KB,
	QDMA_INTR_RNGSZ_20KB,
	QDMA_INTR_RNGSZ_24KB,
	QDMA_INTR_RNGSZ_28KB,
	QDMA_INTR_RNGSZ_32KB
};

/**
 * qdma_wb_intvl - QDMA H2C writeback interval
 **/
enum qdma_wb_intvl {
	QDMA_WB_INTVL_4 = 0,
	QDMA_WB_INTVL_8,
	QDMA_WB_INTVL_16,
	QDMA_WB_INTVL_32,
	QDMA_WB_INTVL_64,
	QDMA_WB_INTVL_128,
	QDMA_WB_INTVL_256,
	QDMA_WB_INTVL_512,
	QDMA_NUM_WB_INTVLS
};

#define QDMA_H2C_ST_DESC_SIZE                   16
#define QDMA_H2C_ST_DESC_DW0_METADATA_MASK      GENMASK_ULL(31, 0)
#define QDMA_H2C_ST_DESC_DW0_LEN_MASK           GENMASK_ULL(47, 32)

struct qdma_h2c_st_desc {
	u32 metadata;
	u16 len;
	u64 src_addr;
};

#define QDMA_C2H_ST_DESC_SIZE                   8

struct qdma_c2h_st_desc {
	u64 dst_addr;
};

#define QDMA_WB_STAT_SIZE                       8
#define QDMA_WB_STAT_DW_PIDX_MASK               GENMASK_ULL(15, 0)
#define QDMA_WB_STAT_DW_CIDX_MASK               GENMASK_ULL(31, 16)

/**
* pidx is the producer index
* cidx is the consumer index
*/
struct qdma_wb_stat {
	u16 pidx;
	u16 cidx;
};

#define QDMA_C2H_CMPL_SIZE                      8
#define QDMA_C2H_CMPL_DW_COLOR_MASK             GENMASK_ULL(1, 1)
#define QDMA_C2H_CMPL_DW_ERR_MASK               GENMASK_ULL(2, 2)
#define QDMA_C2H_CMPL_DW_PKT_LEN_MASK           GENMASK_ULL(47, 32)
#define QDMA_C2H_CMPL_DW_PKT_ID_MASK            GENMASK_ULL(63, 48)

struct qdma_c2h_cmpl {
	u8 color;
	u8 err;
	u16 pkt_len;
	u16 pkt_id;
};

#define QDMA_C2H_CMPL_STAT_SIZE                 8
#define QDMA_C2H_CMPL_STAT_DW_PIDX_MASK         GENMASK_ULL(15, 0)
#define QDMA_C2H_CMPL_STAT_DW_CIDX_MASK         GENMASK_ULL(31, 16)
#define QDMA_C2H_CMPL_STAT_DW_COLOR_MASK        GENMASK_ULL(32, 32)
#define QDMA_C2H_CMPL_STAT_DW_INTR_STATE_MASK   GENMASK_ULL(34, 33)

struct qdma_c2h_cmpl_stat {
	u16 pidx;
	u16 cidx;
	u8 color;
	u8 intr_state;
};

/**
 * These helper functions are used to convert between C structure and bit
 * streams.  Packing functions take C structure and write its content in proper
 * bit stream format.  Unpacking functions perform the opposite.
 **/
void qdma_pack_h2c_st_desc(u8 *data, struct qdma_h2c_st_desc *desc);
void qdma_pack_c2h_st_desc(u8 *data, struct qdma_c2h_st_desc *desc);
void qdma_unpack_wb_stat(struct qdma_wb_stat *stat, u8 *data);
void qdma_unpack_c2h_cmpl(struct qdma_c2h_cmpl *cmpl, u8 *data);
void qdma_unpack_c2h_cmpl_stat(struct qdma_c2h_cmpl_stat *stat, u8 *data);

enum qdma_error_index {
	/* descriptor errors */
	QDMA_DSC_ERR_POISON,
	QDMA_DSC_ERR_UR_CA,
	QDMA_DSC_ERR_PARAM,
	QDMA_DSC_ERR_ADDR,
	QDMA_DSC_ERR_TAG,
	QDMA_DSC_ERR_FLR,
	QDMA_DSC_ERR_TIMEOUT,
	QDMA_DSC_ERR_DAT_POISON,
	QDMA_DSC_ERR_FLR_CANCEL,
	QDMA_DSC_ERR_DMA,
	QDMA_DSC_ERR_DSC,
	QDMA_DSC_ERR_RQ_CANCEL,
	QDMA_DSC_ERR_DBE,
	QDMA_DSC_ERR_SBE,
	QDMA_DSC_ERR_ALL,

	/* TRQ errors */
	QDMA_TRQ_ERR_UNMAPPED,
	QDMA_TRQ_ERR_QID_RANGE,
	QDMA_TRQ_ERR_VF_ACCESS,
	QDMA_TRQ_ERR_TCP_TIMEOUT,
	QDMA_TRQ_ERR_ALL,

	/* C2H errors */
	QDMA_ST_C2H_ERR_MTY_MISMATCH,
	QDMA_ST_C2H_ERR_LEN_MISMATCH,
	QDMA_ST_C2H_ERR_QID_MISMATCH,
	QDMA_ST_C2H_ERR_DESC_RSP_ERR,
	QDMA_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
	QDMA_ST_C2H_ERR_MSI_INT_FAIL,
	QDMA_ST_C2H_ERR_ERR_DESC_CNT,
	QDMA_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
	QDMA_ST_C2H_ERR_PORTID_BYP_IN_MISMATCH,
	QDMA_ST_C2H_ERR_CMPL_INV_Q_ERR,
	QDMA_ST_C2H_ERR_CMPL_QFULL_ERR,
	QDMA_ST_C2H_ERR_CMPL_CIDX_ERR,
	QDMA_ST_C2H_ERR_CMPL_PRTY_ERR,
	QDMA_ST_C2H_ERR_ALL,

	/* fatal errors */
	QDMA_ST_FATAL_ERR_MTY_MISMATCH,
	QDMA_ST_FATAL_ERR_LEN_MISMATCH,
	QDMA_ST_FATAL_ERR_QID_MISMATCH,
	QDMA_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
	QDMA_ST_FATAL_ERR_CMPL_CTXT_RAM_RDBE,
	QDMA_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
	QDMA_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
	QDMA_ST_FATAL_ERR_CMPL_COAL_DATA_RAM_RDBE,
	QDMA_ST_FATAL_ERR_TUSER_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_WPL_DATA_PAR,
	QDMA_ST_FATAL_ERR_ALL,

	/* H2C errors */
	QDMA_ST_H2C_ERR_ZERO_LEN_DESC,
	QDMA_ST_H2C_ERR_CSI_MOP,
	QDMA_ST_H2C_ERR_NO_DMA_DSC,
	QDMA_ST_H2C_ERR_SBE,
	QDMA_ST_H2C_ERR_DBE,
	QDMA_ST_H2C_ERR_ALL,

	/* single bit errors */
	QDMA_SBE_ERR_MI_H2C0_DAT,
	QDMA_SBE_ERR_MI_C2H0_DAT,
	QDMA_SBE_ERR_H2C_RD_BRG_DAT,
	QDMA_SBE_ERR_H2C_WR_BRG_DAT,
	QDMA_SBE_ERR_C2H_RD_BRG_DAT,
	QDMA_SBE_ERR_C2H_WR_BRG_DAT,
	QDMA_SBE_ERR_FUNC_MAP,
	QDMA_SBE_ERR_DSC_HW_CTXT,
	QDMA_SBE_ERR_DSC_CRD_RCV,
	QDMA_SBE_ERR_DSC_SW_CTXT,
	QDMA_SBE_ERR_DSC_CPLI,
	QDMA_SBE_ERR_DSC_CPLD,
	QDMA_SBE_ERR_PASID_CTXT_RAM,
	QDMA_SBE_ERR_TIMER_FIFO_RAM,
	QDMA_SBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_SBE_ERR_QID_FIFO_RAM,
	QDMA_SBE_ERR_TUSER_FIFO_RAM,
	QDMA_SBE_ERR_WB_COAL_DATA_RAM,
	QDMA_SBE_ERR_INT_QID2VEC_RAM,
	QDMA_SBE_ERR_INT_CTXT_RAM,
	QDMA_SBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_SBE_ERR_PFCH_CTXT_RAM,
	QDMA_SBE_ERR_WB_CTXT_RAM,
	QDMA_SBE_ERR_PFCH_LL_RAM,
	QDMA_SBE_ERR_H2C_PEND_FIFO,
	QDMA_SBE_ERR_ALL,

	/* double bit errors */
	QDMA_DBE_ERR_MI_H2C0_DAT,
	QDMA_DBE_ERR_MI_C2H0_DAT,
	QDMA_DBE_ERR_H2C_RD_BRG_DAT,
	QDMA_DBE_ERR_H2C_WR_BRG_DAT,
	QDMA_DBE_ERR_C2H_RD_BRG_DAT,
	QDMA_DBE_ERR_C2H_WR_BRG_DAT,
	QDMA_DBE_ERR_FUNC_MAP,
	QDMA_DBE_ERR_DSC_HW_CTXT,
	QDMA_DBE_ERR_DSC_CRD_RCV,
	QDMA_DBE_ERR_DSC_SW_CTXT,
	QDMA_DBE_ERR_DSC_CPLI,
	QDMA_DBE_ERR_DSC_CPLD,
	QDMA_DBE_ERR_PASID_CTXT_RAM,
	QDMA_DBE_ERR_TIMER_FIFO_RAM,
	QDMA_DBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_DBE_ERR_QID_FIFO_RAM,
	QDMA_DBE_ERR_TUSER_FIFO_RAM,
	QDMA_DBE_ERR_WB_COAL_DATA_RAM,
	QDMA_DBE_ERR_INT_QID2VEC_RAM,
	QDMA_DBE_ERR_INT_CTXT_RAM,
	QDMA_DBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_DBE_ERR_PFCH_CTXT_RAM,
	QDMA_DBE_ERR_WB_CTXT_RAM,
	QDMA_DBE_ERR_PFCH_LL_RAM,
	QDMA_DBE_ERR_H2C_PEND_FIFO,
	QDMA_DBE_ERR_ALL,

	QDMA_ERR_ALL
};

#endif
