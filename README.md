# Main hardware components:
===========================

ref: [http://wiki.friendlyarm.com/wiki/index.php/NanoPi_M4V2#Hardware_Spec](URL)

**GPU** : Mali-T864 GPU，supports OpenGL ES1.1/2.0/3.0/3.1, OpenCL, DX11, and AFBC

**CPU** : big.LITTLE，Dual-Core Cortex-A72(up to 2.0GHz) + Quad-Core Cortex-A53(up to 1.5GHz)

**VPU**(Vision Processing Unit): 4K VP9 and 4K 10bits H265/H264 60fps decoding, Dual VOP, etc

**RAM**: Dual-Channel 4GB LPDDR4

**STORAGE**:
		**EMMC**: no Onboard eMMC, but has a eMMC socket
		**SD**: microSD Slot x 1

**USB**:
		four USB 3.0 Type-A ports
		USB Type-C, Supports USB2.0 OTG and Power input

**HDMI**: HDMI 2.0a, supports 4K@60Hz，HDCP 1.4/2.2

**Ethernet**: Native Gigabit Ethernet

**Wi-Fi/BT**: 802.11a/b/g/n/ac, Bluetooth 4.1, Wi-Fi and Bluetooth combo module, 2x2 MIMO, dual antenna interface

**Buttons**：one PowerKey, one Recovery Key

**Debug**: one Debug UART, 4 Pin 2.54mm header, 3V level, 1500000bps

40Pin GPIO Extension ports:
	3 X 3V/1.8V I2C, up to 1 x 3V UART, 1 X 3V SPI, 1 x SPDIF_TX, up to 8 x 3V GPIOs
	1 x 1.8V 8 channels I2S 
24Pin Extension ports:
	2 independent native USB 2.0 Host
	PCIe x2
   PWM x1, PowerKey

**Others**:

**PMU**: RK808-D PMIC, cooperated with independent DC/DC, enabling DVFS, software power-down, RTC wake-up, system sleep mode

**Audio Out**: 3.5mm Dual channel headphone jack, or HDMI

**Audio In**: one microphone input interface

**LED**: 1 x power LED and 1 x GPIO Controlled LED

**RTC Battery**: 2 Pin 1.27/1.25mm RTC battery input connector

Power supply: DC 5V/3A

**PCB**: 8 Layer, 85 mm x 56 mm

Ambient Operating Temperature: -20℃ to 70℃


# Understand Android BSP
===========================

**Build system and its scripts**:
-----------------------------
cloning:
Android 8.1 source:

> git clone https://gitlab.com/friendlyelec/rk3399-android-8.1 --depth 1 -b master

Once code is cloned build,

> cd rk3399-android-8.1

> ./build-nanopc-t4.sh -F -M

Now get full git history

> git fetch origin master --unshallow

After this we have to get fuse scripts

> git clone https://github.com/friendlyarm/sd-fuse_rk3399


**Flashing emmc and sd card**:
--------------------------
[https://github.com/friendlyarm/sd-fuse_rk3399/blob/master/README.md
](URL)

**Build an sdcard-to-emmc image (eflasher rom)**

Enable exFAT file system support on Ubuntu:

> sudo apt-get install exfat-fuse exfat-utils

Generate the eflasher raw image

> cd sd-fuse_rk3399

> wget http://112.124.9.243/dvdfiles/RK3399/images-for-eflasher/emmc-flasher-images.tgz

> tar xzf emmc-flasher-images.tgz

Now get a prebuilt android emmc eflasher image,

> wget http://112.124.9.243/dvdfiles/RK3399/images-for-eflasher/android-oreo-images.tgz

> tar xzf android-oreo-images.tgz

> sudo ./mk-emmc-image.sh android8

In out folder we get full image,

> out/rk3399-eflasher-android8-20200314.img

Now flash this on a SD card. Use below command for progress,

> (sudo pv -n rk3399-eflasher-android8-20200314.img | sudo dd of=/dev/sdd bs=1M ) 2>&1 | dialog --gauge "Running dd command (cloning), please wait..." 10 70 0

Once flashing is 100%, observe sync and dirty pages

> sync & watch -d grep -e Dirty: -e Writeback: /proc/meminfo

Login onto your board via a serial terminal and type "eflasher" to proceed

> Login: root
> Password: fa

Now copy your android build that are inside **rk3399-android-8.1/rockdev/Image-nanopc_t4** folder and copy it in eflasher SD card. Insert the SD card in device, and boot the device.
Run eflasher and now your emmc is updated with your android's source images.


**Important BSP source files:**
---------------------------

**Android build:**

build/make/core/build_id.mk

build/make/core/version_defaults.mk

vendor/rockchip/common/gpu/Android.mk

vendor/rockchip/common/gpu/MaliT860.mk


**uboot:**

u-boot/board/rockchip/rk33xx/rk33xx.c

u-boot/common/cmd_bootrk.c

PinMuxing:
----------




Images and partitions:
----------------------
