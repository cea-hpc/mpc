#!/bin/sh

if test "x$#" = "x0"; then
        echo "USAGE:"
        echo "$0 [MPC_VERSION]  -> Gen all .md doc"
        echo "$0 [MPC_VERSION] [relative path to MD file] -> Gen this .md doc"
        exit 1
fi


error()
{
	printf "ERROR: $@\n"
	exit 42
}


SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

source=$1
target=$2
version=$3
test -z "$source" && die "Need a source file (Markdown) as first argument"
test -z "$target" && die "Need a destination file (man) as second argument"
test -z "$version" && version=$(${SCRIPTPATH}/../../utils/get_version --all)

which pandoc >/dev/null 2>&1 || error "Failed to find 'pandoc' to build static man-pages"

pandoc_args="-s -f markdown -t man -V header=Multi-Processor-Computing -V date=`date +%m/%Y` -V footer=$version"
pandoc $pandoc_args $source -o $target
