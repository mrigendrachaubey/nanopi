#!/bin/bash

set -e
DEPENDENCIES=gst-plugins-base
PKG=gst-plugins-ugly
VERSION=1.14.4
source $OUTPUT_DIR/.config
if [ ! -e $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz ];then
	wget -P $DOWNLOAD_DIR https://gstreamer.freedesktop.org/src/gst-plugins-ugly/$PKG-$VERSION.tar.xz
fi

if [ ! -d $BUILD_DIR/$PKG/$PKG-$VERSION ];then
	tar -xf $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz -C $BUILD_DIR/$PKG
	mv $BUILD_DIR/$PKG/$PKG-$VERSION/* $BUILD_DIR/$PKG/
fi

cd $BUILD_DIR/$PKG
OPTS="--target=aarch64-linux-gnu --host=aarch64-linux-gnu --prefix=/usr --libdir=/usr/lib/$TOOLCHAIN --disable-gtk-doc --disable-gtk-doc-html --disable-dependency-tracking --disable-nls --disable-static --enable-shared  --disable-examples --disable-valgrind"

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_ASFDEMUX = xy ];then
	OPTS="$OPTS --enable-asfdemux"
else
	OPTS="$OPTS --disable-asfdemux"
fi
if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_DVDLPCMDEC = xy ];then
	OPTS="$OPTS --enable-dvdlpcmdec"
else
	OPTS="$OPTS --disable-dvdlpcmdec"
fi
if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_DVDSUB = xy ];then
	OPTS="$OPTS --enable-dvdsub"
else
	OPTS="$OPTS --disable-dvdsub"
fi
if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_XINGMUX = xy ];then
	OPTS="$OPTS --enable-xingmux"
else
	OPTS="$OPTS --disable-xingmux"
fi
if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_REALMEDIA = xy ];then
	OPTS="$OPTS --enable-realmedia"
else
	OPTS="$OPTS --disable-realmedia"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_A52DEC = xy ];then
	OPTS="$OPTS --enable-a52dec"
else
	OPTS="$OPTS --disable-a52dec"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_AMRNB = xy ];then
	OPTS="$OPTS --enable-amrnb"
else
	OPTS="$OPTS --disable-amrnb"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_AMRWB = xy ];then
	OPTS="$OPTS --enable-amrwb"
else
	OPTS="$OPTS --disable-amrwb"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_CDIO = xy ];then
	OPTS="$OPTS --enable-cdio"
else
	OPTS="$OPTS --disable-cdio"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_DVDREAD = xy ];then
	OPTS="$OPTS --enable-dvdread"
else
	OPTS="$OPTS --disable-dvdread"
fi
if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_MPEG2DEC = xy ];then
	OPTS="$OPTS --enable-mpeg2dec"
	DEPENDENCIES="$DEPENDENCIES libmpeg2-4"
else
	OPTS="$OPTS --disable-mpeg2dec"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_SIDPLAY = xy ];then
	OPTS="$OPTS --enable-sidplay"
else
	OPTS="$OPTS --disable-sidplay"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_UGLY_X264 = xy ];then
	OPTS="$OPTS --enable-x264"
else
	OPTS="$OPTS --disable-x264"
fi
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
echo "opts: $OPTS"
./configure $OPTS
make
make install
$SCRIPTS_DIR/fixlibtool.sh $TARGET_DIR $TARGET_DIR
cd -
