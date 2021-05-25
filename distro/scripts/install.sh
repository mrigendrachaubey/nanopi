#!/bin/bash

pkg_name=$1
installed=`dpkg -s $pkg_name  | grep Status`
if [ "$installed" == "Status: install ok installed" ];then
	echo "$pkg_name already installed"
else
        apt-get install -y $pkg_name
        if [ $? -ne 0 ]; then
                exit 1
        fi
fi
