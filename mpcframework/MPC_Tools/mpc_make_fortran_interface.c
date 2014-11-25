/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <mpcmp.h>
#include <assert.h>

#define MPC_INTEGER_VAL(sa,a) printf("       INTEGER %s\n       PARAMETER (%s=%d)\n",sa,sa,(int)a)

#define MPC_INTEGER_8_VAL(sa,a) printf("       INTEGER*8 %s\n       PARAMETER (%s=%ld)\n",sa,sa,(long)a)

#define MPC_POINTER_CONV_VAL(sa,a) printf("       EXTERNAL %s\n",sa)

int
main_mpc (int argc, char **argv)
{

  printf
    ("\
! ############################# MPC License ############################## \n\
! # Wed Nov 19 15:19:19 CET 2008                                         # \n\
! # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # \n\
! #                                                                      # \n\
! # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # \n\
! # This file is part of the MPC Runtime.                                # \n\
! #                                                                      # \n\
! # This software is governed by the CeCILL-C license under French law   # \n\
! # and abiding by the rules of distribution of free software.  You can  # \n\
! # use, modify and/ or redistribute the software under the terms of     # \n\
! # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # \n\
! # following URL http://www.cecill.info.                                # \n\
! #                                                                      # \n\
! # The fact that you are presently reading this means that you have     # \n\
! # had knowledge of the CeCILL-C license and that you accept its        # \n\
! # terms.                                                               # \n\
! #                                                                      # \n\
! # Authors:                                                             # \n\
! #   - PERACHE Marc marc.perache@cea.fr                                 # \n\
! #                                                                      # \n\
! ######################################################################## \n");

  MPC_INTEGER_VAL ("MPC_SUCCESS", MPC_SUCCESS);
  MPC_INTEGER_VAL ("MPC_UNDEFINED", MPC_UNDEFINED);
  MPC_INTEGER_VAL ("MPC_REQUEST_NULL", -1);
  MPC_INTEGER_VAL ("MPC_COMM_WORLD", MPC_COMM_WORLD);

  MPC_INTEGER_VAL ("MPC_STATUS_IGNORE", (size_t)MPC_STATUS_IGNORE);
  MPC_INTEGER_VAL ("MPC_STATUSES_IGNORE", (size_t)MPC_STATUSES_IGNORE);
  MPC_INTEGER_VAL ("MPC_ANY_TAG", MPC_ANY_TAG);
  MPC_INTEGER_VAL ("MPC_ANY_SOURCE", MPC_ANY_SOURCE);
  MPC_INTEGER_VAL ("MPC_PROC_NULL", MPC_PROC_NULL);
  MPC_INTEGER_VAL ("MPC_COMM_NULL", MPC_COMM_NULL);
  MPC_INTEGER_VAL ("MPC_MAX_PROCESSOR_NAME", MPC_MAX_PROCESSOR_NAME);
  MPC_INTEGER_VAL ("MPC_MAX_NAME_STRING", 256);
  MPC_INTEGER_VAL ("MPC_ROOT", MPC_ROOT);
  MPC_INTEGER_VAL ("MPC_IN_PLACE", (size_t)MPC_IN_PLACE);

  /* Communication argument parameters */
  MPC_INTEGER_VAL ("MPC_ERR_BUFFER", MPC_ERR_BUFFER);
  MPC_INTEGER_VAL ("MPC_ERR_COUNT", MPC_ERR_COUNT);
  MPC_INTEGER_VAL ("MPC_ERR_TYPE", MPC_ERR_TYPE);
  MPC_INTEGER_VAL ("MPC_ERR_TAG", MPC_ERR_TAG);
  MPC_INTEGER_VAL ("MPC_ERR_COMM", MPC_ERR_COMM);
  MPC_INTEGER_VAL ("MPC_ERR_RANK", MPC_ERR_RANK);
  MPC_INTEGER_VAL ("MPC_ERR_REQUEST", MPC_ERR_REQUEST);
  MPC_INTEGER_VAL ("MPC_ERR_ROOT", MPC_ERR_ROOT);
  MPC_INTEGER_VAL ("MPC_ERR_GROUP", MPC_ERR_GROUP);
  MPC_INTEGER_VAL ("MPC_ERR_OP", MPC_ERR_OP);
  MPC_INTEGER_VAL ("MPC_ERR_TOPOLOGY", MPC_ERR_TOPOLOGY);
  MPC_INTEGER_VAL ("MPC_ERR_DIMS", MPC_ERR_DIMS);
  MPC_INTEGER_VAL ("MPC_ERR_ARG", MPC_ERR_ARG);
  MPC_INTEGER_VAL ("MPC_ERR_UNKNOWN", MPC_ERR_UNKNOWN);
  MPC_INTEGER_VAL ("MPC_ERR_TRUNCATE", MPC_ERR_TRUNCATE);
  MPC_INTEGER_VAL ("MPC_ERR_OTHER", MPC_ERR_OTHER);
  MPC_INTEGER_VAL ("MPC_ERR_INTERN", MPC_ERR_INTERN);
  MPC_INTEGER_VAL ("MPC_ERR_IN_STATUS", MPC_ERR_IN_STATUS);
  MPC_INTEGER_VAL ("MPC_ERR_PENDING", MPC_ERR_PENDING);
  MPC_INTEGER_VAL ("MPC_ERR_KEYVAL", MPC_ERR_KEYVAL);
  MPC_INTEGER_VAL ("MPC_ERR_NO_MEM", MPC_ERR_NO_MEM);
  MPC_INTEGER_VAL ("MPC_ERR_BASE", MPC_ERR_BASE);
  MPC_INTEGER_VAL ("MPC_ERR_INFO_KEY", MPC_ERR_INFO_KEY);
  MPC_INTEGER_VAL ("MPC_ERR_INFO_VALUE", MPC_ERR_INFO_VALUE);
  MPC_INTEGER_VAL ("MPC_ERR_INFO_NOKEY", MPC_ERR_INFO_NOKEY);
  MPC_INTEGER_VAL ("MPC_ERR_SPAWN", MPC_ERR_SPAWN);
  MPC_INTEGER_VAL ("MPC_ERR_PORT", MPC_ERR_PORT);
  MPC_INTEGER_VAL ("MPC_ERR_SERVICE", MPC_ERR_SERVICE);
  MPC_INTEGER_VAL ("MPC_ERR_NAME", MPC_ERR_NAME);
  MPC_INTEGER_VAL ("MPC_ERR_WIN", MPC_ERR_WIN);
  MPC_INTEGER_VAL ("MPC_ERR_SIZE", MPC_ERR_SIZE);
  MPC_INTEGER_VAL ("MPC_ERR_DISP", MPC_ERR_DISP);
  MPC_INTEGER_VAL ("MPC_ERR_INFO", MPC_ERR_INFO);
  MPC_INTEGER_VAL ("MPC_ERR_LOCKTYPE", MPC_ERR_LOCKTYPE);
  MPC_INTEGER_VAL ("MPC_ERR_ASSERT", MPC_ERR_ASSERT);
  MPC_INTEGER_VAL ("MPC_ERR_RMA_CONFLICT", MPC_ERR_RMA_CONFLICT);
  MPC_INTEGER_VAL ("MPC_ERR_RMA_SYNC", MPC_ERR_RMA_SYNC);
  MPC_INTEGER_VAL ("MPC_ERR_FILE", MPC_ERR_FILE);
  MPC_INTEGER_VAL ("MPC_ERR_NOT_SAME", MPC_ERR_NOT_SAME);
  MPC_INTEGER_VAL ("MPC_ERR_AMODE", MPC_ERR_AMODE);
  MPC_INTEGER_VAL ("MPC_ERR_UNSUPPORTED_DATAREP", MPC_ERR_UNSUPPORTED_DATAREP);
  MPC_INTEGER_VAL ("MPC_ERR_UNSUPPORTED_OPERATION", MPC_ERR_UNSUPPORTED_OPERATION);
  MPC_INTEGER_VAL ("MPC_ERR_NO_SUCH_FILE", MPC_ERR_NO_SUCH_FILE);
  MPC_INTEGER_VAL ("MPC_ERR_FILE_EXISTS", MPC_ERR_FILE_EXISTS);
  MPC_INTEGER_VAL ("MPC_ERR_BAD_FILE", MPC_ERR_BAD_FILE);
  MPC_INTEGER_VAL ("MPC_ERR_ACCESS", MPC_ERR_ACCESS);
  MPC_INTEGER_VAL ("MPC_ERR_NO_SPACE", MPC_ERR_NO_SPACE);
  MPC_INTEGER_VAL ("MPC_ERR_QUOTA", MPC_ERR_QUOTA);
  MPC_INTEGER_VAL ("MPC_ERR_READ_ONLY", MPC_ERR_READ_ONLY);
  MPC_INTEGER_VAL ("MPC_ERR_FILE_IN_USE", MPC_ERR_FILE_IN_USE);
  MPC_INTEGER_VAL ("MPC_ERR_DUP_DATAREP", MPC_ERR_DUP_DATAREP);
  MPC_INTEGER_VAL ("MPC_ERR_CONVERSION", MPC_ERR_CONVERSION);
  MPC_INTEGER_VAL ("MPC_ERR_IO", MPC_ERR_IO);
  MPC_INTEGER_VAL ("MPC_ERR_LASTCODE", MPC_ERR_LASTCODE);

  MPC_INTEGER_VAL ("MPC_NOT_IMPLEMENTED", MPC_NOT_IMPLEMENTED);

  MPC_INTEGER_VAL ("MPC_DATATYPE_NULL", MPC_DATATYPE_NULL);
  MPC_INTEGER_VAL ("MPC_UB", MPC_UB);
  MPC_INTEGER_VAL ("MPC_LB", MPC_LB);
  MPC_INTEGER_VAL ("MPC_CHAR", MPC_CHAR);
  MPC_INTEGER_VAL ("MPC_BYTE", MPC_BYTE);
  MPC_INTEGER_VAL ("MPC_SHORT", MPC_SHORT);
  MPC_INTEGER_VAL ("MPC_INT", MPC_INT);
  MPC_INTEGER_VAL ("MPC_INTEGER", MPC_INT);
  MPC_INTEGER_VAL ("MPC_2INTEGER", MPC_2INT);
  MPC_INTEGER_VAL ("MPC_INTEGER1", MPC_INTEGER1);
  MPC_INTEGER_VAL ("MPC_INTEGER2", MPC_INTEGER2);
  MPC_INTEGER_VAL ("MPC_INTEGER4", MPC_INTEGER4);
  MPC_INTEGER_VAL ("MPC_INTEGER8", MPC_INTEGER8);
  MPC_INTEGER_VAL ("MPC_LONG", MPC_LONG);
  MPC_INTEGER_VAL ("MPC_LONG_INT", MPC_LONG_INT);
  MPC_INTEGER_VAL ("MPC_FLOAT", MPC_FLOAT);
  MPC_INTEGER_VAL ("MPC_DOUBLE", MPC_DOUBLE);
  MPC_INTEGER_VAL ("MPC_DOUBLE_PRECISION", MPC_DOUBLE);
  MPC_INTEGER_VAL ("MPC_UNSIGNED_CHAR", MPC_UNSIGNED_CHAR);
  MPC_INTEGER_VAL ("MPC_UNSIGNED_SHORT", MPC_UNSIGNED_SHORT);
  MPC_INTEGER_VAL ("MPC_UNSIGNED", MPC_UNSIGNED);
  MPC_INTEGER_VAL ("MPC_UNSIGNED_LONG", MPC_UNSIGNED_LONG);
  MPC_INTEGER_VAL ("MPC_LONG_DOUBLE", MPC_LONG_DOUBLE);
  MPC_INTEGER_VAL ("MPC_LONG_LONG_INT", MPC_LONG_LONG_INT);
  MPC_INTEGER_VAL ("MPC_LONG_LONG", MPC_LONG_LONG);
  MPC_INTEGER_VAL ("MPC_UNSIGNED_LONG_LONG_INT", MPC_UNSIGNED_LONG_LONG_INT);
  MPC_INTEGER_VAL ("MPC_UNSIGNED_LONG_LONG", MPC_UNSIGNED_LONG_LONG);
  MPC_INTEGER_VAL ("MPC_PACKED", MPC_PACKED);
  MPC_INTEGER_VAL ("MPC_FLOAT_INT", MPC_FLOAT_INT);
  MPC_INTEGER_VAL ("MPC_DOUBLE_INT", MPC_DOUBLE_INT);
  MPC_INTEGER_VAL ("MPC_LONG_DOUBLE_INT", MPC_LONG_DOUBLE_INT);
  MPC_INTEGER_VAL ("MPC_SHORT_INT", MPC_SHORT_INT);
  MPC_INTEGER_VAL ("MPC_2INT", MPC_2INT);
  MPC_INTEGER_VAL ("MPC_2FLOAT", MPC_2FLOAT);
  MPC_INTEGER_VAL ("MPC_COMPLEX", MPC_COMPLEX);
  MPC_INTEGER_VAL ("MPC_DOUBLE_COMPLEX", MPC_DOUBLE_COMPLEX);
  MPC_INTEGER_VAL ("MPC_2DOUBLE_PRECISION", MPC_2DOUBLE_PRECISION);
  MPC_INTEGER_VAL ("MPC_LOGICAL", MPC_LOGICAL);
  MPC_INTEGER_VAL ("MPC_2REAL", MPC_2FLOAT);
  MPC_INTEGER_VAL ("MPC_REAL4", MPC_REAL4);
  MPC_INTEGER_VAL ("MPC_REAL8", MPC_REAL8);
  MPC_INTEGER_VAL ("MPC_REAL16", MPC_REAL16);
  MPC_INTEGER_VAL ("MPC_SIGNED_CHAR", MPC_SIGNED_CHAR);
  MPC_INTEGER_VAL ("MPC_REAL", MPC_FLOAT);

  MPC_POINTER_CONV_VAL ("MPC_SUM", MPC_SUM);
  MPC_POINTER_CONV_VAL ("MPC_MAX", MPC_MAX);
  MPC_POINTER_CONV_VAL ("MPC_MIN", MPC_MIN);
  MPC_POINTER_CONV_VAL ("MPC_PROD", MPC_PROD);
  MPC_POINTER_CONV_VAL ("MPC_LAND", MPC_LAND);
  MPC_POINTER_CONV_VAL ("MPC_BAND", MPC_BAND);
  MPC_POINTER_CONV_VAL ("MPC_LOR", MPC_LOR);
  MPC_POINTER_CONV_VAL ("MPC_BOR", MPC_BOR);
  MPC_POINTER_CONV_VAL ("MPC_LXOR", MPC_LXOR);
  MPC_POINTER_CONV_VAL ("MPC_BXOR", MPC_BXOR);
  MPC_POINTER_CONV_VAL ("MPC_MINLOC", MPC_MINLOC);
  MPC_POINTER_CONV_VAL ("MPC_MAXLOC", MPC_MAXLOC);
  MPC_POINTER_CONV_VAL ("MPC_OP_NULL", ((MPC_Op)-1));

  printf ("       INTEGER MPC_BOTTOM\n");

  MPC_POINTER_CONV_VAL ("MPC_GROUP_EMPTY", ((MPC_Group)0));
  MPC_POINTER_CONV_VAL ("MPC_GROUP_NULL", ((MPC_Group)-1));

/* Other Null Handles */
  MPC_POINTER_CONV_VAL ("MPC_INFO_NULL", ((MPC_Info)-1));
  MPC_POINTER_CONV_VAL ("MPC_FILE_NULL", ((MPC_File)-1));
  MPC_POINTER_CONV_VAL ("MPC_WIN_NULL", ((MPC_Win)-1));

  MPC_POINTER_CONV_VAL ("MPC_MAX_ERROR_STRING", MPC_MAX_ERROR_STRING);

  MPC_POINTER_CONV_VAL ("MPC_ERRHANDLER_NULL", ((MPC_Errhandler)0));
  MPC_POINTER_CONV_VAL ("MPC_ERRORS_RETURN", 1);
  MPC_POINTER_CONV_VAL ("MPC_ERRORS_ARE_FATAL", 2);

  MPC_POINTER_CONV_VAL ("MPC_KEYVAL_INVALID", -1);

  MPC_INTEGER_VAL ("MPC_COMM_SELF", MPC_COMM_SELF);

  MPC_INTEGER_VAL ("MPC_MAX_KEY_DEFINED", 7);
  MPC_INTEGER_VAL ("MPC_TAG_UB", 0);
  MPC_INTEGER_VAL ("MPC_HOST", 1);
  MPC_INTEGER_VAL ("MPC_IO", 2);
  MPC_INTEGER_VAL ("MPC_WTIME_IS_GLOBAL", 3);

  /* Thread level */
  MPC_INTEGER_VAL ("MPC_THREAD_SINGLE", MPC_THREAD_SINGLE);
  MPC_INTEGER_VAL ("MPC_THREAD_FUNNELED", MPC_THREAD_FUNNELED);
  MPC_INTEGER_VAL ("MPC_THREAD_SERIALIZED", MPC_THREAD_SERIALIZED);
  MPC_INTEGER_VAL ("MPC_THREAD_MULTIPLE", MPC_THREAD_MULTIPLE);

  /*Architecture dependent part */
  MPC_INTEGER_VAL ("MPC_STATUS_SIZE", (int) (sizeof (MPC_Status) / 4));
  assert (sizeof (MPC_Status) % 4 == 0);
  MPC_INTEGER_VAL ("MPC_REQUEST_SIZE", (int) (sizeof (MPC_Request) / 4));
  assert (sizeof (MPC_Request) % 4 == 0);

  return 0;
}

#define MPC_MPI

#ifdef MPC_MPI
#include <mpc_mpi.h>

#define MPI_INTEGER_VAL(sa,a) printf("       INTEGER %s\n       PARAMETER (%s=%d)\n",sa,sa,(int)a)

#define MPI_INTEGER_8_VAL(sa,a) printf("       INTEGER*8 %s\n       PARAMETER (%s=%ld)\n",sa,sa,(long)a)

#define MPI_POINTER_CONV_VAL(sa,a) printf("       EXTERNAL %s\n",sa)

int
main_mpi (int argc, char **argv)
{

  printf
    ("\
! ############################# MPC License ############################## \n\
! # Wed Nov 19 15:19:19 CET 2008                                         # \n\
! # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # \n\
! #                                                                      # \n\
! # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # \n\
! # This file is part of the MPC Runtime.                                # \n\
! #                                                                      # \n\
! # This software is governed by the CeCILL-C license under French law   # \n\
! # and abiding by the rules of distribution of free software.  You can  # \n\
! # use, modify and/ or redistribute the software under the terms of     # \n\
! # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # \n\
! # following URL http://www.cecill.info.                                # \n\
! #                                                                      # \n\
! # The fact that you are presently reading this means that you have     # \n\
! # had knowledge of the CeCILL-C license and that you accept its        # \n\
! # terms.                                                               # \n\
! #                                                                      # \n\
! # Authors:                                                             # \n\
! #   - PERACHE Marc marc.perache@cea.fr                                 # \n\
! #                                                                      # \n\
! ######################################################################## \n");

  MPI_INTEGER_VAL ("MPI_SOURCE", 1);
  MPI_INTEGER_VAL ("MPI_TAG", 2);
  MPI_INTEGER_VAL ("MPI_ERROR", 3);

  MPI_INTEGER_VAL ("MPI_SUCCESS", MPI_SUCCESS);
  MPI_INTEGER_VAL ("MPI_UNDEFINED", MPI_UNDEFINED);
  MPI_INTEGER_VAL ("MPI_REQUEST_NULL", -1);
  MPI_INTEGER_VAL ("MPI_COMM_WORLD", MPI_COMM_WORLD);
  MPI_INTEGER_VAL ("MPI_CART", MPI_CART);
  MPI_INTEGER_VAL ("MPI_GRAPH", MPI_GRAPH);

  /* Results of the compare operations. */
  MPI_INTEGER_VAL ("MPI_IDENT", MPI_IDENT);
  MPI_INTEGER_VAL ("MPI_CONGRUENT", MPI_CONGRUENT);
  MPI_INTEGER_VAL ("MPI_SIMILAR", MPI_SIMILAR);
  MPI_INTEGER_VAL ("MPI_UNEQUAL", MPI_UNEQUAL);

  MPI_INTEGER_VAL ("MPI_STATUS_IGNORE", (size_t)MPI_STATUS_IGNORE);
  MPI_INTEGER_VAL ("MPI_STATUSES_IGNORE", (size_t)MPI_STATUSES_IGNORE);
  MPI_INTEGER_VAL ("MPI_ANY_TAG", MPI_ANY_TAG);
  MPI_INTEGER_VAL ("MPI_ANY_SOURCE", MPI_ANY_SOURCE);
  MPI_INTEGER_VAL ("MPI_PROC_NULL", MPI_PROC_NULL);
  MPI_INTEGER_VAL ("MPI_COMM_NULL", MPI_COMM_NULL);
  MPI_INTEGER_VAL ("MPI_MAX_PROCESSOR_NAME", MPI_MAX_PROCESSOR_NAME);
  MPI_INTEGER_VAL ("MPI_MAX_NAME_STRING", 256);
  MPI_INTEGER_VAL ("MPI_ROOT", MPI_ROOT);
  MPI_INTEGER_VAL ("MPI_IN_PLACE", (size_t)MPI_IN_PLACE);

  /* Communication argument parameters */
  MPI_INTEGER_VAL ("MPI_ERR_BUFFER", MPI_ERR_BUFFER);
  MPI_INTEGER_VAL ("MPI_ERR_COUNT", MPI_ERR_COUNT);
  MPI_INTEGER_VAL ("MPI_ERR_TYPE", MPI_ERR_TYPE);
  MPI_INTEGER_VAL ("MPI_ERR_TAG", MPI_ERR_TAG);
  MPI_INTEGER_VAL ("MPI_ERR_COMM", MPI_ERR_COMM);
  MPI_INTEGER_VAL ("MPI_ERR_RANK", MPI_ERR_RANK);
  MPI_INTEGER_VAL ("MPI_ERR_REQUEST", MPI_ERR_REQUEST);
  MPI_INTEGER_VAL ("MPI_ERR_ROOT", MPI_ERR_ROOT);
  MPI_INTEGER_VAL ("MPI_ERR_GROUP", MPI_ERR_GROUP);
  MPI_INTEGER_VAL ("MPI_ERR_OP", MPI_ERR_OP);
  MPI_INTEGER_VAL ("MPI_ERR_TOPOLOGY", MPI_ERR_TOPOLOGY);
  MPI_INTEGER_VAL ("MPI_ERR_DIMS", MPI_ERR_DIMS);
  MPI_INTEGER_VAL ("MPI_ERR_ARG", MPI_ERR_ARG);
  MPI_INTEGER_VAL ("MPI_ERR_UNKNOWN", MPI_ERR_UNKNOWN);
  MPI_INTEGER_VAL ("MPI_ERR_TRUNCATE", MPI_ERR_TRUNCATE);
  MPI_INTEGER_VAL ("MPI_ERR_OTHER", MPI_ERR_OTHER);
  MPI_INTEGER_VAL ("MPI_ERR_INTERN", MPI_ERR_INTERN);
  MPI_INTEGER_VAL ("MPI_ERR_IN_STATUS", MPI_ERR_IN_STATUS);
  MPI_INTEGER_VAL ("MPI_ERR_PENDING", MPI_ERR_PENDING);
  MPI_INTEGER_VAL ("MPI_ERR_KEYVAL", MPI_ERR_KEYVAL);
  MPI_INTEGER_VAL ("MPI_ERR_NO_MEM", MPI_ERR_NO_MEM);
  MPI_INTEGER_VAL ("MPI_ERR_BASE", MPI_ERR_BASE);
  MPI_INTEGER_VAL ("MPI_ERR_INFO_KEY", MPI_ERR_INFO_KEY);
  MPI_INTEGER_VAL ("MPI_ERR_INFO_VALUE", MPI_ERR_INFO_VALUE);
  MPI_INTEGER_VAL ("MPI_ERR_INFO_NOKEY", MPI_ERR_INFO_NOKEY);
  MPI_INTEGER_VAL ("MPI_ERR_SPAWN", MPI_ERR_SPAWN);
  MPI_INTEGER_VAL ("MPI_ERR_PORT", MPI_ERR_PORT);
  MPI_INTEGER_VAL ("MPI_ERR_SERVICE", MPI_ERR_SERVICE);
  MPI_INTEGER_VAL ("MPI_ERR_NAME", MPI_ERR_NAME);
  MPI_INTEGER_VAL ("MPI_ERR_WIN", MPI_ERR_WIN);
  MPI_INTEGER_VAL ("MPI_ERR_SIZE", MPI_ERR_SIZE);
  MPI_INTEGER_VAL ("MPI_ERR_DISP", MPI_ERR_DISP);
  MPI_INTEGER_VAL ("MPI_ERR_INFO", MPI_ERR_INFO);
  MPI_INTEGER_VAL ("MPI_ERR_LOCKTYPE", MPI_ERR_LOCKTYPE);
  MPI_INTEGER_VAL ("MPI_ERR_ASSERT", MPI_ERR_ASSERT);
  MPI_INTEGER_VAL ("MPI_ERR_RMA_CONFLICT", MPI_ERR_RMA_CONFLICT);
  MPI_INTEGER_VAL ("MPI_ERR_RMA_SYNC", MPI_ERR_RMA_SYNC);
  MPI_INTEGER_VAL ("MPI_ERR_FILE", MPI_ERR_FILE);
  MPI_INTEGER_VAL ("MPI_ERR_NOT_SAME", MPI_ERR_NOT_SAME);
  MPI_INTEGER_VAL ("MPI_ERR_AMODE", MPI_ERR_AMODE);
  MPI_INTEGER_VAL ("MPI_ERR_UNSUPPORTED_DATAREP", MPI_ERR_UNSUPPORTED_DATAREP);
  MPI_INTEGER_VAL ("MPI_ERR_UNSUPPORTED_OPERATION", MPI_ERR_UNSUPPORTED_OPERATION);
  MPI_INTEGER_VAL ("MPI_ERR_NO_SUCH_FILE", MPI_ERR_NO_SUCH_FILE);
  MPI_INTEGER_VAL ("MPI_ERR_FILE_EXISTS", MPI_ERR_FILE_EXISTS);
  MPI_INTEGER_VAL ("MPI_ERR_BAD_FILE", MPI_ERR_BAD_FILE);
  MPI_INTEGER_VAL ("MPI_ERR_ACCESS", MPI_ERR_ACCESS);
  MPI_INTEGER_VAL ("MPI_ERR_NO_SPACE", MPI_ERR_NO_SPACE);
  MPI_INTEGER_VAL ("MPI_ERR_QUOTA", MPI_ERR_QUOTA);
  MPI_INTEGER_VAL ("MPI_ERR_READ_ONLY", MPI_ERR_READ_ONLY);
  MPI_INTEGER_VAL ("MPI_ERR_FILE_IN_USE", MPI_ERR_FILE_IN_USE);
  MPI_INTEGER_VAL ("MPI_ERR_DUP_DATAREP", MPI_ERR_DUP_DATAREP);
  MPI_INTEGER_VAL ("MPI_ERR_CONVERSION", MPI_ERR_CONVERSION);
  MPI_INTEGER_VAL ("MPI_ERR_IO", MPI_ERR_IO);
  MPI_INTEGER_VAL ("MPI_ERR_LASTCODE", MPI_ERR_LASTCODE);

  MPI_INTEGER_VAL ("MPI_NOT_IMPLEMENTED", MPI_NOT_IMPLEMENTED);

  MPI_INTEGER_VAL ("MPI_DATATYPE_NULL", MPI_DATATYPE_NULL);
  MPI_INTEGER_VAL ("MPI_UB", MPI_UB);
  MPI_INTEGER_VAL ("MPI_LB", MPI_LB);
  MPI_INTEGER_VAL ("MPI_CHAR", MPI_CHAR);
  MPI_INTEGER_VAL ("MPI_CHARACTER", MPI_CHAR);
  MPI_INTEGER_VAL ("MPI_BYTE", MPI_BYTE);
  MPI_INTEGER_VAL ("MPI_SHORT", MPI_SHORT);
  MPI_INTEGER_VAL ("MPI_INT", MPI_INT);
  MPI_INTEGER_VAL ("MPI_INTEGER", MPI_INT);
  MPI_INTEGER_VAL ("MPI_2INTEGER", MPI_2INT);
  MPI_INTEGER_VAL ("MPI_INTEGER1", MPI_INTEGER1);
  MPI_INTEGER_VAL ("MPI_INTEGER2", MPI_INTEGER2);
  MPI_INTEGER_VAL ("MPI_INTEGER4", MPI_INTEGER4);
  MPI_INTEGER_VAL ("MPI_INTEGER8", MPI_INTEGER8);
  MPI_INTEGER_VAL ("MPI_LONG", MPI_LONG);
  MPI_INTEGER_VAL ("MPI_LONG_INT", MPI_LONG_INT);
  MPI_INTEGER_VAL ("MPI_FLOAT", MPI_FLOAT);
  MPI_INTEGER_VAL ("MPI_DOUBLE", MPI_DOUBLE);
  MPI_INTEGER_VAL ("MPI_DOUBLE_PRECISION", MPI_DOUBLE);
  MPI_INTEGER_VAL ("MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR);
  MPI_INTEGER_VAL ("MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT);
  MPI_INTEGER_VAL ("MPI_UNSIGNED", MPI_UNSIGNED);
  MPI_INTEGER_VAL ("MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG);
  MPI_INTEGER_VAL ("MPI_LONG_DOUBLE", MPI_LONG_DOUBLE);
  MPI_INTEGER_VAL ("MPI_LONG_LONG_INT", MPI_LONG_LONG_INT);
  MPI_INTEGER_VAL ("MPI_LONG_LONG", MPI_LONG_LONG);
  MPI_INTEGER_VAL ("MPI_UNSIGNED_LONG_LONG_INT", MPI_UNSIGNED_LONG_LONG_INT);
  MPI_INTEGER_VAL ("MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG);
  MPI_INTEGER_VAL ("MPI_PACKED", MPI_PACKED);
  MPI_INTEGER_VAL ("MPI_FLOAT_INT", MPI_FLOAT_INT);
  MPI_INTEGER_VAL ("MPI_DOUBLE_INT", MPI_DOUBLE_INT);
  MPI_INTEGER_VAL ("MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT);
  MPI_INTEGER_VAL ("MPI_SHORT_INT", MPI_SHORT_INT);
  MPI_INTEGER_VAL ("MPI_2INT", MPI_2INT);
  MPI_INTEGER_VAL ("MPI_2FLOAT", MPI_2FLOAT);
  MPI_INTEGER_VAL ("MPI_COMPLEX", MPI_COMPLEX);
  MPI_INTEGER_VAL ("MPI_DOUBLE_COMPLEX", MPI_DOUBLE_COMPLEX);
  MPI_INTEGER_VAL ("MPI_2DOUBLE_PRECISION", MPI_2DOUBLE_PRECISION);
  MPI_INTEGER_VAL ("MPI_LOGICAL", MPI_LOGICAL);
  MPI_INTEGER_VAL ("MPI_2REAL", MPI_2FLOAT);
  MPI_INTEGER_VAL ("MPI_REAL4", MPI_REAL4);
  MPI_INTEGER_VAL ("MPI_REAL8", MPI_REAL8);
  MPI_INTEGER_VAL ("MPI_REAL16", MPI_REAL16);
  MPI_INTEGER_VAL ("MPI_SIGNED_CHAR", MPI_SIGNED_CHAR);
  MPI_INTEGER_VAL ("MPI_REAL", MPI_FLOAT);

  MPI_INTEGER_VAL ("MPI_SUM", MPI_SUM);
  MPI_INTEGER_VAL ("MPI_MAX", MPI_MAX);
  MPI_INTEGER_VAL ("MPI_MIN", MPI_MIN);
  MPI_INTEGER_VAL ("MPI_PROD", MPI_PROD);
  MPI_INTEGER_VAL ("MPI_LAND", MPI_LAND);
  MPI_INTEGER_VAL ("MPI_BAND", MPI_BAND);
  MPI_INTEGER_VAL ("MPI_LOR", MPI_LOR);
  MPI_INTEGER_VAL ("MPI_BOR", MPI_BOR);
  MPI_INTEGER_VAL ("MPI_LXOR", MPI_LXOR);
  MPI_INTEGER_VAL ("MPI_BXOR", MPI_BXOR);
  MPI_INTEGER_VAL ("MPI_MINLOC", MPI_MINLOC);
  MPI_INTEGER_VAL ("MPI_MAXLOC", MPI_MAXLOC);
  MPI_INTEGER_VAL ("MPI_OP_NULL", ((MPI_Op)-1));

  printf ("       INTEGER MPI_BOTTOM\n");

  MPI_INTEGER_VAL ("MPI_GROUP_EMPTY", ((MPI_Group)0));
  MPI_INTEGER_VAL ("MPI_GROUP_NULL", ((MPI_Group)-1));

   /* Other Null Handles */
  MPI_INTEGER_VAL ("MPI_INFO_NULL", ((MPI_Info)-1));
  MPI_INTEGER_VAL ("MPI_FILE_NULL", ((MPI_File)-1));
  MPI_INTEGER_VAL ("MPI_WIN_NULL", ((MPI_Win)-1));

  MPI_INTEGER_VAL ("MPI_MAX_ERROR_STRING", MPI_MAX_ERROR_STRING);

  MPI_INTEGER_VAL ("MPI_ERRHANDLER_NULL", ((MPI_Errhandler)0));
  MPI_INTEGER_VAL ("MPI_ERRORS_RETURN", 1);
  MPI_INTEGER_VAL ("MPI_ERRORS_ARE_FATAL", 2);

  MPI_INTEGER_VAL ("MPI_KEYVAL_INVALID", -1);

  MPI_POINTER_CONV_VAL ("MPI_NULL_DELETE_FN", ((MPI_Delete_function *)MPC_Mpi_null_delete_fn));
  MPI_POINTER_CONV_VAL ("MPI_NULL_COPY_FN", ((MPI_Copy_function *)MPC_Mpi_null_copy_fn));
  MPI_POINTER_CONV_VAL ("MPI_DUP_FN", ((MPI_Copy_function *)MPC_Mpi_dup_fn));

  MPI_POINTER_CONV_VAL ("MPI_TYPE_NULL_DELETE_FN", ((MPI_Delete_function *)MPC_Mpi_type_null_delete_fn));
  MPI_POINTER_CONV_VAL ("MPI_TYPE_NULL_COPY_FN", ((MPI_Copy_function *)MPC_Mpi_type_null_copy_fn));
  MPI_POINTER_CONV_VAL ("MPI_TYPE_DUP_FN", ((MPI_Copy_function *)MPC_Mpi_type_dup_fn));

  MPI_POINTER_CONV_VAL ("MPI_COMM_NULL_DELETE_FN", ((MPI_Delete_function *)MPC_Mpi_comm_null_delete_fn));
  MPI_POINTER_CONV_VAL ("MPI_COMM_NULL_COPY_FN", ((MPI_Copy_function *)MPC_Mpi_comm_null_copy_fn));
  MPI_POINTER_CONV_VAL ("MPI_COMM_DUP_FN", ((MPI_Copy_function *)MPC_Mpi_comm_dup_fn));

  MPI_POINTER_CONV_VAL ("MPI_WIN_NULL_DELETE_FN", ((MPI_Delete_function *)MPC_Mpi_win_null_delete_fn));
  MPI_POINTER_CONV_VAL ("MPI_WIN_NULL_COPY_FN", ((MPI_Copy_function *)MPC_Mpi_win_null_copy_fn));
  MPI_POINTER_CONV_VAL ("MPI_WIN_DUP_FN", ((MPI_Copy_function *)MPC_Mpi_win_dup_fn));

  MPI_INTEGER_VAL ("MPI_COMM_SELF", MPI_COMM_SELF);

  MPI_INTEGER_VAL ("MPI_MAX_KEY_DEFINED", 7);
  MPI_INTEGER_VAL ("MPI_TAG_UB", 0);
  MPI_INTEGER_VAL ("MPI_HOST", 1);
  MPI_INTEGER_VAL ("MPI_IO", 2);
  MPI_INTEGER_VAL ("MPI_WTIME_IS_GLOBAL", 3);
  MPI_INTEGER_VAL ("MPI_UNIVERSE_SIZE", MPI_KEYVAL_INVALID);
  MPI_INTEGER_VAL ("MPI_LASTUSEDCODE", MPI_KEYVAL_INVALID);
  MPI_INTEGER_VAL ("MPI_APPNUM", MPI_KEYVAL_INVALID);

  /* Thread level */
  MPI_INTEGER_VAL ("MPI_THREAD_SINGLE", MPI_THREAD_SINGLE);
  MPI_INTEGER_VAL ("MPI_THREAD_FUNNELED", MPI_THREAD_FUNNELED);
  MPI_INTEGER_VAL ("MPI_THREAD_SERIALIZED", MPI_THREAD_SERIALIZED);
  MPI_INTEGER_VAL ("MPI_THREAD_MULTIPLE", MPI_THREAD_MULTIPLE);

  /*Architecture dependent part */
  MPI_INTEGER_VAL ("MPI_STATUS_SIZE", (int) (sizeof (MPI_Status) / 4));
  assert (sizeof (MPI_Status) % 4 == 0);
  MPI_INTEGER_VAL ("MPI_REQUEST_SIZE", (int) (sizeof (MPI_Request) / 4));
  assert (sizeof (MPI_Request) % 4 == 0);
  MPI_INTEGER_VAL ("MPI_BSEND_OVERHEAD", (int) ((2 * sizeof(mpi_buffer_overhead_t)) / 4));
  assert ((2 * sizeof(mpi_buffer_overhead_t)) % 4 == 0);

  printf("       DOUBLE PRECISION MPI_WTIME, MPI_WTICK\n");
  printf("       DOUBLE PRECISION PMPI_WTIME, PMPI_WTICK\n");
  return 0;
}
#endif

int
main (int argc, char **argv)
{
  assert(argc == 2);
  if(strcmp(argv[1],"mpc.h") == 0){
    main_mpc(argc,argv);
  } else {
#ifdef MPC_MPI
    if(strcmp(argv[1],"mpi.h") == 0){
      main_mpi(argc,argv);
    } else {
#endif
      assert(0);
    }
#ifdef MPC_MPI
  }
#endif
  return 0;
}
