#!/bin/bash

DEFCONFIG=$1
OUTPUT=$2
INC_DIR=$(dirname $DEFCONFIG)/rockchip

cat /dev/null > $OUTPUT

FILES=$(grep -io -e '[a-z]\+.install' $DEFCONFIG) || exit 0
for f in $FILES; do
  [ -f ${INC_DIR}/$f ] && cat ${INC_DIR}/$f >> $OUTPUT
done
