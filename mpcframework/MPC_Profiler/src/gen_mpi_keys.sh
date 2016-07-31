#!/bin/bash
LINE="1"
read arg1 arg2 arg3 arg4
while test "$?" = "0"; do
    if test "$arg1" = "//" ; then
	if test "$arg2" = "PROFILER" ; then
	    echo "PROBE( $arg3, MPI,$arg4)"
	    TYPE="$arg3"
	fi
    fi
    if test "$arg1" = "#pragma" ; then
	if test "$arg2" = "weak" ; then
	    echo "        PROBE( $arg3, $TYPE , $arg3) //LINE $LINE"
	fi
    fi

    LINE="$(($LINE + 1))"
    read arg1 arg2 arg3 arg4
done
