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
srcdir = $(PWD)
obj-m += onic.o
BASE_OBJS := $(patsubst $(srcdir)/%.c,%.o,$(wildcard $(srcdir)/*.c $(srcdir)/*/*.c $(srcdir)/*/*/*.c))
onic-objs = $(BASE_OBJS)
ccflags-y = -O3 -Wall -Werror -I$(srcdir)/qdma_access -I$(srcdir)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *.o.ur-safe
	rm -f ./qdma_access/*.o.ur-safe
