#!/bin/bash

parse_files() {
	while [ -n "$1" ]; do
		path=`echo -n $1 | sed 's/^.*\(linux-.*\.h\).*$/\1/'`
		if [ ! $1 = $path ]; then
			echo $path >> $tmp2
		fi
		shift
	done
}

tmp1=`mktemp`
tmp2=`mktemp`
cd userspace
make dep DEP=$tmp1 1>&2 || exit 1
cat $tmp1 | while read line; do
	parse_files $line
done
sort < $tmp2 | uniq
rm -f $tmp1
rm -f $tmp2
