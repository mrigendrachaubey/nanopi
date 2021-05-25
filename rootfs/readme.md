## Introduction
A set of shell scripts that will build GNU/Linux distribution rootfs image
for rockchip platform.

## Available Distro
* Debian Stretch (X11)
* ~~Debian Buster (Wayland)~~

## Usage for 32bit Debian
Building a base debian system by ubuntu-build-service from linaro.

	sudo apt-get install binfmt-support qemu-user-static
	sudo dpkg -i ubuntu-build-service/packages/*
	sudo apt-get install -f
	RELEASE=stretch TARGET=desktop ARCH=armhf ./mk-base-debian.sh

Building the rk-debian rootfs:

	RELEASE=stretch ARCH=armhf ./mk-rootfs.sh

Building the rk-debain rootfs with debug:

	VERSION=debug ARCH=armhf ./mk-rootfs-stretch.sh

Creating the ext4 image(linaro-rootfs.img):

	./mk-image.sh
---

## Usage for 64bit Debian
Building a base debian system by ubuntu-build-service from linaro.

	sudo apt-get install binfmt-support qemu-user-static
	sudo dpkg -i ubuntu-build-service/packages/*
	sudo apt-get install -f
	RELEASE=stretch TARGET=desktop ARCH=arm64 ./mk-base-debian.sh

Building the rk-debian rootfs:

	RELEASE=stretch ARCH=arm64 ./mk-rootfs.sh

Building the rk-debain rootfs with debug:

	VERSION=debug ARCH=arm64 ./mk-rootfs-stretch-arm64.sh

Creating the ext4 image(linaro-rootfs.img):

	./mk-image.sh
---

## Cross Compile for ARM Debian

[Docker + Multiarch](http://opensource.rock-chips.com/wiki_Cross_Compile#Docker)

## Package Code Base

Please apply [those patches](https://github.com/rockchip-linux/rk-rootfs-build/tree/master/packages-patches) to release code base before rebuilding!

## FAQ

1. noexec or nodev issue
/usr/share/debootstrap/functions: line 1450: ..../rootfs/ubuntu-build-service/stretch-desktop-armhf/chroot/test-dev-null: Permission denied
E: Cannot install into target '/home/foxluo/work3/rockchip/rk_linux/rk3399_linux/rootfs/ubuntu-build-service/stretch-desktop-armhf/chroot' mounted with noexec or nodev

Solution: mount -o remount,exec,dev xxx (xxx is the mount place), then rebuild it.
