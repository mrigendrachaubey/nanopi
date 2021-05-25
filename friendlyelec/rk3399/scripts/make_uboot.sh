#!/bin/bash

set -e

JOB=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`
########################################### User can modify #############################################
# User's rkbin tool relative path
RKBIN_TOOLS=../rkbin/tools

# User's GCC toolchain and relative path
ADDR2LINE_ARM64=aarch64-linux-gnu-addr2line
OBJ_ARM64=aarch64-linux-gnu-objdump
GCC_ARM64=aarch64-linux-gnu-
TOOLCHAIN_ARM64=../prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin

select_toolchain()
{
	local absolute_path

	if [ -d ${TOOLCHAIN_ARM64} ]; then
		absolute_path=$(cd `dirname ${TOOLCHAIN_ARM64}`; pwd)
		TOOLCHAIN_GCC=${absolute_path}/bin/${GCC_ARM64}
		TOOLCHAIN_OBJDUMP=${absolute_path}/bin/${OBJ_ARM64}
		TOOLCHAIN_ADDR2LINE=${absolute_path}/bin/${ADDR2LINE_ARM64}
	else
		echo "Can't find toolchain: ${TOOLCHAIN_ARM64}"
		exit 1
	fi
	echo "toolchain: ${TOOLCHAIN_GCC}"
}

select_toolchain
make CROSS_COMPILE=${TOOLCHAIN_GCC} rk3399_defconfig
make CROSS_COMPILE=${TOOLCHAIN_GCC} --jobs=${JOB}
exit 0



