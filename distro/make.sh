#!/bin/bash

set -e
DISTRO_DIR=$(dirname $(realpath "$0"))
TOP_DIR=$DISTRO_DIR/..

source $TOP_DIR/device/rockchip/.BoardConfig.mk
source $DISTRO_DIR/envsetup.sh
source $OUTPUT_DIR/.config
DISTRO_CONFIG=$OUTPUT_DIR/.config
ROOTFS_DEBUG_EXT4=$IMAGE_DIR/rootfs.debug.ext4
ROOTFS_DEBUG_SQUASHFS=$IMAGE_DIR/rootfs.debug.squashfs
ROOTFS_EXT4=$IMAGE_DIR/rootfs.ext4
ROOTFS_SQUASHFS=$IMAGE_DIR/rootfs.squashfs
BUILD_PACKAGE=$1
export SUITE=buster
export ARCH=$RK_ARCH

OS=`$SCRIPTS_DIR/get_distro.sh $SUITE`

log() {
    local format="$1"
    shift
    printf -- "$format\n" "$@" >&2
}

die() {
    local format="$1"
    shift
    log "E: $format" "$@"
    exit 1
}

run() {
    log "I: Running command: %s" "$*"
    "$@"
}

clean()
{
	rm -rf $OUTPUT_DIR
}

pack_squashfs()
{
	SRC=$1
	DST=$2
	mksquashfs $SRC $DST -noappend -comp gzip
}

pack_ext4()
{
	SRC=$1
	DST=$2
	SIZE=`du -sk --apparent-size $SRC | cut --fields=1`
	inode_counti=`find $SRC | wc -l`
	inode_counti=$[inode_counti+512]
	EXTRA_SIZE=$[inode_counti*4]
	SIZE=$[SIZE+EXTRA_SIZE]
	run genext2fs -b $SIZE -N $inode_counti -d $SRC $DST
	run e2fsck -fy $DST
	run tune2fs -O dir_index,filetype $DST
#	if [ -x $DISTRO_DIR/../device/rockchip/common/mke2img.sh ];then
#		$DISTRO_DIR/../device/rockchip/common/mke2img.sh $SRC $DST
#	fi
}

target_clean()
{
	system=$1
	for pkg in $(cat $DISTRO_DIR/configs/build.config)
	do
		if [ x$pkg != x`grep $pkg $DISTRO_CONFIG` ];then
			sudo chroot $system apt-get remove -y $pkg
		fi
	done

	sudo chroot $system apt-get autoclean -y
	sudo chroot $system apt-get clean -y
	sudo chroot $system apt-get autoremove -y
	sudo rm -rf $system/usr/share/locale/*
	sudo rm -rf $system/usr/share/man/*
	sudo rm -rf $system/usr/share/doc/*
	sudo rm -rf $system/usr/include/*
	sudo rm -rf $system/var/log/*
	sudo rm -rf $system/var/lib/apt/lists/*
	sudo rm -rf $system/var/cache/*
	echo "remove unused dri..."
	if [ $DISTRO_ARCH = arm64 ];then
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/msm_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/nouveau_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/nouveau_drv_video.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/nouveau_vieux_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/r200_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/r300_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/r600_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/r600_drv_video.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/radeon_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/radeonsi_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/radeonsi_drv_video.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/tegra_dri.so
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/dri/vc4_dri.so
	elif [ $DISTRO_ARCH = arm ];then
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/msm_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/nouveau_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/nouveau_drv_video.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/nouveau_vieux_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/r200_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/r300_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/r600_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/r600_drv_video.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/radeon_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/radeonsi_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/radeonsi_drv_video.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/tegra_dri.so
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/dri/vc4_dri.so
	fi
	echo "remove vdpau..."
	if [ $DISTRO_ARCH = arm64 ];then
		sudo rm -rf $system/usr/lib/aarch64-linux-gnu/vdpau
	elif [ $DISTRO_ARCH = arm ];then
		sudo rm -rf $system/usr/lib/arm-linux-gnueabihf/vdpau
	fi
	sudo rm -rf $system/sdk
}

pack()
{
	echo "packing rootfs image..."
#	rm -rf $ROOTFS_DIR
#	cp -ar $TARGET_DIR $ROOTFS_DIR
#	target_clean $ROOTFS_DIR
	if [ $RK_ROOTFS_TYPE = ext4 ];then
		pack_ext4 $TARGET_DIR $ROOTFS_EXT4
	elif [ $RK_ROOTFS_TYPE = squashfs ];then
		pack_squashfs $ROOTFS_DIR $ROOTFS_SQUASHFS
	fi
}

build_packages()
{
	for p in $(ls $DISTRO_DIR/package/);do
		[ -d $DISTRO_DIR/package/$p ] || continue
		local config=BR2_PACKAGE_$(echo $p|tr 'a-z-' 'A-Z_')
		local build=$(eval echo -n \$$config)
		#echo "Build $pkg($config)? ${build:-n}"
		[ x$build == xy ] && $SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE $p
	done
	echo "finish building all packages"
}

init()
{
	mkdir -p $OUTPUT_DIR $BUILD_DIR $TARGET_DIR $IMAGE_DIR $MOUNT_DIR $SYSROOT_DIR $TARGET_DIR/etc/apt/sources.list.d

	if [ -z $ARCH ];then
		export ARCH=arm64
	fi

	while read line1; do INSTALL_PKG="$INSTALL_PKG $line1"; done < "$OUTPUT_DIR/.install"
}

build_base()
{
	$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$INSTALL_PKG" "init"
}

build_all()
{
	init $1
	build_base
	build_packages
	$SCRIPTS_DIR/override_deb.sh
	run rsync -a --ignore-times --keep-dirlinks --chmod=u=rwX,go=rX --exclude .empty $OVERLAY_DIR/ $TARGET_DIR/
	pack
}

main()
{
	if [ x$1 == ximage ];then
		init
		pack
		exit 0
	elif [ x$1 == xbase ];then
		init
		build_base
		exit 0
	elif [ -z $1 ] || [ x$1 == xdefault ];then
		build_all $1
		exit 0
	else
		init
		p=$1
		if [ x"-rebuild" == x`echo ${p:0-8:8}` ];then
			p=`echo ${p%%-*}`
			rm -rf $BUILD_DIR/$p
		fi
		$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE $p
		exit 0
	fi
}

main "$@"
