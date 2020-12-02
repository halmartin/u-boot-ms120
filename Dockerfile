FROM ubuntu:18.04

# the Marvell toolchain is i386, so we have to add these
RUN dpkg --add-architecture i386
RUN apt-get update && apt-get install -y --no-install-recommends \
  perl binutils make build-essential \
  libc6:i386 libncurses5:i386 libstdc++6:i386

COPY armv7-marvell-linux-gnueabi-softfp_i686_64K_Dev_20131002.tar.gz /tmp/
WORKDIR /tmp
RUN mkdir -p /opt/toolchain && tar -C /opt/toolchain --strip-components=1 -zxvf armv7-marvell-linux-gnueabi-softfp_i686_64K_Dev_20131002.tar.gz

CMD ["/bin/sh", "/tmp/build-uboot.sh"]
