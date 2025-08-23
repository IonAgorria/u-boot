#!/bin/sh

#
# fusing script for ODROID-GO2 based on Rockchip RK3326
#

UBOOT=sd_fuse/uboot.img

if [ -z $1 ]; then
        echo "Usage ./sd_fusing.sh <SD card reader's device>"
        exit 1
fi

sudo dd if=$UBOOT of=$1 conv=fsync bs=512 seek=16384

sync

sudo eject $1

echo Finished
