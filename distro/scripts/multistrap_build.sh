#!/bin/bash

usage() {
	echo "Usage: $0: [-e] [-f] [-n] [-v] [-a arch] [-b seed] [-c conf] [-d directory] [-h hook] [-m mirror] [-p packages] [-s suite] [-u auth]" >&2
}

log() {
    local format="$1"
    shift
    printf -- "$format\n" "$@" >&2
}

run() {
    log "I: Running command: %s" "$*"
    "$@"
}

export DEBIAN_FRONTEND=noninteractive DEBCONF_NONINTERACTIVE_SEEN=true LC_ALL=C LANGUAGE=C LANG=C
export PATH=$PATH:/usr/sbin:/sbin

#if [ "$FAKEROOTKEY" = "" ]; then
#	echo "I: re-executing script inside fakeroot"
#	fakeroot "$0" "$@";
#	exit
#fi

FORCE=""
MSTRAP_SIM=
EXIT_ON_ERROR=true
while getopts efva:b:c:d:m:n:p:s:u: opt; do
	case $opt in
	a) ARCH="$OPTARG";;
	b) SEED="$OPTARG";;
	c) CONF="$OPTARG";;
	d) ROOTDIR="$OPTARG";;
	e) EXIT_ON_ERROR=false;;
	f) FORCE=true;;
	h) HOOK="$OPTARG";;
	m) MIRROR="$OPTARG";;
	n) MSTRAP_SIM="--simulate";;
	p) PACKAGES="$OPTARG";;
	s) SUITE="$OPTARG";;
	u) NOAUTH="$OPTARG";;
	v) set -x;;
	?) usage; exit 1;;
	esac
done
shift $(($OPTIND - 1))
#[ "$#" -lt 6 ] && { echo "wrong arguments" >&2; usage; exit 1; }

[ "$EXIT_ON_ERROR" = true ] && set -e

export QEMU_LD_PREFIX="`readlink -m "$ROOTDIR"`"
#export FAKECHROOT_CMD_SUBST=/usr/bin/ldd=/usr/bin/ldd.fakechroot:/sbin/ldconfig=/bin/true

if [ $ARCH == arm64 ];then
	QEMU_ARCH=aarch64
elif [ $ARCH == arm ];then
	QEMU_ARCH=arm
else
	echo "$ARCH is not a valid arch. we only support arm and arm64! set to arm64"
	QEMU_ARCH=aarch64
fi
QEMU=qemu-$QEMU_ARCH-static
CHROOTQEMUCMD="proot -q $QEMU -v -1 -0 -b /dev -b /sys -b /proc -r"
CHROOTCMD="proot -v -1 -0 -r"

echo "I: ------------------------------------------------------------"
echo "I: suite:   $SUITE"
echo "I: arch:    $ARCH"
echo "I: qemu:    $QEMU"
echo "I: conf:    $CONF"
echo "I: seed:    $SEED"
echo "I: rootdir: $ROOTDIR"
echo "I: mirror:  $MIRROR"
echo "I: pkgs:    $PACKAGES"
echo "I: auth:    $NOAUTH"
echo "I: ------------------------------------------------------------"

[ -e "$ROOTDIR.tar" ] && [ ! "$FORCE" = true ] && { echo "tarball $ROOTDIR.tar still exists" >&2; exit 1; }

# create multistrap.conf
echo "I: create multistrap.conf"
MULTISTRAPCONF=`tempfile -d /tmp -p multistrap`
echo -n > "$MULTISTRAPCONF"
while read line; do
	eval echo $line >> "$MULTISTRAPCONF"
done < $CONF
echo "I: create $MULTISTRAPCONF done"
# download and extract packages
echo "I: run multistrap" >&2
run proot -0 multistrap $NOAUTH $MSTRAP_SIM -f "$MULTISTRAPCONF"
[ -z "$MSTRAP_SIM" ] || exit 0
# preseed debconf
echo "I: preseed debconf"
if [ -r "$SEED" ]; then
	cp "$SEED" $ROOTDIR/tmp/
	run $CHROOTQEMUCMD $ROOTDIR debconf-set-selections /tmp/debconfseed.txt
#	rm $ROOTDIR/tmp/debconfseed.txt
fi

# run preinst scripts
echo "I: run preinst scripts"
for script in $ROOTDIR/var/lib/dpkg/info/*.preinst; do
	[ "$script" = "$ROOTDIR/var/lib/dpkg/info/vpnc.preinst" ] && continue
	echo "I: run preinst script ${script##$ROOTDIR}"
	DPKG_MAINTSCRIPT_NAME=preinst \
	DPKG_MAINTSCRIPT_PACKAGE="`basename $script .preinst`" \
	$CHROOTQEMUCMD $ROOTDIR ${script##$ROOTDIR} install
done

# run dpkg --configure -a twice because of errors during the first run
echo "I: configure packages"
$CHROOTQEMUCMD $ROOTDIR /usr/bin/dpkg --configure -a
#|| $CHROOTCMD $ROOTDIR /usr/bin/dpkg --configure -a

# source hooks
echo "I: run hooks"
if [ -r "$HOOK" ]; then
	for f in $HOOK/*; do
		echo "I: run hook $f"
		. $f
	done
fi

#cleanup
#echo "I: cleanup"
#rm $ROOTDIR/usr/sbin/policy-rc.d

# need to generate tar inside fakechroot so that absolute symlinks are correct
# tar is clever enough to not try and put the archive inside itself
#TARBALL=$(basename $ROOTDIR).tar
#echo "I: create tarball $TARBALL"
#$CHROOTCMD $ROOTDIR tar -cf $TARBALL -C / .
#mv $ROOTDIR/$TARBALL .
