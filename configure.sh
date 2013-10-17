#!/bin/bash

# Install dependencies
echo "[LiSA] Installing dependencies (may require privilegies)..."
if [ -f /etc/lsb-release ]; then
    os=$(lsb_release -s -d)
elif [ -f /etc/redhat-release ]; then
    os=`cat /etc/redhat-release`
else
    os="$(uname -s) $(uname -r)"
fi

if [[ "$os" == *Ubuntu* ]]
then
    sudo apt-get install readline-common
    sudo apt-get install libpcap-dev
elif [[ "$os" == *CentOS* ]]
then
    sudo yum install readline-devel
    sudo yum install libpcap-devel.i686
else
    echo "unknown distro";
    exit 0
fi

# Fix path
echo "[LiSA] Build LiSA cli"
cd userspace/cli
. fix-ld-library-path.sh
# Build lisa cli
cd ..
make clean
make

