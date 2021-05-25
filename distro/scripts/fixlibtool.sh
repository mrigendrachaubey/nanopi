#!/bin/bash

STAGING_DIR=$1
BASE_DIR=$2

f=`find $STAGING_DIR/usr/lib* -name "*.la"`
for la in $f;do
	sed -i "s:$BASE_DIR:@BASE_DIR@:g" $la
	sed -i "s:$STAGING_DIR:@STAGING_DIR@:g" $la
	sed -i "s:\(['= ]\)/usr:\\1@STAGING_DIR@/usr:g" $la
	sed -i "s:@STAGING_DIR@:$STAGING_DIR:g" $la
	sed -i "s:@BASE_DIR@:$BASE_DIR:g" $la
done
#find $STAGING_DIR/usr/lib* -name "*.la" | xargs sed "s:$BASE_DIR:@BASE_DIR@:g" -e "s:$STAGING_DIR:@STAGING_DIR@:g"
#-e "s:\(['= ]\)/usr:\\1@STAGING_DIR@/usr:g" \
#-e "s:@STAGING_DIR@:$STAGING_DIR:g" \
#-e "s:@BASE_DIR@:$BASE_DIR:g"
