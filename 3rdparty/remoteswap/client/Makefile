obj-m += rswap-client.o
rswap-client-y := rswap_client.o

ifeq ($(BACKEND),DRAM)
	rswap-client-y += rswap_dram.o
	rswap-client-y += rswap_dram_ops.o
else
	rswap-client-y += rswap_rdma_ops.o
	rswap-client-y += rswap_rdma.o
endif

OFA_DIR ?= /usr/src/ofa_kernel/default
OFA_INCLUDE := $(OFA_DIR)/include
OFA_SYMVERS := $(OFA_DIR)/Module.symvers

RSWAP_ROOT_PATH := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
ifneq ($(LINUXINCLUDE),)

LINUXINCLUDE :=                 \
	-I$(RSWAP_ROOT_PATH)/.. \
	$(LINUXINCLUDE)
else

export KBUILD_EXTRA_SYMBOLS=$(OFA_SYMVERS)
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
endif
