#!/bin/sh

echo "Start RKNN_DEMO Camera Preview!"

sudo service lightdm stop
/usr/bin/npu_transfer_proxy&
rknn_demo
