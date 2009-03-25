#!/bin/bash

# You must have POSIX message queues support in your kernel
#
# Check CONFIG_POSIX_MQUEUE
#
if [ $UID -ne 0 ]; then
	echo "You must be root to run this script"
	exit 1
fi

mkdir -p /dev/mqueue
mount -t mqueue none /dev/mqueue
