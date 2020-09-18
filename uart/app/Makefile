# example Makefile
include config.mk

PRJTOP ?= $(shell pwd)

FREERTOS_SOURCE			= $(PRJTOP)/FreeRTOS/Source
FREERTOS_PLUS_DEMO		= $(PRJTOP)/FreeRTOS-Plus/Demo/Common/FreeRTOS_Plus_CLI_Demos
FREERTOS_PLUS_SOURCE		= $(PRJTOP)/FreeRTOS-Plus/Source
RCAR_BSP_SOURCE			= $(PRJTOP)/FreeRTOS/RTOS/BSP
DEMO_SOURCE			= $(PRJTOP)/FreeRTOS/Demo/R-Car_Gen3_CR7

ASM_FILES	=	\
				$(wildcard $(FREERTOS_SOURCE)/portable/GCC/ARM_CR7/*.S)		\
				$(wildcard $(DEMO_SOURCE)/common/*.S)

C_FILES 	=	\
				$(wildcard $(DEMO_SOURCE)/common/*.c)		\
				$(wildcard $(DEMO_SOURCE)/CMSIS/*.c)		\
				$(wildcard $(DEMO_SOURCE)/Blinky/*.c)		\
				$(wildcard $(DEMO_SOURCE)/Early_AV/*.c)		\
				$(wildcard $(DEMO_SOURCE)/RZG2_CAN_LED/*.c) \
				$(wildcard $(DEMO_SOURCE)/UART_TEST/*.c)

FREERTOS_PLUS_FILES	=	\
				$(wildcard $(FREERTOS_PLUS_SOURCE)/FreeRTOS-Plus-CLI/*.c)	\
				$(wildcard $(FREERTOS_PLUS_SOURCE)/FreeRTOS-Plus-Trace/*.c)	\
				$(wildcard $(FREERTOS_PLUS_SOURCE)/*.c)			\
				$(FREERTOS_PLUS_DEMO)/Sample-CLI-commands.c		\
				$(FREERTOS_PLUS_DEMO)/UARTCommandConsole.c

KERNEL_SRC	=	\
				$(wildcard $(FREERTOS_SOURCE)/*.c)				\
				$(wildcard $(FREERTOS_SOURCE)/portable/GCC/ARM_CR7/*.c)		\
				$(FREERTOS_SOURCE)/portable/MemMang/heap_4.c

KOBJS		= $(KERNEL_SRC:.c=.o)
OBJS 		:= $(ASM_FILES:.S=.o) $(C_FILES:.c=.o) $(FREERTOS_PLUS_FILES:.c=.o)

BIN		= $(TARGET).bin
OUTPUT 		= $(TARGET).axf
MAP		= $(TARGET).map
SREC		= $(TARGET).srec

LDSCRIPT	= $(DEMO_SOURCE)/common/lscript.ld


INCLUDES	= -I$(PRJTOP)/FreeRTOS/Demo/Common/include
INCLUDES	+= -I$(FREERTOS_SOURCE)/include
INCLUDES	+= -I$(FREERTOS_SOURCE)/portable/GCC/ARM_CR7
INCLUDES	+= -I$(FREERTOS_PLUS_SOURCE)/FreeRTOS-Plus-CLI
INCLUDES	+= -I$(FREERTOS_PLUS_SOURCE)/FreeRTOS-Plus-Trace/Include
INCLUDES	+= -I$(FREERTOS_PLUS_SOURCE)/FreeRTOS-Plus-Trace/config
INCLUDES	+= -I$(DEMO_SOURCE)/common
INCLUDES	+= -I$(DEMO_SOURCE)/CMSIS
INCLUDES	+= -I$(RCAR_BSP_SOURCE)/can/inc
INCLUDES	+= -I$(RCAR_BSP_SOURCE)/uart/inc
INCLUDES	+= -I$(RCAR_BSP_SOURCE)/common/inc
FREERTOS_INC = $(INCLUDES)

BSPLIB_PATH	= $(RCAR_BSP_SOURCE)/lib
BSPLIBS		= $(BSPLIB_PATH)/lib$(BSPNAME).a

export BSPLIBS
export FREERTOS_INC

CPPFLAGS	= $(CONFIG_FLAG) $(DEFINES) $(INCLUDES) -nostartfiles -L$(BSPLIB_PATH) $(CFLAGS)
CPPFLAGS	+= -Wall -nostdlib

ASFLAGS     = -marm -march=armv7-r -nostdinc -ffreestanding -Wa,--fatal-warnings \
			  -Werror -Wmissing-include-dirs -D__ASSEMBLY $(DEFINES) $(INCLUDES)


export RCAR_BSP_SOURCE
#---------------------------------------------------------------------
#all: build-lib build-bsp-lib kernel
all: kernel

build-lib:
	@echo -e "[ BUILD LIB ]"

build-bsp-lib:
	@echo -e "[ BUILD BSP LIB & TEST ]"
	$(MAKE) -C $(RCAR_BSP_SOURCE) all

kernel: $(BIN);

clean:
	@echo -e "[ CLEAN ]"
	$(RM) -f $(MAP) $(OUTPUT) $(BIN) $(KOBJS) $(OBJS) $(SREC)
	$(MAKE) -C $(RCAR_BSP_SOURCE) clean

distclean: clean
	@$(RM) -f GPATH GRTAGS GTAGS

$(OUTPUT): build-lib build-bsp-lib $(KOBJS) $(OBJS)
	#@echo -e "[ BUILD ]"
	$(CC) -T $(LDSCRIPT) -nostartfiles -L$(BSPLIB_PATH) $(CONFIG_FLAG) -g -o $@ $(OBJS) $(KOBJS) -lc -l$(BSPNAME)

$(MAP): $(OUTPUT)
	@echo -e "[ MAP ]"
	$(NM) $< > $@

$(BIN): $(MAP)
	@echo -e "[ BIN ]"
	$(OBJCOPY) -O binary $(OUTPUT) $(BIN)
	$(OBJCOPY) --srec-forceS3 -O srec $(OUTPUT) $(SREC)

%.o: %.S
	$(CC) $(ASFLAGS) -I. -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) -I. -c -o $@ $<

%: force
	$(MAKE) -C $(KERNEL_SRC) $@

force: ;

Makefile: ;

.PHONY: all clean config.mk kernel build-lib build-bsp-lib distclean

