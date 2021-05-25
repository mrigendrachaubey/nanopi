#!/bin/bash

set -e

PKG=camera_engine_rkisp
RKafDir=$TARGET_DIR/usr/lib/rkisp/af
RKaeDir=$TARGET_DIR/usr/lib/rkisp/ae
RKawbDir=$TARGET_DIR/usr/lib/rkisp/awb

mkdir -p $BUILD_DIR/$PKG
mkdir -p $RKafDir
mkdir -p $RKaeDir
mkdir -p $RKawbDir
mkdir -p $TARGET_DIR/etc/iqfiles

cd $TOP_DIR/external/$PKG
make ARCH=$ARCH OUTDIR=$BUILD_DIR/$PKG/build
install -D -m 755 $BUILD_DIR/$PKG/build/bin/rkisp_demo $TARGET_DIR/usr/local/bin/
install -D -m 644 $TOP_DIR/external/$PKG/iqfiles/*.xml $TARGET_DIR/etc/iqfiles/
install -D -m 644 $BUILD_DIR/$PKG/build/lib/librkisp.so $TARGET_DIR/usr/lib/
if [ $RK_ARCH = arm ];then
	install -D -m 644 $TOP_DIR/external/$PKG/plugins/3a/rkiq/af/lib32/librkisp_af.so $RKafDir/
	install -D -m 644 $TOP_DIR/external/$PKG/plugins/3a/rkiq/aec/lib32/librkisp_aec.so $RKaeDir/
	install -D -m 644 $TOP_DIR/external/$PKG/plugins/3a/rkiq/awb/lib32/librkisp_awb.so $RKawbDir/
elif [ $RK_ARCH = arm64 ];then
	install -D -m 644 $TOP_DIR/external/$PKG/plugins/3a/rkiq/af/lib64/librkisp_af.so $RKafDir/
	install -D -m 644 $TOP_DIR/external/$PKG/plugins/3a/rkiq/aec/lib64/librkisp_aec.so $RKaeDir/
	install -D -m 644 $TOP_DIR/external/$PKG/plugins/3a/rkiq/awb/lib64/librkisp_awb.so $RKawbDir/
fi
cd -
