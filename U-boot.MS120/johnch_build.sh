#!/bin/bash
PATH=$PATH:/marvell/toolchain/armv7-marvell-linux-gnueabi-softfp_i686_64K_Dev_20131002/bin
export CROSS_COMPILE_BH=arm-marvell-linux-gnueabi-
export CROSS_COMPILE=arm-marvell-linux-gnueabi-
#make clean
#make mrproper
./build.pl -f spi -v 14t3 -i spi:nand -b ac3_rd
cp u-boot-msys-ac3-14t3-spi.bin /marvell/tftp
