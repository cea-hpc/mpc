#!/bin/sh
set -x -e
cd /root/rpmbuild || exit 1
rpmbuild -bb SPECS/mpc.spec || exit 1
