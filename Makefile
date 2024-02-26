# 
# Copyright (c) 2020 Xilinx, Inc.
# All rights reserved.
# 
# This source code is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
# 
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#
ifdef KVERSION
KERNEL_VERS = $(KVERSION)
else
KERNEL_VERS = $(shell uname -r)
endif

srcdir = $(PWD)
obj-m += onic.o
BASE_OBJS := $(patsubst $(srcdir)/%.c,%.o,$(wildcard $(srcdir)/*.c $(srcdir)/*/*.c $(srcdir)/*/*/*.c))
onic-objs = $(BASE_OBJS)
ccflags-y = -O3 -Wall -Werror -I$(srcdir)/qdma_access -I$(srcdir)/hwmon -I$(srcdir)

KDIR ?= /lib/modules/$(KERNEL_VERS)/build

all:
	make -C $(KDIR) M=$(PWD) modules

with-clang:
	make CC=clang -C $(KDIR) M=$(PWD) modules
	
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *.o.ur-safe
	rm -f ./qdma_access/*.o.ur-safe

install:
	rm -f /lib/modules/$(KERNEL_VERS)/onic.ko
	cp onic.ko /lib/modules/$(KERNEL_VERS)
	depmod

uninstall:
	rm -f /lib/modules/$(KERNEL_VERS)/onic.ko
	depmod
