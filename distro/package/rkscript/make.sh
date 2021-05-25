#!/bin/bash

set -e
DEPENDENCIES="android-tools-adbd parted gdisk"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
source $TOP_DIR/device/rockchip/.BoardConfig.mk
source $OUTPUT_DIR/.config
install -m 0755 -D $TOP_DIR/external/rkscript/S50usbdevice $TARGET_DIR/etc/init.d/
install -m 0644 -D $TOP_DIR/external/rkscript/61-usbdevice.rules $TARGET_DIR/lib/udev/rules.d/
install -m 0755 -D $TOP_DIR/external/rkscript/usbdevice $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/glmarktest.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/gstaudiotest.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/gstmp3play.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/gstmp4play.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/gstvideoplay.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/gstvideotest.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/gstwavplay.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/mp3play.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/waylandtest.sh $TARGET_DIR/usr/bin/
install -m 0755 -D $TOP_DIR/external/rkscript/S21mountall.sh $TARGET_DIR/etc/init.d/
install -m 0755 -D $TOP_DIR/external/rkscript/fstab $TARGET_DIR/etc/
install -m 0644 -D $TOP_DIR/external/rkscript/61-partition-init.rules $TARGET_DIR/lib/udev/rules.d/
install -m 0644 -D $TOP_DIR/external/rkscript/61-sd-cards-auto-mount.rules $TARGET_DIR/lib/udev/rules.d/
install -m 0755 -D $TOP_DIR/external/rkscript/resize-helper $TARGET_DIR/usr/sbin/
install -m 0755 -D $TOP_DIR/external/rkscript/S22resize-disk $TARGET_DIR/etc/init.d/
echo -e "/dev/disk/by-partlabel/oem\t/oem\t\t\t$RK_OEM_FS_TYPE\t\tdefaults\t\t0\t2" >> $TARGET_DIR/etc/fstab
echo -e "/dev/disk/by-partlabel/userdata\t/userdata\t\t$RK_USERDATA_FS_TYPE\t\tdefaults\t\t0\t2" >> $TARGET_DIR/etc/fstab
mkdir -p $TARGET_DIR/oem $TARGET_DIR/userdata $TARGET_DIR/mnt/sdcard
cd $TARGET_DIR/
ln -fs userdata data
ln -fs mnt/sdcard sdcard
cd -

CONFIG_FILE=$TARGET_DIR/etc/init.d/.usb_config
if [ ! -e $CONFIG_FILE ];then
	touch $CONFIG_FILE
fi
if [ x$BR2_PACKAGE_RKSCRIPT_UMS == xy ];then
	if [ ! `grep usb_ums_en $CONFIG_FILE` ];then
		echo usb_ums_en >> $CONFIG_FILE
	fi
fi
if [ x$BR2_PACKAGE_RKSCRIPT_ADBD == xy ];then
	if [ ! `grep usb_adb_en $CONFIG_FILE` ];then
		echo usb_adb_en >> $CONFIG_FILE
	fi
fi

