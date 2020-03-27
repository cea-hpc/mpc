#!/bin/sh

#
# You may run this through "make lint"
#

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")


BUILDIR=$1

if test "x$#" != "x1"; then
	echo "Usage: $0 [BUILD DIR]"
	exit 1
fi

command -v clang-tidy

if test "x$?" != "x0"; then
	echo "Linting requires clang-tidy"
	exit 1
fi

clean()
{
	echo "Cleaning UP"
	rm -fr "$TMP"
	exit 1
}


trap 'clean' INT


TMP=$(mktemp -d)

for i in $(find "${SCRIPTPATH}/../src/" -iname "*.[ch]") $(find "$1" -iname "*.[ch]")
do
	BN=$(basename "$i")
	if test ! -f  "$TMP/${BN}"; then
		ln -s "$i" "$TMP/${BN}"
	fi
done

. "$BUILDIR/src/bin/mpc_build_env.sh"


for i in $(find "${SCRIPTPATH}/../src/" -iname "*.[ch]")
do
	clang-tidy "$i" -- -I "$TMP" $(pkg-config --cflags mpcalloc || true)
done


clean