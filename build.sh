#!/bin/bash

#PATH=/home/renesas/tools/gcc-linaro-arm-linux-gnueabihf-4.8-2014.02_linux/bin:$PATH
#PATH=/home/renesas/tools/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin:$PATH
#PATH=/home/renesas/tools/gcc-linaro-5.4.1-2017.01-x86_64_arm-linux-gnueabihf/bin:$PATH
PATH=/home/renesas/rza1/bsp/rza_linux-4.9_bsp/output/buildroot-2017.02/output/host/usr/bin:$PATH

# As for GCC 4.9, you can get a colorized output
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'

# U-BOOT BUILD SCRIPT

export CROSS_COMPILE="arm-linux-gnueabihf-"
export ARCH=arm

# Switch between building for different platforms
OUT=.out
#OUT=.out_rza1hires
#OUT=.out_genmai
#OUT=.out_grpeach
#OUT=.out_streamit
#OUT=.out_ylcdrza1h
#OUT=.out_test


if [ ! -e $OUT ] ; then
 mkdir $OUT
fi

#########################################################################
# Create a defconfig
#########################################################################
if [ "$1" == "defconfig" ] ; then

  if [ "$2" == "" ] ; then
    echo "./build.sh defconfig rskrza1"
    echo "./build.sh defconfig genmai"
    echo "./build.sh defconfig grpeach"
    echo "./build.sh defconfig streamit"
    echo "./build.sh defconfig ylcdrza1h"
    echo " "
    echo "./build.sh defconfig rza2mevb"
    exit
  fi

  make O=$OUT savedefconfig
  #cp -v $OUT/defconfig configs/$2_defconfig
  cp -v $OUT/defconfig configs/$2_defconfig
  exit
fi

#########################################################################
# relocate for debugging
#########################################################################
if [ "$1" == "reloc" ] ; then

  if [ "$2" == "" ] ; then
    #arm-linux-gnueabihf-objdump -h $OUT/u-boot
    arm-linux-gnueabihf-objdump -h $OUT/u-boot-reloc

    echo "you need to specifythe relocadd from u-boot"
    echo "=> bdinfo"
    echo "./build.sh reloc 0x2096E000"
    exit
  fi

  #arm-linux-gnueabihf-objcopy --change-addresses $2 $OUT/u-boot $OUT/u-boot-reloc
  #arm-linux-gnueabihf-objcopy --change-section-address .text=$2 $OUT/u-boot $OUT/u-boot-reloc
arm-linux-gnueabihf-objcopy --change-section-address .text+$2 $OUT/u-boot $OUT/u-boot-reloc


  exit
fi


#########################################################################
# Write u-boot into RAM, then program into QSPI
#########################################################################
if [ "$1" == "jlink_load" ] ; then
  if [ "$(ps -e | grep JLinkGDBServer)" != "" ]; then
    echo "ERROR: JLinkGDBServer is running"
    exit
  fi
  cp -v $OUT/u-boot.bin /tmp/u-boot.bin

  echo " " > /tmp/jlink_load.txt
  echo "loadbin /tmp/u-boot.bin,0x80000000" >> /tmp/jlink_load.txt
  echo "g" >> /tmp/jlink_load.txt
  echo "exit" >> /tmp/jlink_load.txt
  JLinkExe -speed 15000 -if JTAG -device R7S721001 -CommanderScript /tmp/jlink_load.txt

FILESIZE=$(cat /tmp/u-boot.bin | wc -c)
if [ $FILESIZE -le $((0x40000)) ]; then	# <= 256KB?
  echo "
# Program 256KB u-boot into QSPI using u-boot
=> sf probe 0 ; sf erase 0 40000 ; sf write 80000000 0 40000
"
elif [ $FILESIZE -le $((0x50000))  ]; then	# <= 320KB?
  echo "
# (MICRON) Program 320KB u-boot into QSPI using u-boot
=> sf probe 0 ; sf erase 0 50000 ; sf write 80000000 0 50000
"
elif [ $FILESIZE -le $((0x60000))  ]; then	# <= 384KB?
  echo "
# (MICRON) Program 384KB u-boot into QSPI using u-boot
=> sf probe 0 ; sf erase 0 60000 ; sf write 80000000 0 60000
"
elif [ $FILESIZE -le $((0x70000))  ]; then	# <= 448KB?
  echo "
# (MICRON) Program 448KB u-boot into QSPI using u-boot
=> sf probe 0 ; sf erase 0 70000 ; sf write 80000000 0 70000
"
elif [ $FILESIZE -le $((0x80000))  ]; then	# < 512KB?
  echo "
# (MICRON) Program 512KB u-boot into QSPI using u-boot
=> sf probe 0 ; sf erase 0 80000 ; sf write 80000000 0 80000
"
fi
  echo "
# (SPANSION) Program 512KB u-boot into QSPI using u-boot
=> sf probe 0 ; sf erase 0 80000 ; sf write 80000000 0 80000
"


echo "# Compare
=> md 80000000; qspi single ; md 20000000
"
du -h /tmp/u-boot.bin
  exit
fi

#########################################################################
# Write u-boot into internal RAM and run
#########################################################################
if [ "$1" == "jlink_ram" ] ; then
  if [ "$(ps -e | grep JLinkGDBServer)" != "" ]; then
    echo "ERROR: JLinkGDBServer is running"
    exit
  fi

  if [ ! -e $OUT/u-boot-ram.bin ] ; then
    echo "file  $OUT/u-boot-ram.bin  does not exit"
    exit
  fi
  cp -v $OUT/u-boot-ram.bin /tmp/u-boot.bin

  echo " " > /tmp/jlink_load.txt
  echo "rx 100" >> /tmp/jlink_load.txt
  echo "loadbin /tmp/u-boot.bin,0x80020000" >> /tmp/jlink_load.txt
  echo "SetPC 0x80020000" >> /tmp/jlink_load.txt
  echo "g" >> /tmp/jlink_load.txt
  echo "exit" >> /tmp/jlink_load.txt
  JLinkExe -speed 15000 -if JTAG -jtagconf -1,-1 -device R7S721001 -CommanderScript /tmp/jlink_load.txt

  echo "
NOTE: If it's a blank board, make sure you first power the board, hit reset,
      then plug in the JLINK USB cable
"
  exit
fi

#########################################################################
# Program u-boot directly into QSPI
#########################################################################
if [ "$1" == "jlink_burn" ] ; then
  cp -v $OUT/u-boot.bin /tmp/u-boot.bin

  echo " " > /tmp/jlink_load.txt
  echo "rx 100" >> /tmp/jlink_load.txt
  echo "loadbin /tmp/u-boot.bin,0x20000000" >> /tmp/jlink_load.txt
  echo "SetPC 0x20000000" >> /tmp/jlink_load.txt
  echo "g" >> /tmp/jlink_load.txt
  echo "exit" >> /tmp/jlink_load.txt
  #JLinkExe -speed 15000 -if JTAG -jtagconf -1,-1 -device R7S721001 -CommanderScript /tmp/jlink_load.txt
  JLinkExe -speed 15000 -if JTAG -jtagconf -1,-1 -device R7S921053VCBG -CommanderScript /tmp/jlink_load.txt
echo " Written "
  exit
fi

#########################################################################
# GDB Debugging
#########################################################################
if [ "$1" == "gdb" ] ; then
  if [ "$(ps -e | grep JLinkGDBServer)" == "" ] ; then
    echo "=========================================="
    echo "ERROR: You need start JLinkGDBServer first"
    echo " "
    echo "JLinkGDBServer -speed 15000 -device R7S921053VCBG"
    echo "=========================================="
    exit
  fi

  # GDB location settings
  #GDB_PATH=/home/renesas/rza1/apps/gdb/gdb-7.9.1/build-gdb/z_install/bin
  #GDB_EXE=arm-buildroot-linux-uclibcgnueabi-gdb
#  GDB_PATH=/home/renesas/rza1/rskrza1/rskrza1_bsp/output/gcc-linaro-arm-linux-gnueabihf-4.8-2014.02_linux/bin
  GDB_EXE=arm-linux-gnueabihf-gdb

  cd $OUT

  if [ ! -e .gdbinit_ASDF ] ; then
    echo "
define ramload
	#Get rid of any existing breakpoints
	delete

	# load in our file/binary
	monitor reset
	file u-boot
	monitor loadbin $(pwd)/u-boot.bin,0x80020000

	#Run the code till right before the relocation
	set \$pc = 0x20020000
	tb relocate_code
	c
	shell sleep 1

	#Figure out where it will be copying to, then reload symbol file
	set \$i = gd->relocaddr
	print \$i
	symbol-file
	add-symbol-file u-boot \$i
	tb board_init_r
	c

	#At the end, you should be in the board_init_r function.
	#Now you can set breakpoint or whatever you want
end



target remote localhost:2331
#monitor go
monitor reset
file u-boot" > .gdbinit
  fi

  # Start up GDB in TUI mode
 if [ "$GDB_PATH" != "" ] ; then
   $GDB_PATH/$GDB_EXE -tui
 else
   $GDB_EXE -tui
 fi


  exit
fi

# Find out how many CPU processor cores we have on this machine
# so we can build faster by using multithreaded builds
NPROC=2
if [ "$(which nproc)" != "" ] ; then  # make sure nproc is installed
  NPROC=$(nproc)
fi
BUILD_THREADS=$(expr $NPROC + $NPROC)

#MAKE="make -j8 O=$OUT"
# silent Build
#MAKE="make -s -j8 O=$OUT"
MAKE="make -j$BUILD_THREADS O=$OUT"


# Configure
#if [ ! -e $OUT/.config ] ; then
#  CMD="$MAKE rskrza1_config"
#  echo $CMD ; $CMD
#fi

if [ -e $OUT/u-boot.bin ] ; then
  rm $OUT/u-boot.bin
fi

CMD="$MAKE $1 $2 $3"
echo $CMD ; $CMD

# this means text color
# \[37m TEXT .\033[0m

#this means fg and bg color
# \[37;44m TEXT .\033[0m

# This means BOLD
# \033[1m  TEXT  \033[0m

if [ "$1" == "clean" ]; then
  exit
fi

if [ "${1: -9}" == "defconfig" ] ; then
  exit
fi


# Create List files for debugging
#mkdir -p /tmp/uboot
#arm-linux-gnueabihf-objdump -SdCg $OUT/arch/arm/lib/board.o > /tmp/uboot/board.lst
#arm-linux-gnueabihf-objdump -SdCg $OUT/arch/arm/lib/libarm.o > /tmp/uboot/libarm.lst
#arm-linux-gnueabihf-objdump -SdCg $OUT/arch/arm/cpu/armv7/start.o > /tmp/uboot/start.lst

if [ "$1" == "" ] ; then
if [ -e $OUT/u-boot.bin ] ; then
  echo -ne "\033[1;32m"
  echo "== Build Successfull =="
  echo -ne "\033[00m"
else
  echo -ne "\033[1;31m"
  echo "== Build Failed =="
  echo -ne "\033[00m"
fi
fi

ASDF=`grep "20000000 T _start" $OUT/System.map`
if [ "$ASDF" == "" ] ; then

  echo -ne "\033[1;33m" # YELLOW TEXT
  echo "== WARNING, you just built the debug version that doesn't start at 20000000  =="
  grep "T _start" $OUT/System.map
  echo -ne "\033[00m"

  ASDF=`grep "20020000 T _start" $OUT/System.map`
  if [ "$ASDF" != "" ] ; then
    cp -v $OUT/u-boot.bin $OUT/u-boot-ram.bin
  fi
fi

