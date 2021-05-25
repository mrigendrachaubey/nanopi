#!/bin/bash

if [ $RK_ARCH == arm64 ];then
	export TOOLCHAIN=aarch64-linux-gnu
elif [ $RK_ARCH == arm ];then
	export TOOLCHAIN=arm-linux-gnueabihf
fi
