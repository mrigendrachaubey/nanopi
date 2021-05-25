#!/bin/bash

set -e
DEPENDENCIES=alsa-utils
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE $DEPENDENCIES
install -m 0644 -D $TOP_DIR/external/alsa-config/cards/* $TARGET_DIR/usr/share/alsa/cards/
