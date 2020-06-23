# We will use this same Makefile for two purposes:
# - Start the build process locally by typing make in the directory that this file
#   is sitting in.
# - Pass the build instructions to the kernel of what files we want built.
#
# Since we will be passing this directory to the kernel as a subdirectory we want
# processed during the build process, the kernel will be looking for a 'Makefile'
# (exact same filename) to know what files need to be built, we might as well use
# the same Makefile.
# Given the fact that then the kernel build system is calling this file, we know
# that the macro KERNELRELEASE will be defined as part of its build system, we use
# that knoledge to split up what parts of this file should be a normal stand-alone
# Makefile (any format we want), and what parts need to be in the format the kernel
# expects them.

ifneq ($(KERNELRELEASE),)

# This is the version that the kernel build scripts will see.
obj-m	:= qspi_flash.o
module-objs := qspi_flash.o


else
# This is the version that will be seen when you type make in your local directory.

ifeq ("$(CROSS_COMPILE)","")
	CC_CHECK:=echo !!ERROR!! CROSS_COMPILE not set
endif

ifeq ("$(BUILDROOT_DIR)","")
	BR_CHECK:=echo !!ERROR!! BUILDROOT_DIR not set
else
endif

# Kernel Source Directory
ifeq ("$(shell test -e $(OUTDIR)/linux-3.14 && echo test)","test")
KDIR		:= $(OUTDIR)/linux-3.14
endif

ifeq ("$(shell test -e $(OUTDIR)/linux-4.9 && echo test)","test")
KDIR		:= $(OUTDIR)/linux-4.9
endi

ifeq ("$(shell test -e $(OUTDIR)/linux-4.19 && echo test)","test")
KDIR		:= $(OUTDIR)/linux-4.19
endif

# Cross Compiler Prefix and ARCH
#CROSS_COMPILE	:= arm-linux-gnueabihf-
#ARCH		:= arm

#######################################################################
# Toolchain setup
#######################################################################

# You want to build apps with the same toolchain and libraries
# that you built your root file system with. We'll assume you
# are using Buildroot.
# Therefore, make sure BUILDROOT is defined before calling this
# Makefile.


# 'BUILDROOT_DIR' should be imported after running setup_env.sh

# Define the base path of the toolchain
TC_PATH:=$(BUILDROOT_DIR)/output/host/usr
export PATH:=$(TC_PATH)/bin:$(PATH)

# Find out what the prefix of the toolchain that is used for Buildroot
GCCEXE:=$(notdir $(wildcard $(TC_PATH)/bin/*abi*-gcc))
CROSS_COMPILE:=$(subst gcc,,$(GCCEXE))

# Find the location of the sysroot where all the libraries and header files are
SYSROOT:=$(shell find $(TC_PATH) -name sysroot)

# Manual setup if set_env.sh is not used
#TC_PATH=/home/renesas/toolchain/buildroot-2014.05/output/host/usr
#CROSS_COMPILE=arm-buildroot-linux-uclibcgnueabi-
ARCH:=arm

#######################################################################
# Application Specific setup
#######################################################################
app:=qspi_app

LIBS:=


# Location of Driver Source
DRVDIR		:= $(shell pwd)

MAKE := make

export ARCH CROSS_COMPILE

all: driver app

driver:
	@echo "KDIR = $(KDIR)"
	@echo "CROSS_COMPILE = $(CROSS_COMPILE)"
	@$(CHECK)
	@$(MAKE) -C $(KDIR) M=$(DRVDIR) modules

app: qspi_app.c
	@$(BR_CHECK)
	@$(CROSS_COMPILE)gcc $(STATIC) $(CFLAGS) --sysroot=$(SYSROOT_DIR) -o $(app) $(app).c $(LIBS)

# Strip off debug symbols if not debugging (makes file size smaller)
strip:
	$(CROSS_COMPILE)strip --strip-debug qspi_flash.ko
	$(CROSS_COMPILE)strip --strip-debug qspi_app

clean:
	@$(MAKE) -C $(KDIR) M=$(DRVDIR) $@
	@-rm -f qspi_flash.ko qspi_app

install:
	@# Copy to Buildroot overlay directory (only for Renesas BSP)
	@cp $(app) $(BUILDROOT_DIR)/output/rootfs_overlay/root/bin
	@mkdir -p  $(BUILDROOT_DIR)/output/rootfs_overlay/root/module
	@mkdir -p  $(BUILDROOT_DIR)/output/rootfs_overlay/root/scripts
	@cp -a qspi_flash.ko $(BUILDROOT_DIR)/output/rootfs_overlay/root/module
	@cp -a scripts_rza2mevb/* $(BUILDROOT_DIR)/output/rootfs_overlay/root/scripts
	@echo ''
	@echo 'Please run Buildroot again to repackage the rootfs image.'
	@echo 'For example:'
	@echo ''
	@echo '   cd $$ROOTDIR ; ./build.sh buildroot ; ./build.sh axfs ; cd -'
	@echo ''
	@echo 'After you boot, you can run the scripts qspi_save.sh'
	@echo 'and qspi_restore.sh.'
	@echo ''

# Send the module to the targe board over serial console using ZMODEM
# Enter "rz -y" on the target board (from a R/W directory sush as /tmp), then
# run 'make send' to start the transfer.
# Requires package 'lrzsz' installed on both host and target
send:
	sz -b qspi_flash.ko qspi_app > /dev/ttyACM0 < /dev/ttyACM0


endif

