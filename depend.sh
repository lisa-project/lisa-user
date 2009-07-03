#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $0 <output_path>"
	exit 1
fi
out=`readlink -f $1`
if [ ! -d $out ]; then
	echo "Error: output path $out does not exist"
	exit 2
fi

cd userspace
make dep || exit 1

tmp1=`mktemp`
tmp2=`mktemp`

while read line; do
	first=1
	for token in $line; do
		if [ $first = 1 ]; then
			first=0
			continue
		fi
		echo $token >> $tmp1
	done
done < lisa.dep

cat $tmp1 | sort | uniq > $tmp2
rm -f $tmp1

while read token; do
	dir=`dirname $token`
	if [ "$dir" = "." ]; then
		continue
	fi
	dir=`cd $dir 2>/dev/null && pwd`
	if [ -z "$dir" ]; then
		continue
	fi
	token=$dir/`basename $token`
	target=`echo -n $token | sed 's|.*/\(include/linux/.*\)$|linux-2.6/\1|'`
	if [ "$token" = "$target" ]; then
		continue;
	fi
	target=$out/$target
	dstdir=`dirname $target`
	if [ ! -d $dstdir ]; then
		echo mkdir -p $dstdir
		mkdir -p $dstdir
	fi
	echo cp $token $target
	cp $token $target
done < $tmp2

rm -f $tmp2
