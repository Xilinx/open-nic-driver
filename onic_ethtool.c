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
#include "onic_register.h"

extern const char onic_drv_name[];
extern const char onic_drv_ver[];
// netdev stats are stats kept by the driver, like xdp stats, onic_stats are kept in the NIC and accessed via the on-board registers 
enum { NETDEV_STATS, ONIC_STATS };

enum {
	ETHTOOL_XDP_REDIRECT,
	ETHTOOL_XDP_PASS,
	ETHTOOL_XDP_DROP,
	ETHTOOL_XDP_TX,
	ETHTOOL_XDP_TX_ERR,
	ETHTOOL_XDP_XMIT,
	ETHTOOL_XDP_XMIT_ERR,
};


struct onic_stats {
    char stat_string[ETH_GSTRING_LEN];
    int type;
    int sizeof_stat;
    int stat0_offset;
    int stat1_offset;
};

#define _STAT_ONIC(_name, _stat0, _stat1) { \
	.stat_string = _name, \
	.type = ONIC_STATS, \
	.sizeof_stat = sizeof(u32), \
	.stat0_offset = _stat0, \
	.stat1_offset = _stat1, \
}


#define _STAT_NETDEV(_name,_stat) {\
	.stat_string = _name, \
	.type = NETDEV_STATS, \
	.sizeof_stat = sizeof(u64), \
	.stat0_offset = _stat, \
	.stat1_offset = _stat, \
}

static const struct onic_stats onic_gstrings_stats[] = {
    _STAT_ONIC("stat_tx_total_pkts",
          CMAC_OFFSET_STAT_TX_TOTAL_PKTS(0),
          CMAC_OFFSET_STAT_TX_TOTAL_PKTS(1)),
    _STAT_ONIC("stat_tx_total_good_pkts",
          CMAC_OFFSET_STAT_TX_TOTAL_GOOD_PKTS(0),
          CMAC_OFFSET_STAT_TX_TOTAL_GOOD_PKTS(1)),
    _STAT_ONIC("stat_tx_total_bytes",
          CMAC_OFFSET_STAT_TX_TOTAL_BYTES(0),
          CMAC_OFFSET_STAT_TX_TOTAL_BYTES(1)),
    _STAT_ONIC("stat_tx_total_good_bytes",
          CMAC_OFFSET_STAT_TX_TOTAL_GOOD_BYTES(0),
          CMAC_OFFSET_STAT_TX_TOTAL_GOOD_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_64_bytes",
          CMAC_OFFSET_STAT_TX_PKT_64_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_64_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_65_127_bytes",
          CMAC_OFFSET_STAT_TX_PKT_65_127_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_65_127_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_128_255_bytes",
          CMAC_OFFSET_STAT_TX_PKT_128_255_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_128_255_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_256_511_bytes",
          CMAC_OFFSET_STAT_TX_PKT_256_511_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_256_511_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_512_1023_bytes",
          CMAC_OFFSET_STAT_TX_PKT_512_1023_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_512_1023_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_1024_1518_bytes",
          CMAC_OFFSET_STAT_TX_PKT_1024_1518_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_1024_1518_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_1519_1522_bytes",
          CMAC_OFFSET_STAT_TX_PKT_1519_1522_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_1519_1522_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_1523_1548_bytes",
          CMAC_OFFSET_STAT_TX_PKT_1523_1548_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_1523_1548_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_1549_2047_bytes",
          CMAC_OFFSET_STAT_TX_PKT_1549_2047_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_1549_2047_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_2048_4095_bytes",
          CMAC_OFFSET_STAT_TX_PKT_2048_4095_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_2048_4095_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_4096_8191_bytes",
          CMAC_OFFSET_STAT_TX_PKT_4096_8191_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_4096_8191_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_8192_9215_bytes",
          CMAC_OFFSET_STAT_TX_PKT_8192_9215_BYTES(0),
          CMAC_OFFSET_STAT_TX_PKT_8192_9215_BYTES(1)),
    _STAT_ONIC("stat_tx_pkt_large",
          CMAC_OFFSET_STAT_TX_PKT_LARGE(0),
          CMAC_OFFSET_STAT_TX_PKT_LARGE(1)),
    _STAT_ONIC("stat_tx_pkt_small",
          CMAC_OFFSET_STAT_TX_PKT_SMALL(0),
          CMAC_OFFSET_STAT_TX_PKT_SMALL(1)),
    _STAT_ONIC("stat_tx_bad_fcs",
          CMAC_OFFSET_STAT_TX_BAD_FCS(0),
          CMAC_OFFSET_STAT_TX_BAD_FCS(1)),
    _STAT_ONIC("stat_tx_unicast",
          CMAC_OFFSET_STAT_TX_UNICAST(0),
          CMAC_OFFSET_STAT_TX_UNICAST(1)),
    _STAT_ONIC("stat_tx_multicast",
          CMAC_OFFSET_STAT_TX_MULTICAST(0),
          CMAC_OFFSET_STAT_TX_MULTICAST(1)),
    _STAT_ONIC("stat_tx_broadcast",
          CMAC_OFFSET_STAT_TX_BROADCAST(0),
          CMAC_OFFSET_STAT_TX_BROADCAST(1)),
    _STAT_ONIC("stat_tx_vlan",
          CMAC_OFFSET_STAT_TX_VLAN(0),
          CMAC_OFFSET_STAT_TX_VLAN(1)),
    _STAT_ONIC("stat_tx_pause",
          CMAC_OFFSET_STAT_TX_PAUSE(0),
          CMAC_OFFSET_STAT_TX_PAUSE(1)),
    _STAT_ONIC("stat_tx_user_pause",
          CMAC_OFFSET_STAT_TX_USER_PAUSE(0),
          CMAC_OFFSET_STAT_TX_USER_PAUSE(1)),
    _STAT_ONIC("stat_rx_total_pkts",
          CMAC_OFFSET_STAT_RX_TOTAL_PKTS(0),
          CMAC_OFFSET_STAT_RX_TOTAL_PKTS(1)),
    _STAT_ONIC("stat_rx_total_good_pkts",
          CMAC_OFFSET_STAT_RX_TOTAL_GOOD_PKTS(0),
          CMAC_OFFSET_STAT_RX_TOTAL_GOOD_PKTS(1)),
    _STAT_ONIC("stat_rx_total_bytes",
          CMAC_OFFSET_STAT_RX_TOTAL_BYTES(0),
          CMAC_OFFSET_STAT_RX_TOTAL_BYTES(1)),
    _STAT_ONIC("stat_rx_total_good_bytes",
          CMAC_OFFSET_STAT_RX_TOTAL_GOOD_BYTES(0),
          CMAC_OFFSET_STAT_RX_TOTAL_GOOD_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_64_bytes",
          CMAC_OFFSET_STAT_RX_PKT_64_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_64_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_65_127_bytes",
          CMAC_OFFSET_STAT_RX_PKT_65_127_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_65_127_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_128_255_bytes",
          CMAC_OFFSET_STAT_RX_PKT_128_255_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_128_255_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_256_511_bytes",
          CMAC_OFFSET_STAT_RX_PKT_256_511_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_256_511_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_512_1023_bytes",
          CMAC_OFFSET_STAT_RX_PKT_512_1023_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_512_1023_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_1024_1518_bytes",
          CMAC_OFFSET_STAT_RX_PKT_1024_1518_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_1024_1518_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_1519_1522_bytes",
          CMAC_OFFSET_STAT_RX_PKT_1519_1522_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_1519_1522_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_1523_1548_bytes",
          CMAC_OFFSET_STAT_RX_PKT_1523_1548_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_1523_1548_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_1549_2047_bytes",
          CMAC_OFFSET_STAT_RX_PKT_1549_2047_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_1549_2047_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_2048_4095_bytes",
          CMAC_OFFSET_STAT_RX_PKT_2048_4095_BYTES(0),
          CMAC_OFFSET_STAT_RX_PKT_2048_4095_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_4096_8191_bytes",
           CMAC_OFFSET_STAT_RX_PKT_4096_8191_BYTES(0),
           CMAC_OFFSET_STAT_RX_PKT_4096_8191_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_8192_9215_bytes",
           CMAC_OFFSET_STAT_RX_PKT_8192_9215_BYTES(0),
           CMAC_OFFSET_STAT_RX_PKT_8192_9215_BYTES(1)),
    _STAT_ONIC("stat_rx_pkt_large",
           CMAC_OFFSET_STAT_RX_PKT_LARGE(0),
           CMAC_OFFSET_STAT_RX_PKT_LARGE(1)),
    _STAT_ONIC("stat_rx_pkt_small",
           CMAC_OFFSET_STAT_RX_PKT_SMALL(0),
           CMAC_OFFSET_STAT_RX_PKT_SMALL(1)),
    _STAT_ONIC("stat_rx_undersize",
           CMAC_OFFSET_STAT_RX_UNDERSIZE(0),
           CMAC_OFFSET_STAT_RX_UNDERSIZE(1)),
    _STAT_ONIC("stat_rx_fragment",
           CMAC_OFFSET_STAT_RX_FRAGMENT(0),
           CMAC_OFFSET_STAT_RX_FRAGMENT(1)),
    _STAT_ONIC("stat_rx_oversize",
           CMAC_OFFSET_STAT_RX_OVERSIZE(0),
           CMAC_OFFSET_STAT_RX_OVERSIZE(1)),
    _STAT_ONIC("stat_rx_toolong",
          CMAC_OFFSET_STAT_RX_TOOLONG(0),
          CMAC_OFFSET_STAT_RX_TOOLONG(1)),
    _STAT_ONIC("stat_rx_jabber",
          CMAC_OFFSET_STAT_RX_JABBER(0),
          CMAC_OFFSET_STAT_RX_JABBER(1)),
    _STAT_ONIC("stat_rx_bad_fcs",
           CMAC_OFFSET_STAT_RX_BAD_FCS(0),
           CMAC_OFFSET_STAT_RX_BAD_FCS(1)),
    _STAT_ONIC("stat_rx_pkt_bad_fcs",
          CMAC_OFFSET_STAT_RX_PKT_BAD_FCS(0),
          CMAC_OFFSET_STAT_RX_PKT_BAD_FCS(1)),
    _STAT_ONIC("stat_rx_stomped_fcs",
           CMAC_OFFSET_STAT_RX_STOMPED_FCS(0),
           CMAC_OFFSET_STAT_RX_STOMPED_FCS(1)),
    _STAT_ONIC("stat_rx_unicast",
          CMAC_OFFSET_STAT_RX_UNICAST(0),
          CMAC_OFFSET_STAT_RX_UNICAST(1)),
    _STAT_ONIC("stat_rx_multicast",
          CMAC_OFFSET_STAT_RX_MULTICAST(0),
          CMAC_OFFSET_STAT_RX_MULTICAST(1)),
    _STAT_ONIC("stat_rx_broadcast",
          CMAC_OFFSET_STAT_RX_BROADCAST(0),
          CMAC_OFFSET_STAT_RX_BROADCAST(1)),
    _STAT_ONIC("stat_rx_vlan",
          CMAC_OFFSET_STAT_RX_VLAN(0),
          CMAC_OFFSET_STAT_RX_VLAN(1)),
    _STAT_ONIC("stat_rx_pause",
          CMAC_OFFSET_STAT_RX_PAUSE(0),
          CMAC_OFFSET_STAT_RX_PAUSE(1)),
    _STAT_ONIC("stat_rx_user_pause",
          CMAC_OFFSET_STAT_RX_USER_PAUSE(0),
          CMAC_OFFSET_STAT_RX_USER_PAUSE(1)),
    _STAT_ONIC("stat_rx_inrangeerr",
          CMAC_OFFSET_STAT_RX_INRANGEERR(0),
          CMAC_OFFSET_STAT_RX_INRANGEERR(1)),
    _STAT_ONIC("stat_rx_truncated",
          CMAC_OFFSET_STAT_RX_TRUNCATED(0),
          CMAC_OFFSET_STAT_RX_TRUNCATED(1)),
      
    _STAT_NETDEV("rx_xdp_redirect", ETHTOOL_XDP_REDIRECT),
    _STAT_NETDEV("rx_xdp_pass", ETHTOOL_XDP_PASS ),
    _STAT_NETDEV("rx_xdp_drop", ETHTOOL_XDP_DROP ),
    _STAT_NETDEV("rx_xdp_tx",ETHTOOL_XDP_TX ),
    _STAT_NETDEV("rx_xdp_tx_errors", ETHTOOL_XDP_TX_ERR ),
    _STAT_NETDEV("tx_xdp_xmit", ETHTOOL_XDP_XMIT ),
    _STAT_NETDEV("tx_xdp_xmit_errors", ETHTOOL_XDP_XMIT_ERR ),  
};

#define ONIC_QUEUE_STATS_LEN 0
#define ONIC_GLOBAL_STATS_LEN ARRAY_SIZE(onic_gstrings_stats)
#define ONIC_STATS_LEN        (ONIC_GLOBAL_STATS_LEN + ONIC_QUEUE_STATS_LEN)

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

static u32 onic_get_link(struct net_device *netdev)
{
    u32 val, carrier_ok;
    u8 cmac_idx;
    struct onic_private *priv = netdev_priv(netdev);
    struct onic_hardware *hw = &priv->hw;

    cmac_idx = test_bit(ONIC_FLAG_MASTER_PF, priv->flags) ? 0 : 1;

    /* read twice to flush any previously latched value */
    val = onic_read_reg(hw, CMAC_OFFSET_STAT_RX_STATUS(cmac_idx));
    val = onic_read_reg(hw, CMAC_OFFSET_STAT_RX_STATUS(cmac_idx));

    carrier_ok = netif_carrier_ok(netdev);
    /* verify RX status reg bits are 0x3*/
    val = (val == 0x3);
    
    netdev_info(netdev, "ethtool: onic_get_link port: %d   carrier ok: %u -- "
		    "rx status ok: %u\r\n", cmac_idx, carrier_ok, val);

    return (carrier_ok && val);
}

static void onic_get_ethtool_stats(struct net_device *netdev,
            struct ethtool_stats /*__always_unused*/ *stats,
            u64 *data)
{
    struct onic_private *priv = netdev_priv(netdev);
    struct onic_hardware *hw = &priv->hw;
    struct pci_dev *pdev = priv->pdev;
    int i,j;
    u16 func_id;
    u32 off;

    struct {
            u64 xdp_redirect;
            u64 xdp_pass;
            u64 xdp_drop;
            u64 xdp_tx;
            u64 xdp_tx_err;
            u64 xdp_xmit;
            u64 xdp_xmit_err;
      } global_xdp_stats = {0};


      for (j =0; j < priv->num_rx_queues; j++) {
        global_xdp_stats.xdp_redirect += priv->rx_queue[j]->xdp_rx_stats.xdp_redirect;
        global_xdp_stats.xdp_pass += priv->rx_queue[j]->xdp_rx_stats.xdp_pass;
        global_xdp_stats.xdp_drop += priv->rx_queue[j]->xdp_rx_stats.xdp_drop;
        global_xdp_stats.xdp_tx += priv->rx_queue[j]->xdp_rx_stats.xdp_tx;
        global_xdp_stats.xdp_tx_err += priv->rx_queue[j]->xdp_rx_stats.xdp_tx_err;

      }
      for (j =0; j < priv->num_tx_queues; j++) {
        global_xdp_stats.xdp_xmit += priv->tx_queue[j]->xdp_tx_stats.xdp_xmit;
        global_xdp_stats.xdp_xmit_err += priv->tx_queue[j]->xdp_tx_stats.xdp_xmit_err;
      }
    
    func_id = PCI_FUNC(pdev->devfn);

    // Note :
    //   write 1 into REG_TICK (offset 0x2B0).
    //   this is WriteOnce/SelfClear (WO/SC).
    //   with this, the cmac system updates all STAT_* registers.
    if (func_id == 0)
         onic_write_reg(hw, CMAC_OFFSET_TICK(0), 1);
    else onic_write_reg(hw, CMAC_OFFSET_TICK(1), 1);

    for (i = 0; i < ONIC_GLOBAL_STATS_LEN; i++) {
      if (onic_gstrings_stats[i].type == ONIC_STATS) {
        if (func_id == 0)
          off = onic_gstrings_stats[i].stat0_offset;
        else
          off = onic_gstrings_stats[i].stat1_offset;
        data[i] = onic_read_reg(hw, off);
      } else {
        switch (onic_gstrings_stats[i].stat0_offset) {

        case ETHTOOL_XDP_REDIRECT:
          data[i] = global_xdp_stats.xdp_redirect;
          break;
        case ETHTOOL_XDP_PASS:
          data[i] = global_xdp_stats.xdp_pass;
          break;
        case ETHTOOL_XDP_DROP:
          data[i] = global_xdp_stats.xdp_drop;
          break;
        case ETHTOOL_XDP_TX:
          data[i] = global_xdp_stats.xdp_tx;
          break;
        case ETHTOOL_XDP_TX_ERR:
          data[i] = global_xdp_stats.xdp_tx_err;
          break;
        case ETHTOOL_XDP_XMIT:
          data[i] = global_xdp_stats.xdp_xmit;
          break;
        case ETHTOOL_XDP_XMIT_ERR:
          data[i] = global_xdp_stats.xdp_xmit_err;
          break;
        }
      }
    }

    return;
}

static void onic_get_strings(struct net_device *netdev, u32 stringset,
			      u8 *data)
{
	u8 *p = data;
	int i;

    for (i = 0; i < ONIC_GLOBAL_STATS_LEN; i++) {
        memcpy(p, onic_gstrings_stats[i].stat_string,
            ETH_GSTRING_LEN);
        p += ETH_GSTRING_LEN;
    }
}

static int onic_get_sset_count(struct net_device *netdev, int sset)
{
    return ONIC_STATS_LEN;
}

static const struct ethtool_ops onic_ethtool_ops = {
    .get_drvinfo       = onic_get_drvinfo,
    .get_link          = onic_get_link,
    .get_ethtool_stats = onic_get_ethtool_stats,
    .get_strings       = onic_get_strings,
    .get_sset_count    = onic_get_sset_count,
};

void onic_set_ethtool_ops(struct net_device *netdev)
{
    netdev->ethtool_ops = &onic_ethtool_ops;
}
