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
#ifndef __ONIC_NETDEV_H__
#define __ONIC_NETDEV_H__

#include <linux/netdevice.h>

/**
 * onic_open_netdev - initialize TX/RX queues and open network device
 * @dev: pointer to registered net device
 *
 * Implementation of `ndo_open` in `net_device_ops`.  Return 0 on success,
 * negative on failure.
 **/
int onic_open_netdev(struct net_device *dev);

/**
 * onic_stop_netdev - clear TX/RX queues and stop network device
 * @dev: pointer to registered net device
 *
 * Implementation of `ndo_stop` in `net_device_ops`.  Return 0 on success,
 * negative on failure.
 **/
int onic_stop_netdev(struct net_device *dev);

netdev_tx_t onic_xmit_frame(struct sk_buff *skb, struct net_device *dev);

int onic_set_mac_address(struct net_device *dev, void *addr);

int onic_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

int onic_change_mtu(struct net_device *dev, int mtu);

void onic_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats);

int onic_poll(struct napi_struct *napi, int budget);

int onic_xdp(struct net_device *dev, struct netdev_bpf *xdp);

int onic_xdp_xmit(struct net_device *dev, int n, struct xdp_frame **frames,
          u32 flags);
#endif
