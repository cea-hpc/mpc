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
       INTEGER MPC_SUCCESS
       PARAMETER (MPC_SUCCESS=0)
       INTEGER MPC_UNDEFINED
       PARAMETER (MPC_UNDEFINED=-1)
       INTEGER MPC_REQUEST_NULL
       PARAMETER (MPC_REQUEST_NULL=-1)
       INTEGER MPC_COMM_WORLD
       PARAMETER (MPC_COMM_WORLD=0)
       INTEGER MPC_STATUS_IGNORE
       PARAMETER (MPC_STATUS_IGNORE=0)
       INTEGER MPC_STATUSES_IGNORE
       PARAMETER (MPC_STATUSES_IGNORE=0)
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
       INTEGER MPC_MAX_NAME_STRING
       PARAMETER (MPC_MAX_NAME_STRING=256)
       INTEGER MPC_ROOT
       PARAMETER (MPC_ROOT=-4)
       INTEGER MPC_IN_PLACE
       PARAMETER (MPC_IN_PLACE=-1)
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
       INTEGER MPC_ERR_REQUEST
       PARAMETER (MPC_ERR_REQUEST=7)
       INTEGER MPC_ERR_ROOT
       PARAMETER (MPC_ERR_ROOT=8)
       INTEGER MPC_ERR_GROUP
       PARAMETER (MPC_ERR_GROUP=9)
       INTEGER MPC_ERR_OP
       PARAMETER (MPC_ERR_OP=10)
       INTEGER MPC_ERR_TOPOLOGY
       PARAMETER (MPC_ERR_TOPOLOGY=11)
       INTEGER MPC_ERR_DIMS
       PARAMETER (MPC_ERR_DIMS=12)
       INTEGER MPC_ERR_ARG
       PARAMETER (MPC_ERR_ARG=13)
       INTEGER MPC_ERR_UNKNOWN
       PARAMETER (MPC_ERR_UNKNOWN=14)
       INTEGER MPC_ERR_TRUNCATE
       PARAMETER (MPC_ERR_TRUNCATE=15)
       INTEGER MPC_ERR_OTHER
       PARAMETER (MPC_ERR_OTHER=16)
       INTEGER MPC_ERR_INTERN
       PARAMETER (MPC_ERR_INTERN=17)
       INTEGER MPC_ERR_IN_STATUS
       PARAMETER (MPC_ERR_IN_STATUS=18)
       INTEGER MPC_ERR_PENDING
       PARAMETER (MPC_ERR_PENDING=19)
       INTEGER MPC_ERR_KEYVAL
       PARAMETER (MPC_ERR_KEYVAL=20)
       INTEGER MPC_ERR_NO_MEM
       PARAMETER (MPC_ERR_NO_MEM=21)
       INTEGER MPC_ERR_BASE
       PARAMETER (MPC_ERR_BASE=22)
       INTEGER MPC_ERR_INFO_KEY
       PARAMETER (MPC_ERR_INFO_KEY=23)
       INTEGER MPC_ERR_INFO_VALUE
       PARAMETER (MPC_ERR_INFO_VALUE=24)
       INTEGER MPC_ERR_INFO_NOKEY
       PARAMETER (MPC_ERR_INFO_NOKEY=25)
       INTEGER MPC_ERR_SPAWN
       PARAMETER (MPC_ERR_SPAWN=26)
       INTEGER MPC_ERR_PORT
       PARAMETER (MPC_ERR_PORT=27)
       INTEGER MPC_ERR_SERVICE
       PARAMETER (MPC_ERR_SERVICE=28)
       INTEGER MPC_ERR_NAME
       PARAMETER (MPC_ERR_NAME=29)
       INTEGER MPC_ERR_WIN
       PARAMETER (MPC_ERR_WIN=30)
       INTEGER MPC_ERR_SIZE
       PARAMETER (MPC_ERR_SIZE=31)
       INTEGER MPC_ERR_DISP
       PARAMETER (MPC_ERR_DISP=32)
       INTEGER MPC_ERR_INFO
       PARAMETER (MPC_ERR_INFO=33)
       INTEGER MPC_ERR_LOCKTYPE
       PARAMETER (MPC_ERR_LOCKTYPE=34)
       INTEGER MPC_ERR_ASSERT
       PARAMETER (MPC_ERR_ASSERT=35)
       INTEGER MPC_ERR_RMA_CONFLICT
       PARAMETER (MPC_ERR_RMA_CONFLICT=36)
       INTEGER MPC_ERR_RMA_SYNC
       PARAMETER (MPC_ERR_RMA_SYNC=37)
       INTEGER MPC_ERR_FILE
       PARAMETER (MPC_ERR_FILE=38)
       INTEGER MPC_ERR_NOT_SAME
       PARAMETER (MPC_ERR_NOT_SAME=39)
       INTEGER MPC_ERR_AMODE
       PARAMETER (MPC_ERR_AMODE=40)
       INTEGER MPC_ERR_UNSUPPORTED_DATAREP
       PARAMETER (MPC_ERR_UNSUPPORTED_DATAREP=41)
       INTEGER MPC_ERR_UNSUPPORTED_OPERATION
       PARAMETER (MPC_ERR_UNSUPPORTED_OPERATION=42)
       INTEGER MPC_ERR_NO_SUCH_FILE
       PARAMETER (MPC_ERR_NO_SUCH_FILE=43)
       INTEGER MPC_ERR_FILE_EXISTS
       PARAMETER (MPC_ERR_FILE_EXISTS=44)
       INTEGER MPC_ERR_BAD_FILE
       PARAMETER (MPC_ERR_BAD_FILE=45)
       INTEGER MPC_ERR_ACCESS
       PARAMETER (MPC_ERR_ACCESS=46)
       INTEGER MPC_ERR_NO_SPACE
       PARAMETER (MPC_ERR_NO_SPACE=47)
       INTEGER MPC_ERR_QUOTA
       PARAMETER (MPC_ERR_QUOTA=48)
       INTEGER MPC_ERR_READ_ONLY
       PARAMETER (MPC_ERR_READ_ONLY=49)
       INTEGER MPC_ERR_FILE_IN_USE
       PARAMETER (MPC_ERR_FILE_IN_USE=50)
       INTEGER MPC_ERR_DUP_DATAREP
       PARAMETER (MPC_ERR_DUP_DATAREP=51)
       INTEGER MPC_ERR_CONVERSION
       PARAMETER (MPC_ERR_CONVERSION=52)
       INTEGER MPC_ERR_IO
       PARAMETER (MPC_ERR_IO=53)
       INTEGER MPC_ERR_LASTCODE
       PARAMETER (MPC_ERR_LASTCODE=55)
       INTEGER MPC_NOT_IMPLEMENTED
       PARAMETER (MPC_NOT_IMPLEMENTED=56)
       INTEGER MPC_DATATYPE_NULL
       PARAMETER (MPC_DATATYPE_NULL=-1)
       INTEGER MPC_UB
       PARAMETER (MPC_UB=-2)
       INTEGER MPC_LB
       PARAMETER (MPC_LB=-3)
       INTEGER MPC_CHAR
       PARAMETER (MPC_CHAR=0)
       INTEGER MPC_BYTE
       PARAMETER (MPC_BYTE=1)
       INTEGER MPC_SHORT
       PARAMETER (MPC_SHORT=2)
       INTEGER MPC_INT
       PARAMETER (MPC_INT=3)
       INTEGER MPC_INTEGER
       PARAMETER (MPC_INTEGER=3)
       INTEGER MPC_2INTEGER
       PARAMETER (MPC_2INTEGER=18)
       INTEGER MPC_INTEGER1
       PARAMETER (MPC_INTEGER1=24)
       INTEGER MPC_INTEGER2
       PARAMETER (MPC_INTEGER2=25)
       INTEGER MPC_INTEGER4
       PARAMETER (MPC_INTEGER4=26)
       INTEGER MPC_INTEGER8
       PARAMETER (MPC_INTEGER8=27)
       INTEGER MPC_LONG
       PARAMETER (MPC_LONG=4)
       INTEGER MPC_LONG_INT
       PARAMETER (MPC_LONG_INT=15)
       INTEGER MPC_FLOAT
       PARAMETER (MPC_FLOAT=5)
       INTEGER MPC_DOUBLE
       PARAMETER (MPC_DOUBLE=6)
       INTEGER MPC_DOUBLE_PRECISION
       PARAMETER (MPC_DOUBLE_PRECISION=6)
       INTEGER MPC_UNSIGNED_CHAR
       PARAMETER (MPC_UNSIGNED_CHAR=7)
       INTEGER MPC_UNSIGNED_SHORT
       PARAMETER (MPC_UNSIGNED_SHORT=8)
       INTEGER MPC_UNSIGNED
       PARAMETER (MPC_UNSIGNED=9)
       INTEGER MPC_UNSIGNED_LONG
       PARAMETER (MPC_UNSIGNED_LONG=10)
       INTEGER MPC_LONG_DOUBLE
       PARAMETER (MPC_LONG_DOUBLE=11)
       INTEGER MPC_LONG_LONG_INT
       PARAMETER (MPC_LONG_LONG_INT=12)
       INTEGER MPC_LONG_LONG
       PARAMETER (MPC_LONG_LONG=12)
       INTEGER MPC_UNSIGNED_LONG_LONG_INT
       PARAMETER (MPC_UNSIGNED_LONG_LONG_INT=33)
       INTEGER MPC_UNSIGNED_LONG_LONG
       PARAMETER (MPC_UNSIGNED_LONG_LONG=33)
       INTEGER MPC_PACKED
       PARAMETER (MPC_PACKED=13)
       INTEGER MPC_FLOAT_INT
       PARAMETER (MPC_FLOAT_INT=14)
       INTEGER MPC_DOUBLE_INT
       PARAMETER (MPC_DOUBLE_INT=16)
       INTEGER MPC_LONG_DOUBLE_INT
       PARAMETER (MPC_LONG_DOUBLE_INT=32)
       INTEGER MPC_SHORT_INT
       PARAMETER (MPC_SHORT_INT=17)
       INTEGER MPC_2INT
       PARAMETER (MPC_2INT=18)
       INTEGER MPC_2FLOAT
       PARAMETER (MPC_2FLOAT=19)
       INTEGER MPC_COMPLEX
       PARAMETER (MPC_COMPLEX=20)
       INTEGER MPC_DOUBLE_COMPLEX
       PARAMETER (MPC_DOUBLE_COMPLEX=23)
       INTEGER MPC_2DOUBLE_PRECISION
       PARAMETER (MPC_2DOUBLE_PRECISION=21)
       INTEGER MPC_LOGICAL
       PARAMETER (MPC_LOGICAL=22)
       INTEGER MPC_2REAL
       PARAMETER (MPC_2REAL=19)
       INTEGER MPC_REAL4
       PARAMETER (MPC_REAL4=28)
       INTEGER MPC_REAL8
       PARAMETER (MPC_REAL8=29)
       INTEGER MPC_REAL16
       PARAMETER (MPC_REAL16=30)
       INTEGER MPC_SIGNED_CHAR
       PARAMETER (MPC_SIGNED_CHAR=31)
       INTEGER MPC_REAL
       PARAMETER (MPC_REAL=5)
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
       EXTERNAL MPC_OP_NULL
       INTEGER MPC_BOTTOM
       EXTERNAL MPC_GROUP_EMPTY
       EXTERNAL MPC_GROUP_NULL
       EXTERNAL MPC_INFO_NULL
       EXTERNAL MPC_FILE_NULL
       EXTERNAL MPC_WIN_NULL
       EXTERNAL MPC_MAX_ERROR_STRING
       EXTERNAL MPC_ERRHANDLER_NULL
       EXTERNAL MPC_ERRORS_RETURN
       EXTERNAL MPC_ERRORS_ARE_FATAL
       EXTERNAL MPC_KEYVAL_INVALID
       INTEGER MPC_COMM_SELF
       PARAMETER (MPC_COMM_SELF=1)
       INTEGER MPC_MAX_KEY_DEFINED
       PARAMETER (MPC_MAX_KEY_DEFINED=7)
       INTEGER MPC_TAG_UB
       PARAMETER (MPC_TAG_UB=0)
       INTEGER MPC_HOST
       PARAMETER (MPC_HOST=1)
       INTEGER MPC_IO
       PARAMETER (MPC_IO=2)
       INTEGER MPC_WTIME_IS_GLOBAL
       PARAMETER (MPC_WTIME_IS_GLOBAL=3)
       INTEGER MPC_THREAD_SINGLE
       PARAMETER (MPC_THREAD_SINGLE=0)
       INTEGER MPC_THREAD_FUNNELED
       PARAMETER (MPC_THREAD_FUNNELED=1)
       INTEGER MPC_THREAD_SERIALIZED
       PARAMETER (MPC_THREAD_SERIALIZED=2)
       INTEGER MPC_THREAD_MULTIPLE
       PARAMETER (MPC_THREAD_MULTIPLE=3)
       INTEGER MPC_STATUS_SIZE
       PARAMETER (MPC_STATUS_SIZE=5)
       INTEGER MPC_REQUEST_SIZE
       PARAMETER (MPC_REQUEST_SIZE=14)
