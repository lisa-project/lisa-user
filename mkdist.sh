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
rm -rf "$DST/boot" && echo -n "#"
rm -rf "$DST/dev" && echo -n "#"
rm -rf "$DST/etc" && echo -n "#"
rm -rf "$DST/flash" && echo -n "#"
rm -rf "$DST/lib" && echo -n "#"
rm -rf "$DST/proc" && echo -n "#"
rm -rf "$DST/root" && echo -n "#"
rm -rf "$DST/sbin" && echo -n "#"
rm -rf "$DST/tmp" && echo -n "#"
rm -rf "$DST/usr" && echo -n "#"
rm -rf "$DST/var" && echo -n "#"
echo " done."

echo -n "Creating basic dir structure "
mkdir -p "$DST/dev" && echo -n "#"
mkdir -p "$DST/etc" && echo -n "#"
mkdir -p "$DST/flash" && echo -n "#"
mkdir -p "$DST/proc" && echo -n "#"
mkdir -p "$DST/root" && echo -n "#"
mkdir -p "$DST/tmp" && echo -n "#"
mkdir -p "$DST/var/log" && echo -n "#"
echo " done."

echo -n "Installing base system binaries "
FIX="/bin/ping /bin/traceroute"
for i in /bin/ash /bin/grep /bin/hostname /bin/more /bin/mount /bin/sed \
		/sbin/agetty /sbin/e2fsck /sbin/init /sbin/sysctl \
		/usr/sbin/in.telnetd \
		"/usr/bin/[" \
		\
		/boot/grub/stage1 /boot/grub/e2fs_stage1_5 /boot/grub/stage2 \
		$FIX \
		;do
		install -m 0755 -D "$i" "$DST$i" && echo -n "#"
done
ln -s /bin/ash $DST/bin/sh
ln -s /sbin/e2fsck $DST/sbin/fsck
echo " done."

#FIXME /usr/share/terminfo/l/linux
#FIXME cat si rm in dist. standard, pt ca sunt folosite de rc.sysinit

echo -n "Installing various configuration files "
for i in /etc/ld.so.conf /etc/inittab /etc/passwd /etc/termcap \
		/etc/rc.d/rc.sysinit /etc/fstab /etc/sysctl.conf \
		/boot/grub/grub.conf /boot/grub/device.map \
		;do
		install -m 0644 -D "dist$i" "$DST$i" && echo -n "#"
done
chmod 0755 "$DST/etc/rc.d/rc.sysinit"
if [ -n "$OPT" ]; then
	cat dist/etc/inittab | sed 's/#__OPT__#//' > $DST/etc/inittab
fi
mkdir -p $DST/etc/rc.d/rc0.d &> /dev/null
ln -s /etc/init.d/halt $DST/etc/rc.d/rc0.d/S01halt
mkdir -p $DST/etc/rc.d/rc1.d &> /dev/null
mkdir -p $DST/etc/rc.d/rc2.d &> /dev/null
mkdir -p $DST/etc/rc.d/rc3.d &> /dev/null
mkdir -p $DST/etc/rc.d/rc4.d &> /dev/null
mkdir -p $DST/etc/rc.d/rc5.d &> /dev/null
mkdir -p $DST/etc/rc.d/rc6.d &> /dev/null
ln -s /sbin/init.d/halt $DST/etc/rc.d/rc0.d/S01reboot
echo " done."

echo -n "Installing LMS binaries "
install -m 0755 -D userspace/cdpd/cdpd $DST/bin/cdpd && echo -n "#"
install -m 0755 -D userspace/cli/filter $DST/bin/filter && echo -n "#"
install -m 0755 -D userspace/cli/swcli $DST/bin/swcli && echo -n "#"
install -m 0755 -D userspace/cli/swclid $DST/sbin/swclid && echo -n "#"
install -m 0755 -D userspace/cli/swlogin $DST/usr/sbin/swlogin && echo -n "#"
install -m 0755 -D userspace/cli/swcon $DST/sbin/swcon && echo -n "#"
install -m 0755 -D userspace/cli/swcfgload $DST/sbin/swcfgload && echo -n "#"
install -m 0755 -D userspace/cli/reboot $DST/sbin/reboot && echo -n "#"
install -m 0755 -D userspace/cli/libswcli.so $DST/lib/libswcli.so && echo -n "#"
echo " done."

if [ -n "$OPT" ]; then
	echo -n "Installing optional binaries "
	for i in /bin/bash /bin/cat /bin/ls /bin/ps /bin/vi /bin/netstat \
			/bin/cp /bin/mv /bin/rm /bin/mkdir /bin/rmdir /bin/ln \
			/bin/df /usr/bin/du /bin/mknod \
			/usr/bin/ssh /usr/bin/scp /usr/bin/ncftpget \
			/bin/sync /bin/dmesg \
			/sbin/pidof /sbin/fuser /usr/bin/which \
			/usr/bin/less /usr/bin/strace \
			/usr/bin/ipcs /usr/bin/ipcrm \
			/bin/tar /bin/gzip \
			/sbin/grub /sbin/ifconfig /sbin/ip /sbin/mingetty /sbin/route \
			;do
			install -m 0755 -D "$i" "$DST$i" && echo -n "#"
	done
	install -m 0755 -D userspace/login/login $DST/bin/login && echo -n "#"
	install -m 0755 -D dist/sbin/grub-install $DST/sbin/grub-install && echo -n "#"
	install -m 0755 -D binupdate/binupdate $DST/binupdate && echo -n "#"
	install -m 0755 -D binupdate/tar $DST/tar && echo -n "#"
	echo " done."
fi

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
	cat $TMP2 | grep -vE '(statically linked|not a dynamic executable|not found|=> *\()' | \
	sed 's/^.*=> \(.*\) (.*$/\1/' | sed 's/^[ \t]*\(.*\) (.*$/\1/' | sed 's/tls\///' >> $TMP3
	echo -n "#"
done

echo /lib/libnss_files.so.2 >> $TMP3

cat $TMP3 | sort | uniq | grep -v libswcli > $TMP1
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

echo -n "Creating special device files "
mknod -m 0666 $DST/dev/null				c	1	3
mknod -m 0666 $DST/dev/zero				c	1	5
mknod -m 0666 $DST/dev/random			c	1	8
mknod -m 0666 $DST/dev/urandom			c	1	9
mknod -m 0660 $DST/dev/tty1				c	4	1
mknod -m 0660 $DST/dev/tty2				c	4	2
mknod -m 0660 $DST/dev/tty3				c	4	3
mknod -m 0660 $DST/dev/tty4				c	4	4
mknod -m 0660 $DST/dev/tty5				c	4	5
mknod -m 0660 $DST/dev/tty6				c	4	6
mknod -m 0660 $DST/dev/ttyS0			c	4	64
mknod -m 0666 $DST/dev/tty				c	5	0
mknod -m 0666 $DST/dev/console			c	5	1
mknod -m 0666 $DST/dev/ptmx				c	5	2
mknod -m 0660 $DST/dev/hda				b	3	0
mknod -m 0660 $DST/dev/hda1				b	3	1
mkdir $DST/dev/pts
echo -n "#"
echo " done."

echo -n "Installing kernel "
install -m 0600 -D $SW_KPATH/bzImage $DST/flash/vmlinuz
echo -n "#"
echo " done."
