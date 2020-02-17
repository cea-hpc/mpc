#!/bin/sh

TMP=`mktemp`

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cat_mpit_file()
{
	echo " === Processing $f === " >&2
	cat $1 >> $TMP
}



process_files()
{
	echo "<mpit>" > $TMP
	for f in $1
	do
		cat_mpit_file $f
	done
	echo "</mpit>" >> $TMP

	#echo "======" >&2
	#cat $TMP >&2
	#echo "======" >&2
	
	${SCRIPTPATH}/gen_mpit.py $TMP

	rm -fr $TMP
}


if test "x$#" = "x1"; then
echo "Moving to $1" >&2
cd $1
fi

MPITF=`find . -iname "mpit.xml"`


process_files "$MPITF"

