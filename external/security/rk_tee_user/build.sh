#/bin/bash

PWD=`pwd`

export TA_DEV_KIT_DIR=$PWD/export-user_ta

export MAKE=make
export CROSS_COMPILE=arm-none-eabi-
export CROSS_COMPILE_HOST=arm-none-eabi-
export CROSS_COMPILE_TA=arm-none-eabi-
export CROSS_COMPILE_user_ta=arm-none-eabi-

rm -rf out/

make V=1  -j4

