#!/bin/sh

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

command -v uncrustify

if test "x$?" = "x0"; then
	echo "Formatting with uncrustify config ${SCRIPTPATH}/uncrustify.cfg"
	uncrustify -c "${SCRIPTPATH}/uncrustify.cfg" --replace "$@"
	exit 0
fi

command -v clang-format

if test "x$?" = "x0"; then
	echo "Formatting with uncrustify config ${SCRIPTPATH}/../.clang-format"
	clang-format -style=file -i "$@"
	exit 0
fi

echo "Could not locate uncrustify or clang format"