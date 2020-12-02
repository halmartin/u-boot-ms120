# Summary

Meraki provide the source code for u-boot for the MS120 upon request, however they do not provide the cross compilation toolchain.

This repository downloads the Marvell toolchain from Western Digital, who use the same u-boot release for their MyCloud EX series of NAS devices.

It will then invoke Docker to build u-boot, the output will be in `U-boot.MS120/u-boot.bin`
