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
mpc_headerdir=$(pkgincludedir)
mpc_header_HEADERS= ./include/mpc_lowcomm.h  ./include/mpc_lowcomm_rdma.h ./include/mpc_lowcomm_communicator.h ./include/mpc_lowcomm_group.h ./include/mpc_lowcomm_types.h ./include/mpc_lowcomm_arpc.h ./include/mpc_lowcomm_monitor.h ./include/mpc_lowcomm_handle_ctx.h ./include/mpc_lowcomm_mqd.h ./include/mpc_lowcomm_datatypes.h

if MPC_HAVE_AM
  mpc_header_HEADERS+= ./include/mpc_lowcomm_am.h
endif

SUBDIRS=src
