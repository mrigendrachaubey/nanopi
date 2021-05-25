#!/bin/bash

set -e
ARCH=$1
SUITE=$2
PKGS=$3
INIT=$4

log() {
    local format="$1"
    shift
    printf -- "$format\n" "$@" >&2
}

run() {
    log "I: Running command: %s" "$*"
    "$@"
}

OS=`$SCRIPTS_DIR/get_distro.sh $SUITE`

for p in $PKGS;do
	if [ -x $PACKAGE_DIR/$p/make.sh ];then
		continue
	fi

	if [ -e $BUILD_DIR/$p/.timestamp ];then
		if [ -z `find $BUILD_DIR/$p -newer $BUILD_DIR/$p/.timestamp` ];then
			echo "$p installed before, skiped"
			continue
		fi
	fi

	if [ ! -d $BUILD_DIR/$p ];then
		mkdir -p $BUILD_DIR/$p
	fi

	install="$install $p"
done

if [ ! -z "$install" ];then
	install=eval echo $install
	if [ $OS == debian ];then
		$SCRIPTS_DIR/install_debian_pkg.sh $RK_ARCH $SUITE "$install" $INIT
	elif [ $OS == ubuntu ];then
		$SCRIPTS_DIR/install_ubuntu_pkg.sh $RK_ARCH $SUITE "$install" $INIT
	fi

	for p in $install;do
		touch $BUILD_DIR/$p/.timestamp
	done
fi


for p in $PKGS;do
	if [ -x $PACKAGE_DIR/$p/make.sh ];then
		mkdir -p $BUILD_DIR/$p
		if [ -e $BUILD_DIR/$p/.timestamp ];then
			if [ -z `find $BUILD_DIR/$p -newer $BUILD_DIR/$p/.timestamp` ];then
				echo "$p has been built before, skiped"
				continue
			fi
		fi
		echo "building package $p"
		run $PACKAGE_DIR/$p/make.sh
		echo "install $p done!!!"
		touch $BUILD_DIR/$p/.timestamp
	fi
done


