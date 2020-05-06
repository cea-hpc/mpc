#!/bin/sh
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - JAEGER Julien julien.jaeger@cea.fr                               #
#                                                                      #
########################################################################

ac_expect="2.69"
am_expect="1.15"
lt_expect="2.4.6"

ret=0
err()
{
	1>&2 printf "Error: $@\n"
	ret=1
}

extract_version()
{
	"$1" --version | head -n 1 | grep -E -o "[0-9]+(\.[0-9]+)*"
}

arc=$(command -v autoreconf)
ac=$(command -v autoconf)
am=$(command -v automake)
lt=$(command -v libtoolize)

test -n "$arc" || err "No 'autoreconf' command in PATH. Please install autoconf first."
test -n "$ac" || err "No 'autoconf' command in PATH. Please install autoconf first."
test -n "$am" || err "No 'automake' command in PATH. Please install automake first."
test -n "$lt" || err "No 'libtoolize' command in PATH. Please install libtool first."


ac_version=$(extract_version "$ac")
am_version=$(extract_version "$am")
lt_version=$(extract_version "$lt")


test "$ac_version" = "$ac_expect" || err "Exact version required for Autoconf: $ac_expect (got $ac_version)"
test "$am_version" = "$am_expect" || err "Exact version required for Automake: $am_expect (got $am_version)"
test "$lt_version" = "$lt_expect" || err "Exact version required for Autoconf: $lt_expect (got $lt_version)"

if test "$ret" = "0"; then
	$arc -vi
	ret=$?
fi
test "$ret" = "0" || printf "Abort configuration due to error(s) above.\n"
