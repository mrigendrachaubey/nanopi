#!/bin/bash -e

if [ ! $RELEASE ]; then
	RELEASE='stretch'
fi

if [ ! $ARCH ]; then
	ARCH='armhf'
fi

if [ ! $TARGET ]; then
	TARGET='desktop'
fi

if [ -e linaro-$RELEASE-alip-*.tar.gz ]; then
	rm linaro-$RELEASE-alip-*.tar.gz
fi

cd ubuntu-build-service/$RELEASE-$TARGET-$ARCH

echo -e "\033[36m Staring Download...... \033[0m"

make clean

./configure

make

if [ -e linaro-$RELEASE-alip-*.tar.gz ]; then
	sudo chmod 0666 linaro-$RELEASE-alip-*.tar.gz
	mv linaro-$RELEASE-alip-*.tar.gz ../../
else
	echo -e "\e[31m Failed to run livebuild, please check your network connection. \e[0m"
fi
