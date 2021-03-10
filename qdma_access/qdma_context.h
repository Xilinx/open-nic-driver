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
#ifndef __QDMA_CONTEXT_H__
#define __QDMA_CONTEXT_H__

#include "qdma_device.h"
#include "qdma_export.h"

enum qdma_ctxt_cmd_op {
	QDMA_CTXT_CMD_OP_CLR,
	QDMA_CTXT_CMD_OP_WR,
	QDMA_CTXT_CMD_OP_RD,
	QDMA_CTXT_CMD_OP_INV
};

enum qdma_ctxt_cmd_sel {
	QDMA_CTXT_CMD_SEL_SW_C2H,
	QDMA_CTXT_CMD_SEL_SW_H2C,
	QDMA_CTXT_CMD_SEL_HW_C2H,
	QDMA_CTXT_CMD_SEL_HW_H2C,
	QDMA_CTXT_CMD_SEL_CR_C2H,
	QDMA_CTXT_CMD_SEL_CR_H2C,
	QDMA_CTXT_CMD_SEL_CMPL,
	QDMA_CTXT_CMD_SEL_PFCH,
	QDMA_CTXT_CMD_SEL_INTR_COAL,
	QDMA_CTXT_CMD_SEL_PASID_RAM_LOW,
	QDMA_CTXT_CMD_SEL_PASID_RAM_HIGH,
	QDMA_CTXT_CMD_SEL_TIMER,
	QDMA_CTXT_CMD_SEL_FMAP
};

union qdma_ctxt_cmd {
	u32 word;
	struct {
		u32 busy:1;
		u32 sel:4;
		u32 op:2;
		u32 qid:11;
		u32 rsvd:14;
	} bits;
};

/* context programming */
#define QDMA_CTXT_PROG_TIMEOUT_US             (500*1000) /* 500ms */
#define QDMA_CTXT_PROG_POLL_INTERVAL_US       100        /* 100us */
#define QDMA_CTXT_PROG_NUM_DATA_REGS          8

/* software context */
#define QDMA_SW_CTXT_NUM_WORDS                5
#define QDMA_SW_CTXT_DESC_BASE_GET_L_MASK     GENMASK_ULL(31, 0)
#define QDMA_SW_CTXT_DESC_BASE_GET_H_MASK     GENMASK_ULL(63, 32)
#define QDMA_SW_CTXT_W4_INTR_AGGR_MASK        BIT(11)
#define QDMA_SW_CTXT_W4_VEC_MASK              GENMASK(10, 0)
#define QDMA_SW_CTXT_W3_DESC_BASE_H_MASK      GENMASK(31, 0)
#define QDMA_SW_CTXT_W2_DESC_BASE_L_MASK      GENMASK(31, 0)
#define QDMA_SW_CTXT_W1_IS_MM_MASK            BIT(31)
#define QDMA_SW_CTXT_W1_MRKR_DIS_MASK         BIT(30)
#define QDMA_SW_CTXT_W1_IRQ_REQ_MASK          BIT(29)
#define QDMA_SW_CTXT_W1_ERR_WB_SENT_MASK      BIT(28)
#define QDMA_SW_CTXT_W1_ERR_MASK              GENMASK(27, 26)
#define QDMA_SW_CTXT_W1_IRQ_NO_LAST_MASK      BIT(25)
#define QDMA_SW_CTXT_W1_PORT_ID_MASK          GENMASK(24, 22)
#define QDMA_SW_CTXT_W1_IRQ_EN_MASK           BIT(21)
#define QDMA_SW_CTXT_W1_WBK_EN_MASK           BIT(20)
#define QDMA_SW_CTXT_W1_MM_CHN_MASK           BIT(19)
#define QDMA_SW_CTXT_W1_BYPASS_MASK           BIT(18)
#define QDMA_SW_CTXT_W1_DESC_SZ_MASK          GENMASK(17, 16)
#define QDMA_SW_CTXT_W1_RNG_SZ_MASK           GENMASK(15, 12)
#define QDMA_SW_CTXT_W1_FETCH_MAX_MASK        GENMASK(7, 5)
#define QDMA_SW_CTXT_W1_AT_MASK               BIT(4)
#define QDMA_SW_CTXT_W1_WBI_INTVL_EN_MASK     BIT(3)
#define QDMA_SW_CTXT_W1_WBI_CHK_MASK          BIT(2)
#define QDMA_SW_CTXT_W1_FCRD_EN_MASK          BIT(1)
#define QDMA_SW_CTXT_W1_QEN_MASK              BIT(0)
#define QDMA_SW_CTXT_W0_FUNC_ID_MASK          GENMASK(24, 17)
#define QDMA_SW_CTXT_W0_IRQ_ARM_MASK          BIT(16)
#define QDMA_SW_CTXT_W0_PIDX_MASK             GENMASK(15, 0)

/* Hardware context */
#define QDMA_HW_CTXT_NUM_WORDS                2
#define QDMA_HW_CTXT_W1_FETCH_PEND_MASK       GENMASK(14, 11)
#define QDMA_HW_CTXT_W1_EVENT_PEND_MASK       BIT(10)
#define QDMA_HW_CTXT_W1_IDL_STP_B_MASK        BIT(9)
#define QDMA_HW_CTXT_W1_DESC_PEND_MASK        BIT(8)
#define QDMA_HW_CTXT_W0_CRD_USE_MASK          GENMASK(31, 16)
#define QDMA_HW_CTXT_W0_CIDX_MASK             GENMASK(15, 0)

/* Credit context */
#define QDMA_CR_CTXT_NUM_WORDS                1
#define QDMA_CR_CTXT_W0_CREDIT_MASK           GENMASK(15, 0)

/* C2H prefetch context */
#define QDMA_PFCH_CTXT_NUM_WORDS              2
#define QDMA_PFCH_CTXT_SW_CRDT_GET_H_MASK     GENMASK(15, 3)
#define QDMA_PFCH_CTXT_SW_CRDT_GET_L_MASK     GENMASK(2, 0)
#define QDMA_PFCH_CTXT_W1_VALID_MASK          BIT(13)
#define QDMA_PFCH_CTXT_W1_SW_CRDT_H_MASK      GENMASK(12, 0)
#define QDMA_PFCH_CTXT_W0_SW_CRDT_L_MASK      GENMASK(31, 29)
#define QDMA_PFCH_CTXT_W0_IN_PFCH_MASK        BIT(28)
#define QDMA_PFCH_CTXT_W0_PFCH_EN_MASK        BIT(27)
#define QDMA_PFCH_CTXT_W0_ERR_MASK            BIT(26)
#define QDMA_PFCH_CTXT_W0_PORT_ID_MASK        GENMASK(7, 5)
#define QDMA_PFCH_CTXT_W0_BUFSZ_IDX_MASK      GENMASK(4, 1)
#define QDMA_PFCH_CTXT_W0_BYPASS_MASK         BIT(0)

/* C2H completion context */
#define QDMA_CMPL_CTXT_NUM_WORDS              5
#define QDMA_CMPL_CTXT_BADDR_GET_H_MASK       GENMASK_ULL(63, 38)
#define QDMA_CMPL_CTXT_BADDR_GET_L_MASK       GENMASK_ULL(37, 12)
#define QDMA_CMPL_CTXT_PIDX_GET_H_MASK        GENMASK(15, 4)
#define QDMA_CMPL_CTXT_PIDX_GET_L_MASK        GENMASK(3, 0)
#define QDMA_CMPL_CTXT_W4_INTR_AGGR_MASK      BIT(15)
#define QDMA_CMPL_CTXT_W4_VEC_MASK            GENMASK(14, 4)
#define QDMA_CMPL_CTXT_W4_AT_MASK             BIT(3)
#define QDMA_CMPL_CTXT_W4_OVF_CHK_DIS_MASK    BIT(2)
#define QDMA_CMPL_CTXT_W4_FULL_UPD_MASK       BIT(1)
#define QDMA_CMPL_CTXT_W4_TIMER_RUNNING_MASK  BIT(0)
#define QDMA_CMPL_CTXT_W3_USER_TRIG_PEND_MASK BIT(31)
#define QDMA_CMPL_CTXT_W3_ERR_MASK            GENMASK(30, 29)
#define QDMA_CMPL_CTXT_W3_VALID_MASK          BIT(28)
#define QDMA_CMPL_CTXT_W3_CIDX_MASK           GENMASK(27, 12)
#define QDMA_CMPL_CTXT_W3_PIDX_H_MASK         GENMASK(11, 0)
#define QDMA_CMPL_CTXT_W2_PIDX_L_MASK         GENMASK(31, 28)
#define QDMA_CMPL_CTXT_W2_DESC_SZ_MASK        GENMASK(27, 26)
#define QDMA_CMPL_CTXT_W2_BADDR_H_MASK        GENMASK(25, 0)
#define QDMA_CMPL_CTXT_W1_BADDR_L_MASK        GENMASK(31, 6)
#define QDMA_CMPL_CTXT_W0_RNGSZ_IDX_MASK      GENMASK(31, 28)
#define QDMA_CMPL_CTXT_W0_COLOR_MASK          BIT(27)
#define QDMA_CMPL_CTXT_W0_INTR_ST_MASK        GENMASK(26, 25)
#define QDMA_CMPL_CTXT_W0_TIMER_IDX_MASK      GENMASK(24, 21)
#define QDMA_CMPL_CTXT_W0_COUNTER_IDX_MASK    GENMASK(20, 17)
#define QDMA_CMPL_CTXT_W0_FUNC_ID_MASK        GENMASK(12, 5)
#define QDMA_CMPL_CTXT_W0_TRIG_MODE_MASK      GENMASK(4, 2)
#define QDMA_CMPL_CTXT_W0_INTR_EN_MASK        BIT(1)
#define QDMA_CMPL_CTXT_W0_STAT_EN_MASK        BIT(0)

/* interrupt context */
#define QDMA_INTR_CTXT_NUM_WORDS              3
#define QDMA_INTR_CTXT_BADDR_GET_H_MASK       GENMASK_ULL(51, 49)
#define QDMA_INTR_CTXT_BADDR_GET_M_MASK       GENMASK_ULL(48, 17)
#define QDMA_INTR_CTXT_BADDR_GET_L_MASK       GENMASK_ULL(16, 0)
#define QDMA_INTR_CTXT_W2_AT_MASK             BIT(18)
#define QDMA_INTR_CTXT_W2_PIDX_MASK           GENMASK(17, 6)
#define QDMA_INTR_CTXT_W2_PAGE_SIZE_MASK      GENMASK(5, 3)
#define QDMA_INTR_CTXT_W2_BADDR_H_MASK        GENMASK(2, 0)
#define QDMA_INTR_CTXT_W1_BADDR_M_MASK        GENMASK(31, 0)
#define QDMA_INTR_CTXT_W0_BADDR_L_MASK        GENMASK(31, 15)
#define QDMA_INTR_CTXT_W0_COLOR_MASK          BIT(14)
#define QDMA_INTR_CTXT_W0_INTR_ST_MASK        BIT(13)
#define QDMA_INTR_CTXT_W0_VEC_ID_MASK         GENMASK(11, 1)
#define QDMA_INTR_CTXT_W0_VALID_MASK          BIT(0)

/* function map context */
#define QDMA_FMAP_CTXT_NUM_WORDS              2
#define QDMA_FMAP_CTXT_W1_QMAX_MASK           GENMASK(11, 0)
#define QDMA_FMAP_CTXT_W0_QBASE_MASK          GENMASK(10, 0)

/**
 * qdma_sw_ctxt - descriptor queue software context
 **/
struct qdma_sw_ctxt {
	u32 pidx:16;            /* producer index */
	u32 irq_arm:1;          /* interrupt arm bit */
	u32 func_id:8;
	u32 rsvd0:7;
	u32 qen:1;
	u32 fcrd_en:1;          /* enable fetch credit */
	u32 wbi_chk:1;          /* writeback/interrupt after pending check */
	u32 wbi_intvl_en:1;     /* writeback/interrupt interval */
	u32 at:1;               /* address tanslation */
	u32 fetch_max:3;        /* maximum number of outstanding descriptor fetches */
	u32 rsvd1:4;
	u32 rngsz_idx:4;        /* descriptor ring size index */
	u32 desc_sz:2;          /* descriptor fetch size */
	u32 bypass:1;
	u32 mm_chn:1;
	u32 wbk_en:1;           /* writeback enable */
	u32 irq_en:1;           /* interrupt enable */
	u32 port_id:3;
	u32 irq_no_last:1;
	u32 err:2;              /* error status */
	u32 err_wb_sent:1;
	u32 irq_req:1;          /* interrupt due to error waiting to be sent */
	u32 mrkr_dis:1;         /* disable marker */
	u32 is_mm:1;
	u64 desc_base;          /* base address of descriptor ring */
	u32 vec:11;             /* MSI-X vector number */
	u32 intr_aggr:1;        /* enable interrupt aggregation */
};

/**
 * qdma_hw_ctxt - descriptor queue hardware context
 **/
struct qdma_hw_ctxt {
	u32 cidx:16;            /* consumer index */
	u32 crd_use:16;         /* credits consumed */
	u32 rsvd0:8;
	u32 desc_pend:1;        /* descriptors pending */
	u32 idl_stp_b:1;        /* queue invalid and no descriptors pending */
	u32 event_pend:1;       /* event pending */
	u32 fetch_pend:4;       /* descriptor fetch pending */
	u32 rsvd1:1;
};

/**
 * qdma_cr_ctxt - descriptor queue credit context
 **/
struct qdma_cr_ctxt {
	u32 credit:16;          /* fetch credits received */
	u32 rsvd:16;
};

/**
 * qdma_pfch_ctxt - descriptor queue prefetch context
 */
struct qdma_pfch_ctxt {
	u32 bypass:1;
	u32 bufsz_idx:4;        /* C2H buffer size index */
	u32 port_id:3;
	u32 rsvd0:18;
	u32 err:1;              /* error detected on this queue */
	u32 pfch_en:1;          /* enable prefetch */
	u32 in_pfch:1;          /* queue in prefetch */
	u32 sw_crdt:16;         /* software credit */
	u32 valid:1;
};

/**
 * qdma_cmpl_ctxt - descriptor queue completion context
 */
struct qdma_cmpl_ctxt {
	u32 stat_en:1;          /* enable completion status writeback */
	u32 intr_en:1;          /* enable completion interrupts */
	u32 trig_mode:3;
	u32 func_id:8;
	u32 rsvd0:4;
	u32 counter_idx:4;      /* C2H counter register index */
	u32 timer_idx:4;        /* C2H timer register index */
	u32 intr_st:2;          /* interrupt state */
	u32 color:1;
	u32 rngsz_idx:4;        /* completion ring size index */
	u32 rsvd1:6;
	u64 baddr;              /* completion ring base address */
	u32 desc_sz:2;          /* descriptor size */
	u32 pidx:16;            /* producer index */
	u32 cidx:16;            /* consumer index */
	u32 valid:1;
	u32 err:2;              /* error status */
	u32 user_trig_pend:1;
	u32 timer_running:1;    /* whether timer is running on this queue */
	u32 full_upd:1;         /* full update */
	u32 ovf_chk_dis:1;      /* completion ring overflow check disable */
	u32 at:1;               /* address translation */
	u32 vec:11;             /* interrupt vector */
	u32 intr_aggr:1;        /* interrupt aggregration */
	u32 rsvd2:17;
};

/**
 * qdma_fmap_ctxt - function map context
 **/
struct qdma_fmap_ctxt {
	u32 qbase:11;
	u32 rsvd0:21;
	u32 qmax:12;
	u32 rsvd1:20;
};

/**
 * qdma_write_sw_ctxt - Write descriptor queue software context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 * @ctxt: pointer to QDMA descriptor queue software context
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_write_sw_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir,
		       const struct qdma_sw_ctxt *ctxt);

/**
 * qdma_clear_sw_ctxt - Clear descriptor queue software context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_clear_sw_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir);

/**
 * qdma_invalidate_sw_ctxt - Invalidate descriptor queue software context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_invalidate_sw_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir);

/**
 * qdma_clear_hw_ctxt - Clear descriptor queue hardware context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_clear_hw_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir);

/**
 * qdma_invalidate_hw_ctxt - Invalidate descriptor queue hardware context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_invalidate_hw_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir);

/**
 * qdma_clear_cr_ctxt - Clear descriptor queue credit context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_clear_cr_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir);

/**
 * qdma_invalidate_cr_ctxt - Invalidate descriptor queue credit context
 * @dev: pointer to QDMA device
 * @qid: per-function queue ID
 * @dir: Queue direction (QDMA_C2H or QDMA_H2C)
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_invalidate_cr_ctxt(struct qdma_dev *dev, u16 qid, enum qdma_dir dir);

/**
 * qdma_write_pfch_ctxt - Write descriptor queue prefetch context
 * @dev: pointer to QDMA device
 * @qid: per-function C2H queue ID
 * @ctxt: pointer to QDMA descriptor queue prefetch context
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_write_pfch_ctxt(struct qdma_dev *dev, u16 qid,
			 const struct qdma_pfch_ctxt *ctxt);

/**
 * qdma_clear_pfch_ctxt - Clear descriptor queue prefetch context
 * @dev: pointer to QDMA device
 * @qid: per-function C2H queue ID
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_clear_pfch_ctxt(struct qdma_dev *dev, u16 qid);

/**
 * qdma_invalidate_pfch_ctxt - Invalidate descriptor queue prefetch context
 * @dev: pointer to QDMA device
 * @qid: per-function C2H queue ID
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_invalidate_pfch_ctxt(struct qdma_dev *dev, u16 qid);

/**
 * qdma_write_cmpl_ctxt - Write descriptor queue completion context
 * @dev: pointer to QDMA device
 * @qid: per-function C2H queue ID
 * @ctxt: pointer to QDMA descriptor queue completion context
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_write_cmpl_ctxt(struct qdma_dev *dev, u16 qid,
			 const struct qdma_cmpl_ctxt *ctxt);

/**
 * qdma_clear_cmpl_ctxt - Clear descriptor queue completion context
 * @dev: pointer to QDMA device
 * @qid: per-function C2H queue ID
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_clear_cmpl_ctxt(struct qdma_dev *dev, u16 qid);

/**
 * qdma_invalidate_cmpl_ctxt - Invalidate descriptor queue completion context
 * @dev: pointer to QDMA device
 * @qid: per-function C2H queue ID
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_invalidate_cmpl_ctxt(struct qdma_dev *dev, u16 qid);

/**
 * qdma_write_fmap_ctxt - Write function map context
 * @dev: pointer to QDMA device
 * @ctxt: pointer to QDMA function map context
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_write_fmap_ctxt(struct qdma_dev *dev,
			 const struct qdma_fmap_ctxt *ctxt);

/**
 * qdma_clear_fmap_ctxt - Clear function map context
 * @dev: pointer to QDMA device
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_clear_fmap_ctxt(struct qdma_dev *dev);

/**
 * qdma_invalidate_fmap_ctxt - Invalidate function map context
 * @dev: pointer to QDMA device
 *
 * Returns 0 on success, negative on failure
 **/
int qdma_invalidate_fmap_ctxt(struct qdma_dev *dev);

#endif
