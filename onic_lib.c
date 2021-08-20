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

#include "onic_lib.h"
#include "onic.h"

#define ONIC_MAX_IRQ_NAME 32
static bool debug = 0;

extern int onic_poll(struct napi_struct *napi, int budget);

static const enum qdma_intr_rngsz intr_rngsz = QDMA_INTR_RNGSZ_4KB;

static irqreturn_t onic_q_handler(int irq, void *dev_id)
{
	struct onic_q_vector *vec = dev_id;
	struct onic_private *priv = vec->priv;
	u16 qid = vec->vid;
	struct onic_rx_queue *rxq = priv->rx_queue[qid];
	if (debug) dev_info(&priv->pdev->dev, "starting onic_q_handler \n");

	if (debug) dev_info(&priv->pdev->dev, "queue irq");

	//napi_schedule(&rxq->napi);
	napi_schedule_irqoff(&rxq->napi);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_q_handler\n");
	return IRQ_HANDLED;
}

static irqreturn_t onic_user_handler(int irq, void *dev_id)
{
    struct onic_private *priv = dev_id;
    //struct onic_q_vector *vec = dev_id;
	//struct onic_private *priv = vec->priv;
	if (debug) dev_info(&priv->pdev->dev, "starting onic_user_handler\n");
	dev_info(&priv->pdev->dev, "user irq");
	if (debug) dev_info(&priv->pdev->dev, "completing onic_user_handler\n");
	//return IRQ_WAKE_THREAD;
	return IRQ_HANDLED;
}

static irqreturn_t onic_user_thread_fn(int irq, void *dev_id)
{
    struct onic_private *priv = dev_id;
	//struct onic_q_vector *vec = dev_id;
	//struct onic_private *priv = vec->priv;
	if (debug) dev_info(&priv->pdev->dev, "starting onic_user_thread_fn\n");

	dev_info(&priv->pdev->dev,
		"User IRQ (BH) fired on Funtion#%05x: vector=%d\n",
		PCI_FUNC(priv->pdev->devfn), irq);

	if (debug) dev_info(&priv->pdev->dev, "completing onic_user_thread_fn\n");
	return IRQ_HANDLED;
}

static irqreturn_t onic_error_handler(int irq, void *dev_id)
{
    struct onic_private *priv = dev_id;
    //struct onic_q_vector *vec = dev_id;
    //struct onic_private *priv = vec->priv;
    if (debug) dev_info(&priv->pdev->dev, "starting onic_error_handler\n");
    if (debug) dev_info(&priv->pdev->dev, "completing onic_error_handler\n");
    return IRQ_HANDLED;
	//return IRQ_WAKE_THREAD;
}

static irqreturn_t onic_error_thread_fn(int irq, void *dev_id)
{
    struct onic_private *priv = dev_id;
	//struct onic_q_vector *vec = dev_id;
	//struct onic_private *priv = vec->priv;
	if (debug) dev_info(&priv->pdev->dev, "starting onic_error_thread_fn\n");

	dev_err(&priv->pdev->dev,
		"Error IRQ (BH) fired on Funtion#%05x: vector=%d\n",
		PCI_FUNC(priv->pdev->devfn), irq);

	if (debug) dev_info(&priv->pdev->dev, "completing onic_error_thread_fn\n");
	return IRQ_HANDLED;
}


/**
 * onic_init_q_vector - clear a queue vector
 * @priv: pointer to driver private data
 * @vid: vector ID
 **/
static void onic_clear_q_vector(struct onic_private *priv, u16 vid)
{
	struct onic_q_vector *vec = priv->q_vector[vid];
	if (debug) dev_info(&priv->pdev->dev, "starting onic_clear_q_vector\n");

	if (!vec)
		return;
	free_irq(pci_irq_vector(priv->pdev, vid), vec);
	kfree(vec);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_clear_q_vector\n");
}

/**
 * onic_init_q_vector - initialize a queue vector
 * @priv: pointer to driver private data
 * @vid: vector ID
 *
 * This function does the following: allocate coherent DMA region for interrupt
 * aggregation ring, register NAPI instances, and initialize relevant QDMA
 * interrupt registers.  Return 0 on success, negative on failure
 **/
static int onic_init_q_vector(struct onic_private *priv, u16 vid)
{
	struct pci_dev *pdev = priv->pdev;
	struct onic_q_vector *vec;
	// JMY
	//char name[ONIC_MAX_IRQ_NAME];
	char* name = (char*)vmalloc(sizeof(char)*ONIC_MAX_IRQ_NAME);
	int rv;

	if (debug) dev_info(&priv->pdev->dev, "starting onic_init_q_vector\n");

	vec = kzalloc(sizeof(struct onic_q_vector), GFP_KERNEL);
	if (!vec)
		return -ENOMEM;
	vec->priv = priv;
	vec->vid = vid;

	snprintf(name, ONIC_MAX_IRQ_NAME, "%s-%d", priv->netdev->name, vid);
	rv = request_irq(pci_irq_vector(pdev, vid), onic_q_handler,
			 0, name, vec);
	if (rv < 0) {
		dev_err(&pdev->dev, "Failed to setup queue vector %s", name);
		return rv;
	}

	/* setup affinity mask and node */
	/* cpu = vid % num_online_cpus(); */
	/* cpumask_set_cpu(cpu, &vec->affinity_mask); */
	/* vec->numa_node = node; */

	dev_info(&pdev->dev, "Setup IRQ vector %d with name %s",
		 pci_irq_vector(pdev, vid), name);
	priv->q_vector[vid] = vec;
	if (debug) dev_info(&priv->pdev->dev, "completing onic_init_q_vector\n");

	return 0;
}

/**
 * onic_acquire_msix_vectors - acquire MSI-X vectors
 * @priv: pointer to driver private data
 *
 * Attempt to acquire a suitable range of MSI-X vector interrupts.  Return 0 on
 * success, and negative on error.
 *
 * For every PF, a minimum of 2 vectors are required for proper operation, one
 * for queue interrupt and one for user interreupt.  The master PF requires one
 * additional vector for global error interrupt.
 **/
static int onic_acquire_msix_vectors(struct onic_private *priv)
{
	int vectors, non_q_vectors;

	if (debug) dev_info(&priv->pdev->dev, "starting onic_acquire_msix_vectors\n");

	vectors = ONIC_MAX_QUEUES;
	non_q_vectors = 1;
	if (test_bit(ONIC_FLAG_MASTER_PF, priv->flags))
		non_q_vectors++;
	vectors += non_q_vectors;

	vectors = pci_alloc_irq_vectors(priv->pdev, non_q_vectors + 1, vectors,
					PCI_IRQ_AFFINITY | PCI_IRQ_MSIX);
	if (vectors < 0) {
		dev_err(&priv->pdev->dev,
			"Failed to allocate vectors in the range [%d, %d]",
			non_q_vectors + 1, vectors);
		return vectors;
	}

	vectors -= non_q_vectors;
	priv->num_q_vectors = vectors;

	dev_info(&priv->pdev->dev, "Allocated %d queue vectors\n",
		 priv->num_q_vectors);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_acquire_msix_vectors\n");
	return 0;
}

/**
 * onic_set_num_queues - calculate the number of active queues
 * @priv: pointer to driver private data
 *
 * The number of active queues equals to either the number of queue vectors, or
 * the real number of queues in the associated net device, whichever is smaller.
 **/
static void onic_set_num_queues(struct onic_private *priv)
{
	struct net_device *dev = priv->netdev;

	if (debug) dev_info(&priv->pdev->dev, "starting onic_set_num_queues\n");

	priv->num_tx_queues =
		min_t(u16, priv->num_q_vectors, dev->real_num_tx_queues);
	priv->num_rx_queues =
		min_t(u16, priv->num_q_vectors, dev->real_num_rx_queues);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_set_num_queues\n");
}

int onic_init_capacity(struct onic_private *priv)
{
	int rv;

	if (debug) dev_info(&priv->pdev->dev, "starting onic_init_capacity\n");

	rv = onic_acquire_msix_vectors(priv);
	if (rv < 0)
		return rv;
	onic_set_num_queues(priv);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_init_capacity\n");
	return 0;
}

void onic_clear_capacity(struct onic_private *priv)
{
    if (debug) dev_info(&priv->pdev->dev, "starting onic_clear_capacity\n");
	priv->num_tx_queues = 0;
	priv->num_rx_queues = 0;
	priv->num_q_vectors = 0;
	pci_free_irq_vectors(priv->pdev);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_clear_capacity\n");
}

int onic_init_interrupt(struct onic_private *priv)
{
	struct pci_dev *pdev = priv->pdev;
	int vid, rv;

	if (debug) dev_info(&priv->pdev->dev, "starting onic_init_interrupt\n");

	for (vid = 0; vid < priv->num_q_vectors; ++vid) {
		rv = onic_init_q_vector(priv, vid);
		if (rv < 0)
			goto clear_interrupt;
	}

	rv = request_threaded_irq(pci_irq_vector(pdev, vid),
				  onic_user_handler, onic_user_thread_fn,
				  0, "onic-user", priv);
	if (rv < 0) {
		dev_err(&pdev->dev, "Failed to setup user interrupt\n");
		goto clear_interrupt;
	}
	set_bit(ONIC_USER_INTR, priv->state);

	if (!test_bit(ONIC_FLAG_MASTER_PF, priv->flags))
		return 0;


	vid++;
	rv = request_threaded_irq(pci_irq_vector(pdev, vid),
				  onic_error_handler, onic_error_thread_fn,
				  0, "onic-error", priv);
	if (rv < 0) {
		dev_err(&pdev->dev, "Failed to setup error interrupt\n");
		goto clear_interrupt;
	}
	onic_qdma_init_error_interrupt(priv->hw.qdma, vid);
	set_bit(ONIC_ERROR_INTR, priv->state);
	
	if (debug) dev_info(&priv->pdev->dev, "completing onic_init_interrupt\n");

	return 0;

clear_interrupt:
    if (debug) dev_info(&priv->pdev->dev, "within onic_init_interrupt: clearing interrupt\n");
	onic_clear_interrupt(priv);
	return rv;
}

void onic_clear_interrupt(struct onic_private *priv)
{
	u8 master_pf = test_bit(ONIC_FLAG_MASTER_PF, priv->flags);
	int vid = (master_pf) ?
		priv->num_q_vectors + 1 :
		priv->num_q_vectors;
	if (debug) dev_info(&priv->pdev->dev, "starting onic_clear_interrupt\n");


	if (master_pf) {
		if (test_bit(ONIC_ERROR_INTR, priv->state)) {
			free_irq(pci_irq_vector(priv->pdev, vid), priv);
			onic_qdma_clear_error_interrupt(priv->hw.qdma);
		}
		vid--;
	}

	if (test_bit(ONIC_USER_INTR, priv->state))
		free_irq(pci_irq_vector(priv->pdev, vid), priv);

	while (vid--)
		onic_clear_q_vector(priv, vid);
	if (debug) dev_info(&priv->pdev->dev, "completing onic_clear_interrupt\n");
}
