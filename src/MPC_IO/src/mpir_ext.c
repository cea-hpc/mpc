#include "mpir_ext.h"

#include <mpc_common_debug.h>
#include <sctk_alloc.h>

#include <mpc_thread.h>
#include <mpc_mpi_comm_lib.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>

/***********
* YIEDING *
***********/


/** \brief MPICH says check wether the progress engine is blocked assuming
 * "YIELD"
 */
void MPIR_Ext_cs_yield(void)
{
	mpc_thread_yield();
}

/******************
* ERROR HANDLING *
******************/

int MPIR_Err_create_code_valist(__UNUSED__ int lastcode, __UNUSED__ int fatal, const char fcname[],
                                int line, __UNUSED__ int error_class, const char generic_msg[],
                                const char specific_msg[], __UNUSED__ va_list Argp)
{
	char *buf;
	int   idx=0;

	buf = (char *)sctk_malloc(1024);
	if(buf != NULL)
	{
		idx += snprintf(buf, 1023, "%s (line %d): ", fcname, line);

		if(specific_msg == NULL)
		{
			snprintf(&buf[idx], 1023 - idx, "%s\n", generic_msg);
		}
		else
		{
			vsnprintf(&buf[idx], 1023 - idx, specific_msg, Argp);
		}
		mpc_common_debug_error("%s", buf);
		sctk_free(buf);
	}
	return lastcode;
}

void MPIR_Err_get_string(int errcode, char *msg, int maxlen, __UNUSED__ MPIR_Err_get_class_string_func_t fcname)
{
	char buff[128];
	int  len;

	if(!msg)
	{
		return;
	}

	buff[0] = '\0';
	msg[0]  = '\0';

	mpc_mpi_cl_error_string(errcode, buff, &len);

	if(strlen(buff) )
	{
		snprintf(msg, maxlen, "%s", buff);
	}
}

int MPIR_Err_is_fatal(__UNUSED__ int a)
{
	return 0;
}

void MPIR_Get_file_error_routine(__UNUSED__ MPI_Errhandler a,
                                 __UNUSED__ void(**errr)(void *, int *, ...),
                                 __UNUSED__ int *b)
{
}

int MPIR_Abort(MPI_Comm comm, int mpi_errno, int exit_code, const char *error_msg)
{
	mpc_common_debug_error("ERRNO %d EXIT %d MSG: %s", mpi_errno, exit_code, error_msg);
	return PMPI_Abort(comm, exit_code);
}

int MPIR_File_call_cxx_errhandler( __UNUSED__ void *fh, __UNUSED__ int *errorcode,
			   __UNUSED__ void (*c_errhandler)(void  *, int *, ... ) )
{

	return MPI_SUCCESS;
}


/************************************************************************/
/* Datatype Optimization                                                */
/************************************************************************/
/** \brief This function is used in ROMIO in order to dermine if a data-type is contiguous
 *  We implement it here as it is much faster to do it with the
 *  knowledge of data-type internals
 *
 *  \param datatype Type to be tested
 *  \param flag OUT 1 of cont. 0 otherwize
 */
void MPIR_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
	*flag = mpc_mpi_cl_type_is_contiguous(datatype);
}

/*******************
 * STATUS HANDLING *
 *******************/

/** \brief This function is an extension to the standard used to fill a status from a size
 *
 *  This routine directly computes the count from a size
 *
 *  \param status Status to be filled
 *  \param datatype Datatype stored in the status
 *  \param nbytes Size in bytes of datatype kind stored
 */
int MPIR_Status_set_bytes(MPI_Status *status, MPI_Datatype datatype, MPI_Count nbytes)
{
	MPI_Count type_size;
	PMPI_Type_size_x(datatype, &type_size);

	MPI_Count count = 0;

	if( type_size != 0 )
	{
		count = nbytes / type_size;
	}

	return PMPI_Status_set_elements_x(status, datatype, count);
}

/***********************
 * NODE ID COMPUTATION *
 ***********************/

/** \brief This function replaces the hostname processing in ROMIO it returns a
 * node-id
 * 	In MPC we rely on the SHM compuration
 *
 *  \param comm Source COMM
 *  \param rank Rank in COMM
 *  \param id (out) id of the target node
 *  \return MPI_SUCCESS if all OK
 */
int MPIR_Get_node_id(MPI_Comm comm, int rank, int *id) {
  // TODO use the actual node rank
  int comm_world_rank = mpc_mpi_cl_world_rank(comm, rank);

  struct mpc_launch_pmi_process_layout *nodes_infos = NULL;
  mpc_launch_pmi_get_process_layout(&nodes_infos);

  int node_number = mpc_common_get_node_count();

  struct mpc_launch_pmi_process_layout *tmp = NULL;

  int i, j;

  for (i = 0; i < node_number; i++) {
    HASH_FIND_INT(nodes_infos, &node_number, tmp);

	if (tmp == NULL) {
		return MPI_ERR_INTERN;
	}

    for (j = 0; j < tmp->nb_process; j++) {
      if (tmp->process_list[j] == comm_world_rank) {
        *id = i;
        return MPI_SUCCESS;
      }
    }
  }

  return MPI_SUCCESS;
}

/************************************************************************/
/* C2F/F2C Functions                                                    */
/************************************************************************/


#ifdef MPC_FORTRAN_ENABLED

/* These requests conversion functions are needed because
 * the fortran interface in ROMIO reffers to the old IOWait
 * requests, therefore we just have to remap these calls to
 * the classical request management functions as MPIO_Requests
 * are in fact normal MPI_Requests */

MPI_Fint PMPIO_Request_c2f( MPI_Request request )
{
	return MPI_Request_c2f( request );
}


MPI_Request PMPIO_Request_f2c(MPI_Fint rid )
{
	return MPI_Request_f2c( rid );
}

#endif


#if 0

/** \brief MPC type flattening function for ROMIO
 *  ROMIO has at his core data-type flattening, using this function we make it
 *  faster as we already stor ethe internal layout. And also more reliable
 *  as the recursive implementation ion ROMIO chrashed in some cases (darray).
 *
 *  \param datatype Data-type to be flattened
 *  \param blocklen OUT size of blocks
 *  \param indices OUT offsets of block starts
 *  \param count OUT number of entries
 */
int MPCX_Type_flatten(MPI_Datatype datatype, MPI_Aint **blocklen,
                      MPI_Aint **indices, MPI_Count *count) {
  mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();
  _mpc_dt_derived_t *target_derived_type;
  _mpc_dt_contiguout_t *contiguous_type;

  /* Nothing to do */
  if (datatype == MPI_DATATYPE_NULL) {
    *count = 0;
    return MPI_SUCCESS;
  }

  /* Here we decide what to do in function of the data-type kind:
   *
   * Common and contiguous :
   * 	=> Return start at 0 and the extent
   * Derived:
   * 	=> Extract the flattened representation from MPC
   */

  switch (_mpc_dt_get_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    *count = 1;

    *blocklen = sctk_malloc(*count * sizeof(MPI_Aint));
    *indices = sctk_malloc(*count * sizeof(MPI_Aint));

    assume(*blocklen != NULL);
    assume(*indices != NULL);

    (*indices)[0] = 0;
    (*blocklen)[0] = mpc_lowcomm_datatype_common_get_size(datatype);
    break;
  case MPC_DATATYPES_CONTIGUOUS:
    contiguous_type = _mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get(task_specific, datatype);
    *count = 1;

    *blocklen = sctk_malloc(*count * sizeof(MPI_Aint));
    *indices = sctk_malloc(*count * sizeof(MPI_Aint));

    assume(*blocklen != NULL);
    assume(*indices != NULL);

    (*indices)[0] = 0;
    (*blocklen)[0] = contiguous_type->size;
    break;

  case MPC_DATATYPES_DERIVED:
    target_derived_type =
        _mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get(task_specific, datatype);

    /* Note that we extract the optimized version of the data-type */
    *count = target_derived_type->opt_count;

    *blocklen = sctk_malloc(*count * sizeof(MPI_Aint));
    *indices = sctk_malloc(*count * sizeof(MPI_Aint));

    assume(*blocklen != NULL);
    assume(*indices != NULL);

    unsigned int i;

    /* Here we create start, len pairs from begins/ends pairs
     * note +1 in the extent as those boundaries are INCLUSIVE ! */
    for (i = 0; i < *count; i++) {
      (*indices)[i] = target_derived_type->opt_begins[i];
      (*blocklen)[i] = target_derived_type->opt_ends[i] -
                       target_derived_type->opt_begins[i] + 1;
    }

    break;

  case MPC_DATATYPES_UNKNOWN:
    mpc_common_debug_fatal("CANNOT PROCESS AN UNKNOWN DATATYPE");
    break;
  }

  return MPI_SUCCESS;
}

int MPIR_Type_flatten(MPI_Datatype type, MPI_Aint **off_array,
                      MPI_Aint **size_array, MPI_Aint *array_len_p) {
  mpc_common_debug_error("Hello flatten");
  return MPCX_Type_flatten(type, off_array, size_array, array_len_p);
}

MPI_Aint MPCX_Type_get_count(MPI_Datatype datatype) {
  mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();
  _mpc_dt_derived_t *target_derived_type;


  /* Nothing to do */
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_SUCCESS;
  }

  switch (_mpc_dt_get_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    return 1;
    break;
  case MPC_DATATYPES_CONTIGUOUS:
    return 1;
    break;

  case MPC_DATATYPES_DERIVED:
    target_derived_type =
        _mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get(task_specific, datatype);

    return target_derived_type->opt_count;
    break;

  case MPC_DATATYPES_UNKNOWN:
    mpc_common_debug_fatal("CANNOT PROCESS AN UNKNOWN DATATYPE");
    return 0;
    break;
  }

  return 0;
}
#endif
