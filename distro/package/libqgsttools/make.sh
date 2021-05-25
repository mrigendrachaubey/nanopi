#!/bin/bash

set -e
PKG=libqgsttools
if [ $ARCH = arm64 ];then
	NAME=$PACKAGE_DIR/$PKG/libqgsttools-p1_5.7.1_20161021-2_arm64/data.tar.xz
elif [ $ARCH = arm ];then
	NAME=$PACKAGE_DIR/$PKG/libqgsttools-p1_5.7.1_20161021-2_armhf/data.tar.xz
fi
mkdir -p $BUILD_DIR/$PKG
tar -xf $NAME -C $TARGET_DIR/

