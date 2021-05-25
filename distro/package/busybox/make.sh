#!/bin/bash

set -e
DEPENDENCIES=udev
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE $DEPENDENCIES
PKG=busybox
VERSION=1.27.2
if [ ! -e $DOWNLOAD_DIR/$PKG-$VERSION.tar.bz2 ];then
	wget -P $DOWNLOAD_DIR https://busybox.net/downloads/$PKG-$VERSION.tar.bz2
fi

if [ ! -e $BUILD_DIR/$PKG/.timestamp ];then
	tar -jxf $DOWNLOAD_DIR/$PKG-$VERSION.tar.bz2 -C $BUILD_DIR/$PKG
	mv $BUILD_DIR/$PKG/$PKG-$VERSION/* $BUILD_DIR/$PKG/
	$SCRIPTS_DIR/apply-patches.sh $BUILD_DIR/$PKG $PACKAGE_DIR/busybox
	cp $PACKAGE_DIR/busybox/busybox.config $BUILD_DIR/$PKG/.config
fi

rm -f $TARGET_DIR/bin/mount
rm -f $TARGET_DIR/bin/umount
rm -f $TARGET_DIR/bin/mountpoint

cd $BUILD_DIR/$PKG
make oldconfig
/bin/sed -i -e 's/^noclobber="0"$/noclobber="1"/' $BUILD_DIR/$PKG/applets/install.sh
make
make CONFIG_PREFIX="$TARGET_DIR" install
cd -

install -m 0644 -D $PACKAGE_DIR/busybox/inittab $TARGET_DIR/etc/inittab

if grep -q CONFIG_UDHCPC=y $BUILD_DIR/$PKG/.config; then
	install -m 0755 -D $PACKAGE_DIR/busybox/udhcpc.script $TARGET_DIR/usr/share/udhcpc/default.script; 
	install -m 0755 -d $TARGET_DIR/usr/share/udhcpc/default.script.d; 
fi

if grep -q CONFIG_SYSLOGD=y $BUILD_DIR/$PKG/.config; then
	install -m 0755 -D $PACKAGE_DIR/busybox/S01logging $TARGET_DIR/etc/init.d/S01logging
fi

if grep -q CONFIG_FEATURE_TELNETD_STANDALONE=y $BUILD_DIR/$PKG/.config; then
	install -m 0755 -D $PACKAGE_DIR/busybox/S50telnet $TARGET_DIR/etc/init.d/S50telnet
fi

install -D -m 0755 $PACKAGE_DIR/busybox/rcK $TARGET_DIR/etc/init.d/
install -D -m 0755 $PACKAGE_DIR/busybox/rcS $TARGET_DIR/etc/init.d/
install -D -m 0755 $PACKAGE_DIR/busybox/S20urandom $TARGET_DIR/etc/init.d/
if [ -e $TARGET_DIR/etc/init.d/udev ];then
	mv $TARGET_DIR/etc/init.d/udev $TARGET_DIR/etc/init.d/S10udev
fi

