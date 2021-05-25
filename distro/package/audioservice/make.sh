#!/bin/bash

set -e
PKG=audioservice
DEPENDENCIES=gstreamer
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
mkdir -p $BUILD_DIR/$PKG
rm -f $BUILD_DIR/$PKG/CMakeCache.txt
cd $BUILD_DIR/$PKG
cmake -DCMAKE_INSTALL_LIBDIR=/usr/lib/$TOOLCHAIN -DCMAKE_INSTALL_PREFIX=/usr $TOP_DIR/external/audioservice
make
install -m 0755 -D $BUILD_DIR/$PKG/audioservice $TARGET_DIR/usr/bin/
cd -
