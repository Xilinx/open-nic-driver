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
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/bpf.h>
#include <linux/filter.h>
#include <linux/bpf_trace.h>

#include "onic_netdev.h"
#include "qdma_access/qdma_register.h"
#include "onic.h"

#define ONIC_RX_DESC_STEP 256

inline static u16 onic_ring_get_real_count(struct onic_ring *ring)
{
	/* Valid writeback entry means one less count of descriptor entries */
	return (ring->wb) ? (ring->count - 1) : ring->count;
}

inline static bool onic_ring_full(struct onic_ring *ring)
{
	u16 real_count = onic_ring_get_real_count(ring);
	return ((ring->next_to_use + 1) % real_count) == ring->next_to_clean;
}

inline static void onic_ring_increment_head(struct onic_ring *ring)
{
	u16 real_count = onic_ring_get_real_count(ring);
	ring->next_to_use = (ring->next_to_use + 1) % real_count;
}

inline static void onic_ring_increment_tail(struct onic_ring *ring)
{
	u16 real_count = onic_ring_get_real_count(ring);
	ring->next_to_clean = (ring->next_to_clean + 1) % real_count;
}

static void onic_tx_clean(struct onic_tx_queue *q)
{
	struct onic_private *priv = netdev_priv(q->netdev);
	struct onic_ring *ring = &q->ring;
	struct qdma_wb_stat wb;
	int work, i;

	if (test_and_set_bit(0, q->state))
		return;

	qdma_unpack_wb_stat(&wb, ring->wb);

	if (wb.cidx == ring->next_to_clean) {
		clear_bit(0, q->state);
		return;
	}

	work = wb.cidx - ring->next_to_clean;
	if (work < 0)
		work += onic_ring_get_real_count(ring);

	for (i = 0; i < work; ++i) {
		struct onic_tx_buffer *buf = &q->buffer[ring->next_to_clean];
		struct sk_buff *skb = buf->skb;

		dma_unmap_single(&priv->pdev->dev, buf->dma_addr, buf->len,
				 DMA_TO_DEVICE);
		dev_kfree_skb_any(skb);

		onic_ring_increment_tail(ring);
	}

	clear_bit(0, q->state);
}

static bool onic_rx_high_watermark(struct onic_rx_queue *q)
{
	struct onic_ring *ring = &q->desc_ring;
	int unused;

	unused = ring->next_to_use - ring->next_to_clean;
	if (ring->next_to_use < ring->next_to_clean)
		unused += onic_ring_get_real_count(ring);

	return (unused < (ONIC_RX_DESC_STEP / 2));
}

static void onic_rx_refill(struct onic_rx_queue *q)
{
	struct onic_private *priv = netdev_priv(q->netdev);
	struct onic_ring *ring = &q->desc_ring;

	ring->next_to_use += ONIC_RX_DESC_STEP;
	ring->next_to_use %= onic_ring_get_real_count(ring);

	onic_set_rx_head(priv->hw.qdma, q->qid, ring->next_to_use);
}

static void *onic_run_xdp(struct onic_rx_queue *rx_queue, struct xdp_buff *xdp_buff) {
	int err, result = ONIC_XDP_PASS;
	struct bpf_prog *xdp_prog;
	u32 act;

	xdp_prog = rx_queue->xdp_prog;
	if (!xdp_prog)
		goto out;

	act = bpf_prog_run_xdp(xdp_prog, xdp_buff);
	switch (act){
		case XDP_PASS:
			break;
    case XDP_TX:
			// TODO:
			// result = onic_xdp_xmit(adapter, xdp_buff);
			break;
    case XDP_REDIRECT:
			err = xdp_do_redirect(rx_queue->netdev, xdp_buff, xdp_prog);
			if (!err)
				result = ONIC_XDP_REDIR;
			else
				result = ONIC_XDP_CONSUMED;
			break;
    default:
			bpf_warn_invalid_xdp_action(act);
			fallthrough;
    case XDP_ABORTED:
			trace_xdp_exception(rx_queue->netdev, xdp_prog, act);
			fallthrough;
    case XDP_DROP:
			result = ONIC_XDP_CONSUMED;
			break;
  }

out:
	return ERR_PTR(-result);
}

static int onic_rx_poll(struct napi_struct *napi, int budget)
{
	struct onic_rx_queue *q =
		container_of(napi, struct onic_rx_queue, napi);
	struct onic_private *priv = netdev_priv(q->netdev);
	u16 qid = q->qid;
	struct onic_ring *desc_ring = &q->desc_ring;
	struct onic_ring *cmpl_ring = &q->cmpl_ring;
	struct qdma_c2h_cmpl cmpl;
	struct qdma_c2h_cmpl_stat cmpl_stat;
	u8 *cmpl_ptr;
	u8 *cmpl_stat_ptr;
	u32 color_stat;
	int work = 0;
	int i, rv;
	bool napi_cmpl_rval = 0;
	bool flipped = 0;
	bool debug = 0;
	void *res;

	struct xdp_buff xdp;
	unsigned int xdp_xmit = 0;

	for (i = 0; i < priv->num_tx_queues; i++)
		onic_tx_clean(priv->tx_queue[i]);

	cmpl_ptr =
		cmpl_ring->desc + QDMA_C2H_CMPL_SIZE * cmpl_ring->next_to_clean;
	cmpl_stat_ptr =
		cmpl_ring->desc + QDMA_C2H_CMPL_SIZE * (cmpl_ring->count - 1);

	qdma_unpack_c2h_cmpl(&cmpl, cmpl_ptr);
	qdma_unpack_c2h_cmpl_stat(&cmpl_stat, cmpl_stat_ptr);

	color_stat = cmpl_stat.color;
	if (debug)
		netdev_info(
			q->netdev,
			"\n rx_poll:  cmpl_stat_pidx %u, color_cmpl_stat %u, cmpl_ring next_to_clean %u, cmpl_stat_cidx %u, intr_state %u, cmpl_ring->count %u",
			cmpl_stat.pidx, color_stat, cmpl_ring->next_to_clean,
			cmpl_stat.cidx, cmpl_stat.intr_state, cmpl_ring->count);

	if (debug)
		netdev_info(
			q->netdev,
			"c2h_cmpl pkt_id %u, pkt_len %u, error %u, color %u cmpl_ring->color:%u",
			cmpl.pkt_id, cmpl.pkt_len, cmpl.err, cmpl.color,
			cmpl_ring->color);

	/* Color of completion entries and completion ring are initialized to 0
	 * and 1 respectively.  When an entry is filled, it has a color bit of
	 * 1, thus making it the same as the completion ring color.  A different
	 * color indicates that we are done with the current batch.  When the
	 * ring index wraps around, the color flips in both software and
	 * hardware.  Therefore, it becomes that completion entries are filled
	 * with a color 0, and completion ring has a color 0 as well.
	 */
	if (cmpl.color != cmpl_ring->color) {
		if (debug)
			netdev_info(
				q->netdev,
				"color mismatch1: cmpl.color %u, cmpl_ring->color %u  cmpl_stat_color %u",
				cmpl.color, cmpl_ring->color, color_stat);
	}

	if (cmpl.err == 1) {
		if (debug)
			netdev_info(q->netdev, "completion error detected in cmpl entry!");
		// todo: need to handle the error ...
		onic_qdma_clear_error_interrupt(priv->hw.qdma);
	}

	// main processing loop for rx_poll
	while ((cmpl_ring->next_to_clean != cmpl_stat.pidx)) {
		struct onic_rx_buffer *buf =
			&q->buffer[desc_ring->next_to_clean];
		struct sk_buff *skb;
		int len = cmpl.pkt_len;
		u8 *data;

		// data is the pointer to the data in the page, and its being passed into the sk_buff struct
		/* maximum packet size is 1514, less than the page size */
		data = (u8 *)(page_address(buf->pg) + buf->offset);
		xdp.rxq = &q->xdp_rxq;
	 	xdp.frame_sz = FRAME_SIZE; // i'm not sure this is correct, probably is pkt_len + some headers
	 	xdp.data = data; // data is the pointer to the data in the page, and its being passed into the sk_buff struct
	 	xdp.data_end = data + len; // data + len is the pointer to the end of the data in the page, and its being passed into the sk_buff struct
	 	xdp.data_hard_start = data; // needed by bpf_xpd_adjust_head, not used
	 	xdp.data_meta = data; // additional packet metadata, none ATM

		skb = napi_alloc_skb(napi, len);
		if (!skb) {
			rv = -ENOMEM;
			break;
		}

		res = onic_run_xdp(q, &xdp);
		if (IS_ERR(res)) {
			unsigned int xdp_res = -PTR_ERR(res);

			if (xdp_res & (ONIC_XDP_TX | ONIC_XDP_REDIR)) {
				// TODO:
				xdp_xmit |= xdp_res;
			}
		}

		skb_put_data(skb, data, len);
		skb->protocol = eth_type_trans(skb, q->netdev);
		skb->ip_summed = CHECKSUM_NONE;
		skb_record_rx_queue(skb, qid);
		rv = napi_gro_receive(napi, skb);
		if (rv < 0) {
			netdev_err(q->netdev, "napi_gro_receive, err = %d", rv);
			break;
		}
		priv->netdev_stats.rx_packets++;
		priv->netdev_stats.rx_bytes += len;

		onic_ring_increment_tail(desc_ring);

		if (debug)
			netdev_info(
				q->netdev,
				"desc_ring %u next_to_use:%u next_to_clean:%u",
				onic_ring_get_real_count(desc_ring),
				desc_ring->next_to_use,
				desc_ring->next_to_clean);
		if (onic_ring_full(desc_ring)) {
			netdev_dbg(q->netdev, "desc_ring full");
		}

		if (onic_rx_high_watermark(q)) {
			netdev_dbg(q->netdev, "High watermark: h = %d, t = %d",
				   desc_ring->next_to_use,
				   desc_ring->next_to_clean);
			onic_rx_refill(q);
		}

		onic_ring_increment_tail(cmpl_ring);

		if (debug)
			netdev_info(
				q->netdev,
				"cmpl_ring %u next_to_use:%u next_to_clean:%u, flipped:%s",
				onic_ring_get_real_count(cmpl_ring),
				cmpl_ring->next_to_use,
				cmpl_ring->next_to_clean,
				flipped ? "true" : "false");
		if (onic_ring_full(cmpl_ring)) {
			netdev_dbg(q->netdev, "cmpl_ring full");
		}
		if (cmpl.color != cmpl_ring->color) {
			if (debug)
				netdev_info(
					q->netdev,
					"part 1. cmpl_ring->next_to_clean=%u color *** old fliping *** color[%u]",
					cmpl_ring->next_to_clean,
					cmpl_ring->color);
			cmpl_ring->color = (cmpl_ring->color == 0) ? 1 : 0;
			flipped = 1;
		}
		cmpl_ptr = cmpl_ring->desc +
			   (QDMA_C2H_CMPL_SIZE * cmpl_ring->next_to_clean);

		if ((++work) >= budget) {
			if (debug)
				netdev_info(q->netdev,
					    "watchdog work %u, budget %u", work,
					    budget);
			napi_complete(napi);
			napi_reschedule(napi);
			goto out_of_budget;
		}

		qdma_unpack_c2h_cmpl(&cmpl, cmpl_ptr);

		if (debug)
			netdev_info(
				q->netdev,
				"c2h_cmpl(b) pkt_id %u, pkt_len %u, error %u, color %u",
				cmpl.pkt_id, cmpl.pkt_len, cmpl.err,
				cmpl.color);
	}

	// TODO:
	// if (xdp_xmit & ONIC_XDP_REDIR)
	// 	xdp_do_flush_map();

	if (xdp_xmit & XDP_TX) {
		// TODO: handle XDP_TX
	}

	if (cmpl_ring->next_to_clean == cmpl_stat.pidx) {
		if (debug)
			netdev_info(
				q->netdev,
				"next_to_clean == cmpl_stat.pidx %u, napi_complete work %u, budget %u, rval %s",
				cmpl_stat.pidx, work, budget,
				napi_cmpl_rval ? "true" : "false");
		napi_cmpl_rval = napi_complete_done(napi, work);
		onic_set_completion_tail(priv->hw.qdma, qid,
					 cmpl_ring->next_to_clean, 1);
		if (debug)
			netdev_info(q->netdev, "onic_set_completion_tail ");
	} else if (cmpl_ring->next_to_clean == 0) {
		if (debug)
			netdev_info(
				q->netdev,
				"next_to_clean == 0, napi_complete work %u, budget %u, rval %s",
				work, budget,
				napi_cmpl_rval ? "true" : "false");
		if (debug)
			netdev_info(q->netdev,
				    "napi_complete work %u, budget %u, rval %s",
				    work, budget,
				    napi_cmpl_rval ? "true" : "false");
		napi_cmpl_rval = napi_complete_done(napi, work);
		onic_set_completion_tail(priv->hw.qdma, qid,
					 cmpl_ring->next_to_clean, 1);
		if (debug)
			netdev_info(q->netdev, "onic_set_completion_tail ");
	}

out_of_budget:
	if (debug)
		netdev_info(q->netdev, "rx_poll is done");
	if (debug)
		netdev_info(
			q->netdev,
			"rx_poll returning work %u, rx_packets %lld, rx_bytes %lld",
			work, priv->netdev_stats.rx_packets,
			priv->netdev_stats.rx_bytes);
	return work;
}

static void onic_clear_tx_queue(struct onic_private *priv, u16 qid)
{
	struct onic_tx_queue *q = priv->tx_queue[qid];
	struct onic_ring *ring;
	u32 size;
	int real_count;

	if (!q)
		return;

	onic_qdma_clear_tx_queue(priv->hw.qdma, qid);

	ring = &q->ring;
	real_count = ring->count - 1;
	size = QDMA_H2C_ST_DESC_SIZE * real_count + QDMA_WB_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);

	if (ring->desc)
		dma_free_coherent(&priv->pdev->dev, size, ring->desc,
				  ring->dma_addr);
	kfree(q->buffer);
	kfree(q);
	priv->tx_queue[qid] = NULL;
}

static int onic_init_tx_queue(struct onic_private *priv, u16 qid)
{
	const u8 rngcnt_idx = 0;
	struct net_device *dev = priv->netdev;
	struct onic_tx_queue *q;
	struct onic_ring *ring;
	struct onic_qdma_h2c_param param;
	u16 vid;
	u32 size, real_count;
	int rv;
	bool debug = 0;

	if (priv->tx_queue[qid]) {
		if (debug)
			netdev_info(dev, "Re-initializing TX queue %d", qid);
		onic_clear_tx_queue(priv, qid);
	}

	q = kzalloc(sizeof(struct onic_tx_queue), GFP_KERNEL);
	if (!q)
		return -ENOMEM;

	/* evenly assign to TX queues available vectors */
	vid = qid % priv->num_q_vectors;

	q->netdev = dev;
	q->vector = priv->q_vector[vid];
	q->qid = qid;

	ring = &q->ring;
	ring->count = onic_ring_count(rngcnt_idx);
	real_count = ring->count - 1;

	/* allocate DMA memory for TX descriptor ring */
	size = QDMA_H2C_ST_DESC_SIZE * real_count + QDMA_WB_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);
	ring->desc = dma_alloc_coherent(&priv->pdev->dev, size, &ring->dma_addr,
					GFP_KERNEL);
	if (!ring->desc) {
		rv = -ENOMEM;
		goto clear_tx_queue;
	}
	memset(ring->desc, 0, size);
	ring->wb = ring->desc + QDMA_H2C_ST_DESC_SIZE * real_count;
	ring->next_to_use = 0;
	ring->next_to_clean = 0;
	ring->color = 0;

	/* initialize TX buffers */
	q->buffer =
		kcalloc(real_count, sizeof(struct onic_tx_buffer), GFP_KERNEL);
	if (!q->buffer) {
		rv = -ENOMEM;
		goto clear_tx_queue;
	}

	/* initialize QDMA H2C queue */
	param.rngcnt_idx = rngcnt_idx;
	param.dma_addr = ring->dma_addr;
	param.vid = vid;
	rv = onic_qdma_init_tx_queue(priv->hw.qdma, qid, &param);
	if (rv < 0)
		goto clear_tx_queue;

	priv->tx_queue[qid] = q;
	return 0;

clear_tx_queue:
	onic_clear_tx_queue(priv, qid);
	return rv;
}

static void onic_clear_rx_queue(struct onic_private *priv, u16 qid)
{
	struct onic_rx_queue *q = priv->rx_queue[qid];
	struct onic_ring *ring;
	u32 size, real_count;
	int i;

	if (!q)
		return;

	onic_qdma_clear_rx_queue(priv->hw.qdma, qid);

	napi_disable(&q->napi);
	netif_napi_del(&q->napi);

	ring = &q->desc_ring;
	real_count = ring->count - 1;
	size = QDMA_C2H_ST_DESC_SIZE * real_count + QDMA_WB_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);

	if (ring->desc)
		dma_free_coherent(&priv->pdev->dev, size, ring->desc,
				  ring->dma_addr);

	ring = &q->cmpl_ring;
	real_count = ring->count - 1;
	size = QDMA_C2H_CMPL_SIZE * real_count + QDMA_C2H_CMPL_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);

	if (ring->desc)
		dma_free_coherent(&priv->pdev->dev, size, ring->desc,
				  ring->dma_addr);

	for (i = 0; i < real_count; ++i) {
		struct page *pg = q->buffer[i].pg;
		__free_pages(pg, 0);
	}

	kfree(q->buffer);
	kfree(q);
	priv->rx_queue[qid] = NULL;
}

static int onic_init_rx_queue(struct onic_private *priv, u16 qid)
{
	const u8 bufsz_idx = 13;
	const u8 desc_rngcnt_idx = 13;
	//const u8 cmpl_rngcnt_idx = 15;
	const u8 cmpl_rngcnt_idx = 13;
	struct net_device *dev = priv->netdev;
	struct onic_rx_queue *q;
	struct onic_ring *ring;
	struct onic_qdma_c2h_param param;
	u16 vid;
	u32 size, real_count;
	int i, rv;
	bool debug = 0;

	if (priv->rx_queue[qid]) {
		if (debug)
			netdev_info(dev, "Re-initializing RX queue %d", qid);
		onic_clear_rx_queue(priv, qid);
	}

	q = kzalloc(sizeof(struct onic_rx_queue), GFP_KERNEL);
	if (!q)
		return -ENOMEM;

	/* evenly assign to RX queues available vectors */
	vid = qid % priv->num_q_vectors;

	q->netdev = dev;
	q->vector = priv->q_vector[vid];
	q->qid = qid;

	if (xdp_rxq_info_is_reg(&q->xdp_rxq))
		xdp_rxq_info_unreg(&q->xdp_rxq);

	rv = xdp_rxq_info_reg(&q->xdp_rxq, dev, qid, q->napi.napi_id);
	if (rv < 0) {
		netdev_err(dev, "Failed to register xdp_rxq index %u\n", q->qid);
		return rv;
	}

	/* allocate DMA memory for RX descriptor ring */
	ring = &q->desc_ring;
	ring->count = onic_ring_count(desc_rngcnt_idx);
	real_count = ring->count - 1;

	size = QDMA_C2H_ST_DESC_SIZE * real_count + QDMA_WB_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);
	ring->desc = dma_alloc_coherent(&priv->pdev->dev, size, &ring->dma_addr,
					GFP_KERNEL);
	if (!ring->desc) {
		rv = -ENOMEM;
		goto clear_rx_queue;
	}
	memset(ring->desc, 0, size);
	ring->wb = ring->desc + QDMA_C2H_ST_DESC_SIZE * real_count;
	ring->next_to_use = 0;
	ring->next_to_clean = 0;
	ring->color = 0;

	/* initialize RX buffers */
	q->buffer =
		kcalloc(real_count, sizeof(struct onic_rx_buffer), GFP_KERNEL);
	if (!q->buffer) {
		rv = -ENOMEM;
		goto clear_rx_queue;
	}

	for (i = 0; i < real_count; ++i) {
		struct page *pg = dev_alloc_pages(0);

		if (!pg) {
			rv = -ENOMEM;
			goto clear_rx_queue;
		}

		q->buffer[i].pg = pg;
		q->buffer[i].offset = 0;
	}

	/* map pages and initialize descriptors */
	for (i = 0; i < real_count; ++i) {
		u8 *desc_ptr = ring->desc + QDMA_C2H_ST_DESC_SIZE * i;
		struct qdma_c2h_st_desc desc;
		struct page *pg = q->buffer[i].pg;
		unsigned int offset = q->buffer[i].offset;

		desc.dst_addr = dma_map_page(&priv->pdev->dev, pg, 0, PAGE_SIZE,
					     DMA_FROM_DEVICE);
		desc.dst_addr += offset;

		qdma_pack_c2h_st_desc(desc_ptr, &desc);
	}

	/* allocate DMA memory for completion ring */
	ring = &q->cmpl_ring;
	ring->count = onic_ring_count(cmpl_rngcnt_idx);
	real_count = ring->count - 1;

	size = QDMA_C2H_CMPL_SIZE * real_count + QDMA_C2H_CMPL_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);
	ring->desc = dma_alloc_coherent(&priv->pdev->dev, size, &ring->dma_addr,
					GFP_KERNEL);
	if (!ring->desc) {
		rv = -ENOMEM;
		goto clear_rx_queue;
	}
	memset(ring->desc, 0, size);
	ring->wb = ring->desc + QDMA_C2H_CMPL_SIZE * real_count;
	ring->next_to_use = 0;
	ring->next_to_clean = 0;
	ring->color = 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
	netif_napi_add(dev, &q->napi, onic_rx_poll);
#else
	netif_napi_add(dev, &q->napi, onic_rx_poll, 64);
#endif
	napi_enable(&q->napi);

	/* initialize QDMA C2H queue */
	param.bufsz_idx = bufsz_idx;
	param.desc_rngcnt_idx = desc_rngcnt_idx;
	param.cmpl_rngcnt_idx = cmpl_rngcnt_idx;
	param.cmpl_desc_sz = 0;
	param.desc_dma_addr = q->desc_ring.dma_addr;
	param.cmpl_dma_addr = q->cmpl_ring.dma_addr;
	param.vid = vid;
	if (debug)
		netdev_info(
			dev,
			"bufsz_idx %u, desc_rngcnt_idx %u, cmpl_rngcnt_idx %u, desc_dma_addr 0x%llx, cmpl_dma_addr 0x%llx, vid %d",
			bufsz_idx, desc_rngcnt_idx, cmpl_rngcnt_idx,
			q->desc_ring.dma_addr, q->cmpl_ring.dma_addr, vid);

	rv = onic_qdma_init_rx_queue(priv->hw.qdma, qid, &param);
	if (rv < 0)
		goto clear_rx_queue;

	/* fill RX descriptor ring with a few descriptors */
	q->desc_ring.next_to_use = ONIC_RX_DESC_STEP;
	onic_set_rx_head(priv->hw.qdma, qid, q->desc_ring.next_to_use);
	onic_set_completion_tail(priv->hw.qdma, qid, 0, 1);

	priv->rx_queue[qid] = q;
	return 0;

clear_rx_queue:
	onic_clear_rx_queue(priv, qid);
	return rv;
}

static int onic_init_tx_resource(struct onic_private *priv)
{
	struct net_device *dev = priv->netdev;
	int qid, rv;

	for (qid = 0; qid < priv->num_tx_queues; ++qid) {
		rv = onic_init_tx_queue(priv, qid);
		if (!rv)
			continue;

		netdev_err(dev, "onic_init_tx_queue %d, err = %d", qid, rv);
		goto clear_tx_resource;
	}

	return 0;

clear_tx_resource:
	while (qid--)
		onic_clear_tx_queue(priv, qid);
	return rv;
}

static int onic_init_rx_resource(struct onic_private *priv)
{
	struct net_device *dev = priv->netdev;
	int qid, rv;

	for (qid = 0; qid < priv->num_rx_queues; ++qid) {
		rv = onic_init_rx_queue(priv, qid);
		if (!rv)
			continue;

		netdev_err(dev, "onic_init_rx_queue %d, err = %d", qid, rv);
		goto clear_rx_resource;
	}

	return 0;

clear_rx_resource:
	while (qid--)
		onic_clear_rx_queue(priv, qid);
	return rv;
}

int onic_open_netdev(struct net_device *dev)
{
	struct onic_private *priv = netdev_priv(dev);
	int rv;

	rv = onic_init_tx_resource(priv);
	if (rv < 0)
		goto stop_netdev;

	rv = onic_init_rx_resource(priv);
	if (rv < 0)
		goto stop_netdev;

	netif_tx_start_all_queues(dev);
	netif_carrier_on(dev);
	return 0;

stop_netdev:
	onic_stop_netdev(dev);
	return rv;
}

int onic_stop_netdev(struct net_device *dev)
{
	struct onic_private *priv = netdev_priv(dev);
	int qid;

	/* stop sending */
	netif_carrier_off(dev);
	netif_tx_stop_all_queues(dev);

	for (qid = 0; qid < priv->num_tx_queues; ++qid)
		onic_clear_tx_queue(priv, qid);
	for (qid = 0; qid < priv->num_rx_queues; ++qid)
		onic_clear_rx_queue(priv, qid);

	return 0;
}

netdev_tx_t onic_xmit_frame(struct sk_buff *skb, struct net_device *dev)
{
	struct onic_private *priv = netdev_priv(dev);
	struct onic_tx_queue *q;
	struct onic_ring *ring;
	struct qdma_h2c_st_desc desc;
	u16 qid = skb->queue_mapping;
	dma_addr_t dma_addr;
	u8 *desc_ptr;
	int rv;
	bool debug = 0;

	q = priv->tx_queue[qid];
	ring = &q->ring;

	onic_tx_clean(q);

	if (onic_ring_full(ring)) {
		if (debug)
			netdev_info(dev, "ring is full");
		return NETDEV_TX_BUSY;
	}

	/* minimum Ethernet packet length is 60 */
	rv = skb_put_padto(skb, ETH_ZLEN);

	if (rv < 0)
		netdev_err(dev, "skb_put_padto failed, err = %d", rv);

	dma_addr = dma_map_single(&priv->pdev->dev, skb->data, skb->len,
				  DMA_TO_DEVICE);

	if (unlikely(dma_mapping_error(&priv->pdev->dev, dma_addr))) {
		dev_kfree_skb(skb);
		priv->netdev_stats.tx_dropped++;
		priv->netdev_stats.tx_errors++;
		return NETDEV_TX_OK;
	}

	desc_ptr = ring->desc + QDMA_H2C_ST_DESC_SIZE * ring->next_to_use;
	desc.len = skb->len;
	desc.src_addr = dma_addr;
	desc.metadata = skb->len;
	qdma_pack_h2c_st_desc(desc_ptr, &desc);

	q->buffer[ring->next_to_use].skb = skb;
	q->buffer[ring->next_to_use].dma_addr = dma_addr;
	q->buffer[ring->next_to_use].len = skb->len;

	priv->netdev_stats.tx_packets++;
	priv->netdev_stats.tx_bytes += skb->len;

	onic_ring_increment_head(ring);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	if (onic_ring_full(ring) || !netdev_xmit_more()) {
#elif defined(RHEL_RELEASE_CODE)
#if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 1)
        if (onic_ring_full(ring) || !netdev_xmit_more()) {
#endif
#else
	if (onic_ring_full(ring) || !skb->xmit_more) {
#endif
		wmb();
		onic_set_tx_head(priv->hw.qdma, qid, ring->next_to_use);
	}

	return NETDEV_TX_OK;
}

int onic_set_mac_address(struct net_device *dev, void *addr)
{
	struct sockaddr *saddr = addr;
	u8 *dev_addr = saddr->sa_data;
	if (!is_valid_ether_addr(saddr->sa_data))
		return -EADDRNOTAVAIL;

	netdev_info(dev, "Set MAC address to %02x:%02x:%02x:%02x:%02x:%02x",
			dev_addr[0], dev_addr[1], dev_addr[2],
			dev_addr[3], dev_addr[4], dev_addr[5]);
	eth_hw_addr_set(dev, dev_addr);
	return 0;
}

int onic_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	return 0;
}

int onic_change_mtu(struct net_device *dev, int mtu)
{
	netdev_info(dev, "Requested MTU = %d", mtu);
	return 0;
}

inline void onic_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	struct onic_private *priv = netdev_priv(dev);

	stats->tx_packets = priv->netdev_stats.tx_packets;
	stats->tx_bytes = priv->netdev_stats.tx_bytes;
	stats->rx_packets = priv->netdev_stats.rx_packets;
	stats->rx_bytes = priv->netdev_stats.rx_bytes;
	stats->tx_dropped = priv->netdev_stats.tx_dropped;
	stats->tx_errors = priv->netdev_stats.tx_errors;
}


static int onic_setup_xdp_prog(struct net_device *dev, struct bpf_prog *prog) {
	// Since the maximum packet size is at most 1514, less than a page, and the rx
	// buffer is one page in size, no need to check for size

	struct onic_private *priv = netdev_priv(dev);
	bool running = netif_running(dev);

	bool need_reset;

	struct bpf_prog *old_prog = xchg(&priv->xdp_prog, prog);
	need_reset = (!!prog != !!old_prog);

	if (need_reset && running) {
		onic_stop_netdev(dev);
	} else {
		int i;
		for (i = 0; i < priv->num_rx_queues; i++) {
			xchg(&priv->rx_queue[i]->xdp_prog, priv->xdp_prog);
		}
	}
	if (old_prog)
		bpf_prog_put(old_prog);

	/* bpf is just replaced, RXQ and MTU are already setup */
	if (!need_reset)
		return 0;

	if (running)
		onic_open_netdev(dev);

	return 0;
}

int onic_xdp(struct net_device *dev, struct netdev_bpf *xdp) {
	switch (xdp->command) {
		case XDP_SETUP_PROG:
			return onic_setup_xdp_prog(dev, xdp->prog);
		default:
			return -EINVAL;
	}
}

// TODO: stub
int onic_xdp_xmit(struct net_device *dev, int n, struct xdp_frame **frames, u32 flags) {
	return 0;
}

