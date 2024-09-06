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
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/moduleparam.h>
#include <linux/bpf.h>

#include "onic.h"
#include "onic_hardware.h"
#include "onic_lib.h"
#include "onic_common.h"
#include "onic_netdev.h"

#undef CMS_SUPPORT    /* Need CMS IP in the design @320000 offset */

#ifndef ONIC_VF
#define DRV_STR "OpenNIC Linux Kernel Driver"
char onic_drv_name[] = "onic";
#else
#define DRV_STR "OpenNIC Linux Kernel Driver (VF)"
char onic_drv_name[] = "open-nic-vf";
#endif

#define DRV_VER "0.21"
const char onic_drv_str[] = DRV_STR;
const char onic_drv_ver[] = DRV_VER;

MODULE_AUTHOR("Xilinx Research Labs");
MODULE_DESCRIPTION(DRV_STR);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VER);

static int RS_FEC_ENABLED=1;
module_param(RS_FEC_ENABLED, int, 0644);

#ifdef CMS_SUPPORT
extern int xocl_init_xmc(void);
extern void xocl_fini_xmc(void);
extern struct onic_private *onic_priv;
#endif

static const struct pci_device_id onic_pci_tbl[] = {
	/* Gen 3 PF */
	/* PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0x9031), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9131), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9231), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9331), },	/* PF 3 */
	/* PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0x9032), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9132), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9232), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9332), },	/* PF 3 */
	/* PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0x9034), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9134), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9234), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9334), },	/* PF 3 */
	/* PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0x9038), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9138), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9238), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9338), },	/* PF 3 */
	/* PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0x903f), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x913f), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x923f), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x933f), },	/* PF 3 */
	/* { PCI_DEVICE(0x10ee, 0x6a9f), }, */	     /* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x6aa0), },	/* PF 1 */

	/* Gen 4 PF */
	/* PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0x9041), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9141), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9241), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9341), },	/* PF 3 */
	/* PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0x9042), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9142), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9242), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9342), },	/* PF 3 */
	/* PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0x9044), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9144), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9244), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9344), },	/* PF 3 */
	/* PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0x9048), },	/* PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9148), },	/* PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9248), },	/* PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9348), },	/* PF 3 */

	{0,}
};

MODULE_DEVICE_TABLE(pci, onic_pci_tbl);

/**
 * Default MAC address 00:0A:35:00:00:00
 * First three octets indicate OUI (00:0A:35 for Xilinx)
 * Note that LSB of the first octet must be 0 (unicast)
 **/
static const unsigned char onic_default_dev_addr[] = {
	0x00, 0x0A, 0x35, 0x00, 0x00, 0x00
};

static const struct net_device_ops onic_netdev_ops = {
	.ndo_open = onic_open_netdev,
	.ndo_stop = onic_stop_netdev,
	.ndo_start_xmit = onic_xmit_frame,
	.ndo_set_mac_address = onic_set_mac_address,
	.ndo_do_ioctl = onic_do_ioctl,
	.ndo_change_mtu = onic_change_mtu,
	.ndo_get_stats64 = onic_get_stats64,
	.ndo_bpf = onic_xdp,
// For why we do this, see onic_netdev.c:onix_xdp_run
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	.ndo_xdp_xmit = onic_xdp_xmit,
#elif defined(RHEL_RELEASE_CODE)
#if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 1))
	.ndo_xdp_xmit = onic_xdp_xmit,
#endif
#endif
};

extern void onic_set_ethtool_ops(struct net_device *netdev);

/**
 * onic_probe - Probe and initialize PCI device
 * @pdev: pointer to PCI device
 * @ent: pointer to PCI device ID entries
 *
 * Return 0 on success, negative on failure
 **/
static int onic_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct net_device *netdev;
	struct onic_private *priv;
	struct sockaddr saddr;
	char dev_name[IFNAMSIZ];
	int rv;
#ifdef CMS_SUPPORT
        static int xmc_init=0;
#endif
	/* int pci_using_dac; */

	rv = pci_enable_device_mem(pdev);
	if (rv < 0) {
		dev_err(&pdev->dev, "pci_enable_device_mem, err = %d", rv);
		return rv;
	}

	/* QDMA only supports 32-bit consistent DMA for descriptor ring */
	rv = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
	if (rv < 0) {
		dev_err(&pdev->dev, "Failed to set DMA masks");
		goto disable_device;
	} else {
		dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	}

	rv = pci_request_mem_regions(pdev, onic_drv_name);
	if (rv < 0) {
		dev_err(&pdev->dev, "pci_request_mem_regions, err = %d", rv);
		goto disable_device;
	}

	/* enable relaxed ordering */
	pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_RELAX_EN);
	/* enable extended tag */
	pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_EXT_TAG);
	pci_set_master(pdev);
	pci_save_state(pdev);
	pcie_set_readrq(pdev, 512);

	netdev = alloc_etherdev_mq(sizeof(struct onic_private),
				   ONIC_MAX_QUEUES);
	if (!netdev) {
		dev_err(&pdev->dev, "alloc_etherdev_mq failed");
		rv = -ENOMEM;
		goto release_pci_mem;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);
	netdev->netdev_ops = &onic_netdev_ops;
	onic_set_ethtool_ops(netdev);

	snprintf(dev_name, IFNAMSIZ, "onic%ds%df%d",
		 pdev->bus->number,
		 PCI_SLOT(pdev->devfn),
		 PCI_FUNC(pdev->devfn));
	strlcpy(netdev->name, dev_name, sizeof(netdev->name));

	memset(&saddr, 0, sizeof(struct sockaddr));
	memcpy(saddr.sa_data, onic_default_dev_addr, 6);
	get_random_bytes(saddr.sa_data + 3, 3);
	onic_set_mac_address(netdev, (void *)&saddr);

	priv = netdev_priv(netdev);

	memset(priv, 0, sizeof(struct onic_private));
	priv->RS_FEC = RS_FEC_ENABLED;

	if (PCI_FUNC(pdev->devfn) == 0) {
		dev_info(&pdev->dev, "device is a master PF");
		set_bit(ONIC_FLAG_MASTER_PF, priv->flags);
	}
	priv->pdev = pdev;
	priv->netdev = netdev;
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->rx_lock);

	priv->netdev_stats = alloc_percpu(struct rtnl_link_stats64);
	if (!priv->netdev_stats) {
		dev_err(&pdev->dev, "error in allocating netdev_stats");
		goto free_netdev;
	}

	rv = onic_init_capacity(priv);
	if (rv < 0) {
		dev_err(&pdev->dev, "onic_init_capacity, err = %d", rv);
		goto free_netdev;
	}

	rv = onic_init_hardware(priv);
	if (rv < 0) {
		dev_err(&pdev->dev, "onic_init_hardware, err = %d", rv);
		goto clear_capacity;
	}

	rv = onic_init_interrupt(priv);
	if (rv < 0) {
		dev_err(&pdev->dev, "onic_init_interrupt, err = %d", rv);
		goto clear_hardware;
	}

	netif_set_real_num_tx_queues(netdev, priv->num_tx_queues);
	netif_set_real_num_rx_queues(netdev, priv->num_rx_queues);

	rv = register_netdev(netdev);
	if (rv < 0) {
		dev_err(&pdev->dev, "register_netdev, err = %d", rv);
		goto clear_interrupt;
	}

	pci_set_drvdata(pdev, priv);
	netif_carrier_off(netdev);

#ifdef CMS_SUPPORT
        /* Support CMS sensors (lm-sensors), refer: pg348 */
        if(xmc_init == 0)
        {
            onic_priv = priv;
            xocl_init_xmc();
            xmc_init=1;
        }
#endif

	return 0;

clear_interrupt:
	onic_clear_interrupt(priv);
clear_hardware:
	onic_clear_hardware(priv);
clear_capacity:
	onic_clear_capacity(priv);
free_netdev:
	free_netdev(priv->netdev);
release_pci_mem:
	pci_release_mem_regions(pdev);
disable_device:
	pci_disable_device(pdev);

	return rv;
}

/**
 * onic_remove - remove PCI device
 * @pdev: pointer to PCI device
 **/
static void onic_remove(struct pci_dev *pdev)
{
	struct onic_private *priv = pci_get_drvdata(pdev);
#ifdef CMS_SUPPORT
        static int xmc_remove=0;
#endif

	unregister_netdev(priv->netdev);

	onic_clear_interrupt(priv);
	onic_clear_hardware(priv);
	onic_clear_capacity(priv);

	free_netdev(priv->netdev);

	pci_set_drvdata(pdev, NULL);
	pci_release_mem_regions(pdev);
	pci_disable_device(pdev);

#ifdef CMS_SUPPORT
        /* Support XMC sensors (lm-sensors) */
        if(xmc_remove == 0)
        {
            xocl_fini_xmc();
            xmc_remove=1;
        }
#endif
}

/* static const struct pci_error_handlers qdma_err_handler = { */
/*     .error_detected		    = qdma_error_detected, */
/*     .slot_reset		    = qdma_slot_reset, */
/*     .resume			    = qdma_error_resume, */
/* #if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE */
/*     .reset_prepare		    = qdma_reset_prepare, */
/*     .reset_done		    = qdma_reset_done, */
/* #elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE */
/*     .reset_notify		    = qdma_reset_notify, */
/* #endif */
/* }; */

static struct pci_driver pci_driver = {
	.name = onic_drv_name,
	.id_table = onic_pci_tbl,
	.probe = onic_probe,
	.remove = onic_remove,
};

static int __init onic_init_module(void)
{
	pr_info("%s %s", onic_drv_str, onic_drv_ver);
	return pci_register_driver(&pci_driver);
}

static void __exit onic_exit_module(void)
{
	pci_unregister_driver(&pci_driver);
}

module_init(onic_init_module);
module_exit(onic_exit_module);
