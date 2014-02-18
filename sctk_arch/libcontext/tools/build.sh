#!/bin/sh
rm -rf bin
mkdir bin
for i in test/*; do
	echo "Build $i" 
	gcc -fPIC -IX86_64 X86_64/*.S X86_64/*.c $i -o bin/`basename $i .c`
        if test "$?" != "0" ; then
                exit $?
        fi
	echo "Build $i orig"
	gcc -IGEN $i -o bin/`basename $i .c`_orig
        if test "$?" != "0" ; then
                exit $?
        fi

	echo "Exec bin/`basename $i .c`"
	./bin/`basename $i .c`

	if test "$?" != "0" ; then 
		exit $?
	fi

        echo "Exec bin/`basename $i .c`_orig"
        ./bin/`basename $i .c`_orig

        if test "$?" != "0" ; then
                exit $?
        fi

done
