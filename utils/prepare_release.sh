#!/bin/bash
USER=$(whoami)
CREATEOWNER="adduser $USER -p \"\""

SCRIPTPATH=$(dirname "$(readlink -f "$0")")

##################### FUNCTION ###################
safe_exec()
{
	echo "MPC RELEASE .................... $*"
	"$@"
    if test $? -ne 0; then
        echo -e "$1 failed, try $0 --clean"
    fi
}
move_to_contrib(){
    cd "$SCRIPTPATH/../contrib" || exit
    if ! test -f ../installmpc
    then
        echo "please do not move this file"
        exit
    fi
}

init_env(){
# we fail to configure the installation on rockylinux 8
# because hwloc-devel package doesn't seem to be present on the corresponding yum repositories

# if [ "$DISTRIB" == "rockylinux:8" ]; then
#     CXX="gcc-c++"
#     CFORTRAN="gcc-gfortran"
#     HWLOC_DEVEL="hwloc-libs"
# fi
    CXX="g++"
    CFORTRAN="gfortran"
    HWLOC_DEVEL="hwloc-devel"
}


########### ARCH packages ##############

build_dockerfile_arch(){
cat << EOF > release/Dockerfile
FROM $DISTRIB
RUN useradd archuser
RUN usermod -g root archuser
RUN pacman -Sy --noconfirm --needed base-devel python3 cmake gcc-fortran wget hwloc
COPY archbuild /root/
COPY build_arch.sh /root/
RUN chown archuser /root /root/PKGBUILD /root/build_arch.sh /root/mpcframework-$VERSION.tar.gz
USER archuser
RUN sh /root/build_arch.sh
EOF
}

build_pkgbuild()
{
cat << EOF > release/archbuild/PKGBUILD
# Maintainer: Paratools SAS
pkgname=mpcframework
pkgver=$VERSION
pkgrel=1
epoch=1
pkgdesc="mpi runtime for exascale"
arch=("x86_64")
url="https://gitlab.paratools.com/cea/mpc"
license=('GPL')
groups=(mpcframework)
depends=("python3" "cmake" "make" "gcc" "gcc-fortran" "wget" "hwloc")
makedepends=()
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install="install-mpc"
changelog="changelog"
source=(\$pkgname-\$pkgver.tar.gz)
noextract=()
md5sums=() #generate with 'makepkg -g'

prepare() {
	cd "\$srcdir/\$pkgname-\$pkgver/contrib"
	../installmpc --download
	tar xzf hydra*.gz
	tar xzf openpa*.gz
	rm -rf ./*.tar.gz hydra/ openpa/
	mv hydra* hydra
	mv openpa* openpa
	cd "\$srcdir/\$pkgname-\$pkgver"
	mkdir -p BUILD
}

build() {
	cd "\$srcdir/\$pkgname-\$pkgver"
	cd BUILD
	export DESTDIR=\$pkgdir
	../configure --prefix=/usr/local --enable-rpmbuild
	make
}

check() {
	echo ""
}

package() {
	cd "\$srcdir/\$pkgname-\$pkgver/BUILD"
	DESTDIR="\$pkgdir/" make -j install
	mv \$pkgdir/usr/local/share/man \$pkgdir/usr/local/man
}
EOF
}

build_arch()
{
    move_to_contrib
    safe_exec ../installmpc --download
    safe_exec tar xzf hydra*.gz
    safe_exec tar xzf openpa*.gz
    safe_exec rm -rf ./*.tar.gz hydra/ openpa/
    safe_exec mv hydra* hydra
    safe_exec mv openpa* openpa
    safe_exec cd ..
    tar --transform="flags=r;s|mpc|mpcframework-$VERSION|" -czf /tmp/mpcframework.tar.gz ../mpc
    mkdir -p $PWD/release/archbuild/
    mv /tmp/mpcframework.tar.gz $PWD/release/archbuild/mpcframework-$VERSION.tar.gz
    safe_exec cd release
	safe_exec docker build -t mpc_release_$DISTRIB --rm  .
}

extract_arch()
{
    safe_exec docker run --user $USER --rm -v $(readlink -f $TARGET):/host mpc_release_$DISTRIB /bin/sh -c cp /pkg/mpcframework-1:$VERSION-1-x86_64.pkg.tar.zst /host/mpcframework-"$VERSION"-"$DISTRIB".tar.zst
}

########### DEB packages ###############

build_dockerfile_deb(){
cat << EOF > release/Dockerfile
FROM $DISTRIB
COPY debbuild /root/debbuild
COPY build_deb.sh /root/build_deb.sh
COPY control /root/control
RUN $CREATEOWNER
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata
RUN apt install -y util-linux cmake patch make gcc $CXX $CFORTRAN pkg-config wget git hwloc libhwloc-dev python3 dpkg-dev
RUN sh /root/build_deb.sh
RUN chown $USER /root/installmpc.deb ;
RUN mv /root/installmpc.deb /
EOF
}

build_control()
{
cat << EOF > release/control
Package: mpcframework
Version: $VERSION
Maintainer: CEA/Paratools
Architecture: all
Description: MPI runtime for exascale
Depends: cmake, make, gcc, $CXX, $CFORTRAN, wget, hwloc, libhwloc-dev
EOF
}

build_deb()
{
    move_to_contrib
    safe_exec ../installmpc --download
    safe_exec tar xzf hydra*.gz
    safe_exec tar xzf openpa*.gz
    safe_exec rm -rf ./*.tar.gz hydra/ openpa/
    safe_exec mv hydra* hydra
    safe_exec mv openpa* openpa
    safe_exec cd ..
    tar --transform="flags=r;s|mpc|mpcframework-$VERSION|" -czf /tmp/mpcframework.tar.gz ../mpc
    mkdir -p $PWD/release/debbuild/
    mv /tmp/mpcframework.tar.gz $PWD/release/debbuild/mpcframework.tar.gz
    safe_exec cd release
	safe_exec docker build -t mpc_release_$DISTRIB --rm  .
}

extract_deb()
{
    safe_exec docker run --user $USER --rm -v $(readlink -f $TARGET):/host mpc_release_$DISTRIB /bin/sh -c "cp /installmpc.deb /host/mpcframework-"$VERSION"-"$DISTRIB".deb"
}

########### RPM packages ###############

build_dockerfile_rpm(){
cat << EOF > release/Dockerfile
FROM $DISTRIB
COPY rpmbuild /root/rpmbuild
COPY build_rpm.sh /root/build_rpm.sh
RUN $CREATEOWNER
RUN yum clean all
RUN yum check-update || :
RUN rpm -vv --import /etc/pki/rpm-gpg/*
RUN dnf update -y --nogpgcheck
RUN yum install -y util-linux cmake patch make gcc $CXX $CFORTRAN pkg-config wget rpm rpm-build git hwloc $HWLOC_DEVEL || :
RUN sh /root/build_rpm.sh
RUN chown -R $USER /root/rpmbuild
RUN mv /root/rpmbuild /
EOF
}

build_spec_rpm(){
mkdir -p release/rpmbuild/SPECS
cat << EOF > release/rpmbuild/SPECS/mpc.spec
Name:           mpcframework
Version:        $VERSION
Release:        1%{?dist}
Summary:        mpi runtime for exascale

License:        GPL
URL:		    https://gitlab.paratools.com/cea/mpc
Source0:        %{name}.tar.gz

Requires:	cmake, make, gcc, $CXX, $CFORTRAN, wget, hwloc, $HWLOC_DEVEL

%description
mpc

%global debug_package %{nil}
%prep
%setup -q
mkdir BUILD

%build

cd BUILD
export DESTDIR=\$RPM_BUILD_ROOT
export CFLAGS="\$CFLAGS -Wno-format-security -fno-lto"
export PATH=%{_bindir}:\$PATH
../configure --disable-prefix-check --enable-rpmbuild --prefix=/usr/local --disable-spack --disable-autopriv --disable-mpcalloc

%install

# rpmbuild complains that RPATH is broken because /usr/lib is duplicated
# and nonexistent paths are specified.
# 0x0001|0x0002 are masks used to ignore these errors.
# 0x0003 corresponds to a logical and for 0x0001|0x0002.
# QA_RPATHS=\$[ 0x0001|0x0002 ] would not work because 0x0002 would not be found
# CF https://unix.stackexchange.com/questions/202009/rpm-build-check-rpaths-error-0x0001
export QA_RPATHS=\$[0x0003]

export DESTDIR=\$RPM_BUILD_ROOT
cd BUILD
make install -j

%files
%defattr(-,root,root,-)
/usr/local/*

%changelog
* Mon Jul 18 2022 root
-

EOF
}

build_rpm()
{
    move_to_contrib
    safe_exec ../installmpc --download
    safe_exec tar xzf hydra*.gz
    safe_exec tar xzf openpa*.gz
    safe_exec rm -rf ./*.tar.gz hydra/ openpa/
    safe_exec mv hydra* hydra
    safe_exec mv openpa* openpa
    safe_exec cd ..
    safe_exec rm -f $PWD/release/rpmbuild/SOURCES/mpcframework.tar.gz
    tar --transform="flags=r;s|mpc|mpcframework-$VERSION|" -czf /tmp/mpcframework.tar.gz ../mpc
    mkdir -p $PWD/release/rpmbuild/SOURCES/
    mv /tmp/mpcframework.tar.gz $PWD/release/rpmbuild/SOURCES/
    safe_exec cd release
	safe_exec docker build -t mpc_release_$DISTRIB --rm  .
}

extract_rpm()
{
    safe_exec docker run --user $USER --rm -v $(readlink -f $TARGET):/host mpc_release_$DISTRIB /bin/sh -c "cp -r /rpmbuild/RPMS/x86_64/ /host"
}

clean(){
	safe_exec docker container rm -f mpc_release_container
	safe_exec docker volume rm -f mpc_release_volume
	safe_exec docker image rm -f "$(docker image ls mpc_release_image -q)"
    safe_exec rm -rf contrib/openpa* contrib/hydra* release/dockerfile release/rpmbuild/SOURCES/*
    safe_exec rm -rf release/debbuild/*
    safe_exec rm -rf release/archbuild/*
}

help()
{
cat << EOF
Prepares and builds a rpm archive

You must have docker installed. This script looks for "fedora" image by default.
For any other distribution please specify with adequate option.

This script works for commit a1da0c933be6f07de058b949420cf4070c786c60.
For any previous version please use utils/gen_archive.sh.


Usage: $0 [--distribution=X] [--target=X] deb|rpm

Arguments :
    deb|rpm             Build deb or rpm package (must specify corresponding distribution)
    --distribution=X    Specify distribution (image available in docker). Set to fedora:latest by default
    --target=X          Specify destination for rpm package. Set to release/pkgs by default

Options :
    --extract-only      Look for active image of specified distribution and retreives rpm package.
    --clean             Clean directories and docker, then exits.
    --help              Print this documentation string, then exits.
EOF
    exit 0
}

for arg in "$@"
do
	case "$arg" in
        --distribution=*)
            DISTRIB=$(echo "$arg" | sed -e "s/^--distribution=//g")
            ;;
        --target=*)
            TARGET=$(echo "$arg" | sed -e "s/^--target=//g")
            ;;
        --version=*)
            VERSION=$(echo "$arg" | sed -e "s/^--version=//g")
            ;;
		--help|-h)
			help
			;;
		--extract-only|-e)
			EXTRACT_ONLY=yes
			;;
        --clean|-c)
            clean
            exit 0;
            ;;
        deb)
            BUILD_DEB_ARCHIVE=yes
            ;;
        rpm)
            BUILD_RPM_ARCHIVE=yes
            ;;
        arch)
            BUILD_ARCH_ARCHIVE=yes
            ;;
		*)
			echo "Invalid argument '$arg', please check your command line or get help with --help." 1>&2
			exit 1
	esac
done

if test -z "$DISTRIB"; then
    echo "setting distrib to default ..... (fedora:latest)"
    DISTRIB=fedora:latest
fi

if test -z "$VERSION"; then
    VERSION="$($SCRIPTPATH/../utils/get_version)"
    echo "setting version to current ..... ($VERSION)"
fi

if test -z "$TARGET" ; then
    echo "setting target to default ...... (release/)"
    TARGET=$SCRIPTPATH/../release/pkgs/
else
    mkdir -p $TARGET || (echo "Target must not exist or must already be a directory (is currently $TARGET)"; exit 1)
fi

init_env

if ! test -z "$BUILD_RPM_ARCHIVE"; then
    build_dockerfile_rpm
    build_spec_rpm

    if test -z "$EXTRACT_ONLY" ; then
        build_rpm
    fi
    extract_rpm
fi

if ! test -z "$BUILD_DEB_ARCHIVE"; then
    build_dockerfile_deb
    build_control

    if test -z "$EXTRACT_ONLY" ; then
        build_deb
    fi
    extract_deb
fi

if ! test -z "$BUILD_ARCH_ARCHIVE"; then
    build_pkgbuild
    build_dockerfile_arch

    if test -z "$EXTRACT_ONLY" ; then
        build_arch
    fi
    extract_arch
fi
