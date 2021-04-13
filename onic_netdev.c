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

#include "onic_netdev.h"
#include "onic.h"

#define ONIC_RX_DESC_STEP 256

static u16 onic_ring_get_real_count(struct onic_ring *ring)
{
	/* Valid writeback entry means one less count of descriptor entries */
	return (ring->wb) ? (ring->count - 1) : ring->count;
}

static bool onic_ring_full(struct onic_ring *ring)
{
	u16 real_count = onic_ring_get_real_count(ring);

	return ((ring->next_to_use + 1) % real_count) == ring->next_to_clean;
}

static void onic_ring_increment_head(struct onic_ring *ring)
{
	u16 real_count = onic_ring_get_real_count(ring);

	ring->next_to_use = (ring->next_to_use + 1) % real_count;
}

static void onic_ring_increment_tail(struct onic_ring *ring)
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

		dma_unmap_single(&priv->pdev->dev, buf->dma_addr,
				 buf->len, DMA_TO_DEVICE);
		dev_kfree_skb(skb);

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

static int onic_rx_poll(struct napi_struct *napi, int budget)
{
	struct onic_rx_queue *q =
		container_of(napi, struct onic_rx_queue, napi);
	struct onic_private *priv = netdev_priv(q->netdev);
	u16 qid = q->qid;
	struct onic_ring *desc_ring = &q->desc_ring;
	struct onic_ring *cmpl_ring = &q->cmpl_ring;
	struct qdma_c2h_cmpl cmpl;
	u8 *cmpl_ptr;
	int work = 0;
	int i, rv;

	for (i = 0; i < priv->num_tx_queues; i++)
		onic_tx_clean(priv->tx_queue[i]);

	cmpl_ptr = cmpl_ring->desc +
		QDMA_C2H_CMPL_SIZE * cmpl_ring->next_to_clean;
	qdma_unpack_c2h_cmpl(&cmpl, cmpl_ptr);

	/* Color of completion entries and completion ring are initialized to 0
	 * and 1 respectively.  When an entry is filled, it has a color bit of
	 * 1, thus making it the same as the completion ring color.  A different
	 * color indicates that we are done with the current batch.  When the
	 * ring index wraps around, the color flips in both software and
	 * hardware.  Therefore, it becomes that completion entries are filled
	 * with a color 0, and completion ring has a color 0 as well.
	 */
	while (cmpl.color == cmpl_ring->color) {
		struct onic_rx_buffer *buf =
			&q->buffer[desc_ring->next_to_clean];
		struct sk_buff *skb;
		int len = cmpl.pkt_len;
		u8 *data;

		skb = napi_alloc_skb(napi, len);
		if (!skb) {
			rv = -ENOMEM;
			break;
		}

		/* maximum packet size is 1514, less than the page size */
		data = (u8 *)(page_address(buf->pg) + buf->offset);
		skb_put_data(skb, data, len);
		skb->protocol = eth_type_trans(skb, q->netdev);
		skb->ip_summed = CHECKSUM_NONE;
		skb_record_rx_queue(skb, qid);

		rv = napi_gro_receive(napi, skb);
		if (rv < 0) {
			netdev_err(q->netdev, "napi_gro_receive, err = %d", rv);
			break;
		}

		onic_ring_increment_tail(desc_ring);

		if (onic_rx_high_watermark(q)) {
			netdev_dbg(q->netdev, "High watermark: h = %d, t = %d",
				   desc_ring->next_to_use,
				   desc_ring->next_to_clean);
			onic_rx_refill(q);
		}

		priv->netdev_stats.rx_packets++;
		priv->netdev_stats.rx_bytes += len;

		onic_ring_increment_tail(cmpl_ring);

		if (cmpl_ring->next_to_clean == 0) {
			cmpl_ring->color = (cmpl_ring->color == 0) ? 1 : 0;
			cmpl_ptr = cmpl_ring->desc;
		} else {
			cmpl_ptr += QDMA_C2H_CMPL_SIZE;
		}

		if ((++work) >= budget)
			goto out_of_budget;

		qdma_unpack_c2h_cmpl(&cmpl, cmpl_ptr);
	}

	napi_complete_done(napi, work);
	onic_set_completion_tail(priv->hw.qdma, qid, cmpl_ring->next_to_clean, 1);

out_of_budget:
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

	if (priv->tx_queue[qid]) {
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
	ring->desc = dma_alloc_coherent(&priv->pdev->dev, size,
					&ring->dma_addr, GFP_KERNEL);
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
	q->buffer = kcalloc(real_count, sizeof(struct onic_tx_buffer),
			    GFP_KERNEL);
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
	u32 size;
	int real_count;

	if (!q)
		return;

	onic_qdma_clear_rx_queue(priv->hw.qdma, qid);

	/* No need to call netif_napi_del explicitly as failures will propogate,
	 * forcing net device to be unregistered.  After that, free_netdev will
	 * call netif_napi_del for all napi_structs still associated with the
	 * net device.
	 */
	napi_disable(&q->napi);

	ring = &q->desc_ring;
	real_count = ring->count - 1;
	size = QDMA_C2H_ST_DESC_SIZE * real_count + QDMA_WB_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);

	if (ring->desc)
		dma_free_coherent(&priv->pdev->dev, size,
				  ring->desc, ring->dma_addr);

	ring = &q->cmpl_ring;
	real_count = ring->count - 1;
	size = QDMA_C2H_CMPL_SIZE * real_count + QDMA_C2H_CMPL_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);

	if (ring->desc)
		dma_free_coherent(&priv->pdev->dev, size,
				  ring->desc, ring->dma_addr);

	kfree(q->buffer);
	kfree(q);
	priv->rx_queue[qid] = NULL;
}

static int onic_init_rx_queue(struct onic_private *priv, u16 qid)
{
	const u8 bufsz_idx = 0;
	const u8 desc_rngcnt_idx = 0;
	const u8 cmpl_rngcnt_idx = 0;
	struct net_device *dev = priv->netdev;
	struct onic_rx_queue *q;
	struct onic_ring *ring;
	struct onic_qdma_c2h_param param;
	u16 vid;
	u32 size, real_count;
	int i, rv;

	if (priv->rx_queue[qid]) {
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

	/* allocate DMA memory for RX descriptor ring */
	ring = &q->desc_ring;
	ring->count = onic_ring_count(desc_rngcnt_idx);
	real_count = ring->count - 1;

	size = QDMA_C2H_ST_DESC_SIZE * real_count + QDMA_WB_STAT_SIZE;
	size = ALIGN(size, PAGE_SIZE);
	ring->desc = dma_alloc_coherent(&priv->pdev->dev, size,
					&ring->dma_addr, GFP_KERNEL);
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
	q->buffer = kcalloc(real_count, sizeof(struct onic_rx_buffer),
			    GFP_KERNEL);
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

		desc.dst_addr = dma_map_page(&priv->pdev->dev, pg, 0,
					     PAGE_SIZE,
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
	ring->desc = dma_alloc_coherent(&priv->pdev->dev, size,
					&ring->dma_addr, GFP_KERNEL);
	if (!ring->desc) {
		rv = -ENOMEM;
		goto clear_rx_queue;
	}
	memset(ring->desc, 0, size);
	ring->wb = ring->desc + QDMA_C2H_CMPL_SIZE * real_count;
	ring->next_to_use = 0;
	ring->next_to_clean = 0;
	ring->color = 1;

	netif_napi_add(dev, &q->napi, onic_rx_poll, 64);
	napi_enable(&q->napi);

	/* initialize QDMA C2H queue */
	param.bufsz_idx = bufsz_idx;
	param.desc_rngcnt_idx = desc_rngcnt_idx;
	param.cmpl_rngcnt_idx = cmpl_rngcnt_idx;
	param.cmpl_desc_sz = 0;
	param.desc_dma_addr = q->desc_ring.dma_addr;
	param.cmpl_dma_addr = q->cmpl_ring.dma_addr;
	param.vid = vid;

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
	while (--qid)
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
	while (--qid)
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

	q = priv->tx_queue[qid];
	ring = &q->ring;

	onic_tx_clean(q);

	if (onic_ring_full(ring)) {
		netdev_info(dev, "ring is full");
		return NETDEV_TX_BUSY;
	}

	/* minimum Ethernet packet length is 60 */
	skb_put_padto(skb, ETH_ZLEN);

	dma_addr = dma_map_single(&priv->pdev->dev,
				  skb->data,
				  skb->data_len,
				  DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(&priv->pdev->dev, dma_addr))) {
		netdev_err(dev, "dma map error, skb = %p, dma_addr = %llx",
			   skb, dma_addr);
		return NETDEV_TX_BUSY;
	}

	desc_ptr = ring->desc + QDMA_H2C_ST_DESC_SIZE * ring->next_to_use;
	desc.len = skb->len;
	desc.src_addr = dma_addr;
	desc.metadata = skb->len;
	qdma_pack_h2c_st_desc(desc_ptr, &desc);

	q->buffer[ring->next_to_use].skb = skb;
	q->buffer[ring->next_to_use].dma_addr = dma_addr;
	q->buffer[ring->next_to_use].len = skb->data_len;

	priv->netdev_stats.tx_packets++;
	priv->netdev_stats.tx_bytes += skb->len;

	onic_ring_increment_head(ring);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	if (onic_ring_full(ring) || !netdev_xmit_more()) {
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

	netdev_info(dev, "Set MAC address to %x:%x:%x:%x:%x:%x",
		    dev_addr[0], dev_addr[1], dev_addr[2],
		    dev_addr[3], dev_addr[4], dev_addr[5]);
	memcpy(dev->dev_addr, dev_addr, dev->addr_len);
	return 0;
}

int onic_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	return 0;
}

int onic_change_mtu(struct net_device *dev, int mtu)
{
	netdev_info(dev, "Reqeusted MTU = %d", mtu);
	return 0;
}

void onic_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	struct onic_private *priv = netdev_priv(dev);

	stats->tx_packets = priv->netdev_stats.tx_packets;
	stats->tx_bytes = priv->netdev_stats.tx_bytes;
	stats->rx_packets = priv->netdev_stats.rx_packets;
	stats->rx_bytes = priv->netdev_stats.rx_bytes;
}
