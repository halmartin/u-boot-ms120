#!/bin/sh

export PATH=$PATH:/opt/toolchain/bin
export CROSS_COMPILE=arm-marvell-linux-gnueabi-
export CROSS_COMPILE_BH=arm-marvell-linux-gnueabi-
cd /src && ./build.pl -f nand -b ac3_customer0 -i spi:nand -a -r
