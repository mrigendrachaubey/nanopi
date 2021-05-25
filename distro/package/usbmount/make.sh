#!/bin/bash

PACKAGE=usbmount
METHOD=$1
if [ x$METHOD = xcross ];then
	exit 0
else
	set -e
	/sdk/distro/scripts/install.sh $PACKAGE
	install -m 0644 /sdk/distro/package/usbmount/usbmount.conf /etc/usbmount/usbmount.conf
	ln -sf media/usb udisk
fi

