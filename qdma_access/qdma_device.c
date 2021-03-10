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
#include "qdma_device.h"

struct qdma_dev *qdma_create_dev(struct pci_dev *pdev, u8 bar)
{
	struct qdma_dev *qdev;

	if (bar > 6) {
		dev_err(&pdev->dev, "Bad BAR number %d", bar);
		return NULL;
	}

	qdev = kzalloc(sizeof(struct qdma_dev), GFP_KERNEL);
	if (!qdev)
		return NULL;

	qdev->pdev = pdev;
	qdev->func_id = PCI_FUNC(pdev->devfn);

	qdev->addr = pci_iomap(pdev, bar, pci_resource_len(pdev, bar));
	if (!qdev->addr) {
		kfree(qdev);
		return NULL;
	}

	return qdev;
}

void qdma_destroy_dev(struct qdma_dev *qdev)
{
	if (!qdev)
		return;

	pci_iounmap(qdev->pdev, qdev->addr);
	kfree(qdev);
}
