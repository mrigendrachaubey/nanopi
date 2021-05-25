#!/bin/bash

set -e
PKG=settings
DEPENDENCIES="weston libqt5widgets5 libatomic1 qtwayland5 libqt5bluetooth5 libqt5sql5"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
#QMAKE=/usr/bin/qmake
QMAKE=$TOP_DIR/buildroot/output/$RK_CFG_BUILDROOT/host/bin/qmake
mkdir -p $BUILD_DIR/$PKG
cd $BUILD_DIR/$PKG
$QMAKE $TOP_DIR/app/$PKG
make
mkdir -p $TARGET_DIR/usr/local/$PKG
cp $TOP_DIR/app/$PKG/conf/* $TARGET_DIR/usr/local/$PKG/
mkdir -p $TARGET_DIR/usr/share/applications
install -m 0644 -D $TOP_DIR/app/$PKG/setting.desktop $TARGET_DIR/usr/share/applications/
install -m 0755 -D $BUILD_DIR/$PKG/settings $TARGET_DIR/usr/local/$PKG/settings
cd -

