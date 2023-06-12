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
#include <assert.h>
#include <mpc_lowcomm.h>
#include <mpc_mpi.h>
#include <mpio.h>

int is_mpif_only = 0;

#define MPI_TYPE_VAL(sa, a, t)                                                 \
  printf("{ \"name\" : \"%s\" , \"decl\" : \"       INTEGER %s\\n       "      \
         "PARAMETER (%s=%d)\\n\", \"mpif_only\" : %d, \"value\" : %d , "       \
         "\"type\" : \"%s\"},\n",                                              \
         sa, sa, sa, (int)a, is_mpif_only, (int)a, t)
#define MPI_TYPEP_VAL(sa, a, t)                                                 \
  printf("{ \"name\" : \"%s\" , \"decl\" : \"       INTEGER %s\\n       "      \
         "PARAMETER (%s=%d)\\n\", \"mpif_only\" : %d, \"value\" : %d , "       \
         "\"type\" : \"%s\"},\n",                                              \
         sa, sa, sa, (intptr_t)a, is_mpif_only, (intptr_t)a, t)
#define MPI_TYPEL_VAL(sa, a, t)                                                \
  printf("{ \"name\" : \"%s\" , \"decl\" : \"       INTEGER*8 %s\\n       "    \
         "PARAMETER (%s=%d)\\n\", \"mpif_only\" : %d, \"value\" : %d , "       \
         "\"type\" : \"%s\"},\n",                                              \
         sa, sa, sa, (int)a, is_mpif_only, (int)a, t)
#define MPI_INTEGER_VAL(sa, a)                                                 \
  printf("{ \"name\" : \"%s\" , \"decl\" : \"       INTEGER %s\\n       "      \
         "PARAMETER (%s=%d)\\n\", \"mpif_only\" : %d, \"value\" : %d , "       \
         "\"type\" : \"int\"},\n",                                             \
         sa, sa, sa, (intptr_t)a, is_mpif_only, (intptr_t)a)
#define MPI_INTEGER_ARRAY_CONV(sa,size) printf("{ \"name\" : \"%s\" , \"decl\" : \"       integer %s(%d)\\n       COMMON %s\\n\", \"mpif_only\" : %d },\n",sa, sa, size, sa, is_mpif_only)
#define MPI_INTEGER_ARRAY_OF_ARRAY_CONV(sa,size) printf("{ \"name\" : \"%s\" , \"decl\" : \"       integer %s(%d,1)\\n       COMMON %s\\n\", \"mpif_only\" : %d },\n",sa, sa,size, sa, is_mpif_only)
#define MPI_INTEGER_8_VAL(sa, a)                                               \
  printf("{ \"name\" : \"%s\" , \"decl\" : \"       INTEGER*8 %s\\n       "    \
         "PARAMETER (%s=%ld)\\n\", \"mpif_only\" : %d, \"value\" : %ld, "      \
         "\"type\" : \"long\" },\n",                                           \
         sa, sa, sa, (long)a, is_mpif_only, (long)a)
#define MPI_POINTER_CONV(sa) printf("{ \"name\" : \"%s\" , \"decl\" : \"       EXTERNAL %s\\n\", \"mpif_only\" : %d },\n",sa, sa, is_mpif_only)
#define MPI_DOUBLE_CONV(sa) printf("{ \"name\" : \"%s\" , \"decl\" : \"       DOUBLE PRECISION %s\\n\", \"mpif_only\" : %d },\n",sa, sa, is_mpif_only)

int generate_mpi_fortran_constants()
{
	printf("[\n");

        /* Start of ROMIO */
        MPI_INTEGER_VAL("MPI_MODE_RDONLY", MPI_MODE_RDONLY);
        MPI_INTEGER_VAL("MPI_MODE_RDWR", MPI_MODE_RDWR);
        MPI_INTEGER_VAL("MPI_MODE_WRONLY", MPI_MODE_WRONLY);
        MPI_INTEGER_VAL("MPI_MODE_CREATE", MPI_MODE_CREATE);
        MPI_INTEGER_VAL("MPI_MODE_DELETE_ON_CLOSE", MPI_MODE_DELETE_ON_CLOSE);
        MPI_INTEGER_VAL("MPI_MODE_UNIQUE_OPEN", MPI_MODE_UNIQUE_OPEN);
        MPI_INTEGER_VAL("MPI_MODE_EXCL", MPI_MODE_EXCL);
        MPI_INTEGER_VAL("MPI_MODE_APPEND", MPI_MODE_APPEND);
        MPI_INTEGER_VAL("MPI_MODE_SEQUENTIAL", MPI_MODE_SEQUENTIAL);
        MPI_INTEGER_VAL("MPI_FILE_NULL", (intptr_t)(MPI_FILE_NULL));
        MPI_INTEGER_VAL("MPI_MAX_DATAREP_STRING", MPI_MAX_DATAREP_STRING);
        MPI_INTEGER_VAL("MPI_SEEK_SET", MPI_SEEK_SET);
        MPI_INTEGER_VAL("MPI_SEEK_CUR", MPI_SEEK_CUR);
        MPI_INTEGER_VAL("MPI_SEEK_END", MPI_SEEK_END);
        MPI_INTEGER_VAL("MPIO_REQUEST_NULL",(intptr_t)(MPIO_REQUEST_NULL));
        MPI_INTEGER_8_VAL("MPI_DISPLACEMENT_CURRENT", MPI_DISPLACEMENT_CURRENT);
        /* End of ROMIO */

        /* Status Indexing */
        MPI_INTEGER_VAL("MPI_SOURCE", MPI_F_SOURCE + 1);
        MPI_INTEGER_VAL("MPI_TAG", MPI_F_TAG + 1);
        MPI_INTEGER_VAL("MPI_ERROR", MPI_F_ERROR + 1);

        /* Kinds */
        MPI_INTEGER_VAL("MPI_OFFSET_KIND", 8);
        MPI_INTEGER_VAL("MPI_ADDRESS_KIND", 8);
        MPI_INTEGER_VAL("MPI_COUNT_KIND", 8);
        MPI_INTEGER_VAL("MPI_INTEGER_KIND", 4);

        /* Distribute Array */
        MPI_INTEGER_VAL("MPI_DISTRIBUTE_DFLT_DARG", MPI_DISTRIBUTE_DFLT_DARG);
        MPI_INTEGER_VAL("MPI_DISTRIBUTE_BLOCK", MPI_DISTRIBUTE_BLOCK);
        MPI_INTEGER_VAL("MPI_DISTRIBUTE_CYCLIC", MPI_DISTRIBUTE_CYCLIC);
        MPI_INTEGER_VAL("MPI_DISTRIBUTE_NONE", MPI_DISTRIBUTE_NONE);
        MPI_INTEGER_VAL("MPI_ORDER_C", MPI_ORDER_C);
        MPI_INTEGER_VAL("MPI_ORDER_FORTRAN", MPI_ORDER_FORTRAN);

        /* Max string size */
        MPI_INTEGER_VAL("MPI_MAX_OBJECT_NAME", MPI_MAX_OBJECT_NAME);
        MPI_INTEGER_VAL("MPI_MAX_INFO_VAL", MPI_MAX_INFO_VAL);
        MPI_INTEGER_VAL("MPI_MAX_INFO_KEY", MPI_MAX_INFO_KEY);
        MPI_INTEGER_VAL("MPI_MAX_PROCESSOR_NAME", MPI_MAX_PROCESSOR_NAME);
        MPI_INTEGER_VAL("MPI_MAX_NAME_STRING", MPI_MAX_NAME_STRING);
        MPI_INTEGER_VAL("MPI_MAX_PORT_NAME", MPI_MAX_PORT_NAME);
        MPI_INTEGER_VAL("MPI_MAX_ERROR_STRING", MPI_MAX_ERROR_STRING);
        MPI_INTEGER_VAL("MPI_MAX_KEY_DEFINED", MPI_MAX_KEY_DEFINED);

        /* Combiners */
        MPI_INTEGER_VAL("MPI_COMBINER_UNKNOWN", MPI_COMBINER_UNKNOWN);
        MPI_INTEGER_VAL("MPI_COMBINER_NAMED", MPI_COMBINER_NAMED);
        MPI_INTEGER_VAL("MPI_COMBINER_DUP", MPI_COMBINER_DUP);
        MPI_INTEGER_VAL("MPI_COMBINER_CONTIGUOUS", MPI_COMBINER_CONTIGUOUS);
        MPI_INTEGER_VAL("MPI_COMBINER_VECTOR", MPI_COMBINER_VECTOR);
        MPI_INTEGER_VAL("MPI_COMBINER_HVECTOR", MPI_COMBINER_HVECTOR);
        MPI_INTEGER_VAL("MPI_COMBINER_INDEXED", MPI_COMBINER_INDEXED);
        MPI_INTEGER_VAL("MPI_COMBINER_HINDEXED", MPI_COMBINER_HINDEXED);
        MPI_INTEGER_VAL("MPI_COMBINER_INDEXED_BLOCK",
                        MPI_COMBINER_INDEXED_BLOCK);
        MPI_INTEGER_VAL("MPI_COMBINER_HINDEXED_BLOCK",
                        MPI_COMBINER_HINDEXED_BLOCK);
        MPI_INTEGER_VAL("MPI_COMBINER_STRUCT", MPI_COMBINER_STRUCT);
        MPI_INTEGER_VAL("MPI_COMBINER_SUBARRAY", MPI_COMBINER_SUBARRAY);
        MPI_INTEGER_VAL("MPI_COMBINER_DARRAY", MPI_COMBINER_DARRAY);
        MPI_INTEGER_VAL("MPI_COMBINER_F90_REAL", MPI_COMBINER_F90_REAL);
        MPI_INTEGER_VAL("MPI_COMBINER_F90_INTEGER", MPI_COMBINER_F90_INTEGER);
        MPI_INTEGER_VAL("MPI_COMBINER_F90_COMPLEX", MPI_COMBINER_F90_COMPLEX);
        MPI_INTEGER_VAL("MPI_COMBINER_RESIZED", MPI_COMBINER_RESIZED);
        MPI_INTEGER_VAL("MPI_COMBINER_HINDEXED_INTEGER",
                        MPI_COMBINER_HINDEXED_INTEGER);
        MPI_INTEGER_VAL("MPI_COMBINER_STRUCT_INTEGER",
                        MPI_COMBINER_STRUCT_INTEGER);
        MPI_INTEGER_VAL("MPI_COMBINER_HVECTOR_INTEGER",
                        MPI_COMBINER_HVECTOR_INTEGER);

        /* Null handles */
        MPI_TYPE_VAL("MPI_REQUEST_NULL", MPI_REQUEST_NULL, "MPI_Request");
        MPI_TYPEP_VAL("MPI_COMM_NULL", MPI_COMM_NULL, "MPI_Comm");
        MPI_TYPEP_VAL("MPI_DATATYPE_NULL", MPI_DATATYPE_NULL, "MPI_Datatype");
        MPI_TYPE_VAL("MPI_OP_NULL", ((MPI_Op)MPI_OP_NULL), "MPI_Op");
        MPI_TYPE_VAL("MPI_WIN_NULL", ((MPI_Win)MPI_WIN_NULL), "MPI_Win");

        MPI_TYPE_VAL("MPI_INFO_NULL", ((MPI_Info)MPI_INFO_NULL), "MPI_Info");
        MPI_TYPE_VAL("MPI_ERRHANDLER_NULL", MPI_ERRHANDLER_NULL,
                     "MPI_Errhandler");
        MPI_TYPE_VAL("MPI_MESSAGE_NULL", MPI_REQUEST_NULL, "MPI_Message");

        MPI_INTEGER_VAL("MPI_PROC_NULL",  (intptr_t)(MPI_PROC_NULL));
        MPI_INTEGER_VAL("MPI_ARGV_NULL",  (intptr_t)(MPI_ARGV_NULL));
        MPI_INTEGER_VAL("MPI_ARGVS_NULL", (intptr_t)(MPI_ARGVS_NULL));

        /* Undefined */
        MPI_INTEGER_VAL("MPI_UNDEFINED", MPI_UNDEFINED);

        /* Default Comms */
        MPI_TYPE_VAL("MPI_COMM_WORLD", MPC_LOWCOMM_COMM_WORLD_NUMERIC_ID, "MPI_Comm");
        MPI_TYPE_VAL("MPI_COMM_SELF", MPC_LOWCOMM_COMM_SELF_NUMERIC_ID, "MPI_Comm");

        /* Topologies */
        MPI_INTEGER_VAL("MPI_CART", MPI_CART);
        MPI_INTEGER_VAL("MPI_GRAPH", MPI_GRAPH);
        MPI_INTEGER_VAL("MPI_DIST_GRAPH", MPI_DIST_GRAPH);
        MPI_INTEGER_VAL("MPI_UNWEIGHTED", MPI_UNWEIGHTED);
        MPI_INTEGER_VAL("MPI_WEIGHTS_EMPTY", MPI_WEIGHTS_EMPTY);

        /* Results of the compare operations. */
        MPI_INTEGER_VAL("MPI_IDENT", MPI_IDENT);
        MPI_INTEGER_VAL("MPI_CONGRUENT", MPI_CONGRUENT);
        MPI_INTEGER_VAL("MPI_SIMILAR", MPI_SIMILAR);
        MPI_INTEGER_VAL("MPI_UNEQUAL", MPI_UNEQUAL);

        /* Wildcard Operations */
        MPI_INTEGER_VAL("MPI_ANY_TAG", MPI_ANY_TAG);
        MPI_INTEGER_VAL("MPI_ANY_SOURCE", MPI_ANY_SOURCE);

        /* Collective Modifiers */
        MPI_INTEGER_VAL("MPI_ROOT",     (intptr_t)(MPI_ROOT));
        MPI_INTEGER_VAL("MPI_IN_PLACE", (intptr_t)(MPI_IN_PLACE));

        /* Errors */
        MPI_INTEGER_VAL("MPI_SUCCESS", MPI_SUCCESS);
        MPI_INTEGER_VAL("MPI_ERR_BUFFER", MPI_ERR_BUFFER);
        MPI_INTEGER_VAL("MPI_ERR_COUNT", MPI_ERR_COUNT);
        MPI_INTEGER_VAL("MPI_ERR_TYPE", MPI_ERR_TYPE);
        MPI_INTEGER_VAL("MPI_ERR_TAG", MPI_ERR_TAG);
        MPI_INTEGER_VAL("MPI_ERR_COMM", MPI_ERR_COMM);
        MPI_INTEGER_VAL("MPI_ERR_RANK", MPI_ERR_RANK);
        MPI_INTEGER_VAL("MPI_ERR_REQUEST", MPI_ERR_REQUEST);
        MPI_INTEGER_VAL("MPI_ERR_ROOT", MPI_ERR_ROOT);
        MPI_INTEGER_VAL("MPI_ERR_GROUP", MPI_ERR_GROUP);
        MPI_INTEGER_VAL("MPI_ERR_OP", MPI_ERR_OP);
        MPI_INTEGER_VAL("MPI_ERR_TOPOLOGY", MPI_ERR_TOPOLOGY);
        MPI_INTEGER_VAL("MPI_ERR_DIMS", MPI_ERR_DIMS);
        MPI_INTEGER_VAL("MPI_ERR_ARG", MPI_ERR_ARG);
        MPI_INTEGER_VAL("MPI_ERR_UNKNOWN", MPI_ERR_UNKNOWN);
        MPI_INTEGER_VAL("MPI_ERR_TRUNCATE", MPI_ERR_TRUNCATE);
        MPI_INTEGER_VAL("MPI_ERR_OTHER", MPI_ERR_OTHER);
        MPI_INTEGER_VAL("MPI_ERR_INTERN", MPI_ERR_INTERN);
        MPI_INTEGER_VAL("MPI_ERR_IN_STATUS", MPI_ERR_IN_STATUS);
        MPI_INTEGER_VAL("MPI_ERR_PENDING", MPI_ERR_PENDING);
        MPI_INTEGER_VAL("MPI_ERR_KEYVAL", MPI_ERR_KEYVAL);
        MPI_INTEGER_VAL("MPI_ERR_NO_MEM", MPI_ERR_NO_MEM);
        MPI_INTEGER_VAL("MPI_ERR_BASE", MPI_ERR_BASE);
        MPI_INTEGER_VAL("MPI_ERR_INFO_KEY", MPI_ERR_INFO_KEY);
        MPI_INTEGER_VAL("MPI_ERR_INFO_VALUE", MPI_ERR_INFO_VALUE);
        MPI_INTEGER_VAL("MPI_ERR_INFO_NOKEY", MPI_ERR_INFO_NOKEY);
        MPI_INTEGER_VAL("MPI_ERR_SPAWN", MPI_ERR_SPAWN);
        MPI_INTEGER_VAL("MPI_ERR_PORT", MPI_ERR_PORT);
        MPI_INTEGER_VAL("MPI_ERR_SERVICE", MPI_ERR_SERVICE);
        MPI_INTEGER_VAL("MPI_ERR_NAME", MPI_ERR_NAME);
        MPI_INTEGER_VAL("MPI_ERR_WIN", MPI_ERR_WIN);
        MPI_INTEGER_VAL("MPI_ERR_SIZE", MPI_ERR_SIZE);
        MPI_INTEGER_VAL("MPI_ERR_DISP", MPI_ERR_DISP);
        MPI_INTEGER_VAL("MPI_ERR_INFO", MPI_ERR_INFO);
        MPI_INTEGER_VAL("MPI_ERR_LOCKTYPE", MPI_ERR_LOCKTYPE);
        MPI_INTEGER_VAL("MPI_ERR_ASSERT", MPI_ERR_ASSERT);
        MPI_INTEGER_VAL("MPI_ERR_RMA_CONFLICT", MPI_ERR_RMA_CONFLICT);
        MPI_INTEGER_VAL("MPI_ERR_RMA_SYNC", MPI_ERR_RMA_SYNC);
        MPI_INTEGER_VAL("MPI_ERR_FILE", MPI_ERR_FILE);
        MPI_INTEGER_VAL("MPI_ERR_NOT_SAME", MPI_ERR_NOT_SAME);
        MPI_INTEGER_VAL("MPI_ERR_AMODE", MPI_ERR_AMODE);
        MPI_INTEGER_VAL("MPI_ERR_UNSUPPORTED_DATAREP",
                        MPI_ERR_UNSUPPORTED_DATAREP);
        MPI_INTEGER_VAL("MPI_ERR_UNSUPPORTED_OPERATION",
                        MPI_ERR_UNSUPPORTED_OPERATION);
        MPI_INTEGER_VAL("MPI_ERR_NO_SUCH_FILE", MPI_ERR_NO_SUCH_FILE);
        MPI_INTEGER_VAL("MPI_ERR_FILE_EXISTS", MPI_ERR_FILE_EXISTS);
        MPI_INTEGER_VAL("MPI_ERR_BAD_FILE", MPI_ERR_BAD_FILE);
        MPI_INTEGER_VAL("MPI_ERR_ACCESS", MPI_ERR_ACCESS);
        MPI_INTEGER_VAL("MPI_ERR_NO_SPACE", MPI_ERR_NO_SPACE);
        MPI_INTEGER_VAL("MPI_ERR_QUOTA", MPI_ERR_QUOTA);
        MPI_INTEGER_VAL("MPI_ERR_READ_ONLY", MPI_ERR_READ_ONLY);
        MPI_INTEGER_VAL("MPI_ERR_FILE_IN_USE", MPI_ERR_FILE_IN_USE);
        MPI_INTEGER_VAL("MPI_ERR_DUP_DATAREP", MPI_ERR_DUP_DATAREP);
        MPI_INTEGER_VAL("MPI_ERR_CONVERSION", MPI_ERR_CONVERSION);
        MPI_INTEGER_VAL("MPI_ERR_IO", MPI_ERR_IO);
        MPI_INTEGER_VAL("MPI_ERR_LASTCODE", MPI_ERR_LASTCODE);

        MPI_INTEGER_VAL("MPI_NOT_IMPLEMENTED", MPI_NOT_IMPLEMENTED);

        MPI_TYPE_VAL("MPI_ERRORS_RETURN", MPI_ERRORS_RETURN, "MPI_Errhandler");
        MPI_TYPE_VAL("MPI_ERRORS_ARE_FATAL", MPI_ERRORS_ARE_FATAL,
                     "MPI_Errhandler");

        /* Datatypes */

        MPI_TYPEP_VAL("MPI_UB", MPI_UB, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LB", MPI_LB, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_CHAR", MPI_CHAR, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_WCHAR", MPI_WCHAR, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_CHARACTER", MPI_CHARACTER, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_BYTE", MPI_BYTE, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_SHORT", MPI_SHORT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INT", MPI_INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INTEGER", MPI_INTEGER, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_2INTEGER", MPI_2INTEGER, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INTEGER1", MPI_INTEGER1, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INTEGER2", MPI_INTEGER2, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INTEGER4", MPI_INTEGER4, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INTEGER8", MPI_INTEGER8, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_AINT", MPI_AINT, "MPI_Datatype");

        MPI_TYPEP_VAL("MPI_COMPLEX8", MPI_COMPLEX8, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_COMPLEX16", MPI_COMPLEX16, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_COMPLEX32", MPI_COMPLEX32, "MPI_Datatype");

        MPI_TYPEP_VAL("MPI_INT8_T", MPI_INT8_T, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INT16_T", MPI_INT16_T, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INT32_T", MPI_INT32_T, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_INT64_T", MPI_INT64_T, "MPI_Datatype");

        MPI_TYPEP_VAL("MPI_UINT8_T", MPI_UINT8_T, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UINT16_T", MPI_UINT16_T, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UINT32_T", MPI_UINT32_T, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UINT64_T", MPI_UINT64_T, "MPI_Datatype");

        MPI_TYPEP_VAL("MPI_C_BOOL", MPI_C_BOOL, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_C_COMPLEX", MPI_C_COMPLEX, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX,
                     "MPI_Datatype");

        MPI_TYPEP_VAL("MPI_LONG", MPI_LONG, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LONG_INT", MPI_LONG_INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_FLOAT", MPI_FLOAT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_DOUBLE", MPI_DOUBLE, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_DOUBLE_PRECISION", MPI_DOUBLE_PRECISION,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UNSIGNED", MPI_UNSIGNED, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LONG_DOUBLE", MPI_LONG_DOUBLE, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LONG_LONG_INT", MPI_LONG_LONG_INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LONG_LONG", MPI_LONG_LONG, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UNSIGNED_LONG_LONG_INT", MPI_UNSIGNED_LONG_LONG_INT,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_PACKED", MPI_PACKED, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_FLOAT_INT", MPI_FLOAT_INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_DOUBLE_INT", MPI_DOUBLE_INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_SHORT_INT", MPI_SHORT_INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_2INT", MPI_2INT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_2FLOAT", MPI_2FLOAT, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_COMPLEX", MPI_COMPLEX, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_DOUBLE_COMPLEX", MPI_DOUBLE_COMPLEX, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_2DOUBLE_PRECISION", MPI_2DOUBLE_PRECISION,
                     "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_LOGICAL", MPI_LOGICAL, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_2REAL", MPI_2REAL, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_REAL4", MPI_REAL4, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_REAL8", MPI_REAL8, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_REAL16", MPI_REAL16, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_SIGNED_CHAR", MPI_SIGNED_CHAR, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_REAL", MPI_REAL, "MPI_Datatype");
        MPI_TYPEP_VAL("MPI_COUNT", MPI_COUNT, "MPI_Datatype");
        /*MPI_TYPE_VAL("MPI_AINT", MPI_AINT, "MPI_Datatype");*/
        MPI_TYPEP_VAL("MPI_OFFSET", MPI_OFFSET, "MPI_Datatype");

        /* Operations */

        MPI_TYPE_VAL("MPI_SUM", MPI_SUM, "MPI_Op");
        MPI_TYPE_VAL("MPI_MAX", MPI_MAX, "MPI_Op");
        MPI_TYPE_VAL("MPI_MIN", MPI_MIN, "MPI_Op");
        MPI_TYPE_VAL("MPI_PROD", MPI_PROD, "MPI_Op");
        MPI_TYPE_VAL("MPI_LAND", MPI_LAND, "MPI_Op");
        MPI_TYPE_VAL("MPI_BAND", MPI_BAND, "MPI_Op");
        MPI_TYPE_VAL("MPI_LOR", MPI_LOR, "MPI_Op");
        MPI_TYPE_VAL("MPI_BOR", MPI_BOR, "MPI_Op");
        MPI_TYPE_VAL("MPI_LXOR", MPI_LXOR, "MPI_Op");
        MPI_TYPE_VAL("MPI_BXOR", MPI_BXOR, "MPI_Op");
        MPI_TYPE_VAL("MPI_MINLOC", MPI_MINLOC, "MPI_Op");
        MPI_TYPE_VAL("MPI_MAXLOC", MPI_MAXLOC, "MPI_Op");
        MPI_TYPE_VAL("MPI_REPLACE", MPI_REPLACE, "MPI_Op");
        MPI_TYPE_VAL("MPI_NO_OP", MPI_NO_OP, "MPI_Op");

        MPI_INTEGER_VAL("MPI_BOTTOM", (intptr_t)(MPI_BOTTOM));

        MPI_INTEGER_VAL("MPI_GROUP_EMPTY", (intptr_t)(-1));
        MPI_INTEGER_VAL("MPI_GROUP_NULL",  (intptr_t)(0));

        MPI_INTEGER_VAL("MPI_KEYVAL_INVALID", MPI_KEYVAL_INVALID);

        /* Thread level */
        MPI_INTEGER_VAL("MPI_THREAD_SINGLE", MPI_THREAD_SINGLE);
        MPI_INTEGER_VAL("MPI_THREAD_FUNNELED", MPI_THREAD_FUNNELED);
        MPI_INTEGER_VAL("MPI_THREAD_SERIALIZED", MPI_THREAD_SERIALIZED);
        MPI_INTEGER_VAL("MPI_THREAD_MULTIPLE", MPI_THREAD_MULTIPLE);

        /*Architecture dependent part */
        MPI_INTEGER_VAL("MPI_STATUS_SIZE", MPI_F_STATUS_SIZE);
        assert(sizeof(MPI_Status) % 4 == 0);
        MPI_INTEGER_VAL("MPI_REQUEST_SIZE", (int)(sizeof(MPI_Request) / 4));
        assert(sizeof(MPI_Request) % 4 == 0);
        MPI_INTEGER_VAL("MPI_BSEND_OVERHEAD", MPI_BSEND_OVERHEAD / 4);
        assert(MPI_BSEND_OVERHEAD % 4 == 0);

        /* Versions */
        MPI_INTEGER_VAL("MPI_VERSION", MPI_VERSION);
        MPI_INTEGER_VAL("MPI_SUBVERSION", MPI_SUBVERSION);

        MPI_INTEGER_VAL("MPI_COMM_TYPE_SHARED", MPI_COMM_TYPE_SHARED);
        MPI_INTEGER_VAL("MPI_MESSAGE_NO_PROC", MPI_MESSAGE_NO_PROC);

        /* Attributes */
        MPI_INTEGER_VAL("MPI_WTIME_IS_GLOBAL", MPI_WTIME_IS_GLOBAL);
        MPI_INTEGER_VAL("MPI_UNIVERSE_SIZE", MPI_UNIVERSE_SIZE);
        MPI_INTEGER_VAL("MPI_LASTUSEDCODE", MPI_LASTUSEDCODE);
        MPI_INTEGER_VAL("MPI_APPNUM", MPI_APPNUM);
        MPI_INTEGER_VAL("MPI_TAG_UB", MPI_TAG_UB);
        MPI_INTEGER_VAL("MPI_HOST", MPI_HOST);
        MPI_INTEGER_VAL("MPI_IO", MPI_IO);

        MPI_INTEGER_VAL("MPI_WIN_BASE", MPI_WIN_BASE);
        MPI_INTEGER_VAL("MPI_WIN_SIZE", MPI_WIN_SIZE);

        MPI_INTEGER_VAL("MPI_WIN_DISP_UNIT", MPI_WIN_DISP_UNIT);
        MPI_INTEGER_VAL("MPI_WIN_CREATE_FLAVOR", MPI_WIN_CREATE_FLAVOR);
        MPI_INTEGER_VAL("MPI_WIN_MODEL", MPI_WIN_MODEL);

        /* RMA defines */
        MPI_INTEGER_VAL("MPI_WIN_FLAVOR_CREATE", MPI_WIN_FLAVOR_CREATE);
        MPI_INTEGER_VAL("MPI_WIN_FLAVOR_ALLOCATE", MPI_WIN_FLAVOR_ALLOCATE);
        MPI_INTEGER_VAL("MPI_WIN_FLAVOR_DYNAMIC", MPI_WIN_FLAVOR_DYNAMIC);
        MPI_INTEGER_VAL("MPI_WIN_FLAVOR_SHARED", MPI_WIN_FLAVOR_SHARED);
        MPI_INTEGER_VAL("MPI_WIN_SEPARATE", MPI_WIN_SEPARATE);
        MPI_INTEGER_VAL("MPI_WIN_UNIFIED", MPI_WIN_UNIFIED);

        MPI_INTEGER_VAL("MPI_MODE_NOCHECK", MPI_MODE_NOCHECK);
        MPI_INTEGER_VAL("MPI_MODE_NOSTORE", MPI_MODE_NOSTORE);
        MPI_INTEGER_VAL("MPI_MODE_NOPUT", MPI_MODE_NOPUT);
        MPI_INTEGER_VAL("MPI_MODE_NOPRECEDE", MPI_MODE_NOPRECEDE);
        MPI_INTEGER_VAL("MPI_MODE_NOSUCCEED", MPI_MODE_NOSUCCEED);

        MPI_INTEGER_VAL("MPI_LOCK_EXCLUSIVE", MPI_LOCK_EXCLUSIVE);
        MPI_INTEGER_VAL("MPI_LOCK_SHARED", MPI_LOCK_SHARED);

        /* Statuses */

        MPI_INTEGER_ARRAY_CONV("MPI_STATUS_IGNORE",
                               (int)(sizeof(MPI_Status) / 4));
        MPI_INTEGER_ARRAY_OF_ARRAY_CONV("MPI_STATUSES_IGNORE",
                               (int)(sizeof(MPI_Status) / 4));

        /* These definitions are only intended to the mpif.h header
         * as the module file handle them directly however they
         * are needed when using the legacy header file for this
         * reason we set the is_mpif_only to 1 for this section */

        is_mpif_only = 1;

        MPI_POINTER_CONV("MPI_NULL_DELETE_FN");
        MPI_POINTER_CONV("MPI_NULL_COPY_FN");
        MPI_POINTER_CONV("MPI_DUP_FN");

        MPI_POINTER_CONV("MPI_TYPE_NULL_DELETE_FN");
        MPI_POINTER_CONV("MPI_TYPE_NULL_COPY_FN");
        MPI_POINTER_CONV("MPI_TYPE_DUP_FN");

        MPI_POINTER_CONV("MPI_COMM_NULL_DELETE_FN");
        MPI_POINTER_CONV("MPI_COMM_NULL_COPY_FN");
        MPI_POINTER_CONV("MPI_COMM_DUP_FN");

        MPI_POINTER_CONV("MPI_WIN_NULL_DELETE_FN");
        MPI_POINTER_CONV("MPI_WIN_NULL_COPY_FN");
        MPI_POINTER_CONV("MPI_WIN_DUP_FN");

        MPI_POINTER_CONV("MPI_WTICK");
        MPI_POINTER_CONV("PMPI_WTICK");
        MPI_POINTER_CONV("MPI_WTIME");
        MPI_POINTER_CONV("PMPI_WTIME");

        MPI_DOUBLE_CONV("MPI_WTIME");
        MPI_DOUBLE_CONV("PMPI_WTIME");
        MPI_DOUBLE_CONV("MPI_WTICK");
        MPI_DOUBLE_CONV("PMPI_WTICK");

        is_mpif_only = 0;

        printf("{}\n");
        printf("]\n");

        return 0;
}


int main (int argc, char **argv)
{
	generate_mpi_fortran_constants();

	return 0;
}
