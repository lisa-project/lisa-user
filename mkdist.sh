#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $0 <dest_dir>"
	exit 1
fi

DST="$1"

if [ "$DST" = "/" ]; then
	echo "DST is / and you definitely DON'T WANT THAT! Aborting."
	exit 1
fi

if [ ! -d "$DST" ]; then
	echo "$DST does not exist or is not a directory"
	exit 1
fi

echo -n "Deleting existing fs "
rm -rf "$DST/bin" && echo -n "#"
rm -rf "$DST/etc" && echo -n "#"
rm -rf "$DST/flash" && echo -n "#"
rm -rf "$DST/lib" && echo -n "#"
rm -rf "$DST/sbin" && echo -n "#"
rm -rf "$DST/tmp" && echo -n "#"
rm -rf "$DST/usr" && echo -n "#"
rm -rf "$DST/var" && echo -n "#"
echo " done."

echo -n "Creating basic dir structure "
mkdir -p "$DST/etc" && echo -n "#"
mkdir -p "$DST/flash" && echo -n "#"
mkdir -p "$DST/tmp" && echo -n "#"
mkdir -p "$DST/var/log" && echo -n "#"
echo " done."

echo -n "Installing base system binaries "
FIX="/bin/ping /bin/traceroute"
for i in /bin/more /bin/mount /sbin/init $FIX \
		;do
		install -m 0755 -D "$i" "$DST$i" && echo -n "#"
done
echo " done."

echo -n "Installing various configuration files "
for i in /etc/ld.so.conf \
		;do
		install -m 0644 -D "dist$i" "$DST$i" && echo -n "#"
done
echo " done."

echo -n "Installing LMS binaries "
install -m 0755 -D userspace/cli/filter $DST/bin/filter && echo -n "#"
install -m 0755 -D userspace/cli/swcli $DST/bin/swcli && echo -n "#"
install -m 0755 -D userspace/cli/swclid $DST/sbin/swclid && echo -n "#"
install -m 0755 -D userspace/cli/swlogin $DST/sbin/swlogin && echo -n "#"
echo " done."

echo -n "Installing optional binaries "
for i in /bin/bash /bin/cat /bin/ls /bin/ps \
		/usr/bin/less \
		/sbin/ifconfig /sbin/ip /sbin/route \
		;do
		install -m 0755 -D "$i" "$DST$i" && echo -n "#"
done
echo " done."

echo -n "Finding installed binaries "
TMP1=`mktemp` || exit 1
TMP2=`mktemp` || exit 1
TMP3=`mktemp` || exit 1
find $DST -type f -perm +0222 | tee $TMP1 | sh -c "while read x; do echo -n '#'; done"
echo " done."

echo -n "Calculating dependencies "
exec < $TMP1
while read FILE; do
	ldd $FILE > $TMP2 2> /dev/null || continue
	cat $TMP2 | sed 's/^.*=> \(.*\) (.*$/\1/' | sed 's/^[ \t]*\(.*\) (.*$/\1/' | sed 's/tls\///' >> $TMP3
	echo -n "#"
done
cat $TMP3 | sort | uniq > $TMP1
echo " done."

echo -n "Installing libraries "
exec < $TMP1
while read FILE; do
	REAL=`readlink -f -n $FILE 2>/dev/null`
	install -m 0755 -D "$REAL" "$DST$REAL"
	if [ "$FILE" != "$REAL" ]; then
		cp -a $FILE $DST$FILE
	fi
	echo -n "#"
done
echo " done."
rm -f $TMP1
rm -f $TMP2
rm -f $TMP3

# fstab
# inittab
# passwd
# shadow ???
# sshd (opt)
# kernel
# grub
# *getty
# sh
# dev entries ( null, zero, pty*, ttyS* , hda*, pts*, console)
