current_dir:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

.PHONY: all

target: all

deps:
	# fetch u-boot source
	# mirrored: https://web.archive.org/web/20201202202817/https://dl.meraki.net/U-boot-MS120-20191119.tar.bz2
	wget -c https://dl.meraki.net/U-boot-MS120-20191119.tar.bz2
	if [ -f U-boot-MS120-20191119.tar.bz2 ] && [ ! -d U-boot.MS120 ]; then tar -jxf U-boot-MS120-20191119.tar.bz2; fi

docker-image:
	docker build -t ubuntu1804-marvell -f Dockerfile .

docker-build:
	docker run -it --rm -v $(current_dir)/U-boot.MS120:/src -v $(current_dir)/build-uboot.sh:/tmp/build-uboot.sh ubuntu1804-marvell

clean:
	rm -rf U-boot.MS120

clean-all:
	rm -rf U-boot.MS120

all: deps docker-image docker-build
