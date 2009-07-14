#!/bin/bash

target="i686"

major=`grep '^+LISA_KAPI_MAJOR' ../SOURCES/linux-2.6-lisa.patch | sed 's/^.*= *\([0-9]\+\).*/\1/'`
[ -z "$major" ] && exit 1
minor=`grep '^+LISA_KAPI_MINOR' ../SOURCES/linux-2.6-lisa.patch | sed 's/^.*= *\([0-9]\+\).*/\1/'`
[ -z "$minor" ] && exit 1
patch=`grep '^+LISA_KAPI_PATCH' ../SOURCES/linux-2.6-lisa.patch | sed 's/^.*= *\([0-9]\+\).*/\1/'`
[ -z "$patch" ] && exit 1


rpmbuild \
	--define "lisa_kapi_major $major" \
	--define "lisa_kapi_minor $minor" \
	--define "lisa_kapi_patch $patch" \
	-ba --target $target $* kernel.spec
