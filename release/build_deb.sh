#!/bin/sh

set -e -x
mkdir /root/mpc /root/installmpc
cd /root/mpc || exit 1
tar xzf /root/debbuild/mpcframework.tar.gz
cd mpc* || exit 1
mkdir BUILD
cd BUILD || exit 1
export DESTDIR=/root/installmpc
export CFLAGS="$CFLAGS -Wno-format-security -fno-lto"
export PATH=/usr/local/bin:$PATH
../configure --disable-prefix-check --enable-rpmbuild --prefix=/usr/local --disable-spack --disable-autopriv --disable-mpcalloc
make install -j
cd /root/ || exit 1
mkdir installmpc/DEBIAN
cp /root/control installmpc/DEBIAN/
dpkg-deb --build installmpc