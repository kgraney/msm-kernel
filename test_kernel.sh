#!/bin/sh

adb reboot bootloader
sleep 5
sudo `which fastboot` -c "console=ttyHSL0,115200,n8 androidboot.hardware=flo user_debug=31 msm_rtb.filter=0x3F ehci-hcd.park=3" -b 0x80200000 -n 2048 -r 0x02000000 boot arch/arm/boot/zImage ramdisk.img
