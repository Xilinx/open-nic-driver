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
#include <linux/netdevice.h>
#include <linux/ethtool.h>

#include "onic.h"

extern const char onic_drv_name[];
extern const char onic_drv_ver[];

static void onic_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *drvinfo)
{
	struct onic_private *priv = netdev_priv(netdev);

	strlcpy(drvinfo->driver, onic_drv_name, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, onic_drv_ver,
		sizeof(drvinfo->version));
	strlcpy(drvinfo->bus_info, pci_name(priv->pdev),
		sizeof(drvinfo->bus_info));
}

static const struct ethtool_ops onic_ethtool_ops = {
	.get_drvinfo = onic_get_drvinfo,
	.get_link = ethtool_op_get_link,
};

void onic_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &onic_ethtool_ops;
}
