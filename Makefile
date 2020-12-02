current_dir:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

.PHONY: all

target: all

deps:
	# fetch u-boot source
	wget -c https://dl.meraki.net/U-boot-MS120-20191119.tar.bz2
	if [ -f U-boot-MS120-20191119.tar.bz2 ] && [ ! -d U-boot.MS120 ]; then tar -jxf U-boot-MS120-20191119.tar.bz2; fi
	# Meraki doesn't include the Marvell toolchain used to build u-boot, nice people at Cisco, so get it from WD
	wget -c https://downloads.wdc.com/gpl/WDMyCloud_EX2100_GPL_v2.31.204_20191206.tar.gz
	if [ -f WDMyCloud_EX2100_GPL_v2.31.204_20191206.tar.gz ] && [ ! -f armv7-marvell-linux-gnueabi-softfp_i686_64K_Dev_20131002.tar.gz ]; then tar --strip-components=2 -zxf WDMyCloud_EX2100_GPL_v2.31.204_20191206.tar.gz WDMyCloud_EX2100_GPL_v2.31.204_20191206/toolchain/; fi

docker-image:
	docker build -t ubuntu1804-marvell -f Dockerfile .

docker-build:
	docker run -it --rm -v $(current_dir)/U-boot.MS120:/src -v $(current_dir)/build-uboot.sh:/tmp/build-uboot.sh ubuntu1804-marvell

clean:
	rm -rf U-boot.MS120

clean-all:
	rm -rf U-boot.MS120
	rm U-boot-MS120-20191119.tar.bz2
	rm WDMyCloud_EX2100_GPL_v2.31.204_20191206.tar.gz

all: deps docker-image docker-build
