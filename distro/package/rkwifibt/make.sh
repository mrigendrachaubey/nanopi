#!/bin/bash

set -e
DEPENDENCIES="wpasupplicant dhcpcd5 ntp ntpdate"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
source $OUTPUT_DIR/.config
if [ -e $TOP_DIR/build.sh ] && [ ! -e $OUTPUT_DIR/.kernelmodules.done ];then
	export LDFLAGS="--sysroot=$SYSROOT"
	$TOP_DIR/build.sh modules
	touch $OUTPUT_DIR/.kernelmodules.done
fi

BT_TTY=ttyS0
echo "BR2_PACKAGE_RKWIFIBT_BTUART:$BR2_PACKAGE_RKWIFIBT_BTUART"
if [ -n $BR2_PACKAGE_RKWIFIBT_BTUART ];then
	BT_TTY=$BR2_PACKAGE_RKWIFIBT_BTUART
fi

mkdir -p $TARGET_DIR/system/lib/modules
mkdir -p $TARGET_DIR/system/etc/firmware
$GCC $TOP_DIR/external/rkwifibt/src/rk_wifi_init.c -o $TARGET_DIR/usr/bin/rk_wifi_init
$GCC $TOP_DIR/external/rkwifibt/brcm_tools/brcm_patchram_plus1.c -o $TARGET_DIR/usr/bin/brcm_patchram_plus1
find $TOP_DIR/kernel/drivers/net/wireless/rockchip_wlan/*  -name "*.ko" | xargs -n1 -i cp {} $TARGET_DIR/system/lib/modules/
install -m 0644 -D $TOP_DIR/external/rkwifibt/firmware/broadcom/all/WIFI_FIRMWARE/* $TARGET_DIR/system/etc/firmware/
install -m 0644 -D $TOP_DIR/external/rkwifibt/firmware/broadcom/all/BT_FIRMWARE/* $TARGET_DIR/system/etc/firmware/
install -m 0755 -D $TOP_DIR/external/rkwifibt/S66load_wifi_modules $TARGET_DIR/etc/init.d/
sed -i "s/BT_TTY_DEV/\/dev\/$BT_TTY/g" $TARGET_DIR/etc/init.d/S66load_wifi_modules
install -m 0644 -D $TOP_DIR/external/rkwifibt/wpa_supplicant.conf $TARGET_DIR/etc/wpa_supplicant.conf
install -m 0644 -D $TOP_DIR/external/rkwifibt/dnsmasq.conf $TARGET_DIR/etc/dnsmasq.conf
install -m 0755 -D $TOP_DIR/external/rkwifibt/wifi_start.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkwifibt/S67wifi $TARGET_DIR/etc/init.d/
install -m 0755 -D $PACKAGE_DIR/rkwifibt/S41dhcpcd $TARGET_DIR/etc/init.d/
install -m 0755 -D $PACKAGE_DIR/rkwifibt/S69ntp $TARGET_DIR/etc/init.d/
install -m 0755 -D $PACKAGE_DIR/rkwifibt/watch_ntpd.sh $TARGET_DIR/usr/bin/

sed -i s/debian/cn/g $TARGET_DIR/etc/ntp.conf
