#!/bin/bash

set -e
ARCH=$1
SUITE=$2
PKG=$3
INIT=$4

log() {
    local format="$1"
    shift
    printf -- "$format\n" "$@" >&2
}

run() {
    log "I: Running command: %s" "$*"
    "$@"
}

if [ x$ARCH == xarm64 ]; then
        qemu_arch=aarch64
elif [ x$arch == xarm ]; then
        qemu_arch=arm
else
        echo "wrong arch parameter, it should be arm or arm64"
fi

QEMU=qemu-$qemu_arch-static

if [ x$INIT == xinit ];then
	if [ ! -d BUILD_DIR/base/ ];then
		run $SCRIPTS_DIR/get_ubuntu_base.sh $ARCH $SUITE $TARGET_DIR
		run proot -q $QEMU -v -1 -0 -b /dev -b /sys -b /proc -r output/target/ apt-get update
#		run $SCRIPTS_DIR/chmount.sh -m $TARGET_DIR
#		run sudo chroot $TARGET_DIR apt-get update
#		run $SCRIPTS_DIR/chmount.sh -u $TARGET_DIR
		sleep 1
	fi
fi

run proot -q $QEMU -v -1 -0 -b /dev -b /sys -b /proc -r output/target/ apt-get install -y $PKG
#run $SCRIPTS_DIR/chmount.sh -m $TARGET_DIR
#run sudo chroot $TARGET_DIR apt-get install -y $PKG
#run $SCRIPTS_DIR/chmount.sh -u $TARGET_DIR

$SCRIPTS_DIR/fix_link.sh $TARGET_DIR/usr/lib/$TOOLCHAIN

