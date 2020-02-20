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


extract_man_info()
{
        CLEAN_IN=$(echo $1 | sed -e "s#.*man/man#=#" -e "s/.md//" -e "s,/,=,")
        mannum=$(echo $CLEAN_IN | cut -d "=" -f 2)
        manname=$(echo $CLEAN_IN | cut -d "=" -f 3)
}

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

version=$1
source_md_file=$2

test -n "$version" || exit 42

which pandoc >/dev/null 2>&1 || error "Failed to find 'pandoc' to build static man-pages"

pandoc_args="-s -f markdown -t man -V header=Multi-Processor-Computing -V date=`date +%x` -V footer=MPC-${version}"

if test -z "$source_md_file"; then
        for markdown in `find ${SCRIPTPATH}/man/ -name "*.md"`
        do
                extract_man_info "$markdown"

                manfile="${SCRIPTPATH}/man_static/man${mannum}/${manname}.${mannum}in"
                echo "  GEN $(basename $manfile)"
                pandoc $pandoc_args $markdown -V section="$mannum" -o $manfile

        done
else
        extract_man_info "$source_md_file"
	pandoc $pandoc_args "$source_md_file" -V section=${mannum}
fi
