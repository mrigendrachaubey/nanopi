#!/bin/bash

set -e
DEPENDENCIES=libmad0
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
MINIMAD_GZ=$SYSROOT/usr/share/doc/libmad0-dev/examples/minimad.c.gz
if [ -e $MINIMAD_GZ ];then
	gzip -d $MINIMAD_GZ
fi

$GCC $SYSROOT/usr/share/doc/libmad0-dev/examples/minimad.c --sysroot=$SYSROOT_DIR -lmad -I$SYSROOT_DIR/usr/include -I$SYSROOT_DIR/usr/include/$TOOLCHAIN -o $TARGET_DIR/usr/bin/minimad

