#!/bin/bash

log() {
    local format="$1"
    shift
    printf -- "$format\n" "$@" >&2
}

run() {
    log "I: Running command: %s" "$*"
    "$@"
}

CONFIGS_DIR=$1
DEFCONFIG=$2
RK=$3
if [ $DEFCONFIG ] && [ -e $CONFIGS_DIR/$DEFCONFIG ];then
	for f in $(cat $CONFIGS_DIR/$DEFCONFIG); do
		if [ x$RK == "xrk" ];then
			if [ `echo "$f" | cut -d "." -f2` == rk ];then
				while read line; do PACKAGES="$PACKAGES $line"; done < "$CONFIGS_DIR/$f"
			fi
		else
			if [ `echo "$f" | cut -d "." -f2` != rk ];then
				while read line; do PACKAGES="$PACKAGES $line"; done < "$CONFIGS_DIR/$f"
			fi
		fi
	done
	echo "$PACKAGES"
fi


