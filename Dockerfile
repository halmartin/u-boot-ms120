FROM ubuntu:18.04

# the Marvell toolchain is i386, so we have to add these
RUN dpkg --add-architecture i386
RUN apt-get update && apt-get install -y --no-install-recommends \
  perl binutils make build-essential wget ca-certificates \
  libc6:i386 libncurses5:i386 libstdc++6:i386

RUN mkdir -p /opt/toolchain && wget -qO- https://downloads.wdc.com/gpl/WDMyCloud_EX2100_GPL_v2.31.204_20191206.tar.gz | tar --strip-components=2 -O -zxf - WDMyCloud_EX2100_GPL_v2.31.204_20191206/toolchain/armv7-marvell-linux-gnueabi-softfp_i686_64K_Dev_20131002.tar.gz | tar -C /opt/toolchain --strip-components=1 -zxf -

CMD ["/bin/sh", "/tmp/build-uboot.sh"]
