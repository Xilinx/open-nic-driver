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
#ifndef __QDMA_REGISTER_H__
#define __QDMA_REGISTER_H__

#include <linux/bitops.h>

#include "onic_common.h"
#include "qdma_device.h"

/**
 * qdma_read_reg - Read QDMA register
 * @qdev: pointer to QDMA device
 * @offset: register offset
 *
 * Returns the value of register
 **/
static inline u32 qdma_read_reg(struct qdma_dev *qdev, u32 offset)
{
	return ioread32(qdev->addr + offset);
}

/**
 * qdma_write_reg - Write value to QDMA register
 * @qdev: pointer to QDMA device
 * @offset: register offset
 * @val: value to be written
 **/
static inline void qdma_write_reg(struct qdma_dev *qdev, u32 offset, u32 val)
{
	iowrite32(val, qdev->addr + offset);
}

/* ------------------------- QDMA_TRQ_SEL_IND (0x00800) ----------------*/
#define QDMA_OFFSET_IND_CTXT_DATA                           0x804
#define QDMA_OFFSET_IND_CTXT_MASK                           0x824
#define QDMA_OFFSET_IND_CTXT_CMD                            0x844
#define     QDMA_IND_CTXT_CMD_BUSY_MASK                     0x1

/* ------------------------ QDMA_TRQ_SEL_GLBL (0x00200)-------------------*/
#define QDMA_OFFSET_GLBL_RNG_SZ                             0x204
#define QDMA_OFFSET_GLBL_SCRATCH                            0x244
#define QDMA_OFFSET_GLBL_ERR_STAT                           0x248
#define QDMA_OFFSET_GLBL_ERR_MASK                           0x24C
#define QDMA_OFFSET_GLBL_DSC_CFG                            0x250
#define     QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK               GENMASK(2, 0)
#define     QDMA_GLBL_DSC_CFG_MAX_DSC_FETCH_MASK            GENMASK(5, 3)
#define QDMA_OFFSET_GLBL_DSC_ERR_STS                        0x254
#define QDMA_OFFSET_GLBL_DSC_ERR_MSK                        0x258
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG0                       0x25C
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG1                       0x260
#define QDMA_OFFSET_GLBL_TRQ_ERR_STS                        0x264
#define QDMA_OFFSET_GLBL_TRQ_ERR_MSK                        0x268
#define QDMA_OFFSET_GLBL_TRQ_ERR_LOG                        0x26C
#define QDMA_OFFSET_GLBL_DSC_DBG_DAT0                       0x270
#define QDMA_OFFSET_GLBL_DSC_DBG_DAT1                       0x274
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG2                       0x27C
#define QDMA_OFFSET_GLBL_INTERRUPT_CFG                      0x2C4
#define     QDMA_GLBL_INTR_CFG_EN_LGCY_INTR_MASK            BIT(0)
#define     QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK              BIT(1)

/* ------------------------ QDMA_TRQ_SEL_FMAP (0x00400)-------------------*/
#define QDMA_OFFSET_SEL_FMAP                                0x400
#define     QDMA_SEL_FMAP_QBASE_MASK                        GENMASK(10, 0)
#define     QDMA_SEL_FMAP_QMAX_MASK                         GENMASK(22, 11)

/* ------------------------- QDMA_TRQ_SEL_C2H (0x00A00) ------------------*/
#define QDMA_OFFSET_C2H_TIMER_CNT                           0xA00
#define QDMA_OFFSET_C2H_CNT_TH                              0xA40
#define QDMA_OFFSET_C2H_QID2VEC_MAP_QID                     0xA80
#define QDMA_OFFSET_C2H_QID2VEC_MAP                         0xA84
#define QDMA_OFFSET_C2H_STAT_S_AXIS_C2H_ACCEPTED            0xA88
#define QDMA_OFFSET_C2H_STAT_S_AXIS_CMPL_ACCEPTED           0xA8C
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_PKT_ACCEPTED          0xA90
#define QDMA_OFFSET_C2H_STAT_AXIS_PKG_CMP                   0xA94
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_ACCEPTED              0xA98
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_CMP                   0xA9C
#define QDMA_OFFSET_C2H_STAT_WRQ_OUT                        0xAA0
#define QDMA_OFFSET_C2H_STAT_WPL_REN_ACCEPTED               0xAA4
#define QDMA_OFFSET_C2H_STAT_TOTAL_WRQ_LEN                  0xAA8
#define QDMA_OFFSET_C2H_STAT_TOTAL_WPL_LEN                  0xAAC
#define QDMA_OFFSET_C2H_BUF_SZ                              0xAB0
#define QDMA_OFFSET_C2H_ERR_STAT                            0xAF0
#define QDMA_OFFSET_C2H_ERR_MASK                            0xAF4
#define QDMA_OFFSET_C2H_FATAL_ERR_STAT                      0xAF8
#define QDMA_OFFSET_C2H_FATAL_ERR_MASK                      0xAFC
#define QDMA_OFFSET_C2H_FATAL_ERR_ENABLE                    0xB00
#define QDMA_OFFSET_C2H_ERR_INT                             0xB04
#define QDMA_OFFSET_C2H_PFCH_CFG                            0xB08
#define     QDMA_C2H_EVT_QCNT_TH_MASK                       GENMASK(31, 25)
#define     QDMA_C2H_PFCH_QCNT_MASK                         GENMASK(24, 18)
#define     QDMA_C2H_NUM_PFCH_MASK                          GENMASK(17, 9)
#define     QDMA_C2H_PFCH_FL_TH_MASK                        GENMASK(8, 0)
#define QDMA_OFFSET_C2H_INT_TIMER_TICK                      0xB0C
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_DROP_ACCEPTED         0xB10
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_ERR_ACCEPTED          0xB14
#define QDMA_OFFSET_C2H_STAT_DESC_REQ                       0xB18
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_0                0xB1C
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_1                0xB20
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_2                0xB24
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_3                0xB28
#define QDMA_OFFSET_C2H_DBG_PFCH_ERR_CTXT                   0xB2C
#define QDMA_OFFSET_C2H_FIRST_ERR_QID                       0xB30
#define QDMA_OFFSET_C2H_STAT_NUM_CMPL_IN                    0xB34
#define QDMA_OFFSET_C2H_STAT_NUM_CMPL_OUT                   0xB38
#define QDMA_OFFSET_C2H_STAT_NUM_CMPL_DRP                   0xB3C
#define QDMA_OFFSET_C2H_STAT_NUM_STAT_DESC_OUT              0xB40
#define QDMA_OFFSET_C2H_STAT_NUM_DSC_CRDT_SENT              0xB44
#define QDMA_OFFSET_C2H_STAT_NUM_FCH_DSC_RCVD               0xB48
#define QDMA_OFFSET_C2H_STAT_NUM_BYP_DSC_RCVD               0xB4C
#define QDMA_OFFSET_C2H_WB_COAL_CFG                         0xB50
#define     QDMA_C2H_MAX_BUF_SZ_MASK                        GENMASK(31, 26)
#define     QDMA_C2H_TICK_VAL_MASK                          GENMASK(25, 14)
#define     QDMA_C2H_TICK_CNT_MASK                          GENMASK(13, 2)
#define     QDMA_C2H_SET_GLB_FLUSH_MASK                     BIT(1)
#define     QDMA_C2H_DONE_GLB_FLUSH_MASK                    BIT(0)
#define QDMA_OFFSET_C2H_INTR_H2C_REQ                        0xB54
#define QDMA_OFFSET_C2H_INTR_C2H_MM_REQ                     0xB58
#define QDMA_OFFSET_C2H_INTR_ERR_INT_REQ                    0xB5C
#define QDMA_OFFSET_C2H_INTR_C2H_ST_REQ                     0xB60
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK        0xB64
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL       0xB68
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX    0xB6C
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL      0xB70
#define QDMA_OFFSET_C2H_INTR_C2H_ST_MSIX_ACK                0xB74
#define QDMA_OFFSET_C2H_INTR_C2H_ST_MSIX_FAIL               0xB78
#define QDMA_OFFSET_C2H_INTR_C2H_ST_NO_MSIX                 0xB7C
#define QDMA_OFFSET_C2H_INTR_C2H_ST_CTXT_INVAL              0xB80
#define QDMA_OFFSET_C2H_STAT_WR_CMP                         0xB84
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_4                0xB88
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_5                0xB8C
#define QDMA_OFFSET_C2H_DBG_PFCH_QID                        0xB90
#define QDMA_OFFSET_C2H_DBG_PFCH                            0xB94
#define QDMA_OFFSET_C2H_INT_DEBUG                           0xB98
#define QDMA_OFFSET_C2H_STAT_IMM_ACCEPTED                   0xB9C
#define QDMA_OFFSET_C2H_STAT_MARKER_ACCEPTED                0xBA0
#define QDMA_OFFSET_C2H_STAT_DISABLE_CMP_ACCEPTED           0xBA4
#define QDMA_OFFSET_C2H_PAYLOAD_FIFO_CRDT_CNT               0xBA8
#define QDMA_OFFSET_C2H_PFCH_CACHE_DEPTH                    0xBE0
#define QDMA_OFFSET_C2H_CMPL_COAL_BUF_DEPTH                 0xBE4

/* ------------------------- QDMA_TRQ_SEL_H2C (0x00E00) ------------------*/
#define QDMA_OFFSET_H2C_ERR_STAT                            0xE00
#define QDMA_OFFSET_H2C_ERR_MASK                            0xE04
#define QDMA_OFFSET_H2C_FIRST_ERR_QID                       0xE08
#define QDMA_OFFSET_H2C_DBG_REG0                            0xE0C
#define QDMA_OFFSET_H2C_DBG_REG1                            0xE10
#define QDMA_OFFSET_H2C_DBG_REG2                            0xE14
#define QDMA_OFFSET_H2C_DBG_REG3                            0xE18
#define QDMA_OFFSET_H2C_DBG_REG4                            0xE1C
#define QDMA_OFFSET_H2C_FATAL_ERR_EN                        0xE20
#define QDMA_OFFSET_H2C_REQ_THROT                           0xE24
#define     QDMA_H2C_REQ_THROT_EN_REQ_MASK                  BIT(31)
#define     QDMA_H2C_REQ_THRESH_MASK                        GENMASK(25, 17)
#define     QDMA_H2C_REQ_THROT_EN_DATA_MASK                 BIT(16)
#define     QDMA_H2C_DATA_THRESH_MASK                       GENMASK(15, 0)


/* ------------------------- QDMA_TRQ_SEL_H2C_MM (0x1200) ----------------*/
#define QDMA_OFFSET_H2C_MM_CONTROL                          0x1204
#define QDMA_OFFSET_H2C_MM_CONTROL_W1S                      0x1208
#define QDMA_OFFSET_H2C_MM_CONTROL_W1C                      0x120C
#define QDMA_OFFSET_H2C_MM_STATUS                           0x1240
#define QDMA_OFFSET_H2C_MM_STATUS_RC                        0x1244
#define QDMA_OFFSET_H2C_MM_COMPLETED_DESC_COUNT             0x1248
#define QDMA_OFFSET_H2C_MM_ERR_CODE_EN_MASK                 0x1254
#define QDMA_OFFSET_H2C_MM_ERR_CODE                         0x1258
#define QDMA_OFFSET_H2C_MM_ERR_INFO                         0x125C
#define QDMA_OFFSET_H2C_MM_PERF_MON_CONTROL                 0x12C0
#define QDMA_OFFSET_H2C_MM_PERF_MON_CYCLE_COUNT_0           0x12C4
#define QDMA_OFFSET_H2C_MM_PERF_MON_CYCLE_COUNT_1           0x12C8
#define QDMA_OFFSET_H2C_MM_PERF_MON_DATA_COUNT_0            0x12CC
#define QDMA_OFFSET_H2C_MM_PERF_MON_DATA_COUNT_1            0x12D0
#define QDMA_OFFSET_H2C_MM_DEBUG                            0x12E8

/* ------------------------- QDMA_TRQ_SEL_C2H_MM (0x1000) ----------------*/
#define QDMA_OFFSET_C2H_MM_CONTROL                          0x1004
#define QDMA_OFFSET_C2H_MM_CONTROL_W1S                      0x1008
#define QDMA_OFFSET_C2H_MM_CONTROL_W1C                      0x100C
#define QDMA_OFFSET_C2H_MM_STATUS                           0x1040
#define QDMA_OFFSET_C2H_MM_STATUS_RC                        0x1044
#define QDMA_OFFSET_C2H_MM_COMPLETED_DESC_COUNT             0x1048
#define QDMA_OFFSET_C2H_MM_ERR_CODE_EN_MASK                 0x1054
#define QDMA_OFFSET_C2H_MM_ERR_CODE                         0x1058
#define QDMA_OFFSET_C2H_MM_ERR_INFO                         0x105C
#define QDMA_OFFSET_C2H_MM_PERF_MON_CONTROL                 0x10C0
#define QDMA_OFFSET_C2H_MM_PERF_MON_CYCLE_COUNT_0           0x10C4
#define QDMA_OFFSET_C2H_MM_PERF_MON_CYCLE_COUNT_1           0x10C8
#define QDMA_OFFSET_C2H_MM_PERF_MON_DATA_COUNT_0            0x10CC
#define QDMA_OFFSET_C2H_MM_PERF_MON_DATA_COUNT_1            0x10D0
#define QDMA_OFFSET_C2H_MM_DEBUG                            0x10E8

/* ------------------------- QDMA_TRQ_SEL_GLBL1 (0x0) -----------------*/
#define QDMA_OFFSET_CONFIG_BLOCK_ID                         0x0
#define     QDMA_CONFIG_BLOCK_ID_MASK                       GENMASK(31, 16)

/* ------------------------- QDMA_TRQ_SEL_GLBL2 (0x00100) ----------------*/
#define QDMA_OFFSET_GLBL2_ID                                0x100
#define QDMA_OFFSET_GLBL2_PF_BARLITE_INT                    0x104
#define     QDMA_GLBL2_PF3_BAR_MAP_MASK                     GENMASK(23, 18)
#define     QDMA_GLBL2_PF2_BAR_MAP_MASK                     GENMASK(17, 12)
#define     QDMA_GLBL2_PF1_BAR_MAP_MASK                     GENMASK(11, 6)
#define     QDMA_GLBL2_PF0_BAR_MAP_MASK                     GENMASK(5, 0)
#define QDMA_OFFSET_GLBL2_PF_VF_BARLITE_INT                 0x108
#define QDMA_OFFSET_GLBL2_PF_BARLITE_EXT                    0x10C
#define QDMA_OFFSET_GLBL2_PF_VF_BARLITE_EXT                 0x110
#define QDMA_OFFSET_GLBL2_CHANNEL_INST                      0x114
#define QDMA_OFFSET_GLBL2_CHANNEL_MDMA                      0x118
#define     QDMA_GLBL2_ST_C2H_MASK                          BIT(16)
#define     QDMA_GLBL2_ST_H2C_MASK                          BIT(17)
#define     QDMA_GLBL2_MM_C2H_MASK                          BIT(8)
#define     QDMA_GLBL2_MM_H2C_MASK                          BIT(0)
#define QDMA_OFFSET_GLBL2_CHANNEL_STRM                      0x11C
#define QDMA_OFFSET_GLBL2_CHANNEL_QDMA_CAP                  0x120
#define     QDMA_GLBL2_MULTQ_MAX_MASK                       GENMASK(11, 0)
#define QDMA_OFFSET_GLBL2_CHANNEL_PASID_CAP                 0x128
#define QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET                  0x12C
#define QDMA_OFFSET_GLBL2_SYSTEM_ID                         0x130
#define QDMA_OFFSET_GLBL2_MISC_CAP                          0x134
#define     QDMA_GLBL2_EVEREST_IP_MASK                      BIT(28)
#define     QDMA_GLBL2_VIVADO_RELEASE_MASK                  GENMASK(27, 24)
#define     QDMA_GLBL2_RTL_VERSION_MASK                     GENMASK(23, 16)
#define     QDMA_GLBL2_MM_CMPL_EN_MASK                      BIT(2)
#define     QDMA_GLBL2_FLR_PRESENT_MASK                     BIT(1)
#define     QDMA_GLBL2_MAILBOX_EN_MASK                      BIT(0)
#define QDMA_OFFSET_GLBL2_DBG_PCIE_RQ0                      0x1B8
#define QDMA_OFFSET_GLBL2_DBG_PCIE_RQ1                      0x1BC
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_WR0                     0x1C0
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_WR1                     0x1C4
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_RD0                     0x1C8
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_RD1                     0x1CC

/* VF BARs identification */
#define QDMA_OFFSET_VF_USER_BAR_ID                          0x1018
#define QDMA_OFFSET_VF_CONFIG_BAR_ID                        0x1014

/* FLR programming */
#define QDMA_OFFSET_VF_REG_FLR_STATUS                       0x1100
#define QDMA_OFFSET_PF_REG_FLR_STATUS                       0x2500
#define     QDMA_FLR_STATUS_MASK                            0x1

/* VF qdma version */
#define QDMA_OFFSET_VF_VERSION                              0x1014
#define     QDMA_GLBL2_VF_EVEREST_IP_MASK                   BIT(12)
#define     QDMA_GLBL2_VF_VIVADO_RELEASE_MASK               GENMASK(11, 8)
#define     QDMA_GLBL2_VF_RTL_VERSION_MASK                  GENMASK(7, 0)

/* ------------------------- QDMA_TRQ_SEL_QUEUE_PF (0x18000) ----------------*/

#define QDMA_OFFSET_DMAP_SEL_INTR_CIDX                      0x18000
#define QDMA_OFFSET_DMAP_SEL_H2C_DESC_PIDX                  0x18004
#define QDMA_OFFSET_DMAP_SEL_C2H_DESC_PIDX                  0x18008
#define QDMA_OFFSET_DMAP_SEL_CMPL_CIDX                      0x1800C

#define QDMA_OFFSET_VF_DMAP_SEL_INTR_CIDX                   0x3000
#define QDMA_OFFSET_VF_DMAP_SEL_H2C_DESC_PIDX               0x3004
#define QDMA_OFFSET_VF_DMAP_SEL_C2H_DESC_PIDX               0x3008
#define QDMA_OFFSET_VF_DMAP_SEL_CMPL_CIDX                   0x300C

#define     QDMA_DMAP_SEL_INTR_SW_CIDX_MASK                 GENMASK(15, 0)
#define     QDMA_DMAP_SEL_INTR_RING_IDX_MASK                GENMASK(23, 16)
#define     QDMA_DMAP_SEL_DESC_PIDX_MASK                    GENMASK(15, 0)
#define     QDMA_DMAP_SEL_DESC_IRQ_ARM_MASK                 BIT(16)
#define     QDMA_DMAP_SEL_CMPL_IRQ_ARM_MASK                 BIT(28)
#define     QDMA_DMAP_SEL_CMPL_STAT_EN_MASK                 BIT(27)
#define     QDMA_DMAP_SEL_CMPL_TRIG_MODE_MASK               GENMASK(26, 24)
#define     QDMA_DMAP_SEL_CMPL_TIMER_IDX_MASK               GENMASK(23, 20)
#define     QDMA_DMAP_SEL_CMPL_COUNTER_IDX_MASK             GENMASK(19, 16)
#define     QDMA_DMAP_SEL_CMPL_CIDX_MASK                    GENMASK(15, 0)

/* ------------------------- Hardware Errors ------------------------------ */
#define TOTAL_LEAF_ERROR_AGGREGATORS                        7

#define QDMA_OFFSET_GLBL_ERR_INT                            0xB04
#define     QDMA_GLBL_ERR_FUNC_MASK                         GENMASK(7, 0)
#define     QDMA_GLBL_ERR_VEC_MASK                          GENMASK(22, 12)
#define     QDMA_GLBL_ERR_ARM_MASK                          BIT(24)

#define QDMA_OFFSET_GLBL_ERR_STAT                           0x248
#define QDMA_OFFSET_GLBL_ERR_MASK                           0x24C
#define     QDMA_GLBL_ERR_RAM_SBE_MASK                      BIT(0)
#define     QDMA_GLBL_ERR_RAM_DBE_MASK                      BIT(1)
#define     QDMA_GLBL_ERR_DSC_MASK                          BIT(2)
#define     QDMA_GLBL_ERR_TRQ_MASK                          BIT(3)
#define     QDMA_GLBL_ERR_ST_C2H_MASK                       BIT(8)
#define     QDMA_GLBL_ERR_ST_H2C_MASK                       BIT(11)

#define QDMA_OFFSET_C2H_ERR_STAT                            0xAF0
#define QDMA_OFFSET_C2H_ERR_MASK                            0xAF4
#define     QDMA_C2H_ERR_MTY_MISMATCH_MASK                  BIT(0)
#define     QDMA_C2H_ERR_LEN_MISMATCH_MASK                  BIT(1)
#define     QDMA_C2H_ERR_QID_MISMATCH_MASK                  BIT(3)
#define     QDMA_C2H_ERR_DESC_RSP_ERR_MASK                  BIT(4)
#define     QDMA_C2H_ERR_ENG_WPL_DATA_PAR_ERR_MASK          BIT(6)
#define     QDMA_C2H_ERR_MSI_INT_FAIL_MASK                  BIT(7)
#define     QDMA_C2H_ERR_ERR_DESC_CNT_MASK                  BIT(9)
#define     QDMA_C2H_ERR_PORTID_CTXT_MISMATCH_MASK          BIT(10)
#define     QDMA_C2H_ERR_PORTID_BYP_IN_MISMATCH_MASK        BIT(11)
#define     QDMA_C2H_ERR_CMPL_INV_Q_ERR_MASK                BIT(12)
#define     QDMA_C2H_ERR_CMPL_QFULL_ERR_MASK                BIT(13)
#define     QDMA_C2H_ERR_CMPL_CIDX_ERR_MASK                 BIT(14)
#define     QDMA_C2H_ERR_CMPL_PRTY_ERR_MASK                 BIT(15)
#define     QDMA_C2H_ERR_ALL_MASK                           0xFEDB

#define QDMA_OFFSET_C2H_FATAL_ERR_STAT                      0xAF8
#define QDMA_OFFSET_C2H_FATAL_ERR_MASK                      0xAFC
#define     QDMA_C2H_FATAL_ERR_MTY_MISMATCH_MASK            BIT(0)
#define     QDMA_C2H_FATAL_ERR_LEN_MISMATCH_MASK            BIT(1)
#define     QDMA_C2H_FATAL_ERR_QID_MISMATCH_MASK            BIT(3)
#define     QDMA_C2H_FATAL_ERR_TIMER_FIFO_RAM_RDBE_MASK     BIT(4)
#define     QDMA_C2H_FATAL_ERR_PFCH_II_RAM_RDBE_MASK        BIT(8)
#define     QDMA_C2H_FATAL_ERR_CMPL_CTXT_RAM_RDBE_MASK      BIT(9)
#define     QDMA_C2H_FATAL_ERR_PFCH_CTXT_RAM_RDBE_MASK      BIT(10)
#define     QDMA_C2H_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE_MASK  BIT(11)
#define     QDMA_C2H_FATAL_ERR_INT_CTXT_RAM_RDBE_MASK       BIT(12)
#define     QDMA_C2H_FATAL_ERR_CMPL_COAL_DATA_RAM_RDBE_MASK BIT(14)
#define     QDMA_C2H_FATAL_ERR_TUSER_FIFO_RAM_RDBE_MASK     BIT(15)
#define     QDMA_C2H_FATAL_ERR_QID_FIFO_RAM_RDBE_MASK       BIT(16)
#define     QDMA_C2H_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE_MASK   BIT(17)
#define     QDMA_C2H_FATAL_ERR_WPL_DATA_PAR_MASK            BIT(18)
#define     QDMA_C2H_FATAL_ERR_ALL_MASK                     0x7DF1B

#define QDMA_OFFSET_H2C_ERR_STAT                            0xE00
#define QDMA_OFFSET_H2C_ERR_MASK                            0xE04
#define     QDMA_H2C_ERR_ZERO_LEN_DESC_MASK                 BIT(0)
#define     QDMA_H2C_ERR_CSI_MOP_MASK                       BIT(1)
#define     QDMA_H2C_ERR_NO_DMA_DSC_MASK                    BIT(2)
#define     QDMA_H2C_ERR_SBE_MASK                           BIT(3)
#define     QDMA_H2C_ERR_DBE_MASK                           BIT(4)
#define     QDMA_H2C_ERR_ALL_MASK                           0x1F

#define QDMA_OFFSET_GLBL_DSC_ERR_STAT                       0x254
#define QDMA_OFFSET_GLBL_DSC_ERR_MASK                       0x258
#define     QDMA_GLBL_DSC_ERR_POISON_MASK                   BIT(0)
#define     QDMA_GLBL_DSC_ERR_UR_CA_MASK                    BIT(1)
#define     QDMA_GLBL_DSC_ERR_PARAM_MASK                    BIT(2)
#define     QDMA_GLBL_DSC_ERR_ADDR_MASK                     BIT(3)
#define     QDMA_GLBL_DSC_ERR_TAG_MASK                      BIT(4)
#define     QDMA_GLBL_DSC_ERR_FLR_MASK                      BIT(5)
#define     QDMA_GLBL_DSC_ERR_TIMEOUT_MASK                  BIT(9)
#define     QDMA_GLBL_DSC_ERR_DAT_POISON_MASK               BIT(16)
#define     QDMA_GLBL_DSC_ERR_FLR_CANCEL_MASK               BIT(19)
#define     QDMA_GLBL_DSC_ERR_DMA_MASK                      BIT(20)
#define     QDMA_GLBL_DSC_ERR_DSC_MASK                      BIT(21)
#define     QDMA_GLBL_DSC_ERR_RQ_CANCEL_MASK                BIT(22)
#define     QDMA_GLBL_DSC_ERR_DBE_MASK                      BIT(23)
#define     QDMA_GLBL_DSC_ERR_SBE_MASK                      BIT(24)
#define     QDMA_GLBL_DSC_ERR_ALL_MASK                      0x1F9023F

#define QDMA_OFFSET_GLBL_TRQ_ERR_STAT                       0x264
#define QDMA_OFFSET_GLBL_TRQ_ERR_MASK                       0x268
#define     QDMA_GLBL_TRQ_ERR_UNMAPPED_MASK                 BIT(0)
#define     QDMA_GLBL_TRQ_ERR_QID_RANGE_MASK                BIT(1)
#define     QDMA_GLBL_TRQ_ERR_VF_ACCESS_MASK                BIT(2)
#define     QDMA_GLBL_TRQ_ERR_TCP_TIMEOUT_MASK              BIT(3)
#define     QDMA_GLBL_TRQ_ERR_ALL_MASK                      0xF

#define QDMA_OFFSET_RAM_SBE_STAT                            0xF4
#define QDMA_OFFSET_RAM_SBE_MASK                            0xF0
#define     QDMA_SBE_ERR_MI_H2C0_DAT_MASK                   BIT(0)
#define     QDMA_SBE_ERR_MI_C2H0_DAT_MASK                   BIT(4)
#define     QDMA_SBE_ERR_H2C_RD_BRG_DAT_MASK                BIT(9)
#define     QDMA_SBE_ERR_H2C_WR_BRG_DAT_MASK                BIT(10)
#define     QDMA_SBE_ERR_C2H_RD_BRG_DAT_MASK                BIT(11)
#define     QDMA_SBE_ERR_C2H_WR_BRG_DAT_MASK                BIT(12)
#define     QDMA_SBE_ERR_FUNC_MAP_MASK                      BIT(13)
#define     QDMA_SBE_ERR_DSC_HW_CTXT_MASK                   BIT(14)
#define     QDMA_SBE_ERR_DSC_CRD_RCV_MASK                   BIT(15)
#define     QDMA_SBE_ERR_DSC_SW_CTXT_MASK                   BIT(16)
#define     QDMA_SBE_ERR_DSC_CPLI_MASK                      BIT(17)
#define     QDMA_SBE_ERR_DSC_CPLD_MASK                      BIT(18)
#define     QDMA_SBE_ERR_PASID_CTXT_RAM_MASK                BIT(19)
#define     QDMA_SBE_ERR_TIMER_FIFO_RAM_MASK                BIT(20)
#define     QDMA_SBE_ERR_PAYLOAD_FIFO_RAM_MASK              BIT(21)
#define     QDMA_SBE_ERR_QID_FIFO_RAM_MASK                  BIT(22)
#define     QDMA_SBE_ERR_TUSER_FIFO_RAM_MASK                BIT(23)
#define     QDMA_SBE_ERR_WB_COAL_DATA_RAM_MASK              BIT(24)
#define     QDMA_SBE_ERR_INT_QID2VEC_RAM_MASK               BIT(25)
#define     QDMA_SBE_ERR_INT_CTXT_RAM_MASK                  BIT(26)
#define     QDMA_SBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT(27)
#define     QDMA_SBE_ERR_PFCH_CTXT_RAM_MASK                 BIT(28)
#define     QDMA_SBE_ERR_WB_CTXT_RAM_MASK                   BIT(29)
#define     QDMA_SBE_ERR_PFCH_LL_RAM_MASK                   BIT(30)
#define     QDMA_SBE_ERR_H2C_PEND_FIFO_MASK                 BIT(31)
#define     QDMA_SBE_ERR_ALL_MASK                           0xFFFFFF11

#define QDMA_OFFSET_RAM_DBE_STAT                            0xFC
#define QDMA_OFFSET_RAM_DBE_MASK                            0xF8
#define     QDMA_DBE_ERR_MI_H2C0_DAT_MASK                   BIT(0)
#define     QDMA_DBE_ERR_MI_C2H0_DAT_MASK                   BIT(4)
#define     QDMA_DBE_ERR_H2C_RD_BRG_DAT_MASK                BIT(9)
#define     QDMA_DBE_ERR_H2C_WR_BRG_DAT_MASK                BIT(10)
#define     QDMA_DBE_ERR_C2H_RD_BRG_DAT_MASK                BIT(11)
#define     QDMA_DBE_ERR_C2H_WR_BRG_DAT_MASK                BIT(12)
#define     QDMA_DBE_ERR_FUNC_MAP_MASK                      BIT(13)
#define     QDMA_DBE_ERR_DSC_HW_CTXT_MASK                   BIT(14)
#define     QDMA_DBE_ERR_DSC_CRD_RCV_MASK                   BIT(15)
#define     QDMA_DBE_ERR_DSC_SW_CTXT_MASK                   BIT(16)
#define     QDMA_DBE_ERR_DSC_CPLI_MASK                      BIT(17)
#define     QDMA_DBE_ERR_DSC_CPLD_MASK                      BIT(18)
#define     QDMA_DBE_ERR_PASID_CTXT_RAM_MASK                BIT(19)
#define     QDMA_DBE_ERR_TIMER_FIFO_RAM_MASK                BIT(20)
#define     QDMA_DBE_ERR_PAYLOAD_FIFO_RAM_MASK              BIT(21)
#define     QDMA_DBE_ERR_QID_FIFO_RAM_MASK                  BIT(22)
#define     QDMA_DBE_ERR_TUSER_FIFO_RAM_MASK                BIT(23)
#define     QDMA_DBE_ERR_WB_COAL_DATA_RAM_MASK              BIT(24)
#define     QDMA_DBE_ERR_INT_QID2VEC_RAM_MASK               BIT(25)
#define     QDMA_DBE_ERR_INT_CTXT_RAM_MASK                  BIT(26)
#define     QDMA_DBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT(27)
#define     QDMA_DBE_ERR_PFCH_CTXT_RAM_MASK                 BIT(28)
#define     QDMA_DBE_ERR_WB_CTXT_RAM_MASK                   BIT(29)
#define     QDMA_DBE_ERR_PFCH_LL_RAM_MASK                   BIT(30)
#define     QDMA_DBE_ERR_H2C_PEND_FIFO_MASK                 BIT(31)
#define     QDMA_DBE_ERR_ALL_MASK                           0xFFFFFF11

#endif
