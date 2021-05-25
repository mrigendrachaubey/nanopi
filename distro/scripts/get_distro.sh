#!/bin/bash

set -e
SUITE=$1
OS=unknown
if [[ $SUITE = buster ]] || [[ $SUITE = stretch ]] || [[ $SUITE = sid ]] || [[ $SUITE = testing ]];then
        OS=debian
elif [[ $SUITE = bionic ]] || [[ $SUITE = xenial ]] || [[ $SUITE = trusty ]];then
        OS=ubuntu
fi

echo "$OS"

