#!/bin/bash

set -e
DEPENDENCIES="libdrm v4l-utils gstreamer gst-plugins-base camera_engine_rkisp"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
mkdir -p $BUILD_DIR/gst-plugins-rockchip
cd $BUILD_DIR/gst-plugins-rockchip
$TOP_DIR/external/gst-plugins-rockchip/autogen.sh  --prefix=/usr --libdir=/usr/lib/$TOOLCHAIN --host=aarch64-linux-gnu
make
make install
cd -
