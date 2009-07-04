#!/bin/bash

if [ -z "$LINUX_DIR" ]; then
	dir=`dirname $0`
	LINUX_DIR=`readlink -f $dir`/linux-2.6
fi

makefile=$LINUX_DIR/Makefile

if [ ! -f $makefile ]; then
	exit 1
fi

version=`grep '^VERSION' $makefile | sed 's/^.*= *\([0-9]\+\).*/\1/'`
patchlevel=`grep '^PATCHLEVEL' $makefile | sed 's/^.*= *\([0-9]\+\).*/\1/'`
sublevel=`grep '^SUBLEVEL' $makefile | sed 's/^.*= *\([0-9]\+\).*/\1/'`

echo $version.$patchlevel.$sublevel
