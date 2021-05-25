#!/bin/bash

set -e
DEPENDENCIES="gstreamer-rockchip weston libqt5widgets5 libatomic1 qtwayland5 libqt5multimedia5 libqt5multimediawidgets5 libqt5quickwidgets5 qml-module-qtmultimedia qml-module-qtquick2"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
PKG=video
#QMAKE=/usr/bin/qmake
QMAKE=$TOP_DIR/buildroot/output/$RK_CFG_BUILDROOT/host/bin/qmake
mkdir -p $BUILD_DIR/$PKG
cd $BUILD_DIR/$PKG
$QMAKE $TOP_DIR/app/$PKG
make
mkdir -p $TARGET_DIR/usr/local/$PKG
cp $TOP_DIR/app/$PKG/conf/* $TARGET_DIR/usr/local/$PKG/
mkdir -p $TARGET_DIR/usr/share/applications
install -m 0644 -D $TOP_DIR/app/$PKG/video.desktop $TARGET_DIR/usr/share/applications/
install -m 0755 -D $BUILD_DIR/$PKG/videoPlayer $TARGET_DIR/usr/local/$PKG/videoPlayer
cd -

