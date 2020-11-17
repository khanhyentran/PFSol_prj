# vim: syntax=make ts=4 sw=4 sts=4
# Author has to know the difference among '?=', '=', '+=' and ':='

PRJTOP ?= $(shell cd ../ ; pwd)
TOPDIR ?= $(shell cd ../../ ; pwd)

# Output filename
TARGET			?= uart_demo

# Common Project code
C_FILES_COMMON 	?=	\
				$(wildcard $(PRJTOP)/common/*.c)		\
				$(wildcard $(PRJTOP)/CMSIS/*.c)

# Application files to include
C_FILES_APP 	=	\
				$(wildcard $(PRJTOP)/uart_demo/*.c)
LDSCRIPT	= $(PRJTOP)/common/lscript.ld

# Only include common.mk after above have been set
include ../common.mk

# Include this application directory for include path
INCLUDES	+= -I$(shell pwd)

#---------------------------------------------------------------------
all: $(BIN)

#	@cp -v $(BIN) cr7_os.bin
#	@cp -v $(SREC) cr7_os.srec

# Copy binary file to TFTP directory
tftp:
	@cp -v $(BIN) /var/lib/tftpboot

Makefile: ;

../common.mk: ;  # no target for common.mk

.PHONY: all clean kernel build-lib build-bsp-lib distclean

