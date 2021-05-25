#!/bin/sh

relpath_prefix() {
	target="${1#/}" ;
	MAX_DEPTH=4
	for depth in `seq 1 $MAX_DEPTH` ; do
		if [ -e ${target} ] ; then
			echo "$target" ;
			return ;
		fi
		target="../${target}" ;
	done ;

	# Failed to find target, keep it!
	echo "$1" ;
}

simplify_symlink() {
	if [ ! -d $1 ];then
		return
	else
		cd $1
	fi
	for link in $(find . -maxdepth 1 -type l); do
		target=$(readlink ${link}) ;
		if [ "${target}" = "${target#/}" ] ; then
			continue ;
		fi ;
		relpath="$(relpath_prefix ${target})" ;
		echo "Fixing symlink ${link} from ${target} to ${relpath}" ;
		ln -sf ${relpath} ${link} ;
	done ;
}

simplify_symlink $1
