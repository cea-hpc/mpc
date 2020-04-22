#!/bin/sh

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

file=$1
version=$2
package_name=$3

test -n "$version" || exit 42
test -n "$file" || exit 42

manpage="`cat $1`"

echo "$manpage" | sed -e "s@#PACKAGE_DATE#@`date +%Y`@" \
    -e "s,#PACKAGE_VERSION#,$version," \
    -e "s@#PACKAGE_NAME#@$package_name@"

if test -n "`echo "$file" | grep "MPI_.*.3in"`"; then
cat<<EOF
.SH DISCLAIMER
.ft R
This man page, while documenting a function from the MPI standard, is fully part of the OpenMPI implementation and the "Multi-Processor Computing" (MPC) team complies in full respect regarding the MPI standard. It has been generated and original authors are mentioned in the Open-MPI project.

The Open-MPI project is distributed under the 3-clause BSD license and freely available at https://www.open-mpi.org/. There is NO WARRANTY to the extent permitted by law.

EOF
fi

cat<<EOF
.SH REPORTING BUGS
.ft R
Julien Jaeger <julien.jaeger@cea.fr>, ParaTools Staff <info@paratools.com>
EOF
