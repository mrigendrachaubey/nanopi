#!/bin/bash

set -e
DEPENDENCIES="libdrm libpng-dev libjpeg-dev libudev-dev libmali"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
COMMIT=9a03892d0ef250b0eb5c87792dbfbd48e23d15bb
PKG=glmark2
if [ ! -e $DOWNLOAD_DIR/$PKG-$COMMIT.tar.gz ];then
	wget -O $DOWNLOAD_DIR/$PKG-$COMMIT.tar.gz https://github.com/glmark2/glmark2/archive/$COMMIT/$PKG-$COMMIT.tar.gz
fi

if [ ! -e $BUILD_DIR/$PKG/.timestamp ];then
	rm -rf $BUILD_DIR/$PKG
	tar -xzf $DOWNLOAD_DIR/$PKG-$COMMIT.tar.gz -C $BUILD_DIR
	mv $BUILD_DIR/$PKG-$COMMIT $BUILD_DIR/$PKG
fi
source $OUTPUT_DIR/.config
cd $BUILD_DIR/$PKG
if [ x$BR2_PACKAGE_LIBDRM = xy ] && [ x$BR2_PACKAGE_WESTON = xy ];then
	./waf configure --with-flavors=drm-glesv2,wayland-glesv2 --prefix=/usr
elif [ x$BR2_PACKAGE_LIBDRM = xy ];then
	./waf configure --with-flavors=drm-glesv2 --prefix=/usr
elif [ x$BR2_PACKAGE_WESTON = xy ];then
	./waf configure --with-flavors=wayland-glesv2 --prefix=/usr
fi
./waf
./waf install
cd -
