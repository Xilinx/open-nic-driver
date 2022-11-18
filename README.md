# AMD OpenNIC Driver

This is one of the three components of the
[OpenNIC project](https://github.com/Xilinx/open-nic.git).  The other components are:
- [OpenNIC shell](https://github.com/Xilinx/open-nic-shell.git) and
- [OpenNIC DPDK](https://github.com/Xilinx/open-nic-dpdk.git).

OpenNIC driver implements a Linux kernel driver for OpenNIC shell.  It supports
multiple PCI-e PFs with multiple TX/RX queues in each PF, and up to two 100Gbps
ports on the same card.  As of version 1.0, the driver has not implemented the
ethtool routines to change the hash key and the indirection table.

The driver has been tested on under Ubuntu 18.04, 20.04, and 22.04 with multiple versions of the Linux kernel.

## Building the Driver

Follow the steps below to build the driver.

1. Run `make` to compile the loadable kernel module `onic.ko`.
2. Connect 100Gbps cables/loopback adapters to enabled ports before insert the
   kernel module.  Currently, the driver does not detect link status change.
   Thus links should be ready before loading the driver.
3. Run `sudo insmod onic.ko` to insert the kernel module. (There is an optional 
   parameter RS_FEC_ENABLED, which can be set to either zero or one.)
5. Verify that no error message is printed through `dmesg`, and new devices show
   up in `ifconfig` output.

The driver registers a net device for each PF it probed.  Net devices are
registered with multiple queues.  The number of queues depends on the number of
MSI-X vectors available through the associated PF.  In particular, for PF0 which
acts as the master PF, the number of queues equals to the number of MSI-X
vectors minus 2, one for card-level error interrupt and one for function-level
user interrupt; for other PFs, it equals to the number of MSI-X vectors minus 1.
Each net device has the same number of TX and RX queues.

For each FPGA card loaded with the OpenNIC shell bitstream, the driver detects
the number of CMAC instances and manages the links accordingly.  Only PF0 can
enable/disable the links.  The default bitstream, when configured with 2 PFs and
2 CMAC instances, maps PF0 to port0 and PF1 to port1.

## Testing the Driver

### Loopback Test

A loopback register is accessible from BAR2 at the offset `0x8090` and `0xC090`,
for port0 and port1 respectively.  One could use
[pcimem](https://github.com/billfarrow/pcimem) to read/write PCI device
registers.

To enable loopback, write `0x1` to the loopback register.  For instance, to
enable loopback on port0, issue the following command.

    sudo ./pcimem /sys/devices/pci0000:d7/0000:d7:00.0/0000:d8:00.0/resource2 0x8090 w 0x1

After that, packets received by CMAC0 will be looped back to the host.  Write
`0x0` to disable loopback.

Here is a simple scenario to test the loopback mode.  Assume the interface name
is `enp216s0f0` and the IP address is `192.168.1.10`.

1. Run `tcpdump` to capture packets on the interface.

        sudo tcpdump -i enp216s0f0 -xx

2. Run `ping 192.168.1.10`.
3. Observe that packets captured by `tcpdump` are always duplicated.

### ETHTOOL Test

`ethtool` is a Linux utility and used to control and read status of
various MAC parameters. Also, this tool is used to obtain various counter registers
(such as total good packets etc.) from the MAC.

  If the tool is not found, install it from distro.

  ```
  $ sudo apt install ethtool
  ```

  List all options that the tool can support

  ```
  $ ethtool -h
  ```

  List active interfaces, activate required interface
  Note: assume interface name is xyz01, IP address is 192.168.1.1

  ```
  $ ifconfig -a

  $ ifconfig xyz01 192.168.1.1 up
  ```

  Use ethtool interface to see the status
  Note: assume interface name is xyz01

  ```
  $ ethtool xyz01
  ```

  Show driver information
  Note: assume interface name is xyz01

  ```
  $ ethtool -i xyz01
  ```

  Show adapter statistics
  Note: assume interface name is xyz01

  ```
  $ ethtool -S xyz01
  ```


## Known Issues

### Static IP Address

It has been found that in some cases, DHCP clients may cause kernel panic after
inserting the kernel module.  A message similar as below show up in `dmesg`.

    [  224.835445] BUG: unable to handle kernel paging request at ffff9d7f45effa1f

Assigning a static network address seems to solve the issue in most cases.  Add
the following lines into `/etc/network/interfaces` with the correct interface
name, IP address.

    ```
    auto IF_NAME
    iface IF_NAME inet static
          address IP_ADDRESS
    ```

An alternative is to uninstall DHCP.  This can be done by killing any running processes using DHCP with
`ps -eF | grep dhclient`, and then to disable DHCP.


### Machine locks up when installing kernel module

This seems to be related to the DHCP issue mentioned above in "Static IP Address".  The recommendation
is to disable DHCP.



---

# Copyright Notice and Disclaimer

© Copyright 2020 – 2021 Xilinx, Inc. All rights reserved.

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER

This disclaimer is not a license and does not grant any rights to the materials
distributed herewith.  Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law: (1) THESE
MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX HEREBY
DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising
under or in connection with these materials, including for any direct, or any
indirect, special, incidental, or consequential loss or damage (including loss
of data, profits, goodwill, or any type of loss or damage suffered as a result
of any action brought by a third party) even if such damage or loss was
reasonably foreseeable or Xilinx had been advised of the possibility of the
same.

CRITICAL APPLICATIONS

Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring failsafe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer assumes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES.
