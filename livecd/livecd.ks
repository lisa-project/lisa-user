lang en_US.UTF-8
keyboard us
timezone US/Eastern
auth --useshadow --enablemd5
selinux --disabled
firewall --disabled
part / --size 1024
services --disabled=avahi-daemon,haldaemon,ip6tables,iptables,kudzu,mcstrans,messagebus,netfs,xfs

repo --name=CentOS-5.3 --baseurl=http://ftp.ines.lug.ro/centos/5.3/os/i386/
repo --name=LiSA-el5.3 --baseurl=file:///home/blade/i386/

%packages
@core
anaconda-runtime
bash
kernel
passwd
chkconfig
# This package is needed for the selinux configuration
system-config-securitylevel-tui
rootfiles
lisa

%post
cat > /etc/lisa/config.text << EOF
!
no ip igmp snooping
EOF

cat > /etc/issue << EOF
LiSA 2.0 LiveCD based on CentOS release 5.3
Kernel \r on an \m (\l)

Login with user root and no password.
Type swcli to get started with LiSA.

EOF

cat > /etc/issue.net << EOF
LiSA 2.0 LiveCD based on CentOS release 5.3
Kernel \r on an \m (\l)

Login with user root and no password.
Type swcli to get started with LiSA.

EOF
