#!/bin/bash

set -e
DEPENDENCIES=libdrm
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
mkdir -p $BUILD_DIR/mpp
rm -f $BUILD_DIR/mpp/CMakeCache.txt
cd $BUILD_DIR/mpp
cmake -DRKPLATFORM=ON -DHAVE_DRM=ON -DCMAKE_INSTALL_LIBDIR=/usr/lib/$TOOLCHAIN -DCMAKE_INSTALL_PREFIX=/usr $TOP_DIR/external/mpp
make
make install
cd -
