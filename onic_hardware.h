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
#ifndef __ONIC_HARDWARE_H__
#define __ONIC_HARDWARE_H__

#include "qdma_export.h"

#define ONIC_MAX_CMACS			2
#define ONIC_CMAC_CORE_VERSION		0x00000301

struct onic_hardware {
    int RS_FEC;
	unsigned long qdma;
	u8 num_cmacs;
	void __iomem *addr;	/* mapping of shell registers */
};

struct onic_qdma_h2c_param {
	u8 rngcnt_idx;
	dma_addr_t dma_addr;
	u16 vid;
};

struct onic_qdma_c2h_param {
	u8 bufsz_idx;
	u8 desc_rngcnt_idx;
	u8 cmpl_rngcnt_idx;
	u8 cmpl_desc_sz;
	dma_addr_t desc_dma_addr;
	dma_addr_t cmpl_dma_addr;
	u16 vid;
};

struct onic_private;

/**
 * onic_ring_count - get the number of descriptors from index
 * @idx: index into the pool
 *
 * Return the number of descriptors pointed at index
 **/
u16 onic_ring_count(u8 idx);

/**
 * onic_init_hardware - initialize NIC hardware
 * @priv: pointer to driver private data
 *
 * Return 0 on success, negative on failure
 **/
int onic_init_hardware(struct onic_private *priv);

/**
 * onic_clear_hardware - clear NIC hardware
 * @priv: pointer to driver private data
 **/
void onic_clear_hardware(struct onic_private *priv);

/**
 * onic_qdma_init_error_interrupt - initialize QDMA error interrupt
 * @qdma: handle to QDMA device
 * @vid: vector ID
 **/
void onic_qdma_init_error_interrupt(unsigned long qdma, u16 vid);

/**
 * onic_qdma_clear_error_interrupt - invalidate QDMA error interrupt
 * @qdma: handle to QDMA device
 **/
void onic_qdma_clear_error_interrupt(unsigned long qdma);

/**
 * onic_qdma_init_tx_queue - initialize a QDMA H2C queue
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @param: pointer to QDMA H2C queue parameters
 *
 * Return 0 on success, negative on failure
 **/
int onic_qdma_init_tx_queue(unsigned long qdma, u16 qid,
			    const struct onic_qdma_h2c_param *param);

/**
 * onic_qdma_init_rx_queue - initialize a QDMA C2H queue
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @param: pointer to QDMA C2H queue parameters
 *
 * Return 0 on success, negative on failure
 **/
int onic_qdma_init_rx_queue(unsigned long qdma, u16 qid,
			    const struct onic_qdma_c2h_param *param);

/**
 * onic_qdma_clear_tx_queue - clear a QDMA H2C queue
 * @qdma: handle to QDMA device
 * @qid: queue ID
 **/
void onic_qdma_clear_tx_queue(unsigned long qdma, u16 qid);

/**
 * onic_qdma_clear_rx_queue - clear a QDMA C2H queue
 * @qdma: handle to QDMA device
 * @qid: queue ID
 **/
void onic_qdma_clear_rx_queue(unsigned long qdma, u16 qid);

/**
 * onic_set_tx_head - set TX ring head pointer
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @head: head pointer of the TX ring, i.e., next_to_use
 **/
void onic_set_tx_head(unsigned long qdma, u16 qid, u16 head);

/**
 * onic_set_rx_head - set RX ring head pointer
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @head: head pointer of the RX ring, i.e., next_to_use
 **/
void onic_set_rx_head(unsigned long qdma, u16 qid, u16 head);

/**
 * onic_set_completion_tail - set RX completion ring tail pointer
 * @qdma: handle to QDMA device
 * @qid: queue ID
 * @tail: tail pointer of the RX completion ring, i.e., next_to_clean
 **/
void onic_set_completion_tail(unsigned long qdma, u16 qid, u16 tail, u8 irq_arm);

#endif
