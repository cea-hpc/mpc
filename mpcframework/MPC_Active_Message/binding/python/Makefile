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
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#    - ADAM Julien <adamj@paratools.com>                               #
#                                                                      #
########################################################################
all: build install

build: api.c setup.py

install: build
	@echo "LIB arpc4py.so"
	@ARPC_LDFLAGS="mpc_framework" ARPC_HDRS="$(M_PWD)/INSTALL/include/mpcframework" python3 setup.py install --install-lib=$(M_PWD)/INSTALL/lib64/python > /dev/null
	@python3 setup.py clean --all >/dev/null 2>&1

