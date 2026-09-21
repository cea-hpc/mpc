#!/bin/bash

set -e -x
mkdir /root/mpc /root/installmpc
cd /root/mpc || exit 1
tar xzf /root/debbuild/mpcframework.tar.gz
export CFLAGS="$CFLAGS -Wno-format-security -fno-lto"
export PATH=/usr/local/bin:$PATH
# Build the embedded contrib dependencies into a staging prefix
# so that the CMake build of MPC can consume them
STAGING=$PWD/staging
(cd contrib/openpa && ./configure --prefix=$STAGING && make -j && make install)
(cd contrib/hydra && ./configure --prefix=$STAGING && make -j && make install)
cmake -B BUILD -DCMAKE_INSTALL_PREFIX=/usr/local -DMPC_OPENPA_PREFIX=$STAGING -DMPC_LAUNCHER=HYDRA -DMPC_HYDRA_PREFIX=$STAGING -DMPC_ENABLE_PROCESS_MODE=ON -DMPC_ENABLE_MPCALLOC=OFF
cmake --build BUILD --parallel
DESTDIR=/root/installmpc cmake --install BUILD
cd /root/ || exit 1
mkdir installmpc/DEBIAN
cp /root/control installmpc/DEBIAN/
dpkg-deb --build installmpc
