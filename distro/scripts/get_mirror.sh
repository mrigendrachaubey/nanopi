#!/bin/bash

DEFAULT_DEBIAN_MIRROR1=http://ftp.cn.debian.org/debian
DEFAULT_DEBIAN_MIRROR2=http://mirror.xtom.com.hk/debian
DEFAULT_DEBIAN_MIRROR3=http://mirrors.ustc.edu.cn/debian
DEFAULT_DEBIAN_MIRROR4=http://cdn.debian.net/debian
DEFAULT_DEBIAN_MIRROR=$DEFAULT_DEBIAN_MIRROR3
DEFAULT_UBUNTU_MIRROR=http://mirrors.ustc.edu.cn/ubuntu
NETSELECT_APT_DEB=netselect-apt_0.3.ds1-28_all.deb
NETSELECT_APT_URL=http://ftp.cn.debian.org/debian/pool/main/n/netselect/$NETSELECT_APT_DEB
NETSELECT_DEB=netselect_0.3.ds1-28+b1_amd64.deb
NETSELECT_URL=http://ftp.cn.debian.org/debian/pool/main/n/netselect/$NETSELECT_DEB

OS=`$SCRIPTS_DIR/get_distro.sh $1`
ARCH=$2

if [ x$3 = xdefault ];then
	if [ $OS = debian ];then
		echo "$DEFAULT_DEBIAN_MIRROR"
		exit 0
	elif [ $OS = ubuntu ];then
		echo "$DEFAULT_UBUNTU_MIRROR"
		exit 0
	fi
fi

usage() {
        echo "Usage: $0: distro arch" >&2
}

netselect_chkinstall()
{
	if [ -z `which netselect` ];then
		wget -P /tmp $NETSELECT_URL
		sudo dpkg -i /tmp/$NETSELECT_DEB
	fi
}

netselect_apt_chkinstall()
{
	if [ -z `which netselect-apt` ];then
		wget -P /tmp $NETSELECT_APT_URL
		sudo dpkg -i /tmp/$NETSELECT_APT_DEB
	fi
}

main()
{
	if [ x$OS = xdebian ];then
		sudo touch /tmp/.test
		if [ $? == 1 ];then
			echo "$DEFAULT_DEBIAN_MIRROR"
			exit 0
		else
			sudo rm -f /tmp/.test
		fi
		netselect_chkinstall
		netselect_apt_chkinstall
		MIRROR=`sudo netselect-apt -o /tmp/.sources.list -t 20 -a $ARCH 2>&1 |grep -A 1 "fastest valid for HTTP" | tail -1 | cut -d ' ' -f 9`
		sudo rm -f /tmp/.sources.list
	elif [ x$OS = xubuntu ];then
		sudo touch /tmp/.test
		if [ $? == 1 ];then
			echo "$DEFAULT_UBUNTU_MIRROR"
			exit 0
		else
			sudo rm -f /tmp/.test
		fi
		netselect_chkinstall
		MIRROR=`sudo netselect -s 1 $(wget -qO - mirrors.ubuntu.com/mirrors.txt) | cut -d ' ' -f 5`
	else
		usage
	fi
	echo "$MIRROR"
}

main "$@"
