#!/bin/bash

cd /data1/KhanhTran
./rza_linux-4.14_bsp/output/buildroot-2017.02/output/host/usr/bin/arm-linux-gnueabihf-gcc -static -O2 --sysroot=rza_linux-4.14_bsp/output/buildroot-2017.02/output/host/usr/arm-buildroot-linux-gnueabihf/sysroot/ -o i2c_test_auto1 ./debug_VIN/i2c_test_app.c

cp i2c_test_auto1 rza_linux-4.14_bsp/output/buildroot-2017.02/output/rootfs_overlay/root/
cd rza_linux-4.14_bsp/output/buildroot-2017.02/
make
cd ../../../debug_VIN/rza_linux-4.14_misano/
./build.sh  jlink_rootfs /data1/KhanhTran/rza_linux-4.14_bsp/output/buildroot-2017.02/output/images/rootfs.axfs
