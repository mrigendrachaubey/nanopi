#!/bin/bash -e

# Directory contains the target rootfs
TARGET_ROOTFS_DIR="binary"

if [ ! $ARCH ]; then  
	ARCH='armhf'  
fi 
if [ ! $VERSION ]; then  
	VERSION="debug"
fi 

if [ ! -e linaro-buster-alip-*.tar.gz ]; then 
	echo "\033[36m Run mk-base-debian.sh first \033[0m"
fi

finish() {
	sudo umount $TARGET_ROOTFS_DIR/dev
	exit -1
}
trap finish ERR

echo -e "\033[36m Extract image \033[0m"
#sudo tar -xpf linaro-buster-alip-*.tar.gz
sudo tar -xpf linaro-stretch-alip-*.tar.gz

echo -e "\033[36m Copy overlay to rootfs \033[0m"
sudo mkdir -p $TARGET_ROOTFS_DIR/packages
sudo cp -rf packages/$ARCH/* $TARGET_ROOTFS_DIR/packages

# some configs
sudo cp -rf overlay/* $TARGET_ROOTFS_DIR/
if [ "$ARCH" == "armhf"  ]; then
    sudo cp overlay-firmware/usr/bin/brcm_patchram_plus1_32 $TARGET_ROOTFS_DIR/usr/bin/brcm_patchram_plus1
    sudo cp overlay-firmware/usr/bin/rk_wifi_init_32 $TARGET_ROOTFS_DIR/usr/bin/rk_wifi_init
fi

# bt,wifi,audio firmware
sudo mkdir -p $TARGET_ROOTFS_DIR/system/lib/modules/
sudo find ../kernel/drivers/net/wireless/rockchip_wlan/*  -name "*.ko" | \
    xargs -n1 -i sudo cp {} $TARGET_ROOTFS_DIR/system/lib/modules/

sudo cp -rf overlay-firmware/* $TARGET_ROOTFS_DIR/

# adb
if [ "$ARCH" == "armhf" ]; then
sudo cp -rf overlay-debug/usr/local/share/adb/adbd-32 $TARGET_ROOTFS_DIR/usr/local/bin/adbd
fi

# glmark2
if [ "$ARCH" == "armhf" ]; then
sudo rm -rf $TARGET_ROOTFS_DIR/usr/local/share/glmark2
sudo mkdir -p $TARGET_ROOTFS_DIR/usr/local/share/glmark2
sudo cp -rf overlay-debug/usr/local/share/glmark2/armhf/share/* $TARGET_ROOTFS_DIR/usr/local/share/glmark2
sudo cp overlay-debug/usr/local/share/glmark2/armhf/bin/glmark2-es2 $TARGET_ROOTFS_DIR/usr/local/bin/glmark2-es2
fi

if [ "$VERSION" == "debug" ] || [ "$VERSION" == "jenkins" ]; then
	# adb, video, camera  test file
	sudo cp -rf overlay-debug/* $TARGET_ROOTFS_DIR/
fi

if  [ "$VERSION" == "jenkins" ] ; then
	# network
	sudo cp -b /etc/resolv.conf  $TARGET_ROOTFS_DIR/etc/resolv.conf
fi

echo -e "\033[36m Change root.....................\033[0m"
sudo cp /usr/bin/qemu-arm-static $TARGET_ROOTFS_DIR/usr/bin/
sudo mount -o bind /dev $TARGET_ROOTFS_DIR/dev

cat << EOF | sudo chroot $TARGET_ROOTFS_DIR

apt-get update

apt-get install -y lxpolkit
apt-get install -y blueman
echo exit 101 > /usr/sbin/policy-rc.d
chmod +x /usr/sbin/policy-rc.d
apt-get install -y blueman
rm -f /usr/sbin/policy-rc.d

apt-get install -y systemd-sysv vi

#---------------ForwardPort Linaro overlay -------------- 
apt-get install -y e2fsprogs
wget http://repo.linaro.org/ubuntu/linaro-overlay/pool/main/l/linaro-overlay/linaro-overlay-minimal_1112.10_all.deb
wget http://repo.linaro.org/ubuntu/linaro-overlay/pool/main/9/96boards-tools/96boards-tools-common_0.9_all.deb
dpkg -i *.deb
rm -rf *.deb
apt-get install -f -y

#---------------conflict workaround --------------
apt-get remove -y xserver-xorg-input-evdev

apt-get install -y libxfont1 libinput-bin libinput10 libwacom-common libwacom2 libunwind8 xserver-xorg-input-libinput

#---------------Xserver--------------
echo -e "\033[36m Setup Xserver.................... \033[0m"
dpkg -i  /packages/xserver/*
apt-get install -f -y

#---------------Video--------------
echo -e "\033[36m Setup Video.................... \033[0m"
apt-get install -y gstreamer1.0-plugins-base gstreamer1.0-tools gstreamer1.0-alsa \
	gstreamer1.0-plugins-good  gstreamer1.0-plugins-bad alsa-utils

dpkg -i  /packages/video/mpp/librockchip-mpp1_*_armhf.deb
dpkg -i  /packages/video/mpp/librockchip-mpp-dev_*_armhf.deb
dpkg -i  /packages/video/mpp/librockchip-vpu0_*_armhf.deb
dpkg -i  /packages/video/gstreamer/*.deb
apt-get install -f -y

#---------------Qt-Video--------------
dpkg -l | grep lxde
if [ "$?" -eq 0 ]; then
	# if target is base, we won't install qt
	apt-get install  -y libqt5opengl5 libqt5qml5 libqt5quick5 libqt5widgets5 libqt5gui5 libqt5core5a qml-module-qtquick2 \
		libqt5multimedia5 libqt5multimedia5-plugins libqt5multimediaquick-p5
	dpkg -i  /packages/video/qt/*
	apt-get install -f -y
else
	echo "won't install qt"
fi


#---------------TODO: USE DEB-------------- 
#---------------Setup Graphics-------------- 
apt-get install -y weston
cd /usr/lib/arm-linux-gnueabihf
wget https://github.com/rockchip-linux/libmali/blob/29mirror/lib/arm-linux-gnueabihf/libmali-bifrost-g31-rxp0-wayland-gbm.so
ln -s libmali-bifrost-g31-rxp0-wayland-gbm.so libmali-bifrost-g31-rxp0-wayland-gbm.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libEGL.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libEGL.so.1
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libEGL.so.1.0.0
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libGLESv2.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libGLESv2.so.2
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libGLESv2.so.2.0.0
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libMaliOpenCL.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libOpenCL.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libgbm.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libgbm.so.1
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libgbm.so.1.0.0
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libwayland-egl.so
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libwayland-egl.so.1
ln -sf libmali-bifrost-g31-rxp0-wayland-gbm.so libwayland-egl.so.1.0.0
cd /


#---------------Custom Script-------------- 
systemctl mask systemd-networkd-wait-online.service
systemctl mask NetworkManager-wait-online.service
rm /lib/systemd/system/wpa_supplicant@.service

#---------------Clean-------------- 
rm -rf /var/lib/apt/lists/*

EOF

sudo umount $TARGET_ROOTFS_DIR/dev
