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
! #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # 
! #   - AUTOGENERATED by genmod.py                                       # 
! #                                                                      # 
! ######################################################################## 

! /!\ DO NOT EDIT THIS FILE IS AUTOGENERATED BY modulegen.py

module mpi_f08_constants


   use,intrinsic :: iso_c_binding
use :: mpi_f08_ctypes
use :: mpi_f08_types


	type(MPI_Status), bind(C, name="mpc_f08_status_ignore"), target :: MPI_STATUS_IGNORE
	type(MPI_Status), dimension(1), bind(C, name="mpc_f08_statuses_ignore"), target :: MPI_STATUSES_IGNORE
	integer, parameter :: MPI_MODE_RDONLY = 2
	integer, parameter :: MPI_MODE_RDWR = 8
	integer, parameter :: MPI_MODE_WRONLY = 4
	integer, parameter :: MPI_MODE_CREATE = 1
	integer, parameter :: MPI_MODE_DELETE_ON_CLOSE = 16
	integer, parameter :: MPI_MODE_UNIQUE_OPEN = 32
	integer, parameter :: MPI_MODE_EXCL = 64
	integer, parameter :: MPI_MODE_APPEND = 128
	integer, parameter :: MPI_MODE_SEQUENTIAL = 256
	integer, parameter :: MPI_FILE_NULL = 0
	integer, parameter :: MPI_MAX_DATAREP_STRING = 128
	integer, parameter :: MPI_SEEK_SET = 600
	integer, parameter :: MPI_SEEK_CUR = 602
	integer, parameter :: MPI_SEEK_END = 604
	integer, parameter :: MPIO_REQUEST_NULL = 0
	integer*8, parameter :: MPI_DISPLACEMENT_CURRENT = -54278278
	integer, parameter :: MPI_SOURCE = 1
	integer, parameter :: MPI_TAG = 2
	integer, parameter :: MPI_ERROR = 3
	integer, parameter :: MPI_OFFSET_KIND = 8
	integer, parameter :: MPI_ADDRESS_KIND = 8
	integer, parameter :: MPI_COUNT_KIND = 8
	integer, parameter :: MPI_INTEGER_KIND = 4
	integer, parameter :: MPI_DISTRIBUTE_DFLT_DARG = 100
	integer, parameter :: MPI_DISTRIBUTE_BLOCK = 101
	integer, parameter :: MPI_DISTRIBUTE_CYCLIC = 102
	integer, parameter :: MPI_DISTRIBUTE_NONE = 1
	integer, parameter :: MPI_ORDER_C = 200
	integer, parameter :: MPI_ORDER_FORTRAN = 201
	integer, parameter :: MPI_MAX_OBJECT_NAME = 256
	integer, parameter :: MPI_MAX_INFO_VAL = 1024
	integer, parameter :: MPI_MAX_INFO_KEY = 255
	integer, parameter :: MPI_MAX_PROCESSOR_NAME = 255
	integer, parameter :: MPI_MAX_NAME_STRING = 256
	integer, parameter :: MPI_MAX_PORT_NAME = 256
	integer, parameter :: MPI_MAX_ERROR_STRING = 512
	integer, parameter :: MPI_MAX_KEY_DEFINED = 7
	integer, parameter :: MPI_COMBINER_UNKNOWN = 0
	integer, parameter :: MPI_COMBINER_NAMED = 1
	integer, parameter :: MPI_COMBINER_DUP = 2
	integer, parameter :: MPI_COMBINER_CONTIGUOUS = 3
	integer, parameter :: MPI_COMBINER_VECTOR = 4
	integer, parameter :: MPI_COMBINER_HVECTOR = 5
	integer, parameter :: MPI_COMBINER_INDEXED = 6
	integer, parameter :: MPI_COMBINER_HINDEXED = 7
	integer, parameter :: MPI_COMBINER_INDEXED_BLOCK = 8
	integer, parameter :: MPI_COMBINER_HINDEXED_BLOCK = 9
	integer, parameter :: MPI_COMBINER_STRUCT = 10
	integer, parameter :: MPI_COMBINER_SUBARRAY = 11
	integer, parameter :: MPI_COMBINER_DARRAY = 12
	integer, parameter :: MPI_COMBINER_F90_REAL = 13
	integer, parameter :: MPI_COMBINER_F90_INTEGER = 15
	integer, parameter :: MPI_COMBINER_F90_COMPLEX = 14
	integer, parameter :: MPI_COMBINER_RESIZED = 16
	integer, parameter :: MPI_COMBINER_HINDEXED_INTEGER = 17
	integer, parameter :: MPI_COMBINER_STRUCT_INTEGER = 18
	integer, parameter :: MPI_COMBINER_HVECTOR_INTEGER = 19
	type(MPI_Request) :: MPI_REQUEST_NULL = MPI_Request(-1)
	type(MPI_Comm) :: MPI_COMM_NULL = MPI_Comm(0)
	type(MPI_Datatype) :: MPI_DATATYPE_NULL = MPI_Datatype(-1)
	type(MPI_Op) :: MPI_OP_NULL = MPI_Op(-1)
	type(MPI_Win) :: MPI_WIN_NULL = MPI_Win(-1)
	type(MPI_Group) :: MPI_GROUP_NULL = MPI_Group(0)
	type(MPI_Info) :: MPI_INFO_NULL = MPI_Info(-1)
	type(MPI_Errhandler) :: MPI_ERRHANDLER_NULL = MPI_Errhandler(-1)
	type(MPI_Message) :: MPI_MESSAGE_NULL = MPI_Message(-1)
	integer, parameter :: MPI_PROC_NULL = -2
	integer, parameter :: MPI_ARGV_NULL = 0
	integer, parameter :: MPI_ARGVS_NULL = 0
	integer, parameter :: MPI_UNDEFINED = -1
	type(MPI_Comm) :: MPI_COMM_WORLD = MPI_Comm(1)
	type(MPI_Comm) :: MPI_COMM_SELF = MPI_Comm(2)
	integer, parameter :: MPI_CART = -2
	integer, parameter :: MPI_GRAPH = -3
	integer, parameter :: MPI_DIST_GRAPH = -4
	integer, parameter :: MPI_UNWEIGHTED = 2
	integer, parameter :: MPI_WEIGHTS_EMPTY = 3
	integer, parameter :: MPI_IDENT = 0
	integer, parameter :: MPI_CONGRUENT = 3
	integer, parameter :: MPI_SIMILAR = 1
	integer, parameter :: MPI_UNEQUAL = 2
	integer, parameter :: MPI_ANY_TAG = -1
	integer, parameter :: MPI_ANY_SOURCE = -1
	integer, parameter :: MPI_ROOT = -4
	integer(c_int), bind(C, name="mpc_f08_in_place"), target :: MPI_IN_PLACE
	integer, parameter :: MPI_SUCCESS = 0
	integer, parameter :: MPI_ERR_BUFFER = 3
	integer, parameter :: MPI_ERR_COUNT = 4
	integer, parameter :: MPI_ERR_TYPE = 1
	integer, parameter :: MPI_ERR_TAG = 5
	integer, parameter :: MPI_ERR_COMM = 6
	integer, parameter :: MPI_ERR_RANK = 7
	integer, parameter :: MPI_ERR_REQUEST = 12
	integer, parameter :: MPI_ERR_ROOT = 8
	integer, parameter :: MPI_ERR_GROUP = 10
	integer, parameter :: MPI_ERR_OP = 11
	integer, parameter :: MPI_ERR_TOPOLOGY = 13
	integer, parameter :: MPI_ERR_DIMS = 14
	integer, parameter :: MPI_ERR_ARG = 15
	integer, parameter :: MPI_ERR_UNKNOWN = 51
	integer, parameter :: MPI_ERR_TRUNCATE = 0
	integer, parameter :: MPI_ERR_OTHER = 50
	integer, parameter :: MPI_ERR_INTERN = 52
	integer, parameter :: MPI_ERR_IN_STATUS = 35
	integer, parameter :: MPI_ERR_PENDING = 2
	integer, parameter :: MPI_ERR_KEYVAL = 36
	integer, parameter :: MPI_ERR_NO_MEM = 40
	integer, parameter :: MPI_ERR_BASE = 41
	integer, parameter :: MPI_ERR_INFO_KEY = 37
	integer, parameter :: MPI_ERR_INFO_VALUE = 38
	integer, parameter :: MPI_ERR_INFO_NOKEY = 39
	integer, parameter :: MPI_ERR_SPAWN = 42
	integer, parameter :: MPI_ERR_PORT = 16
	integer, parameter :: MPI_ERR_SERVICE = 17
	integer, parameter :: MPI_ERR_NAME = 18
	integer, parameter :: MPI_ERR_WIN = 19
	integer, parameter :: MPI_ERR_SIZE = 20
	integer, parameter :: MPI_ERR_DISP = 21
	integer, parameter :: MPI_ERR_INFO = 22
	integer, parameter :: MPI_ERR_LOCKTYPE = 23
	integer, parameter :: MPI_ERR_ASSERT = 24
	integer, parameter :: MPI_ERR_RMA_CONFLICT = 43
	integer, parameter :: MPI_ERR_RMA_SYNC = 44
	integer, parameter :: MPI_ERR_FILE = 25
	integer, parameter :: MPI_ERR_NOT_SAME = 9
	integer, parameter :: MPI_ERR_AMODE = 26
	integer, parameter :: MPI_ERR_UNSUPPORTED_DATAREP = 27
	integer, parameter :: MPI_ERR_UNSUPPORTED_OPERATION = 45
	integer, parameter :: MPI_ERR_NO_SUCH_FILE = 28
	integer, parameter :: MPI_ERR_FILE_EXISTS = 29
	integer, parameter :: MPI_ERR_BAD_FILE = 30
	integer, parameter :: MPI_ERR_ACCESS = 31
	integer, parameter :: MPI_ERR_NO_SPACE = 46
	integer, parameter :: MPI_ERR_QUOTA = 47
	integer, parameter :: MPI_ERR_READ_ONLY = 32
	integer, parameter :: MPI_ERR_FILE_IN_USE = 33
	integer, parameter :: MPI_ERR_DUP_DATAREP = 48
	integer, parameter :: MPI_ERR_CONVERSION = 49
	integer, parameter :: MPI_ERR_IO = 34
	integer, parameter :: MPI_ERR_LASTCODE = 75
	integer, parameter :: MPI_NOT_IMPLEMENTED = 54
	type(MPI_Errhandler) :: MPI_ERRORS_RETURN = MPI_Errhandler(-6)
	type(MPI_Errhandler) :: MPI_ERRORS_ARE_FATAL = MPI_Errhandler(-7)
	type(MPI_Datatype) :: MPI_UB = MPI_Datatype(-2)
	type(MPI_Datatype) :: MPI_LB = MPI_Datatype(-3)
	type(MPI_Datatype) :: MPI_CHAR = MPI_Datatype(13)
	type(MPI_Datatype) :: MPI_WCHAR = MPI_Datatype(45)
	type(MPI_Datatype) :: MPI_CHARACTER = MPI_Datatype(54)
	type(MPI_Datatype) :: MPI_BYTE = MPI_Datatype(1)
	type(MPI_Datatype) :: MPI_SHORT = MPI_Datatype(2)
	type(MPI_Datatype) :: MPI_INT = MPI_Datatype(3)
	type(MPI_Datatype) :: MPI_INTEGER = MPI_Datatype(55)
	type(MPI_Datatype) :: MPI_2INTEGER = MPI_Datatype(4083)
	type(MPI_Datatype) :: MPI_INTEGER1 = MPI_Datatype(24)
	type(MPI_Datatype) :: MPI_INTEGER2 = MPI_Datatype(25)
	type(MPI_Datatype) :: MPI_INTEGER4 = MPI_Datatype(26)
	type(MPI_Datatype) :: MPI_INTEGER8 = MPI_Datatype(27)
	type(MPI_Datatype) :: MPI_AINT = MPI_Datatype(49)
	type(MPI_Datatype) :: MPI_COMPLEX8 = MPI_Datatype(4079)
	type(MPI_Datatype) :: MPI_COMPLEX16 = MPI_Datatype(4080)
	type(MPI_Datatype) :: MPI_COMPLEX32 = MPI_Datatype(4081)
	type(MPI_Datatype) :: MPI_INT8_T = MPI_Datatype(34)
	type(MPI_Datatype) :: MPI_INT16_T = MPI_Datatype(36)
	type(MPI_Datatype) :: MPI_INT32_T = MPI_Datatype(38)
	type(MPI_Datatype) :: MPI_INT64_T = MPI_Datatype(40)
	type(MPI_Datatype) :: MPI_UINT8_T = MPI_Datatype(35)
	type(MPI_Datatype) :: MPI_UINT16_T = MPI_Datatype(37)
	type(MPI_Datatype) :: MPI_UINT32_T = MPI_Datatype(39)
	type(MPI_Datatype) :: MPI_UINT64_T = MPI_Datatype(41)
	type(MPI_Datatype) :: MPI_C_BOOL = MPI_Datatype(53)
	type(MPI_Datatype) :: MPI_C_FLOAT_COMPLEX = MPI_Datatype(4079)
	type(MPI_Datatype) :: MPI_C_COMPLEX = MPI_Datatype(4076)
	type(MPI_Datatype) :: MPI_C_DOUBLE_COMPLEX = MPI_Datatype(4080)
	type(MPI_Datatype) :: MPI_C_LONG_DOUBLE_COMPLEX = MPI_Datatype(4081)
	type(MPI_Datatype) :: MPI_LONG = MPI_Datatype(4)
	type(MPI_Datatype) :: MPI_LONG_INT = MPI_Datatype(4071)
	type(MPI_Datatype) :: MPI_FLOAT = MPI_Datatype(5)
	type(MPI_Datatype) :: MPI_DOUBLE = MPI_Datatype(6)
	type(MPI_Datatype) :: MPI_DOUBLE_PRECISION = MPI_Datatype(57)
	type(MPI_Datatype) :: MPI_UNSIGNED_CHAR = MPI_Datatype(7)
	type(MPI_Datatype) :: MPI_UNSIGNED_SHORT = MPI_Datatype(8)
	type(MPI_Datatype) :: MPI_UNSIGNED = MPI_Datatype(9)
	type(MPI_Datatype) :: MPI_UNSIGNED_LONG = MPI_Datatype(10)
	type(MPI_Datatype) :: MPI_LONG_DOUBLE = MPI_Datatype(11)
	type(MPI_Datatype) :: MPI_LONG_LONG_INT = MPI_Datatype(52)
	type(MPI_Datatype) :: MPI_LONG_LONG = MPI_Datatype(12)
	type(MPI_Datatype) :: MPI_UNSIGNED_LONG_LONG_INT = MPI_Datatype(58)
	type(MPI_Datatype) :: MPI_UNSIGNED_LONG_LONG = MPI_Datatype(33)
	type(MPI_Datatype) :: MPI_PACKED = MPI_Datatype(0)
	type(MPI_Datatype) :: MPI_FLOAT_INT = MPI_Datatype(4070)
	type(MPI_Datatype) :: MPI_DOUBLE_INT = MPI_Datatype(4072)
	type(MPI_Datatype) :: MPI_LONG_DOUBLE_INT = MPI_Datatype(4078)
	type(MPI_Datatype) :: MPI_SHORT_INT = MPI_Datatype(4073)
	type(MPI_Datatype) :: MPI_2INT = MPI_Datatype(4074)
	type(MPI_Datatype) :: MPI_2FLOAT = MPI_Datatype(4075)
	type(MPI_Datatype) :: MPI_COMPLEX = MPI_Datatype(4076)
	type(MPI_Datatype) :: MPI_DOUBLE_COMPLEX = MPI_Datatype(4082)
	type(MPI_Datatype) :: MPI_2DOUBLE_PRECISION = MPI_Datatype(4077)
	type(MPI_Datatype) :: MPI_LOGICAL = MPI_Datatype(22)
	type(MPI_Datatype) :: MPI_2REAL = MPI_Datatype(4084)
	type(MPI_Datatype) :: MPI_REAL4 = MPI_Datatype(28)
	type(MPI_Datatype) :: MPI_REAL8 = MPI_Datatype(29)
	type(MPI_Datatype) :: MPI_REAL16 = MPI_Datatype(30)
	type(MPI_Datatype) :: MPI_SIGNED_CHAR = MPI_Datatype(31)
	type(MPI_Datatype) :: MPI_REAL = MPI_Datatype(56)
	type(MPI_Datatype) :: MPI_COUNT = MPI_Datatype(51)
	type(MPI_Datatype) :: MPI_OFFSET = MPI_Datatype(50)
	type(MPI_Op) :: MPI_SUM = MPI_Op(0)
	type(MPI_Op) :: MPI_MAX = MPI_Op(1)
	type(MPI_Op) :: MPI_MIN = MPI_Op(2)
	type(MPI_Op) :: MPI_PROD = MPI_Op(3)
	type(MPI_Op) :: MPI_LAND = MPI_Op(4)
	type(MPI_Op) :: MPI_BAND = MPI_Op(5)
	type(MPI_Op) :: MPI_LOR = MPI_Op(6)
	type(MPI_Op) :: MPI_BOR = MPI_Op(7)
	type(MPI_Op) :: MPI_LXOR = MPI_Op(8)
	type(MPI_Op) :: MPI_BXOR = MPI_Op(9)
	type(MPI_Op) :: MPI_MINLOC = MPI_Op(10)
	type(MPI_Op) :: MPI_MAXLOC = MPI_Op(11)
	type(MPI_Op) :: MPI_REPLACE = MPI_Op(13)
	type(MPI_Op) :: MPI_NO_OP = MPI_Op(14)
	integer, parameter :: MPI_BOTTOM = 0
	integer, parameter :: MPI_GROUP_EMPTY = 0
	integer, parameter :: MPI_KEYVAL_INVALID = -1
	integer, parameter :: MPI_THREAD_SINGLE = 0
	integer, parameter :: MPI_THREAD_FUNNELED = 1
	integer, parameter :: MPI_THREAD_SERIALIZED = 2
	integer, parameter :: MPI_THREAD_MULTIPLE = 3
	integer, parameter :: MPI_STATUS_SIZE = 5
	integer, parameter :: MPI_REQUEST_SIZE = 1
	integer, parameter :: MPI_BSEND_OVERHEAD = 4
	integer, parameter :: MPI_VERSION = 3
	integer, parameter :: MPI_SUBVERSION = 1
	integer, parameter :: MPI_COMM_TYPE_SHARED = 1
	integer, parameter :: MPI_MESSAGE_NO_PROC = -2
	integer, parameter :: MPI_WTIME_IS_GLOBAL = 3
	integer, parameter :: MPI_UNIVERSE_SIZE = 5
	integer, parameter :: MPI_LASTUSEDCODE = 6
	integer, parameter :: MPI_APPNUM = 4
	integer, parameter :: MPI_TAG_UB = 0
	integer, parameter :: MPI_HOST = 1
	integer, parameter :: MPI_IO = 2
	integer, parameter :: MPI_WIN_BASE = 1
	integer, parameter :: MPI_WIN_SIZE = 2
	integer, parameter :: MPI_WIN_DISP_UNIT = 3
	integer, parameter :: MPI_WIN_CREATE_FLAVOR = 4
	integer, parameter :: MPI_WIN_MODEL = 0
	integer, parameter :: MPI_WIN_FLAVOR_CREATE = 1
	integer, parameter :: MPI_WIN_FLAVOR_ALLOCATE = 2
	integer, parameter :: MPI_WIN_FLAVOR_DYNAMIC = 3
	integer, parameter :: MPI_WIN_FLAVOR_SHARED = 4
	integer, parameter :: MPI_WIN_SEPARATE = 1
	integer, parameter :: MPI_WIN_UNIFIED = 0
	integer, parameter :: MPI_MODE_NOCHECK = 1
	integer, parameter :: MPI_MODE_NOSTORE = 2
	integer, parameter :: MPI_MODE_NOPUT = 4
	integer, parameter :: MPI_MODE_NOPRECEDE = 8
	integer, parameter :: MPI_MODE_NOSUCCEED = 16
	integer, parameter :: MPI_LOCK_EXCLUSIVE = 234
	integer, parameter :: MPI_LOCK_SHARED = 235


end module mpi_f08_constants
