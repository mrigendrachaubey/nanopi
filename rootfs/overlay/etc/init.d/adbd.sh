#!/bin/bash -e
### BEGIN INIT INFO
# Provides:          adbd
# Required-Start:
# Required-Stop:
# Default-Start: S
# Default-Stop: 6
# Short-Description:
# Description:       Linux ADB
### END INIT INFO

# setup configfs for adbd, usb mass storage and MTP....

UMS_EN=off
ADB_EN=off
MTP_EN=off

make_config_string()
{
	tmp=$CONFIG_STRING
	if [ -n "$CONFIG_STRING" ]; then
		CONFIG_STRING=${tmp}_${1}
	else
		CONFIG_STRING=$1
	fi
}

parameter_init()
{
	while read line
	do
		case "$line" in
			usb_mtp_en)
				MTP_EN=on
				make_config_string mtp
				;;
			usb_adb_en)
				ADB_EN=on
				make_config_string adb
				;;
			usb_ums_en)
				UMS_EN=on
				make_config_string ums
				;;
		esac
	done < $DIR/.usb_config


	case "$CONFIG_STRING" in
		ums)
			PID=0x0000
			;;
		mtp)
			PID=0x0001
			;;
		adb)
			PID=0x0006
			;;
		mtp_adb | adb_mtp)
			PID=0x0011
			;;
		ums_adb | adb_ums)
			PID=0x0018
			;;
		*)
			PID=0x0019
	esac
}

configfs_init()
{
	mkdir -p /sys/kernel/config/usb_gadget/rockchip -m 0770
	echo 0x2207 > /sys/kernel/config/usb_gadget/rockchip/idVendor
	echo $PID > /sys/kernel/config/usb_gadget/rockchip/idProduct
	mkdir -p /sys/kernel/config/usb_gadget/rockchip/strings/0x409 -m 0770
	echo "0123456789ABCDEF" > /sys/kernel/config/usb_gadget/rockchip/strings/0x409/serialnumber
	echo "rockchip"  > /sys/kernel/config/usb_gadget/rockchip/strings/0x409/manufacturer
	echo "rk3xxx"  > /sys/kernel/config/usb_gadget/rockchip/strings/0x409/product
	mkdir -p /sys/kernel/config/usb_gadget/rockchip/configs/b.1 -m 0770
	mkdir -p /sys/kernel/config/usb_gadget/rockchip/configs/b.1/strings/0x409 -m 0770
	echo 500 > /sys/kernel/config/usb_gadget/rockchip/configs/b.1/MaxPower
	echo \"$CONFIG_STRING\" > /sys/kernel/config/usb_gadget/rockchip/configs/b.1/strings/0x409/configuration
}

function_init()
{
	if [ $UMS_EN = on ];then
		if [ ! -e "/sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0" ] ;
		then
			mkdir -p /sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0
			echo /dev/disk/by-partlabel/userdata > /sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0/lun.0/file
			ln -s /sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0 /sys/kernel/config/usb_gadget/rockchip/configs/b.1/mass_storage.0
		fi
	fi

	if [ $ADB_EN = on ];then
		if [ ! -e "/sys/kernel/config/usb_gadget/rockchip/functions/ffs.adb" ] ;
		then
			mkdir -p /sys/kernel/config/usb_gadget/rockchip/functions/ffs.adb
			ln -s /sys/kernel/config/usb_gadget/rockchip/functions/ffs.adb /sys/kernel/config/usb_gadget/rockchip/configs/b.1/ffs.adb
		fi
	fi

	if [ $MTP_EN = on ];then
		if [ ! -e "mkdir -p /sys/kernel/config/usb_gadget/rockchip/functions/mtp.gs0" ] ;
		then
			mkdir -p /sys/kernel/config/usb_gadget/rockchip/functions/mtp.gs0
			ln -s /sys/kernel/config/usb_gadget/rockchip/functions/mtp.gs0 /sys/kernel/config/usb_gadget/rockchip/configs/b.1/mtp.gs0
		fi
	fi
}

case "$1" in
start)
	DIR=$(cd `dirname $0`; pwd)
	if [ ! -e "$DIR/.usb_config" ]; then
		echo "$0: Cannot find .usb_config"
		exit 0
	fi

	parameter_init
	if [ -z $CONFIG_STRING ]; then
		echo "$0: no function be selected"
		exit 0
	fi
	configfs_init
	function_init

	if [ $ADB_EN = on ];then
		if [ ! -e "/dev/usb-ffs/adb" ] ;
		then
			mkdir -p /dev/usb-ffs/adb
			mount -o uid=2000,gid=2000 -t functionfs adb /dev/usb-ffs/adb
		fi
		export service_adb_tcp_port=5555
		start-stop-daemon --start --oknodo --pidfile /var/run/adbd.pid --startas /usr/local/bin/adbd --background
		sleep 1
	fi

	if [ $MTP_EN = on ];then
		if [ $MTP_EN = on ]; then
			mtp-server&
		else
			sleep 1 && mtp-server&
		fi
	fi

	UDC=`ls /sys/class/udc/| awk '{print $1}'`
	echo $UDC > /sys/kernel/config/usb_gadget/rockchip/UDC
	;;
stop)
	echo "none" > /sys/kernel/config/usb_gadget/rockchip/UDC
	if [ $ADB_EN = on ];then
		start-stop-daemon --stop --oknodo --pidfile /var/run/adbd.pid --retry 5
	fi
	;;
restart|reload)
	;;
*)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit 0
