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
#include <linux/pci.h>

#include "onic_hardware.h"
#include "onic_register.h"
#include "onic.h"
#include "qdma_register.h"
#include "qdma_context.h"
#include "qdma_error_info.h"

/* default CSR values for QDMA */
#define DEFAULT_MAX_DESC_FETCH			6
#define DEFAULT_WB_INTVL			QDMA_WB_INTVL_4
#define DEFAULT_PFCH_STOP_THRES			256
#define DEFAULT_PFCH_NUM_ENTRIES_PER_Q		8
#define DEFAULT_PFCH_MAX_Q_CNT			16
#define DEFAULT_C2H_INTR_TIMER_TICK		25
#define DEFAULT_CMPL_COAL_TIMER_CNT		5
#define DEFAULT_CMPL_COAL_TIMER_TICK		25
#define DEFAULT_CMPL_COAL_MAX_BUFSZ		32
#define DEFAULT_H2C_THROT_DATA_THRES		0x4000
#define DEFAULT_THROT_EN_DATA			1
#define DEFAULT_THROT_EN_REQ			0
#define DEFAULT_H2C_THROT_REQ_THRES		0x60

#define RX_ALIGN_TIMEOUT_MS			100
#define CMAC_RESET_WAIT_MS			1

static const u16 rngcnt_pool[QDMA_NUM_DESC_RNGCNT] = {
	4096, 64, 128, 192, 256, 384, 512, 768,
	1024, 1536, 3072, 4096, 6144, 8192, 12288, 16384
};

static const u16 c2h_bufsz_pool[QDMA_NUM_C2H_BUFSZ] = {
	4096, 256, 512, 1024, 2048, 3968, 4096, 4096,
	4096, 4096, 4096, 4096, 4096, 8192, 9018, 16384
};

static const u16 c2h_timer_pool[QDMA_NUM_C2H_TIMERS] = {
	10, 2, 4, 5, 8, 10, 15, 20, 25,
	30, 50, 75, 100, 125, 150, 200
};

static const u16 c2h_thres_pool[QDMA_NUM_C2H_COUNTERS] = {
	64, 2, 4, 8, 16, 24, 32, 48,
	80, 96, 112, 128, 144, 160, 176, 192
};

u16 onic_ring_count(u8 idx)
{
	return (idx < QDMA_NUM_DESC_RNGCNT) ? rngcnt_pool[idx] : 0;
}

/**
 * onic_qdma_init_csr - initialize QDMA config/status registers
 * @qdev: pointer to QDMA device
 *
 * This function writes to various H2C and C2H registers, getting QDMA ready for
 * queue operations.  Values of these registers are hard-coded.
 **/
static void onic_qdma_init_csr(struct qdma_dev *qdev)
{
	u32 offset, val;
	int i;

	/* initialize descriptor ring size registers */
	for (i = 0; i < QDMA_NUM_DESC_RNGCNT; ++i) {
		offset = QDMA_OFFSET_GLBL_RNG_SZ + (i * 4);
		val = rngcnt_pool[i];
		qdma_write_reg(qdev, offset, val);
	}

	/* initialize C2H buffer size registers */
	for (i = 0; i < QDMA_NUM_C2H_BUFSZ; ++i) {
		offset = QDMA_OFFSET_C2H_BUF_SZ + (i * 4);
		val = c2h_bufsz_pool[i];
		qdma_write_reg(qdev, offset, val);
	}

	/* set QDMA_C2H_INT_TIMER_TICK (0xB0C) register to 25, which corresponds
	 * to 100ns (1 tick = 4ns for 250MHz user clock)
	 */
	offset = QDMA_OFFSET_C2H_INT_TIMER_TICK;
	val = DEFAULT_C2H_INTR_TIMER_TICK;
	qdma_write_reg(qdev, offset, val);

	/* initialize C2H timer counter registers. */
	for (i = 0; i < QDMA_NUM_C2H_TIMERS; ++i) {
		offset = QDMA_OFFSET_C2H_TIMER_CNT + (i * 4);
		val = c2h_timer_pool[i];
		qdma_write_reg(qdev, offset, val);
	}

	/* initialize C2H counter threshold registers */
	for (i = 0; i < QDMA_NUM_C2H_COUNTERS; ++i) {
		offset = QDMA_OFFSET_C2H_CNT_TH + (i * 4);
		val = c2h_thres_pool[i];
		qdma_write_reg(qdev, offset, val);
	}

	/* set QDMA_GLBL_DSC_CFG (0x250) register for max descriptor fetch and
	 * writeback interval
	 */
	offset = QDMA_OFFSET_GLBL_DSC_CFG;
	val = (FIELD_SET(QDMA_GLBL_DSC_CFG_MAX_DSC_FETCH_MASK,
			 DEFAULT_MAX_DESC_FETCH) |
	       FIELD_SET(QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK,
			 DEFAULT_WB_INTVL));
	qdma_write_reg(qdev, offset, val);

	/* read QDMA_C2H_PFCH_CACHE_DEPTH (0xBE0) register and set
	 * QDMA_C2H_PFCH_CFG (0xB08) register accordingly
	 */
	val = qdma_read_reg(qdev, QDMA_OFFSET_C2H_PFCH_CACHE_DEPTH);
	offset = QDMA_OFFSET_C2H_PFCH_CFG;
	val = (FIELD_SET(QDMA_C2H_PFCH_FL_TH_MASK,
			 DEFAULT_PFCH_STOP_THRES) |
	       FIELD_SET(QDMA_C2H_NUM_PFCH_MASK,
			 DEFAULT_PFCH_NUM_ENTRIES_PER_Q) |
	       FIELD_SET(QDMA_C2H_PFCH_QCNT_MASK,
			 (val >> 1)) |
	       FIELD_SET(QDMA_C2H_EVT_QCNT_TH_MASK,
			 ((val >> 1) - 2)));
	qdma_write_reg(qdev, offset, val);

	/* read QDMA_C2H_CMPL_COAL_BUF_DEPTH (0xBE4) register and set
	 * QDMA_C2H_WB_COAL_CFG (0xB50) register accordingly
	 *
	 * Note that the tick field is set to 25, which corresponds to 100ns (1
	 * tick = 4ns for 250MHz user clock).
	 *
	 * TODO: verify the value for C2H_MAX_BUF_SZ.  QDMA document says that
	 * this should be set to QDMA_C2H_CMPL_COAL_BUF_DEPTH.buf_depth - 2; but
	 * the libqdma code ignores the minus 2 part.
	 */
	val = qdma_read_reg(qdev, QDMA_OFFSET_C2H_CMPL_COAL_BUF_DEPTH);
	offset = QDMA_OFFSET_C2H_WB_COAL_CFG;
	val = (FIELD_SET(QDMA_C2H_TICK_CNT_MASK,
			 DEFAULT_CMPL_COAL_TIMER_CNT) |
	       FIELD_SET(QDMA_C2H_TICK_VAL_MASK,
			 DEFAULT_CMPL_COAL_TIMER_TICK) |
	       FIELD_SET(QDMA_C2H_MAX_BUF_SZ_MASK, val));
	qdma_write_reg(qdev, offset, val);

	/* set QDMA_H2C_REQ_THROT (0xE24) register.
	 *
	 * Data and request-based throttle are enabled only if the respective
	 * threshold is set to a nonzero value.
	 */
	offset = QDMA_OFFSET_H2C_REQ_THROT;
	val = (FIELD_SET(QDMA_H2C_DATA_THRESH_MASK,
			 DEFAULT_H2C_THROT_DATA_THRES) |
	       FIELD_SET(QDMA_H2C_REQ_THROT_EN_DATA_MASK,
			 DEFAULT_THROT_EN_DATA) |
	       FIELD_SET(QDMA_H2C_REQ_THRESH_MASK,
			 DEFAULT_H2C_THROT_REQ_THRES) |
	       FIELD_SET(QDMA_H2C_REQ_THROT_EN_REQ_MASK,
			 DEFAULT_THROT_EN_REQ));
	qdma_write_reg(qdev, offset, val);
}



static int onic_enable_cmac(struct onic_hardware *hw, u8 cmac_id)
{
	if (cmac_id != 0 && cmac_id != 1)
		return -EINVAL;

    if (hw->RS_FEC) {
	/* Enable RS-FEC for CMACs with RS-FEC implemented */
	onic_write_reg(hw, CMAC_OFFSET_RSFEC_CONF_ENABLE(cmac_id), 0x3);
	onic_write_reg(hw, CMAC_OFFSET_RSFEC_CONF_IND_CORRECTION(cmac_id), 0x7);
    }

	if (cmac_id == 0) {
		onic_write_reg(hw, SYSCFG_OFFSET_SHELL_RESET, 0x2);
		while ((onic_read_reg(hw, SYSCFG_OFFSET_SHELL_STATUS) & 0x2) != 0x2)
			mdelay(CMAC_RESET_WAIT_MS);
	} else {
		onic_write_reg(hw, SYSCFG_OFFSET_SHELL_RESET, 0x4);
		while ((onic_read_reg(hw, SYSCFG_OFFSET_SHELL_STATUS) & 0x4) != 0x4)
			mdelay(CMAC_RESET_WAIT_MS);
	}

	onic_write_reg(hw, CMAC_OFFSET_CONF_RX_1(cmac_id), 0x1);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_1(cmac_id), 0x10);

	
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_1(cmac_id), 0x1);

	/* RX flow control */
	onic_write_reg(hw, CMAC_OFFSET_CONF_RX_FC_CTRL_1(cmac_id), 0x00003DFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_RX_FC_CTRL_2(cmac_id), 0x0001C631);

	/* TX flow control */
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_QNTA_1(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_QNTA_2(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_QNTA_3(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_QNTA_4(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_QNTA_5(cmac_id), 0x0000FFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_RFRH_1(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_RFRH_2(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_RFRH_3(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_RFRH_4(cmac_id), 0xFFFFFFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_RFRH_5(cmac_id), 0x0000FFFF);
	onic_write_reg(hw, CMAC_OFFSET_CONF_TX_FC_CTRL_1(cmac_id), 0x000001FF);

	return 0;
}

int onic_init_hardware(struct onic_private *priv)
{
	struct onic_hardware *hw = &priv->hw;
	struct pci_dev *pdev = priv->pdev;
	struct qdma_dev *qdev;
	struct qdma_fmap_ctxt fmap_ctxt;
	u16 qbase, qmax, func_id;
	u32 val;
	u8 master_pf = test_bit(ONIC_FLAG_MASTER_PF, priv->flags);
	int i, rv;

        priv->hw.RS_FEC = priv->RS_FEC;

	/* shell registers uses BAR-2 */
	hw->addr = pci_iomap_range(pdev, 2, SHELL_START, SHELL_MAXLEN);
	if (!hw->addr)
		return -EINVAL;

	/* QDMA IP registers uses BAR-0 */
	qdev = qdma_create_dev(pdev, 0);
	if (!qdev)
		return -ENOMEM;

	func_id = PCI_FUNC(pdev->devfn);
	qbase = func_id * ONIC_MAX_QUEUES;
	qmax = max(priv->num_tx_queues, priv->num_rx_queues);

	/* initialize QDMA function map context */
	memset(&fmap_ctxt, 0, sizeof(struct qdma_fmap_ctxt));
	fmap_ctxt.qbase = qbase;
	fmap_ctxt.qmax = qmax;
	rv = qdma_clear_fmap_ctxt(qdev);
	if (rv < 0)
		goto clear_hardware;
	rv = qdma_write_fmap_ctxt(qdev, &fmap_ctxt);
	if (rv < 0)
		goto clear_hardware;

	/* inform shell about the function map */
	val = (FIELD_SET(QDMA_FUNC_QCONF_QBASE_MASK, qbase) |
	       FIELD_SET(QDMA_FUNC_QCONF_NUMQ_MASK, qmax));
	onic_write_reg(hw, QDMA_FUNC_OFFSET_QCONF(func_id), val);

	/* initialize indirection table */
	for (i = 0; i < 128; ++i) {
		u32 val = (i % qmax) & 0x0000FFFF;
		u32 offset = QDMA_FUNC_OFFSET_INDIR_TABLE(func_id, i);
		onic_write_reg(hw, offset, val);
	}

	/* initialize global registers if device is a master PF */
	if (master_pf)
		onic_qdma_init_csr(qdev);

        hw->qdma = (unsigned long)qdev;

	/* get the number of CMAC instances */
	for (i = 0; i < ONIC_MAX_CMACS; ++i) {
		val = onic_read_reg(hw, CMAC_OFFSET_CORE_VERSION(i));
		if (val != ONIC_CMAC_CORE_VERSION)
			break;
		if (master_pf)
			onic_enable_cmac(hw, i);
	}
	hw->num_cmacs = i;
	dev_info(&pdev->dev, "Number of CMAC instances = %d", hw->num_cmacs);

	return 0;

clear_hardware:
	onic_clear_hardware(priv);
	return rv;
}

void onic_clear_hardware(struct onic_private *priv)
{
	struct onic_hardware *hw = &priv->hw;
	struct pci_dev *pdev = priv->pdev;
	struct qdma_dev *qdev = (struct qdma_dev *)hw->qdma;
	u16 func_id = PCI_FUNC(pdev->devfn);

	/* clear the function map in shell */
	onic_write_reg(hw, QDMA_FUNC_OFFSET_QCONF(func_id), 0);

	qdma_invalidate_fmap_ctxt(qdev);
	qdma_destroy_dev(qdev);

	pci_iounmap(pdev, hw->addr);

	memset(hw, 0, sizeof(struct onic_hardware));
}

void onic_qdma_init_error_interrupt(unsigned long qdma, u16 vid)
{
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;
	u32 offset, val;
	int i;

	offset = QDMA_OFFSET_GLBL_ERR_INT;
	val = (FIELD_SET(QDMA_GLBL_ERR_FUNC_MASK, qdev->func_id) |
	       FIELD_SET(QDMA_GLBL_ERR_VEC_MASK, vid) |
	       FIELD_SET(QDMA_GLBL_ERR_ARM_MASK, 0));
	qdma_write_reg(qdev, offset, val);

	for (i = 0; i < NUM_LEAF_ERROR_AGGREGATORS; i++) {
		u32 err_idx = leaf_error_aggregators[i];

		offset = qdma_error_info[err_idx].mask_reg_addr;
		val = qdma_error_info[err_idx].leaf_err_mask;
		qdma_write_reg(qdev, offset, val);

		offset = QDMA_OFFSET_GLBL_ERR_MASK;
		val = qdma_read_reg(qdev, offset);
		val |= FIELD_SET(qdma_error_info[err_idx].glbl_err_mask, 1);
		qdma_write_reg(qdev, offset, val);
	}

	offset = QDMA_OFFSET_GLBL_ERR_INT;
	val = (FIELD_SET(QDMA_GLBL_ERR_FUNC_MASK, qdev->func_id) |
	       FIELD_SET(QDMA_GLBL_ERR_VEC_MASK, vid) |
	       FIELD_SET(QDMA_GLBL_ERR_ARM_MASK, 1));
	qdma_write_reg(qdev, offset, val);
}

void onic_qdma_clear_error_interrupt(unsigned long qdma)
{
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;

	qdma_write_reg(qdev, QDMA_OFFSET_GLBL_ERR_INT, 0);
}

int onic_qdma_init_tx_queue(unsigned long qdma, u16 qid,
			    const struct onic_qdma_h2c_param *param)
{
	const enum qdma_dir dir = QDMA_H2C;
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;
	struct qdma_sw_ctxt sw_ctxt;
	int rv;

	if (qid < 0)
		return qid;

	/* initialize software context */
	memset(&sw_ctxt, 0, sizeof(struct qdma_sw_ctxt));
	sw_ctxt.func_id = qdev->func_id;
	sw_ctxt.qen = 1;
	sw_ctxt.wbk_en = 1;
	sw_ctxt.is_mm = 0;
	sw_ctxt.irq_arm = 0;
	sw_ctxt.irq_en = 0;
	sw_ctxt.desc_sz = 1; /* 1: 16B for H2C stream */
	sw_ctxt.fcrd_en = 0;
	sw_ctxt.wbi_chk = 1;
	sw_ctxt.wbi_intvl_en = 1;
	sw_ctxt.at = 0;
	sw_ctxt.rngsz_idx = param->rngcnt_idx;
	sw_ctxt.desc_base = param->dma_addr;
	sw_ctxt.vec = param->vid;
	sw_ctxt.intr_aggr = 0;

	rv = qdma_clear_sw_ctxt(qdev, qid, dir);
	if (rv < 0)
		goto clear_tx_queue;
	rv = qdma_write_sw_ctxt(qdev, qid, dir, &sw_ctxt);
	if (rv < 0)
		goto clear_tx_queue;

	/* initialize hardware and credit context */
	rv = qdma_clear_hw_ctxt(qdev, qid, dir);
	if (rv < 0)
		goto clear_tx_queue;
	rv = qdma_clear_cr_ctxt(qdev, qid, dir);
	if (rv < 0)
		goto clear_tx_queue;

	return 0;

clear_tx_queue:
	onic_qdma_clear_tx_queue(qdma, qid);
	return rv;
}

int onic_qdma_init_rx_queue(unsigned long qdma, u16 qid,
			    const struct onic_qdma_c2h_param *param)
{
	const enum qdma_dir dir = QDMA_C2H;
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;
	struct qdma_sw_ctxt sw_ctxt;
	struct qdma_pfch_ctxt pfch_ctxt;
	struct qdma_cmpl_ctxt cmpl_ctxt;
	int rv;

	if (qid < 0)
		return qid;

	/* initialize software context */
	memset(&sw_ctxt, 0, sizeof(struct qdma_sw_ctxt));
	sw_ctxt.func_id = qdev->func_id;
	sw_ctxt.qen = 1;
	sw_ctxt.wbk_en = 1;
	sw_ctxt.is_mm = 0;
	sw_ctxt.desc_sz = 0; /* 0: 8B for C2H stream */
	sw_ctxt.fcrd_en = 1;
	sw_ctxt.rngsz_idx = param->desc_rngcnt_idx;
	sw_ctxt.desc_base = param->desc_dma_addr;

	rv = qdma_clear_sw_ctxt(qdev, qid, dir);
	if (rv < 0)
		goto clear_rx_queue;
	rv = qdma_write_sw_ctxt(qdev, qid, dir, &sw_ctxt);
	if (rv < 0)
		goto clear_rx_queue;

	/* initialize hardware and credit context */
	rv = qdma_clear_hw_ctxt(qdev, qid, dir);
	if (rv < 0)
		goto clear_rx_queue;
	rv = qdma_clear_cr_ctxt(qdev, qid, dir);
	if (rv < 0)
		goto clear_rx_queue;

	/* initialize prefetch and completion contexts */
	memset(&pfch_ctxt, 0, sizeof(struct qdma_pfch_ctxt));
	pfch_ctxt.bufsz_idx = param->bufsz_idx;
	pfch_ctxt.pfch_en = 1;
	pfch_ctxt.valid = 1;

	rv = qdma_clear_pfch_ctxt(qdev, qid);
	if (rv < 0)
		goto clear_rx_queue;
	rv = qdma_write_pfch_ctxt(qdev, qid, &pfch_ctxt);
	if (rv < 0)
		goto clear_rx_queue;

	memset(&cmpl_ctxt, 0, sizeof(struct qdma_cmpl_ctxt));
	cmpl_ctxt.stat_en = 1;
	//cmpl_ctxt.stat_en = 0;
	cmpl_ctxt.intr_en = 1;
	cmpl_ctxt.trig_mode = 0x5;
	cmpl_ctxt.func_id = qdev->func_id;
	cmpl_ctxt.counter_idx = 0;
	cmpl_ctxt.timer_idx = 0;
	cmpl_ctxt.color = 1;
	cmpl_ctxt.rngsz_idx = param->cmpl_rngcnt_idx;
	cmpl_ctxt.baddr = param->cmpl_dma_addr;
	cmpl_ctxt.desc_sz = param->cmpl_desc_sz;
	cmpl_ctxt.valid = 1;
	cmpl_ctxt.full_upd = 0;
	cmpl_ctxt.ovf_chk_dis = 0;
	cmpl_ctxt.vec = param->vid;
	cmpl_ctxt.intr_aggr = 0;

	rv = qdma_clear_cmpl_ctxt(qdev, qid);
	if (rv < 0)
		goto clear_rx_queue;
	rv = qdma_write_cmpl_ctxt(qdev, qid, &cmpl_ctxt);
	if (rv < 0)
		goto clear_rx_queue;

	return 0;

clear_rx_queue:
	onic_qdma_clear_rx_queue(qdma, qid);
	return rv;
}

void onic_qdma_clear_tx_queue(unsigned long qdma, u16 qid)
{
	const enum qdma_dir dir = QDMA_H2C;
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;

	if (qid < 0)
		return;

	qdma_invalidate_sw_ctxt(qdev, qid, dir);
	qdma_invalidate_hw_ctxt(qdev, qid, dir);
	qdma_invalidate_cr_ctxt(qdev, qid, dir);
}

void onic_qdma_clear_rx_queue(unsigned long qdma, u16 qid)
{
	const enum qdma_dir dir = QDMA_C2H;
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;

	if (qid < 0)
		return;

	qdma_invalidate_sw_ctxt(qdev, qid, dir);
	qdma_invalidate_hw_ctxt(qdev, qid, dir);
	qdma_invalidate_cr_ctxt(qdev, qid, dir);
	qdma_invalidate_pfch_ctxt(qdev, qid);
	qdma_invalidate_cmpl_ctxt(qdev, qid);
}

/**
 * onic_qdma_set_q_pidx - set QDMA queue producer index
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @dir: queue direction
 * @pidx: producer index
 * @irq_arm: interrupt arm bit for next interrupt generation
 **/
static void onic_qdma_set_q_pidx(unsigned long qdma, u16 qid,
				 enum qdma_dir dir, u16 pidx, u8 irq_arm)
{
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;
	u32 offset, val;
	bool debug = 0;
	if (debug) dev_info(&qdev->pdev->dev, "onic_qdma_set_q_pidx(qid:%u, dir:%x, pidx:%u, irq_qrm:%u)", qid, dir, pidx, irq_arm);

	if (qid < 0)
		return;

	if (dir == QDMA_C2H)
		offset = QDMA_OFFSET_DMAP_SEL_C2H_DESC_PIDX + (qid * 16);
	else
		offset = QDMA_OFFSET_DMAP_SEL_H2C_DESC_PIDX + (qid * 16);

	val = (FIELD_SET(QDMA_DMAP_SEL_DESC_PIDX_MASK, pidx) |
	       FIELD_SET(QDMA_DMAP_SEL_DESC_IRQ_ARM_MASK, irq_arm));
	qdma_write_reg(qdev, offset, val);
}

void onic_set_tx_head(unsigned long qdma, u16 qid, u16 head)
{
	onic_qdma_set_q_pidx(qdma, qid, QDMA_H2C, head, 0);
}

void onic_set_rx_head(unsigned long qdma, u16 qid, u16 head)
{
	onic_qdma_set_q_pidx(qdma, qid, QDMA_C2H, head, 0);
}

/**
 * onic_qdma_set_cmpl_cidx - set QDMA completion queue consumer index
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @cidx: consumer index
 * @counter_idx: index to C2H counter threshold registers
 * @timer_idx: index to C2H timer registers
 * @trig_mode: interrupt and status descriptor trigger mode
 * @stat_en: enable completion status writeback
 * @irq_arm: interrupt arm bit for next interrupt generation
 **/
static void onic_qdma_set_cmpl_cidx(unsigned long qdma, u16 qid, u16 cidx,
				       u8 counter_idx, u8 timer_idx,
				       u8 trig_mode, u8 stat_en, u8 irq_arm)
{
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;
	u32 offset, val;
	bool debug = 0;
	if (debug) dev_info(&qdev->pdev->dev, "onic_qdma_set_cmpl_cidx(qid:%u, cidx:%u, idx:%u, timser_idx:%u, trig_mode:%u irq_arm:%u)", qid, cidx, counter_idx, timer_idx, trig_mode, irq_arm);

	if (qid < 0)
		return;

	offset = QDMA_OFFSET_DMAP_SEL_CMPL_CIDX + (qid * 16);

	val = (FIELD_SET(QDMA_DMAP_SEL_CMPL_CIDX_MASK, cidx) |
	       FIELD_SET(QDMA_DMAP_SEL_CMPL_COUNTER_IDX_MASK, counter_idx) |
	       FIELD_SET(QDMA_DMAP_SEL_CMPL_TIMER_IDX_MASK, timer_idx) |
	       FIELD_SET(QDMA_DMAP_SEL_CMPL_TRIG_MODE_MASK, trig_mode) |
	       FIELD_SET(QDMA_DMAP_SEL_CMPL_STAT_EN_MASK, stat_en) |
	       FIELD_SET(QDMA_DMAP_SEL_CMPL_IRQ_ARM_MASK, irq_arm));
	qdma_write_reg(qdev, offset, val);
}

void onic_set_completion_tail(unsigned long qdma, u16 qid, u16 tail, u8 irq_arm)
{
	struct qdma_dev *qdev = (struct qdma_dev *)qdma;
	u8 trig_mode = 5; // trigger from: user, count, or timer
	u8 stat_en = 1;  // enabled is necessary for getting proper completion_status, e.g. for knowing pidx
	bool debug = 0;
	if (debug) dev_info(&qdev->pdev->dev, "onic_set_completion_tail (qid:%u, tail:%u, irq_arm:%u)", qid, tail, irq_arm);
	onic_qdma_set_cmpl_cidx(qdma, qid, tail, 0, 0, trig_mode, stat_en, irq_arm);
}
