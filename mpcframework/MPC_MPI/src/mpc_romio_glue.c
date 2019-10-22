#include "comm_lib.h"


#include "sctk_communicator.h"
#include "mpc_common_rank.h"
#include "datatype.h"
#include "mpc_mpi.h"
#include "mpc_launch_pmi.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "uthash.h"
#include <string.h>

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
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific;

	*flag = 0;

	/* UB or LB are not contiguous */
	if( _mpc_dt_is_boundary(datatype) )
		return;

  _mpc_dt_derived_t *target_type = NULL;
	switch( _mpc_dt_get_kind( datatype ) )
	{
		/* CONT and COMMON are contiguous by definition */
		case MPC_DATATYPES_CONTIGUOUS:
			*flag = 1;
		break;

		case MPC_DATATYPES_COMMON:
			*flag = 1;
		break;

		/* For dereived dat-types we have to check */
		case MPC_DATATYPES_DERIVED:
			task_specific = mpc_cl_per_mpi_process_ctx_get ();
			target_type = _mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get(  task_specific, datatype );
			assume( target_type != NULL );

			/* If there is no block (0 size) or one block in the optimized representation
			 * then this data-type is contiguous */
			if( (target_type->count == 0) ||  (target_type->opt_count == 1) )
				*flag = 1;

		break;

		default:
			not_reachable();
	}

}

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
    (*blocklen)[0] = _mpc_dt_common_get_size(datatype);
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
    sctk_fatal("CANNOT PROCESS AN UNKNOWN DATATYPE");
    break;
  }

  return MPI_SUCCESS;
}

int MPIR_Type_flatten(MPI_Datatype type, MPI_Aint **off_array,
                      MPI_Aint **size_array, MPI_Aint *array_len_p) {
  sctk_error("Hello flatten");
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
    sctk_fatal("CANNOT PROCESS AN UNKNOWN DATATYPE");
    return 0;
    break;
  }

  return 0;
}

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
  int comm_world_rank = sctk_get_comm_world_rank(comm, rank);

  struct mpc_launch_pmi_process_layout *nodes_infos = NULL;
  mpc_launch_pmi_get_process_layout(&nodes_infos);

  int node_number = mpc_common_get_node_count();

  struct mpc_launch_pmi_process_layout *tmp;

  int i, j;

  for (i = 0; i < node_number; i++) {
    HASH_FIND_INT(nodes_infos, &node_number, tmp);
    for (j = 0; j < tmp->nb_process; j++) {
      if (tmp->process_list[j] == comm_world_rank) {
        *id = i;
        return MPI_SUCCESS;
      }
    }
  }

  return MPI_SUCCESS;
}

/** \brief MPICH says check wether the progress engine is blocked assuming
 * "YIELD"
 */
void MPIR_Ext_cs_yield(void) { sctk_thread_yield(); }

/************************************************************************/
/* Locks                                                                */
/************************************************************************/

/* Here we gathered the locks which are required by ROMIO
 * in order to guarantee writing thread safety during
 * both strided and shared lock. The reason they have 
 * to be in MPC is that in ROMIO locks would be privatized
 * as global variables and it is not what we want. */

/* STRIDED LOCK */
mpc_common_spinlock_t mpio_strided_lock = 0;

void MPIO_lock_strided()
{
	mpc_common_spinlock_lock(&mpio_strided_lock);
}


void MPIO_unlock_strided()
{
	mpc_common_spinlock_unlock(&mpio_strided_lock);
}

/* SHARED LOCK */
mpc_common_spinlock_t mpio_shared_lock = 0;

void MPIO_lock_shared()
{
	mpc_common_spinlock_lock(&mpio_shared_lock);
}


void MPIO_unlock_shared()
{
	mpc_common_spinlock_unlock(&mpio_shared_lock);
}


/************************************************************************/
/* Dummy IO requests                                                    */
/************************************************************************/
/* These two functions are just reffered to in the FORTRAN
 * interface where the IO_Wait and IO_Tests bindings are 
 * defined, we do not expect them to be called as we rely
 * on extended requests. Therefore we just directly return */

int PMPIO_Wait(__UNUSED__ void *r, __UNUSED__ MPI_Status *s)
{
	return MPI_SUCCESS;
}

int PMPIO_Test(__UNUSED__ void * r, __UNUSED__ int * c, __UNUSED__ MPI_Status *s)
{
	return MPI_SUCCESS;
}


/************************************************************************/
/* C2F/F2C Functions                                                    */
/************************************************************************/

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

/* File Pointers */

unsigned int MPI_File_fortran_id = 0;

struct MPI_File_Fortran_cell
{
	int id;
	void * fh;
	UT_hash_handle hh;
};

struct MPI_File_Fortran_cell * mpi_file_lookup_table = NULL;
mpc_common_spinlock_t mpi_file_lookup_table_lock = SCTK_SPINLOCK_INITIALIZER;

MPI_Fint MPIO_File_c2f(void * fh)
{
	MPI_Fint ret = 0;
	
	mpc_common_spinlock_lock( &mpi_file_lookup_table_lock );

	struct MPI_File_Fortran_cell * new_cell = NULL;
	new_cell = sctk_malloc( sizeof( struct MPI_File_Fortran_cell ) );
	assume( new_cell != NULL );
	new_cell->fh = fh;
	new_cell->id = ++MPI_File_fortran_id;
	ret = new_cell->id;
	HASH_ADD_INT( mpi_file_lookup_table, id, new_cell );

	mpc_common_spinlock_unlock( &mpi_file_lookup_table_lock );
	
	return ret;
}

void * MPIO_File_f2c(int fid)
{
	void * ret = NULL;
	
	mpc_common_spinlock_lock( &mpi_file_lookup_table_lock );
	
	struct MPI_File_Fortran_cell * previous_cell = NULL;
	
	HASH_FIND_INT( mpi_file_lookup_table, &fid, previous_cell );
	
	if( previous_cell )
		ret = previous_cell->fh;
	
	mpc_common_spinlock_unlock( &mpi_file_lookup_table_lock );
	
	return ret;
}

/************************************************************************/
/* ERROR Handling                                                       */
/************************************************************************/

int MPIR_Err_create_code_valist(__UNUSED__ int a, __UNUSED__ int b, __UNUSED__ const char c[], __UNUSED__ int d,__UNUSED__ int e, 
				const char __UNUSED__  f[], const char __UNUSED__ g[],__UNUSED__  va_list args )
{
	return MPI_SUCCESS;
}

int MPIR_Err_is_fatal(__UNUSED__ int a)
{
	return 0;
}

typedef int (* MPIR_Err_get_class_string_func_t)(int error, char *str, int length);

void MPIR_Err_get_string( int errcode, char *msg, int maxlen,__UNUSED__  MPIR_Err_get_class_string_func_t fcname )
{
	char buff[128];
	int len;
	
	if( !msg )
		return;
	
	buff[0] = '\0';
	msg[0] = '\0';
	
	_mpc_cl_error_string (errcode, buff, &len);

	if( strlen( buff ) )
		snprintf( msg, maxlen, "%s", buff );
}


struct MPID_Comm;
int MPID_Abort(__UNUSED__ struct MPID_Comm *comm, int mpi_errno, int exit_code, const char *error_msg)
{
	sctk_error("FATAL : %d [exit : %d ] : %s", mpi_errno, exit_code, error_msg );
	MPI_Abort( MPI_COMM_WORLD, mpi_errno );
	return MPI_SUCCESS;
}



void MPIR_Get_file_error_routine( __UNUSED__ MPI_Errhandler a, 
				  __UNUSED__ void (**errr)(void * , int * , ...), 
				  __UNUSED__ int * b)
{
	
	
	
}


int MPIR_File_call_cxx_errhandler( __UNUSED__ void *fh, __UNUSED__ int *errorcode, 
			   __UNUSED__ void (*c_errhandler)(void  *, int *, ... ) )
{

	return MPI_SUCCESS;
}


int MPIO_Err_create_code(__UNUSED__ int lastcode, __UNUSED__ int fatal, const char fcname[],
			 int line, int error_class, const char generic_msg[],
			 const char specific_msg[], ... )
{
    va_list Argp;
    int idx = 0;
    char *buf;

    buf = (char *) sctk_malloc(1024);
    if (buf != NULL) {
	idx += snprintf(buf, 1023, "%s (line %d): ", fcname, line);
	if (specific_msg == NULL) {
	    snprintf(&buf[idx], 1023 - idx, "%s\n", generic_msg);
	}
	else {
	    va_start(Argp, specific_msg);
	    vsnprintf(&buf[idx], 1023 - idx, specific_msg, Argp);
	    va_end(Argp);
	}
	fprintf(stderr, "%s\n", buf);
	sctk_free(buf);
    }

    return error_class;
}

