! ############################# MPC License ############################## 
! # Wed Nov 19 15:19:19 CET 2008                                         # 
! # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # 
! #                                                                      # 
! # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # 
! # This file is part of the MPC Runtime.                                # 
! #                                                                      # 
! # This software is governed by the CeCILL-C license under French law   # 
! # and abiding by the rules of distribution of free software.  You can  # 
! # use, modify and/ or redistribute the software under the terms of     # 
! # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # 
! # following URL http://www.cecill.info.                                # 
! #                                                                      # 
! # The fact that you are presently reading this means that you have     # 
! # had knowledge of the CeCILL-C license and that you accept its        # 
! # terms.                                                               # 
! #                                                                      # 
! # Authors:                                                             # 
! #   - PERACHE Marc marc.perache@cea.fr                                 # 
! #                                                                      # 
! ######################################################################## 
       INTEGER MPC_THREAD_SINGLE
       PARAMETER (MPC_THREAD_SINGLE=0)
       INTEGER MPC_THREAD_FUNNELED
       PARAMETER (MPC_THREAD_FUNNELED=1)
       INTEGER MPC_THREAD_SERIALIZED
       PARAMETER (MPC_THREAD_SERIALIZED=2)
       INTEGER MPC_THREAD_MULTIPLE
       PARAMETER (MPC_THREAD_MULTIPLE=3)
       INTEGER MPC_COMM_WORLD
       PARAMETER (MPC_COMM_WORLD=0)
       INTEGER MPC_SUCCESS
       PARAMETER (MPC_SUCCESS=0)
       INTEGER MPC_ERR_BUFFER
       PARAMETER (MPC_ERR_BUFFER=1)
       INTEGER MPC_ERR_COUNT
       PARAMETER (MPC_ERR_COUNT=2)
       INTEGER MPC_ERR_TYPE
       PARAMETER (MPC_ERR_TYPE=3)
       INTEGER MPC_ERR_TAG
       PARAMETER (MPC_ERR_TAG=4)
       INTEGER MPC_ERR_COMM
       PARAMETER (MPC_ERR_COMM=5)
       INTEGER MPC_ERR_RANK
       PARAMETER (MPC_ERR_RANK=6)
       INTEGER MPC_ERR_ROOT
       PARAMETER (MPC_ERR_ROOT=7)
       INTEGER MPC_ERR_TRUNCATE
       PARAMETER (MPC_ERR_TRUNCATE=14)
       INTEGER MPC_ERR_GROUP
       PARAMETER (MPC_ERR_GROUP=8)
       INTEGER MPC_ERR_OP
       PARAMETER (MPC_ERR_OP=9)
       INTEGER MPC_ERR_REQUEST
       PARAMETER (MPC_ERR_REQUEST=19)
       INTEGER MPC_ERR_TOPOLOGY
       PARAMETER (MPC_ERR_TOPOLOGY=10)
       INTEGER MPC_ERR_DIMS
       PARAMETER (MPC_ERR_DIMS=11)
       INTEGER MPC_ERR_ARG
       PARAMETER (MPC_ERR_ARG=12)
       INTEGER MPC_ERR_OTHER
       PARAMETER (MPC_ERR_OTHER=15)
       INTEGER MPC_ERR_UNKNOWN
       PARAMETER (MPC_ERR_UNKNOWN=13)
       INTEGER MPC_ERR_INTERN
       PARAMETER (MPC_ERR_INTERN=16)
       INTEGER MPC_ERR_IN_STATUS
       PARAMETER (MPC_ERR_IN_STATUS=17)
       INTEGER MPC_ERR_PENDING
       PARAMETER (MPC_ERR_PENDING=18)
       INTEGER MPC_NOT_IMPLEMENTED
       PARAMETER (MPC_NOT_IMPLEMENTED=49)
       INTEGER MPC_ERR_LASTCODE
       PARAMETER (MPC_ERR_LASTCODE=50)
       INTEGER MPC_ANY_TAG
       PARAMETER (MPC_ANY_TAG=-1)
       INTEGER MPC_ANY_SOURCE
       PARAMETER (MPC_ANY_SOURCE=-1)
       INTEGER MPC_PROC_NULL
       PARAMETER (MPC_PROC_NULL=-2)
       INTEGER MPC_COMM_NULL
       PARAMETER (MPC_COMM_NULL=-1)
       INTEGER MPC_MAX_PROCESSOR_NAME
       PARAMETER (MPC_MAX_PROCESSOR_NAME=255)
       INTEGER MPC_DATATYPE_NULL
       PARAMETER (MPC_DATATYPE_NULL=-1)
       INTEGER MPC_UB
       PARAMETER (MPC_UB=-2)
       INTEGER MPC_LB
       PARAMETER (MPC_LB=-3)
       INTEGER MPC_CHAR
       PARAMETER (MPC_CHAR=0)
       INTEGER MPC_LOGICAL
       PARAMETER (MPC_LOGICAL=22)
       INTEGER MPC_BYTE
       PARAMETER (MPC_BYTE=1)
       INTEGER MPC_SHORT
       PARAMETER (MPC_SHORT=2)
       INTEGER MPC_INT
       PARAMETER (MPC_INT=3)
       INTEGER MPC_INTEGER
       PARAMETER (MPC_INTEGER=3)
       INTEGER MPC_INTEGER1
       PARAMETER (MPC_INTEGER1=24)
       INTEGER MPC_INTEGER2
       PARAMETER (MPC_INTEGER2=25)
       INTEGER MPC_INTEGER4
       PARAMETER (MPC_INTEGER4=26)
       INTEGER MPC_INTEGER8
       PARAMETER (MPC_INTEGER8=27)
       INTEGER MPC_2INT
       PARAMETER (MPC_2INT=18)
       INTEGER MPC_2INTEGER
       PARAMETER (MPC_2INTEGER=18)
       INTEGER MPC_LONG
       PARAMETER (MPC_LONG=4)
       INTEGER MPC_FLOAT
       PARAMETER (MPC_FLOAT=5)
       INTEGER MPC_DOUBLE
       PARAMETER (MPC_DOUBLE=6)
       INTEGER MPC_REAL
       PARAMETER (MPC_REAL=5)
       INTEGER MPC_DOUBLE_PRECISION
       PARAMETER (MPC_DOUBLE_PRECISION=6)
       INTEGER MPC_REAL4
       PARAMETER (MPC_REAL4=28)
       INTEGER MPC_REAL8
       PARAMETER (MPC_REAL8=29)
       INTEGER MPC_REAL16
       PARAMETER (MPC_REAL16=30)
       INTEGER MPC_COMPLEX
       PARAMETER (MPC_COMPLEX=20)
       INTEGER MPC_DOUBLE_COMPLEX
       PARAMETER (MPC_DOUBLE_COMPLEX=23)
       INTEGER MPC_2REAL
       PARAMETER (MPC_2REAL=19)
       INTEGER MPC_2DOUBLE_PRECISION
       PARAMETER (MPC_2DOUBLE_PRECISION=21)
       INTEGER MPC_BOTTOM
       EXTERNAL MPC_SUM
       EXTERNAL MPC_MAX
       EXTERNAL MPC_MIN
       EXTERNAL MPC_PROD
       EXTERNAL MPC_LAND
       EXTERNAL MPC_BAND
       EXTERNAL MPC_LOR
       EXTERNAL MPC_BOR
       EXTERNAL MPC_LXOR
       EXTERNAL MPC_BXOR
       EXTERNAL MPC_MINLOC
       EXTERNAL MPC_MAXLOC
       INTEGER MPC_STATUS_SIZE
       PARAMETER (MPC_STATUS_SIZE=5)
       INTEGER MPC_REQUEST_SIZE
       PARAMETER (MPC_REQUEST_SIZE=14)
