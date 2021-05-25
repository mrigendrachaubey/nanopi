#!/bin/bash

export DISTRO_DIR=$(dirname $(realpath "$0"))
export TOP_DIR=$(realpath $DISTRO_DIR/..)
export OUTPUT_DIR=$DISTRO_DIR/output
export TARGET_DIR=$OUTPUT_DIR/target
export BUILD_DIR=$OUTPUT_DIR/build
export IMAGE_DIR=$OUTPUT_DIR/images
export ROOTFS_DIR=$OUTPUT_DIR/rootfs
export SYSROOT_DIR=$OUTPUT_DIR/target
export CONFIGS_DIR=$DISTRO_DIR/configs
export PACKAGE_DIR=$DISTRO_DIR/package
export DOWNLOAD_DIR=$DISTRO_DIR/download
export SCRIPTS_DIR=$DISTRO_DIR/scripts
export OVERLAY_DIR=$DISTRO_DIR/overlay
export MOUNT_DIR=$TARGET_DIR/sdk
export BUILDROOT_DIR=$TOP_DIR/buildroot
export BUILDROOT_PKG_DIR=$BUILDROOT_DIR/package
export OVERRIDE_PKG_DIR=$DISTRO_DIR/package/override

if [ $RK_ARCH == arm64 ];then
	export TOOLCHAIN_DIR=$TOP_DIR/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
	export TOOLCHAIN=aarch64-linux-gnu
elif [ $RK_ARCH == arm ];then
	export TOOLCHAIN_DIR=$TOP_DIR/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf
	export TOOLCHAIN=arm-linux-gnueabihf
fi
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin"
export ARCH="RK_ARCH"
export CROSS_COMPILE="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-"
export AR="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-ar"
export AS="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-as"
export LD="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-ld"
export NM="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-nm"
export CC="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-gcc"
export GCC="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-gcc"
export CPP="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-cpp"
export CXX="$TOOLCHAIN_DIR/bin/$TOOLCHAIN-g++"
export SYSROOT="$SYSROOT_DIR"
export CMAKE_SYSROOT="$SYSROOT_DIR"
export STAGING_DIR="$SYSROOT_DIR"
export PKG_CONFIG="/usr/bin/pkg-config"
export PKG_CONFIG_PATH="$TARGET_DIR/usr/lib/$TOOLCHAIN/pkgconfig:$TARGET_DIR/usr/share/pkgconfig"
export PKG_CONFIG_LIBDIR="$TARGET_DIR/usr/lib/$TOOLCHAIN/pkgconfig:$TARGET_DIR/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$TARGET_DIR"
export PKG_CONFIG_ALLOW_SYSTEM_LIBS=1
export DESTDIR="$TARGET_DIR"
export PREFIX="$TARGET_DIR"
export CFLAGS="-I$SYSROOT_DIR/usr/include -I$SYSROOT_DIR/usr/include/$TOOLCHAIN -I$TARGET_DIR/usr/include -I$TARGET_DIR/usr/include/$TOOLCHAIN --sysroot=$SYSROOT_DIR"
export CXXFLAGS=$CFLAGS
export LDFLAGS="--sysroot=$TARGET_DIR -Wl,-rpath-link,$TARGET_DIR/lib:$TARGET_DIR/usr/lib:$TARGET_DIR/lib/$TOOLCHAIN:$TARGET_DIR/usr/lib:$TARGET_DIR/usr/lib/$TOOLCHAIN -L$TARGET_DIR/usr/lib"
