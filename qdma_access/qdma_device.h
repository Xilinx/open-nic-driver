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
#ifndef __QDMA_DEVICE_H__
#define __QDMA_DEVICE_H__

#include <linux/pci.h>

#define QDMA_FLAG_FMAP		 BIT(1)

struct qdma_dev {
	struct pci_dev *pdev;
	u16 func_id;
	u16 q_base;
	u16 num_queues;
	void __iomem *addr;	/* mappaed address of device registers */
};

/**
 * qdma_create_dev - Create a QDMA device
 * @pdev: pointer to PCI device
 * @bar: BAR number for QDMA registers
 *
 * Return a pointer to the created QDMA devcie, or NULL on failure
 **/
struct qdma_dev *qdma_create_dev(struct pci_dev *pdev, u8 bar);

/**
 * qdma_destroy_dev - Destroy a QDMA device
 * @qdev: pointer to QDMA device
 *
 * The input pointer is checked.  So it is safe to pass in NULL pointers.
 **/
void qdma_destroy_dev(struct qdma_dev *qdev);

#endif
