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
#include <asm-generic/delay.h>

#include "qdma_register.h"
#include "qdma_context.h"

static DEFINE_MUTEX(ctxt_lock);

/**
 * qdma_get_real_qid - Get physical queue ID from per-function queue ID
 * @qdev: pointer to QDMA device
 * @qid: per-function queue ID
 *
 * This function does not change the validity of per-function qid.  So make sure
 * that it does not go beyond the configured range.
 **/
static inline u16 qdma_get_real_qid(struct qdma_dev *qdev, u16 qid)
{
	return (qid + qdev->q_base);
}

/**
 * qdma_program_ctxt - Perform QDMA context programming
 * @qdev: pointer to QDMA device
 * @cmd: pointer to context command
 * @data: write data for write command, or read data for read command
 * @len: length of data (in words)
 *
 * Returns 0 on success, negative on failure
 *
 * Depending on the context command, data is treated in different ways.	 For
 * write command, data is used as input and must contain values to be written
 * into context data registers.	 For read command, context is read into context
 * data registers which are then copied into the data buffer, up to the
 * specified length.  For clear and invalidate commands, data should be NULL and
 * len should be 0.
 **/
static int qdma_program_ctxt(struct qdma_dev *qdev,
			     const union qdma_ctxt_cmd *cmd,
			     u32 *data, u8 len)
{
	u32 data_offset, mask_offset;
	int i, num_polls;

	mutex_lock(&ctxt_lock);

	if (cmd->bits.op == QDMA_CTXT_CMD_OP_WR) {
		data_offset = QDMA_OFFSET_IND_CTXT_DATA;
		mask_offset = QDMA_OFFSET_IND_CTXT_MASK;
		for (i = 0; i < QDMA_CTXT_PROG_NUM_DATA_REGS; ++i) {
			if (i < len)
				qdma_write_reg(qdev, data_offset, data[i]);
			else
				qdma_write_reg(qdev, data_offset, 0);

			qdma_write_reg(qdev, mask_offset, 0xFFFFFFFF);

			data_offset += 4;
			mask_offset += 4;
		}
	}

	qdma_write_reg(qdev, QDMA_OFFSET_IND_CTXT_CMD, cmd->word);

	num_polls = QDMA_CTXT_PROG_TIMEOUT_US /
		QDMA_CTXT_PROG_POLL_INTERVAL_US;

	for (i = 0; i < num_polls; ++i) {
		u32 val = qdma_read_reg(qdev, QDMA_OFFSET_IND_CTXT_CMD);

		if ((val & QDMA_IND_CTXT_CMD_BUSY_MASK) == 0)
			break;

		if (i == num_polls - 1)
			goto ctxt_prog_timeout;
		udelay(QDMA_CTXT_PROG_POLL_INTERVAL_US);
	}

	if (cmd->bits.op == QDMA_CTXT_CMD_OP_RD) {
		data_offset = QDMA_OFFSET_IND_CTXT_DATA;
		for (i = 0; i < QDMA_CTXT_PROG_NUM_DATA_REGS; ++i) {
			if (i < len)
				data[i] = qdma_read_reg(qdev, data_offset);
			data_offset += 4;
		}
	}

	mutex_unlock(&ctxt_lock);
	return 0;

ctxt_prog_timeout:
	mutex_unlock(&ctxt_lock);
	return -EBUSY;
}

int qdma_write_sw_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir,
		       const struct qdma_sw_ctxt *ctxt)
{
	union qdma_ctxt_cmd cmd;
	u32 desc_base_l, desc_base_h;
	u32 data[QDMA_SW_CTXT_NUM_WORDS] = {0};
	u32 num_words = 0;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_SW_C2H : QDMA_CTXT_CMD_SEL_SW_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_WR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	desc_base_l = (u32)BITFIELD_GET(QDMA_SW_CTXT_DESC_BASE_GET_L_MASK,
				     ctxt->desc_base);
	desc_base_h = (u32)BITFIELD_GET(QDMA_SW_CTXT_DESC_BASE_GET_H_MASK,
				     ctxt->desc_base);

	data[num_words++] =
		FIELD_SET(QDMA_SW_CTXT_W0_PIDX_MASK, ctxt->pidx) |
		FIELD_SET(QDMA_SW_CTXT_W0_IRQ_ARM_MASK, ctxt->irq_arm) |
		FIELD_SET(QDMA_SW_CTXT_W0_FUNC_ID_MASK, ctxt->func_id);

	data[num_words++] =
		FIELD_SET(QDMA_SW_CTXT_W1_QEN_MASK, ctxt->qen) |
		FIELD_SET(QDMA_SW_CTXT_W1_FCRD_EN_MASK, ctxt->fcrd_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_WBI_CHK_MASK, ctxt->wbi_chk) |
		FIELD_SET(QDMA_SW_CTXT_W1_WBI_INTVL_EN_MASK, ctxt->wbi_intvl_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_AT_MASK, ctxt->at) |
		FIELD_SET(QDMA_SW_CTXT_W1_FETCH_MAX_MASK, ctxt->fetch_max) |
		FIELD_SET(QDMA_SW_CTXT_W1_RNG_SZ_MASK, ctxt->rngsz_idx) |
		FIELD_SET(QDMA_SW_CTXT_W1_DESC_SZ_MASK, ctxt->desc_sz) |
		FIELD_SET(QDMA_SW_CTXT_W1_BYPASS_MASK, ctxt->bypass) |
		FIELD_SET(QDMA_SW_CTXT_W1_MM_CHN_MASK, ctxt->mm_chn) |
		FIELD_SET(QDMA_SW_CTXT_W1_WBK_EN_MASK, ctxt->wbk_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_IRQ_EN_MASK, ctxt->irq_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(QDMA_SW_CTXT_W1_IRQ_NO_LAST_MASK, ctxt->irq_no_last) |
		FIELD_SET(QDMA_SW_CTXT_W1_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_SW_CTXT_W1_ERR_WB_SENT_MASK, ctxt->err_wb_sent) |
		FIELD_SET(QDMA_SW_CTXT_W1_IRQ_REQ_MASK, ctxt->irq_req) |
		FIELD_SET(QDMA_SW_CTXT_W1_MRKR_DIS_MASK, ctxt->mrkr_dis) |
		FIELD_SET(QDMA_SW_CTXT_W1_IS_MM_MASK, ctxt->is_mm);

	data[num_words++] = FIELD_SET(QDMA_SW_CTXT_W2_DESC_BASE_L_MASK, desc_base_l);

	data[num_words++] = FIELD_SET(QDMA_SW_CTXT_W3_DESC_BASE_H_MASK, desc_base_h);

	data[num_words++] =
		FIELD_SET(QDMA_SW_CTXT_W4_VEC_MASK, ctxt->vec) |
		FIELD_SET(QDMA_SW_CTXT_W4_INTR_AGGR_MASK, ctxt->intr_aggr);

	BUG_ON(num_words != QDMA_SW_CTXT_NUM_WORDS);
	return qdma_program_ctxt(qdev, &cmd, data, num_words);
}

int qdma_clear_sw_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_SW_C2H : QDMA_CTXT_CMD_SEL_SW_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_invalidate_sw_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_SW_C2H : QDMA_CTXT_CMD_SEL_SW_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_INV;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_clear_hw_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_HW_C2H : QDMA_CTXT_CMD_SEL_HW_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_invalidate_hw_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_HW_C2H : QDMA_CTXT_CMD_SEL_HW_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_INV;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_clear_cr_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_CR_C2H : QDMA_CTXT_CMD_SEL_CR_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_invalidate_cr_ctxt(struct qdma_dev *qdev, u16 qid, enum qdma_dir dir)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = (dir == QDMA_C2H) ?
		QDMA_CTXT_CMD_SEL_CR_C2H : QDMA_CTXT_CMD_SEL_CR_H2C;
	cmd.bits.op = QDMA_CTXT_CMD_OP_INV;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_write_pfch_ctxt(struct qdma_dev *qdev, u16 qid,
			 const struct qdma_pfch_ctxt *ctxt)
{
	union qdma_ctxt_cmd cmd;
	u32 sw_crdt_l, sw_crdt_h;
	u32 data[QDMA_PFCH_CTXT_NUM_WORDS] = {0};
	u32 num_words = 0;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_PFCH;
	cmd.bits.op = QDMA_CTXT_CMD_OP_WR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	sw_crdt_l = (u32)BITFIELD_GET(QDMA_PFCH_CTXT_SW_CRDT_GET_L_MASK,
				   ctxt->sw_crdt);
	sw_crdt_h = (u32)BITFIELD_GET(QDMA_PFCH_CTXT_SW_CRDT_GET_H_MASK,
				   ctxt->sw_crdt);

	data[num_words++] =
		FIELD_SET(QDMA_PFCH_CTXT_W0_BYPASS_MASK, ctxt->bypass) |
		FIELD_SET(QDMA_PFCH_CTXT_W0_BUFSZ_IDX_MASK, ctxt->bufsz_idx) |
		FIELD_SET(QDMA_PFCH_CTXT_W0_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(QDMA_PFCH_CTXT_W0_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_PFCH_CTXT_W0_PFCH_EN_MASK, ctxt->pfch_en) |
		FIELD_SET(QDMA_PFCH_CTXT_W0_IN_PFCH_MASK, ctxt->in_pfch) |
		FIELD_SET(QDMA_PFCH_CTXT_W0_SW_CRDT_L_MASK, sw_crdt_l);

	data[num_words++] =
		FIELD_SET(QDMA_PFCH_CTXT_W1_SW_CRDT_H_MASK, sw_crdt_h) |
		FIELD_SET(QDMA_PFCH_CTXT_W1_VALID_MASK, ctxt->valid);

	BUG_ON(num_words != QDMA_PFCH_CTXT_NUM_WORDS);
	return qdma_program_ctxt(qdev, &cmd, data, num_words);
}

int qdma_clear_pfch_ctxt(struct qdma_dev *qdev, u16 qid)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_PFCH;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_invalidate_pfch_ctxt(struct qdma_dev *qdev, u16 qid)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_PFCH;
	cmd.bits.op = QDMA_CTXT_CMD_OP_INV;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_write_cmpl_ctxt(struct qdma_dev *qdev, u16 qid,
			 const struct qdma_cmpl_ctxt *ctxt)
{
	union qdma_ctxt_cmd cmd;
	u32 baddr_l, baddr_h;
	u32 pidx_l, pidx_h;
	u32 data[QDMA_CMPL_CTXT_NUM_WORDS] = {0};
	u32 num_words = 0;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_CMPL;
	cmd.bits.op = QDMA_CTXT_CMD_OP_WR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	baddr_l = (u32)BITFIELD_GET(QDMA_CMPL_CTXT_BADDR_GET_L_MASK,
				 ctxt->baddr);
	baddr_h = (u32)BITFIELD_GET(QDMA_CMPL_CTXT_BADDR_GET_H_MASK,
				 ctxt->baddr);
	pidx_l = (u32)BITFIELD_GET(QDMA_CMPL_CTXT_PIDX_GET_L_MASK,
				ctxt->pidx);
	pidx_h = (u32)BITFIELD_GET(QDMA_CMPL_CTXT_PIDX_GET_H_MASK,
				ctxt->pidx);

	data[num_words++] =
		FIELD_SET(QDMA_CMPL_CTXT_W0_STAT_EN_MASK, ctxt->stat_en) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_INTR_EN_MASK, ctxt->intr_en) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_TRIG_MODE_MASK, ctxt->trig_mode) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_FUNC_ID_MASK, ctxt->func_id) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_COUNTER_IDX_MASK,
			  ctxt->counter_idx) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_TIMER_IDX_MASK, ctxt->timer_idx) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_INTR_ST_MASK, ctxt->intr_st) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_COLOR_MASK, ctxt->color) |
		FIELD_SET(QDMA_CMPL_CTXT_W0_RNGSZ_IDX_MASK, ctxt->rngsz_idx);

	data[num_words++] =
		FIELD_SET(QDMA_CMPL_CTXT_W1_BADDR_L_MASK, baddr_l);

	data[num_words++] =
		FIELD_SET(QDMA_CMPL_CTXT_W2_BADDR_H_MASK, baddr_h) |
		FIELD_SET(QDMA_CMPL_CTXT_W2_DESC_SZ_MASK, ctxt->desc_sz) |
		FIELD_SET(QDMA_CMPL_CTXT_W2_PIDX_L_MASK, pidx_l);

	data[num_words++] =
		FIELD_SET(QDMA_CMPL_CTXT_W3_PIDX_H_MASK, pidx_h) |
		FIELD_SET(QDMA_CMPL_CTXT_W3_CIDX_MASK, ctxt->cidx) |
		FIELD_SET(QDMA_CMPL_CTXT_W3_VALID_MASK, ctxt->valid) |
		FIELD_SET(QDMA_CMPL_CTXT_W3_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_CMPL_CTXT_W3_USER_TRIG_PEND_MASK,
			  ctxt->user_trig_pend);

	data[num_words++] =
		FIELD_SET(QDMA_CMPL_CTXT_W4_TIMER_RUNNING_MASK,
			  ctxt->timer_running) |
		FIELD_SET(QDMA_CMPL_CTXT_W4_FULL_UPD_MASK, ctxt->full_upd) |
		FIELD_SET(QDMA_CMPL_CTXT_W4_OVF_CHK_DIS_MASK,
			  ctxt->ovf_chk_dis) |
		FIELD_SET(QDMA_CMPL_CTXT_W4_AT_MASK, ctxt->at) |
		FIELD_SET(QDMA_CMPL_CTXT_W4_VEC_MASK, ctxt->vec) |
		FIELD_SET(QDMA_CMPL_CTXT_W4_INTR_AGGR_MASK, ctxt->intr_aggr);

	BUG_ON(num_words != QDMA_CMPL_CTXT_NUM_WORDS);
	return qdma_program_ctxt(qdev, &cmd, data, num_words);
}

int qdma_clear_cmpl_ctxt(struct qdma_dev *qdev, u16 qid)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_CMPL;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_invalidate_cmpl_ctxt(struct qdma_dev *qdev, u16 qid)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_CMPL;
	cmd.bits.op = QDMA_CTXT_CMD_OP_INV;
	cmd.bits.qid = qdma_get_real_qid(qdev, qid);

	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_write_fmap_ctxt(struct qdma_dev *qdev,
			 const struct qdma_fmap_ctxt *ctxt)
{
	union qdma_ctxt_cmd cmd;
	u32 data[QDMA_FMAP_CTXT_NUM_WORDS] = {0};
	u32 num_words = 0;
	int rv;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_FMAP;
	cmd.bits.op = QDMA_CTXT_CMD_OP_WR;
	cmd.bits.qid = qdev->func_id;

	data[num_words++] =
		FIELD_SET(QDMA_FMAP_CTXT_W0_QBASE_MASK, ctxt->qbase);
	data[num_words++] =
		FIELD_SET(QDMA_FMAP_CTXT_W1_QMAX_MASK, ctxt->qmax);

	BUG_ON(num_words != QDMA_FMAP_CTXT_NUM_WORDS);
	rv = qdma_program_ctxt(qdev, &cmd, data, num_words);
	if (rv < 0) {
		qdev->q_base = 0;
		qdev->num_queues = 0;
		return rv;
	}

	qdev->q_base = ctxt->qbase;
	qdev->num_queues = ctxt->qmax;
	return 0;
}

int qdma_clear_fmap_ctxt(struct qdma_dev *qdev)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_FMAP;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdev->func_id;

	qdev->q_base = 0;
	qdev->num_queues = 0;
	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}

int qdma_invalidate_fmap_ctxt(struct qdma_dev *qdev)
{
	union qdma_ctxt_cmd cmd;

	cmd.word = 0;
	cmd.bits.sel = QDMA_CTXT_CMD_SEL_FMAP;
	cmd.bits.op = QDMA_CTXT_CMD_OP_CLR;
	cmd.bits.qid = qdev->func_id;

	qdev->q_base = 0;
	qdev->num_queues = 0;
	return qdma_program_ctxt(qdev, &cmd, NULL, 0);
}
