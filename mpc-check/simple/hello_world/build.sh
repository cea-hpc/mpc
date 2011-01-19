#!/bin/sh

if test "`ls *.c 2> /dev/null`" != "" ; then 
    mpc_cc -g *.c -DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_LONG_LONG_INT=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1 -o ./a.out
    if test "$?" != "0" ; then 
	exit 1
    fi
fi

if test "`ls *.f 2> /dev/null`" != "" ; then 
    mpc_f77 -g *.f -o ./a.out
    if test "$?" != "0" ; then 
	exit 1
    fi
fi

#while ! test -f ./a.out ; do 
#    sleep 1s
#done

test -f ./a.out
exit $?
