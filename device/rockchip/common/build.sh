#!/bin/bash

COMMON_DIR=$(cd `dirname $0`; pwd)
if [ -h $0 ]
then
        CMD=$(readlink $0)
        COMMON_DIR=$(dirname $CMD)
fi
cd $COMMON_DIR
cd ../../..
TOP_DIR=$(pwd)
SDFUSE_DIR=$TOP_DIR/friendlyelec/rk3399/sd-fuse_rk3399
COMMON_DIR=$TOP_DIR/device/rockchip/common
BOARD_CONFIG=$TOP_DIR/device/rockchip/.BoardConfig.mk
if [ ! -e "$BOARD_CONFIG" ]; then
	(cd $TOP_DIR/device/rockchip && {
		ln -s rk3399/BoardConfig.mk .BoardConfig.mk
	})
fi
source $BOARD_CONFIG

if [ ! -n "$1" ];then
	BUILD_TARGET=
else
	BUILD_TARGET="$1"
	NEW_BOARD_CONFIG=$TOP_DIR/device/rockchip/$RK_TARGET_PRODUCT/$1
fi

source $TOP_DIR/device/rockchip/common/Version.mk

function usage()
{
	echo "Usage: build.sh [OPTIONS]"
	echo "Available options:"
#	echo "BoardConfig*.mk    -switch to specified board config"
	echo "all                -build uboot, kernel, rootfs, recovery image"
	echo "uboot              -build uboot"
	echo "kernel             -build kernel"
#	echo "modules            -build kernel modules"
	echo "rootfs             -build default rootfs, currently build buildroot as default"
	echo "buildroot          -build buildroot rootfs"
#	echo "ramboot            -build ramboot image"
#	echo "multi-npu_boot     -build boot image for multi-npu board"
#	echo "yocto              -build yocto rootfs"
#	echo "debian             -build debian9 stretch rootfs"
#	echo "distro             -build debian10 buster rootfs"
#	echo "pcba               -build pcba"
#	echo "recovery           -build recovery"
	echo "cleanall           -clean uboot, kernel, rootfs, recovery"
#	echo "firmware           -pack all the image we need to boot up system"
#	echo "updateimg          -pack update image"
#	echo "otapackage         -pack ab update otapackage image"
	echo "sd-img             -pack sd-card image, used to create bootable SD card"
	echo "emmc-img           -pack sd-card image, used to install buildroot to emmc"
#	echo "save               -save images, patches, commands used to debug"
#	echo "allsave            -build all & firmware & updateimg & save"
	echo ""
#	echo "Default option is 'all'."
}

if [ ! -n "$1" ];then
	usage
	exit 1
fi

function build_uboot(){
	echo "============Start build uboot============"
	echo "TARGET_UBOOT_CONFIG=$RK_UBOOT_DEFCONFIG"
	echo "========================================="
	if [ -f u-boot/*_loader_*.bin ]; then
		rm u-boot/*_loader_*.bin
	fi
	cd u-boot && $TOP_DIR/friendlyelec/rk3399/scripts/make_uboot.sh  && cd -
	if [ $? -eq 0 ]; then
		echo "====Build uboot ok!===="
	else
		echo "====Build uboot failed!===="
		exit 1
	fi
}

function build_kernel(){
	echo "============Start build kernel============"
	echo "TARGET_ARCH          =$RK_ARCH"
	echo "TARGET_KERNEL_CONFIG =$RK_KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS    =$RK_KERNEL_DTS"
	echo "=========================================="
	cd $TOP_DIR/kernel && $TOP_DIR/friendlyelec/rk3399/scripts/make_kernel.sh && cd -
	if [ $? -eq 0 ]; then
		echo "====Build kernel ok!===="
	else
		echo "====Build kernel failed!===="
		exit 1
	fi
}

function build_modules(){
	echo "============Start build kernel modules============"
	echo "TARGET_ARCH          =$RK_ARCH"
	echo "TARGET_KERNEL_CONFIG =$RK_KERNEL_DEFCONFIG"
	echo "=================================================="
	cd $TOP_DIR/kernel && make ARCH=$RK_ARCH $RK_KERNEL_DEFCONFIG && make ARCH=$RK_ARCH modules -j$RK_JOBS && cd -
	if [ $? -eq 0 ]; then
		echo "====Build kernel ok!===="
	else	
		echo "====Build kernel failed!===="
		exit 1
	fi
}

function build_buildroot(){
	echo "==========Start build buildroot=========="
	echo "TARGET_BUILDROOT_CONFIG=$RK_CFG_BUILDROOT"
	echo "========================================="
	/usr/bin/time -f "you take %E to build builroot" $COMMON_DIR/mk-buildroot.sh $BOARD_CONFIG
	if [ $? -eq 0 ]; then
		echo "====Build buildroot ok!===="
	else
		echo "====Build buildroot failed!===="
		exit 1
	fi
}

function build_ramboot(){
	echo "=========Start build ramboot========="
	echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "====================================="
	/usr/bin/time -f "you take %E to build ramboot" $COMMON_DIR/mk-ramdisk.sh ramboot.img $RK_CFG_RAMBOOT
	if [ $? -eq 0 ]; then
		echo "====Build ramboot ok!===="
	else
		echo "====Build ramboot failed!===="
		exit 1
	fi
}

function build_multi-npu_boot(){
	if [ -z "$RK_MULTINPU_BOOT" ]; then
		echo "=========Please set 'RK_MULTINPU_BOOT=y' in BoardConfig.mk========="
		exit 1
	fi
	echo "=========Start build multi-npu boot========="
	echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "====================================="
	/usr/bin/time -f "you take %E to build multi-npu boot" $COMMON_DIR/mk-multi-npu_boot.sh
	if [ $? -eq 0 ]; then
		echo "====Build multi-npu boot ok!===="
	else
		echo "====Build multi-npu boot failed!===="
		exit 1
	fi
}

function build_yocto(){
	if [ -z "$RK_YOCTO_MACHINE" ]; then
		echo "This board doesn't support yocto!"
		exit 1
	fi

	echo "=========Start build ramboot========="
	echo "TARGET_MACHINE=$RK_YOCTO_MACHINE"
	echo "====================================="

	cd yocto
	ln -sf $RK_YOCTO_MACHINE.conf build/conf/local.conf
	source oe-init-build-env
	bitbake core-image-minimal
	cd ..

	if [ $? -eq 0 ]; then
		echo "====Build yocto ok!===="
	else
		echo "====Build yocto failed!===="
		exit 1
	fi
}

function build_debian(){
	cd debian

	if [ "$RK_ARCH" == "arm" ]; then
		ARCH=armhf
	fi
	if [ "$RK_ARCH" == "arm64" ]; then
		ARCH=arm64
	fi

	if [ ! -e linaro-stretch-alip-*.tar.gz ]; then
		echo "\033[36m Run mk-base-debian.sh first \033[0m"
		RELEASE=stretch TARGET=desktop ARCH=$ARCH ./mk-base-debian.sh
	fi

	if [ "$ARCH" = "armhf" ]; then
		VERSION=debug ARCH=$ARCH ./mk-rootfs-stretch.sh

	else
		VERSION=debug ARCH=$ARCH ./mk-rootfs-stretch-arm64.sh
	fi

	./mk-image.sh
	cd ..
	if [ $? -eq 0 ]; then
		echo "====Build Debian9 ok!===="
	else
		echo "====Build Debian9 failed!===="
		exit 1
	fi
}

function build_distro(){
	echo "===========Start build debian==========="
	echo "TARGET_ARCH=$RK_ARCH"
	echo "RK_DISTRO_DEFCONFIG=$RK_DISTRO_DEFCONFIG"
	echo "========================================"
	/usr/bin/time -f "you take %E to build debian" $TOP_DIR/distro/make.sh $RK_DISTRO_DEFCONFIG
	if [ $? -eq 0 ]; then
		echo "====Build debian ok!===="
	else
		echo "====Build debian failed!===="
		exit 1
	fi
}

function build_rootfs(){
	rm -f $RK_ROOTFS_IMG

	case "$1" in
		yocto)
			build_yocto
			ROOTFS_IMG=yocto/build/tmp/deploy/images/$RK_YOCTO_MACHINE/rootfs.img
			;;
		debian)
			build_debian
			ROOTFS_IMG=debian/linaro-rootfs.img
			;;
		distro)
			build_distro
			ROOTFS_IMG=rootfs/linaro-rootfs.img
			;;
		*)
			build_buildroot
			ROOTFS_IMG=buildroot/output/$RK_CFG_BUILDROOT/images/rootfs.$RK_ROOTFS_TYPE
			;;
	esac

	[ -z "$ROOTFS_IMG" ] && return

	if [ ! -f "$ROOTFS_IMG" ]; then
		echo "$ROOTFS_IMG not generated?"
	else
		mkdir -p ${RK_ROOTFS_IMG%/*}
		ln -rsf $TOP_DIR/$ROOTFS_IMG $RK_ROOTFS_IMG
	fi
}

function build_recovery(){
	echo "==========Start build recovery=========="
	echo "TARGET_RECOVERY_CONFIG=$RK_CFG_RECOVERY"
	echo "========================================"
	/usr/bin/time -f "you take %E to build recovery" $COMMON_DIR/mk-ramdisk.sh recovery.img $RK_CFG_RECOVERY
	if [ $? -eq 0 ]; then
		echo "====Build recovery ok!===="
	else
		echo "====Build recovery failed!===="
		exit 1
	fi
}

function build_pcba(){
	echo "==========Start build pcba=========="
	echo "TARGET_PCBA_CONFIG=$RK_CFG_PCBA"
	echo "===================================="
	/usr/bin/time -f "you take %E to build pcba" $COMMON_DIR/mk-ramdisk.sh pcba.img $RK_CFG_PCBA
	if [ $? -eq 0 ]; then
		echo "====Build pcba ok!===="
	else
		echo "====Build pcba failed!===="
		exit 1
	fi
}

function build_all(){
	echo "============================================"
	echo "TARGET_ARCH=$RK_ARCH"
	echo "TARGET_PLATFORM=$RK_TARGET_PRODUCT"
	echo "TARGET_UBOOT_CONFIG=$RK_UBOOT_DEFCONFIG"
	echo "TARGET_KERNEL_CONFIG=$RK_KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS=$RK_KERNEL_DTS"
	echo "TARGET_BUILDROOT_CONFIG=$RK_CFG_BUILDROOT"
	echo "TARGET_RECOVERY_CONFIG=$RK_CFG_RECOVERY"
	echo "TARGET_PCBA_CONFIG=$RK_CFG_PCBA"
	echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "============================================"
	build_uboot
	build_kernel
	build_rootfs ${RK_ROOTFS_SYSTEM:-buildroot}
	build_sdimg
}

function build_cleanall(){
	echo "clean uboot, kernel, rootfs, recovery"
	cd $TOP_DIR/u-boot/ && make distclean && cd -
	cd $TOP_DIR/kernel && make distclean && cd -
	rm -rf $TOP_DIR/buildroot/output
	rm -rf $TOP_DIR/yocto/build
	rm -rf $TOP_DIR/distro/output
	rm -rf $TOP_DIR/debian/binary
	(cd friendlyelec/rk3399/sd-fuse_rk3399 && {
		./clean.sh
	})
}

function build_firmware(){
	./mkfirmware.sh $BOARD_CONFIG
	if [ $? -eq 0 ]; then
		echo "Make image ok!"
	else
		echo "Make image failed!"
		exit 1
	fi
}

function build_updateimg(){
	IMAGE_PATH=$TOP_DIR/rockdev
	PACK_TOOL_DIR=$TOP_DIR/tools/linux/Linux_Pack_Firmware
	if [ "$RK_LINUX_AB_ENABLE"x = "true"x ];then
		echo "Make Linux a/b update.img."
		build_otapackage
		source_package_file_name=`ls -lh $PACK_TOOL_DIR/rockdev/package-file | awk -F ' ' '{print $NF}'`
		cd $PACK_TOOL_DIR/rockdev && ln -fs "$source_package_file_name"-ab package-file && ./mkupdate.sh && cd -
		mv $PACK_TOOL_DIR/rockdev/update.img $IMAGE_PATH/update_ab.img
		cd $PACK_TOOL_DIR/rockdev && ln -fs $source_package_file_name package-file && cd -
		if [ $? -eq 0 ]; then
			echo "Make Linux a/b update image ok!"
		else
			echo "Make Linux a/b update image failed!"
			exit 1
		fi

	else
		echo "Make update.img"
		cd $PACK_TOOL_DIR/rockdev && ./mkupdate.sh && cd -
		mv $PACK_TOOL_DIR/rockdev/update.img $IMAGE_PATH
		if [ $? -eq 0 ]; then
			echo "Make update image ok!"
		else
			echo "Make update image failed!"
			exit 1
		fi
	fi
}

function build_otapackage(){
	IMAGE_PATH=$TOP_DIR/rockdev
	PACK_TOOL_DIR=$TOP_DIR/tools/linux/Linux_Pack_Firmware

	echo "Make ota ab update.img"
	source_package_file_name=`ls -lh $PACK_TOOL_DIR/rockdev/package-file | awk -F ' ' '{print $NF}'`
	cd $PACK_TOOL_DIR/rockdev && ln -fs "$source_package_file_name"-ota package-file && ./mkupdate.sh && cd -
	mv $PACK_TOOL_DIR/rockdev/update.img $IMAGE_PATH/update_ota.img
	cd $PACK_TOOL_DIR/rockdev && ln -fs $source_package_file_name package-file && cd -
	if [ $? -eq 0 ]; then
		echo "Make update ota ab image ok!"
	else
		echo "Make update ota ab image failed!"
		exit 1
	fi
}

function copy_and_verify(){
	if [ ! -f $1 ]; then
        echo "not found: $1"
		echo "$3"
		exit 1
	fi
	cp $1 $2
}

function prepare_image_for_friendlyelec_eflasher(){
    local OS_DIR=$1
    local ROOTFS=$2
    if [ ! -d ${SDFUSE_DIR}/${OS_DIR} ]; then
        mkdir ${SDFUSE_DIR}/${OS_DIR}
    fi
    rm -rf ${SDFUSE_DIR}/${OS_DIR}/*

    copy_and_verify $TOP_DIR/u-boot/rk3399_loader_v1.22.119.bin ${SDFUSE_DIR}/${OS_DIR}/MiniLoaderAll.bin "error: please build uboot first."
    copy_and_verify $TOP_DIR/u-boot/uboot.img ${SDFUSE_DIR}/${OS_DIR} "error: please build uboot first."
    copy_and_verify $TOP_DIR/u-boot/trust.img ${SDFUSE_DIR}/${OS_DIR} "error: please build uboot first."
    copy_and_verify $TOP_DIR/kernel/kernel.img ${SDFUSE_DIR}/${OS_DIR} "error: please build kernel first."
    copy_and_verify $TOP_DIR/kernel/resource.img ${SDFUSE_DIR}/${OS_DIR} "error: please build kernel first."

    ###############################
    ######## ramdisk      ########
    ###############################
    copy_and_verify ${SDFUSE_DIR}/prebuilt/idbloader.img ${SDFUSE_DIR}/${OS_DIR} ""
    copy_and_verify ${SDFUSE_DIR}/prebuilt/boot.img ${SDFUSE_DIR}/${OS_DIR} ""

    ###############################
    ######## custom rootfs ########
    ###############################
    FA_TMP_DIR=$TOP_DIR/friendlyelec/tmp-rootfs
    rm -rf $FA_TMP_DIR
    mkdir -p $FA_TMP_DIR

    if [ -f ${ROOTFS} ]; then
        mkdir -p $FA_TMP_DIR/mnt
        copy_and_verify ${ROOTFS} $FA_TMP_DIR/rootfs-org.img "error: please build rootfs first."
        mount -t ext4 -o loop $FA_TMP_DIR/rootfs-org.img $FA_TMP_DIR/mnt
        mkdir $FA_TMP_DIR/rootfs
        cp -af $FA_TMP_DIR/mnt/* $FA_TMP_DIR/rootfs
        umount $FA_TMP_DIR/mnt
        rm -rf $FA_TMP_DIR/mnt
    elif [ -d ${ROOTFS} ]; then
        mkdir $FA_TMP_DIR/rootfs
        echo "Coping rootfs ..."
        cp -af ${ROOTFS}/* $FA_TMP_DIR/rootfs
        clean_device_files $FA_TMP_DIR/rootfs
    else
        echo "error: please build rootfs first."
        exit 1
    fi

    cat << 'EOL' > $FA_TMP_DIR/rootfs/etc/fstab
    # <file system>                 <mount pt>              <type>          <options>               <dump>  <pass>
    proc                            /proc                   proc            defaults                0       0
    devpts                          /dev/pts                devpts          defaults,gid=5,mode=620 0       0
    tmpfs                           /dev/shm                tmpfs           mode=0777               0       0
    tmpfs                           /tmp                    tmpfs           mode=1777               0       0
    tmpfs                           /run                    tmpfs           mode=0755,nosuid,nodev  0       0
    sysfs                           /sys                    sysfs           defaults                0       0
    debug                           /sys/kernel/debug       debugfs         defaults                0       0
    pstore                          /sys/fs/pstore          pstore          defaults                0       0
EOL

    #kernel modules
    KER_MODULES_OUTDIR=/tmp/output_rk3399_kmodules
    mkdir -p $FA_TMP_DIR/rootfs/lib/modules
    if [ -d $KER_MODULES_OUTDIR/lib/modules ]; then
        cp -af $KER_MODULES_OUTDIR/lib/modules/* $FA_TMP_DIR/rootfs/lib/modules/
    else
        echo "error: please build kernel first."
        exit 1
    fi

    #fs-overlay
    rsync -a --exclude='.git' $TOP_DIR/friendlyelec/rk3399/fs-overlay-64/* $FA_TMP_DIR/rootfs/

    # oem
    [ -d $FA_TMP_DIR/rootfs/oem ] || mkdir $FA_TMP_DIR/rootfs/oem
    cp -af $TOP_DIR/device/rockchip/oem/oem_normal/* $FA_TMP_DIR/rootfs/oem
    # userdata
    [ -d $FA_TMP_DIR/rootfs/userdata ] || mkdir $FA_TMP_DIR/rootfs/userdata
    cp -af $TOP_DIR/device/rockchip/userdata/userdata_normal/* $FA_TMP_DIR/rootfs/userdata

    # misc
    rm -f $FA_TMP_DIR/rootfs/misc
    mkdir $FA_TMP_DIR/rootfs/misc

    # remove /dev/console
    rm -f $FA_TMP_DIR/rootfs/dev/console


    MKFS=${SDFUSE_DIR}/tools/make_ext4fs
    ROOT=${FA_TMP_DIR}/rootfs

    # calc image size
    ROOTFS_SIZE=`du -s -B 1 ${ROOT} | cut -f1`
    MAX_IMG_SIZE=7100000000
    TMPFILE=`tempfile`
    ${MKFS} -s -l ${MAX_IMG_SIZE} -a root -L rootfs /dev/null ${ROOT} > ${TMPFILE}
    IMG_SIZE=`cat ${TMPFILE} | grep "Suggest size:" | cut -f2 -d ':' | awk '{gsub(/^\s+|\s+$/, "");print}'`
    cat $TMPFILE
    rm -f ${TMPFILE}

    if [ ${ROOTFS_SIZE} -gt ${IMG_SIZE} ]; then
            pt_error "IMG_SIZE less than ROOTFS_SIZE, why?"
            exit 1
    fi

    # make rootfs.img
    ${MKFS} -s -l ${IMG_SIZE} -a root -L rootfs $FA_TMP_DIR/rootfs.img ${ROOT}
    copy_and_verify $FA_TMP_DIR/rootfs.img ${SDFUSE_DIR}/${OS_DIR}/rootfs.img " "
    rm -rf $FA_TMP_DIR

    # fill size to parameter.txt and partmap.txt
    ROOTFS_PARTITION_SIZE=`printf "0x%X" $(($IMG_SIZE/512))`
    ROOTFS_PARTITION_ADDR=0x00030000
    USERDATA_PARTITION_ADDR=`printf "0x%X" $((${ROOTFS_PARTITION_ADDR}+${ROOTFS_PARTITION_SIZE}))`

    ###############################
    ######## parameter.txt ########
    ###############################
cat > ${SDFUSE_DIR}/${OS_DIR}/parameter.txt << EOL
FIRMWARE_VER: 6.0.1
MACHINE_MODEL: RK3399
MACHINE_ID: 007
MANUFACTURER: RK3399
MAGIC: 0x5041524B
ATAG: 0x00200800
MACHINE: 3399
CHECK_MASK: 0x80
PWR_HLD: 0,0,A,0,1
#KERNEL_IMG: 0x00280000
#FDT_NAME: rk-kernel.dtb
#RECOVER_KEY: 1,1,0,20,0
#in section; per section 512(0x200) bytes
CMDLINE: root=/dev/mmcblk1p7 rw rootfstype=ext4 data=/dev/mmcblk1p8 consoleblank=0 mtdparts=rk29xxnand:0x00002000@0x00002000(uboot),0x00002000@0x00004000(trust),0x00002000@0x00006000(misc),0x00006000@0x00008000(resource),0x00010000@0x0000e000(kernel),0x00010000@0x0001e000(boot),${ROOTFS_PARTITION_SIZE}@0x00030000(rootfs),-@${USERDATA_PARTITION_ADDR}(userdata)

EOL

    mkdir -p ${SDFUSE_DIR}/${OS_DIR}/sd-boot
    ##########################################################
    ######## param4sd.txt (parameter.txt for sd-boot) ########
    ##########################################################
cat > ${SDFUSE_DIR}/${OS_DIR}/sd-boot/param4sd.txt << EOL
FIRMWARE_VER: 6.0.1
MACHINE_MODEL: RK3399
MACHINE_ID: 007
MANUFACTURER: RK3399
MAGIC: 0x5041524B
ATAG: 0x00200800
MACHINE: 3399
CHECK_MASK: 0x80
PWR_HLD: 0,0,A,0,1
#KERNEL_IMG: 0x00280000
#FDT_NAME: rk-kernel.dtb
#RECOVER_KEY: 1,1,0,20,0
#in section; per section 512(0x200) bytes
CMDLINE: root=/dev/mmcblk0p1 rw rootfstype=ext4 data=/dev/mmcblk0p2 consoleblank=0 mtdparts=rk29xxnand:0x00002000@0x00002000(uboot),0x00002000@0x00004000(trust),0x00002000@0x00006000(misc),0x00006000@0x00008000(resource),0x00010000@0x0000e000(kernel),0x00010000@0x0001e000(boot),${ROOTFS_PARTITION_SIZE}@0x00030000(rootfs),-@${USERDATA_PARTITION_ADDR}(userdata)

EOL

    ROOTFS_PARTITION_SIZE=`printf "0x%X" ${IMG_SIZE}`
    ROOTFS_PARTITION_ADDR=0x6000000
    USERDATA_PARTITION_ADDR=`printf "0x%X" $((${ROOTFS_PARTITION_ADDR}+${ROOTFS_PARTITION_SIZE}))`

    #########################################
    ######## partmap.txt for sd-boot ########
    #########################################
cat > ${SDFUSE_DIR}/${OS_DIR}/sd-boot/partmap.txt << EOL
flash=mmc,1:loader:idb:0x8000,0x280000:idbloader.img;
flash=mmc,1:env:env:0x3F8000,0x8000;
flash=mmc,1:parm:parm:0x400000,0x0400000:param4sd.txt;
flash=mmc,1:uboot:raw:0x800000,0x0400000:uboot.img;
flash=mmc,1:trust:raw:0xC00000,0x0400000:trust.img;
flash=mmc,1:misc:raw:0x1000000,0x0400000;
flash=mmc,1:resc:raw:0x1400000,0x0C00000:resource.img;
flash=mmc,1:kern:raw:0x2000000,0x2000000:kernel.img;
flash=mmc,1:boot:raw:0x4000000,0x2000000:boot.img;
flash=mmc,1:rootfs:ext4:0x6000000,${ROOTFS_PARTITION_SIZE}:rootfs.img;
flash=mmc,1:userdata:ext4:${USERDATA_PARTITION_ADDR},0x0:userdata.img;

EOL

    # gen empty userdata.img
    if [ ! -f ${SDFUSE_DIR}/${OS_DIR}/userdata.img ]; then
        USERDATA_SIZE=104857600
        TMPDIR=`mktemp -d`
        ${MKFS} -s -l ${USERDATA_SIZE} -a root -L userdata ${SDFUSE_DIR}/${OS_DIR}/userdata.img ${TMPDIR}
        rm -rf ${TMPDIR}
    fi
}

function clean_device_files()
{
    # create tmp dir
    if [ ! -d ${1}/tmp ]; then
        mkdir ${1}/tmp
    fi
    chmod 1777 ${1}/tmp
    chown root:root ${1}/tmp
    (cd ${1}/dev && find . ! -type d -exec rm {} \;)
}

function build_sdimg(){
    local IMG=
    local ROOTFS=
    if [ -z ${OS_NAME} ]; then
        OS_NAME=buildroot
    fi
    if [ x${OS_NAME} = xbuildroot ]; then
        IMG=rk3399-sd-buildroot-linux-4.4-arm64-$(date +%Y%m%d).img
        ROOTFS=${TOP_DIR}/buildroot/output/rockchip_rk3399/images/rootfs.ext2
    elif [ x${OS_NAME} = xdebian ]; then
        IMG=rk3399-sd-debian9-4.4-armhf-$(date +%Y%m%d).img
        ROOTFS=${TOP_DIR}/rootfs/binary
        if [ ! -d ${TOP_DIR}/rootfs/binary ]; then
            echo "error: please build debian first."
            exit 1
        fi
    else
        echo "Unknow OS: ${OS_NAME}"
        exit 1
    fi
    prepare_image_for_friendlyelec_eflasher ${OS_NAME} ${ROOTFS} && (cd ${SDFUSE_DIR} && ./mk-sd-image.sh ${OS_NAME}) && {
        echo "-----------------------------------------"
        echo "Run the following for sdcard install:"
        echo "    dd if=out/${IMG} of=/dev/sdX bs=1M"
        echo "-----------------------------------------"
    }
}

function build_emmcimg() {
    local IMG=
    local ROOTFS=
    if [ -z ${OS_NAME} ]; then
        OS_NAME=buildroot
    fi
    if [ x${OS_NAME} = xbuildroot ]; then
        IMG=rk3399-eflasher-buildroot-linux-4.4-arm64-$(date +%Y%m%d).img
        ROOTFS=${TOP_DIR}/buildroot/output/rockchip_rk3399/images/rootfs.ext2
    elif [ x${OS_NAME} = xdebian ]; then
        IMG=rk3399-eflasher-debian9-4.4-armhf-$(date +%Y%m%d).img
        ROOTFS=${TOP_DIR}/rootfs/binary
    else
        echo "Unknow OS: ${OS_NAME}"
        exit 1
    fi
    prepare_image_for_friendlyelec_eflasher ${OS_NAME} ${ROOTFS} && (cd ${SDFUSE_DIR} && ./mk-emmc-image.sh ${OS_NAME}) && {
        echo "-----------------------------------------"
        echo "Run the following for sdcard install:"
        echo "    dd if=out/${IMG} of=/dev/sdX bs=1M"
        echo "-----------------------------------------"
    }
}

function build_save(){
	IMAGE_PATH=$TOP_DIR/rockdev
	DATE=$(date  +%Y%m%d.%H%M)
	STUB_PATH=Image/"$RK_KERNEL_DTS"_"$DATE"_RELEASE_TEST
	STUB_PATH="$(echo $STUB_PATH | tr '[:lower:]' '[:upper:]')"
	export STUB_PATH=$TOP_DIR/$STUB_PATH
	export STUB_PATCH_PATH=$STUB_PATH/PATCHES
	mkdir -p $STUB_PATH

	#Generate patches
	$TOP_DIR/.repo/repo/repo forall -c "$TOP_DIR/device/rockchip/common/gen_patches_body.sh"

	#Copy stubs
	$TOP_DIR/.repo/repo/repo manifest -r -o $STUB_PATH/manifest_${DATE}.xml
	mkdir -p $STUB_PATCH_PATH/kernel
	cp $TOP_DIR/kernel/.config $STUB_PATCH_PATH/kernel
	cp $TOP_DIR/kernel/vmlinux $STUB_PATCH_PATH/kernel
	mkdir -p $STUB_PATH/IMAGES/
	cp $IMAGE_PATH/* $STUB_PATH/IMAGES/

	#Save build command info
	echo "UBOOT:  defconfig: $RK_UBOOT_DEFCONFIG" >> $STUB_PATH/build_cmd_info
	echo "KERNEL: defconfig: $RK_KERNEL_DEFCONFIG, dts: $RK_KERNEL_DTS" >> $STUB_PATH/build_cmd_info
	echo "BUILDROOT: $RK_CFG_BUILDROOT" >> $STUB_PATH/build_cmd_info

}

function build_allsave(){
	build_all
	build_firmware
	build_updateimg
	build_save
}

#=========================
# build targets
#=========================
if [ "$BUILD_TARGET" == uboot ];then
    build_uboot
    exit 0
elif [ "$BUILD_TARGET" == kernel ];then
    build_kernel
    exit 0
elif [ "$BUILD_TARGET" == rootfs ];then
    build_rootfs
    exit 0
elif [ "$BUILD_TARGET" == buildroot ];then
    build_buildroot
    exit 0
elif [ "$BUILD_TARGET" == recovery ];then
    build_recovery
    exit 0
elif [ "$BUILD_TARGET" == ramboot ];then
    build_ramboot
    exit 0
elif [ "$BUILD_TARGET" == pcba ];then
    build_pcba
    exit 0
elif [ "$BUILD_TARGET" == yocto ];then
    build_yocto
    exit 0
elif [ "$BUILD_TARGET" == ros ];then
    build_ros
    exit 0
elif [ "$BUILD_TARGET" == debian ];then
    build_debian
    exit 0
elif [ "$BUILD_TARGET" == updateimg ];then
    build_updateimg
    exit 0
elif [ "$BUILD_TARGET" == sd-img ]; then
    build_sdimg
    exit 0
elif [ "$BUILD_TARGET" == emmc-img ]; then
    build_emmcimg
    exit 0
elif [ "$BUILD_TARGET" == all ];then
    build_all
    exit 0
elif [ "$BUILD_TARGET" == firmware ];then
    build_firmware
    exit 0
#elif [ "$BUILD_TARGET" == save ];then
#    build_save
#    exit 0
elif [ "$BUILD_TARGET" == cleanall ];then
    build_cleanall
    exit 0
elif [ "$BUILD_TARGET" == --help ] || [ "$BUILD_TARGET" == help ] || [ "$BUILD_TARGET" == -h ];then
    usage
    exit 0
elif [ -f "$NEW_BOARD_CONFIG" ];then
    rm -f $BOARD_CONFIG
    ln -s $NEW_BOARD_CONFIG $BOARD_CONFIG
else
    echo "Can't found build config, please check again"
    usage
    exit 1
fi
