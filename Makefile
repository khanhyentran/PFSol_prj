#CC=gcc
#CFLAGS=-std=c99
TC_PATH := /home/renesas/rza1/bsp/rza_linux-4.19_bsp/output/buildroot-2018.02/output/host
CC = $(TC_PATH)/bin/arm-linux-gnueabihf-gcc
SYSROOT = $(TC_PATH)/arm-buildroot-linux-gnueabihf/sysroot
CFLAGS = --sysroot=$(SYSROOT)

ifdef DEBUG
  CFLAGS += -ggdb -Wall -DDEBUG
else
  CFLAGS += -O3 -Wall
endif


all: bayer2rgb
	
bayer2rgb: bayer2rgb.c bayer.o
	$(CC) $(CFLAGS) -o bayer2rgb bayer.o bayer2rgb.c -lm
ifdef RELEASE
	strip bayer2rgb
endif

bayer.o: bayer.c
	$(CC) $(CFLAGS) -std=c99 -c bayer.c

clean:
	-rm bayer2rgb bayer.o *\~ 
