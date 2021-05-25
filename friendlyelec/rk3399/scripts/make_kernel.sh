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

function build_kernel_modules() {
    KOPT="ARCH=arm64 CROSS_COMPILE=${TOOLCHAIN_GCC}"
    OUT=/tmp/output_rk3399_kmodules
    rm -rf ${OUT}
    mkdir -p ${OUT}
    make ${KOPT} nanopi4_linux_defconfig
    make ${KOPT} INSTALL_MOD_PATH=${OUT} modules -j$RK_JOBS
    make ${KOPT} INSTALL_MOD_PATH=${OUT} modules_install
    KREL=`make kernelrelease`
    rm -rf ${OUT}/lib/modules/${KREL}/kernel/drivers/gpu/arm/mali400/
    [ ! -f ${OUT}/lib/modules/${KREL}/modules.dep ] && \
        depmod -b ${OUT} -E Module.symvers -F System.map -w ${KREL}
    (cd ${OUT} && find . -name \*.ko | xargs ${TOOLCHAIN_GCC}strip --strip-unneeded)
}

select_toolchain
make ARCH=arm64 CROSS_COMPILE=${TOOLCHAIN_GCC} nanopi4_linux_defconfig
make ARCH=arm64 CROSS_COMPILE=${TOOLCHAIN_GCC} nanopi4-bootimg --jobs=${JOB}
build_kernel_modules
exit 0

