#!/bin/bash

set -e
DEPENDENCIES="libdrm-dev"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
PKG=libdrm
mkdir -p $BUILD_DIR/$PKG
cd $BUILD_DIR/$PKG
$TOP_DIR/external/libdrm/autogen.sh --srcdir=$TOP_DIR/external/libdrm --libdir=/usr/lib/$TOOLCHAIN --includedir=/usr/include --target=aarch64-linux-gnu --host=aarch64-linux-gnu --disable-dependency-tracking --disable-static --enable-shared  --disable-cairo-tests --disable-manpages --disable-intel --disable-radeon --disable-amdgpu --disable-nouveau --disable-vmwgfx --disable-omap-experimental-api --disable-etnaviv-experimental-api --disable-exynos-experimental-api --disable-freedreno --disable-tegra-experimental-api --disable-vc4 --enable-rockchip-experimental-api --enable-udev --disable-valgrind --enable-install-test-programs
make 
make install
$SCRIPTS_DIR/fixlibtool.sh $TARGET_DIR $TARGET_DIR
cd -
