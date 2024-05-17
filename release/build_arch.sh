#!/bin/sh

set -x -e

cd /root || exit 1
mkdir /root/pkg
mv PKGBUILD mpcframework*.tar.gz pkg/
cd /root/pkg || exit 1
touch changelog
echo '#!/bin/sh' >> install-mpc
makepkg -g >> PKGBUILD && makepkg
