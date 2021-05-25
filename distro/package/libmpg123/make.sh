#!/bin/bash

set -e
DEPENDENCIES=libmpg123-dev
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
MPG123_TO_WAV_C=$DISTRO_DIR/package/libmpg123/mpg123_to_wav.c
MPGLIB=$DISTRO_DIR/package/libmpg123/mpglib.c

$GCC $MPG123_TO_WAV_C --sysroot=$TARGET_DIR -lmpg123 -lout123 -lm -ldl -I$TARGET_DIR/usr/include -I$TARGET_DIR/usr/include/$TOOLCHAIN -o $TARGET_DIR/usr/bin/mpg123_to_wav

$GCC $MPGLIB --sysroot=$TARGET_DIR -lmpg123 -lm -ldl -I$TARGET_DIR/usr/include -I$TARGET_DIR/usr/include/$TOOLCHAIN -o $TARGET_DIR/usr/bin/mpglib
