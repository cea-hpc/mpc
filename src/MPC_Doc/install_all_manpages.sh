#!/bin/sh

error()
{
	printf "ERROR: %s\n" "$@"
	exit 42
}


SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

version=$1
package_name=$2
dest_mandir=$3

test -z "$version" && exit 42
test -z "$dest_mandir" && exit 42


extract_man_info()
{
        CLEAN_IN=$(echo $1 | sed -e "s#.*man_static/man#=#" -e "s/.[0-9]in//" -e "s,/,=,")
        mannum=$(echo "$CLEAN_IN" | cut -d "=" -f 2)
        manname=$(echo "$CLEAN_IN" | cut -d "=" -f 3)
}


for path in $(find ${SCRIPTPATH}/man_static/ -iname "*[0-9]in")
do
        extract_man_info "$path"
	man_dest=${dest_mandir}/man${mannum}/${manname}.${mannum}

	if test ! -f "${man_dest}"; then
		echo "  GEN $(basename "$path")"
		manpage=$("${SCRIPTPATH}/patch_manpage.sh" "$path" "$version" "${package_name}")

		mkdir -p "${dest_mandir}/man${mannum}/"
		man_dest=${dest_mandir}/man${mannum}/${manname}.${mannum}
		echo "  MAN $(basename "$man_dest")"
		echo "$manpage" > "$man_dest"
	fi
done