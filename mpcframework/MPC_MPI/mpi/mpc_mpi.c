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
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */
#include <math.h>
#include <string.h>
#include <mpc_mpi_internal.h>
#include <mpc_internal_thread.h>

#include "sctk_ht.h"
#include "sctk_handle.h"
#include "mpc_mpi_halo.h"

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_mpi_weak.h"
#endif

#include "sctk_thread_generic.h"
#include "mpit_internal.h"

#if defined(MPC_Accelerators)
#include <sctk_accelerators.h>
#endif

char * sctk_char_fortran_to_c (char *buf, int size, char ** free_ptr)
{
	char *tmp;
	long int i;
	tmp = sctk_malloc (size + 1);
	TODO("check memory liberation")
	assume( tmp != NULL );
	*free_ptr = tmp;
	
	for (i = 0; i < size; i++)
	{
	tmp[i] = buf[i];
	}
	tmp[i] = '\0';
	
	/* Trim */

	while( *tmp == ' ')
	{
		tmp++;
	}
	
	int len = strlen( tmp );
	
	char *begin = tmp;
	
	while( (tmp[len - 1] == ' ') && (&tmp[len] != begin) )
	{
		tmp[len - 1] = '\0';
		len--;
	}
		
	
	
	return tmp;
}

void sctk_char_c_to_fortran (char *buf, int size)
{
  long int i;
  for (i = strlen (buf); i < size; i++)
    {
      buf[i] = ' ';
    }
}

/* Now Define Special Fortran pointers */

static void **mpi_predef08_bottom(void) {
  static void *mpi_bottom_ptr = NULL;
  return &mpi_bottom_ptr;
}

static void **mpi_predef08_inplace(void) {
  static void *mpi_in_place_ptr = NULL;
  return &mpi_in_place_ptr;
}

static void **mpi_predef08_status_ignore(void) {
  static void *mpi_status_ignore_ptr = NULL;
  return &mpi_status_ignore_ptr;
}

static void **mpi_predef08_statuses_ignore(void) {
  static void *mpi_statuses_ignore_ptr = NULL;
  return &mpi_statuses_ignore_ptr;
}

void mpc_predef08_init_inplace_(void *inplace) {
  *(mpi_predef08_inplace()) = inplace;
}

void mpc_predef08_init_bottom_(void *bottom) {
  *(mpi_predef08_bottom()) = bottom;
}

void mpc_predef08_init_status_ignore_(void *status_ignore) {
  *(mpi_predef08_status_ignore()) = status_ignore;
}

void mpc_predef08_init_statuses_ignore_(void *statuses_ignore) {
  *(mpi_predef08_statuses_ignore()) = statuses_ignore;
}

static void **mpi_predef_bottom(void) {
  static void *mpi_bottom_ptr = NULL;
  return &mpi_bottom_ptr;
}

static void **mpi_predef_inplace(void) {
  static void *mpi_in_place_ptr = NULL;
  return &mpi_in_place_ptr;
}

static void **mpi_predef_status_ignore(void) {
  static void *mpi_status_ignore_ptr = NULL;
  return &mpi_status_ignore_ptr;
}

static void **mpi_predef_statuses_ignore(void) {
  static void *mpi_statuses_ignore_ptr = NULL;
  return &mpi_statuses_ignore_ptr;
}

void mpc_predef_init_inplace_(void *inplace) {
  *(mpi_predef_inplace()) = inplace;
}

void mpc_predef_init_bottom_(void *bottom) { *(mpi_predef_bottom()) = bottom; }

void mpc_predef_init_status_ignore_(void *status_ignore) {
  *(mpi_predef_status_ignore()) = status_ignore;
}

void mpc_predef_init_statuses_ignore_(void *statuses_ignore) {
  *(mpi_predef_statuses_ignore()) = statuses_ignore;
}

static volatile int did_resolve_fortran_binds = 0;
static sctk_spinlock_t did_resolve_fortran_binds_lock;

static inline void fortran_check_binds_resolve() {
  if (did_resolve_fortran_binds) {
    return;
  }

  sctk_spinlock_lock_yield(&did_resolve_fortran_binds_lock);

  if (did_resolve_fortran_binds) {
    sctk_spinlock_unlock(&did_resolve_fortran_binds_lock);
    return;
  }

  void *handle = dlopen(NULL, RTLD_LAZY);

  void (*fortran_init)();

  fortran_init =
      (void (*)())dlsym(handle, "mpc_fortran_init_predefined_constants_");

  if (!fortran_init) {
    fortran_init =
        (void (*)())dlsym(handle, "mpc_fortran_init_predefined_constants__");
  }

  if (!fortran_init) {
    fortran_init =
        (void (*)())dlsym(handle, "mpc_fortran_init_predefined_constants___");
  }

  if (fortran_init) {
    (fortran_init)();
  } else {
    sctk_nodebug("No symbol");
  }

  void (*fortran08_init)();

  fortran08_init =
      (void (*)())dlsym(handle, "mpc_fortran08_init_predefined_constants_");

  if (!fortran08_init) {
    fortran08_init =
        (void (*)())dlsym(handle, "mpc_fortran08_init_predefined_constants__");
  }

  if (!fortran08_init) {
    fortran_init =
        (void (*)())dlsym(handle, "mpc_fortran08_init_predefined_constants___");
  }

  if (fortran08_init) {
    (fortran08_init)();
  } else {
    sctk_nodebug("No symbol08");
  }

  dlclose(handle);

  did_resolve_fortran_binds = 1;

  sctk_spinlock_unlock(&did_resolve_fortran_binds_lock);
}

#undef ffunc
#define ffunc(a) a##_
#include <mpc_mpi_fortran.h>

#undef ffunc
#define	ffunc(a) a##__
#include <mpc_mpi_fortran.h>
#undef ffunc

/*
  INTERNAL FUNCTIONS
*/
static int __INTERNAL__PMPI_Send (void *, int, MPI_Datatype, int, int,
				  MPI_Comm);
static int __INTERNAL__PMPIX_Swap(void **sendrecv_buf , int remote_rank, MPI_Count size , MPI_Comm comm);
static int __INTERNAL__PMPIX_Exchange(void **send_buf , void **recvbuff, int remote_rank, MPI_Count size , MPI_Comm comm);
static int __INTERNAL__PMPI_Recv (void *, int, MPI_Datatype, int, int,
				  MPI_Comm, MPI_Status *);
static int __INTERNAL__PMPI_Get_count (MPI_Status *, MPI_Datatype, int *);
static int __INTERNAL__PMPI_Bsend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm);
static int __INTERNAL__PMPI_Ssend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm);
static int __INTERNAL__PMPI_Rsend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm);
static int __INTERNAL__PMPI_Buffer_attach (void *, int);
static int __INTERNAL__PMPI_Buffer_detach (void *, int *);
static int __INTERNAL__PMPI_Isend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Ibsend (void *, int, MPI_Datatype, int, int,
				    MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Issend (void *, int, MPI_Datatype, int, int,
				    MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Irsend (void *, int, MPI_Datatype, int, int,
				    MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Wait (MPI_Request *, MPI_Status *);
static int __INTERNAL__PMPI_Test (MPI_Request *, int *, MPI_Status *);
static int __INTERNAL__PMPI_Request_free (MPI_Request *);
static int __INTERNAL__PMPI_Testany (int, MPI_Request *, int *, int *,
				     MPI_Status *);
static int __INTERNAL__PMPI_Testall (int, MPI_Request *, int *, MPI_Status *);
static int __INTERNAL__PMPI_Waitsome (int, MPI_Request *, int *, int *,
				      MPI_Status *);
static int __INTERNAL__PMPI_Testsome (int, MPI_Request *, int *, int *,
				      MPI_Status *);
static int __INTERNAL__PMPI_Iprobe (int, int, MPI_Comm, int *, MPI_Status *);
static int __INTERNAL__PMPI_Probe (int, int, MPI_Comm, MPI_Status *);
static int __INTERNAL__PMPI_Cancel (MPI_Request *);
static int __INTERNAL__PMPI_Test_cancelled (MPI_Status *, int *);
static int __INTERNAL__PMPI_Send_init (void *, int, MPI_Datatype, int, int,
				       MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Bsend_init (void *, int, MPI_Datatype, int, int,
					MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Ssend_init (void *, int, MPI_Datatype, int, int,
					MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Rsend_init (void *, int, MPI_Datatype, int, int,
					MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Recv_init (void *, int, MPI_Datatype, int, int,
				       MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Start (MPI_Request *);
static int __INTERNAL__PMPI_Startall (int, MPI_Request *);
static int __INTERNAL__PMPI_Sendrecv (void *, int, MPI_Datatype, int, int,
				      void *, int, MPI_Datatype, int, int,
				      MPI_Comm, MPI_Status *);
static int __INTERNAL__PMPI_Sendrecv_replace (void *, int, MPI_Datatype, int,
					      int, int, int, MPI_Comm,
					      MPI_Status *);
static int __INTERNAL__PMPI_Type_contiguous (unsigned long, MPI_Datatype,
					     MPI_Datatype *);
static int __INTERNAL__PMPI_Type_contiguous_inherits (unsigned long, MPI_Datatype,
					     MPI_Datatype *, struct Datatype_External_context * ctx );
static int __INTERNAL__PMPI_Type_vector (int, int, int, MPI_Datatype,
					 MPI_Datatype *);
static int __INTERNAL__PMPI_Type_hvector (int, int, MPI_Aint, MPI_Datatype,
					  MPI_Datatype *);
static int __INTERNAL__PMPI_Type_indexed (int, int *, int *, MPI_Datatype,
					  MPI_Datatype *);
static int __INTERNAL__PMPI_Type_hindexed (int, int *, MPI_Aint *,
					   MPI_Datatype, MPI_Datatype *);
static int __INTERNAL__PMPI_Type_struct (int, int *, MPI_Aint *,
					 MPI_Datatype *, MPI_Datatype *);
static int __INTERNAL__PMPI_Address (void *, MPI_Aint *);
int __INTERNAL__PMPI_Type_extent (MPI_Datatype, MPI_Aint *);
int __INTERNAL__PMPI_Type_size (MPI_Datatype, int *);
static int __INTERNAL__PMPI_Type_lb (MPI_Datatype, MPI_Aint *);
static int __INTERNAL__PMPI_Type_ub (MPI_Datatype, MPI_Aint *);
static int __INTERNAL__PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type);
static int __INTERNAL__PMPI_Type_commit (MPI_Datatype *);
static int __INTERNAL__PMPI_Type_free (MPI_Datatype *);
static int __INTERNAL__PMPI_Get_elements_x (MPI_Status *, MPI_Datatype, MPI_Count *);
int __INTERNAL__PMPI_Type_create_darray (int size,
					 int rank,
					 int ndims,
					 int array_of_gsizes[],
					 int array_of_distribs[],
					 int array_of_dargs[],
					 int array_of_psizes[],
					 int order,
					 MPI_Datatype oldtype,
					 MPI_Datatype *newtype);
int __INTERNAL__PMPI_Type_create_subarray (int ndims,
					   int array_of_sizes[],
					   int array_of_subsizes[],
					   int array_of_starts[],
					   int order,
					   MPI_Datatype oldtype,
					   MPI_Datatype * new_type);

int __INTERNAL__PMPI_Pack_external_size (char *datarep , int incount, MPI_Datatype datatype, MPI_Aint *size);
int __INTERNAL__PMPI_Pack_external (char *datarep , void *inbuf, int incount, MPI_Datatype datatype, void * outbuf, MPI_Aint outsize, MPI_Aint * position);
int __INTERNAL__PMPI_Unpack_external (char * datarep, void * inbuf, MPI_Aint insize, MPI_Aint * position, void * outbuf, int outcount, MPI_Datatype datatype);

static inline int __INTERNAL__PMPI_Barrier(MPI_Comm);
int __INTERNAL__PMPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Gather (void *, int, MPI_Datatype, void *, int,
				    MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Gatherv (void *, int, MPI_Datatype, void *, int *,
				     int *, MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Scatter (void *, int, MPI_Datatype, void *, int,
				     MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Scatterv (void *, int *, int *, MPI_Datatype,
				      void *, int, MPI_Datatype, int,
				      MPI_Comm);
int __INTERNAL__PMPI_Allgather (void *, int, MPI_Datatype, void *, int,
				       MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Allgatherv (void *, int, MPI_Datatype, void *,
					int *, int *, MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Alltoall (void *, int, MPI_Datatype, void *, int,
				      MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Alltoallv (void *, int *, int *, MPI_Datatype,
				       void *, int *, int *, MPI_Datatype,
				       MPI_Comm);
				       
/* Neighbor collectives */
static int __INTERNAL__PMPI_Neighbor_allgather_cart(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_allgather_graph(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_allgatherv_cart(void *, int , MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_allgatherv_graph(void *, int , MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoall_cart(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoall_graph(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoallv_cart(void *, int [], int [], MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoallv_graph(void *, int [], int [], MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoallw_cart(void *, int [], MPI_Aint [], MPI_Datatype [], void *, int [], MPI_Aint [], MPI_Datatype [], MPI_Comm comm);
static int __INTERNAL__PMPI_Neighbor_alltoallw_graph(void *, int [], MPI_Aint [], MPI_Datatype [], void *, int [], MPI_Aint [], MPI_Datatype [], MPI_Comm comm);
int __INTERNAL__PMPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op,
				    int, MPI_Comm);
static int __INTERNAL__PMPI_Op_create (MPI_User_function *, int, MPI_Op *);
static int __INTERNAL__PMPI_Op_free (MPI_Op *);
int __INTERNAL__PMPI_Allreduce (void *, void *, int, MPI_Datatype,
				       MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Reduce_scatter (void *, void *, int *,
					    MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Reduce_scatter_block (void *, void *, int,
					    MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op,
				  MPI_Comm);
int __INTERNAL__PMPI_Exscan (void *, void *, int, MPI_Datatype, MPI_Op,
				  MPI_Comm);
static int __INTERNAL__PMPI_Group_size (MPI_Group, int *);
static int __INTERNAL__PMPI_Group_rank (MPI_Group, int *);
static int __INTERNAL__PMPI_Group_translate_ranks (MPI_Group, int, int *,
						   MPI_Group, int *);
static int __INTERNAL__PMPI_Group_compare (MPI_Group, MPI_Group, int *);
static int __INTERNAL__PMPI_Comm_group (MPI_Comm, MPI_Group *);
static int __INTERNAL__PMPI_Group_union (MPI_Group, MPI_Group, MPI_Group *);
static int __INTERNAL__PMPI_Group_intersection (MPI_Group, MPI_Group,
						MPI_Group *);
static int __INTERNAL__PMPI_Group_difference (MPI_Group, MPI_Group,
					      MPI_Group *);
static int __INTERNAL__PMPI_Group_incl (MPI_Group, int, int *, MPI_Group *);
static int __INTERNAL__PMPI_Group_excl (MPI_Group, int, int *, MPI_Group *);
static int __INTERNAL__PMPI_Group_range_incl (MPI_Group, int, int[][3],
					      MPI_Group *);
static int __INTERNAL__PMPI_Group_range_excl (MPI_Group, int, int[][3],
					      MPI_Group *);
static int __INTERNAL__PMPI_Group_free (MPI_Group *);
static int __INTERNAL__PMPI_Comm_size (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_rank (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_compare (MPI_Comm, MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_dup (MPI_Comm, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_create (MPI_Comm, MPI_Group, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_create_from_intercomm (MPI_Comm, MPI_Group, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_split (MPI_Comm, int, int, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_free (MPI_Comm *);
static int __INTERNAL__PMPI_Comm_test_inter (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_remote_size (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_remote_group (MPI_Comm, MPI_Group *);
static int __INTERNAL__PMPI_Intercomm_create (MPI_Comm, int, MPI_Comm, int,
					      int, MPI_Comm *);
static int __INTERNAL__PMPI_Intercomm_merge (MPI_Comm, int, MPI_Comm *);
static int __INTERNAL__PMPI_Keyval_create (MPI_Copy_function *,
					   MPI_Delete_function *, int *,
					   void *);
static int __INTERNAL__PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
										MPI_Comm_delete_attr_function *comm_delete_attr_fn,
										int *comm_keyval, void *extra_state);
static int __INTERNAL__PMPI_Comm_free_keyval(int *comm_keyval);
static int __INTERNAL__PMPI_Keyval_free (int *);
static int __INTERNAL__PMPI_Attr_put (MPI_Comm, int, void *);
static int __INTERNAL__PMPI_Attr_get (MPI_Comm, int, void *, int *);
static int __INTERNAL__PMPI_Attr_get_fortran (MPI_Comm, int, int *, int *);
static int __INTERNAL__PMPI_Attr_delete (MPI_Comm, int);
static int __INTERNAL__PMPI_Topo_test (MPI_Comm, int *);
static int __INTERNAL__PMPI_Cart_create (MPI_Comm, int, int *, int *, int,
					 MPI_Comm *);
static int __INTERNAL__PMPI_Dims_create (int, int, int *);
static int __INTERNAL__PMPI_Graph_create (MPI_Comm, int, int *, int *, int,
					  MPI_Comm *);
static int __INTERNAL__PMPI_Graphdims_get (MPI_Comm, int *, int *);
static int __INTERNAL__PMPI_Graph_get (MPI_Comm, int, int, int *, int *);
static int __INTERNAL__PMPI_Cartdim_get (MPI_Comm, int *);
static int __INTERNAL__PMPI_Cart_get (MPI_Comm, int, int *, int *, int *);
static int __INTERNAL__PMPI_Cart_rank (MPI_Comm, int *, int *, int);
static int __INTERNAL__PMPI_Cart_coords (MPI_Comm, int, int, int *, int);
static int __INTERNAL__PMPI_Graph_neighbors_count (MPI_Comm, int, int *);
static int __INTERNAL__PMPI_Graph_neighbors (MPI_Comm, int, int, int *);
static int __INTERNAL__PMPI_Cart_shift (MPI_Comm, int, int, int *, int *);
static int __INTERNAL__PMPI_Cart_sub (MPI_Comm, int *, MPI_Comm *);
static int __INTERNAL__PMPI_Cart_map (MPI_Comm, int, int *, int *, int *);
static int __INTERNAL__PMPI_Graph_map (MPI_Comm, int, int *, int *, int *);
static int __INTERNAL__PMPI_Get_processor_name (char *, int *);
static int __INTERNAL__PMPI_Get_version (int *, int *);
static int __INTERNAL__PMPI_Errhandler_create (MPI_Handler_function *,
					       MPI_Errhandler *);
static int __INTERNAL__PMPI_Errhandler_set (MPI_Comm, MPI_Errhandler);
static int __INTERNAL__PMPI_Errhandler_get (MPI_Comm, MPI_Errhandler *);
static int __INTERNAL__PMPI_Errhandler_free (MPI_Errhandler *);
static int __INTERNAL__PMPI_Error_string (int, char *, int *);
static int __INTERNAL__PMPI_Error_class (int, int *);
static double __INTERNAL__PMPI_Wtime (void);
static double __INTERNAL__PMPI_Wtick (void);
static int __INTERNAL__PMPI_Init (int *, char ***);
static int __INTERNAL__PMPI_Finalize (void);
static int __INTERNAL__PMPI_Finalized (int*);
static int __INTERNAL__PMPI_Initialized (int *);
static int __INTERNAL__PMPI_Abort (MPI_Comm, int);
struct MPI_request_struct_s;
static int __INTERNAL__PMPI_Isend_test_req (void *buf, int count,
					    MPI_Datatype datatype, int dest, int tag, 
					    MPI_Comm comm,
					    MPI_Request * request, int is_valid_request,
					    struct MPI_request_struct_s* requests);

  /* MPI-2 functions */
static int __INTERNAL__PMPI_Comm_get_name (MPI_Comm, char *, int *);
static int __INTERNAL__PMPI_Comm_set_name (MPI_Comm, char *);
static int __INTERNAL__PMPI_Init_thread (int *, char ***, int, int *);
static int __INTERNAL__PMPI_Query_thread (int *);

/* checkpoint */
static int __INTERNAL__PMPIX_Checkpoint(MPIX_Checkpoint_state*);

/* Halo */

static int __INTERNAL__PMPIX_Halo_cell_init( MPI_Halo * halo, char * label, MPI_Datatype type, int count );
static int __INTERNAL__PMPIX__Halo_cell_release( MPI_Halo * halo );

static int __INTERNAL__PMPIX__Halo_cell_set( MPI_Halo halo, void * ptr );
static int __INTERNAL__PMPIX__Halo_cell_get( MPI_Halo halo, void ** ptr );

/* Halo Exchange */

static int __INTERNAL__PMPIX__Halo_exchange_init( MPI_Halo_exchange * ex );
static int __INTERNAL__PMPIX__Halo_exchange_release( MPI_Halo_exchange * ex );

static int __INTERNAL__PMPIX__Halo_exchange_commit( MPI_Halo_exchange ex );
static int __INTERNAL__PMPIX__Halo_exchange( MPI_Halo_exchange ex );
static int __INTERNAL__PMPIX__Halo_iexchange( MPI_Halo_exchange ex );
static int __INTERNAL__PMPIX_Halo_iexchange_wait( MPI_Halo_exchange ex );

static int __INTERNAL__PMPIX__Halo_cell_bind_local( MPI_Halo_exchange ex, MPI_Halo halo );
static int __INTERNAL__PMPIX__Halo_cell_bind_remote( MPI_Halo_exchange ex, MPI_Halo halo, int remote, char * remote_label );




typedef struct
{
  void *attr;
  int flag;
} MPI_Caching_key_value_t;

typedef struct
{
  int ndims;
  int *dims;
  int *periods;
  int reorder;
} mpi_topology_cart_t;

typedef struct
{
  int nnodes;
  int nedges;
  int nindex;

  int *edges;
  int *index;
  int reorder;
} mpi_topology_graph_t;

typedef union
{
  mpi_topology_cart_t cart;
  mpi_topology_graph_t graph;
} mpi_topology_info_t;

typedef struct
{
  int type;
  mpi_topology_info_t data;
  char names[MPI_MAX_NAME_STRING + 1];
  sctk_spinlock_t lock;
} mpi_topology_per_comm_t;

#define MAX_TOPO_DEPTH 10

typedef struct mpc_mpi_per_communicator_s{

  /****** Attributes ******/
  MPI_Caching_key_value_t* key_vals;
  int max_number;

  /****** Topologies ******/
  mpi_topology_per_comm_t topo;

  /****** LOCK ******/
  sctk_spinlock_t lock;
}mpc_mpi_per_communicator_t;

#define MPC_MPI_MAX_NUMBER_FUNC 3

struct MPI_request_struct_s;
struct MPI_group_struct_s;
typedef struct MPI_group_struct_s MPI_group_struct_t;
struct MPI_buffer_struct_s;
typedef struct MPI_buffer_struct_s MPI_buffer_struct_t;
struct sctk_mpi_ops_s;
typedef struct sctk_mpi_ops_s sctk_mpi_ops_t;



static
void mpc_mpi_per_communicator_copy_func(mpc_mpi_per_communicator_t** to, mpc_mpi_per_communicator_t* from)
{
	int i = 0;
	sctk_spinlock_lock (&(from->lock));
	*to = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s));
	memcpy(*to,from,sizeof(mpc_mpi_per_communicator_t));
	((*to)->key_vals) = sctk_malloc(from->max_number*sizeof(MPI_Caching_key_value_t));
	for(i = 0 ; i < from->max_number ; i++)
	{
		((*to)->key_vals[i].attr) = NULL;
		((*to)->key_vals[i].flag) = 0;
	}

        /* Reset TOPO */
        (*to)->topo.lock = 0;
        (*to)->topo.type = MPI_UNDEFINED;

        sctk_spinlock_unlock(&(from->lock));
        sctk_spinlock_unlock(&((*to)->lock));
}

static
void mpc_mpi_per_communicator_dup_copy_func(mpc_mpi_per_communicator_t** to, mpc_mpi_per_communicator_t* from)
{
	sctk_spinlock_lock (&(from->lock));
	*to = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s));
	memcpy(*to,from,sizeof(mpc_mpi_per_communicator_t));

	/* Reset TOPO */
        (*to)->topo.lock = 0;
        (*to)->topo.type = MPI_UNDEFINED;

        sctk_spinlock_unlock(&(from->lock));
        sctk_spinlock_unlock(&((*to)->lock));
}

static inline mpc_mpi_per_communicator_t *
mpc_mpc_get_per_comm_data(sctk_communicator_t comm) {
  struct sctk_task_specific_s *task_specific;
  mpc_per_communicator_t *tmp;

  static __thread int task_rank = -1;
  static __thread int tcomm = MPI_COMM_NULL;
  static __thread mpc_mpi_per_communicator_t *data = NULL;

  if (task_rank == sctk_get_task_rank()) {
    if (tcomm == comm) {
      return data;
    }
  }

  task_specific = __MPC_get_task_specific();
  tmp = sctk_thread_getspecific_mpc_per_comm(task_specific, comm);

  if (tmp == NULL)
    return NULL;

  data = tmp->mpc_mpi_per_communicator;
  task_rank = sctk_get_task_rank();
  tcomm = comm;

  return tmp->mpc_mpi_per_communicator;
}

static void __sctk_init_mpi_topo_per_comm(mpc_mpi_per_communicator_t *tmp) {
  tmp->topo.type = MPI_UNDEFINED;
  sprintf(tmp->topo.names, "undefined");
}

int mpc_mpi_per_communicator_init(mpc_mpi_per_communicator_t *pc) {
  __sctk_init_mpi_topo_per_comm(pc);
  pc->max_number = 0;
  pc->topo.lock = SCTK_SPINLOCK_INITIALIZER;

  return 0;
}

static inline mpc_mpi_data_t * mpc_mpc_get_per_task_data()
{
	struct sctk_task_specific_s * task_specific;
	task_specific = __MPC_get_task_specific ();
	return task_specific->mpc_mpi_data;
}

static inline void PMPC_Get_requests(struct MPI_request_struct_s **requests){
  *requests = mpc_mpc_get_per_task_data()->requests;
}

static inline void PMPC_Set_requests(struct MPI_request_struct_s *requests){
  mpc_mpc_get_per_task_data()->requests = requests;
}

static inline void PMPC_Get_groups(struct MPI_group_struct_s **groups){
  *groups = mpc_mpc_get_per_task_data()->groups;
}

static inline void PMPC_Set_groups(struct MPI_group_struct_s *groups){
  mpc_mpc_get_per_task_data()->groups = groups;
}

static inline void PMPC_Get_buffers(struct MPI_buffer_struct_s **buffers){
  *buffers = mpc_mpc_get_per_task_data()->buffers;
}

static inline void PMPC_Set_buffers(struct MPI_buffer_struct_s *buffers){
  mpc_mpc_get_per_task_data()->buffers = buffers;
}

static inline void PMPC_Get_op(struct sctk_mpi_ops_s **ops){
  *ops = mpc_mpc_get_per_task_data()->ops;
}

static inline void PMPC_Set_op(struct sctk_mpi_ops_s *ops){
  mpc_mpc_get_per_task_data()->ops = ops;
}

/** Fast yield logic */

static volatile int __do_yield = 0;

/** Do we need to yield in this process for collectives (overloaded)
 */
static inline void sctk_init_yield_as_overloaded() {
  if (sctk_get_cpu_number() < sctk_get_local_task_number()) {
    __do_yield = 1;
  }

  if (1 < sctk_get_process_number()) {
    // We need to progress messages
    __do_yield = 1;
  }
}

/*
  ERRORS HANDLING
*/
inline int
SCTK__MPI_ERROR_REPORT__ (MPC_Comm comm, int error, char *message, char *file,
			  int line)
{
  MPI_Errhandler errh = (MPI_Errhandler)sctk_handle_get_errhandler(
      (sctk_handle)comm, SCTK_HANDLE_COMM);

  if (errh != MPI_ERRHANDLER_NULL) {
    sctk_nodebug("ERRH is %d for %d", errh, comm);
    MPI_Handler_function *func = sctk_errhandler_resolve(errh);
    int error_id = error;
    MPI_Comm comm_id = comm;

    sctk_nodebug("CALL %p (%d)", func, errh);
    if (func) {
      (func)(&comm_id, &error_id, message, file, line);
    }
  }
  return error;
}

static void __sctk_init_mpi_errors() { __MPC_Error_init(); }

#define MPI_ERROR_SUCESS() return MPI_SUCCESS

#define mpi_check_type(datatype,comm)		\
  if (datatype == MPI_DATATYPE_NULL) \
    MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "Bad datatype provided");

#define mpi_check_type_create(datatype,comm)		\
  if ((datatype >= SCTK_USER_DATA_TYPES_MAX) && (sctk_datatype_is_derived(datatype) != 1) && (sctk_datatype_is_contiguous(datatype) != 1) && ((datatype != MPI_UB) && (datatype != MPI_LB))) \
    MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "");

static int is_finalized = 0;
static int is_initialized = 0;

TODO("to optimize")
#define mpi_check_comm(com, comm)                                              \
  if ((is_finalized != 0) || (is_initialized != 1)) {                          \
    MPI_ERROR_REPORT(MPC_COMM_WORLD, MPI_ERR_OTHER, "");                       \
  } else if (com == MPI_COMM_NULL) {                                           \
    MPI_ERROR_REPORT(MPC_COMM_WORLD, MPI_ERR_COMM, "Error in communicator");   \
  }

#define mpi_check_status(status,comm)		\
  if(status == MPI_STATUS_IGNORE)	\
    MPI_ERROR_REPORT(comm,MPI_ERR_IN_STATUS,"Error status is MPI_STATUS_IGNORE")

#define mpi_check_buf(buf,comm)					\
 if((buf == NULL) && (buf != MPI_BOTTOM))                                     \
    MPI_ERROR_REPORT(comm,MPI_ERR_BUFFER,"")


#define mpi_check_count(count,comm)				\
  if(count < 0)							\
    MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"Error count cannot be negative")

#define mpi_check_rank(task,max_rank,comm)		\
  if((((task < 0) || (task >= max_rank)) && (sctk_is_inter_comm (comm) == 0)) && (task != MPI_ANY_SOURCE) && (task != MPI_PROC_NULL)) \
    MPI_ERROR_REPORT(comm,MPI_ERR_RANK,"Error bad rank provided")

#define mpi_check_rank_send(task,max_rank,comm)		\
  if((((task < 0) || (task >= max_rank)) && (sctk_is_inter_comm (comm) == 0)) && (task != MPI_PROC_NULL)) \
    MPI_ERROR_REPORT(comm,MPI_ERR_RANK,"Error bad rank provided")

#define mpi_check_root(task,max_rank,comm)		\
  if(((task < 0) || (task >= max_rank)) && (task != MPI_PROC_NULL) && (task != MPI_ROOT))		\
    MPI_ERROR_REPORT(comm,MPI_ERR_ROOT,"Error bad root rank provided")

#define mpi_check_tag(tag,comm)				\
  if(((tag < 0) || (tag > MPI_TAG_UB_VALUE)) && (tag != MPI_ANY_TAG))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_TAG,"Error bad tag provided")

#define mpi_check_tag_send(tag,comm)				\
  if(((tag < 0) || (tag > MPI_TAG_UB_VALUE)))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_TAG,"Error bad tag provided")

#define mpi_check_op_type_func(t) case t: return mpi_check_op_type_func_##t(datatype)
#define mpi_check_op_type_func_notavail(t) case t: return 1 

#if 0
static int mpi_check_op_type_func_MPI_(MPI_Datatype datatype){
        switch(datatype){
		default:return 0;
        }
        return 0;
}
#endif
#define mpi_check_op_type_func_integer()\
	mpi_check_op_type_func_notavail(MPC_INT);\
        mpi_check_op_type_func_notavail(MPC_LONG)

#define mpi_check_op_type_func_float()\
        mpi_check_op_type_func_notavail(MPC_FLOAT);\
	mpi_check_op_type_func_notavail(MPC_DOUBLE);\
    mpi_check_op_type_func_notavail(MPC_LONG_DOUBLE)


static int mpi_check_op_type_func_MPI_SUM(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default: return 0;
        }
        return 0;
}

static int mpi_check_op_type_func_MPI_MAX(MPI_Datatype datatype){
	switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);
	}
	return 0;
}
static int mpi_check_op_type_func_MPI_MIN(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_PROD(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);	
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_LAND(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_float();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_BAND(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_LOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_BOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_LXOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_BXOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_MINLOC(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_integer();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_MAXLOC(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_integer();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}

static int mpi_check_op_type(MPI_Op op, MPI_Datatype datatype){
#if 1
	if((op <= MPI_MAXLOC) && ( sctk_datatype_is_common( datatype) )){
		switch(op){
			mpi_check_op_type_func(MPI_SUM);
			mpi_check_op_type_func(MPI_MAX);
			mpi_check_op_type_func(MPI_MIN);
			mpi_check_op_type_func(MPI_PROD);
			mpi_check_op_type_func(MPI_LAND);
			mpi_check_op_type_func(MPI_BAND);
			mpi_check_op_type_func(MPI_LOR);
			mpi_check_op_type_func(MPI_BOR);
			mpi_check_op_type_func(MPI_LXOR);
			mpi_check_op_type_func(MPI_BXOR);
			mpi_check_op_type_func(MPI_MINLOC);
			mpi_check_op_type_func(MPI_MAXLOC);		
		}
		return 0;
	}
#endif
	return 0;
}

#define mpi_check_op(op,type,comm)				\
  if((op == MPI_OP_NULL) || mpi_check_op_type(op,type))							\
    MPI_ERROR_REPORT(comm,MPI_ERR_OP,"")

static int SCTK__MPI_Attr_clean_communicator (MPI_Comm comm);
static int SCTK__MPI_Attr_communicator_dup (MPI_Comm old, MPI_Comm new);

/* const MPI_Comm MPI_COMM_SELF = MPC_COMM_SELF; */

/*
 * MPI Level Per Thread CTX 
 *
 */


#ifdef MPC_MPI_USE_REQUEST_CACHE
    #define MPC_MPI_REQUEST_CACHE_SIZE 128
#endif


#ifdef MPC_MPI_USE_LOCAL_REQUESTS_QUEUE
    #define MPC_MPI_LOCAL_REQUESTS_QUEUE_SIZE 10
#endif

typedef struct MPI_per_thread_ctx_s
{
#ifdef MPC_MPI_USE_REQUEST_CACHE
    MPI_internal_request_t* mpc_mpi_request_cache[MPC_MPI_REQUEST_CACHE_SIZE];
#endif

#ifdef MPC_MPI_USE_LOCAL_REQUESTS_QUEUE
    MPI_internal_request_t * mpc_mpi_local_request_queue;
    int mpc_mpi_local_request_queue_nb_req;
#endif
}MPI_per_thread_ctx_t;



static inline MPI_per_thread_ctx_t * MPI_per_thread_ctx_new()
{
    MPI_per_thread_ctx_t * ret = sctk_malloc(sizeof(MPI_per_thread_ctx_t));

    if( !ret )
    {
        sctk_abort();
    }

#ifdef MPC_MPI_USE_LOCAL_REQUESTS_QUEUE
    ret->mpc_mpi_local_request_queue = NULL;
    ret->mpc_mpi_local_request_queue_nb_req = 0;
#endif 

    return ret;
}



static inline MPI_per_thread_ctx_t * MPI_per_thread_ctx_get()
{

    sctk_thread_data_t * th = sctk_thread_data_get();

    if( !th->mpi_per_thread )
    {
        th->mpi_per_thread = MPI_per_thread_ctx_new();
    }

    return th->mpi_per_thread;
}




MPI_request_struct_t * __sctk_internal_get_MPC_requests()
{
  MPI_request_struct_t *requests;
  PMPC_Get_requests ((void *) &requests);
  return requests;
}

/** \brief Initialize MPI interface request handling */
void __sctk_init_mpc_request() {
  static sctk_thread_mutex_t sctk_request_lock = SCTK_THREAD_MUTEX_INITIALIZER;
  MPI_request_struct_t *requests;

  /* Check wether requests are already initalized */
  PMPC_Get_requests((void *)&requests);
  assume(requests == NULL);

  sctk_thread_mutex_lock(&sctk_request_lock);

  /* Allocate the request structure */
  requests = sctk_malloc(sizeof(MPI_request_struct_t));

  /*Init request struct */
  requests->lock = 0;
  requests->tab = NULL;
  requests->free_list = NULL;
  requests->auto_free_list = NULL;
  requests->max_size = 0;
  /* Create the associated buffered allocator */
  sctk_buffered_alloc_create(&(requests->buf), sizeof(MPI_internal_request_t));

  /* Register the new array in the task specific data-structure */
  PMPC_Set_requests(requests);

  sctk_thread_mutex_unlock(&sctk_request_lock);
}

#ifdef MPC_MPI_USE_REQUEST_CACHE

inline MPI_internal_request_t *
__sctk_convert_mpc_request_internal_cache_get (MPI_Request * req,
                                           MPI_request_struct_t *requests)
{
        int id;
        MPI_internal_request_t * tmp;

        id = *req;
        id = id % MPC_MPI_REQUEST_CACHE_SIZE;

        tmp = mpc_mpi_request_cache[id];

        if((tmp->rank != *req) || (tmp->task_req_id != requests))
                return NULL;

        return tmp;
}

inline void __sctk_convert_mpc_request_internal_cache_register(MPI_internal_request_t * req)
{
        int id;

        id = req->rank % MPC_MPI_REQUEST_CACHE_SIZE;
        mpc_mpi_request_cache[id] = req;
}
#else

#define __sctk_convert_mpc_request_internal_cache_get(a,b) NULL
#define __sctk_convert_mpc_request_internal_cache_register(a) (void)(0)

#endif

inline void sctk_delete_internal_request_clean(MPI_internal_request_t *tmp)
{
        /* Release the internal request */
        tmp->used = 0;
        sctk_free (tmp->saved_datatype);
        tmp->saved_datatype = NULL;

}


#ifdef MPC_MPI_USE_LOCAL_REQUESTS_QUEUE
#define MPC_MPI_LOCAL_REQUESTS_QUEUE_SIZE 10

inline MPI_internal_request_t * __sctk_new_mpc_request_internal_local_get (MPI_Request * req,
                                 MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp = NULL;

        MPI_per_thread_ctx_t *tctx = MPI_per_thread_ctx_get();

	tmp = tctx->mpc_mpi_local_request_queue;
	
	if(tmp != NULL){
		if(tmp->task_req_id == requests) {
			tctx->mpc_mpi_local_request_queue = tmp->next;
			tctx->mpc_mpi_local_request_queue_nb_req --;
		} else {
			tmp = NULL;
		}
	}

        return tmp;
}

inline int sctk_delete_internal_request_local_put(MPI_internal_request_t *tmp, MPI_request_struct_t *requests )
{
        MPI_per_thread_ctx_t *tctx = MPI_per_thread_ctx_get();

	if(tctx->mpc_mpi_local_request_queue_nb_req < MPC_MPI_LOCAL_REQUESTS_QUEUE_SIZE){

		sctk_delete_internal_request_clean(tmp);

		tmp->next = tctx->mpc_mpi_local_request_queue;
		tctx->mpc_mpi_local_request_queue = tmp;

		tctx->mpc_mpi_local_request_queue_nb_req ++;

		return 1;
	} else {
		return 0;
	}
}
#else
#define __sctk_new_mpc_request_internal_local_get(a,b) NULL
#define sctk_delete_internal_request_local_put(a,b) 0
#endif


/** \brief Delete an internal request and put it in the free-list */
inline void sctk_delete_internal_request(MPI_internal_request_t *tmp, MPI_request_struct_t *requests )
{
	sctk_delete_internal_request_clean(tmp);
	
	/* Push in in the head of the free list */
	tmp->next = requests->free_list;
	requests->free_list = tmp;
}

/** \brief Walk the auto-free list in search for a terminated call (only head) 
 * 
 *  \param requests a pointer to the request array structure 
 * */
inline void sctk_check_auto_free_list(MPI_request_struct_t *requests)
{
	/* If there is an auto free list */
	if(requests->auto_free_list != NULL)
	{
		MPI_internal_request_t *tmp;
		int flag;
		int res;

		/* Take HEAD */
		tmp = (MPI_internal_request_t *)requests->auto_free_list;
		
		/* Test it */
		res = PMPC_Test (&(tmp->req), &flag, MPC_STATUS_IGNORE);
		
		/* If call has ended */
		if (flag != 0)
		{
			MPI_internal_request_t *tmp_new;

			/* Remove HEAD from the auto-free list */
			tmp_new = (MPI_internal_request_t *)tmp->next;
			requests->auto_free_list = tmp_new;
			
			/* Free the request and put it in the free list */
			sctk_delete_internal_request(tmp,requests);
		}
	}
}

/** \brief Initialize an \ref MPI_internal_request_t */
inline void __sctk_init_mpc_request_internal(MPI_internal_request_t *tmp){
	memset (&(tmp->req), 0, sizeof (MPC_Request));
}

/** \brief Create a new \ref MPI_internal_request_t 
 * 
 *  \param req Request to allocate (will be written with the ID of the request)
 * */
/* extern inline */
MPI_internal_request_t * 
__sctk_new_mpc_request_internal (MPI_Request * req, 
				 MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	tmp = __sctk_new_mpc_request_internal_local_get(req,requests);
	
	if(tmp == NULL){
		/* Lock the request struct */
		sctk_spinlock_lock (&(requests->lock));
	
		/* Try to free the HEAD of the auto free list */
		sctk_check_auto_free_list(requests);
	
		/* If the free list is empty */
		if (requests->free_list == NULL)
		{
			int old_size;
			int i;
			
			/* Previous size */
			old_size = requests->max_size;
			/* New incremented size */
			requests->max_size += 10;
			/* Allocate TAB */
			requests->tab = sctk_realloc (requests->tab,  requests->max_size * sizeof (MPI_internal_request_t *));
		
			assume (requests->tab != NULL);
		
			/* Fill in the new array slots */
			for (i = old_size; i < requests->max_size; i++)
			{
				MPI_internal_request_t *tmp;
				sctk_nodebug ("%lu", i);
				/* Allocate the MPI_internal_request_t */
				tmp = sctk_buffered_malloc (&(requests->buf),  sizeof (MPI_internal_request_t));
				assume (tmp != NULL);
				
				/* Save it in the array */
				requests->tab[i] = tmp;
				
				/* Set not used */
				tmp->used = 0;
				tmp->lock = 0;
				tmp->task_req_id = requests;
				
				/* Put the newly allocated slot in the free list */
				tmp->next = requests->free_list;
				requests->free_list = tmp;
				
				/* Register its offset in the tab array */
				tmp->rank = i;
				tmp->saved_datatype = NULL;
			}
		}
	
		/* Now we should have a request in the free list */
		
		/* Take head from the free list */
		tmp = (MPI_internal_request_t *) requests->free_list;

		/* Remove from free list */
		requests->free_list = tmp->next;

		sctk_spinlock_unlock (&(requests->lock));
	}

        /* Mark it as used */
        tmp->used = 1;
        /* Mark it as freable */
        tmp->freeable = 1;
        /* Mark it as active */
        tmp->is_active = 1;
        /* Disable auto-free */
        tmp->auto_free = 0;
        tmp->saved_datatype = NULL;

	/* Mark it as not an nbc */
	tmp->is_nbc = 0;



	__sctk_convert_mpc_request_internal_cache_register(tmp);
	
	/* Set request to be the id in the tab array (rank) */
	*req = tmp->rank;

	/* Intialize request content */
	__sctk_init_mpc_request_internal(tmp);

	return tmp;
}

/** \brief Create a new \ref MPC_Request */
/* extern inline */
MPC_Request * __sctk_new_mpc_request (MPI_Request * req,MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;
	/* Acquire a free MPI_Iternal request */
	tmp = __sctk_new_mpc_request_internal (req,requests);

        /* Clear request */
        memset(&tmp->req, 0, sizeof(sctk_request_t));

        /* Return its inner MPC_Request */
        return &(tmp->req);
}

/** \brief Convert an \ref MPI_Request to an \ref MPI_internal_request_t
 * 
 * 	\param req Request to convert to an \ref MPI_internal_request_t
 *  \return a pointer to the \MPI_internal_request_t associated with req NULL if not found
 *  */
MPI_internal_request_t *
__sctk_convert_mpc_request_internal(MPI_Request *req,
                                    MPI_request_struct_t *requests) {
  MPI_internal_request_t *tmp;

  /* Retrieve the interger ID of this request */
  int int_req = *req;

  /* If it is request NULL we have nothing to get */
  if (int_req == MPI_REQUEST_NULL) {
    return NULL;
  }

  /* Retrieve the request array */
  assume(requests != NULL);

  /* Check bounds */
  assume(((int_req) >= 0) && ((int_req) < requests->max_size));

  tmp = __sctk_convert_mpc_request_internal_cache_get(req, requests);
  if (tmp == NULL) {
    /* Lock it */
    sctk_spinlock_lock(&(requests->lock));

    sctk_nodebug("Convert request %d", *req);

    /* Directly get the request in the tab */
    tmp = requests->tab[int_req];

    /* Unlock the request array */
    sctk_spinlock_unlock(&(requests->lock));

    __sctk_convert_mpc_request_internal_cache_register(tmp);
  }

  assume(tmp->task_req_id == requests);

  /* Is rank coherent with the offset */
  assume(tmp->rank == *req);

  /* Is this request in used */
  assume(tmp->used);

  /* Return the MPI_internal_request_t * */
  return tmp;
}

/** \brief Convert an MPI_Request to an MPC_Request
 * \param req Request to convert to an \ref MPC_Request
 * \return a pointer to the MPC_Request NULL if not found
 */
inline MPC_Request * __sctk_convert_mpc_request (MPI_Request * req,MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	/* Resolve the MPI_internal_request_t */
	tmp = __sctk_convert_mpc_request_internal (req,requests);
	
	/* If there was no MPI_internal_request_t or it was unactive */
	if ((tmp == NULL) || (tmp->is_active == 0))
	{
		/* Return the null request */
		return &MPC_REQUEST_NULL;
	}
	
	/* Return the MPC_Request field */
	return &(tmp->req);
}

/** Brief save a data-type in a request 
 * 
 *  \param req Target request
 *  \param t Datatype to store
 * 
 * */
inline void __sctk_add_in_mpc_request (MPI_Request * req, void *t,MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;
	tmp = __sctk_convert_mpc_request_internal (req,requests);
	tmp->saved_datatype = t;
}

/** Delete a request
 *  \param req Request to delete
 */
inline void __sctk_delete_mpc_request( MPI_Request *req,
									   MPI_request_struct_t *requests )
{
	MPI_internal_request_t *tmp;

	/* Convert resquest to an integer */
	int int_req = *req;

	/* If it is request null there is nothing to do */
	if ( int_req == MPI_REQUEST_NULL )
	{
		return;
	}

	assume( requests != NULL );
	/* Lock it */
	sctk_nodebug( "Delete request %d", *req );

	/* Retrieve the request */
	tmp = __sctk_convert_mpc_request_internal( req, requests );

	memset( &tmp->req, 0, sizeof( sctk_request_t ) );

	/* Clear the request */
	sctk_spinlock_lock( &( tmp->lock ) );

	/* if request is not active disable auto-free */
	if ( tmp->is_active == 0 )
	{
		tmp->auto_free = 0;
	}

	/* Deactivate the request */
	tmp->is_active = 0;

	/* Can the request be freed ? */
	if ( tmp->freeable == 1 )
	{
		/* Make sure the rank matches the TAB offset */
		assume( tmp->rank == *req );

		/* Auto free ? */
		if ( tmp->auto_free == 0 )
		{
			/* Call delete internal request to push it in the free list */
			if ( sctk_delete_internal_request_local_put( tmp, requests ) == 0 )
			{
				sctk_spinlock_lock( &( requests->lock ) );
				sctk_delete_internal_request( tmp, requests );
				sctk_spinlock_unlock( &( requests->lock ) );
			}
			/* Set the source request to NULL */
			*req = MPI_REQUEST_NULL;
		}
		else
		{
			/* Remove it from the free list */
			sctk_spinlock_lock( &( requests->lock ) );
			tmp->next = requests->auto_free_list;
			requests->auto_free_list = tmp;
			sctk_spinlock_unlock( &( requests->lock ) );
			/* Set the source request to NULL */
			*req = MPI_REQUEST_NULL;
		}
	}
	sctk_spinlock_unlock( &( tmp->lock ) );
}

/************************************************************************/
/* Halo storage Handling                                                */
/************************************************************************/

static volatile int __sctk_halo_initialized = 0;
sctk_spinlock_t __sctk_halo_initialized_lock = 0;
static struct sctk_mpi_halo_context __sctk_halo_context;

/** \brief Halo Context getter for MPI */
static inline struct sctk_mpi_halo_context * sctk_halo_context_get()
{
	return &__sctk_halo_context;
}

/** \brief Function called in \ref MPI_Init Initialized halo context once
 */
static inline void __sctk_init_mpc_halo ()
{
	sctk_spinlock_lock( &__sctk_halo_initialized_lock );
	
	if( __sctk_halo_initialized )
	{
		sctk_spinlock_unlock( &__sctk_halo_initialized_lock );
		return;
	}
	
	sctk_mpi_halo_context_init( sctk_halo_context_get() );
	
	__sctk_halo_initialized = 1;
	
	sctk_spinlock_unlock( &__sctk_halo_initialized_lock );
}





static int
__INTERNAL__PMPI_Send (void *buf, int count, MPI_Datatype datatype, int dest,
		       int tag, MPI_Comm comm)
{
        sctk_nodebug ("SEND buf %p type %d tag %d", buf, datatype,tag);
	if (sctk_datatype_is_derived (datatype) && (count != 0))
	{
		int res;

		sctk_nodebug ("Send derived type");

		if (count > 1)
		{
			sctk_nodebug("count > 1");
			MPI_Datatype new_datatype;
			res =
			__INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
			if (res != MPI_SUCCESS)
			{
			return res;
			}
			res = __INTERNAL__PMPI_Type_commit (&new_datatype);
			if (res != MPI_SUCCESS)
			{
			return res;
			}
			res = __INTERNAL__PMPI_Send (buf, 1, new_datatype, dest, tag, comm);
			if (res != MPI_SUCCESS)
			{
			return res;
			}
			res = __INTERNAL__PMPI_Type_free (&new_datatype);
			return res;
		}
		else
		{
			MPC_Request request;
			MPC_Status status;

			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);

			if (res != MPI_SUCCESS)
			{
			return res;
			}
			res = PMPC_Open_pack (&request);
			if (res != MPI_SUCCESS)
			{
			return res;
			}

			res = PMPC_Add_pack_absolute (buf, derived_datatype.count, derived_datatype.begins, derived_datatype.ends, MPC_CHAR, &request);
			if (res != MPI_SUCCESS)
			{
			return res;
			}

			res = PMPC_Isend_pack (dest, tag, comm, &request);
			if (res != MPI_SUCCESS)
			{
			return res;
			}
			
			res = PMPC_Wait (&request, &status);
			return res;
		}
	}
	else
	{
		sctk_nodebug ("Send contiguous type");
		if (buf == NULL && count != 0)
		{
                  MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "");
                }

                if (count == 0) {
                  /* code to avoid derived datatype */
                  datatype = MPC_CHAR;
                }

                return PMPC_Send(buf, count, datatype, dest, tag, comm);
        }
}

static int __INTERNAL__PMPIX_Exchange(void **send_buf , void **recvbuff, int remote_rank, MPI_Count size , MPI_Comm comm)
{
	int res =  MPI_ERR_INTERN;
	
	/* First resolve the source and dest rank
	 * in the comm_world */
	int remote = sctk_get_comm_world_rank ((const sctk_communicator_t) comm, remote_rank);
	
	/* Now check if we are on the same node for both communications */
	if( !sctk_is_net_message( remote ) )
	{
		/* Perform the zero-copy message */
		
		/* Exchange pointers */
		res = __INTERNAL__PMPI_Sendrecv (  send_buf, sizeof( void *), MPI_BYTE, remote_rank, 58740,
						   recvbuff, sizeof( void *),  MPI_BYTE, remote_rank, 58740,
						   comm, MPI_STATUS_IGNORE);
		
		if( res != MPI_SUCCESS )
			return res;
		
		sctk_debug("SWAPPING EX!");
	}
	else
	{
		/* Fallback to sendrecv */
		/* We have to do a standard sendrecv */
		res = __INTERNAL__PMPI_Sendrecv ( *send_buf, size, MPI_BYTE, remote_rank, 58740,
						       *recvbuff, size,  MPI_BYTE, remote_rank, 58740,
						       comm, MPI_STATUS_IGNORE);
		
		sctk_debug("COPYING EX!");
		
		if( res != MPI_SUCCESS )
			return res;
	}
	
	return MPI_SUCCESS;
}



static int __INTERNAL__PMPIX_Swap(void **sendrecv_buf , int remote_rank, MPI_Count size , MPI_Comm comm)
{
	int res =  MPI_ERR_INTERN;
	
	/* First resolve the source and dest rank
	 * in the comm_world */
	int remote = sctk_get_comm_world_rank ((const sctk_communicator_t) comm, remote_rank);
	
	/* Now check if we are on the same node for both communications */
	if( !sctk_is_net_message( remote ) )
	{
		/* Perform the zero-copy message */
		
		void * tmp_ptr = NULL;
		
		/* Exchange pointers */
		res = __INTERNAL__PMPI_Sendrecv (  sendrecv_buf, sizeof( void *), MPI_BYTE, remote_rank, 58740,
						   &tmp_ptr, sizeof( void *),  MPI_BYTE, remote_rank, 58740,
						   comm, MPI_STATUS_IGNORE);
		
		if( res != MPI_SUCCESS )
			return res;
		
		sctk_debug("SWAPPING ");
		
		/* Replace by the remote ptr */
		*sendrecv_buf = tmp_ptr;
	}
	else
	{
		/* Allocate a temporary buffer for the copy */
		void *tmp_buff = sctk_malloc( size );
		
		assume( tmp_buff != NULL );
		
		/* We have to do a standard sendrecv */
		res = __INTERNAL__PMPI_Sendrecv ( *sendrecv_buf, size, MPI_BYTE, remote_rank, 58740,
						       tmp_buff, size,  MPI_BYTE, remote_rank, 58740,
						       comm, MPI_STATUS_IGNORE);
		
		sctk_debug("COPYING !");
		
		if( res != MPI_SUCCESS )
			return res;
		
		/* Copy from the source buffer to the target one */
		memcpy(*sendrecv_buf, tmp_buff, size );
		
		sctk_free( tmp_buff );
		
	}
	
	return MPI_SUCCESS;
}


static int
__INTERNAL__PMPI_Recv (void *buf, int count, MPI_Datatype datatype,
		       int source, int tag, MPI_Comm comm,
		       MPI_Status * status)
{
	sctk_nodebug ("RECV buf %p", buf);
	if (sctk_datatype_is_derived (datatype))
	{
		int res;

		if (count > 1)
		{
			MPI_Datatype new_datatype;
			res =
			__INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = __INTERNAL__PMPI_Type_commit (&new_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res =
			__INTERNAL__PMPI_Recv (buf, 1, new_datatype, source, tag, comm,
			status);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = __INTERNAL__PMPI_Type_free (&new_datatype);
			return res;
		}
		else
		{
			MPC_Request request;
                        memset(&request, 0, sizeof(sctk_request_t));

                        int derived_ret = 0;
                        sctk_derived_datatype_t derived_datatype;

                        res = MPC_Is_derived_datatype(datatype, &derived_ret,
                                                      &derived_datatype);
                        if (res != MPI_SUCCESS) {
                          return res;
                        }
                        res = PMPC_Open_pack(&request);
                        if (res != MPI_SUCCESS) {
                          return res;
                        }

                        res = PMPC_Add_pack_absolute(
                            buf, derived_datatype.count,
                            derived_datatype.begins, derived_datatype.ends,
                            MPC_CHAR, &request);
                        if (res != MPI_SUCCESS) {
                          return res;
                        }

                        res = PMPC_Irecv_pack(source, tag, comm, &request);
                        if (res != MPI_SUCCESS) {
                          return res;
                        }
                        res = PMPC_Wait(&request, status);
                        return res;
                }
        } else {
          sctk_nodebug("Recv contiguous type");
          if (buf == NULL && count != 0) {
            MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "");
          }
          return PMPC_Recv(buf, count, datatype, source, tag, comm, status);
        }
}


static int __INTERNAL__PMPI_Get_count (MPI_Status * status, MPI_Datatype datatype, int *count)
{
	return  PMPC_Get_count(status, datatype, count);
}

static int
__INTERNAL__PMPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  MPI_Request request;
  int res;
  res = __INTERNAL__PMPI_Ibsend (buf, count, datatype, dest, tag, comm, &request);
  if(res != MPI_SUCCESS){
    return res;
  }
  res = __INTERNAL__PMPI_Wait(&request, MPI_STATUS_IGNORE);
  return res;
}


static int
__INTERNAL__PMPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  if (sctk_datatype_is_derived (datatype) && (count != 0))
    {
      return __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
    }
  else
    {
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
	if(count == 0)
	{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
	}

      return PMPC_Ssend (buf, count, datatype, dest, tag, comm);
    }
}


static int
__INTERNAL__PMPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  if (sctk_datatype_is_derived (datatype) && (count != 0))
    {
      return __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
    }
  else
    {
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
	if(count == 0)
	{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
	}
      return PMPC_Rsend (buf, count, datatype, dest, tag, comm);
    }
}

/*
  BUFFERS
*/
typedef struct MPI_buffer_struct_s
{
  void *buffer;
  int size;
  sctk_spinlock_t lock;
} mpi_buffer_t;

static inline void
__sctk_init_mpi_buffer ()
{
  mpi_buffer_t *tmp;
  tmp = sctk_malloc (sizeof (mpi_buffer_t));
  tmp->size = 0;
  tmp->buffer = NULL;
  tmp->lock = 0;
  PMPC_Set_buffers (tmp);
}

static inline mpi_buffer_overhead_t *
SCTK__buffer_next_header (mpi_buffer_overhead_t * head, mpi_buffer_t * tmp)
{
  mpi_buffer_overhead_t *head_next;

  head_next =
    (mpi_buffer_overhead_t *) ((char *) head +
			       (sizeof (mpi_buffer_overhead_t) + head->size));
  if ((unsigned long) head_next >= (unsigned long) tmp->buffer + tmp->size)
    {
      head_next = NULL;
    }

  return head_next;
}

static inline mpi_buffer_overhead_t *
SCTK__MPI_Compact_buffer (int size, mpi_buffer_t * tmp)
{
	mpi_buffer_overhead_t *found = NULL;
	mpi_buffer_overhead_t *head;
	mpi_buffer_overhead_t *head_next;

	int flag;
	head = (mpi_buffer_overhead_t *) (tmp->buffer);
/* 	fprintf(stderr,"Min %p max %p\n",tmp->buffer , ((char*)tmp->buffer) + tmp->size); */

	while (head != NULL)
	{
/* 	  fprintf(stderr,"%p %d\n",head,head->request); */
		if (head->request != MPI_REQUEST_NULL)
		{
			__INTERNAL__PMPI_Test (&(head->request), &flag, MPI_STATUS_IGNORE);
			assume((head->request == MPI_REQUEST_NULL) || (flag == 0));
		}
		if (head->request == MPI_REQUEST_NULL)
		{
			sctk_nodebug ("1 : %p freed", head);
			head_next = SCTK__buffer_next_header (head, tmp);

			/*Compact from head */
			while (head_next != NULL)
			{
/* 	  fprintf(stderr,"NEXT %p %d\n",head_next,head_next->request); */
				if (head_next->request != MPI_REQUEST_NULL)
				{
					__INTERNAL__PMPI_Test (&(head_next->request), &flag, MPI_STATUS_IGNORE);
					assume((head->request == MPI_REQUEST_NULL) || (flag == 0));
				}
				if (head_next->request == MPI_REQUEST_NULL)
				{
					sctk_nodebug ("2 : %p freed", head_next);
					head->size = head->size + head_next->size + sizeof (mpi_buffer_overhead_t);
					sctk_nodebug("MERGE Create new buffer of size %d (%d + %d)", head->size, head_next->size, head->size - head_next->size - sizeof (mpi_buffer_overhead_t));
				}
				else
				{
					break;
				}

				head_next = SCTK__buffer_next_header (head, tmp);
			}
		}

		if ((head->request == MPI_REQUEST_NULL) && (head->size >= size) /* && (found == NULL) */)
		{
		  if(found != NULL){
		    if(head->size < found->size){
		      found = head;
		    }
		  } else {
		    found = head;
		  }
		} 

		head_next = SCTK__buffer_next_header (head, tmp);
		head = head_next;
	}
	return found;
}

static int
__INTERNAL__PMPI_Buffer_attach (void *buf, int count)
{
  mpi_buffer_t *tmp;
  mpi_buffer_overhead_t *head;
  PMPC_Get_buffers (&tmp);

  sctk_spinlock_lock (&(tmp->lock));
  if((tmp->buffer != NULL) || (count < 0)){
    sctk_spinlock_unlock (&(tmp->lock));
    return MPI_ERR_BUFFER;
  }
  tmp->buffer = buf;
  tmp->size = count;
  sctk_nodebug ("Buffer size %d", count);
  head = (mpi_buffer_overhead_t *) (tmp->buffer);

  head->size = tmp->size - sizeof (mpi_buffer_overhead_t);
  head->request = MPI_REQUEST_NULL;
  sctk_spinlock_unlock (&(tmp->lock));

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Buffer_detach (void *bufferptr, int *size)
{
  mpi_buffer_t *tmp;
  PMPC_Get_buffers (&tmp);
  sctk_spinlock_lock (&(tmp->lock));

  /* Flush pending messages */
  {
    mpi_buffer_overhead_t *head;
    mpi_buffer_overhead_t *head_next;
    int flag;
    int pending;
    do
      {
	pending = 0;
	head = (mpi_buffer_overhead_t *) (tmp->buffer);

	while (head != NULL)
	  {
	    if (head->request != MPI_REQUEST_NULL)
	      {
		__INTERNAL__PMPI_Test (&(head->request), &flag,
				       MPI_STATUS_IGNORE);
	      }
	    if (head->request != MPI_REQUEST_NULL)
	      {
		pending++;
	      }
	    head_next = SCTK__buffer_next_header (head, tmp);
	    head = head_next;
	  }
	if (pending != 0) sctk_thread_yield ();
      }
    while (pending != 0);
  }

  *((void **) bufferptr) = tmp->buffer;
  *size = tmp->size;
  tmp->size = 0;
  tmp->buffer = NULL;
  sctk_spinlock_unlock (&(tmp->lock));
  
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Ibsend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request,
				  struct MPI_request_struct_s* requests)
{
	mpi_buffer_t *tmp;
	int size;
	int res;
	mpi_buffer_overhead_t *head;
	mpi_buffer_overhead_t *head_next;
	void *head_buf;
	mpi_buffer_overhead_t *found = NULL;
	//~ get the pack size

	res = __INTERNAL__PMPI_Pack_size (count, datatype, comm, &size);
	
	if (res != MPI_SUCCESS)
	{
		return res;
	}

	if (size % sizeof (mpi_buffer_overhead_t))
	{
		size +=
		sizeof (mpi_buffer_overhead_t) -
		(size % sizeof (mpi_buffer_overhead_t));
	}
	assume (size % sizeof (mpi_buffer_overhead_t) == 0);

	sctk_nodebug ("MSG size %d", size);

	PMPC_Get_buffers (&tmp);
	sctk_spinlock_lock_yield (&(tmp->lock));

	if (tmp->buffer == NULL)
	{
		sctk_spinlock_unlock (&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "No buffer available");
	}

	found = SCTK__MPI_Compact_buffer (size, tmp);

	sctk_nodebug("found = %d", found);
	
	if (found)
	{
		int position = 0;
		head = found;
		head_buf = (char *) head + sizeof (mpi_buffer_overhead_t);

		assume (head->request == MPI_REQUEST_NULL);

		if (head->size >= size + (int)sizeof (mpi_buffer_overhead_t))
		{
			int old_size;
			old_size = head->size;
			head->size = size;
			head_next = SCTK__buffer_next_header (head, tmp);
			head_next->size = old_size - size - sizeof (mpi_buffer_overhead_t);
			head_next->request = MPI_REQUEST_NULL;
			sctk_nodebug ("SPLIT Create new buffer of size %d old %d",
			head_next->size, old_size);
		}

		sctk_nodebug ("CHUNK %p<%p-%p<%p", tmp->buffer, head,
		SCTK__buffer_next_header (head, tmp),
		(void *) ((unsigned long) tmp->buffer + tmp->size));

		res =
		__INTERNAL__PMPI_Pack (buf, count, datatype, head_buf, head->size,
		&position, comm);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(tmp->lock));
			return res;
		}
		
		assume (position <= size);


		res = __INTERNAL__PMPI_Isend_test_req (head_buf, position, MPI_PACKED, dest, tag, comm, &(head->request), 0,requests);

		/*       fprintf(stderr,"Add request %d %d\n",head->request,res); */

		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(tmp->lock));
			return res;
		}

		if(is_valid_request == 1){
			MPI_internal_request_t* tmp_request;

			tmp_request = __sctk_convert_mpc_request_internal (request,requests);
			tmp_request->req.completion_flag = SCTK_MESSAGE_DONE;

		} else {
			//	*request = MPI_REQUEST_NULL;
			MPI_internal_request_t* tmp_request;
			__sctk_new_mpc_request (request,requests);
			tmp_request = __sctk_convert_mpc_request_internal (request,requests);
			tmp_request->req.completion_flag = SCTK_MESSAGE_DONE;
		}


	}
	else
	{
        sctk_spinlock_unlock (&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "No space left in buffer");
	}

	sctk_spinlock_unlock (&(tmp->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Ibsend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Ibsend_test_req (buf, count, datatype, dest, tag,
					   comm, request, 0,
					   __sctk_internal_get_MPC_requests());
}

static int
__INTERNAL__PMPI_Isend_test_req (void *buf, int count, MPI_Datatype datatype,
				 int dest, int tag, MPI_Comm comm,
				 MPI_Request * request, int is_valid_request,
				 MPI_request_struct_t *requests)
{
	if (sctk_datatype_is_derived (datatype) && (count != 0))
	{
		int res;

		if (count > 1)
		{
			MPI_Datatype new_datatype;
			res =
			__INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = __INTERNAL__PMPI_Type_commit (&new_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res =
			__INTERNAL__PMPI_Isend_test_req (buf, 1, new_datatype, dest, tag,
							 comm, request, is_valid_request,requests);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = __INTERNAL__PMPI_Type_free (&new_datatype);
			return res;
		}
		else
		{
			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			if (is_valid_request)
			{
			  res = PMPC_Open_pack (__sctk_convert_mpc_request (request,requests));
			}
			else
			{
			  res = PMPC_Open_pack (__sctk_new_mpc_request (request,requests));
			}
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			{
				mpc_pack_absolute_indexes_t *tmp;
				tmp = sctk_malloc (derived_datatype.opt_count * 2 * sizeof (mpc_pack_absolute_indexes_t));
				__sctk_add_in_mpc_request (request, tmp,requests);
				
				memcpy (tmp, derived_datatype.opt_begins, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));
				memcpy (&(tmp[derived_datatype.opt_count]), derived_datatype.opt_ends, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));
				
				derived_datatype.opt_begins = tmp;
				derived_datatype.opt_ends = &(tmp[derived_datatype.opt_count]);
			}

			res = PMPC_Add_pack_absolute (buf, derived_datatype.opt_count, derived_datatype.opt_begins, derived_datatype.opt_ends, MPC_CHAR,	__sctk_convert_mpc_request (request,requests));
		
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			res = PMPC_Isend_pack (dest, tag, comm, __sctk_convert_mpc_request (request,requests));
			return res;
		}
	}
	else
	{
		if(count == 0)
		{
			/* code to avoid derived datatype */
			datatype = MPC_CHAR;
		}
		if (is_valid_request)
		{
			return PMPC_Isend (buf, count, datatype, dest, tag, comm,
					   __sctk_convert_mpc_request (request,requests));
		}
		else
		{
			return PMPC_Isend (buf, count, datatype, dest, tag, comm,
					   __sctk_new_mpc_request (request,requests));
		}
	}
}

static int
__INTERNAL__PMPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm, MPI_Request * request)
{
  return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					  comm, request, 0,__sctk_internal_get_MPC_requests());
}

static int
__INTERNAL__PMPI_Issend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request,
				  MPI_request_struct_t *requests)
{
  if (sctk_datatype_is_derived (datatype) && (count != 0))
    {
      return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					      comm, request,
					      is_valid_request,requests);
    }
  else
    {
	if(count == 0)
	{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
	}
      if (is_valid_request)
	{
	  return PMPC_Issend (buf, count, datatype, dest, tag, comm,
			     __sctk_convert_mpc_request (request,requests));
	}
      else
	{
	  return PMPC_Issend (buf, count, datatype, dest, tag, comm,
			     __sctk_new_mpc_request (request,requests));
	}
    }
}
static int
__INTERNAL__PMPI_Issend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Issend_test_req (buf, count, datatype, dest, tag,
					   comm, request, 0,__sctk_internal_get_MPC_requests());
}
static int
__INTERNAL__PMPI_Irsend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request,
				  MPI_request_struct_t *requests)
{
  if (sctk_datatype_is_derived (datatype))
    {
      return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					      comm, request,
					      is_valid_request,requests);
    }
  else
    {
      if (is_valid_request)
	{
	  return PMPC_Irsend (buf, count, datatype, dest, tag, comm,
			      __sctk_convert_mpc_request (request,requests));
	}
      else
	{
	  return PMPC_Irsend (buf, count, datatype, dest, tag, comm,
			      __sctk_new_mpc_request (request,requests));
	}
    }
}

static int __INTERNAL__PMPI_Irsend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Irsend_test_req (buf, count, datatype, dest, tag, comm, request, 0,
					   __sctk_internal_get_MPC_requests());
}


static int __INTERNAL__PMPI_Irecv_test_req (void *buf, int count, MPI_Datatype datatype,
				 int source, int tag, MPI_Comm comm,
					    MPI_Request * request, int is_valid_request,
					    struct MPI_request_struct_s * requests)
{
	if (sctk_datatype_is_derived (datatype))
	{
		int res;

		if (count > 1)
		{
			MPI_Datatype new_datatype;
			res =
			__INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = __INTERNAL__PMPI_Type_commit (&new_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res =
			__INTERNAL__PMPI_Irecv_test_req (buf, 1, new_datatype, source,
			tag, comm, request,
							 is_valid_request,requests);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = __INTERNAL__PMPI_Type_free (&new_datatype);
			return res;
		}
		else
		{
			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			
			if (is_valid_request)
			{
			  res = PMPC_Open_pack (__sctk_convert_mpc_request (request,requests));
			}
			else
			{
			  res = PMPC_Open_pack (__sctk_new_mpc_request (request,requests));
			}
			
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			{
				mpc_pack_absolute_indexes_t *tmp;
				tmp = sctk_malloc (derived_datatype.opt_count * 2 * sizeof (mpc_pack_absolute_indexes_t));
				__sctk_add_in_mpc_request (request, tmp,requests);

				memcpy (tmp, derived_datatype.opt_begins, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));
				memcpy (&(tmp[derived_datatype.opt_count]), derived_datatype.opt_ends, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));

				derived_datatype.opt_begins = tmp;
				derived_datatype.opt_ends = &(tmp[derived_datatype.opt_count]);
			}

			res =
			  PMPC_Add_pack_absolute (buf, derived_datatype.opt_count, derived_datatype.opt_begins, derived_datatype.opt_ends, MPC_CHAR, __sctk_convert_mpc_request (request,requests));
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			res =
			PMPC_Irecv_pack (source, tag, comm,
					 __sctk_convert_mpc_request (request,requests));
			return res;
		}
	}
	else
	{
		if (is_valid_request)
		{
			return PMPC_Irecv (buf, count, datatype, source, tag, comm,
					   __sctk_convert_mpc_request (request,requests));
		}
		else
		{
			return PMPC_Irecv (buf, count, datatype, source, tag, comm,
					   __sctk_new_mpc_request (request,requests));
		}
	}
}

int __INTERNAL__PMPI_Irecv (void *buf, int count, MPI_Datatype datatype,
			int source, int tag, MPI_Comm comm,
			MPI_Request * request)
{
  return __INTERNAL__PMPI_Irecv_test_req (buf, count, datatype, source, tag,
					  comm, request, 0,__sctk_internal_get_MPC_requests());
}

static int __INTERNAL__PMPI_Wait (MPI_Request * request, MPI_Status * status)
{
	int res = MPI_SUCCESS;
	MPI_request_struct_t *requests;
	MPI_internal_request_t *tmp;
	sctk_nodebug("wait request %d", *request);

	requests = __sctk_internal_get_MPC_requests();
	tmp = __sctk_convert_mpc_request_internal(request,requests);
	
	if((tmp != NULL ) && (tmp->is_nbc == 1 ))
	{
		res =  NBC_Wait(&(tmp->nbc_handle), status);
	}
	else
	{
		MPC_Request * mpcreq = __sctk_convert_mpc_request (request,requests);

		if( mpcreq->request_type == REQUEST_GENERALIZED )
		{
			res = PMPC_Waitall(1, mpcreq, status); 
		}
		else
		{
			res = PMPC_Wait ( mpcreq , status);
		}
	}
	__sctk_delete_mpc_request (request,requests);
	
	return res;
}

static int __INTERNAL__PMPI_Test (MPI_Request * request, int *flag, MPI_Status * status)
{
  int res = -1;
  MPI_internal_request_t *tmp;
  MPI_request_struct_t *requests;
  requests = __sctk_internal_get_MPC_requests();
  tmp = __sctk_convert_mpc_request_internal(request, requests);

  if ((tmp != NULL) && (tmp->is_nbc == 1)) {
    res = NBC_Test(&(tmp->nbc_handle), flag, status);
  } else {
    res =
        PMPC_Test(__sctk_convert_mpc_request (request,requests), flag, status);
  }

  if (*flag) {
    __sctk_delete_mpc_request(request, requests);
  } else {
    mpc_thread_yield();
    }

    return res;
}

static int __INTERNAL__PMPI_Request_free (MPI_Request * request)
{
	int res = MPI_SUCCESS;
	MPI_internal_request_t *tmp;
	MPI_request_struct_t *requests;
	requests = __sctk_internal_get_MPC_requests();
	tmp = __sctk_convert_mpc_request_internal (request,requests);

	if (tmp)
    {
		tmp->freeable = 1;
		tmp->auto_free = 1;
		__sctk_delete_mpc_request (request,requests);
		*request = MPI_REQUEST_NULL;
    }
	return res;
}

int __INTERNAL__PMPI_Waitany (int count, MPI_Request * array_of_requests, int *index, MPI_Status * status)
{
	int flag = 0;
	int ret = MPI_SUCCESS;

	while (!flag)
    {
		ret = __INTERNAL__PMPI_Testany (count, array_of_requests, index, &flag, status);
	
    }
    
	return ret;
}

static int __INTERNAL__PMPI_Testany (int count, MPI_Request * array_of_requests, int *index, int *flag, MPI_Status * status)
{
  int i;
  *index = MPI_UNDEFINED;
  *flag = 1;
  int tmp;
  MPI_request_struct_t *requests;

  if (status != MPC_STATUSES_IGNORE) {
    status->MPI_ERROR = MPC_SUCCESS;
  }

  if (!array_of_requests) {
    return MPI_SUCCESS;
  }

  requests = __sctk_internal_get_MPC_requests();

  for (i = 0; i < count; i++) {
    if (array_of_requests[i] == MPI_REQUEST_NULL) {
      continue;
    }

    {
      MPC_Request *req;
      req = __sctk_convert_mpc_request(&(array_of_requests[i]), requests);
      if (req == &MPC_REQUEST_NULL) {
        continue;
      } else {
        MPI_internal_request_t *reqtmp;
        reqtmp = __sctk_convert_mpc_request_internal(&(array_of_requests[i]),
                                                     requests);
        int flag_test = 0;
        if ((reqtmp != NULL) && (reqtmp->is_nbc == 1)) {
          tmp = NBC_Test(&(reqtmp->nbc_handle), &flag_test, status);
        } else {

          tmp = PMPC_Test(req, &flag_test, status);
          if (flag_test == 0)
            *flag = 0;
          else
            *flag = 1;
        }
      }
    }

    if (tmp != MPI_SUCCESS) {
      return tmp;
    }
    if (*flag) {
      __sctk_delete_mpc_request(&(array_of_requests[i]), requests);
      *index = i;
      return tmp;
    }
  }

  return MPI_SUCCESS;
}

#define PMPI_WAIT_ALL_STATIC_TRSH 32

int __INTERNAL__PMPI_Waitall (int count, MPI_Request * array_of_requests, MPI_Status * array_of_statuses)
{
	int flag = 0;
	int i;
	MPI_request_struct_t *requests;
	
	requests = __sctk_internal_get_MPC_requests();

	/* Set the MPI_Status to MPI_SUCCESS */
	if(array_of_statuses != MPI_STATUSES_IGNORE){
	  for (i = 0; i < count; i++)
	    {
	      array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
	    }
	}
	
	/* Convert MPI resquests to MPC ones */
	
	/* Prepare an array for MPC requests */
	MPC_Request ** mpc_array_of_requests;
	MPC_Request * static_array_of_requests[PMPI_WAIT_ALL_STATIC_TRSH];
	
	if( count < PMPI_WAIT_ALL_STATIC_TRSH )
	{
		mpc_array_of_requests = static_array_of_requests;
	}
	else
	{
		mpc_array_of_requests = sctk_malloc(sizeof(MPC_Request *) * count);
		assume( mpc_array_of_requests != NULL );
	}

        /* Prepare an array for NBC requests */
        MPI_Request *mpc_array_of_requests_nbc;
        MPI_Request static_array_of_requests_nbc[PMPI_WAIT_ALL_STATIC_TRSH];

        if (count < PMPI_WAIT_ALL_STATIC_TRSH) {
          mpc_array_of_requests_nbc = static_array_of_requests_nbc;
        } else {
          mpc_array_of_requests_nbc = sctk_malloc(sizeof(MPI_Request) * count);
          assume(mpc_array_of_requests_nbc != NULL);
        }

        MPI_internal_request_t *tmp;

        int has_nbc = 0;

        /* Fill the array with those requests */
        for (i = 0; i < count; i++) {
          tmp = __sctk_convert_mpc_request_internal(&(array_of_requests[i]),
                                                    requests);

          if ((tmp != NULL) && (tmp->is_nbc == 1)) {
            has_nbc = 1;
            mpc_array_of_requests[i] = &MPC_REQUEST_NULL;
            mpc_array_of_requests_nbc[i] = array_of_requests[i];
          } else {
            mpc_array_of_requests_nbc[i] = MPI_REQUEST_NULL;

            /* Handle NULL requests */
            if (array_of_requests[i] == MPI_REQUEST_NULL) {
              mpc_array_of_requests[i] = &MPC_REQUEST_NULL;
            } else {
              mpc_array_of_requests[i] = __sctk_convert_mpc_request(&(array_of_requests[i]), requests);
            }
          }
        }

        if (has_nbc) {
          int nbc_flag = 0;

          while (!nbc_flag) {
            __INTERNAL__PMPI_Testall(count, mpc_array_of_requests_nbc,
                                     &nbc_flag, MPI_STATUSES_IGNORE);
          }
        }

        /* Call the MPC waitall implementation */
        int ret = __MPC_Waitallp(count, mpc_array_of_requests,
                                 (MPC_Status *)array_of_statuses);

        /* Something bad hapenned ? */
        if (ret != MPI_SUCCESS)
          return ret;

        /* Delete the MPI requests */
        for (i = 0; i < count; i++)
          __sctk_delete_mpc_request(&(array_of_requests[i]), requests);

        /* If needed free the mpc_array_of_requests */
        if (PMPI_WAIT_ALL_STATIC_TRSH <= count) {
          sctk_free(mpc_array_of_requests);
          sctk_free(mpc_array_of_requests_nbc);
        }

        return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Testall (int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])
{
    int i;
    int done = 0;
    int loc_flag;
    MPI_request_struct_t *requests;
    requests = __sctk_internal_get_MPC_requests();
    *flag = 0;
    if(array_of_statuses != MPC_STATUSES_IGNORE){
        for (i = 0; i < count; i++)
        {
            array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
        }
    }

    for (i = 0; i < count; i++)
    {
        int tmp;

        if (array_of_requests[i] == MPI_REQUEST_NULL)
        {
            done++;
            loc_flag = 0;
            tmp = MPI_SUCCESS;
        }
        else
        {
            MPI_internal_request_t *reqtmp;
            reqtmp = __sctk_convert_mpc_request_internal(
                        &(array_of_requests[i]), requests);
            MPC_Request *req;
            req = &reqtmp->req;

            if (req == &MPC_REQUEST_NULL) {
                done++;
                loc_flag = 0;
                tmp = MPI_SUCCESS;
            } else {
                if ((reqtmp != NULL) && (reqtmp->is_nbc == 1)) {
                    tmp = NBC_Test(
                            &(reqtmp->nbc_handle), &loc_flag,
                            (array_of_statuses == MPI_STATUSES_IGNORE)
                            ? MPI_STATUS_IGNORE
                            : &(array_of_statuses[i]));
                    if (loc_flag) {
                        array_of_requests[i] = MPI_REQUEST_NULL;
                    }
                } else {
                    tmp = PMPC_Test(
                            req, &loc_flag,
                            (array_of_statuses == MPC_STATUSES_IGNORE)
                            ? MPC_STATUS_IGNORE
                            : &(array_of_statuses[i]));
                }
            }
        }
        if (loc_flag) {
            done++;
        }
        if (tmp != MPI_SUCCESS) {
            return tmp;
        }
    }

    if (done == count) {
        for (i = 0; i < count; i++) {
            if (array_of_requests[i] != MPI_REQUEST_NULL) {
                __sctk_delete_mpc_request(&(array_of_requests[i]), requests);
            }
        }
    }
    sctk_nodebug("done %d tot %d", done, count);
    *flag = (done == count);
    if (*flag == 0) {
        sctk_thread_yield();
    }
    return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Waitsome (int incount, MPI_Request * array_of_requests, int *outcount, int *array_of_indices, MPI_Status * array_of_statuses)
{
  int i;
  int req_null_count = 0;
  MPI_request_struct_t *requests;
  int res;
  requests = __sctk_internal_get_MPC_requests();

  *outcount = MPI_UNDEFINED;

  for(i = 0; i < incount; i++){
    if(array_of_requests[i] == MPI_REQUEST_NULL){
      req_null_count++;
    } else {
      MPC_Request *req;
      req = __sctk_convert_mpc_request (&(array_of_requests[i]),requests);
      if (req == &MPC_REQUEST_NULL)
	{
	  req_null_count++;
	}
    }
  }

  if(req_null_count == incount){
    return MPI_SUCCESS;
  }

  do
    {
      //      sctk_thread_yield ();
      res = __INTERNAL__PMPI_Testsome(incount, array_of_requests, outcount,
                                      array_of_indices, array_of_statuses);
  } while ((res == MPI_SUCCESS) &&
           ((*outcount == MPI_UNDEFINED) || (*outcount == 0)));

  return res;
}

static int __INTERNAL__PMPI_Testsome (int incount, MPI_Request * array_of_requests, int *outcount, int *array_of_indices, MPI_Status * array_of_statuses)
{
	int i;
	int done = 0;
	int no_active_done = 0;
	int loc_flag;
	MPI_request_struct_t *requests;
	requests = __sctk_internal_get_MPC_requests();

	for (i = 0; i < incount; i++)
	{
	  if (array_of_requests[i] != MPI_REQUEST_NULL)
		{
			int tmp;
			MPC_Request *req;
			req = __sctk_convert_mpc_request (&(array_of_requests[i]),requests);
			if (req == &MPC_REQUEST_NULL)
			{
				loc_flag = 0;
				tmp = MPI_SUCCESS;
				no_active_done++;
			}
			else
			{
				tmp = PMPC_Test (req, &loc_flag,(array_of_statuses == MPI_STATUSES_IGNORE) ? MPC_STATUS_IGNORE:&(array_of_statuses[done]));
			}
			if (loc_flag)
			{
				array_of_indices[done] = i;
				__sctk_delete_mpc_request (&(array_of_requests[i]),requests);
				done++;
			}
			if (tmp != MPI_SUCCESS)
			{
				return tmp;
			}
		}
		else
		{
			no_active_done++;
		}
	}

        *outcount = done;

        if (no_active_done == incount) {
          *outcount = MPI_UNDEFINED;
          sctk_thread_yield();
        } else {
          sctk_thread_yield();
        }
        return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
			 MPI_Status * status)
{
  int res;
  res = PMPC_Iprobe (source, tag, comm, flag, status);
  if (!(*flag)) sctk_thread_yield ();
  return res;
}

static int
__INTERNAL__PMPI_Probe (int source, int tag, MPI_Comm comm,
			MPI_Status * status)
{
  return PMPC_Probe (source, tag, comm, status);
}

static int
__INTERNAL__PMPI_Cancel (MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  MPI_internal_request_t *req;
  MPI_request_struct_t *requests;
  requests = __sctk_internal_get_MPC_requests();

  req = __sctk_convert_mpc_request_internal (request,requests);
  if (req->is_active == 1)
	{
	  res = PMPC_Cancel (__sctk_convert_mpc_request (request,requests));
	}
  else
    {
      res = MPI_ERR_REQUEST;
    }
  return res;
}

static int __INTERNAL__PMPI_Test_cancelled (MPI_Status * status, int *flag)
{
	PMPC_Test_cancelled (status, flag);
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Send_init (void *buf, int count, MPI_Datatype datatype,
			    int dest, int tag, MPI_Comm comm,
			    MPI_Request * request)
{
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
	if(dest == MPC_PROC_NULL)
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Send_init;

	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Send_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Bsend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
	int rank;
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
	sctk_nodebug("new request %d", *request);
	if(dest == MPC_PROC_NULL)
	{
	  	req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Bsend_init;
	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Bsend_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Ssend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
	if(dest == MPC_PROC_NULL)
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Ssend_init;
	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Ssend_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Rsend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
	if(dest == MPC_PROC_NULL)
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Rsend_init;
	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Rsend_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Recv_init (void *buf, int count, MPI_Datatype datatype,
			    int source, int tag, MPI_Comm comm,
			    MPI_Request * request)
{
  MPI_internal_request_t *req;
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->req.request_type = REQUEST_RECV;

  req->persistant.buf = buf;
  req->persistant.count = count;
  req->persistant.datatype = datatype;
  req->persistant.dest_source = source;
  req->persistant.tag = tag;
  req->persistant.comm = comm;
  req->persistant.op = Recv_init;

  return MPI_SUCCESS;
}

static inline int
____INTERNAL__PMPI_Start (MPI_Request * request)
{
  int res = 0;
  MPI_internal_request_t *req;
  MPI_request_struct_t *requests;
  requests = __sctk_internal_get_MPC_requests();
  req = __sctk_convert_mpc_request_internal (request,requests);
  if(req->is_active != 0)
	return MPI_ERR_REQUEST;
  req->is_active = 1;

  if(req->req.request_type == REQUEST_NULL)
  {
    req->is_active = 0; 
	return MPI_SUCCESS;
  }

  switch (req->persistant.op)
    {
    case Send_init:
      res =
	__INTERNAL__PMPI_Isend_test_req (req->persistant.buf,
					 req->persistant.count,
					 req->persistant.datatype,
					 req->persistant.dest_source,
					 req->persistant.tag,
					 req->persistant.comm, request, 1,requests);
      break;
    case Bsend_init:
      res =
	__INTERNAL__PMPI_Ibsend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1,requests);
      break;
    case Rsend_init:
      res =
	__INTERNAL__PMPI_Irsend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1,requests);
      break;
    case Ssend_init:
      res =
	__INTERNAL__PMPI_Issend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1,requests);
      break;
    case Recv_init:
      res =
	__INTERNAL__PMPI_Irecv_test_req (req->persistant.buf,
					 req->persistant.count,
					 req->persistant.datatype,
					 req->persistant.dest_source,
					 req->persistant.tag,
					 req->persistant.comm, request, 1,requests);
      break;
    default:
      not_reachable ();
    }
  return res;
}

static int
__INTERNAL__PMPI_Start (MPI_Request * request)
{
  return ____INTERNAL__PMPI_Start (request);
}

static int
__INTERNAL__PMPI_Startall (int count, MPI_Request * array_of_requests)
{
  int i;
  for (i = 0; i < count; i++)
    {
      int res;
      res = ____INTERNAL__PMPI_Start (&(array_of_requests[i]));
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Sendrecv (void *sendbuf, int sendcount,
			   MPI_Datatype sendtype, int dest, int sendtag,
			   void *recvbuf, int recvcount,
			   MPI_Datatype recvtype, int source, int recvtag,
			   MPI_Comm comm, MPI_Status * status)
{
	int res;
	MPI_Request s_request;
	MPI_Request r_request;
	MPI_Status s_status;

	sctk_nodebug ("__INTERNAL__PMPI_Sendrecv TYPE %d %d", sendtype, recvtype);

	res = __INTERNAL__PMPI_Isend (sendbuf, sendcount, sendtype, dest, sendtag, comm, &s_request);
	
	if (res != MPI_SUCCESS)
	{
		return res;
	}
	res = __INTERNAL__PMPI_Irecv (recvbuf, recvcount, recvtype, source, recvtag, comm, &r_request);
	
	if (res != MPI_SUCCESS)
	{
		return res;
	}
	
	res = __INTERNAL__PMPI_Wait (&s_request, &s_status);
	
	if (res != MPI_SUCCESS)
	{
		return res;
	}
	
	res = __INTERNAL__PMPI_Wait (&r_request, status);

	sctk_nodebug ("__INTERNAL__PMPI_Sendrecv TYPE %d %d done", sendtype, recvtype);
	return res;
}

static int
__INTERNAL__PMPI_Sendrecv_replace (void *buf, int count,
				   MPI_Datatype datatype, int dest,
				   int sendtag, int source, int recvtag,
				   MPI_Comm comm, MPI_Status * status)
{
  int type_size;
  int res;
  char *tmp;
  MPI_Request sendreq;
  MPI_Request recvreq;
  int position = 0;

  sctk_nodebug ("__INTERNAL__PMPI_Sendrecv_replace TYPE %d", datatype);

  res = __INTERNAL__PMPI_Pack_size (count, datatype, comm, &type_size);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  tmp = sctk_malloc (type_size);
  if (tmp == NULL)
    {
      return MPI_ERR_INTERN;
    }

  res =
    __INTERNAL__PMPI_Pack (buf, count, datatype, tmp, type_size, &position,
			   comm);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  sctk_nodebug ("position %d, %d", position, type_size);
  sctk_nodebug("receive type = %d", datatype);
  res =
    __INTERNAL__PMPI_Irecv (buf, count, datatype, source, recvtag, comm,
			    &recvreq);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  sctk_nodebug("Send type = %d", MPI_PACKED);
  res =
    __INTERNAL__PMPI_Isend (tmp, position, MPI_PACKED, dest, sendtag, comm,
			    &sendreq);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  res = __INTERNAL__PMPI_Wait (&sendreq, status);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  res = __INTERNAL__PMPI_Wait (&recvreq, status);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }

  sctk_free (tmp);
  return MPI_SUCCESS;
}

/************************************************************************/
/* Generalized Requests                                                 */
/************************************************************************/

int PMPI_Grequest_start( MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function * free_fn,
					  MPI_Grequest_cancel_function * cancel_fn, void *extra_state, MPI_Request * request)
{
	MPC_Request *new_request = __sctk_new_mpc_request (request,__sctk_internal_get_MPC_requests());
	
	return PMPC_Grequest_start( query_fn, free_fn, cancel_fn, extra_state, new_request );
}


int PMPI_Grequest_complete(  MPI_Request request )
{
	MPC_Request *mpc_req = __sctk_convert_mpc_request (&request,__sctk_internal_get_MPC_requests());
	return PMPC_Grequest_complete( *mpc_req );
}

/************************************************************************/
/* Extended Generalized Requests                                        */
/************************************************************************/

  int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
                          MPI_Grequest_free_function * free_fn,
           		  MPI_Grequest_cancel_function * cancel_fn, 
           		  MPIX_Grequest_poll_fn * poll_fn, 
           		  void *extra_state, 
           		  MPI_Request * request)
  {
          MPC_Request *new_request = __sctk_new_mpc_request (request,__sctk_internal_get_MPC_requests());
	  return PMPCX_Grequest_start(query_fn, free_fn, cancel_fn, poll_fn, extra_state, new_request);
  }

/************************************************************************/
/* Extended Generalized Requests Class                                  */
/************************************************************************/

int PMPIX_Grequest_class_create( MPI_Grequest_query_function * query_fn,
				 MPI_Grequest_free_function * free_fn,
				 MPI_Grequest_cancel_function * cancel_fn,
				 MPIX_Grequest_poll_fn * poll_fn,
				 MPIX_Grequest_wait_fn * wait_fn,
				 MPIX_Grequest_class * new_class )
{
	return PMPCX_GRequest_class_create(query_fn, cancel_fn, free_fn, poll_fn, wait_fn, new_class );
}
  
int PMPIX_Grequest_class_allocate( MPIX_Grequest_class target_class, void *extra_state, MPI_Request *request )
{
        MPC_Request *new_request = __sctk_new_mpc_request (request,__sctk_internal_get_MPC_requests());
	return PMPCX_Grequest_class_allocate( target_class, extra_state, new_request );
}

/************************************************************************/
/* Datatype Handling                                                    */
/************************************************************************/

/** \brief This function creates a contiguous MPI_Datatype with possible CTX inheritance
 *  
 *  Such datatype is obtained by directly appending  count copies of data_in type
    HERE CONTEXT CAN BE OVERRIDED (for example to be a contiguous vector)
 *  
 *  \param count Number of elements of type data_in in the data_out type
 *  \param data_in The type to be replicated count times
 *  \param data_out The output type
 *  \param ctx THe context is is inherited from (in case another datatype is built-on top of it)
 * 
 */
static int __INTERNAL__PMPI_Type_contiguous_inherits (unsigned long count, MPI_Datatype data_in,  MPI_Datatype * data_out, struct Datatype_External_context *ref_dtctx)
{
/* Set its context */
	struct Datatype_External_context *dtctx = NULL;
	
	struct Datatype_External_context local_dtctx;
	sctk_datatype_external_context_clear( &local_dtctx );
	local_dtctx.combiner = MPI_COMBINER_CONTIGUOUS;
	local_dtctx.count = count;
	local_dtctx.oldtype = data_in;
	
	if( ref_dtctx == NULL )
	{
		dtctx = &local_dtctx;
	}
	else
	{
		dtctx = ref_dtctx;
	}
		
	/* If source datatype is a derived datatype we have to create a new derived datatype */
	if (sctk_datatype_is_derived (data_in))
	{
		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (data_in, &derived_ret, &input_datatype);

		/* Compute the total datatype size including boundaries */
		unsigned long extent;
		__INTERNAL__PMPI_Type_extent (data_in, (MPI_Aint *) & extent);

		/* By definition the number of output types is count times the one of the input type */
		unsigned long count_out = input_datatype.count * count ;
		
		/* Allocate datatype description */
		mpc_pack_absolute_indexes_t *begins_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		mpc_pack_absolute_indexes_t *ends_out =	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		sctk_datatype_t * datatypes = 	sctk_malloc (count_out * sizeof (sctk_datatype_t));

		if( !begins_out || !ends_out || !datatypes )
		{
			MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_INTERN, "Failled to allocate type context");
		}

		/* Fill in datatype description for a contiguous type */
		unsigned long i;

		/* Input : ABC of length count_in
		 * Output : ABCABCABC... of length count_out
		 */

		long new_ub = input_datatype.ub;
		long new_lb = input_datatype.lb;
		long next_ub,next_lb,cur_ub,cur_lb;
		
		next_ub = input_datatype.ub;
		next_lb = input_datatype.lb;

		for (i = 0; i < count_out; i++)
		{
		        cur_ub = next_ub;
			cur_lb = next_lb;

			begins_out[i] = input_datatype.begins[i % input_datatype.count]    /* Original begin offset in the input block */
							+ extent * (i / input_datatype.count); /* New offset due to type replication */
							
			ends_out[i] = input_datatype.ends[i % input_datatype.count]		   /* Original end offset in the input block */
							+ extent * (i / input_datatype.count); /* New offset due to type replication */
			
			datatypes[i] = input_datatype.datatypes[i % input_datatype.count];

			if(i % input_datatype.count == input_datatype.count - 1){   
			  next_ub = cur_ub + extent;
			  next_lb = cur_ub;
			  
			  if(cur_ub > new_ub) new_ub = cur_ub;
			  if(cur_lb < new_lb) new_lb = cur_lb;
			}

			
			sctk_nodebug ("%d , %lu-%lu <- %lu-%lu", i, begins_out[i],
					ends_out[i], input_datatype.begins[i % input_datatype.count],
					input_datatype.ends[i % input_datatype.count]);
		}

		/* Handle the NULL count case */
		if( !count ){
			new_ub = 0;
			new_lb = 0;
		}
		
		/* Actually create the new datatype */
		PMPC_Derived_datatype (data_out, begins_out, ends_out, datatypes, count_out, new_lb,	input_datatype.is_lb, new_ub, input_datatype.is_ub, dtctx);

		/* Free temporary buffers */
		sctk_free (datatypes);
		sctk_free (begins_out);
		sctk_free (ends_out);
	}
	else
	{
		/* Here we handle contiguous or common datatypes which can be replicated directly */
		__INTERNAL__PMPC_Type_hcontiguous (data_out, count, &data_in, dtctx);
	}
	
	return MPI_SUCCESS;
}


/** \brief This function creates a contiguous MPI_Datatype
 *  
 *  Such datatype is obtained by directly appending  count copies of data_in type
 *  
 *  If the source datatype is A[ ]B and count is 3 we obtain A[ ]BA[ ]BA[ ]B
 *  
 *  \param count Number of elements of type data_in in the data_out type
 *  \param data_in The type to be replicated count times
 *  \param data_out The output type
 * 
 */
static int __INTERNAL__PMPI_Type_contiguous (unsigned long count, MPI_Datatype data_in,  MPI_Datatype * data_out )
{
	/* Here we set the ctx to NULL in order to create a contiguous type (no overriding) */
	return __INTERNAL__PMPI_Type_contiguous_inherits (count,  data_in, data_out, NULL);
}

/** \brief This function creates a vector datatype
 * 	\param count Number of items in the vector
 * 	\param blocklen Lenght of a block in terms of old_type
 * 	\param stride Size of gaps in terms of old_type
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_vector (int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Aint extent;
	/* Retrieve type extent */
	__INTERNAL__PMPI_Type_extent(old_type, &extent);
	
	/* Set its context to overide the one from hvector */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_VECTOR;
	dtctx.count = count;
	dtctx.blocklength = blocklen;
	dtctx.stride = stride;
	dtctx.oldtype = old_type;


	if( (blocklen == stride) && ( 0 <= stride ) )
	{
		int ret = __INTERNAL__PMPI_Type_contiguous_inherits (count * blocklen, old_type,  newtype_p, &dtctx);
		MPC_Datatype_set_context( *newtype_p, &dtctx);
		return ret;
	}
		
	
	/* Compute the stride in bytes */
	unsigned long stride_t = stride * extent ;
	
	/* Call the hvector function */
	int res =  __INTERNAL__PMPI_Type_hvector( count, blocklen,  stride_t, old_type,  newtype_p);
	

	MPC_Datatype_set_context( *newtype_p, &dtctx);
	
	return res;
}

/** \brief This function creates a vector datatype with a stride in bytes
 * 	\param count Number of items in the vector
 * 	\param blocklen Lenght of a block in terms of old_type
 * 	\param stride Size of gaps in bytes
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_hvector (int count,
				 	 int blocklen,
					 MPI_Aint stride,
					 MPI_Datatype old_type,
					 MPI_Datatype * newtype_p)
{
	unsigned long extent;
	unsigned long stride_t;
	
	stride_t = (unsigned long) stride;

	/* Set its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_HVECTOR;
	dtctx.count = count;
	dtctx.blocklength = blocklen;
	dtctx.stride_addr = stride;
	dtctx.oldtype = old_type;
	
	/* Is it a derived data-type ? */
	if (sctk_datatype_is_derived (old_type))
	{
		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (old_type, &derived_ret, &input_datatype);
		
		/* Compute the extent */
		__INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

		/*  Handle the contiguous case or Handle count == 0 */
		if( (((blocklen * extent) == stride) && ( 0 <= stride ))
		|| (count == 0 ) )
		{
			int ret = __INTERNAL__PMPI_Type_contiguous_inherits (count * blocklen, old_type,  newtype_p, &dtctx);
			MPC_Datatype_set_context( *newtype_p, &dtctx);
			return ret;
		}
		
		sctk_nodebug ("extend %lu, count %d, blocklen %d, stide %lu", extent, count, blocklen, stride);

		/* The vector is created by repeating the input blocks
		 * count * blocklen times where the actual input type
		 * contains input_datatype.count entries */
		unsigned long count_out = input_datatype.count * count * blocklen;
		
		mpc_pack_absolute_indexes_t *begins_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		mpc_pack_absolute_indexes_t *ends_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		sctk_datatype_t *datatypes = sctk_malloc (count_out * sizeof (sctk_datatype_t));
		
		/* Here we flatten the vector */
		int i;
		int j;
		unsigned long k;


		long new_ub = input_datatype.ub;
		long new_lb = input_datatype.lb;
		long next_ub,next_lb,cur_ub,cur_lb;
		
		next_ub = input_datatype.ub;
		next_lb = input_datatype.lb;


		for (i = 0; i < count; i++)
		{
			/* For block */
			for (j = 0; j < blocklen; j++)
			{
			        cur_ub = next_ub;
				cur_lb = next_lb;

				/* For each block in the block length */
				for (k = 0; k < input_datatype.count; k++)
				{
					/* For each input block */
					begins_out[(i * blocklen + j) * input_datatype.count + k] =	input_datatype.begins[k] + (stride_t * i) + (j * extent);
					ends_out[(i * blocklen + j) * input_datatype.count + k] = input_datatype.ends[k] + (stride_t * i) + (j * extent);
					datatypes[(i * blocklen + j) * input_datatype.count + k] = input_datatype.datatypes[k];
				}                
				next_ub = cur_ub + extent;
				next_lb = cur_lb + extent;
				
				if(cur_ub > new_ub) new_ub = cur_ub;
				if(cur_lb < new_lb) new_lb = cur_lb;
			}
			
			next_ub = input_datatype.ub + ((stride* (i + 1)));
			next_lb = input_datatype.lb + ((stride* (i + 1)));
			
		}

		/* Create the derived datatype */
		PMPC_Derived_datatype (newtype_p, begins_out, ends_out, datatypes, count_out, new_lb, input_datatype.is_lb, new_ub, input_datatype.is_ub, &dtctx);
		
		/* Free temporary arrays */
		sctk_free (begins_out);
		sctk_free (ends_out);
		sctk_free (datatypes);

		return MPI_SUCCESS;
	}
	else
	{
		/* Here vector only handles derived data-types*/
		int res;
		MPI_Datatype data_out;
		
		/* Convert the contiguous type to a derived data-type */
		PMPC_Type_convert_to_derived( old_type, &data_out );
		
		/* Call vector again with the temporary
		 * derived types */
		res = __INTERNAL__PMPI_Type_hvector (count, blocklen, (MPI_Aint) stride_t, data_out, newtype_p);
		
		/* Free the temporary type */
		__INTERNAL__PMPI_Type_free (&data_out);

		return res;
	}
}

/** \brief This function creates a sequence of contiguous blocks each with their offset and length
 * 	\param count Number of blocks
 * 	\param blocklen length of individual blocks
 * 	\param indices Offset of each block in old_type size multiples
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_indexed (int count, int blocklens[], int indices[], MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Aint extent;
	/* Retrieve type extent */
	__INTERNAL__PMPI_Type_extent(old_type, &extent);
	
	/* Create a temporary offset array */
	MPI_Aint * byte_offsets = sctk_malloc (count * sizeof (MPI_Aint));
	assume( byte_offsets != NULL );
	
	int i;
	/* Fill it with by converting type based indices to bytes */
	for( i = 0 ; i < count ; i++ )
		byte_offsets[i] = indices[i] * extent;
	
	/* Call the byte offseted version of type_indexed */
	int res =  __INTERNAL__PMPI_Type_hindexed (count, blocklens, byte_offsets, old_type, newtype);
	
	/* Set its context to overide the one from hdindexed */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_INDEXED;
	dtctx.count = count;
	dtctx.array_of_blocklenght = blocklens;
	dtctx.array_of_displacements = indices;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype, &dtctx);

	/* Release the temporary byte offset array */
	sctk_free( byte_offsets );
	
	return res;
}

/** \brief This function creates a sequence of contiguous blocks each with their offset but with the same length
 * 	\param count Number of blocks
 * 	\param blocklength length of every blocks
 * 	\param indices Offset of each block in old_type size multiples
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	/* Here we just artificially create an array of blocklength to call __INTERNAL__PMPI_Type_indexed */
	int * blocklength_array = sctk_malloc( count * sizeof( int ) );
	assume( blocklength_array != NULL );
	
	int i;
	for( i = 0 ; i < count ; i++ )
		blocklength_array[i] = blocklength;
	
	/* Call the orignal indexed function */
	int res = __INTERNAL__PMPI_Type_hindexed(count, blocklength_array, indices, old_type,  newtype);
	
	/* Set its context to overide the one from hdindexed */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_HINDEXED_BLOCK;
	dtctx.count = count;
	dtctx.blocklength = blocklength;
	dtctx.array_of_displacements_addr = indices;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	/* Free the tmp buffer */
	sctk_free( blocklength_array );
	
	return res;
}


/** \brief This function creates a sequence of contiguous blocks each with their offset but with the same length
 * 	\param count Number of blocks
 * 	\param blocklength length of every blocks
 * 	\param indices Offset of each block in old_type size multiples
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_create_indexed_block(int count, int blocklength, int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	/* Convert the indices to bytes */
	MPI_Aint extent;
	/* Retrieve type extent */
	__INTERNAL__PMPI_Type_extent(old_type, &extent);
	

	/* Create a temporary offset array */
	MPI_Aint * byte_offsets = sctk_malloc (count * sizeof (MPI_Aint));
	assume( byte_offsets != NULL );
	
	int i;
	/* Fill it with by converting type based indices to bytes */
	for( i = 0 ; i < count ; i++ )
		byte_offsets[i] = indices[i] * extent;

	/* Call the orignal indexed function */
	int res = __INTERNAL__PMPI_Type_create_hindexed_block (count, blocklength, byte_offsets, old_type,  newtype);
	
	/* Set its context to overide the one from hdindexed block */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_INDEXED_BLOCK;
	dtctx.count = count;
	dtctx.blocklength = blocklength;
	dtctx.array_of_displacements = indices;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	/* Free the temporary byte offset */
	sctk_free( byte_offsets );
	
	return res;
}



/** \brief This function creates a sequence of contiguous blocks each with their offset and length
 * 	\param count Number of blocks
 * 	\param blocklen length of individual blocks
 * 	\param indices Offset of each block in bytes
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_hindexed (int count,
					  int blocklens[],
					  MPI_Aint indices[],
					  MPI_Datatype old_type, MPI_Datatype * newtype)
{
	/* Set its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_HINDEXED;
	dtctx.count = count;
	dtctx.array_of_blocklenght = blocklens;
	dtctx.array_of_displacements_addr = indices;
	dtctx.oldtype = old_type;
		
	/* Is it a derived data-type ? */
	if (sctk_datatype_is_derived (old_type))
	{
		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (old_type, &derived_ret, &input_datatype);
		
		/* Get datatype extent */
		unsigned long extent;
		__INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

		/*  Handle the contiguous case or Handle count == 0 */
		if( count == 0 )
		{
			int ret = __INTERNAL__PMPI_Type_contiguous_inherits ( 0, old_type, newtype, &dtctx);
			MPC_Datatype_set_context( *newtype, &dtctx);
			return ret;
		}
		
		
		/* Compute the total number of blocks */
		unsigned long count_out = 0;
		int i;
		for (i = 0; i < count; i++)
		{
			count_out += input_datatype.count * blocklens[i];
		}

		/* Allocate context */
		mpc_pack_absolute_indexes_t * begins_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		mpc_pack_absolute_indexes_t * ends_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		sctk_datatype_t *datatypes = sctk_malloc (count_out * sizeof (sctk_datatype_t));

		sctk_nodebug ("%lu extent", extent);

		int j;
		unsigned long k;
		unsigned long step = 0;
		long int new_ub  = input_datatype.ub ;
		long int new_lb = input_datatype.lb;
		
	
		for (i = 0; i < count; i++)
		{
			/* For each output entry */
			for (j = 0; j < blocklens[i]; j++)
			{
				/* For each entry in the block */
				for (k = 0; k < input_datatype.count; k++)
				{
					/* For each block in the input */
					begins_out[step] = input_datatype.begins[k] + indices[i] + j * extent;
					ends_out[step] = input_datatype.ends[k] + indices[i] + j * extent;
					datatypes[step] = input_datatype.datatypes[k];
					step++;
				}
			}
			
			
			/* Update UB */
			long int candidate_b = 0;
			
			if (input_datatype.is_ub)
			{
				candidate_b = input_datatype.ub + indices[i] + extent * (blocklens[i] - 1);
				
				if ( input_datatype.ub < candidate_b)
				{
					new_ub = candidate_b;
				}
			}
			
			/* Update LB */
			if (input_datatype.is_lb)
			{
				candidate_b = input_datatype.lb + indices[i];
				
				if ( candidate_b < input_datatype.lb)
				{
					new_lb = candidate_b;
				}
			}
		}
	
		/* Create the derived datatype */
		PMPC_Derived_datatype (newtype, begins_out, ends_out, datatypes, count_out, new_lb,	input_datatype.is_lb, new_ub, input_datatype.is_ub, &dtctx);

		
		/* Free temporary arrays */
		sctk_free (begins_out);
		sctk_free (ends_out);
		sctk_free (datatypes);
		
		return MPI_SUCCESS;
	}
	else
	{
		/* Here type hindexed only handles derived data-types */
		int res;
		MPI_Datatype data_out;
		
		/* Convert the contiguous/common type to a derived one */
		PMPC_Type_convert_to_derived( old_type, &data_out );
		
		/* Call the hindexed function again */
		res = __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, data_out, newtype);
		
		/* Free the temporary type */
		__INTERNAL__PMPI_Type_free (&data_out);

		return res;
	}
}

static int __INTERNAL__PMPI_Type_struct(int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype * newtype)
{
	int i;
	int res = MPI_SUCCESS;

	unsigned long count_out = 0;
	unsigned long glob_count_out = 0;
	mpc_pack_absolute_indexes_t new_lb = (mpc_pack_absolute_indexes_t) (-1);
	int new_is_lb = 0;
	mpc_pack_absolute_indexes_t new_ub = 0;
	mpc_pack_absolute_indexes_t common_type_size = 0;
	int new_is_ub = 0;
	unsigned long my_count_out = 0;
	
	if( ! count )
		new_lb = 0;


	// find malloc size
	for (i = 0; i < count; i++)
	{
		if ((old_types[i] != MPI_UB) && (old_types[i] != MPI_LB))
		{
			int derived_ret = 0;
			sctk_derived_datatype_t input_datatype;
			
			int count_in = 0;

			if (sctk_datatype_is_derived(old_types[i]))
			{
				MPC_Is_derived_datatype(old_types[i], &derived_ret, &input_datatype);
				count_in = input_datatype.count;
				sctk_nodebug("[%d]Derived length %d", old_types[i], count_in);
			}
			else
			{
				count_in = 1;
				sctk_nodebug("[%d]Contiguous length %d",old_types[i], count_in);
			}

			my_count_out += count_in * blocklens[i];
		}
	}
	
	
	
	sctk_nodebug("my_count_out = %d", my_count_out);
	mpc_pack_absolute_indexes_t * begins_out = sctk_malloc(my_count_out * sizeof(mpc_pack_absolute_indexes_t));
	mpc_pack_absolute_indexes_t * ends_out = sctk_malloc(my_count_out * sizeof(mpc_pack_absolute_indexes_t));
	sctk_datatype_t *datatypes = sctk_malloc (my_count_out * sizeof (sctk_datatype_t));

	for (i = 0; i < count; i++)
	{
		if ((old_types[i] != MPI_UB) && (old_types[i] != MPI_LB))
		{
			int res, j, is_lb, is_ub;
			size_t tmp;
			
			unsigned long count_in, extent, k, stride_t;
			unsigned long local_count_out = 0;
			unsigned long prev_count_out = 0;

			int derived_ret = 0;
			sctk_derived_datatype_t input_datatype;


			if (sctk_datatype_is_derived(old_types[i]))
			{
				MPC_Is_derived_datatype(old_types[i], &derived_ret, &input_datatype);
				
				__INTERNAL__PMPI_Type_extent(old_types[i], (MPI_Aint *) &extent);
				sctk_nodebug("Extend %lu", extent);
				stride_t = (unsigned long) indices[i];
				
				for (j = 0; j < blocklens[i]; j++)
				{
					for (k = 0; k < input_datatype.count; k++)
					{
						begins_out[glob_count_out] = input_datatype.begins[k] + stride_t + (extent * j);
						ends_out[glob_count_out] = input_datatype.ends[k] + stride_t + (extent * j);
						datatypes[glob_count_out] = input_datatype.datatypes[k];
						
						sctk_nodebug(" begins_out[%lu] = %lu", glob_count_out, begins_out[glob_count_out]);
						sctk_nodebug(" ends_out[%lu] = %lu", glob_count_out, ends_out[glob_count_out]);

						glob_count_out++;
					}
				}
				
				sctk_nodebug("derived type %d new_lb %d new_ub %d before", i, new_lb, new_ub);
				
				if (input_datatype.is_ub)
				{
					mpc_pack_absolute_indexes_t new_b;
					new_b = input_datatype.ub + indices[i] + extent * (blocklens[i] - 1);
					sctk_nodebug("cur ub %d", new_b);
					if ((long int) new_b > (long int) new_ub || new_is_ub == 0)
					{
						new_ub = new_b;
					}
					new_is_ub = 1;
				}
				
				if (input_datatype.is_lb)
				{
					mpc_pack_absolute_indexes_t new_b;
					new_b = input_datatype.lb + indices[i];
					sctk_nodebug("cur lb %d", new_b);
					if ((long int) new_b < (long int) new_lb || new_is_lb == 0)
					{
						new_lb = new_b;
					}
					new_is_lb = 1;
				}
				
				sctk_nodebug("derived type %d new_lb %d new_ub %d after ", i, new_lb, new_ub);
			}
			else
			{
				mpc_pack_absolute_indexes_t *begins_in, *ends_in;
				mpc_pack_absolute_indexes_t begins_in_static, ends_in_static;
				
				begins_in = &begins_in_static;
				ends_in = &ends_in_static;
				begins_in[0] = 0;
				PMPC_Type_size(old_types[i], &(tmp));
				ends_in[0] = begins_in[0] + tmp - 1;
				count_in = 1;
				sctk_nodebug("Type size %lu", tmp);

				__INTERNAL__PMPI_Type_extent(old_types[i], (MPI_Aint *) &extent);

				stride_t = (unsigned long) indices[i];
				for (j = 0; j < blocklens[i]; j++)
				{
						begins_out[glob_count_out] = begins_in[0] + stride_t + (extent * j);
						ends_out[glob_count_out] = ends_in[0] + stride_t + (extent * j);
						datatypes[glob_count_out] = old_types[i];

						/* We use this variable to keep track of structure ending
						 * in order to do alignment when we only handle common types */
						if( common_type_size < ends_out[glob_count_out] )
							common_type_size = ends_out[glob_count_out];

						glob_count_out++;
				}
				sctk_nodebug("simple type %d new_lb %d new_ub %d", i, new_lb, new_ub);
/* 				fprintf(stderr,"Type struct %ld-%ld\n",begins_out[i],ends_out[i]+1); */
			}

			prev_count_out = count_out;
			local_count_out = count_in * blocklens[i];
			count_out += local_count_out;
		}
		else
		{
			if (old_types[i] == MPI_UB)
			{
				new_is_ub = 1;
				new_ub = indices[i];
			}
			else
			{
				new_is_lb = 1;
				new_lb = indices[i];
			}
			sctk_nodebug("hard bound %d new_lb %d new_ub %d", i, new_lb, new_ub);
		}
		sctk_nodebug("%d new_lb %d new_ub %d", i, new_lb, new_ub);
	}
/* 	fprintf(stderr,"End Type\n"); */


	/* Now check for padding if involved types are all common
	 * in order to make alignment decisions (p. 85 of the standard)
	 * 
	 * struct
	 * {
	 * 		int a;
	 * 		char c;
	 * };
	 * 
	 * I I I I C
	 * 
	 * Should map to
	 * 
	 * I I I I C - - - 
	 * 
	 * To fulfil alignment constraints
	 * 
	 * */

	int did_pad_struct = 0;

	if( count )
	{

		int types_are_all_common = 1;
		int max_pad_value = 0;
		
		/* First check if we are playing with common datatypes
		 * Note that types with UB LB are ignored */
		for (i = 0; i < count; i++)
		{
			MPI_Aint cur_type_extent = 0;
			if( !sctk_datatype_is_common( old_types[i] ) )
			{
					types_are_all_common = 0;
					break;
			}
			/*
			  Check if all common types are well aligned. If not, skip the 
			  padding procedure
			 */
			__INTERNAL__PMPI_Type_extent(old_types[i], &cur_type_extent);
			if(indices[i] % cur_type_extent != 0){
			  types_are_all_common = 0;
			  break;
			} else {
			  if(max_pad_value < cur_type_extent){
			    max_pad_value = cur_type_extent;
			  }
			}
		}
	

		if( types_are_all_common )
		{			
		  common_type_size++;
			if(( max_pad_value > 0 ) && (common_type_size % max_pad_value  != 0) )
			{
				int extent_mod = max_pad_value;

				
				int missing_to_allign = common_type_size % extent_mod;

				/* Set an UB to the type to fulfil alignment */
				new_is_ub = 1;
				new_ub = common_type_size + extent_mod - missing_to_allign;
				
				if( new_ub != common_type_size )
					did_pad_struct = 1;
				
			}
		}

	}


	/* Padding DONE HERE */



	/* Set its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_STRUCT;
	dtctx.count = count;
	dtctx.array_of_blocklenght = blocklens;
	dtctx.array_of_displacements_addr = indices;
	dtctx.array_of_types = old_types;

	res = PMPC_Derived_datatype(newtype, begins_out, ends_out, datatypes, glob_count_out, new_lb, new_is_lb, new_ub, new_is_ub, &dtctx);
	assert(res == MPI_SUCCESS);


	/* Set the struct type as padded to return the UB in MPI_Type_size */
	
	if( did_pad_struct )
	{
		PMPC_Type_flag_padded( *newtype );
	}
	
	/*   sctk_nodebug("new_type %d",* newtype); */
	/*   sctk_nodebug("final new_lb %d,%d new_ub %d %d",new_lb,new_is_lb,new_ub,new_is_ub); */
	/*   { */
	/*     int i ;  */
	/*     for(i = 0; i < glob_count_out; i++){ */
	/*       sctk_nodebug("%d begins %lu ends %lu",i,begins_out[i],ends_out[i]); */
	/*     } */
	/*   } */

	sctk_free(begins_out);
	sctk_free(ends_out);
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type)
{
	if( sctk_datatype_is_derived(old_type) )
	{
		int derived_ret;
		sctk_derived_datatype_t input_datatype;
		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (old_type, &derived_ret, &input_datatype);
		
		struct Datatype_External_context dtctx;
		sctk_datatype_external_context_clear( &dtctx );
		dtctx.combiner = MPI_COMBINER_RESIZED;
		dtctx.lb = lb;
		dtctx.extent = extent;
		dtctx.oldtype = old_type;
		
		/* Duplicate it with updated bounds in new_type */
		PMPC_Derived_datatype ( new_type,
					input_datatype.begins,
					input_datatype.ends,
					input_datatype.datatypes,
					input_datatype.count,
					lb, 1,
					lb + extent, 1, &dtctx);
		
		
		return MPI_SUCCESS;
	}
	else
	{
		/* Here tPMPI_Type_create_resized only handles derived data-types */
		int res;
		MPI_Datatype data_out;
		
		/* Convert the contiguous/common type to a derived one */
		PMPC_Type_convert_to_derived( old_type, &data_out );
		
		/* Call the hindexed function again */
		res = __INTERNAL__PMPI_Type_create_resized (data_out, lb, extent, new_type);
		
		/* Free the temporary type */
		__INTERNAL__PMPI_Type_free (&data_out);

		return res;
	}
}



/* #########################################################################
 * sctk_MPIOI_Type_block, sctk_MPIOI_Type_cyclic,  sctk_Type_create_darray
 * and sctk_Type_create_subarray are from the ROMIO implemntations and
 * are subject to ROMIO copyright:
 * Copyright (C) 1997 University of Chicago. 
                               COPYRIGHT
 * #########################################################################
 */

/************************************************************************/
/* DARRAY IMPLEMENTATION                                                */
/************************************************************************/


static int sctk_MPIOI_Type_block(int *array_of_gsizes, int dim, int ndims, int nprocs,
		     int rank, int darg, int order, MPI_Aint orig_extent,
		     MPI_Datatype type_old, MPI_Datatype *type_new,
		     MPI_Aint *st_offset);

static int sctk_MPIOI_Type_cyclic(int *array_of_gsizes, int dim, int ndims, int nprocs,
		      int rank, int darg, int order, MPI_Aint orig_extent,
		      MPI_Datatype type_old, MPI_Datatype *type_new,
		      MPI_Aint *st_offset);


int sctk_Type_create_darray(int size, int rank, int ndims, 
			    int *array_of_gsizes, int *array_of_distribs, 
			    int *array_of_dargs, int *array_of_psizes, 
			    int order, MPI_Datatype oldtype, 
			    MPI_Datatype *newtype) 
{
    MPI_Datatype type_old, type_new=MPI_DATATYPE_NULL, types[3];
    int procs, tmp_rank, i, tmp_size, blklens[3], *coords;
    MPI_Aint *st_offsets, orig_extent, disps[3];

    MPI_Type_extent(oldtype, &orig_extent);

/* calculate position in Cartesian grid as MPI would (row-major
   ordering) */
    coords = (int *) sctk_malloc(ndims*sizeof(int));
    procs = size;
    tmp_rank = rank;
    for (i=0; i<ndims; i++) {
	procs = procs/array_of_psizes[i];
	coords[i] = tmp_rank/procs;
	tmp_rank = tmp_rank % procs;
    }

    st_offsets = (MPI_Aint *) sctk_malloc(ndims*sizeof(MPI_Aint));
    type_old = oldtype;

    if (order == MPI_ORDER_FORTRAN) {
      /* dimension 0 changes fastest */
	for (i=0; i<ndims; i++) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims,
				 array_of_psizes[i],
				 coords[i], array_of_dargs[i],
				 order, orig_extent, 
				 type_old, &type_new,
				 st_offsets+i); 
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		sctk_MPIOI_Type_cyclic(array_of_gsizes, i, ndims, 
				  array_of_psizes[i], coords[i],
				  array_of_dargs[i], order,
				  orig_extent, type_old,
				  &type_new, st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, 1, 0, 
				 MPI_DISTRIBUTE_DFLT_DARG, order,
				 orig_extent, 
				 type_old, &type_new,
				 st_offsets+i); 
		break;
	    }
	    if (i) MPI_Type_free(&type_old);
	    type_old = type_new;
	}

	/* add displacement and UB */
	disps[1] = st_offsets[0];
	tmp_size = 1;
	for (i=1; i<ndims; i++) {
	    tmp_size *= array_of_gsizes[i-1];
	    disps[1] += (MPI_Aint)tmp_size*st_offsets[i];
	}
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
        /* dimension ndims-1 changes fastest */
	for (i=ndims-1; i>=0; i--) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
				 coords[i], array_of_dargs[i], order,
				 orig_extent, type_old, &type_new,
				 st_offsets+i); 
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		sctk_MPIOI_Type_cyclic(array_of_gsizes, i, ndims, 
				  array_of_psizes[i], coords[i],
				  array_of_dargs[i], order, 
				  orig_extent, type_old, &type_new,
				  st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
		      coords[i], MPI_DISTRIBUTE_DFLT_DARG, order, orig_extent, 
                           type_old, &type_new, st_offsets+i); 
		break;
	    }
	    if (i != ndims-1) MPI_Type_free(&type_old);
	    type_old = type_new;
	}

	/* add displacement and UB */
	disps[1] = st_offsets[ndims-1];
	tmp_size = 1;
	for (i=ndims-2; i>=0; i--) {
	    tmp_size *= array_of_gsizes[i+1];
	    disps[1] += (MPI_Aint)tmp_size*st_offsets[i];
	}
    }

    disps[1] *= orig_extent;

    disps[2] = orig_extent;
    for (i=0; i<ndims; i++) disps[2] *= (MPI_Aint)array_of_gsizes[i];
	
    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = type_new;
    types[2] = MPI_UB;
    
    PMPI_Type_struct(3, blklens, disps, types, newtype);

    MPI_Type_free(&type_new);
    sctk_free(st_offsets);
    sctk_free(coords);
    
    return MPI_SUCCESS;
}


/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int sctk_MPIOI_Type_block(int *array_of_gsizes, int dim, int ndims, int nprocs,
		     int rank, int darg, int order, MPI_Aint orig_extent,
		     MPI_Datatype type_old, MPI_Datatype *type_new,
		     MPI_Aint *st_offset) 
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int blksize, global_size, mysize, i, j;
    MPI_Aint stride;
    
    global_size = array_of_gsizes[dim];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG)
	blksize = (global_size + nprocs - 1)/nprocs;
    else {
	blksize = darg;

	/* --BEGIN ERROR HANDLING-- */
	if (blksize <= 0) {
	    return MPI_ERR_ARG;
	}

	if (blksize * nprocs < global_size) {
	    return MPI_ERR_ARG;
	}
	/* --END ERROR HANDLING-- */
    }

    j = global_size - blksize*rank;
    mysize = (blksize < j)?blksize:j;
    if (mysize < 0) mysize = 0;

    stride = orig_extent;
    if (order == MPI_ORDER_FORTRAN) {
	if (dim == 0) 
	    MPI_Type_contiguous(mysize, type_old, type_new);
	else {
	    for (i=0; i<dim; i++) stride *= (MPI_Aint)array_of_gsizes[i];
	    MPI_Type_hvector(mysize, 1, stride, type_old, type_new);
	}
    }
    else {
	if (dim == ndims-1) 
	    MPI_Type_contiguous(mysize, type_old, type_new);
	else {
	    for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)array_of_gsizes[i];
	    MPI_Type_hvector(mysize, 1, stride, type_old, type_new);
	}

    }

    *st_offset = (MPI_Aint)blksize * (MPI_Aint)rank;
     /* in terms of no. of elements of type oldtype in this dimension */
    if (mysize == 0) *st_offset = 0;

    return MPI_SUCCESS;
}


/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int sctk_MPIOI_Type_cyclic(int *array_of_gsizes, int dim, int ndims, int nprocs,
		      int rank, int darg, int order, MPI_Aint orig_extent,
		      MPI_Datatype type_old, MPI_Datatype *type_new,
		      MPI_Aint *st_offset) 
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int blksize, i, blklens[3], st_index, end_index, local_size, rem, count;
    MPI_Aint stride, disps[3];
    MPI_Datatype type_tmp, types[3];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG) blksize = 1;
    else blksize = darg;

    /* --BEGIN ERROR HANDLING-- */
    if (blksize <= 0) {
	return MPI_ERR_ARG;
    }
    /* --END ERROR HANDLING-- */
    
    st_index = rank*blksize;
    end_index = array_of_gsizes[dim] - 1;

    if (end_index < st_index) local_size = 0;
    else {
	local_size = ((end_index - st_index + 1)/(nprocs*blksize))*blksize;
	rem = (end_index - st_index + 1) % (nprocs*blksize);
	local_size +=  (rem < blksize)?rem:blksize;
    }

    count = local_size/blksize;
    rem = local_size % blksize;
    
    stride = (MPI_Aint)nprocs*(MPI_Aint)blksize*orig_extent;
    if (order == MPI_ORDER_FORTRAN)
	for (i=0; i<dim; i++) stride *= (MPI_Aint)array_of_gsizes[i];
    else for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)array_of_gsizes[i];

    MPI_Type_hvector(count, blksize, stride, type_old, type_new);

    if (rem) {
	/* if the last block is of size less than blksize, include
	   it separately using MPI_Type_struct */

	types[0] = *type_new;
	types[1] = type_old;
	disps[0] = 0;
	disps[1] = (MPI_Aint)count*stride;
	blklens[0] = 1;
	blklens[1] = rem;

	PMPI_Type_struct(2, blklens, disps, types, &type_tmp);

	MPI_Type_free(type_new);
	*type_new = type_tmp;
    }

    /* In the first iteration, we need to set the displacement in that
       dimension correctly. */ 
    if ( ((order == MPI_ORDER_FORTRAN) && (dim == 0)) ||
         ((order == MPI_ORDER_C) && (dim == ndims-1)) ) {
        types[0] = MPI_LB;
        disps[0] = 0;
        types[1] = *type_new;
        disps[1] = (MPI_Aint)rank * (MPI_Aint)blksize * orig_extent;
        types[2] = MPI_UB;
        disps[2] = orig_extent * (MPI_Aint)array_of_gsizes[dim];
        blklens[0] = blklens[1] = blklens[2] = 1;
        PMPI_Type_struct(3, blklens, disps, types, &type_tmp);
        MPI_Type_free(type_new);
        *type_new = type_tmp;

        *st_offset = 0;  /* set it to 0 because it is taken care of in
                            the struct above */
    }
    else {
        *st_offset = (MPI_Aint)rank * (MPI_Aint)blksize; 
        /* st_offset is in terms of no. of elements of type oldtype in
         * this dimension */ 
    }

    if (local_size == 0) *st_offset = 0;

    return MPI_SUCCESS;
}


/************************************************************************/
/* SUBARRAY IMPLEMENTATION                                              */
/************************************************************************/


int sctk_Type_create_subarray(int ndims,
			      int *array_of_sizes, 
			      int *array_of_subsizes,
			      int *array_of_starts,
			      int order,
			      MPI_Datatype oldtype, 
			      MPI_Datatype *newtype)
{
    MPI_Aint extent, disps[3], size;
    int i, blklens[3];
    MPI_Datatype tmp1, tmp2, types[3];

    MPI_Type_extent(oldtype, &extent);

    if (order == MPI_ORDER_FORTRAN) {
	/* dimension 0 changes fastest */
	if (ndims == 1) {
	    MPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
	}
	else {
	    MPI_Type_vector(array_of_subsizes[1],
			    array_of_subsizes[0],
			    array_of_sizes[0], oldtype, &tmp1);
	    
	    size = (MPI_Aint)array_of_sizes[0]*extent;
	    for (i=2; i<ndims; i++) {
		size *= (MPI_Aint)array_of_sizes[i-1];
		MPI_Type_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
		MPI_Type_free(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	disps[1] = array_of_starts[0];
	size = 1;
	for (i=1; i<ndims; i++) {
	    size *= (MPI_Aint)array_of_sizes[i-1];
	    disps[1] += size*(MPI_Aint)array_of_starts[i];
	}  
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
	/* dimension ndims-1 changes fastest */
	if (ndims == 1) {
	    MPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
	}
	else {
	    MPI_Type_vector(array_of_subsizes[ndims-2],
			    array_of_subsizes[ndims-1],
			    array_of_sizes[ndims-1], oldtype, &tmp1);
	    
	    size = (MPI_Aint)array_of_sizes[ndims-1]*extent;
	    for (i=ndims-3; i>=0; i--) {
		size *= (MPI_Aint)array_of_sizes[i+1];
		MPI_Type_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
		MPI_Type_free(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	disps[1] = array_of_starts[ndims-1];
	size = 1;
	for (i=ndims-2; i>=0; i--) {
	    size *= (MPI_Aint)array_of_sizes[i+1];
	    disps[1] += size*(MPI_Aint)array_of_starts[i];
	}
    }
    
    disps[1] *= extent;
    
    disps[2] = extent;
    for (i=0; i<ndims; i++) disps[2] *= (MPI_Aint)array_of_sizes[i];
    
    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = tmp1;
    types[2] = MPI_UB;
    
    PMPI_Type_struct(3, blklens, disps, types, newtype);

    MPI_Type_free(&tmp1);

    return MPI_SUCCESS;
}

/* #########################################################################
 *  END OF ROMIO CODE
 * ########################################################################*/

int __INTERNAL__PMPI_Type_create_darray (int size,
					 int rank,
					 int ndims,
					 int array_of_gsizes[],
					 int array_of_distribs[],
					 int array_of_dargs[],
					 int array_of_psizes[],
					 int order,
					 MPI_Datatype oldtype,
					 MPI_Datatype *newtype)
{
	*newtype = MPC_DATATYPE_NULL;
	

	int res = sctk_Type_create_darray( size, rank, ndims,  array_of_gsizes,
					   array_of_distribs,  array_of_dargs, array_of_psizes,
					   order, oldtype,   newtype );

	if( res != MPI_SUCCESS )
		return res;
	
	/* Fill its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_DARRAY;
	dtctx.size = size;
	dtctx.ndims = ndims;
	dtctx.rank = rank;
	dtctx.array_of_gsizes = array_of_gsizes;
	dtctx.array_of_distribs = array_of_distribs;
	dtctx.array_of_dargs = array_of_dargs;
	dtctx.array_of_psizes = array_of_psizes;
	dtctx.order = order;
	dtctx.oldtype = oldtype;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Type_create_subarray (int ndims,
					   int array_of_sizes[],
					   int array_of_subsizes[],
					   int array_of_starts[],
					   int order,
					   MPI_Datatype oldtype,
					   MPI_Datatype * new_type)
{
	/* Create the subarray */
	int res = sctk_Type_create_subarray( ndims,  array_of_sizes,  array_of_subsizes, array_of_starts,  order, oldtype,  new_type);

	if( res != MPI_SUCCESS )
		return res;
	
	/* Fill its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_SUBARRAY;
	dtctx.ndims = ndims;
	dtctx.array_of_sizes = array_of_sizes;
	dtctx.array_of_subsizes = array_of_subsizes;
	dtctx.array_of_starts = array_of_starts;
	dtctx.order = order;
	dtctx.oldtype = oldtype;
	MPC_Datatype_set_context( *new_type, &dtctx);
	
	return res;
}



static int __INTERNAL__PMPI_Address (void *location, MPI_Aint * address)
{
	*address = (MPI_Aint) location;
	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Type_extent (MPI_Datatype datatype, MPI_Aint * extent)
{
  switch (datatype) {
  case MPI_LONG:
    *extent = sizeof(long);
    break;
  case MPI_SHORT:
    *extent = sizeof(short);
    break;
  case MPI_BYTE:
  case MPI_CHAR:
    *extent = sizeof(char);
    break;
  case MPI_INT:
    *extent = sizeof(int);
    break;
  case MPI_FLOAT:
    *extent = sizeof(float);
    break;
  case MPI_DOUBLE:
    *extent = sizeof(double);
    break;
  default: {
    MPI_Aint UB;
    MPI_Aint LB;

    /* Special cases */
    mpi_check_type(datatype, MPI_COMM_WORLD);

    __INTERNAL__PMPI_Type_lb(datatype, &LB);
    __INTERNAL__PMPI_Type_ub(datatype, &UB);

    *extent = (MPI_Aint)((unsigned long)UB - (unsigned long)LB);

    sctk_nodebug("UB %d LB %d EXTENT %d", UB, LB, *extent);
  }
  }

  return MPI_SUCCESS;
}

/* See the 1.1 version of the Standard.  The standard made an
   unfortunate choice here, however, it is the standard.  The size returned
   by MPI_Type_size is specified as an int, not an MPI_Aint */
int __INTERNAL__PMPI_Type_size (MPI_Datatype datatype, int *size)
{
	size_t tmp_size;
	int real_val;
	int res;
	
	res = PMPC_Type_size (datatype, &tmp_size);
	
	real_val = tmp_size;
	*size = real_val;
	return res;
}

static int __INTERNAL__PMPI_Type_size_x (MPI_Datatype datatype, MPI_Count *size)
{
	size_t tmp_size;
	MPI_Count real_val;
	int res;

	res = PMPC_Type_size (datatype, &tmp_size);
	
	real_val = tmp_size;
	*size = real_val;
	return res;
}





/* MPI_Type_count was withdrawn in MPI 1.1 */
static int __INTERNAL__PMPI_Type_lb (MPI_Datatype datatype, MPI_Aint * displacement)
{
	unsigned long i;

	int derived_ret = 0;
	sctk_derived_datatype_t input_datatype;

	MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
	
	if (derived_ret)
	{
		if ( (input_datatype.is_lb == 0) && input_datatype.count )
		{
			
			*displacement = (MPI_Aint) input_datatype.opt_begins[0];
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				if ((mpc_pack_absolute_indexes_t) * displacement > input_datatype.opt_begins[i])
				{
					*displacement = (MPI_Aint) input_datatype.opt_begins[i];
				}
			}
		}
		else
		{
			*displacement = input_datatype.lb;
		}
	}
	else
	{
		*displacement = 0;
	}
	
	sctk_nodebug ("%lu lb", *displacement);
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Type_ub (MPI_Datatype datatype, MPI_Aint * displacement)
{
	unsigned long i;
	int e;

	int derived_ret = 0;
	sctk_derived_datatype_t input_datatype;

	MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
	
	if (derived_ret)
	{
		if ( (input_datatype.is_ub == 0)  && input_datatype.count )
		{
			*displacement = (MPI_Aint) input_datatype.opt_ends[0];
			
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				if ((mpc_pack_absolute_indexes_t) * displacement < input_datatype.opt_ends[i])
				{
					*displacement = (MPI_Aint) input_datatype.opt_ends[i];
				}
				sctk_nodebug ("Current ub %lu, %lu", input_datatype.opt_ends[i], *displacement);
			}
			
			e = 1;
			*displacement = (MPI_Aint) ((unsigned long) (*displacement) + e);
		}
		else
		{
			*displacement = input_datatype.ub;
		}
	}
	else
	{
		size_t tmp;
		PMPC_Type_size (datatype, &tmp);
		*displacement = (MPI_Aint) tmp;
	}
	
	sctk_nodebug ("%lu ub", *displacement);
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Type_commit (MPI_Datatype * datatype)
{
  return PMPC_Type_commit(datatype);
}

static int __INTERNAL__PMPI_Type_free (MPI_Datatype * datatype)
{
  return PMPC_Type_free (datatype);
}

static int __INTERNAL__PMPI_Get_elements_x (MPI_Status * status, MPI_Datatype datatype, MPI_Count *elements)
{
	int res = MPI_SUCCESS;
	long long int size;
	int data_size;

	/* First check if the status is valid */
	if (NULL == status || MPI_STATUSES_IGNORE == status ||  MPI_STATUS_IGNORE == status || NULL == elements)
	{
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid argument");
	}
	
	/* Now check the data-type */
	mpi_check_type( datatype, MPI_COMM_WORLD );

	/* Get type size */
	res = __INTERNAL__PMPI_Type_size (datatype, &data_size);
	
	if (res != MPI_SUCCESS) {
		return res;
	}

	
	MPI_Datatype data_in;
	int data_in_size;
	size_t count;
	sctk_task_specific_t *task_specific;
	unsigned long i;

	*elements = 0;
	
	/* If size is NULL we have no element */
	if (data_size == 0)
	{
		return res;
	}
	
	/* If we are here our type has a size
	 * we can now count the number of elements*/
	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_CONTIGUOUS:
			/* A contiguous datatype is made of several
			 * elements of a single type next to each other
			 * we have to find out how many elements of this
			 * type are here */
			task_specific = __MPC_get_task_specific ();
			
			/* First retrieve the contiguous type descriptor */
			sctk_datatype_lock( task_specific );
			sctk_contiguous_datatype_t *contiguous_user_types = sctk_task_specific_get_contiguous_datatype( task_specific, datatype );
			/* Number of entries in the contiguous type */
			count = contiguous_user_types->count;
			/* Input type */
			data_in = contiguous_user_types->datatype;
			sctk_datatype_unlock( task_specific );
			
			/* Retrieve the size of the input type */
			res = __INTERNAL__PMPI_Type_size (data_in, &data_in_size);

			/* This is the size we received */
			size = status->size;

			*elements = size/data_in_size;
			
			if (res != MPI_SUCCESS) {
				return res;
			}
		break;
		
		case MPC_DATATYPES_DERIVED:
	
			task_specific = __MPC_get_task_specific ();
			
			/* This is the size we received */
			size = status->size;

			*elements = 0;

			/* Retrieve the derived datatype */
			sctk_datatype_lock( task_specific );
			sctk_assert ( MPC_TYPE_MAP_TO_DERIVED(datatype) < SCTK_USER_DATA_TYPES_MAX);	
			sctk_derived_datatype_t *target_type = sctk_task_specific_get_derived_datatype(  task_specific, datatype );
			sctk_assert ( target_type != NULL);
			sctk_datatype_unlock( task_specific );
			
			/* Try to rely on the datype layout */
			struct Datatype_layout *layout = sctk_datatype_layout( &target_type->context, &count );
			
			int done = 0;
			
			if( 0 /* Do not Use the Layout to compute Get_Element*/ )
			{
				if( ! count )
				{
					sctk_fatal("We found an empty layout");
				}
				
				while( !done )
				{
				
					sctk_error("count : %d  size : %d done : %d", count, size, done);
					for(i = 0; i < count; i++)
					{
						sctk_error("BLOCK SIZE  : %d", layout[i].size );
						
						size -= layout[i].size;
						
						(*elements)++;

						if(size <= 0)
						{	
							done = 1;
							break;
						}
					}
				
				}
				
				sctk_free(layout);
			}
			else
			{
				/* Retrieve the number of block in the datatype */
				count = target_type->count;

				while( !done )
				{
					
					/* Count the number of elements by substracting
					* individual blocks from total size until reaching 0 */
					for(i = 0; i < count; i++)
					{
						size -= target_type->ends[i] - target_type->begins[i] + 1;
						
						(*elements)++;

						if(size <= 0)
						{
							done = 1;
							break;
						}
					}
				}
			}
		break;
		
		case MPC_DATATYPES_COMMON:
			/* This is the size we received */
			size = status->size;
			
			sctk_nodebug ("Normal type : count %d, data_type %d (size %d)", size, datatype, data_size);
			
			/* Check if we have an exact number of object */
			if (size % data_size == 0)
			{
				/* If so elements can be directly computed */
				size = size / data_size;
				*elements = size;
			}
			else
			{
				/* Here we can say nothing as the size is not
				 * a multiple of the data-type size */ 
				*elements = MPI_UNDEFINED;
			}
			break;
		
		default:
			not_reachable();
	}


	return res;
}


int
__INTERNAL__PMPI_Pack (void *inbuf,
		       int incount,
		       MPI_Datatype datatype,
		       void *outbuf, int outcount, int *position,
		       MPI_Comm comm)
{
	int copied = 0;
	if (sctk_datatype_is_derived (datatype))
	{
		unsigned long i;
		int j;
		unsigned long extent;

		__INTERNAL__PMPI_Type_extent (datatype, (MPI_Aint *) & extent);

		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;
		
		MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);

		for (j = 0; j < incount; j++)
		{
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				unsigned long size;
				size = input_datatype.opt_ends[i] - input_datatype.opt_begins[i] + 1;
				
				sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu", input_datatype.opt_begins[i] + extent * j, input_datatype.opt_ends[i] + extent * j, *position, size);
				
				if (size != 0)
				{
					memcpy (&(((char *) outbuf)[*position]), &(((char *) inbuf)[input_datatype.opt_begins[i]]), size);
				}
				
				copied += size;
				sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu done", input_datatype.opt_begins[i] + extent * j, input_datatype.opt_ends[i] + extent * j, *position, size);

				
				*position = *position + size;
				
				assume (copied <= outcount);
			}
			
			inbuf = (char *) inbuf + extent;
		}

		sctk_nodebug ("%lu <= %lu", copied, outcount);
		assume (copied <= outcount);
		return MPI_SUCCESS;
	}
	else
	{
		size_t size;

		if(inbuf == NULL){
			return MPI_ERR_BUFFER;
		}

		if(outbuf == NULL){
			return MPI_ERR_BUFFER;
		}

		PMPC_Type_size (datatype, &size);
		sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu", 0, size * incount, *position, size * incount);
		memcpy (&(((char *) outbuf)[*position]), inbuf, size * incount);
		sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu done", 0, size * incount, *position, size * incount);
		*position = *position + size * incount;
		copied += size * incount;
		sctk_nodebug ("%lu == %lu", copied, outcount);
		assume (copied <= outcount);
		
		return MPI_SUCCESS;
	}
}

int
__INTERNAL__PMPI_Unpack (void *inbuf,
			 int insize,
			 int *position,
			 void *outbuf, int outcount, MPI_Datatype datatype,
			 MPI_Comm comm)
{
	int copied = 0;
	sctk_nodebug("MPI_Unpack insise = %d, position = %d, outcount = %d, datatype = %d, comm = %d", insize, *position, outcount, datatype, comm);
	if (sctk_datatype_is_derived (datatype))
	{
		unsigned long count_out;
		unsigned long i;
		int j;
		unsigned long extent;

		__INTERNAL__PMPI_Type_extent (datatype, (MPI_Aint *) & extent);

		int derived_ret = 0;
		sctk_derived_datatype_t output_datatype;

		MPC_Is_derived_datatype (datatype, &derived_ret, &output_datatype);
		for (j = 0; j < outcount; j++)
		{
			for (i = 0; i < output_datatype.opt_count; i++)
			{
				size_t size;
				size = output_datatype.opt_ends[i] - output_datatype.opt_begins[i] + 1;
				copied += size;
				sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu", *position, size, output_datatype.opt_begins[i] + extent * j, output_datatype.ends[i] + extent * j);
				if (size != 0)
				{
					memcpy (&(((char *) outbuf)[output_datatype.opt_begins[i]]), &(((char *) inbuf)[*position]), size);
				}
				sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu done", *position, size, output_datatype.opt_begins[i] + extent * j, output_datatype.opt_ends[i] + extent * j);
				*position = *position + size;
				/* done */
				assume (copied <= insize);
			}
			outbuf = (char *) outbuf + extent;
		}

		sctk_nodebug ("%lu <= %lu", copied, insize);
		assume (copied <= insize);
		return MPI_SUCCESS;
	}
	else
	{
		size_t size;
		PMPC_Type_size (datatype, &size);
		sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu", *position, size * outcount, 0, size * outcount);
		memcpy (outbuf, &(((char *) inbuf)[*position]), size * outcount);
		sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu done", *position, size * outcount, 0, size * outcount);
		*position = *position + size * outcount;
		copied += size * outcount;
		assume (copied <= insize);
		return MPI_SUCCESS;
	}
}

int
__INTERNAL__PMPI_Pack_size (int incount, MPI_Datatype datatype, MPI_Comm comm,
			    int *size)
{
	*size = 0;
	sctk_nodebug ("incount size %d", incount);
	if (sctk_datatype_is_derived (datatype))
	{
		int res;
		mpc_pack_absolute_indexes_t *begins_in;
		mpc_pack_absolute_indexes_t *ends_in;
		unsigned long count_in;
		unsigned long i;
		mpc_pack_absolute_indexes_t lb;
		int is_lb;
		mpc_pack_absolute_indexes_t ub;
		int is_ub;
		int j;

		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;
		
		MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
		
		sctk_nodebug("derived datatype : res %d, count_in %d, lb %d, is_lb %d, ub %d, is_ub %d", derived_ret, input_datatype.count, input_datatype.lb, input_datatype.is_lb, input_datatype.ub, input_datatype.is_ub);
		
		for (j = 0; j < incount; j++)
		{
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				size_t sizet;
				sizet = input_datatype.opt_ends[i] - input_datatype.opt_begins[i] + 1;
				*size = *size + sizet;
				sctk_nodebug ("PACK derived part size %d, %lu-%lu", sizet,
				input_datatype.opt_begins[i], input_datatype.opt_ends[i]);
			}
		}
		
		sctk_nodebug ("PACK derived final size %d", *size);
		return MPI_SUCCESS;
	}
	else
	{
		size_t size_t;
		PMPC_Type_size (datatype, &size_t);
		*size = size_t * incount;
		sctk_nodebug ("PACK contiguous size %d", *size);
		return MPI_SUCCESS;
	}
}


int __INTERNAL__PMPI_Pack_external_size (char *datarep , int incount, MPI_Datatype datatype, MPI_Aint *size)
{
	if( strcmp( datarep, "external32" ) )
	{
		sctk_warning("MPI_Pack_external_size: unsuported data-rep %s", datarep);
		return MPI_ERR_ARG;
	}

	sctk_task_specific_t *task_specific;

	/* Now generate the final size according to the internal type
	 * derived or contiguous one */
	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_CONTIGUOUS:
			task_specific = __MPC_get_task_specific ();
			sctk_datatype_lock( task_specific );
			sctk_contiguous_datatype_t *contiguous_user_types = sctk_task_specific_get_contiguous_datatype( task_specific, datatype );
			sctk_datatype_unlock( task_specific );
			
			/* For contiguous it is count times the external extent */
			if( sctk_datatype_is_common( contiguous_user_types->datatype ) )
			{
				*size = MPC_Extern32_common_type_size( contiguous_user_types->datatype ) * contiguous_user_types->count * incount;
			}
			else
			{
				/* If the internal type is also a contiguous, continue unfolding */
				__INTERNAL__PMPI_Pack_external_size (datarep , contiguous_user_types->count, contiguous_user_types->datatype, size);
				*size = *size * contiguous_user_types->count;
			}
		break;
		
		case MPC_DATATYPES_DERIVED:
			task_specific = __MPC_get_task_specific ();
		
			sctk_datatype_lock( task_specific );
			sctk_derived_datatype_t *derived_user_types = sctk_task_specific_get_derived_datatype( task_specific, datatype );
			sctk_datatype_unlock( task_specific );
		
			int i;
			MPI_Datatype local_type;
			MPI_Aint count;
			MPI_Aint extent;
			
			*size = 0;
			
			/* For derived, we work block by block */
			for( i = 0 ; i < derived_user_types->count ; i++ )
			{
				/* Get type extent */
				__INTERNAL__PMPI_Type_extent (derived_user_types->datatypes[i], &extent);
				
				if( ! extent )
					continue;
				
				/* Compute count */
				count = (derived_user_types->ends[i] - derived_user_types->begins[i] + 1) / extent;
				
				/* Add count times external size */
				*size +=  MPC_Extern32_common_type_size( derived_user_types->datatypes[i] ) * count;
			}
			
			*size = *size * incount;
		break;
		
		case MPC_DATATYPES_COMMON:
			/* For commom we can directly map */
			*size =  MPC_Extern32_common_type_size( datatype ) * incount;
		break;
		
		default:
			sctk_fatal("__INTERNAL__PMPI_Pack_external_size unreachable");
	}

	return MPI_SUCCESS;
}


MPI_Datatype * sctk_datatype_get_typemask( MPI_Datatype datatype, int * type_mask_count, MPC_Datatype * static_type )
{
	sctk_task_specific_t *task_specific;
	
	*type_mask_count = 0;
	
	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			*type_mask_count = 1;
			*static_type = datatype;
			return static_type;
		break;
		case MPC_DATATYPES_CONTIGUOUS:
			task_specific = __MPC_get_task_specific ();
			
			sctk_datatype_lock( task_specific );
			sctk_contiguous_datatype_t *contiguous_user_types = sctk_task_specific_get_contiguous_datatype( task_specific, datatype );
			sctk_datatype_unlock( task_specific );
			
			*type_mask_count = 1;
			
			if( sctk_datatype_is_common(contiguous_user_types->datatype) )
			{
				
				*static_type = contiguous_user_types->datatype;
				return static_type;
			}
			else
			{
				/* We have to continue the unpacking until finding a common type */
				return sctk_datatype_get_typemask( contiguous_user_types->datatype, type_mask_count, static_type );
			}
		break;
		
		case MPC_DATATYPES_DERIVED:
			task_specific = __MPC_get_task_specific ();

			sctk_datatype_lock( task_specific );
			sctk_derived_datatype_t *derived_user_types = sctk_task_specific_get_derived_datatype( task_specific, datatype );
			sctk_datatype_unlock( task_specific );
		
			*type_mask_count = derived_user_types->count;
			return derived_user_types->datatypes;
		break;
		
		default:
			sctk_fatal("Unreachable code in sctk_datatype_get_typemask");
	}
	
	return NULL;
}



int __INTERNAL__PMPI_Pack_external (char *datarep , void *inbuf, int incount, MPI_Datatype datatype, void * outbuf, MPI_Aint outsize, MPI_Aint * position)
{
	if( !strcmp( datarep, "external32" ) )
	{
		int pack_size = 0;
		MPI_Aint ext_pack_size = 0;
		MPI_Pack_external_size ( datarep , incount, datatype, &ext_pack_size);
		MPI_Pack_size( incount, datatype, MPI_COMM_WORLD, &pack_size );

		int pos = 0;
		/* MPI_Pack takes an integer output size */
		int int_outsize = pack_size;
		
		char * native_pack_buff = sctk_malloc( pack_size );
		memset( native_pack_buff, 0 , pack_size );
		assume( native_pack_buff != NULL );
		
		/* Just pack */
		PMPI_Pack(inbuf, incount,  datatype, native_pack_buff, int_outsize, &pos, MPI_COMM_WORLD);

		*position += ext_pack_size;

		/* We now have a contiguous vector gathering data-types
		 * Now apply the conversion first by extracting the datatype vector
		 * and then by converting */

		int type_vector_count = 0;
		MPI_Datatype * type_vector = NULL;
		MPC_Datatype static_type;
		
		/* Now extract the type mask */
		type_vector = sctk_datatype_get_typemask( datatype, &type_vector_count, &static_type );
		
		/* And now apply the encoding */
		MPC_Extern32_convert( type_vector ,
					type_vector_count,
					native_pack_buff, 
					pack_size, 
					outbuf, 
					ext_pack_size , 
					1);
		
		sctk_free( native_pack_buff );
	}
	else
	{
		sctk_warning("MPI_Pack_external: MPI_Pack_external: No such representation %s", datarep );
		return MPI_ERR_ARG;
	}
	
	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Unpack_external (char * datarep, void * inbuf, MPI_Aint insize, MPI_Aint * position, void * outbuf, int outcount, MPI_Datatype datatype)
{
	if( !strcmp( datarep, "external32" ) )
	{
		int pack_size = 0;
		MPI_Aint ext_pack_size = 0;
		MPI_Pack_external_size ( datarep , outcount, datatype, &ext_pack_size);
		MPI_Pack_size( outcount, datatype, MPI_COMM_WORLD, &pack_size );
		
		char * native_pack_buff = sctk_malloc( pack_size );
		memset( native_pack_buff, 0 , pack_size );
		assume( native_pack_buff != NULL );
		
		/* We start with a contiguous vector gathering data-types
		 * first extracting the datatype vector
		 * and then by converting the key is that the endiannes conversion
		 * is a symetrical one*/

		int type_vector_count = 0;
		MPI_Datatype * type_vector = NULL;
		MPC_Datatype static_type;

		/* Now extract the type mask */
		type_vector = sctk_datatype_get_typemask( datatype, &type_vector_count, &static_type );
		
		/* And now apply the encoding */
		MPC_Extern32_convert( type_vector ,
					type_vector_count,
					native_pack_buff, 
					pack_size, 
					inbuf, 
					ext_pack_size , 
					0);
		
		/* Now we just have to unpack the converted content */
		int pos = 0;
		PMPI_Unpack (native_pack_buff, insize, &pos, outbuf, outcount, datatype, MPI_COMM_WORLD);
		
		*position = pos;

		sctk_free( native_pack_buff );
	}
	else
	{
		sctk_warning("MPI_Unpack_external : No such representation %s", datarep );
		return MPI_ERR_ARG;
	}
	
	return MPI_SUCCESS;
}


#define MPI_MAX_CONCURENT 128

/* Function pointer for user collectives */
int (*barrier_intra)(MPI_Comm);
int (*barrier_intra_shm)(MPI_Comm);
int (*barrier_intra_shared_node)(MPI_Comm);
int (*barrier_inter)(MPI_Comm);

int (*bcast_intra_shm)(void *, int, MPI_Datatype, int, MPI_Comm);
int (*bcast_intra_shared_node)(void *, int, MPI_Datatype, int, MPI_Comm);
int (*bcast_intra)(void *, int, MPI_Datatype, int, MPI_Comm);
int (*bcast_inter)(void *, int, MPI_Datatype, int, MPI_Comm);

int (*gather_intra)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int (*gather_inter)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

int (*gatherv_intra)(void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, int, MPI_Comm);
int (*gatherv_intra_shm)(void *, int, MPI_Datatype, void *, int *, int *,
                         MPI_Datatype, int, MPI_Comm);
int (*gatherv_inter)(void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, int, MPI_Comm);

int (*scatter_intra)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int (*scatter_intra_shared_node)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int (*scatter_inter)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

int (*scatterv_intra)(void *, int *, int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int (*scatterv_intra_shm)(void *, int *, int *, MPI_Datatype, void *, int,
                          MPI_Datatype, int, MPI_Comm);
int (*scatterv_inter)(void *, int *, int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

int (*allgather_intra)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int (*allgather_inter)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);

int (*allgatherv_intra)(void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int (*allgatherv_inter)(void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);

int (*alltoall_inter)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int (*alltoall_intra)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int (*alltoall_intra_shared_node)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);

int (*alltoallv_intra)(void *, int *, int *, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int (*alltoallv_intra_shm)(void *, int *, int *, MPI_Datatype, void *, int *,
                           int *, MPI_Datatype, MPI_Comm);
int (*alltoallv_inter)(void *, int *, int *, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);

int (*alltoallw_intra)(void *, int *, int *, MPI_Datatype *, void *, int *, int *, MPI_Datatype *, MPI_Comm);
int (*alltoallw_inter)(void *, int *, int *, MPI_Datatype *, void *, int *, int *, MPI_Datatype *, MPI_Comm);

int (*reduce_intra)(void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
int (*reduce_intra_shm)(void *, void *, int, MPI_Datatype, MPI_Op, int,
                        MPI_Comm);
int (*reduce_inter)(void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);

int (*allreduce_intra)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int (*allreduce_inter)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

int (*reduce_scatter_intra)(void *, void *, int *, MPI_Datatype, MPI_Op, MPI_Comm);
int (*reduce_scatter_inter)(void *, void *, int *, MPI_Datatype, MPI_Op, MPI_Comm);

int (*reduce_scatter_block_intra)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int (*reduce_scatter_block_inter)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

int (*scan_intra)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

int (*exscan_intra)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);


/* Collectives */
int __MPC_poll_progress();

int __INTERNAL__PMPI_Barrier_intra_shm_sig(MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
  struct shared_mem_barrier_sig *barrier_ctx = &coll->shm_barrier_sig;

  // sctk_error("BARRIER CTX : %p", barrier_ctx	);

  if (!coll) {
    return MPI_ERR_COMM;
  }

  int rank;
  __INTERNAL__PMPI_Comm_rank(comm, &rank);

  volatile int the_signal = 0;

  int *volatile toll = &barrier_ctx->tollgate[rank];

  int cnt = 0;

  if (__do_yield) {
    while (*toll != sctk_atomics_load_int(&barrier_ctx->fare)) {
      sctk_thread_yield();
      if( (cnt++ && 0xFF) == 0 )
        __MPC_poll_progress();
    }
  } else {
    while (*toll != sctk_atomics_load_int(&barrier_ctx->fare)) {
      sctk_cpu_relax();
      if( (cnt++ && 0xFF) == 0 )
        __MPC_poll_progress();
    }
  }

  /* I Own the cell */
  sctk_atomics_store_ptr(&barrier_ctx->sig_points[rank], (void *)&the_signal);

  /* Next time we expect the opposite */
  *toll = !*toll;

  if (sctk_atomics_fetch_and_decr_int(&barrier_ctx->counter) == 1) {
    /* The last task */
    int size = coll->comm_size;

    /* Reset counter */
    sctk_atomics_store_int(&barrier_ctx->counter, size);

    /* Free others */
    int i;
    for (i = 0; i < size; i++) {
      int *sig = sctk_atomics_load_ptr(&barrier_ctx->sig_points[i]);
      *sig = 1;
    }

    /* Reverse the Fare */
    int current_fare = sctk_atomics_load_int(&barrier_ctx->fare);
    sctk_atomics_store_int(&barrier_ctx->fare, !current_fare);

  } else {
    if (__do_yield) {
      while (the_signal == 0) {
        sctk_thread_yield();
        if( (cnt++ && 0xFF) == 0 )
            __MPC_poll_progress();
      }
    } else {
      while (the_signal == 0) {
        sctk_cpu_relax();
        if( (cnt++ && 0xFF) == 0 )
            __MPC_poll_progress();
      }
    }
  }

  return MPI_SUCCESS;
}



int __INTERNAL__PMPI_Barrier_intra_shm_on_ctx(struct shared_mem_barrier *barrier_ctx,
                                              int comm_size) {

  int my_phase = !sctk_atomics_load_int(&barrier_ctx->phase);

  if (sctk_atomics_fetch_and_decr_int(&barrier_ctx->counter) == 1) {
    sctk_atomics_store_int(&barrier_ctx->counter, comm_size);
    sctk_atomics_store_int(&barrier_ctx->phase, my_phase);
  } else {
    if (__do_yield ) {
      while (sctk_atomics_load_int(&barrier_ctx->phase) != my_phase) {
		sched_yield();
	  }
    } else {
      while (sctk_atomics_load_int(&barrier_ctx->phase) != my_phase) {
      	sctk_cpu_relax();
	  }
    }
  }

  return MPI_SUCCESS;
}





int __INTERNAL__PMPI_Barrier_intra_shm(MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
  struct shared_mem_barrier *barrier_ctx = &coll->shm_barrier;

  if (!coll) {
    return MPI_ERR_COMM;
  }

  return __INTERNAL__PMPI_Barrier_intra_shm_on_ctx( barrier_ctx, coll->comm_size );
}


int __MPC_init_node_comm_ctx( struct sctk_comm_coll * coll, MPI_Comm comm )
{
    int is_shared = 0;

    int rank = 0;
    void* tmp_ctx;

    PMPI_Comm_size( comm, &coll->comm_size );
    PMPI_Comm_rank( comm, &rank );


    if( !rank )
    {
        tmp_ctx = mpc_MPI_allocmem_pool_alloc_check(sizeof(struct sctk_comm_coll),
                &is_shared);

        if( is_shared == 0 )
        {
            sctk_free( tmp_ctx);
            tmp_ctx = NULL;
        }

        if( !tmp_ctx )
        {
            tmp_ctx = 0x1;
        }
        else
        {
            sctk_per_node_comm_context_init( tmp_ctx, comm,  coll->comm_size );
        }


	sctk_assert(tmp_ctx != NULL);
        sctk_broadcast(  &tmp_ctx, sizeof( void * ), 0, comm );
	sctk_barrier(comm);
	coll->node_ctx = tmp_ctx;
    }
    else
    {
        sctk_broadcast(  &tmp_ctx, sizeof( void * ), 0, comm );
	sctk_barrier(comm);
	coll->node_ctx = tmp_ctx;
    }

    return MPI_SUCCESS;
}



static inline __MPC_node_comm_coll_check(  struct sctk_comm_coll *coll , MPI_Comm comm )
{
        if( coll->node_ctx == 0x1 )
        {
                /* A previous alloc failed */
                return 0;
        }

        if( coll->node_ctx == NULL )
        {
                __MPC_init_node_comm_ctx( coll, comm );
        }

        return 1;
}


int __INTERNAL__PMPI_Barrier_intra_shared_node(MPI_Comm comm) {

        struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);

        if( __MPC_node_comm_coll_check( coll, comm ) )
        {
                return __INTERNAL__PMPI_Barrier_intra_shm_on_ctx(&coll->node_ctx->shm_barrier,
                                              coll->comm_size);

        }
        else
        {
                return (barrier_intra)( comm );
        }
}





#define FOR_MPI_BARRIER_STATIC_REQ 32

static inline int __INTERNAL__PMPI_Barrier_intra_for(MPI_Comm comm, int size) {
  int i, res = MPI_ERR_INTERN, rank;

  if (size == 1)
    MPI_ERROR_SUCESS();

  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  /* All non-root send & receive zero-length message. */
  if (rank > 0) {
    res = __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, 0, MPC_BARRIER_TAG, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }

    res = __INTERNAL__PMPI_Recv(NULL, 0, MPI_BYTE, 0, MPC_BARRIER_TAG, comm,
                                MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  /* The root collects and broadcasts the messages. */
  else {

	if( (size - 1) < FOR_MPI_BARRIER_STATIC_REQ )
	{
		MPI_Request reqs[FOR_MPI_BARRIER_STATIC_REQ];

		for (i = 1; i < size; ++i) {
    	  res = __INTERNAL__PMPI_Irecv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE,
    	                              MPC_BARRIER_TAG, comm, &reqs[i-1] );
    	  if (res != MPI_SUCCESS) {
    	    return res;
    	  }
    	}

		res = __INTERNAL__PMPI_Waitall( size -1 , reqs, MPI_STATUSES_IGNORE );

    	if (res != MPI_SUCCESS) {
    	    return res;
   	    }

		for (i = 1; i < size; ++i) {
    	  res = __INTERNAL__PMPI_Isend(NULL, 0, MPI_BYTE, i, MPC_BARRIER_TAG, comm, &reqs[i-1]);
    	  if (res != MPI_SUCCESS) {
    	    return res;
    	  }
    	}

		res = __INTERNAL__PMPI_Waitall( size -1 , reqs, MPI_STATUSES_IGNORE );

    	if (res != MPI_SUCCESS) {
    	    return res;
   	    }

	}
	else
	{

    	for (i = 1; i < size; ++i) {
    	  res = __INTERNAL__PMPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE,
    	                              MPC_BARRIER_TAG, comm, MPI_STATUS_IGNORE);
    	  if (res != MPI_SUCCESS) {
    	    return res;
    	  }
    	}

    	for (i = 1; i < size; ++i) {
    	  res = __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, i, MPC_BARRIER_TAG, comm);
    	  if (res != MPI_SUCCESS) {
    	    return res;
    	  }
    	}

	}
  }
  MPI_ERROR_SUCESS();
}

static inline int __INTERNAL__PMPI_Barrier_btree_mpi(MPI_Comm comm, int size) {
  int i, res = MPI_ERR_INTERN, rank;


  if (size == 1)
    MPI_ERROR_SUCESS();

  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  int parent = -1;

  if (rank)
    parent = ((rank + 1) / 2) - 1;

  int left_child = ((rank + 1) * 2) - 1;
  int right_child = ((rank + 1) * 2);

  if (size <= left_child) {
    left_child = -1;
  }

  if (size <= right_child) {
    right_child = -1;
  }

  // sctk_error("%d P %d LC %d RC %d", rank, parent, left_child, right_child );

  /* To Child */

  if (parent != -1) {
    res = __INTERNAL__PMPI_Recv(NULL, 0, MPI_BYTE, parent, MPC_BARRIER_TAG,
                                comm, MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (left_child != -1) {
    res = __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, left_child, MPC_BARRIER_TAG,
                                comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (right_child != -1) {
    res = __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, right_child, MPC_BARRIER_TAG,
                                comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  /* From Child */

  if (left_child != -1) {
    res = __INTERNAL__PMPI_Recv(NULL, 0, MPI_BYTE, left_child, MPC_BARRIER_TAG,
                                comm, MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (right_child != -1) {
    res = __INTERNAL__PMPI_Recv(NULL, 0, MPI_BYTE, right_child, MPC_BARRIER_TAG,
                                comm, MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (parent != -1) {
    res =
        __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, parent, MPC_BARRIER_TAG, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  /* To Child */

  if (parent != -1) {
    res = __INTERNAL__PMPI_Recv(NULL, 0, MPI_BYTE, parent, MPC_BARRIER_TAG,
                                comm, MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (left_child != -1) {
    res = __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, left_child, MPC_BARRIER_TAG,
                                comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (right_child != -1) {
    res = __INTERNAL__PMPI_Send(NULL, 0, MPI_BYTE, right_child, MPC_BARRIER_TAG,
                                comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  MPI_ERROR_SUCESS();
}


int __INTERNAL__PMPI_Barrier_intra(MPI_Comm comm) {

	int size, res;
	__INTERNAL__PMPI_Comm_size( comm, &size );

	if( size < sctk_runtime_config_get()->modules.collectives_intra.barrier_intra_for_trsh )
	{
		res = __INTERNAL__PMPI_Barrier_intra_for( comm, size );
	}
	else
	{
		res = __INTERNAL__PMPI_Barrier_btree_mpi( comm , size);
	}

	return res;
}




int __INTERNAL__PMPI_Barrier_inter(MPI_Comm comm) {
  int root = 0, buf = 0, rank, size;
  int res = MPI_ERR_INTERN;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  /* Barrier on local intracomm */
  if (size > 1) {
    res = __INTERNAL__PMPI_Barrier(sctk_get_local_comm_id(comm));
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  /* Broadcasts between local and remote groups */
  if (sctk_is_in_local_group(comm)) {
    root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
    res = __INTERNAL__PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
    root = 0;
    res = __INTERNAL__PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    root = 0;
    res = __INTERNAL__PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
    root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
    res = __INTERNAL__PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return MPI_SUCCESS;
}

static inline int __INTERNAL__PMPI_Barrier(MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (barrier_inter == NULL) {
      barrier_inter = (int (*)(MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.barrier_inter.value);
    }

    res = (barrier_inter)(comm);

  } else {
    /* Intracomm */

    if (barrier_intra_shm == NULL) {
      barrier_intra_shm = (int (*)(MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm.barrier_intra_shm.value);
    }

    if (barrier_intra_shared_node == NULL) {
      barrier_intra_shared_node = (int (*)(MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm_shared.barrier_intra_shared_node.value);
    }

    if (barrier_intra == NULL) {
      barrier_intra = (int (*)(MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.barrier_intra.value);
    }

    if (sctk_is_shared_mem(comm) ) {
      /* Here only work in shared-mem */
      res = (barrier_intra_shm)(comm);
    } else if( sctk_is_shared_node( comm ) ) {
      
      res = (barrier_intra_shared_node)( comm );
    }
    else {
      /* Use a full net barrier */
      res = (barrier_intra)(comm);
    }
  }

  return res;
}


int __INTERNAL__PMPI_Bcast_inter(void *buffer, int count, MPI_Datatype datatype,
                                 int root, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  MPC_Status status;
  int rank;

  if (root == MPI_PROC_NULL) {
    MPI_ERROR_SUCESS();
  } else if (root == MPC_ROOT) {
    /* root send to remote group leader */
    res = __INTERNAL__PMPI_Send(buffer, count, datatype, 0, MPC_BROADCAST_TAG,
                                comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
    if (res != MPI_SUCCESS) {
      return res;
    }

    if (rank == 0) {
      /* local leader recv from remote group leader */
      res = __INTERNAL__PMPI_Recv(buffer, count, datatype, root,
                                  MPC_BROADCAST_TAG, comm, &status);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
    /* Intracomm broadcast */
    res = __INTERNAL__PMPI_Bcast(buffer, count, datatype, 0,
                                 sctk_get_local_comm_id(comm));
    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  return res;
}

int __INTERNAL__PMPI_Bcast_intra_shm(void *buffer, int count,
                                     MPI_Datatype datatype, int root,
                                     MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);

  int rank, res;
  PMPI_Comm_rank(comm, &rank);

  struct shared_mem_bcast *bcast_ctx =sctk_comm_coll_get_bcast(coll, rank);


  /* First pay the toll gate */
  if (__do_yield) {
    while (bcast_ctx->tollgate[rank] !=
           sctk_atomics_load_int(&bcast_ctx->fare)) {
      sctk_thread_yield();
    }

  } else {
    while (bcast_ctx->tollgate[rank] !=
           sctk_atomics_load_int(&bcast_ctx->fare)) {
      sctk_cpu_relax();
    }
  }

  /* Reverse state so that only a root done can unlock by
   * also reversing the fare */
  bcast_ctx->tollgate[rank] = !bcast_ctx->tollgate[rank];

  void *data_buff = buffer;
  MPI_Aint tsize = 0;
  res = __INTERNAL__PMPI_Type_extent(datatype, &tsize);
  if (res != MPI_SUCCESS) {
    return res;
  }

  int is_shared_mem_buffer = sctk_mpi_type_is_shared_mem(datatype, count);
  int is_contig_type = sctk_datatype_contig_mem(datatype);

  /* Now am I the root ? */
  if (root == rank) {
    if (__do_yield) {

      while (sctk_atomics_cas_int(&bcast_ctx->owner, -1, -2) != -1) {
        sctk_thread_yield();
      }

    } else {

      while (sctk_atomics_cas_int(&bcast_ctx->owner, -1, -2) != -1) {
        sctk_cpu_relax();
      }
    }

    bcast_ctx->root_in_buff = 0;

    /* Does root need to pack ? */
    if (!is_contig_type) {
      /* We have a tmp bufer where to reduce */
      data_buff = sctk_malloc(count * tsize);

      assume(data_buff != NULL);

      /* If non-contig, we need to pack to the TMP buffer
       * where the reduction will be operated */
      int cnt = 0;
      PMPI_Pack(buffer, count, datatype, data_buff, tsize * count, &cnt, comm);

      /* We had to allocate the segment save it for release by the last */
      sctk_atomics_store_ptr(&bcast_ctx->to_free, data_buff);

      /* Set pack as reference */
      bcast_ctx->target_buff = data_buff;

    } else {
      /* Set the ref buffer */
      bcast_ctx->target_buff = data_buff;

      /* Can we use the SHM buffer ? */

      if (is_shared_mem_buffer) {
        /* Set my value in the TMP buffer */
        sctk_mpi_shared_mem_buffer_fill(&bcast_ctx->buffer, datatype, count,
                                        data_buff);
        bcast_ctx->root_in_buff = 1;
      }
    }

    /* Save source type infos */
    bcast_ctx->stype_size = tsize;
    bcast_ctx->scount = count;

    /* Now unleash the others */
    sctk_atomics_store_int(&bcast_ctx->owner, rank);

  } else {
    /* Wait for the root */
    if (__do_yield) {
      while (sctk_atomics_load_int(&bcast_ctx->owner) != root) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_load_int(&bcast_ctx->owner) != root) {
        sctk_cpu_relax();
      }
    }
  }

  /* If we are here the root has set its data */
  if (rank != root) {

    /* We are in the TMB buffers */
    if (bcast_ctx->root_in_buff) {
      sctk_mpi_shared_mem_buffer_get(&bcast_ctx->buffer, datatype, count,
                                     buffer,
                                     bcast_ctx->scount * bcast_ctx->stype_size);
    } else {
      void *src = bcast_ctx->target_buff;

      /* Datatype has to be unpacked */
      if (!is_contig_type) {
        /* If non-contig, we need to unpack to the final buffer */
        int cnt = 0;
        MPI_Unpack(src, bcast_ctx->scount * bcast_ctx->stype_size, &cnt, buffer,
                   count, datatype, comm);
      } else {
        /* Yes ! this type is contiguous */
        memcpy(buffer, src, tsize * count);
      }
    }
  }

  /* Now leave the pending list and if I am the last I free */

  if (bcast_ctx->root_in_buff) {
    if (sctk_atomics_fetch_and_decr_int(&bcast_ctx->left_to_pop) == 1) {
      goto SHM_BCAST_RELEASE;
    }
  } else {
    /* Sorry rank 0 we have to make sure that the root stays here if we are
 * not using the async buffers */

    sctk_atomics_decr_int(&bcast_ctx->left_to_pop);

    if (rank == root) {
      /* Wait for everybody */

      if (__do_yield) {
        while (sctk_atomics_load_int(&bcast_ctx->left_to_pop) != 0) {
          sctk_thread_yield();
        }
      } else {
        while (sctk_atomics_load_int(&bcast_ctx->left_to_pop) != 0) {
          sctk_cpu_relax();
        }
      }

      goto SHM_BCAST_RELEASE;
    }
  }

  return MPI_SUCCESS;

SHM_BCAST_RELEASE : {
  void *to_free = sctk_atomics_load_ptr(&bcast_ctx->to_free);

  if (to_free) {
    sctk_atomics_store_ptr(&bcast_ctx->to_free, 0);
    sctk_free(to_free);
  }

  /* Set the counter */
  sctk_atomics_store_int(&bcast_ctx->left_to_pop, coll->comm_size);

  sctk_atomics_store_int(&bcast_ctx->owner, -1);

  int current_fare = sctk_atomics_load_int(&bcast_ctx->fare);
  sctk_atomics_store_int(&bcast_ctx->fare, !current_fare);

  return MPI_SUCCESS;
}
}


struct shared_node_coll_data
{
        void * buffer_addr;
        char is_counter;
};


int __INTERNAL__PMPI_Bcast_intra_shared_node_impl(void *buffer, int count, MPI_Datatype datatype,
        int root, MPI_Comm comm, struct sctk_comm_coll *coll ) {
    int rank;
    PMPI_Comm_rank( comm, &rank );

    int tsize;
    PMPI_Type_size( datatype, &tsize );

    size_t to_bcast_size = tsize * count;

    struct shared_node_coll_data cdata;
    cdata.is_counter = 0;

    if( rank == root )
    {
        if( !_mpc_MPI_allocmem_is_in_pool(buffer) )
        {

            int is_shared = 0;
            cdata.buffer_addr = mpc_MPI_allocmem_pool_alloc_check( to_bcast_size + sizeof(sctk_atomics_int),
                    &is_shared);

            if( !is_shared )
            {
                /* We failed ! */
                sctk_free( cdata.buffer_addr );
                cdata.buffer_addr = NULL;
            }
            else
            {
                /* Fill the buffer */
                sctk_atomics_store_int( (sctk_atomics_int*)cdata.buffer_addr, coll->comm_size );
                memcpy( cdata.buffer_addr + sizeof(sctk_atomics_int), buffer, to_bcast_size );
                cdata.is_counter = 1;
            }
        }
        else
        {
            cdata.buffer_addr = buffer;
        }
    }

    (bcast_intra)( &cdata.buffer_addr, sizeof(struct shared_node_coll_data), MPI_CHAR, root, comm );

    if( cdata.buffer_addr != NULL )
    {
        if( rank != root )
        {
            
            if( cdata.is_counter )
            {
                
                memcpy( buffer,  cdata.buffer_addr + sizeof(sctk_atomics_int), to_bcast_size );
                
                int token = sctk_atomics_fetch_and_decr_int( (sctk_atomics_int *)cdata.buffer_addr );
                if( token == 2 )
                {
                        mpc_MPI_allocmem_pool_free_size( cdata.buffer_addr ,  to_bcast_size + sizeof(sctk_atomics_int));
                }
            }
            else
            {
                memcpy( buffer,  cdata.buffer_addr , to_bcast_size );
            }
        }
    }
    else
    {
        /* Fallback to regular bcast */
        return (bcast_intra)( buffer, count, datatype, root, comm );
    }

    return MPI_SUCCESS;
}


int __INTERNAL__PMPI_Bcast_intra_shared_node(void *buffer, int count, MPI_Datatype datatype,
    int root, MPI_Comm comm) {

    struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
    //TODO to expose as config vars
    if( __MPC_node_comm_coll_check( coll, comm ) 
    && ( (4 <= coll->comm_size) || (1024 < count) )  )
    {
        return __INTERNAL__PMPI_Bcast_intra_shared_node_impl( buffer, count, datatype, root, comm, coll );
    }
    else
    {
        return (bcast_intra)( buffer, count, datatype, root, comm );
    }
}


int __INTERNAL__PMPI_Bcast_intra(void *buffer, int count, MPI_Datatype datatype,
		int root, MPI_Comm comm) {

	int res = MPI_ERR_INTERN, size, rank;

	res = __INTERNAL__PMPI_Comm_size(comm, &size);
	if (res != MPI_SUCCESS) {
		return res;
	}
	res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
	if (res != MPI_SUCCESS) {
		return res;
	}

  MPI_Aint tsize;
  res = __INTERNAL__PMPI_Type_extent(datatype, &tsize);
  if (res != MPI_SUCCESS) {
    return res;
  }
  int derived_ret = 0;
  int vec_size;
  sctk_derived_datatype_t input_datatype;
  if(sctk_datatype_is_derived(datatype))  {
    MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
    vec_size = input_datatype.size*count;
  }
  else {
    vec_size = tsize*count;
  }

	if( (size < sctk_runtime_config_get()->modules.collectives_intra.bcast_intra_for_trsh)
	&&  (vec_size < sctk_runtime_config_get()->modules.collectives_intra.bcast_intra_for_count_trsh*4) )
	{
		int i,j;
		MPI_Request req_recv;
		MPI_Request *reqs_send;


		reqs_send = sctk_malloc(size * sizeof(MPI_Request));

		if (rank != root) {
			res = __INTERNAL__PMPI_Irecv(buffer, count, datatype, root,
					MPC_BROADCAST_TAG, comm, &req_recv);
			if (res != MPI_SUCCESS) {
				sctk_free(reqs_send);
				return res;
			}
			res = __INTERNAL__PMPI_Wait(&(req_recv), MPI_STATUS_IGNORE);
			if (res != MPI_SUCCESS) {
				sctk_free(reqs_send);
				return res;
			}
		} else {
			for (i = 0, j = 0; i < size; i++) {
				if (i == rank)
					continue;

				res = __INTERNAL__PMPI_Isend(buffer, count, datatype, i,
						MPC_BROADCAST_TAG, comm, &(reqs_send[j]));
				if (res != MPI_SUCCESS) {
					sctk_free(reqs_send);
					return res;
				}
				j++;
			}
			res = __INTERNAL__PMPI_Waitall(j, reqs_send, MPI_STATUSES_IGNORE);
			if (res != MPI_SUCCESS) {
				sctk_free(reqs_send);
				return res;
			}
		}

		sctk_free(reqs_send);

	}
	else
	{
		/* Btree disemination */

		/* Normalize */
		if( root != 0 )
		{
			if( rank == 0 )
			{
				res = __INTERNAL__PMPI_Recv( buffer , count , datatype , root , MPC_BROADCAST_TAG , comm , MPI_STATUS_IGNORE );	
			
				if (res != MPI_SUCCESS) {
					return res;
				}
			}
		
			if( rank == root )
			{
				res = __INTERNAL__PMPI_Send( buffer , count , datatype , 0 , MPC_BROADCAST_TAG , comm  );	
			
				if (res != MPI_SUCCESS) {
					return res;
				}
			}
		}

		/* Compute the tree */

		int parent = (rank+1)/2 - 1;
		int lc = (rank + 1) * 2 - 1;
		int rc = (rank + 1) * 2;
	
		if( size <= lc )
		{
			lc = -1;
		}
	
		if( size <= rc )
		{
			rc = -1;
		}

		if( rank == 0 )
		{
			parent = -1;
		}
		
		MPI_Request reqs[2];

		int min_pipeline_blk = 1024;

                /* NOTE : The second algorithm has been disabled as it clearly stresses Infiniband layer with
                 * too much messages to send when large blocks are required to be sent. The ibuf starvation leads
                 * the application to hang with an out of memory.
                 * Multiple solutions to fix it :
                 *  - the first one is to limit the number of messages to send for one MPI call (what we did here)
                 *      It is not the best solution but the fastest to deploy at this right moment.
                 *  - A solution would be to verify if the message protocol can be related to this issue. It seems because
                 *      messages are sent in buffered mode, the starvation occurs. We did not investiguate more on this point.
                 *  - A solution would be to allow IB to free supplementary-allocated ibuf segments (to avoid the bottleneck) but
                 *      such an approach is likelyt to have performance drawbacks.
                 */
#if 0
		if( (count < min_pipeline_blk)
		|| ! sctk_datatype_contig_mem( datatype ) )
		{
#endif

			if( 0 <= parent )
			{
				res = __INTERNAL__PMPI_Recv( buffer , count , datatype , parent , MPC_BROADCAST_TAG , comm , MPI_STATUS_IGNORE );	
				
				if (res != MPI_SUCCESS) {
					return res;
				}
			}


			reqs[0]= MPI_REQUEST_NULL;
			reqs[1]= MPI_REQUEST_NULL;

			if( 0<= lc )
			{
					res = __INTERNAL__PMPI_Isend( buffer , count , datatype , lc , MPC_BROADCAST_TAG , comm , &reqs[0] );	
				
					if (res != MPI_SUCCESS) {
						return res;
					}
		
			}

			if( 0<= rc )
			{
					res = __INTERNAL__PMPI_Isend( buffer , count , datatype , rc , MPC_BROADCAST_TAG , comm , &reqs[1] );	
				
					if (res != MPI_SUCCESS) {
						return res;
					}
		
			}

			res = MPI_Waitall( 2 , reqs , MPI_STATUSES_IGNORE );

#if 0 /* see the comment above */
		}
		else
		{
			int left_to_process = count;
			int current_offset = 0;

			MPI_Aint tsize = 0;
  			res = __INTERNAL__PMPI_Type_extent(datatype, &tsize);
 			
			if (res != MPI_SUCCESS) {
				return res;
 			}

			
			while( left_to_process )
			{
				int count_this_step = count / 16;

				if( count_this_step < 1024 )
				{
					count_this_step = 1024;
				}


				if( left_to_process < min_pipeline_blk )
				{
					count_this_step = left_to_process;
				}

				if( 0 <= parent )
				{
					res = __INTERNAL__PMPI_Recv( buffer + current_offset * tsize , count_this_step , datatype , parent , MPC_BROADCAST_TAG , comm , MPI_STATUS_IGNORE );	
					
					if (res != MPI_SUCCESS) {
						return res;
					}
				}


				reqs[0]= MPI_REQUEST_NULL;
				reqs[1]= MPI_REQUEST_NULL;

				if( 0<= lc )
				{
						res = __INTERNAL__PMPI_Isend( buffer  + current_offset * tsize  , count_this_step , datatype , lc , MPC_BROADCAST_TAG , comm , &reqs[0] );	
					
						if (res != MPI_SUCCESS) {
							return res;
						}
		
				}

				if( 0<= rc )
				{
						res = __INTERNAL__PMPI_Isend( buffer   + current_offset * tsize , count_this_step , datatype , rc , MPC_BROADCAST_TAG , comm , &reqs[1] );	
					
						if (res != MPI_SUCCESS) {
							return res;
						}
		
				}

				res = PMPI_Waitall( 2 , reqs , MPI_STATUSES_IGNORE );


				current_offset += count_this_step;
				left_to_process -= count_this_step;
			}
		}
#endif
	}

	return res;
}




int __INTERNAL__PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype,
                           int root, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (bcast_inter == NULL)
      bcast_inter = (int (*)(void *, int, MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.bcast_inter.value);
    res = bcast_inter(buffer, count, datatype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {

    /* Intracomm */
    int size;
	__INTERNAL__PMPI_Comm_size( comm , &size );

	if( size == 1 )
	{
		/* Nothing to do */
		return MPI_SUCCESS;
	}

    if (bcast_intra_shm == NULL) {
      bcast_intra_shm = (int (*)(void *, int, MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm.bcast_intra_shm.value);
    }


    if (bcast_intra_shared_node == NULL) {
      bcast_intra_shared_node = (int (*)(void *, int, MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm_shared.bcast_intra_shared_node.value);
    }


    if (bcast_intra == NULL) {
      bcast_intra = (int (*)(void *, int, MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.bcast_intra.value);
    }

    if (sctk_is_shared_mem(comm)) {
      /* Here only work in shared-mem */
      res = (bcast_intra_shm)(buffer, count, datatype, root, comm);
    } else if(  sctk_is_shared_node(comm) && sctk_datatype_contig_mem( datatype ) ) {
       
         res = (bcast_intra_shared_node)(buffer, count, datatype, root, comm );
    } else {
      res = (bcast_intra)(buffer, count, datatype, root, comm);
    }
  }
  return res;
}

int __INTERNAL__PMPI_Gather_intra(void *sendbuf, int sendcnt,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  int recvcnt, MPI_Datatype recvtype, int root,
                                  MPI_Comm comm) {
  MPI_Aint dsize;
  int res = MPI_ERR_INTERN, rank, size;
  MPI_Request request;
  MPI_Request *recvrequest;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  recvrequest = (MPI_Request *)sctk_calloc(size, sizeof(MPI_Request));

  if ((sendbuf == MPC_IN_PLACE) && (rank == root)) {
    request = MPI_REQUEST_NULL;
  } else {
    res = __INTERNAL__PMPI_Isend(sendbuf, sendcnt, sendtype, root,
                                 MPC_GATHER_TAG, comm, &request);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }
  }

  if (rank == root) {
    int i = 0;

    res = __INTERNAL__PMPI_Type_extent(recvtype, &dsize);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }

    for (i = 0; i < size; i++) {
      if ((sendbuf == MPI_IN_PLACE) && (i == root))
        recvrequest[i] = MPI_REQUEST_NULL;
      else {
        res = __INTERNAL__PMPI_Irecv(((char *)recvbuf) + (i * recvcnt * dsize),
                                     recvcnt, recvtype, i, MPC_GATHER_TAG, comm,
                                     &(recvrequest[i]));
        if (res != MPI_SUCCESS) {
          sctk_free(recvrequest);
          return res;
        }
      }
    }
    res = __INTERNAL__PMPI_Waitall(size, recvrequest, MPI_STATUSES_IGNORE);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }
  }

  res = __INTERNAL__PMPI_Wait(&(request), MPI_STATUS_IGNORE);
  sctk_free(recvrequest);
  return res;
}

int __INTERNAL__PMPI_Gather_inter(void *sendbuf, int sendcnt,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  int recvcnt, MPI_Datatype recvtype, int root,
                                  MPI_Comm comm) {
  char *ptmp;
  int i, res = MPI_ERR_INTERN, rank, size;
  MPI_Aint incr, extent;

  res = __INTERNAL__PMPI_Comm_remote_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if (MPI_PROC_NULL == root) {
    MPI_ERROR_SUCESS();
  } else if (root != MPI_ROOT) {
    res = __INTERNAL__PMPI_Send(sendbuf, sendcnt, sendtype, root,
                                MPC_GATHER_TAG, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    res = __INTERNAL__PMPI_Type_extent(recvtype, &extent);
    if (res != MPI_SUCCESS) {
      return res;
    }

    incr = extent * recvcnt;
    for (i = 0, ptmp = (char *)recvbuf; i < size; ++i, ptmp += incr) {
      res = __INTERNAL__PMPI_Recv(ptmp, recvcnt, recvtype, i, MPC_GATHER_TAG,
                                  comm, MPI_STATUS_IGNORE);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
  }
  return res;
}

int __INTERNAL__PMPI_Gather(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                            void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                            int root, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (gather_inter == NULL)
      gather_inter = (int (*)(void *, int, MPI_Datatype, void *, int,
                              MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.gather_inter.value);
    res = gather_inter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype,
                       root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */
    if (gatherv_intra_shm == NULL) {
      gatherv_intra_shm =
          (int (*)(void *, int, MPI_Datatype, void *, int *, int *,
                   MPI_Datatype, int, MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_shm.gatherv_intra_shm.value;
    }

    if (gather_intra == NULL) {
      gather_intra =
          (int (*)(void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
                   MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_intra.gather_intra.value;
    }

    if (sctk_is_shared_mem(comm)) {
      res = (gatherv_intra_shm)(sendbuf, sendcnt, sendtype, recvbuf, &recvcnt,
                                NULL, recvtype, root, comm);

    } else {
      res = (gather_intra)(sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
                           recvtype, root, comm);
    }
  }

  return res;
}

int __INTERNAL__PMPI_Gatherv_intra_shm(void *sendbuf, int sendcnt,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int *recvcnts, int *displs,
                                       MPI_Datatype recvtype, int root,
                                       MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
  struct shared_mem_gatherv *gv_ctx = &coll->shm_gatherv;

  int rank, res;
  PMPI_Comm_rank(comm, &rank);

  /* First pay the toll gate */
  if (__do_yield) {
    while (gv_ctx->tollgate[rank] != sctk_atomics_load_int(&gv_ctx->fare)) {
      sctk_thread_yield();
    }

  } else {
    while (gv_ctx->tollgate[rank] != sctk_atomics_load_int(&gv_ctx->fare)) {
      sctk_cpu_relax();
    }
  }

  /* Reverse state so that only a root done can unlock by
   * also reversing the fare */
  gv_ctx->tollgate[rank] = !gv_ctx->tollgate[rank];

  void *data_buff = sendbuf;
  MPI_Aint stsize = 0;
  res = __INTERNAL__PMPI_Type_extent(sendtype, &stsize);
  if (res != MPI_SUCCESS) {
    return res;
  }
  int did_allocate_send = 0;

  gv_ctx->send_type_size[rank] = stsize;

  /* Does root need to pack ? */
  if (!sctk_datatype_contig_mem(sendtype)) {
    /* We have a tmp bufer where to reduce */
    data_buff = sctk_malloc(sendcnt * stsize);

    assume(data_buff != NULL);

    /* If non-contig, we need to pack to the TMP buffer
     * where the reduction will be operated */
    int cnt = 0;
    PMPI_Pack(sendbuf, sendcnt, sendtype, data_buff, stsize * sendcnt, &cnt,
              comm);

    /* We had to allocate the segment save it for release by the last */
    sctk_atomics_store_ptr(&gv_ctx->src_buffs[rank], data_buff);
    did_allocate_send = 1;
  }

  gv_ctx->send_count[rank] = sendcnt;

  /* Now am I the root ? */
  if (root == rank) {
    if (__do_yield) {
      while (sctk_atomics_cas_int(&gv_ctx->owner, -1, -2) != -1) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_cas_int(&gv_ctx->owner, -1, -2) != -1) {
        sctk_cpu_relax();
      }
    }

    /* Set the ref buffer */
    gv_ctx->target_buff = recvbuf;
    gv_ctx->counts = recvcnts;
    gv_ctx->disps = displs;

    MPI_Aint rtsize = 0;
    res = __INTERNAL__PMPI_Type_extent(recvtype, &rtsize);
    gv_ctx->rtype_size = rtsize;

    if (!sctk_datatype_contig_mem(sendtype)) {
      gv_ctx->let_me_unpack = 1;
    } else {
      gv_ctx->let_me_unpack = 0;
    }

    /* Now unleash the others */
    sctk_atomics_store_int(&gv_ctx->owner, rank);

  } else {
    /* Wait for the root */
    if (__do_yield) {
      while (sctk_atomics_load_int(&gv_ctx->owner) != root) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_load_int(&gv_ctx->owner) != root) {
        sctk_cpu_relax();
      }
    }
  }

  /* if some other processes don't have contig mem we should also unpack */
  if (!sctk_datatype_contig_mem(sendtype) && rank != root) {
     gv_ctx->let_me_unpack = 1;
  }
  /* Where to write ? */
  if (sendbuf != MPI_IN_PLACE) {
    if (gv_ctx->let_me_unpack) {
      /* If we are here the root has a non
       * contiguous data-type, we then
       * have to save our buffer and
       * then leave the root at work to fill the segments */

      /* Is it already packed on our side ? */
      if (!did_allocate_send) {
        /* We need to put it in buffer */
        data_buff = sctk_malloc(sendcnt * stsize);
        assume(data_buff != NULL);
        memcpy(data_buff, sendbuf, sendcnt * stsize);
        sctk_atomics_store_ptr(&gv_ctx->src_buffs[rank], data_buff);
      }
      /*else
      {
               sctk_atomics_store_ptr( &gv_ctx->src_buffs[rank], data_buff );
               was done when packing
      }*/
    } else {
      /* If we are here we can directly write
       * in the target buffer as the type is contig
       * we just have to look for the right disp */

      void *to = NULL;
      size_t to_cpy = 0;

      if (!gv_ctx->disps) {
        /* Gather case */
        to_cpy = gv_ctx->counts[0];
        to = gv_ctx->target_buff + (to_cpy * gv_ctx->rtype_size) * rank;
      } else {
        to_cpy = gv_ctx->counts[rank];
        to = gv_ctx->target_buff + (gv_ctx->disps[rank] * gv_ctx->rtype_size);
      }

      memcpy(to, sendbuf, to_cpy * gv_ctx->rtype_size);
    }
  }

  sctk_atomics_decr_int(&gv_ctx->left_to_push);

  if (rank == root) {
    /* Wait for all the others */
    if (__do_yield) {
      while (sctk_atomics_load_int(&gv_ctx->left_to_push) != 0) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_load_int(&gv_ctx->left_to_push) != 0) {
        sctk_cpu_relax();
      }
    }

    /* Was it required that the root
     * unpacks the whole thing ? */
    if (gv_ctx->let_me_unpack) {
      int i;

      for (i = 0; i < coll->comm_size; i++) {
        int cnt = 0;
        void *to = NULL;
        void *from = NULL;

        from = sctk_atomics_load_ptr(&gv_ctx->src_buffs[i]);

        if (!gv_ctx->disps) {
          /* Gather case */
          sctk_nodebug("UNPACK %d@%d in %d => %d@%d", gv_ctx->send_count[i],
                       gv_ctx->send_type_size[i], i, gv_ctx->counts[0],
                       gv_ctx->rtype_size);
          to = gv_ctx->target_buff +
               (gv_ctx->counts[0] * gv_ctx->rtype_size) * i;
          PMPI_Unpack(from, gv_ctx->send_count[i] * gv_ctx->send_type_size[i],
                      &cnt, to, gv_ctx->counts[0], recvtype, comm);
        } else {
          /* Gatherv case */
          to = gv_ctx->target_buff + (gv_ctx->disps[i] * gv_ctx->rtype_size);
          PMPI_Unpack(from, gv_ctx->send_count[i] * gv_ctx->send_type_size[i],
                      &cnt, to, gv_ctx->counts[i], recvtype, comm);
        }
      }
    }

    /* On free */
    sctk_atomics_store_int(&gv_ctx->left_to_push, coll->comm_size);

    sctk_atomics_store_int(&gv_ctx->owner, -1);

    int current_fare = sctk_atomics_load_int(&gv_ctx->fare);
    sctk_atomics_store_int(&gv_ctx->fare, !current_fare);
  }

  return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Gatherv_intra(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int *recvcnts, int *displs,
                                   MPI_Datatype recvtype, int root,
                                   MPI_Comm comm) {
  MPI_Aint dsize;
  int res = MPI_ERR_INTERN, rank, size;
  MPI_Request request;
  MPI_Request *recvrequest;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if ((rank != root) || (MPI_IN_PLACE != sendbuf)) {
    res = __INTERNAL__PMPI_Isend(sendbuf, sendcnt, sendtype, root,
                                 MPC_GATHERV_TAG, comm, &request);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    request = MPI_REQUEST_NULL;
  }

  recvrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request));

  if (rank == root) {
    int i = 0;
    int j;
    res = __INTERNAL__PMPI_Type_extent(recvtype, &dsize);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }

    while (i < size) {
      for (j = 0; i < size;) {
        if ((i != root) || (MPI_IN_PLACE != sendbuf)) {
          res = __INTERNAL__PMPI_Irecv(
              ((char *)recvbuf) + (displs[i] * dsize), recvcnts[i], recvtype, i,
              MPC_GATHERV_TAG, comm, &(recvrequest[j]));
          if (res != MPI_SUCCESS) {
            sctk_free(recvrequest);
            return res;
          }
        } else {
          recvrequest[j] = MPI_REQUEST_NULL;
        }
        i++;
        j++;
      }
      j--;
      res = __INTERNAL__PMPI_Waitall(size, recvrequest, MPI_STATUSES_IGNORE);
      if (res != MPI_SUCCESS) {
        sctk_free(recvrequest);
        return res;
      }
    }
  }

  res = __INTERNAL__PMPI_Wait(&(request), MPI_STATUS_IGNORE);
  if (res != MPI_SUCCESS) {
    sctk_free(recvrequest);
    return res;
  }

  res = __INTERNAL__PMPI_Barrier(comm);
  sctk_free(recvrequest);
  return res;
}

int __INTERNAL__PMPI_Gatherv_inter(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int *recvcnts, int *displs,
                                   MPI_Datatype recvtype, int root,
                                   MPI_Comm comm) {
  int i, size, res = MPI_ERR_INTERN;
  char *ptmp;
  MPI_Aint extent;
  MPI_Request *recvrequest;

  res = __INTERNAL__PMPI_Comm_remote_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  recvrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request));

  if (root == MPI_PROC_NULL) {
    res = MPI_SUCCESS;
  } else if (root != MPI_ROOT) {
    res = __INTERNAL__PMPI_Send(sendbuf, sendcnt, sendtype, root,
                                MPC_GATHERV_TAG, comm);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }
  } else {
    res = __INTERNAL__PMPI_Type_extent(recvtype, &extent);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }

    for (i = 0; i < size; ++i) {
      ptmp = ((char *)recvbuf) + (extent * displs[i]);

      res = __INTERNAL__PMPI_Irecv(ptmp, recvcnts[i], recvtype, i,
                                   MPC_GATHERV_TAG, comm, &recvrequest[i]);
      if (res != MPI_SUCCESS) {
        sctk_free(recvrequest);
        return res;
      }
    }

    res = __INTERNAL__PMPI_Waitall(size, recvrequest, MPI_STATUSES_IGNORE);
    if (res != MPI_SUCCESS) {
      sctk_free(recvrequest);
      return res;
    }
  }
  sctk_free(recvrequest);
  return res;
}

int __INTERNAL__PMPI_Gatherv(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                             void *recvbuf, int *recvcnts, int *displs,
                             MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (gatherv_inter == NULL)
      gatherv_inter = (int (*)(void *, int, MPI_Datatype, void *, int *, int *,
                               MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.gatherv_inter.value);
    res = gatherv_inter(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs,
                        recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */

    if (gatherv_intra_shm == NULL) {
      gatherv_intra_shm =
          (int (*)(void *, int, MPI_Datatype, void *, int *, int *,
                   MPI_Datatype, int, MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_shm.gatherv_intra_shm.value;
    }

    if (gatherv_intra == NULL) {
      gatherv_intra =
          (int (*)(void *, int, MPI_Datatype, void *, int *, int *,
                   MPI_Datatype, int, MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_intra.gatherv_intra.value;
    }

    if (sctk_is_shared_mem(comm)) {
      res = (gatherv_intra_shm)(sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
                                displs, recvtype, root, comm);
    } else {
      res = (gatherv_intra)(sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
                            displs, recvtype, root, comm);
    }

    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return res;
}

int __INTERNAL__PMPI_Scatter_intra(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcnt, MPI_Datatype recvtype, int root,
                                   MPI_Comm comm) {
  int i, j, res = MPI_ERR_INTERN, size, rank;
  MPI_Aint dsize;
  MPI_Request request;
  MPI_Request *sendrequest;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  sendrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request));

  if ((recvbuf == MPI_IN_PLACE) && (rank == root)) {
    request = MPI_REQUEST_NULL;
  } else {
    res = __INTERNAL__PMPI_Irecv(recvbuf, recvcnt, recvtype, root,
                                 MPC_SCATTER_TAG, comm, &request);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }
  }

  if (rank == root) {
    i = 0;
    res = __INTERNAL__PMPI_Type_extent(sendtype, &dsize);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }
    while (i < size) {
      for (j = 0; i < size;) {
        if ((recvbuf == MPI_IN_PLACE) && (i == root)) {
          sendrequest[j] = MPI_REQUEST_NULL;
        } else {
          res = __INTERNAL__PMPI_Isend(
              ((char *)sendbuf) + (i * sendcnt * dsize), sendcnt, sendtype, i,
              MPC_SCATTER_TAG, comm, &(sendrequest[j]));
          if (res != MPI_SUCCESS) {
            sctk_free(sendrequest);
            return res;
          }
        }
        i++;
        j++;
      }
      j--;
      res = __INTERNAL__PMPI_Waitall(size, sendrequest, MPC_STATUSES_IGNORE);
      if (res != MPI_SUCCESS) {
        sctk_free(sendrequest);
        return res;
      }
    }
  }

  res = __INTERNAL__PMPI_Wait(&(request), MPC_STATUS_IGNORE);
  sctk_free(sendrequest);
  return res;
}

int __INTERNAL__PMPI_Scatter_inter(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcnt, MPI_Datatype recvtype, int root,
                                   MPI_Comm comm) {
  int i, size, res = MPI_ERR_INTERN;
  char *ptmp;
  MPI_Aint incr;
  MPI_Request *sendrequest;

  res = __INTERNAL__PMPI_Comm_remote_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  sendrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request));

  if (root == MPI_PROC_NULL) {
    res = MPI_SUCCESS;
  } else if (root != MPI_ROOT) {
    res = __INTERNAL__PMPI_Recv(recvbuf, recvcnt, recvtype, root,
                                MPC_SCATTER_TAG, comm, MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }

  } else {
    res = __INTERNAL__PMPI_Type_extent(sendtype, &incr);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }

    incr *= sendcnt;
    for (i = 0, ptmp = (char *)sendbuf; i < size; ++i, ptmp += incr) {
      res = __INTERNAL__PMPI_Isend(ptmp, sendcnt, sendtype, i, MPC_SCATTER_TAG,
                                   comm, &(sendrequest[i]));
      if (res != MPI_SUCCESS) {
        sctk_free(sendrequest);
        return res;
      }
    }

    res = __INTERNAL__PMPI_Waitall(size, sendrequest, MPI_STATUSES_IGNORE);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }
  }
  sctk_free(sendrequest);
  return res;
}

int __INTERNAL__PMPI_Scatter_intra_shared_node_impl(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
        void *recvbuf, int recvcnt, MPI_Datatype recvtype,
        int root, MPI_Comm comm, struct sctk_comm_coll * coll ) { 

    /* WARNING we can only be here with a regular scatter
       sendtype == recvtype && recvcount == sendcount */
    int rank;
    PMPI_Comm_rank( comm, &rank );

    int tsize;
    PMPI_Type_size( sendtype, &tsize );

    size_t to_scatter_size = tsize * sendcnt * coll->comm_size;

    static __thread struct shared_node_coll_data *cdata = NULL;


    if( rank == root )
    {
        if( !cdata )
        {
                cdata = mpc_MPI_allocmem_pool_alloc( sizeof(struct shared_node_coll_data) );
                cdata->is_counter = 0;
        }

        if(  !_mpc_MPI_allocmem_is_in_pool(sendbuf) )
        {
            int is_shared = 0;
            cdata->buffer_addr = mpc_MPI_allocmem_pool_alloc_check( to_scatter_size + sizeof(sctk_atomics_int),
                    &is_shared);

            if( !is_shared )
            {
                /* We failed ! */
                sctk_free( cdata->buffer_addr );
                cdata->buffer_addr = NULL;
            }
            else
            {
                /* Fill the buffer */
                sctk_atomics_store_int( (sctk_atomics_int*)cdata->buffer_addr, coll->comm_size );
                memcpy( cdata->buffer_addr + sizeof(sctk_atomics_int), sendbuf, to_scatter_size );
                cdata->is_counter = 1;
            }
        }
        else
        {
            cdata->buffer_addr = sendbuf;
        }
    }

    __INTERNAL__PMPI_Bcast_intra_shared_node( &cdata->buffer_addr, sizeof(struct shared_node_coll_data), MPI_CHAR, root, comm );

    if( cdata->buffer_addr != NULL )
    {

        if( cdata->is_counter )
        {
                memcpy( recvbuf,  cdata->buffer_addr + sizeof(sctk_atomics_int) + (rank * tsize * sendcnt), recvcnt * tsize );
                int token = sctk_atomics_fetch_and_decr_int( (sctk_atomics_int *)cdata->buffer_addr );
                if( token == 1 )
                {
                        mpc_MPI_allocmem_pool_free_size( cdata->buffer_addr ,  to_scatter_size + sizeof(sctk_atomics_int));
                }
        }
        else
        {
                memcpy( recvbuf,  cdata->buffer_addr + (rank * tsize * sendcnt), recvcnt * tsize );
        }

    }
    else
    {
        /* Fallback to regular bcast */
        return (scatter_intra)( sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm );
    }

    return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Scatter_intra_shared_node(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                             void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                             int root, MPI_Comm comm) {

    struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
    //TODO to expose as config vars
    if( __MPC_node_comm_coll_check( coll, comm ) 
    &&  _mpc_MPI_allocmem_is_in_pool(sendbuf) )
    {
     
        int bool_val = sctk_datatype_contig_mem( sendtype );
        bool_val &=  (sendtype == recvtype);
        bool_val &=  (sendcnt == recvcnt);

        int rez = 0;

        __INTERNAL__PMPI_Allreduce( &bool_val, &rez, 1, MPI_INT, MPI_BAND, comm);

        if( !rez )
        {
            return (scatter_intra)( sendbuf, sendcnt, sendtype,
                             recvbuf, recvcnt, recvtype,
                             root, comm);
        
        }


        /* We need to be able to amortize the agreement allreduce */
        return __INTERNAL__PMPI_Scatter_intra_shared_node_impl( sendbuf, sendcnt, sendtype,
                             recvbuf, recvcnt, recvtype,
                             root, comm, coll); 
    }
    else
    {
        return (scatter_intra)( sendbuf, sendcnt, sendtype,
                             recvbuf, recvcnt, recvtype,
                             root, comm);
    }
}




int __INTERNAL__PMPI_Scatter(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                             void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                             int root, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (scatter_inter == NULL)
      scatter_inter = (int (*)(void *, int, MPI_Datatype, void *, int,
                               MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.scatter_inter.value);
    res = scatter_inter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype,
                        root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */

    if (scatterv_intra_shm == NULL) {
      scatterv_intra_shm =
          (int (*)(void *, int *, int *, MPI_Datatype, void *, int,
                   MPI_Datatype, int, MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_shm.scatterv_intra_shm.value;
    }

    if (scatter_intra == NULL) {
      scatter_intra = (int (*)(void *, int, MPI_Datatype, void *, int,
                               MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.scatter_intra.value);
    }


    if (scatter_intra_shared_node == NULL) {
      scatter_intra_shared_node = (int (*)(void *, int, MPI_Datatype, void *, int,
                               MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm_shared.scatter_intra_shared_node.value);
    }



    /* Note there is a bug we did not find with derived data-types
     * on the scatter(v) operation deactivated for now */
    //if (sctk_is_shared_mem(comm) && sctk_datatype_contig_mem(sendtype) &&
        //sctk_datatype_contig_mem(recvtype))
    if (sctk_is_shared_mem(comm)) 
    {
      res = (scatterv_intra_shm)(sendbuf, &sendcnt, NULL, sendtype, recvbuf,
                                 recvcnt, recvtype, root, comm);
    } else if( sctk_is_shared_node(comm) ) {
        res = (scatter_intra_shared_node)(sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
                            recvtype, root, comm);

    } else {
      res = (scatter_intra)(sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
                            recvtype, root, comm);
    }

    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  return res;
}

int __INTERNAL__PMPI_Scatterv_intra_shm(void *sendbuf, int *sendcnts,
                                        int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcnt,
                                        MPI_Datatype recvtype, int root,
                                        MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
  struct shared_mem_scatterv *sv_ctx = &coll->shm_scatterv;

  sctk_nodebug("SCATTER SEND %d CNT %d RECV %d CNT %d", sendtype, sendcnts[0],
               recvtype, recvcnt);

  int rank, res;
  PMPI_Comm_rank(comm, &rank);

  /* First pay the toll gate */
  if (__do_yield) {
    while (sv_ctx->tollgate[rank] != sctk_atomics_load_int(&sv_ctx->fare)) {
      sctk_thread_yield();
    }

  } else {
    while (sv_ctx->tollgate[rank] != sctk_atomics_load_int(&sv_ctx->fare)) {
      sctk_cpu_relax();
    }
  }

  /* Reverse state so that only a root done can unlock by
   * also reversing the fare */
  sv_ctx->tollgate[rank] = !sv_ctx->tollgate[rank];

  void *data_buff = sendbuf;
  int did_allocate_send = 0;

  MPI_Aint rtype_size = 0;
  if(rank != root) 
  {
    res = __INTERNAL__PMPI_Type_extent(recvtype, &rtype_size);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  /* RDV with ROOT */

  /* Now am I the root ? */
  if (root == rank) {
    MPI_Aint stsize = 0;
    res = __INTERNAL__PMPI_Type_extent(sendtype, &stsize);
    if (res != MPI_SUCCESS) {
      return res;
    }

    if (__do_yield) {
      while (sctk_atomics_cas_int(&sv_ctx->owner, -1, -2) != -1) {
        sctk_thread_yield();
      }

    } else {
      while (sctk_atomics_cas_int(&sv_ctx->owner, -1, -2) != -1) {
        sctk_cpu_relax();
      }
    }

    /* Does root need to pack ? */
    if (!sctk_datatype_contig_mem(sendtype)) {
      /* Sorry derived data-types involved
       * lets pack it all */

      /* Are we in the Scatter config ? */
      if (!displs) {
        int cnt = 0;
        sctk_nodebug("PACK S t %d cnt %d extent %d", sendtype,
                     sendcnts[0] * coll->comm_size, stsize);
        /* We are a Scatter */
        size_t buff_size = sendcnts[0] * stsize * coll->comm_size;
        data_buff = sctk_malloc(buff_size);
        assume(data_buff != NULL);
        PMPI_Pack(sendbuf, sendcnts[0] * coll->comm_size, sendtype, data_buff,
                  buff_size, &cnt, comm);
        /* Only store in 0 the big Pack */
        sctk_atomics_store_ptr(&sv_ctx->src_buffs[0], data_buff);
        sv_ctx->stype_size = cnt/(coll->comm_size*sendcnts[0]);
      } else {
        /* We are a Scatterv */
        int i;
        for (i = 0; i < coll->comm_size; ++i) {
          int cnt = 0;
          void *from = sendbuf + displs[i] * stsize;
          size_t to_cpy = sendcnts[i];

          data_buff = sctk_malloc(sendcnts[i] * stsize);
          assume(data_buff != NULL);
          PMPI_Pack(from, sendcnts[i], sendtype, data_buff,
                    stsize * sendcnts[i], &cnt, comm);
          /* Only store in 0 the big Pack */
          sctk_atomics_store_ptr(&sv_ctx->src_buffs[i], data_buff);
          if(sendcnts[i]) 
          {
            sv_ctx->stype_size = cnt/sendcnts[i];
          }
        }
      }

      did_allocate_send = 1;

      /* Notify leaves that it is
       * going to be expensive */
      sv_ctx->was_packed = 1;
    } else {
      /* Yes ! We are contiguous we can start
       * to perform a little */
      sctk_atomics_store_ptr(&sv_ctx->src_buffs[0], data_buff);
      /* Notify leaves that we are on fastpath */
      sv_ctx->was_packed = 0;
      sv_ctx->stype_size = stsize;
    }

    /* Set root infos */
    sv_ctx->disps = displs;
    sv_ctx->counts = sendcnts;

    /* Now unleash the others */
    sctk_atomics_store_int(&sv_ctx->owner, rank);

  } else {
    /* Wait for the root */
    if (__do_yield) {
      while (sctk_atomics_load_int(&sv_ctx->owner) != root) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_load_int(&sv_ctx->owner) != root) {
        sctk_cpu_relax();
      }
    }
  }

  //if (!sctk_datatype_contig_mem(sendtype) && rank != root) {                                    
      //sv_ctx->was_packed = 1;                                                                   
  //}
  /* Where to write ? */
  if (recvbuf != MPI_IN_PLACE) {
    if (sv_ctx->was_packed) {
      void *from = NULL;
      size_t to_cpy = 0;
      int do_free = 0;
      /* Root packed it all for us
       * as it has a non-contig datatype */

      /* Is it already packed on our side ? */
      if (!displs) {
        sctk_nodebug("UNPACK S RT %d rcnt %d rext %d FROM %d ext %d", recvtype,
                     recvcnt, rtype_size, sv_ctx->counts[0],
                     sv_ctx->stype_size);
        /* We are a Scatter only data in [0] */
        void *data = sctk_atomics_load_ptr(&sv_ctx->src_buffs[0]);
        from = data + rank * sv_ctx->counts[0] * sv_ctx->stype_size;
        to_cpy = sv_ctx->counts[0] * sv_ctx->stype_size;

        /* Will be freed by root */
      } else {
        /* We are a ScatterV data in the whole array */
        from = sctk_atomics_load_ptr(&sv_ctx->src_buffs[rank]);
        to_cpy = sv_ctx->counts[rank] * sv_ctx->stype_size;
        do_free = 1;
      }

      int cnt = 0;
      PMPI_Unpack(from, to_cpy, &cnt, recvbuf, recvcnt, recvtype, comm);

      if (do_free) {
        sctk_free(from);
      }

    } else {
      /* If we are here we can directly read
       * in the target buffer as the type is contig
       * we just have to look for the right disp */
      void *from = NULL;
      size_t to_cpy = 0;

      if (!sv_ctx->disps) {
        /* Scatter case */
        void *data = sctk_atomics_load_ptr(&sv_ctx->src_buffs[0]);
        from = data + sv_ctx->counts[0] * rank * sv_ctx->stype_size;
        to_cpy = sv_ctx->counts[0] * sv_ctx->stype_size;
      } else {
        /* ScatterV case */
        void *data = sctk_atomics_load_ptr(&sv_ctx->src_buffs[0]);
        from = data + sv_ctx->disps[rank] * sv_ctx->stype_size;
        to_cpy = sv_ctx->counts[rank] * sv_ctx->stype_size;
      }

      if (!sctk_datatype_contig_mem(recvtype)) {
        /* Recvtype is non-contig */
        int cnt = 0;
        PMPI_Unpack(from, to_cpy, &cnt, recvbuf, recvcnt, recvtype, comm);
      } else {
        /* Recvtype is contiguous */
        memcpy(recvbuf, from, to_cpy);
      }
    }
  }

  sctk_atomics_decr_int(&sv_ctx->left_to_pop);

  if (rank == root) {
    /* Wait for all the others */
    if (__do_yield) {
      while (sctk_atomics_load_int(&sv_ctx->left_to_pop) != 0) {
        sctk_thread_yield();
      }

    } else {
      while (sctk_atomics_load_int(&sv_ctx->left_to_pop) != 0) {
        sctk_cpu_relax();
      }
    }

    /* Do we need to free data packed for Scatter case ? */
    if (!sctk_datatype_contig_mem(sendtype)) {
      if (!displs) {
        void *data = sctk_atomics_load_ptr(&sv_ctx->src_buffs[0]);
        sctk_free(data);
      }
    }

    /* On free */
    sctk_atomics_store_int(&sv_ctx->left_to_pop, coll->comm_size);

    sctk_atomics_store_int(&sv_ctx->owner, -1);

    int current_fare = sctk_atomics_load_int(&sv_ctx->fare);
    sctk_atomics_store_int(&sv_ctx->fare, !current_fare);
  }

  return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Scatterv_intra(void *sendbuf, int *sendcnts, int *displs,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcnt, MPI_Datatype recvtype,
                                    int root, MPI_Comm comm) {
  int size, rank, i, j, res = MPI_ERR_INTERN;
  MPI_Request request;
  MPI_Request *sendrequest;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  sendrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request));

  if ((recvbuf == MPI_IN_PLACE) && (rank == root)) {
    request = MPI_REQUEST_NULL;
  } else {
    res = __INTERNAL__PMPI_Irecv(recvbuf, recvcnt, recvtype, root,
                                 MPC_SCATTERV_TAG, comm, &request);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }
  }

  if (rank == root) {
    MPI_Aint send_type_size;
    res = __INTERNAL__PMPI_Type_extent(sendtype, &send_type_size);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }

    i = 0;
    while (i < size) {
      for (j = 0; i < size;) {
        if ((recvbuf == MPI_IN_PLACE) && (i == root)) {
          sendrequest[j] = MPI_REQUEST_NULL;
        } else {
          res = __INTERNAL__PMPI_Isend(
              ((char *)sendbuf) + (displs[i] * send_type_size), sendcnts[i],
              sendtype, i, MPC_SCATTERV_TAG, comm, &(sendrequest[j]));
          if (res != MPI_SUCCESS) {
            sctk_free(sendrequest);
            return res;
          }
        }
        i++;
        j++;
      }
      j--;
      res = __INTERNAL__PMPI_Waitall(size, sendrequest, MPI_STATUSES_IGNORE);
      if (res != MPI_SUCCESS) {
        sctk_free(sendrequest);
        return res;
      }
    }
  }

  res = __INTERNAL__PMPI_Wait(&(request), MPI_STATUS_IGNORE);
  if (res != MPI_SUCCESS) {
    sctk_free(sendrequest);
    return res;
  }
  res = __INTERNAL__PMPI_Barrier(comm);
  sctk_free(sendrequest);
  return res;
}

int __INTERNAL__PMPI_Scatterv_inter(void *sendbuf, int *sendcnts, int *displs,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcnt, MPI_Datatype recvtype,
                                    int root, MPI_Comm comm) {
  int i, rsize, res = MPI_ERR_INTERN;
  char *ptmp;
  MPI_Aint extent;
  MPI_Request request;
  MPI_Request *sendrequest;

  res = __INTERNAL__PMPI_Comm_remote_size(comm, &rsize);
  if (res != MPI_SUCCESS) {
    return res;
  }

  sendrequest = (MPI_Request *)sctk_malloc(rsize * sizeof(MPI_Request));

  if (root == MPI_PROC_NULL) {
    res = MPI_SUCCESS;
  } else if (MPI_ROOT != root) {
    res = __INTERNAL__PMPI_Recv(recvbuf, recvcnt, recvtype, root,
                                MPC_SCATTERV_TAG, comm, MPI_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }
  } else {
    res = __INTERNAL__PMPI_Type_extent(sendtype, &extent);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }

    for (i = 0; i < rsize; ++i) {
      ptmp = ((char *)sendbuf) + (extent * displs[i]);
      res = __INTERNAL__PMPI_Isend(ptmp, sendcnts[i], sendtype, i,
                                   MPC_SCATTERV_TAG, comm, &(sendrequest[i]));
      if (res != MPI_SUCCESS) {
        sctk_free(sendrequest);
        return res;
      }
    }

    res = __INTERNAL__PMPI_Waitall(rsize, sendrequest, MPI_STATUSES_IGNORE);
    if (res != MPI_SUCCESS) {
      sctk_free(sendrequest);
      return res;
    }
  }
  sctk_free(sendrequest);
  return res;
}

int __INTERNAL__PMPI_Scatterv(void *sendbuf, int *sendcnts, int *displs,
                              MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                              MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (scatterv_inter == NULL)
      scatterv_inter = (int (*)(void *, int *, int *, MPI_Datatype, void *, int,
                                MPI_Datatype, int, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.scatterv_inter.value);
    res = scatterv_inter(sendbuf, sendcnts, displs, sendtype, recvbuf, recvcnt,
                         recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */
    if (scatterv_intra_shm == NULL) {
      scatterv_intra_shm =
          (int (*)(void *, int *, int *, MPI_Datatype, void *, int,
                   MPI_Datatype, int, MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_shm.scatterv_intra_shm.value;
    }

    if (scatterv_intra == NULL) {
      scatterv_intra =
          (int (*)(void *, int *, int *, MPI_Datatype, void *, int,
                   MPI_Datatype, int, MPI_Comm))sctk_runtime_config_get()
              ->modules.collectives_intra.scatterv_intra.value;
    }

    if (sctk_is_shared_mem(comm)) {

      res = (scatterv_intra_shm)(sendbuf, sendcnts, displs, sendtype, recvbuf,
                                 recvcnt, recvtype, root, comm);
    } else {
      res = (scatterv_intra)(sendbuf, sendcnts, displs, sendtype, recvbuf,
                             recvcnt, recvtype, root, comm);
    }

    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  return res;
}

int __INTERNAL__PMPI_Allgather_intra(void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int recvcount, MPI_Datatype recvtype,
                                     MPI_Comm comm) {
  int root = 0, size, rank, res = MPI_ERR_INTERN;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if (sendbuf == MPI_IN_PLACE) {
    MPI_Aint extent;
    res = __INTERNAL__PMPI_Type_extent(recvtype, &extent);
    if (res != MPI_SUCCESS) {
      return res;
    }
    sendbuf = ((char *)recvbuf) + (rank * extent * recvcount);
    sendtype = recvtype;
    sendcount = recvcount;
  }

  res = __INTERNAL__PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm);
  if (res != MPI_SUCCESS) {
    return res;
  }

  res = __INTERNAL__PMPI_Bcast(recvbuf, size * recvcount, recvtype, root, comm);
  if (res != MPI_SUCCESS) {
    return res;
  }

  return res;
}

int __INTERNAL__PMPI_Allgather_inter(void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int recvcount, MPI_Datatype recvtype,
                                     MPI_Comm comm) {
  int size, rank, remote_size, res = MPI_ERR_INTERN;
  int root = 0;
  MPI_Aint slb, sextent;
  void *tmp_buf;

  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if ((rank == 0) && (sendcount != 0)) {
    res = __INTERNAL__PMPI_Type_lb(sendtype, &slb);
    if (res != MPI_SUCCESS) {
      return res;
    }
    res = __INTERNAL__PMPI_Type_extent(sendtype, &sextent);
    if (res != MPI_SUCCESS) {
      return res;
    }

    tmp_buf = (void *)sctk_malloc(sextent * sendcount * size * sizeof(void *));
    tmp_buf = (void *)((char *)tmp_buf - slb);
  }

  if (sendcount != 0) {
    res = __INTERNAL__PMPI_Gather(sendbuf, sendcount, sendtype, tmp_buf,
                                  sendcount, sendtype, 0,
                                  sctk_get_local_comm_id(comm));
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  if (sctk_is_in_local_group(comm)) {
    if (sendcount != 0) {
      root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
      res = __INTERNAL__PMPI_Bcast(tmp_buf, size * sendcount, sendtype, root,
                                   comm);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }

    if (recvcount != 0) {
      root = 0;
      res = __INTERNAL__PMPI_Bcast(recvbuf, remote_size * recvcount, recvtype,
                                   root, comm);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
  } else {
    if (recvcount != 0) {
      root = 0;
      res = __INTERNAL__PMPI_Bcast(recvbuf, remote_size * recvcount, recvtype,
                                   root, comm);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }

    if (sendcount != 0) {
      root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
      res = __INTERNAL__PMPI_Bcast(tmp_buf, size * sendcount, sendtype, root,
                                   comm);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
  }
  return res;
}

int __INTERNAL__PMPI_Allgather(void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf,
                               int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (allgather_inter == NULL)
      allgather_inter = (int (*)(void *, int, MPI_Datatype, void *, int,
                                 MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.allgather_inter.value);
    res = allgather_inter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */
    if (allgather_intra == NULL)
      allgather_intra = (int (*)(void *, int, MPI_Datatype, void *, int,
                                 MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.allgather_intra.value);
    res = allgather_intra(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return res;
}

int __INTERNAL__PMPI_Allgatherv_intra(void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int *recvcounts, int *displs,
                                      MPI_Datatype recvtype, MPI_Comm comm) {
  int size, rank;
  int root = 0;
  MPI_Aint extent;
  int res = MPI_ERR_INTERN;

  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Type_extent(recvtype, &extent);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if (sendbuf == MPI_IN_PLACE) {
    int i = 0;
    sendtype = recvtype;
    sendbuf = (char *)recvbuf;
    for (i = 0; i < rank; ++i) {
      sendbuf += (recvcounts[i] * extent);
    }
    sendcount = recvcounts[rank];
  }

  res = __INTERNAL__PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root, comm);
  if (res != MPI_SUCCESS) {
    return res;
  }

  size--;
  for (; size >= 0; size--) {
    res = __INTERNAL__PMPI_Bcast(((char *)recvbuf) + (displs[size] * extent),
                                 recvcounts[size], recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  res = __INTERNAL__PMPI_Barrier(comm);
  if (res != MPI_SUCCESS) {
    return res;
  }

  return res;
}

int __INTERNAL__PMPI_Allgatherv_inter(void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int *recvcounts, int *displs,
                                      MPI_Datatype recvtype, MPI_Comm comm) {
  int size, rsize, rank, res = MPI_ERR_INTERN;
  int root = 0;
  MPI_Aint extent;
  MPC_Comm local_comm;

  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_remote_size(comm, &rsize);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if (sctk_is_in_local_group(comm)) {
    root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
    res = __INTERNAL__PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }

    root = 0;
    res = __INTERNAL__PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    root = 0;
    res = __INTERNAL__PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }

    root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
    res = __INTERNAL__PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  rsize--;
  root = 0;
  local_comm = sctk_get_local_comm_id(comm);
  res = __INTERNAL__PMPI_Type_extent(recvtype, &extent);
  for (; rsize >= 0; rsize--) {
    res = __INTERNAL__PMPI_Bcast(((char *)recvbuf) + (displs[rsize] * extent),
                                 recvcounts[rsize], recvtype, root, local_comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return res;
}

int __INTERNAL__PMPI_Allgatherv(void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                int *recvcounts, int *displs,
                                MPI_Datatype recvtype, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (allgatherv_inter == NULL)
      allgatherv_inter = (int (*)(void *, int, MPI_Datatype, void *, int *,
                                  int *, MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.allgatherv_inter.value);
    res = allgatherv_inter(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                           displs, recvtype, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */
    if (allgatherv_intra == NULL)
      allgatherv_intra = (int (*)(void *, int, MPI_Datatype, void *, int *,
                                  int *, MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.allgatherv_intra.value);
    res = allgatherv_intra(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                           displs, recvtype, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return res;
}

int __INTERNAL__PMPI_Alltoall_intra(void *sendbuf, int sendcount,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcount, MPI_Datatype recvtype,
                                    MPI_Comm comm) {
  int i;
  int res = MPI_ERR_INTERN;
  int size;
  int rank;
  int bblock = 4;
  MPI_Request requests[2 * bblock * sizeof(MPI_Request)];
  int ss, ii;
  int dst;
  MPI_Aint d_send, d_recv;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  res = __INTERNAL__PMPI_Type_extent(sendtype, &d_send);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if(sendbuf == MPI_IN_PLACE)
  {
    size_t to_cpy = size * recvcount * d_send;

    sendbuf = sctk_malloc(to_cpy);
    memcpy(sendbuf,recvbuf,to_cpy);
  }
  res = __INTERNAL__PMPI_Type_extent(recvtype, &d_recv);
  if (res != MPI_SUCCESS) {
    return res;
  }

  for (ii = 0; ii < size; ii += bblock) {
    ss = size - ii < bblock ? size - ii : bblock;
    for (i = 0; i < ss; ++i) {
      dst = (rank + i + ii) % size;
      res = __INTERNAL__PMPI_Irecv(
          ((char *)recvbuf) + (dst * recvcount * d_recv), recvcount, recvtype,
          dst, MPC_ALLTOALL_TAG, comm, &requests[i]);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
    for (i = 0; i < ss; ++i) {
      dst = (rank - i - ii + size) % size;
      res = __INTERNAL__PMPI_Isend(
          ((char *)sendbuf) + (dst * sendcount * d_send), sendcount, sendtype,
          dst, MPC_ALLTOALL_TAG, comm, &requests[i + ss]);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
    res = __INTERNAL__PMPI_Waitall(2 * ss, requests, MPC_STATUSES_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return res;
}

int __INTERNAL__PMPI_Alltoall_inter(void *sendbuf, int sendcount,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcount, MPI_Datatype recvtype,
                                    MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  int local_size, remote_size, max_size, i;
  MPC_Status status;
  MPI_Aint sendtype_extent, recvtype_extent;
  int src, dst, rank;
  char *sendaddr, *recvaddr;

  res = __INTERNAL__PMPI_Comm_size(comm, &local_size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  res = __INTERNAL__PMPI_Type_extent(sendtype, &sendtype_extent);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Type_extent(recvtype, &recvtype_extent);
  if (res != MPI_SUCCESS) {
    return res;
  }

  max_size = (local_size < remote_size) ? remote_size : local_size;
  for (i = 0; i < max_size; i++) {
    src = (rank - i + max_size) % max_size;
    dst = (rank + i) % max_size;

    if (src >= remote_size) {
      src = MPI_PROC_NULL;
      recvaddr = NULL;
    } else {
      recvaddr = (char *)recvbuf + src * recvcount * recvtype_extent;
    }

    if (dst >= remote_size) {
      dst = MPI_PROC_NULL;
      sendaddr = NULL;
    } else {
      sendaddr = (char *)sendbuf + dst * sendcount * sendtype_extent;
    }
    res = __INTERNAL__PMPI_Sendrecv(
        sendaddr, sendcount, sendtype, dst, MPC_ALLTOALL_TAG, recvaddr,
        recvcount, recvtype, src, MPC_ALLTOALL_TAG, comm, &status);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  return res;
}


int __INTERNAL__PMPI_Alltoall_intra_shared_node(void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm) {
        /* We handle the simple case only */
        int bool_val = sctk_datatype_contig_mem( sendtype );
        bool_val &=  (sendtype == recvtype);
        bool_val &=  (sendcount == recvcount);

        struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);

        if( !__MPC_node_comm_coll_check( coll, comm ) )
        {
                return (alltoall_intra)( sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
        }

        int rez = 0;

        __INTERNAL__PMPI_Allreduce( &bool_val, &rez, 1, MPI_INT, MPI_BAND, comm);

        if( !rez )
        {
                return (alltoall_intra)( sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
        }
        else
        {
                int i;
                int tsize, rank;
                PMPI_Type_size( sendtype, &tsize );
                PMPI_Comm_rank( comm, &rank );

                for(i = 0 ; i < coll->comm_size ; i++)
                {
                        __INTERNAL__PMPI_Scatter( sendbuf, sendcount, sendtype, recvbuf + (tsize * recvcount)*i, recvcount, recvtype, i, comm);
                }
        }

        return MPI_SUCCESS;
} 



int __INTERNAL__PMPI_Alltoall(void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (alltoall_inter == NULL)
      alltoall_inter = (int (*)(void *, int, MPI_Datatype, void *, int,
                                MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.alltoall_inter.value);
    res = alltoall_inter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                         recvtype, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */

    if (alltoallv_intra_shm == NULL) {
      alltoallv_intra_shm = (int (*)(void *, int *, int *, MPI_Datatype, void *,
                                     int *, int *, MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm.alltoallv_intra_shm.value);
    }

    if (alltoall_intra == NULL) {
      alltoall_intra = (int (*)(void *, int, MPI_Datatype, void *, int,
                                MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.alltoall_intra.value);
    }


    if (alltoall_intra_shared_node == NULL) {
      alltoall_intra_shared_node = (int (*)(void *, int, MPI_Datatype, void *, int,
                                MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm_shared.alltoall_intra_shared_node.value);
    }


    if (sctk_is_shared_mem(comm)) {
      res = (alltoallv_intra_shm)(sendbuf, &sendcount, NULL, sendtype, recvbuf,
                                  &recvcount, NULL, recvtype, comm);
    } else if(sctk_is_shared_node(comm) ) {
      res = (alltoall_intra_shared_node)(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm );
    } else {
      res = (alltoall_intra)(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, comm);
    }
  }

  return res;
}

int __INTERNAL__PMPI_Alltoallv_intra_shm(void *sendbuf, int *sendcnts,
                                         int *sdispls, MPI_Datatype sendtype,
                                         void *recvbuf, int *recvcnts,
                                         int *rdispls, MPI_Datatype recvtype,
                                         MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);
  struct shared_mem_a2a *aa_ctx = &coll->shm_a2a;

  int rank;
  __INTERNAL__PMPI_Comm_rank(comm, &rank);

  struct sctk_shared_mem_a2a_infos info;
  int is_in_place = 0;

  if (sendbuf == MPI_IN_PLACE) {
    __INTERNAL__PMPI_Type_extent(recvtype, &info.stype_size);
    info.disps = rdispls;
    info.counts = recvcnts;
    is_in_place = 1;
    size_t to_cpy = 0;
    if(sdispls)
    {
      int i;
      for (i = 0; i < coll->comm_size; i++) {
        to_cpy += info.stype_size * rdispls[i];
      }
      to_cpy +=recvcnts[coll->comm_size -1]*info.stype_size;

    }
    else
    {
      to_cpy = coll->comm_size * recvcnts[0]*info.stype_size;
    }
    info.source_buff = malloc(to_cpy);
    memcpy(info.source_buff,recvbuf,to_cpy);

  } else {
    __INTERNAL__PMPI_Type_extent(sendtype, &info.stype_size);
    info.source_buff = sendbuf;
    info.disps = sdispls;
    info.counts = sendcnts;
  }

  MPI_Aint rtsize;
  __INTERNAL__PMPI_Type_extent(recvtype, &rtsize);

  /* Pack if needed */
  if (!sctk_datatype_contig_mem(sendtype)) {
    int i;

    info.packed_buff = sctk_malloc(sizeof(void *) * coll->comm_size);

    assume(info.packed_buff != NULL);

    for (i = 0; i < coll->comm_size; i++) {
      size_t to_cpy = 0;
      void *from = NULL;
      int scnt = 0;

      if (!sdispls) {
        /* Alltoall */
        from = sendbuf + sendcnts[0] * i * info.stype_size;
        to_cpy = info.stype_size * sendcnts[0];
        scnt = sendcnts[0];
      } else {
        /* Alltoallv */
        from = sendbuf + sdispls[i] * info.stype_size;
        to_cpy = info.stype_size * sendcnts[i];
        scnt = sendcnts[i];
      }

      /* Sendtype is non-contig */
      info.packed_buff[i] = sctk_malloc(to_cpy);
      assume(info.source_buff != NULL);

      int cnt = 0;
      PMPI_Pack(from, scnt, sendtype, info.packed_buff[i], to_cpy, &cnt, comm);
    }

  } else {
    info.packed_buff = NULL;
  }

  /* Register the infos in the array */
  aa_ctx->infos[rank] = &info;

  __INTERNAL__PMPI_Barrier(comm);

  int i, j;

  int current_rank = 0;

  for (j = 0; j < coll->comm_size; j++) {
    /* Try to split readings */
    i = (rank + j) % coll->comm_size;

    /* No need to copy if we work in place */
    if (is_in_place && (rank == i))
      continue;

    /* Get data from each rank */

    void *from = NULL;
    void *to = NULL;
    size_t to_cpy = 0;
    int rcount = 0;

    /* Choose dest and size */
    if (sdispls && rdispls) {
      /* This is All2Allv */
      // to_cpy = aa_ctx->infos[i]->counts[i] * aa_ctx->infos[i]->stype_size;
      to_cpy = recvcnts[i] * rtsize;
      to = recvbuf + (rdispls[i] * rtsize);
      rcount = recvcnts[i];
    } else {
      /* This is All2All */
      // to_cpy = aa_ctx->infos[i]->counts[0] * aa_ctx->infos[i]->stype_size;
      to_cpy = recvcnts[0] * rtsize;
      to = recvbuf + (recvcnts[0] * rtsize) * i;
      rcount = recvcnts[0];
    }

    if (aa_ctx->infos[i]->packed_buff) {
      from = aa_ctx->infos[i]->packed_buff[rank];
    } else {
      if (sdispls && rdispls) {
        /* Alltoallv */
        from = aa_ctx->infos[i]->source_buff +
               aa_ctx->infos[i]->disps[rank] * aa_ctx->infos[i]->stype_size;
      } else {
        /* Alltoall */
        from = aa_ctx->infos[i]->source_buff + to_cpy * rank;
      }
    }

    if (!sctk_datatype_contig_mem(recvtype)) {
      /* Recvtype is non-contig */
      int cnt = 0;
      PMPI_Unpack(from, to_cpy, &cnt, to, rcount, recvtype, comm);

    } else {
      /* We can memcpy */
      memcpy(to, from, to_cpy);
    }
  }

  __INTERNAL__PMPI_Barrier(comm);

  if(is_in_place) {
    sctk_free(info.source_buff);
  }

  if (!sctk_datatype_contig_mem(sendtype)) {
    int i;

    for (i = 0; i < coll->comm_size; i++) {
      sctk_free(info.packed_buff[i]);
    }

    sctk_free(info.packed_buff);
    info.source_buff = NULL;
  }

  return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Alltoallv_intra(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype recvtype, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  int i, size, rank, ss, ii, dst,is_in_place = 0;
  int bblock = 4;
  MPI_Request requests[2 * bblock * sizeof(MPI_Request)];
  MPI_Aint d_send, d_recv;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  res = __INTERNAL__PMPI_Type_extent(recvtype, &d_recv);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if (sendbuf == MPI_IN_PLACE) {
    sendcnts = recvcnts;
    sdispls = rdispls;
    d_send = d_recv;
    size_t to_cpy = 0;
    int i;
    for (i = 0; i < size; i++) {
      to_cpy +=d_recv*rdispls[i];
    }
    to_cpy +=recvcnts[size -1]*d_recv; 
    sendbuf = malloc(to_cpy);
    memcpy(sendbuf,recvbuf,to_cpy);
    is_in_place = 1;
  }
  else
  {
    res = __INTERNAL__PMPI_Type_extent(sendtype, &d_send);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }


  for (ii = 0; ii < size; ii += bblock) {
    ss = size - ii < bblock ? size - ii : bblock;
    for (i = 0; i < ss; ++i) {
      dst = (rank + i + ii) % size;
      res = __INTERNAL__PMPI_Irecv(((char *)recvbuf) + (rdispls[dst] * d_recv),
                                   recvcnts[dst], recvtype, dst,
                                   MPC_ALLTOALLV_TAG, comm, &requests[i]);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
    for (i = 0; i < ss; ++i) {
      dst = (rank - i - ii + size) % size;
      res = __INTERNAL__PMPI_Isend(((char *)sendbuf) + (sdispls[dst] * d_send),
                                   sendcnts[dst], sendtype, dst,
                                   MPC_ALLTOALLV_TAG, comm, &requests[i + ss]);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
    res = __INTERNAL__PMPI_Waitall(2 * ss, requests, MPC_STATUS_IGNORE);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }
  if(is_in_place == 1)
    sctk_free(sendbuf);

  return res;
}

int __INTERNAL__PMPI_Alltoallv_inter(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype recvtype, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  int local_size, remote_size, max_size, i;
  MPC_Status status;
  size_t sendtype_extent, recvtype_extent;
  int src, dst, rank, sendcount, recvcount;
  char *sendaddr, *recvaddr;

  res = __INTERNAL__PMPI_Comm_size(comm, &local_size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  res = __INTERNAL__PMPI_Type_extent(sendtype, (MPI_Aint *)&sendtype_extent);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Type_extent(recvtype, (MPI_Aint *)&recvtype_extent);
  if (res != MPI_SUCCESS) {
    return res;
  }

  max_size = (local_size < remote_size) ? remote_size : local_size;
  for (i = 0; i < max_size; i++) {
    src = (rank - i + max_size) % max_size;
    dst = (rank + i) % max_size;
    if (src >= remote_size) {
      src = MPI_PROC_NULL;
      recvaddr = NULL;
      recvcount = 0;
    } else {
      recvaddr = (char *)recvbuf + rdispls[src] * recvtype_extent;
      recvcount = recvcnts[src];
    }
    if (dst >= remote_size) {
      dst = MPI_PROC_NULL;
      sendaddr = NULL;
      sendcount = 0;
    } else {
      sendaddr = (char *)sendbuf + sdispls[dst] * sendtype_extent;
      sendcount = sendcnts[dst];
    }

    res = __INTERNAL__PMPI_Sendrecv(
        sendaddr, sendcount, sendtype, dst, MPC_ALLTOALLV_TAG, recvaddr,
        recvcount, recvtype, src, MPC_ALLTOALLV_TAG, comm, &status);
  }
  return res;
}

int __INTERNAL__PMPI_Alltoallv(void *sendbuf, int *sendcnts, int *sdispls,
                               MPI_Datatype sendtype, void *recvbuf,
                               int *recvcnts, int *rdispls,
                               MPI_Datatype recvtype, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (alltoallv_inter == NULL)
      alltoallv_inter = (int (*)(void *, int *, int *, MPI_Datatype, void *,
                                 int *, int *, MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.alltoallv_inter.value);
    res = alltoallv_inter(sendbuf, sendcnts, sdispls, sendtype, recvbuf,
                          recvcnts, rdispls, recvtype, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */
    if (alltoallv_intra_shm == NULL) {
      alltoallv_intra_shm = (int (*)(void *, int *, int *, MPI_Datatype, void *,
                                     int *, int *, MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_shm.alltoallv_intra_shm.value);
    }

    if (alltoallv_intra == NULL) {
      alltoallv_intra = (int (*)(void *, int *, int *, MPI_Datatype, void *,
                                 int *, int *, MPI_Datatype, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.alltoallv_intra.value);
    }

    if (sctk_is_shared_mem(comm)) {
      res = alltoallv_intra_shm(sendbuf, sendcnts, sdispls, sendtype, recvbuf,
                                recvcnts, rdispls, recvtype, comm);
    } else {
      res = alltoallv_intra(sendbuf, sendcnts, sdispls, sendtype, recvbuf,
                            recvcnts, rdispls, recvtype, comm);
    }
  }
  return res;
}

int __INTERNAL__PMPI_Alltoallw_intra(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype *sendtypes, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype *recvtypes, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  int i, j, ii, ss, dst;
  int type_size, size;
  int rank, bblock = 4;
  MPI_Status status;
  MPI_Status *starray;
  MPI_Request *reqarray;
  int outstanding_requests;

  res = __INTERNAL__PMPI_Comm_size(comm, &size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }

  if (bblock < size)
    bblock = size;

  if (sendbuf == MPI_IN_PLACE) {
    for (i = 0; i < size; ++i) {
      for (j = i; j < size; ++j) {
        if (rank == i) {
          res = __INTERNAL__PMPI_Sendrecv_replace(
              ((char *)recvbuf + rdispls[j]), recvcnts[j], recvtypes[j], j,
              MPC_ALLTOALLW_TAG, j, MPC_ALLTOALLW_TAG, comm, &status);
          if (res != MPI_SUCCESS) {
            return res;
          }
        } else if (rank == j) {
          res = __INTERNAL__PMPI_Sendrecv_replace(
              ((char *)recvbuf + rdispls[i]), recvcnts[i], recvtypes[i], i,
              MPC_ALLTOALLW_TAG, i, MPC_ALLTOALLW_TAG, comm, &status);
          if (res != MPI_SUCCESS) {
            return res;
          }
        }
      }
    }
  } else {
    starray = (MPI_Status *)sctk_malloc(2 * bblock * sizeof(MPI_Status));
    reqarray = (MPI_Request *)sctk_malloc(2 * bblock * sizeof(MPI_Request));

    for (ii = 0; ii < size; ii += bblock) {
      outstanding_requests = 0;
      ss = size - ii < bblock ? size - ii : bblock;

      for (i = 0; i < ss; i++) {
        dst = (rank + i + ii) % size;
        if (recvcnts[dst]) {
          res = __INTERNAL__PMPI_Type_size(recvtypes[dst], &type_size);
          if (res != MPI_SUCCESS) {
            return res;
          }
          if (type_size) {
            res = __INTERNAL__PMPI_Irecv(
                (char *)recvbuf + rdispls[dst], recvcnts[dst], recvtypes[dst],
                dst, MPC_ALLTOALLW_TAG, comm, &reqarray[outstanding_requests]);
            if (res != MPI_SUCCESS) {
              return res;
            }
            outstanding_requests++;
          }
        }
      }

      for (i = 0; i < ss; i++) {
        dst = (rank - i - ii + size) % size;
        if (sendcnts[dst]) {
          res = __INTERNAL__PMPI_Type_size(sendtypes[dst], &type_size);
          if (res != MPI_SUCCESS) {
            return res;
          }
          if (type_size) {
            res = __INTERNAL__PMPI_Isend(
                (char *)sendbuf + sdispls[dst], sendcnts[dst], sendtypes[dst],
                dst, MPC_ALLTOALLW_TAG, comm, &reqarray[outstanding_requests]);
            if (res != MPI_SUCCESS) {
              return res;
            }
            outstanding_requests++;
          }
        }
      }

      res = __INTERNAL__PMPI_Waitall(outstanding_requests, reqarray, starray);
      if (res != MPI_SUCCESS) {
        return res;
      }
    }
  }
  return res;
}

int __INTERNAL__PMPI_Alltoallw_inter(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype *sendtypes, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype *recvtypes, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  int local_size, remote_size, max_size, i;
  MPI_Status status;
  MPI_Datatype sendtype, recvtype;
  int src, dst, rank, sendcount, recvcount;
  char *sendaddr, *recvaddr;

  res = __INTERNAL__PMPI_Comm_size(comm, &local_size);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
  if (res != MPI_SUCCESS) {
    return res;
  }
  res = __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
  if (res != MPI_SUCCESS) {
    return res;
  }

  max_size = (local_size < remote_size) ? remote_size : local_size;
  for (i = 0; i < max_size; i++) {
    src = (rank - i + max_size) % max_size;
    dst = (rank + i) % max_size;
    if (src >= remote_size) {
      src = MPI_PROC_NULL;
      recvaddr = NULL;
      recvcount = 0;
      recvtype = MPI_DATATYPE_NULL;
    } else {
      recvaddr = (char *)recvbuf + rdispls[src];
      recvcount = recvcnts[src];
      recvtype = recvtypes[src];
    }
    if (dst >= remote_size) {
      dst = MPI_PROC_NULL;
      sendaddr = NULL;
      sendcount = 0;
      sendtype = MPI_DATATYPE_NULL;
    } else {
      sendaddr = (char *)sendbuf + sdispls[dst];
      sendcount = sendcnts[dst];
      sendtype = sendtypes[dst];
    }

    res = __INTERNAL__PMPI_Sendrecv(
        sendaddr, sendcount, sendtype, dst, MPC_ALLTOALLW_TAG, recvaddr,
        recvcount, recvtype, src, MPC_ALLTOALLW_TAG, comm, &status);
  }
  return res;
}

int __INTERNAL__PMPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls,
                               MPI_Datatype *sendtypes, void *recvbuf,
                               int *recvcnts, int *rdispls,
                               MPI_Datatype *recvtypes, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;

  /* Intercomm */
  if (sctk_is_inter_comm(comm)) {
    if (alltoallw_inter == NULL)
      alltoallw_inter = (int (*)(void *, int *, int *, MPI_Datatype *, void *,
                                 int *, int *, MPI_Datatype *, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_inter.alltoallw_inter.value);
    res = alltoallw_inter(sendbuf, sendcnts, sdispls, sendtypes, recvbuf,
                          recvcnts, rdispls, recvtypes, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  } else {
    /* Intracomm */
    if (alltoallw_intra == NULL)
      alltoallw_intra = (int (*)(void *, int *, int *, MPI_Datatype *, void *,
                                 int *, int *, MPI_Datatype *, MPI_Comm))(
          sctk_runtime_config_get()
              ->modules.collectives_intra.alltoallw_intra.value);
    res = alltoallw_intra(sendbuf, sendcnts, sdispls, sendtypes, recvbuf,
                          recvcnts, rdispls, recvtypes, comm);
    if (res != MPI_SUCCESS) {
      return res;
    }
  }

  return res;
}

/* Neighbor collectives */
static int __INTERNAL__PMPI_Neighbor_allgather_cart(
    void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
    int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  MPI_Aint extent;
  int rank;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;
  int rc = MPI_SUCCESS, dim, nreqs = 0;
  PMPI_Comm_rank(comm, &rank);

  PMPI_Type_extent(recvtype, &extent);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  reqs = sctk_malloc((4 * (topo->data.cart.ndims)) * sizeof(MPI_Request *));

  for (dim = 0, nreqs = 0; dim < topo->data.cart.ndims; ++dim) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgather_cart: Recv from %d to %d", srank,
          rank);
      rc = PMPI_Irecv(recvbuf, recvcount, recvtype, srank, 2, comm,
                      &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;

      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgather_cart: Rank %d send to %d", rank,
          srank);
      rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, srank, 2, comm,
                      &reqs[nreqs + 1]);
      if (rc != MPI_SUCCESS)
        break;

      nreqs += 2;
    }

    recvbuf = (char *)recvbuf + extent * recvcount;

    if (drank != MPI_PROC_NULL) {
      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgather_cart: Recv from %d to %d", drank,
          rank);
      rc = PMPI_Irecv(recvbuf, recvcount, recvtype, drank, 2, comm,
                      &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;

      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgather_cart: Rank %d send to %d", rank,
          drank);
      rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, drank, 2, comm,
                      &reqs[nreqs + 1]);
      if (rc != MPI_SUCCESS)
        break;

      nreqs += 2;
    }

    recvbuf = (char *)recvbuf + extent * recvcount;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgather_graph(
    void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
    int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int i = 0;
  int rc = MPI_SUCCESS;
  int degree;
  int neighbor;
  int rank;
  const int *edges;
  MPI_Aint extent;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;

  PMPI_Comm_rank(comm, &rank);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  PMPI_Graph_neighbors_count(comm, rank, &degree);

  edges = topo->data.graph.edges;
  if (rank > 0) {
    edges += topo->data.graph.index[rank - 1];
  }
  PMPI_Type_extent(recvtype, &extent);
  reqs = sctk_malloc((2 * degree) * sizeof(MPI_Request *));

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    sctk_nodebug(
        "__INTERNAL__PMPI_Neighbor_allgather_graph: Recv from %d to %d",
        edges[neighbor], rank);
    rc = PMPI_Irecv(recvbuf, recvcount, recvtype, edges[neighbor], 1, comm,
                    &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
    recvbuf = (char *)recvbuf + extent * recvcount;

    sctk_nodebug(
        "__INTERNAL__PMPI_Neighbor_allgather_graph: Rank %d send to %d", rank,
        edges[neighbor]);
    rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, edges[neighbor], 1,
                    comm, &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
  }
  if (rc != MPI_SUCCESS)
    return rc;

  return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgatherv_cart(
    void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
    int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm) {
  MPI_Aint extent;
  int rank;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;
  int rc = MPI_SUCCESS, dim, nreqs = 0, i;
  PMPI_Comm_rank(comm, &rank);

  PMPI_Type_extent(recvtype, &extent);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  reqs = sctk_malloc((4 * (topo->data.cart.ndims)) * sizeof(MPI_Request *));

  for (dim = 0, i = 0, nreqs = 0; dim < topo->data.cart.ndims; ++dim, i += 2) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgatherv_cart: Recv from %d to %d",
          srank, rank);
      rc = PMPI_Irecv((char *)recvbuf + displs[i] * extent, recvcounts[i],
                      recvtype, srank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;

      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgatherv_cart: Rank %d send to %d", rank,
          srank);
      rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, srank, 2, comm,
                      &reqs[nreqs + 1]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs += 2;
    }

    if (drank != MPI_PROC_NULL) {
      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgatherv_cart: Recv from %d to %d",
          drank, rank);
      rc =
          PMPI_Irecv((char *)recvbuf + displs[i + 1] * extent,
                     recvcounts[i + 1], recvtype, drank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;

      sctk_nodebug(
          "__INTERNAL__PMPI_Neighbor_allgatherv_cart: Rank %d send to %d", rank,
          drank);
      rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, drank, 2, comm,
                      &reqs[nreqs + 1]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs += 2;
    }
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgatherv_graph(
    void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
    int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm) {
  int i = 0;
  int rc = MPI_SUCCESS;
  int degree;
  int neighbor;
  int rank;
  const int *edges;
  MPI_Aint extent;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;

  PMPI_Comm_rank(comm, &rank);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  PMPI_Graph_neighbors_count(comm, rank, &degree);

  edges = topo->data.graph.edges;
  if (rank > 0) {
    edges += topo->data.graph.index[rank - 1];
  }

  PMPI_Type_extent(recvtype, &extent);
  reqs = sctk_malloc((2 * degree) * sizeof(MPI_Request *));

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    sctk_nodebug(
        "__INTERNAL__PMPI_Neighbor_allgatherv_graph: Recv from %d to %d",
        edges[neighbor], rank);
    rc = PMPI_Irecv((char *)recvbuf + displs[neighbor] * extent,
                    recvcounts[neighbor], recvtype, edges[neighbor], 2, comm,
                    &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;

    sctk_nodebug(
        "__INTERNAL__PMPI_Neighbor_allgatherv_graph: Rank %d send to %d", rank,
        edges[neighbor]);
    rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, edges[neighbor], 2,
                    comm, &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
  }

  if (rc != MPI_SUCCESS)
    return rc;

  return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoall_cart(void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype,
                                                   void *recvbuf, int recvcount,
                                                   MPI_Datatype recvtype,
                                                   MPI_Comm comm) {
  MPI_Aint rdextent;
  MPI_Aint sdextent;
  int rank;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;
  int rc = MPI_SUCCESS, dim, nreqs = 0, i;
  PMPI_Comm_rank(comm, &rank);

  PMPI_Type_extent(recvtype, &rdextent);
  PMPI_Type_extent(recvtype, &sdextent);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  reqs = sctk_malloc((4 * (topo->data.cart.ndims)) * sizeof(MPI_Request *));

  for (dim = 0, nreqs = 0; dim < topo->data.cart.ndims; ++dim) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      rc = PMPI_Irecv(recvbuf, recvcount, recvtype, srank, 2, comm,
                      &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    recvbuf = (char *)recvbuf + rdextent * recvcount;

    if (drank != MPI_PROC_NULL) {
      rc = PMPI_Irecv(recvbuf, recvcount, recvtype, drank, 2, comm,
                      &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    recvbuf = (char *)recvbuf + rdextent * recvcount;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  for (dim = 0; dim < topo->data.cart.ndims; ++dim) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, srank, 2, comm,
                      &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    sendbuf = (char *)sendbuf + sdextent * sendcount;

    if (drank != MPI_PROC_NULL) {
      rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, drank, 2, comm,
                      &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    sendbuf = (char *)sendbuf + sdextent * sendcount;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoall_graph(
    void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
    int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int i = 0;
  int rc = MPI_SUCCESS;
  int degree;
  int neighbor;
  int rank;
  const int *edges;
  MPI_Aint rdextent;
  MPI_Aint sdextent;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;

  PMPI_Comm_rank(comm, &rank);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  PMPI_Graph_neighbors_count(comm, rank, &degree);

  edges = topo->data.graph.edges;
  if (rank > 0) {
    edges += topo->data.graph.index[rank - 1];
  }

  PMPI_Type_extent(recvtype, &rdextent);
  PMPI_Type_extent(sendtype, &sdextent);
  reqs = sctk_malloc((2 * degree) * sizeof(MPI_Request *));

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    rc = PMPI_Irecv(recvbuf, recvcount, recvtype, edges[neighbor], 1, comm,
                    &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
    recvbuf = (char *)recvbuf + rdextent * recvcount;
  }

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, edges[neighbor], 1,
                    comm, &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
    sendbuf = (char *)sendbuf + sdextent * sendcount;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallv_cart(
    void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype,
    void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype,
    MPI_Comm comm) {
  MPI_Aint rdextent;
  MPI_Aint sdextent;
  int rank;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;
  int rc = MPI_SUCCESS, dim, nreqs = 0, i;
  PMPI_Comm_rank(comm, &rank);

  PMPI_Type_extent(recvtype, &rdextent);
  PMPI_Type_extent(recvtype, &sdextent);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  reqs = sctk_malloc((4 * (topo->data.cart.ndims)) * sizeof(MPI_Request *));

  for (dim = 0, nreqs = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      rc = PMPI_Irecv((char *)recvbuf + rdispls[i] * rdextent, recvcounts[i],
                      recvtype, srank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    if (drank != MPI_PROC_NULL) {
      rc =
          PMPI_Irecv((char *)recvbuf + rdispls[i + 1] * rdextent,
                     recvcounts[i + 1], recvtype, drank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  for (dim = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      rc = PMPI_Isend((char *)sendbuf + sdispls[i] * sdextent, sendcounts[i],
                      sendtype, srank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    if (drank != MPI_PROC_NULL) {
      rc =
          PMPI_Isend((char *)sendbuf + sdispls[i + 1] * sdextent,
                     sendcounts[i + 1], sendtype, drank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallv_graph(
    void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype,
    void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype,
    MPI_Comm comm) {
  int i = 0;
  int rc = MPI_SUCCESS;
  int degree;
  int neighbor;
  int rank;
  const int *edges;
  MPI_Aint rdextent;
  MPI_Aint sdextent;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;

  PMPI_Comm_rank(comm, &rank);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  PMPI_Graph_neighbors_count(comm, rank, &degree);

  edges = topo->data.graph.edges;
  if (rank > 0) {
    edges += topo->data.graph.index[rank - 1];
  }

  PMPI_Type_extent(recvtype, &rdextent);
  PMPI_Type_extent(sendtype, &sdextent);
  reqs = sctk_malloc((2 * degree) * sizeof(MPI_Request *));

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    rc = PMPI_Irecv((char *)recvbuf + rdispls[neighbor] * rdextent,
                    recvcounts[neighbor], recvtype, edges[neighbor], 1, comm,
                    &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    rc = PMPI_Isend((char *)sendbuf + sdispls[neighbor] * sdextent,
                    sendcounts[neighbor], sendtype, edges[neighbor], 1, comm,
                    &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallw_cart(
    void *sendbuf, int sendcounts[], MPI_Aint sdispls[],
    MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[],
    MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm) {
  int rank;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;
  int rc = MPI_SUCCESS, dim, nreqs = 0, i;
  PMPI_Comm_rank(comm, &rank);

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  reqs = sctk_malloc((4 * (topo->data.cart.ndims)) * sizeof(MPI_Request *));

  for (dim = 0, nreqs = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      rc = PMPI_Irecv((char *)recvbuf + rdispls[i], recvcounts[i], recvtypes[i],
                      srank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    if (drank != MPI_PROC_NULL) {
      rc = PMPI_Irecv((char *)recvbuf + rdispls[i + 1], recvcounts[i + 1],
                      recvtypes[i + 1], drank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  for (dim = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2) {
    int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

    if (topo->data.cart.dims[dim] > 1) {
      PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
    } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
      srank = drank = rank;
    }

    if (srank != MPI_PROC_NULL) {
      rc = PMPI_Isend((char *)sendbuf + sdispls[i], sendcounts[i], sendtypes[i],
                      srank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }

    if (drank != MPI_PROC_NULL) {
      rc = PMPI_Isend((char *)sendbuf + sdispls[i + 1], sendcounts[i + 1],
                      sendtypes[i + 1], drank, 2, comm, &reqs[nreqs]);
      if (rc != MPI_SUCCESS)
        break;
      nreqs++;
    }
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallw_graph(
    void *sendbuf, int sendcounts[], MPI_Aint sdispls[],
    MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[],
    MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm) {
  int i = 0;
  int rc = MPI_SUCCESS;
  int degree;
  int neighbor;
  int rank;
  const int *edges;
  MPI_Request *reqs;
  mpi_topology_per_comm_t *topo;
  mpc_mpi_per_communicator_t *tmp;

  PMPI_Comm_rank(comm, &rank);
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);
  PMPI_Graph_neighbors_count(comm, rank, &degree);

  edges = topo->data.graph.edges;
  if (rank > 0) {
    edges += topo->data.graph.index[rank - 1];
  }

  reqs = sctk_malloc((2 * degree) * sizeof(MPI_Request *));

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    rc = PMPI_Irecv((char *)recvbuf + rdispls[neighbor], recvcounts[neighbor],
                    recvtypes[neighbor], edges[neighbor], 1, comm, &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  for (neighbor = 0; neighbor < degree; ++neighbor) {
    rc = PMPI_Isend((char *)sendbuf + sdispls[neighbor], sendcounts[neighbor],
                    sendtypes[neighbor], edges[neighbor], 1, comm, &reqs[i]);
    if (rc != MPI_SUCCESS)
      break;
    i++;
  }

  if (rc != MPI_SUCCESS) {
    return rc;
  }

  return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

struct sctk_mpi_ops_s {
  sctk_op_t *ops;
  int size;
  sctk_spinlock_t lock;
};

static sctk_op_t defined_op[MAX_MPI_DEFINED_OP];

#define sctk_add_op(operation)                                                 \
  defined_op[MPI_##operation].op = MPC_##operation;                            \
  defined_op[MPI_##operation].used = 1;                                        \
  defined_op[MPI_##operation].commute = 1;

void __sctk_init_mpi_op() {
  sctk_mpi_ops_t *ops;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  static volatile int done = 0;
  int i;

  ops = (sctk_mpi_ops_t *)sctk_malloc(sizeof(sctk_mpi_ops_t));
  ops->size = 0;
  ops->ops = NULL;
  ops->lock = 0;

  sctk_thread_mutex_lock(&lock);
  if (done == 0) {
    for (i = 0; i < MAX_MPI_DEFINED_OP; i++) {
      defined_op[i].used = 0;
    }

    sctk_add_op(SUM);
    sctk_add_op(MAX);
    sctk_add_op(MIN);
    sctk_add_op(PROD);
    sctk_add_op(LAND);
    sctk_add_op(BAND);
    sctk_add_op(LOR);
    sctk_add_op(BOR);
    sctk_add_op(LXOR);
    sctk_add_op(BXOR);
    sctk_add_op(MINLOC);
    sctk_add_op(MAXLOC);
    done = 1;
  }
  sctk_thread_mutex_unlock(&lock);

#ifdef MPC_PosixAllocator
  sctk_add_global_var(defined_op, sizeof(defined_op));
#endif

  PMPC_Set_op(ops);
}

sctk_op_t *sctk_convert_to_mpc_op(MPI_Op op) {
  sctk_mpi_ops_t *ops;
  sctk_op_t *tmp;

  if (op < MAX_MPI_DEFINED_OP) {
    assume(defined_op[op].used == 1);
    return &(defined_op[op]);
  }

  PMPC_Get_op(&ops);
  sctk_spinlock_lock(&(ops->lock));

  op -= MAX_MPI_DEFINED_OP;
  assume(op < ops->size);
  assume(ops->ops[op].used == 1);

  tmp = &(ops->ops[op]);

  sctk_spinlock_unlock(&(ops->lock));
  return tmp;
}

#define ADD_FUNC_HANDLER(func, t, op)                                          \
  case t:                                                                      \
    op = (MPC_Op_f)func##_##t;                                                 \
    break
#define COMPAT_DATA_TYPE(op, func)                                             \
  if (op == func) {                                                            \
    switch (datatype) {                                                        \
      ADD_FUNC_HANDLER(func, MPC_SIGNED_CHAR, op);                             \
      ADD_FUNC_HANDLER(func, MPC_CHAR, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_CHARACTER, op);                               \
      ADD_FUNC_HANDLER(func, MPC_BYTE, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_SHORT, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_INT, op);                                     \
      ADD_FUNC_HANDLER(func, MPC_INTEGER, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_LONG, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_FLOAT, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_INTEGER1, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INTEGER2, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INTEGER4, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INTEGER8, op);                                \
      ADD_FUNC_HANDLER(func, MPC_REAL, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_REAL4, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_REAL8, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_REAL16, op);                                  \
      ADD_FUNC_HANDLER(func, MPC_DOUBLE, op);                                  \
      ADD_FUNC_HANDLER(func, MPC_DOUBLE_PRECISION, op);                        \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_CHAR, op);                           \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_SHORT, op);                          \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_LONG, op);                           \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_LONG_LONG, op);                      \
      ADD_FUNC_HANDLER(func, MPC_LONG_LONG_INT, op);                           \
      ADD_FUNC_HANDLER(func, MPC_LONG_DOUBLE, op);                             \
      ADD_FUNC_HANDLER(func, MPC_LONG_LONG, op);                               \
      ADD_FUNC_HANDLER(func, MPC_DOUBLE_COMPLEX, op);                          \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX8, op);                    \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX16, op);                         \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX32, op);                         \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_LONG_LONG_INT, op);                  \
      ADD_FUNC_HANDLER(func, MPC_UINT8_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_UINT16_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UINT32_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UINT64_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INT8_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INT16_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INT32_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INT64_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_COUNT, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_AINT, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_OFFSET, op);                                \
    default:                                                                   \
      not_reachable();                                                         \
    }                                                                          \
  }

#define COMPAT_DATA_TYPE2(op, func)                                            \
  if (op == func) {                                                            \
    switch (datatype) {                                                        \
      ADD_FUNC_HANDLER(func, MPC_SIGNED_CHAR, op);                             \
      ADD_FUNC_HANDLER(func, MPC_CHAR, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_BYTE, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_SHORT, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_INT, op);                                     \
      ADD_FUNC_HANDLER(func, MPC_INTEGER, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INTEGER1, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INTEGER2, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INTEGER4, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INTEGER8, op);                                \
      ADD_FUNC_HANDLER(func, MPC_LONG, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_LONG_LONG, op);                               \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_CHAR, op);                           \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_SHORT, op);                          \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UNSIGNED_LONG, op);                           \
      ADD_FUNC_HANDLER(func, MPC_LOGICAL, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_UINT8_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UINT16_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UINT32_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_UINT64_T, op);                                \
      ADD_FUNC_HANDLER(func, MPC_INT8_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INT16_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INT32_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_INT64_T, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_COUNT, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_AINT, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_OFFSET, op);                                  \  
      ADD_FUNC_HANDLER(func, MPC_C_BOOL,op);                                   \           
    default:                                                                   \
      not_reachable();                                                         \
    }                                                                          \
  }

#define COMPAT_DATA_TYPE3(op, func)                                            \
  if (op == func) {                                                            \
    switch (datatype) {                                                        \
      ADD_FUNC_HANDLER(func, MPC_FLOAT_INT, op);                               \
      ADD_FUNC_HANDLER(func, MPC_LONG_INT, op);                                \
      ADD_FUNC_HANDLER(func, MPC_DOUBLE_INT, op);                              \
      ADD_FUNC_HANDLER(func, MPC_LONG_DOUBLE_INT, op);                         \
      ADD_FUNC_HANDLER(func, MPC_SHORT_INT, op);                               \
      ADD_FUNC_HANDLER(func, MPC_2INT, op);                                    \
      ADD_FUNC_HANDLER(func, MPC_2FLOAT, op);                                  \
      ADD_FUNC_HANDLER(func, MPC_2INTEGER, op);                                \
      ADD_FUNC_HANDLER(func, MPC_2REAL, op);                                   \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX, op);                                 \
      ADD_FUNC_HANDLER(func, MPC_2DOUBLE_PRECISION, op);                       \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX8, op);                                \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX16, op);                               \
      ADD_FUNC_HANDLER(func, MPC_DOUBLE_COMPLEX, op);                          \
      ADD_FUNC_HANDLER(func, MPC_COMPLEX32, op);                               \
    default:                                                                   \
      not_reachable();                                                         \
    }                                                                          \
  }

MPC_Op_f sctk_get_common_function(MPC_Datatype datatype, MPC_Op op) {
  MPC_Op_f func;

  func = op.func;

  /*Internals function */
  COMPAT_DATA_TYPE(func, MPC_SUM_func)
  else COMPAT_DATA_TYPE(func, MPC_MAX_func) else COMPAT_DATA_TYPE(func, MPC_MIN_func) else COMPAT_DATA_TYPE(func, MPC_PROD_func) else COMPAT_DATA_TYPE2(func, MPC_BAND_func) else COMPAT_DATA_TYPE2(func, MPC_LAND_func) else COMPAT_DATA_TYPE2(
      func,
      MPC_BXOR_func) else COMPAT_DATA_TYPE2(func,
                                            MPC_LXOR_func) else COMPAT_DATA_TYPE2(func,
                                                                                  MPC_BOR_func) else COMPAT_DATA_TYPE2(func,
                                                                                                                       MPC_LOR_func) else COMPAT_DATA_TYPE3(func,
                                                                                                                                                            MPC_MAXLOC_func) else COMPAT_DATA_TYPE3(func,
                                                                                                                                                                                                    MPC_MINLOC_func) else {
    sctk_error("No such operation");
    sctk_abort();
  }

  return func;
}

#define MPI_SHM_OP_SUM(t)                                                      \
  for (i = 0; i < size; i++) {                                                 \
    for (j = 0; j < count; j++)                                                \
      res.t[j] += b[i].t[j];                                                   \
  }

#define MPI_SHM_OP_PROD(t)                                                     \
  for (i = 0; i < size; i++) {                                                 \
    for (j = 0; j < count; j++)                                                \
      res.t[j] *= b[i].t[j];                                                   \
  }

static inline void
sctk_mpi_shared_mem_buffer_collect(union shared_mem_buffer *b,
                                   MPI_Datatype type, int count, MPI_Op op,
                                   void *dest, int size) {
  int i, j;
  union shared_mem_buffer res = {0};
  size_t tsize = 0;

  switch (type) {
  case MPI_INT:
    tsize = sizeof(int);
    break;
  case MPI_FLOAT:
    tsize = sizeof(float);
    break;
  case MPI_CHAR:
    tsize = sizeof(char);
    break;

  case MPI_DOUBLE:
    tsize = sizeof(double);
    break;

  default:
    sctk_fatal("Unsupported data-type");
  }

  switch (op) {
  case MPI_SUM:
    switch (type) {
    case MPI_INT:
      MPI_SHM_OP_SUM(i)
      break;
    case MPI_FLOAT:
      MPI_SHM_OP_SUM(f)
      break;
    case MPI_CHAR:
      MPI_SHM_OP_SUM(c)
      break;

    case MPI_DOUBLE:
      MPI_SHM_OP_SUM(d)
      break;

    default:
      sctk_fatal("Unsupported data-type");
    }
    break;
  case MPI_PROD:
    switch (type) {
    case MPI_INT:
      MPI_SHM_OP_PROD(i)
      break;
    case MPI_FLOAT:
      MPI_SHM_OP_PROD(f)
      break;
    case MPI_CHAR:
      MPI_SHM_OP_PROD(c)
      break;

    case MPI_DOUBLE:
      MPI_SHM_OP_PROD(d)
      break;

    default:
      sctk_fatal("Unsupported data-type");
    }
    break;
  }

  sctk_mpi_shared_mem_buffer_get(&res, type, count, dest, count * tsize);
}

static inline int __INTERNAL__PMPI_Reduce_derived_no_commute_ring(
		void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
		int root, MPI_Comm comm, MPC_Op mpc_op, sctk_op_t *mpi_op, int size,
		int rank) {
	int res;
	void *tmp_buf;
	int allocated = 0;
	int is_MPI_IN_PLACE = 0;
	MPC_Op_f func;

	tmp_buf = recvbuf;

	not_implemented();

	if ((rank != root) || (sendbuf == MPI_IN_PLACE)) {
		MPI_Aint dsize;
		res = __INTERNAL__PMPI_Type_extent(datatype, &dsize);
		if (res != MPI_SUCCESS) {
			return res;
		}

		tmp_buf = sctk_malloc(count * dsize);
		if (sendbuf == MPI_IN_PLACE)
			sendbuf = recvbuf;
		allocated = 1;
		is_MPI_IN_PLACE = 1;
	}

	if (rank == size - 1) {
		res = __INTERNAL__PMPI_Send(sendbuf, count, datatype,
				(rank + size - 1) % size, MPC_REDUCE_TAG, comm);
		if (res != MPI_SUCCESS) {
			return res;
		}
	} else {
		res = __INTERNAL__PMPI_Recv(tmp_buf, count, datatype, (rank + 1) % size,
				MPC_REDUCE_TAG, comm, MPI_STATUS_IGNORE);
		if (res != MPI_SUCCESS) {
			return res;
		}
	}

	if (rank != size - 1) {
		if (mpc_op.u_func != NULL) {
			mpc_op.u_func(sendbuf, tmp_buf, &count, &datatype);
		} else {
			MPC_Op_f func;
			func = sctk_get_common_function(datatype, mpc_op);
			func(sendbuf, tmp_buf, count, datatype);
		}
	}

	if ((rank == 0) && (root != 0)) {
		res = __INTERNAL__PMPI_Send(tmp_buf, count, datatype, root, MPC_REDUCE_TAG,
				comm);
		if (res != MPI_SUCCESS) {
			return res;
		}
	} else {
		if ((rank != size - 1) && ((rank != 0))) {
			res =
				__INTERNAL__PMPI_Send(tmp_buf, count, datatype,
						(rank + size - 1) % size, MPC_REDUCE_TAG, comm);
			if (res != MPI_SUCCESS) {
				return res;
			}
		}
	}

	if ((rank == 0) && (root == 0))
	{
		if (is_MPI_IN_PLACE == 1)
		{
			MPI_Request request_send;
			MPI_Request request_recv;

			res = __INTERNAL__PMPI_Isend(tmp_buf, count, datatype, 0, MPC_REDUCE_TAG,
					comm, &request_send);
			if (res != MPI_SUCCESS) {
				return res;
			}

			res = __INTERNAL__PMPI_Irecv(recvbuf, count, datatype, 0, MPC_REDUCE_TAG,
					comm, &request_recv);
			if (res != MPI_SUCCESS) {
				return res;
			}

			res = __INTERNAL__PMPI_Wait(&(request_recv), MPI_STATUS_IGNORE);
			if (res != MPI_SUCCESS) {
				return res;
			}

			res = __INTERNAL__PMPI_Wait(&(request_send), MPI_STATUS_IGNORE);
			if (res != MPI_SUCCESS) {
				return res;
			}
		}
	}

	if (allocated == 1) {
		sctk_free(tmp_buf);
	}

	return res;
}

#define MAX_FOR_RED_STATIC_BUFF 4096
#define MAX_FOR_RED_STATIC_REQ 24

static inline int __INTERNAL__PMPI_Reduce_derived_no_commute_for(
		void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
		int root, MPI_Comm comm, MPC_Op mpc_op, sctk_op_t *mpi_op, int size,
		int rank) {

	int res;

  if(sendbuf == MPI_IN_PLACE)
  {
    sendbuf = recvbuf;
  }

	if( rank == root )
	{
		void * sumbuff = NULL;
		int root_did_alloc = 0;
		char red_buffer[MAX_FOR_RED_STATIC_BUFF];
	
		MPI_Aint dsize;
	
		res = __INTERNAL__PMPI_Type_extent(datatype, &dsize);
		
		if (res != MPI_SUCCESS) {
			return res;
		}
	

		size_t blob = dsize * count;
	
		if( MAX_FOR_RED_STATIC_BUFF <= (blob * size) )
		{

			sumbuff = sctk_malloc( blob * size );

			if( !sumbuff )
			{
				perror("malloc");
				return MPI_ERR_INTERN;
			}

		}
		else
		{
			sumbuff = red_buffer;
		}

		MPI_Request staticrreqs[MAX_FOR_RED_STATIC_REQ];
		MPI_Request * rreqs = NULL;

		if( MAX_FOR_RED_STATIC_REQ <=  size )
		{
			rreqs = sctk_malloc( (size-1) * sizeof(MPI_Request) );

			if( !rreqs )
			{
				perror("malloc");
				return MPI_ERR_INTERN;
			}

		}
		else
		{
			rreqs = staticrreqs;
		}

		int i;
    int cnt = 0;
		for( i = 0 ; i  < size ; i++ )
		{
			if(i == root)
				continue;
			__INTERNAL__PMPI_Irecv( sumbuff + (blob * i) , count , datatype , i , MPC_REDUCE_TAG, comm , &rreqs[cnt] );
      cnt++;
		}


		__INTERNAL__PMPI_Waitall( size -1 , rreqs , MPI_STATUSES_IGNORE );

		if(rreqs != staticrreqs)
		{
			sctk_free(rreqs);
		}

		int j;

    /* These are the fastpaths */
    if( (datatype == MPI_FLOAT) && (op = MPI_SUM) )
    {
      if( sendbuf != MPI_IN_PLACE )
      {
        memcpy( recvbuf, sendbuf , blob );
      }

      for( i = 1 ; i < size ; i ++ )
      {
        float * fsrc = (float *)sumbuff + (blob * i);

        for( j = 0 ; j < count ; j++ )
        {
          ((float*)recvbuf)[i] += fsrc[i];
        }

      }

    }
    else if( (datatype == MPI_DOUBLE) && (op = MPI_SUM) )
    {
      if( sendbuf != MPI_IN_PLACE )
      {
        memcpy( recvbuf, sendbuf , blob );
      }

      for( i = 1 ; i < size ; i ++ )
      {
        double * dsrc = (double *)sumbuff + (blob * i);

        for( j = 0 ; j < count ; j++ )
        {
          ((double*)recvbuf)[i] += dsrc[i];
        }

      }
    }
    else if( (datatype == MPI_INT) && (op = MPI_SUM) )
    {
      if( sendbuf != MPI_IN_PLACE )
      {
        memcpy( recvbuf, sendbuf , blob );
      }

      for( i = 1 ; i < size ; i ++ )
      {
        int * isrc = (int *)sumbuff + (blob * i);

        for( j = 0 ; j < count ; j++ )
        {
          ((int*)recvbuf)[i] += isrc[i];
        }

      }
    }
    else
    {
      /* This is the generic Slow-Path */
      memcpy(sumbuff + (root * blob),sendbuf, blob);

      for(i=1 ; i<size ; i++)
      {
        if (mpc_op.u_func != NULL) {
          mpc_op.u_func( sumbuff + (blob * (i-1)),  sumbuff + (blob * i), &count, &datatype);
        } else {
          MPC_Op_f func;
          func = sctk_get_common_function(datatype, mpc_op);
          func(sumbuff + (blob * (i-1)), sumbuff + (blob * i), count, datatype);
        }
      }
      memcpy(recvbuf, sumbuff + (blob * (size -1)), blob);
    }
    if( MAX_FOR_RED_STATIC_BUFF <= blob * size )
    {
      sctk_free(sumbuff);
    }
  }
  else
  {
    res = __INTERNAL__PMPI_Send( sendbuf , count , datatype , root , MPC_REDUCE_TAG , comm );

    if (res != MPI_SUCCESS) {
      return res;
    }

	}


	return MPI_SUCCESS;
}




static inline int __INTERNAL__PMPI_Reduce_derived_no_commute(
		void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
		int root, MPI_Comm comm, MPC_Op mpc_op, sctk_op_t *mpi_op, int size,
		int rank) {

	int res;

	if( 1 || (size < sctk_runtime_config_get()->modules.collectives_intra.reduce_intra_for_trsh)
	&&  sctk_datatype_contig_mem(datatype)
	&&  (count <  sctk_runtime_config_get()->modules.collectives_intra.reduce_intra_for_count_trsh) )
	{
		res = __INTERNAL__PMPI_Reduce_derived_no_commute_for(
				sendbuf, recvbuf, count, datatype, op,
				root, comm, mpc_op, mpi_op, size, rank);
	}
	else
	{
		res = __INTERNAL__PMPI_Reduce_derived_no_commute_ring(
				sendbuf, recvbuf, count, datatype, op,
				root, comm, mpc_op, mpi_op, size, rank);
	}

	return res;
}


#define MPI_RED_TREE_STATIC_BUFF 1024

static inline int
__INTERNAL__PMPI_Reduce_derived_commute(void *sendbuf, void *recvbuf, int count,
		MPI_Datatype datatype, MPI_Op op,
		int root, MPI_Comm comm, MPC_Op mpc_op,
		sctk_op_t *mpi_op, int size, int rank) {
	int res;

    /* Temporary buffers for LC & RC contributions */
	char st_buff1[MPI_RED_TREE_STATIC_BUFF];
	char st_buff2[MPI_RED_TREE_STATIC_BUFF];
	void * tBuffLC = NULL;
	void * tBuffRC = NULL;

    /* Buffer of local reduction result */
	void * tBuffRes = NULL;

	MPI_Aint dsize;
	
	res = __INTERNAL__PMPI_Type_extent(datatype, &dsize);
		
	if (res != MPI_SUCCESS) {
		return res;
	}

    /* Do we need to dynamically allocate memory? */
    int allocated = count * dsize < MPI_RED_TREE_STATIC_BUFF ? 0 : 1;

    /* Retrieve operation function */
    MPC_Op_f func = NULL;
    if (mpc_op.u_func == NULL)
        func = sctk_get_common_function(datatype, mpc_op);

    /* Are we in place at root? */
    if( rank == root )
    {
        tBuffRes = recvbuf;

        if( sendbuf != MPI_IN_PLACE )
          memcpy(tBuffRes, sendbuf, count * dsize);
    }

    /* Calculate new rank when root != 0 */
    if( 0 < root )
        rank = ( rank - root + size ) % size;

    /* Compute the btree */
    int parent = ((rank + 1)/ 2) -1;
    int lc = (rank + 1) * 2 - 1;
    int rc = (rank + 1) * 2;

    if( rank == 0 )
        parent = -1;

    if( size <= lc && size <= rc )
    {
        lc = -1;
        rc = -1;
        tBuffRes = sendbuf;
    }
    else if( size > lc && size > rc )
    {
        if( allocated )
        {
            tBuffLC = sctk_malloc( 2 * count * dsize );

            if( !tBuffLC )
            {
                perror("malloc");
                return MPI_ERR_INTERN;
            }

            tBuffRC = (char*) tBuffLC + count * dsize;
        }
        else
        {
            tBuffLC = (void *)st_buff1;
            tBuffRC = (void *)st_buff2;
        }
    }
    else if( size > lc )
    {
        /* No right child */
        rc = -1;

        if( allocated )
        {
            tBuffLC = sctk_malloc( count * dsize );

            if( !tBuffLC )
            {
                perror("malloc");
                return MPI_ERR_INTERN;
            }
        }
        else
            tBuffLC = (void *)st_buff1;
    }
    else
    {
        /* No left child */
        lc = -1;

        if( allocated )
        {
            tBuffLC = sctk_malloc( count * dsize );

            if( !tBuffLC )
            {
                perror("malloc");
                return MPI_ERR_INTERN;
            }

            tBuffRC = tBuffLC;
        }
        else
            tBuffRC = (void *)st_buff2;
    }

	/* Post the RECV when needed */
	MPI_Request rlc, rrc;
	int rlcc = 0, rrcc = 0;

	if( 0 <= lc )
	{
		__INTERNAL__PMPI_Irecv( tBuffLC, count, datatype,
                                (lc + root) % size, MPC_REDUCE_TAG, comm, &rlc );	
	}
	else
	{
		rlcc = 1;		
	}

	if( 0 <= rc )
	{
		__INTERNAL__PMPI_Irecv( tBuffRC, count, datatype,
                                (rc + root) % size, MPC_REDUCE_TAG, comm, &rrc );	
	}
	else
	{
		rrcc = 1;
	}

	while( !rlcc || !rrcc )
	{
		if( !rlcc && (0 <= lc) )
		{
			__INTERNAL__PMPI_Test( &rlc , &rlcc , MPI_STATUS_IGNORE );
		
			/* We just completed LC in TMPBUFF */
			if( rlcc )
			{
                /* Local contribution already accumulated */
                if( tBuffRes )
                {
                    if( func == NULL )
                        mpc_op.u_func(tBuffLC, tBuffRes, &count, &datatype);
                    else
                        func(tBuffLC, tBuffRes, count, datatype);
                }
                else
                {
                    if( func == NULL )
                        mpc_op.u_func(sendbuf, tBuffLC, &count, &datatype);
                    else
                        func(sendbuf, tBuffLC, count, datatype);

                    tBuffRes = tBuffLC;
                }
			}
		}

		if( !rrcc && (0 <= rc) )
		{
			__INTERNAL__PMPI_Test( &rrc , &rrcc , MPI_STATUS_IGNORE );
		
			if( rrcc )
			{
                /* Local contribution already accumulated */
                if( tBuffRes )
                {
                    if( func == NULL )
                        mpc_op.u_func(tBuffRC, tBuffRes, &count, &datatype);
                    else
                        func(tBuffRC, tBuffRes, count, datatype);
                }
                else
                {
                    if( func == NULL )
                        mpc_op.u_func(sendbuf, tBuffRC, &count, &datatype);
                    else
                        func(sendbuf, tBuffRC, count, datatype);

                    tBuffRes = tBuffRC;
                }
			}
		}
	}

	/* Now that we accumulated the childs lets move to parent */
	if( 0 <= parent )
	{
		__INTERNAL__PMPI_Send( tBuffRes, count, datatype, 
                               (parent + root) % size, MPC_REDUCE_TAG, comm );
	}

	if( allocated )
		sctk_free( tBuffLC );

	return MPI_SUCCESS;

}

int __INTERNAL__PMPI_Reduce_shm(void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root,
                                MPI_Comm comm) {
  struct sctk_comm_coll *coll = sctk_communicator_get_coll(comm);

  int rank;
  __INTERNAL__PMPI_Comm_rank(comm, &rank);

  struct shared_mem_reduce *reduce_ctx = sctk_comm_coll_get_red( coll, rank);

  int res;
  MPI_Aint tsize = 0;
  res = __INTERNAL__PMPI_Type_extent(datatype, &tsize);
  if (res != MPI_SUCCESS) {
    return res;
  }

  /* Only set when going through FP */
  int used_fast_path = 0;

  int i, size;
  size = coll->comm_size;

  /* Handle in-place */
  if (sendbuf == MPI_IN_PLACE) {
    /* We will use the local recv data in root
     * to reduce in tmp_buff */
    sendbuf = recvbuf;
  }

  /* First pay the toll gate */

  if (__do_yield) {
    while (reduce_ctx->tollgate[rank] !=
           sctk_atomics_load_int(&reduce_ctx->fare)) {
      sctk_thread_yield();
    }
  } else {
    while (reduce_ctx->tollgate[rank] !=
           sctk_atomics_load_int(&reduce_ctx->fare)) {
      sctk_cpu_relax();
    }
  }

  /* Reverse state so that only a root done can unlock by
   * also reversing the fare */
  reduce_ctx->tollgate[rank] = !reduce_ctx->tollgate[rank];

  void *data_buff = sendbuf;
  void *result_buff = recvbuf;
  int allocated = 0;

  // sctk_error("OP %d T %d CONT %d FROM %p to %p (%p, %p)", op, datatype, count
  // , sendbuf, recvbuf, data_buff, result_buff );

  int will_be_in_shm_buff = sctk_mpi_type_is_shared_mem(datatype, count) &&
                            (sctk_mpi_op_is_shared_mem(op));
  int is_contig_type = sctk_datatype_contig_mem(datatype);

  /* Do we need to pack ? */
  if (!is_contig_type) {
    /* We have a tmp bufer where to reduce */
    data_buff = sctk_malloc(count * tsize);

    assume(data_buff != NULL);

    /* If non-contig, we need to pack to the TMP buffer
     * where the reduction will be operated */
    int cnt = 0;
    PMPI_Pack(sendbuf, count, datatype, data_buff, tsize * count, &cnt, comm);
    /* We had to allocate the segment */
    allocated = 1;

  } else {
    if ((rank == root) && (sendbuf != MPI_IN_PLACE) && !will_be_in_shm_buff)
      memcpy(result_buff, data_buff, count * tsize);
  }

  /* Root RDV phase */

  if (root == rank) {
    if (__do_yield) {
      while (sctk_atomics_cas_int(&reduce_ctx->owner, -1, -2) != -1) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_cas_int(&reduce_ctx->owner, -1, -2) != -1) {
        sctk_cpu_relax();
      }
    }

    /* Set the local infos */

    /* Now put in the CTX where we would like to reduce */
    if (is_contig_type) {
      reduce_ctx->target_buff = result_buff;
    } else {
      reduce_ctx->target_buff = data_buff;
    }

    /* Now unleash the others */
    sctk_atomics_store_int(&reduce_ctx->owner, rank);

  } else {
    if (__do_yield) {
      while (sctk_atomics_load_int(&reduce_ctx->owner) != root) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_load_int(&reduce_ctx->owner) != root) {
        sctk_cpu_relax();
      }
    }
  }

  /* This is the TMP buffer case fastpath */
  if (will_be_in_shm_buff) {
    /* Set my value in the TMB buffer */
    sctk_mpi_shared_mem_buffer_fill(&reduce_ctx->buffer[rank], datatype, count,
                                    data_buff);

    goto SHM_REDUCE_DONE;
  }

  /* If we are here the buffer is probably too large */
  size_t reduce_pipelined_tresh =
      sctk_runtime_config_get()->modules.collectives_shm.reduce_pipelined_tresh;
  size_t reduce_force_nocommute =
      sctk_runtime_config_get()->modules.collectives_shm.coll_force_nocommute;

  sctk_op_t *mpi_op = sctk_convert_to_mpc_op(op);
  MPC_Op mpc_op = mpi_op->op;

  if ( sctk_op_can_commute(mpi_op, datatype) || reduce_force_nocommute) {

    /* Is the target buffer large enough ? */
    if (reduce_pipelined_tresh <= (count * tsize)) {
      int per_lock = count / reduce_ctx->pipelined_blocks;
      int rest = count % per_lock;

      size_t stripe_offset = tsize * per_lock;

      int i;
      int rest_done = 0;

      for (i = 0; i < reduce_ctx->pipelined_blocks; i++) {
        /* Now process the core */
        int target_cell = (rank + i) % reduce_ctx->pipelined_blocks;

        void *from = data_buff + target_cell * stripe_offset;
        void *to = reduce_ctx->target_buff + target_cell * stripe_offset;

        /* As we want to process in order we have
         * to notify the next rank to allow it
         * to start processing the block he have just done */
        if (rank != 0) {
          int dummy_go;
          PMPC_Recv(&dummy_go, 1, MPI_INT, rank - 1, MPC_REDUCE_TAG, comm,
                    MPI_STATUS_IGNORE);
        }

        if (rank != root) {
          sctk_spinlock_lock_yield(&reduce_ctx->buff_lock[target_cell]);

          if (mpc_op.u_func != NULL) {
            mpc_op.u_func(from, to, &per_lock, &datatype);
          } else {
            MPC_Op_f func;
            func = sctk_get_common_function(datatype, mpc_op);
            func(from, to, per_lock, datatype);
          }

          sctk_spinlock_unlock(&reduce_ctx->buff_lock[target_cell]);

          if (rest_done == 0) {
            from = data_buff + reduce_ctx->pipelined_blocks * stripe_offset;
            to = reduce_ctx->target_buff +
                 reduce_ctx->pipelined_blocks * stripe_offset;

            if (mpc_op.u_func != NULL) {
              mpc_op.u_func(from, to, &rest, &datatype);
            } else {
              MPC_Op_f func;
              func = sctk_get_common_function(datatype, mpc_op);
              func(from, to, rest, datatype);
            }

            rest_done = 1;
          }
        }

        if (rank != (size - 1)) {
          int dummy_go;
          PMPC_Send(&dummy_go, 1, MPI_INT, rank + 1, MPC_REDUCE_TAG, comm);
        }
      }

      if (rank == root) {
        /* Wait for the GO in order */

        if (__do_yield) {

          while (sctk_atomics_load_int(&reduce_ctx->left_to_push) != 1) {
            sctk_thread_yield();
          }

        } else {

          while (sctk_atomics_load_int(&reduce_ctx->left_to_push) != 1) {
            sctk_cpu_relax();
          }
        }
      }
    } else {
      /* Wait for the GO in order */

      if (__do_yield) {
        while (sctk_atomics_load_int(&reduce_ctx->left_to_push) != (rank + 1)) {
          sctk_thread_yield();
        }
      } else {
        while (sctk_atomics_load_int(&reduce_ctx->left_to_push) != (rank + 1)) {
          sctk_cpu_relax();
        }
      }

      if (rank != root) {
        if (mpc_op.u_func != NULL) {
          mpc_op.u_func(data_buff, reduce_ctx->target_buff, &count, &datatype);
        } else {
          MPC_Op_f func;
          func = sctk_get_common_function(datatype, mpc_op);
          func(data_buff, reduce_ctx->target_buff, count, datatype);
        }
      }
    }

  } else {
    if (rank != root) {

      if (reduce_pipelined_tresh < (count * tsize)) {
        int per_lock = count / reduce_ctx->pipelined_blocks;
        int rest = count % per_lock;

        size_t stripe_offset = tsize * per_lock;

        int i;
        int rest_done = 0;

        for (i = 0; i < reduce_ctx->pipelined_blocks; i++) {
          /* Now process the rest (only once) */
          if ((rank % reduce_ctx->pipelined_blocks) == i) {
            if ((rest != 0) && (rest_done == 0)) {
              // sctk_error("THe rest %d over %d divided by %d", rest, count,
              // SHM_COLL_BUFF_LOCKS);
              void *from =
                  data_buff + reduce_ctx->pipelined_blocks * stripe_offset;
              void *to = reduce_ctx->target_buff +
                         reduce_ctx->pipelined_blocks * stripe_offset;

              sctk_spinlock_lock_yield(&reduce_ctx->buff_lock[0]);

              if (mpc_op.u_func != NULL) {
                mpc_op.u_func(from, to, &rest, &datatype);
              } else {
                MPC_Op_f func;
                func = sctk_get_common_function(datatype, mpc_op);
                func(from, to, rest, datatype);
              }

              sctk_spinlock_unlock(&reduce_ctx->buff_lock[0]);

              rest_done = 1;
            }
          }

          /* Now process the core */
          int target_cell = (rank + i) % reduce_ctx->pipelined_blocks;
          // sctk_error("TARG %d R %d i %d/%d SEG %d STR %ld", target_cell,
          // rank, i, reduce_ctx->pipelined_blocks, per_lock, stripe_offset);

          void *from = data_buff + target_cell * stripe_offset;
          void *to = reduce_ctx->target_buff + target_cell * stripe_offset;

          sctk_spinlock_lock_yield(&reduce_ctx->buff_lock[target_cell]);

          if (mpc_op.u_func != NULL) {
            mpc_op.u_func(from, to, &per_lock, &datatype);
          } else {
            MPC_Op_f func;
            func = sctk_get_common_function(datatype, mpc_op);
            func(from, to, per_lock, datatype);
          }

          sctk_spinlock_unlock(&reduce_ctx->buff_lock[target_cell]);
        }

      } else {

        sctk_spinlock_lock_yield(&reduce_ctx->buff_lock[0]);

        if (mpc_op.u_func != NULL) {
          mpc_op.u_func(data_buff, reduce_ctx->target_buff, &count, &datatype);
        } else {
          MPC_Op_f func;
          func = sctk_get_common_function(datatype, mpc_op);
          func(data_buff, reduce_ctx->target_buff, count, datatype);
        }

        sctk_spinlock_unlock(&reduce_ctx->buff_lock[0]);
      }
    }
  }

SHM_REDUCE_DONE:

  /* I'm done, notify */
  sctk_atomics_decr_int(&reduce_ctx->left_to_push);

  /* Do we need to unpack and/or free ? */

  if (rank == root) {
    if (__do_yield) {
      while (sctk_atomics_load_int(&reduce_ctx->left_to_push) != 0) {
        sctk_thread_yield();
      }
    } else {
      while (sctk_atomics_load_int(&reduce_ctx->left_to_push) != 0) {
        sctk_cpu_relax();
      }
    }

    if (will_be_in_shm_buff) {
      sctk_mpi_shared_mem_buffer_collect(reduce_ctx->buffer, datatype, count,
                                         op, result_buff, size);
    } else if (!sctk_datatype_contig_mem(datatype)) {
      /* If non-contig, we need to unpack to the final buffer */
      int cnt = 0;
      MPI_Unpack(reduce_ctx->target_buff, tsize * count, &cnt, recvbuf, count,
                 datatype, comm);

      /* We had to allocate the segment */
      sctk_free(data_buff);
    }
    /* Set the counter */
    sctk_atomics_store_int(&reduce_ctx->left_to_push, size);

    /* Now flag slot as free */
    sctk_atomics_store_int(&reduce_ctx->owner, -1);

    /* And reverse the fare */
    int current_fare = sctk_atomics_load_int(&reduce_ctx->fare);
    sctk_atomics_store_int(&reduce_ctx->fare, !current_fare);

  } else {
    if (allocated) {
      sctk_free(data_buff);
    }
  }

  return MPI_SUCCESS;
}

int
__INTERNAL__PMPI_Reduce_intra (void *sendbuf, void *recvbuf, int count,
			 MPI_Datatype datatype, MPI_Op op, int root,
			 MPI_Comm comm)
{
	int size;
	int rank;
	int res;
	MPC_Op mpc_op;
	sctk_op_t *mpi_op;

	mpi_op = sctk_convert_to_mpc_op (op);
	mpc_op = mpi_op->op;
	
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}

	if(size == 1)
	{

		if (sendbuf == MPI_IN_PLACE)
		{
			return MPI_SUCCESS;
		}

		if( sctk_datatype_is_contiguous( datatype ) )
		{
			int tsize;
			PMPI_Type_size(datatype , &tsize );
			memcpy( recvbuf,  sendbuf , tsize * count );
		}
		else
		{
			int psize = 0;
			PMPI_Pack_size( count, datatype, comm, &psize );

			void *tmp = sctk_malloc(psize);

			assume( tmp != NULL );

			int pos = 0;
			PMPI_Pack( sendbuf, count, datatype, tmp, psize, &pos, comm);

			pos = 0;
			PMPI_Unpack(tmp, psize, &pos, recvbuf, count, datatype, comm );

			sctk_free( tmp );
		}

		return MPI_SUCCESS;
	} 
	else 
	{

          if (reduce_intra_shm == NULL) {
            reduce_intra_shm = (int (*)(void *, void *, int, MPI_Datatype,
                                        MPI_Op, int, MPI_Comm))(
                sctk_runtime_config_get()
                    ->modules.collectives_shm.reduce_intra_shm.value);
            assume(reduce_intra_shm != NULL);
          }

          if (sctk_is_shared_mem(comm) &&
              !sctk_datatype_is_struct_datatype(datatype) &&
              (mpi_op->op.u_func == NULL)) {
            return (reduce_intra_shm)(sendbuf, recvbuf, count, datatype, op,
                                      root, comm);
          }

          if ( !sctk_op_can_commute(mpi_op, datatype) || !sctk_datatype_contig_mem( datatype ) ) {
            res = __INTERNAL__PMPI_Reduce_derived_no_commute(
                sendbuf, recvbuf, count, datatype, op, root, comm, mpc_op,
                mpi_op, size, rank);
            if (res != MPI_SUCCESS) {
              return res;
            }
          } else {
            res = __INTERNAL__PMPI_Reduce_derived_commute(
                sendbuf, recvbuf, count, datatype, op, root, comm, mpc_op,
                mpi_op, size, rank);
            if (res != MPI_SUCCESS) {
              return res;
            }
          }
        }

        return res;
}

int
__INTERNAL__PMPI_Reduce_inter (void *sendbuf, void *recvbuf, int count,
			 MPI_Datatype datatype, MPI_Op op, int root,
			 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	unsigned long size;
	sctk_task_specific_t *task_specific;
	MPC_Status status;
	int rank;
	void *tmp_buf;
	
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	
	if (root == MPI_PROC_NULL)
		MPI_ERROR_SUCESS ();

	if (root == MPI_ROOT)
	{
		res = __INTERNAL__PMPI_Recv(recvbuf, count, datatype, 0, MPC_REDUCE_TAG, comm, &status);
		if(res != MPI_SUCCESS){return res;}
	}
	else
  {
    int type_size;
    res = __INTERNAL__PMPI_Type_size(datatype, &type_size);
    if(res != MPI_SUCCESS){return res;}

    size = count * type_size;
    tmp_buf = (void *) sctk_malloc(size);
    res = __INTERNAL__PMPI_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, sctk_get_local_comm_id(comm));
		if(res != MPI_SUCCESS){return res;}

		if (rank == 0)
		{
			res = __INTERNAL__PMPI_Send(tmp_buf, count, datatype, root, MPC_REDUCE_TAG, comm);
			if(res != MPI_SUCCESS){return res;}
		}
	}
	return res;
}

int
__INTERNAL__PMPI_Reduce (void *sendbuf, void *recvbuf, int count,
			 MPI_Datatype datatype, MPI_Op op, int root,
			 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	
	/* Intercomm */
	if(sctk_is_inter_comm(comm))
	{
		if(reduce_inter == NULL)
		reduce_inter = (int (*)(void *, void *, int, MPI_Datatype, 
		MPI_Op, int, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_inter.reduce_inter.value);
                res = (reduce_inter)(sendbuf, recvbuf, count, datatype, op,
                                     root, comm);
                if (res != MPI_SUCCESS) {
                  return res;
                }
        } else {
          /* Intracomm */
          if (reduce_intra == NULL) {
            reduce_intra = (int (*)(void *, void *, int, MPI_Datatype, MPI_Op,
                                    int, MPI_Comm))(
                sctk_runtime_config_get()
                    ->modules.collectives_intra.reduce_intra.value);
          }

          res =
              (reduce_intra)(sendbuf, recvbuf, count, datatype, op, root, comm);
          if (res != MPI_SUCCESS) {
            return res;
          }
        }
        return res;
}


static int
__INTERNAL__PMPI_Op_create (MPI_User_function * function, int commute,
			    MPI_Op * op)
{
  MPC_Op *mpc_op = NULL;
  sctk_mpi_ops_t *ops;
  int i;

  PMPC_Get_op ( &ops);
  sctk_spinlock_lock (&(ops->lock));

  for (i = 0; i < ops->size; i++)
    {
      if (ops->ops[i].used == 0)
	{
	  mpc_op = &(ops->ops[i].op);
	  break;
	}
    }

  if (mpc_op == NULL)
    {
      i = ops->size;
      ops->size += 10;
      ops->ops = sctk_realloc (ops->ops, ops->size * sizeof (sctk_op_t));
      assume (ops->ops != NULL);
    }

  *op = i + MAX_MPI_DEFINED_OP;
  ops->ops[i].used = 1;
  mpc_op = &(ops->ops[i].op);

  ops->ops[i].commute = commute;

  if (commute)
    {
      PMPC_Op_create (function, commute, mpc_op);
    }
  else
    {
      mpc_op->u_func = function;
    }

  sctk_spinlock_unlock (&(ops->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Op_free (MPI_Op * op)
{
  MPC_Op *mpc_op = NULL;
  sctk_mpi_ops_t *ops;

  PMPC_Get_op ( &ops);
  sctk_spinlock_lock (&(ops->lock));

  assume (*op >= MAX_MPI_DEFINED_OP);

  *op -= MAX_MPI_DEFINED_OP;

  assume (*op < ops->size);
  assume (ops->ops[*op].used == 1);

  mpc_op = &(ops->ops[*op].op);

  PMPC_Op_free (mpc_op);
  *op = MPI_OP_NULL;
  sctk_spinlock_unlock (&(ops->lock));
  return MPI_SUCCESS;
}

int
__INTERNAL__PMPI_Allreduce_intra_simple(void *sendbuf, void *recvbuf, int count,
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res;

	res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0,
			comm);
	if (res != MPI_SUCCESS) {
		return res;
	}
	
	res = __INTERNAL__PMPI_Bcast(recvbuf, count, datatype, 0, comm);
	if (res != MPI_SUCCESS) {
		return res;
	}

	return res;
}

#define STATIC_ALLRED_BLOB 4096

int
__INTERNAL__PMPI_Allreduce_intra_pipeline(void *sendbuf, void *recvbuf, int count,
										  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res;

	char blob[STATIC_ALLRED_BLOB];
	char blob1[STATIC_ALLRED_BLOB];
	void * tmp_buff = blob;
	void * tmp_buff1 = blob1;
	int to_free = 0;

	sctk_op_t *mpi_op = sctk_convert_to_mpc_op(op);
  	MPC_Op mpc_op = mpi_op->op;


	MPI_Aint dsize;
	
	res = __INTERNAL__PMPI_Type_extent(datatype, &dsize);
		
	if (res != MPI_SUCCESS) {
		return res;
	}

	int per_block = 1024;

	if( STATIC_ALLRED_BLOB <= (dsize * per_block) )
	{
		tmp_buff = sctk_malloc( dsize * per_block );
		tmp_buff1 = sctk_malloc( dsize * per_block );
		to_free = 1;
	}


	int size;
	__INTERNAL__PMPI_Comm_size( comm, &size );
	int rank;
	__INTERNAL__PMPI_Comm_rank( comm, &rank );

	int left_to_send = count;
	int sent = 0;


	if( sendbuf == MPI_IN_PLACE )
	{
		sendbuf = recvbuf;
	}

	void * sbuff = sendbuf;

	int left = (rank - 1);
	int right = (rank + 1) % size;

	if( (left < 0)  )
	{
		left = (size - 1);
	}


	/* Create Blocks in the COUNT */
	while( left_to_send )
	{

		int to_send = per_block;
	
		if( left_to_send < to_send)
		{
			to_send = left_to_send;
		}

		int i;
		MPI_Request rr;
		MPI_Request lr;

		for( i = 0 ; i < size ; i++ )
		{
			if( i == 0 )
			{
				/* Is is now time to send local buffer to the right while accumulating it locally */
				__INTERNAL__PMPI_Isend( sendbuf + dsize * sent , to_send , datatype , right , MPC_ALLREDUCE_TAG , comm , &rr );

				/* Copy local block in recv */
				if( (sbuff != MPI_IN_PLACE) )
				{
					memcpy( recvbuf + dsize * sent , sendbuf + dsize * sent, to_send *dsize );
				}
			}
			else
			{
				PMPI_Wait( &lr, MPI_STATUS_IGNORE );
				/* Our data is already moving around we now forward tmp_buff */
		
                                memcpy(tmp_buff1, tmp_buff, dsize * to_send );

				if( i != (size - 1))
				{
					__INTERNAL__PMPI_Isend( tmp_buff1 , to_send , datatype , right , MPC_ALLREDUCE_TAG , comm , &rr );
				}
			}
                        
                        if( i != (size-1))
			{
				__INTERNAL__PMPI_Irecv( tmp_buff , to_send , datatype , left , MPC_ALLREDUCE_TAG , comm , &lr );
			}
                        
                        if( i != 0 )
			{
				/* Data is ready now accumulate */
				if (mpc_op.u_func != NULL) {
						mpc_op.u_func(tmp_buff1, recvbuf + sent * dsize, &to_send, &datatype);
				} else {
						MPC_Op_f func;
						func = sctk_get_common_function(datatype, mpc_op);
						func(tmp_buff1, recvbuf + sent * dsize, to_send, datatype);
				}
			}

			if( i != (size-1))
			{
				__INTERNAL__PMPI_Wait( &rr, MPI_STATUS_IGNORE );
			}


			
			
		}




		left_to_send -= to_send;
		sent += to_send;
	}




	if( to_free == 1 )
	{
		sctk_free( tmp_buff );
		sctk_free( tmp_buff1 );
	}

	return res;


}


int
__INTERNAL__PMPI_Allreduce_intra (void *sendbuf, void *recvbuf, int count,
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

  sctk_op_t *mpi_op = sctk_convert_to_mpc_op (op);
	if( !sctk_datatype_contig_mem(datatype) || ! sctk_op_can_commute( mpi_op, datatype ))
	{
		res = __INTERNAL__PMPI_Allreduce_intra_simple( sendbuf, recvbuf, count, datatype, op, comm );
	}
	else
	{
		res = __INTERNAL__PMPI_Allreduce_intra_pipeline( sendbuf, recvbuf, count, datatype, op, comm );	
	}

	return res;
}

static int
__INTERNAL__PMPI_copy_buffer(void *sendbuf, void *recvbuf, int count,
			     MPI_Datatype datatype){

  int res = MPI_ERR_INTERN;
  if(sctk_datatype_is_derived (datatype) && (count != 0)){
    MPI_Request request_send;
    MPI_Request request_recv;
    
    res = __INTERNAL__PMPI_Isend (sendbuf, count, datatype, 
				  0, MPC_COPY_TAG, MPI_COMM_SELF, &request_send);
    if(res != MPI_SUCCESS){return res;}
    
    res = __INTERNAL__PMPI_Irecv (recvbuf, count, datatype, 
				 0, MPC_COPY_TAG, MPI_COMM_SELF, &request_recv);
    if(res != MPI_SUCCESS){return res;}
    
    res = __INTERNAL__PMPI_Wait (&(request_recv), MPI_STATUS_IGNORE);
    if(res != MPI_SUCCESS){return res;}
    
    res = __INTERNAL__PMPI_Wait (&(request_send), MPI_STATUS_IGNORE);
    if(res != MPI_SUCCESS){return res;}
  } else {
      MPI_Aint dsize;
      res = __INTERNAL__PMPI_Type_extent (datatype, &dsize);
      if(res != MPI_SUCCESS){return res;}
      memcpy(recvbuf,sendbuf,count*dsize);
  }
  return res;
}

int
__INTERNAL__PMPI_Allreduce_intra_binary_tree (void *sendbuf, void *recvbuf, int count,
					      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  MPC_Op mpc_op;
  sctk_op_t *mpi_op;
  int size;
  int rank;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;
	
  res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
  if(res != MPI_SUCCESS){return res;}
  res = __INTERNAL__PMPI_Comm_size (comm, &size);
  if(res != MPI_SUCCESS){return res;}

  if( sctk_op_can_commute(mpi_op, datatype)  || (size ==1) || (size % 2 != 0))
    {
      res = __INTERNAL__PMPI_Allreduce_intra(sendbuf,recvbuf,count,datatype,op,comm);
      if(res != MPI_SUCCESS){return res;}
    } 
  else 
    {
      void* tmp_buf;
      MPI_Aint dsize;
      int allocated = 0; 
      int is_MPI_IN_PLACE = 0;
      int step = 2;

      MPC_Op_f func;
      func = sctk_get_common_function(datatype, mpc_op);

      res = __INTERNAL__PMPI_Type_extent (datatype, &dsize);
      if(res != MPI_SUCCESS){return res;}
	    
      tmp_buf = sctk_malloc(count*dsize);
      if(sendbuf == MPI_IN_PLACE){
	is_MPI_IN_PLACE = 1;
	sendbuf = recvbuf;
      }
      allocated = 1;

      if(is_MPI_IN_PLACE == 0){
	res = __INTERNAL__PMPI_copy_buffer(sendbuf,recvbuf,count,datatype);
	if(res != MPI_SUCCESS){return res;}
      }
	   
      for(step = 1; step < size; step = step *2){
	if(rank % (2*step) != 0){
	  MPI_Request request_send;

	  //fprintf(stderr,"DOWN STEP %d %d Send to %d\n",step,rank,rank-step);

	  res = __INTERNAL__PMPI_Isend (recvbuf, count, datatype, 
					rank - step, MPC_ALLREDUCE_TAG, comm, &request_send);
	  if(res != MPI_SUCCESS){return res;}
				
	  res = __INTERNAL__PMPI_Wait (&(request_send), MPI_STATUS_IGNORE);
	  if(res != MPI_SUCCESS){return res;}

	  step = step *2;
	  break;
		
	} else {
	  if(rank + step < size){
	    //fprintf(stderr,"DOWN STEP %d %d Recv from %d\n",step,rank,rank+step);
	    res = __INTERNAL__PMPI_Recv (tmp_buf, count, datatype, 
					 rank + step, MPC_ALLREDUCE_TAG, comm, MPI_STATUS_IGNORE);
	    if(res != MPI_SUCCESS){return res;}
		  
		  
	    if (mpc_op.u_func != NULL)
	      {
		mpc_op.u_func(tmp_buf, recvbuf, &count, &datatype);
	      }
	    else
	      {
		func(tmp_buf, recvbuf, count, datatype);
	      }
		  
	  }
	}
      }

      step = step / 2;
      //fprintf(stderr,"DONE %d STEP %d\n",rank,step);

      for(;step > 0;step = step / 2){
	if(rank % (2*step) == 0){
	  if(rank+step < size){
	    MPI_Request request_send;
	    //fprintf(stderr,"UP STEP %d %d Send to %d\n",step,rank,rank+step);
	    res = __INTERNAL__PMPI_Isend (recvbuf, count, datatype, 
					  rank + step, MPC_ALLREDUCE_TAG, comm, &request_send);
	    if(res != MPI_SUCCESS){return res;}
	    
	    res = __INTERNAL__PMPI_Wait (&(request_send), MPI_STATUS_IGNORE);
	    if(res != MPI_SUCCESS){return res;}
	  }
	} else {
	  //fprintf(stderr,"UP STEP %d %d Recv from %d\n",step,rank,rank-step);
	  res = __INTERNAL__PMPI_Recv (recvbuf, count, datatype, 
				       rank - step, MPC_ALLREDUCE_TAG, comm, MPI_STATUS_IGNORE);
	}	
      }
 
      if(allocated == 1){
	free(tmp_buf);
      }
    }
  //__INTERNAL__PMPI_Barrier(comm);
  return res;
}

int
__INTERNAL__PMPI_Allreduce_intra_no_mix_derived_types (void *sendbuf, void *recvbuf, int count,
						       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  MPC_Op mpc_op;
  sctk_op_t *mpi_op;
  int size;
  int rank;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;
	
  res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
  if(res != MPI_SUCCESS){return res;}
  res = __INTERNAL__PMPI_Comm_size (comm, &size);
  if(res != MPI_SUCCESS){return res;}

  if((mpi_op->commute == 0) || (sctk_datatype_is_derived(datatype)))
    {
      res = __INTERNAL__PMPI_Allreduce_intra(sendbuf,recvbuf,count,datatype,op,comm);
      if(res != MPI_SUCCESS){return res;}
    } 
  else 
    {
      res = PMPC_Allreduce(sendbuf,recvbuf,count,datatype,mpc_op,comm);
      if(res != MPI_SUCCESS){return res;}
    }
  return res;
}

int
__INTERNAL__PMPI_Allreduce_inter (void *sendbuf, void *recvbuf, int count,
			    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int root, rank, res = MPI_ERR_INTERN;
	
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	
	if(sctk_is_in_local_group(comm))
	{
		root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
		res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype, 
		op, root, comm);
		if(res != MPI_SUCCESS){return res;}

                root = 0;
                res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype,
                                              op, root, comm);
                if (res != MPI_SUCCESS) {
                  return res;
                }
        } else {
          root = 0;
          res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype, op,
                                        root, comm);
          if (res != MPI_SUCCESS) {
            return res;
          }

          root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
          res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype, op,
                                        root, comm);
          if (res != MPI_SUCCESS) {
            return res;
          }
        }

        res = __INTERNAL__PMPI_Bcast(recvbuf, count, datatype, 0,
                                     sctk_get_local_comm_id(comm));
        return res;
}

int
__INTERNAL__PMPI_Allreduce (void *sendbuf, void *recvbuf, int count,
			    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	
	/* Intercomm */
	if(sctk_is_inter_comm(comm))
	{
		if(allreduce_inter == NULL)
		allreduce_inter = (int (*)(void *, void *, int, MPI_Datatype, 
		MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_inter.allreduce_inter.value);
		res = allreduce_inter(sendbuf, recvbuf, count, datatype, op, 
		comm);
		if(res != MPI_SUCCESS){return res;}
	}
	else
	{
		/* Intracomm */
		if(allreduce_intra == NULL)
		allreduce_intra = (int (*)(void *, void *, int, MPI_Datatype, 
		MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_intra.allreduce_intra.value);
		res = allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm);
		if(res != MPI_SUCCESS){return res;}
	}
	return res;
}

int
__INTERNAL__PMPI_Reduce_scatter_intra (void *sendbuf, void *recvbuf, int *recvcnts,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int i;
	MPI_Aint dsize;
	int size;
	int acc = 0;

	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Type_extent (datatype, &dsize);
	if(res != MPI_SUCCESS){return res;}

  if (sendbuf == MPI_IN_PLACE)
  {
    int total_size = 0; 
    for(i=0;i<size;i++)
      total_size += recvcnts[i];
    sendbuf = sctk_malloc(dsize * total_size);
    memcpy(sendbuf,recvbuf,dsize*total_size);    
  }
  for (i = 0; i < size; i++)
  {
    res = __INTERNAL__PMPI_Reduce (((char *) sendbuf) + (acc * dsize), recvbuf, recvcnts[i], datatype, op, i, comm);
    if(res != MPI_SUCCESS){return res;}
    acc += recvcnts[i];
  }

	res = __INTERNAL__PMPI_Barrier (comm);
	return res;
}

int
__INTERNAL__PMPI_Reduce_scatter_inter (void *sendbuf, void *recvbuf, int *recvcnts,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
	int res, i, rank, root = 0, rsize, lsize;
    int totalcounts;
    MPI_Aint extent;
    char *tmpbuf = NULL, *tmpbuf2 = NULL;
    MPI_Request req;
    int *disps = NULL;
    MPC_Op mpc_op;
	sctk_op_t *mpi_op;
	MPC_Op_f func;

	mpi_op = sctk_convert_to_mpc_op (op);
	mpc_op = mpi_op->op;

    res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
    if(res != MPI_SUCCESS){return res;}
    res = __INTERNAL__PMPI_Comm_size(comm, &lsize);
    if(res != MPI_SUCCESS){return res;}
    res = __INTERNAL__PMPI_Comm_remote_size(comm, &rsize);
    if(res != MPI_SUCCESS){return res;}

    for (totalcounts = 0, i = 0; i < lsize; i++) 
    {
        totalcounts += recvcnts[i];
    }

    if (rank == root) 
    {
        res = __INTERNAL__PMPI_Type_extent(datatype, &extent);
        if(res != MPI_SUCCESS){return res;}

        disps = (int*) sctk_malloc(sizeof(int) * lsize);
        if (NULL == disps){
            return MPI_ERR_INTERN;
        }
        disps[0] = 0;
        for (i = 0; i < (lsize - 1); ++i) 
        {
            disps[i + 1] = disps[i] + recvcnts[i];
        }

        tmpbuf = (char *) sctk_malloc(totalcounts * extent);
        tmpbuf2 = (char *) sctk_malloc(totalcounts * extent);
        if (NULL == tmpbuf || NULL == tmpbuf2) {
            return MPI_ERR_INTERN;
        }

        res = __INTERNAL__PMPI_Isend(sendbuf, totalcounts, datatype, 0, 
        MPC_REDUCE_SCATTER_TAG, comm, &req);
        if(res != MPI_SUCCESS){return res;}

        res = __INTERNAL__PMPI_Recv(tmpbuf2, totalcounts, datatype, 0, 
        MPC_REDUCE_SCATTER_TAG, comm, MPI_STATUS_IGNORE);
        if(res != MPI_SUCCESS){return res;}

        res = __INTERNAL__PMPI_Wait( &req, MPI_STATUS_IGNORE);
        if(res != MPI_SUCCESS){return res;}

        for (i = 1; i < rsize; i++) 
        {
            res = __INTERNAL__PMPI_Recv(tmpbuf, totalcounts, datatype, 
            i, MPC_REDUCE_SCATTER_TAG, comm, MPI_STATUS_IGNORE);
            if(res != MPI_SUCCESS){return res;}
            
            if (mpc_op.u_func != NULL)
            {
                mpc_op.u_func(tmpbuf, tmpbuf2, &totalcounts, &datatype);
            }
            else
            {
                MPC_Op_f func;
                func = sctk_get_common_function(datatype, mpc_op);
                func(tmpbuf, tmpbuf2, totalcounts, datatype);
            }
        }
    } 
    else 
    {
        res = __INTERNAL__PMPI_Send(sendbuf, totalcounts, datatype, root, 
        MPC_REDUCE_SCATTER_TAG, comm);
        if(res != MPI_SUCCESS){return res;}
    }

    res = __INTERNAL__PMPI_Scatterv(tmpbuf2, recvcnts, disps, datatype, 
    recvbuf, recvcnts[rank], datatype, 0, sctk_get_local_comm_id(comm));

    return res;
}

int
__INTERNAL__PMPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	
	/* Intercomm */
	if(sctk_is_inter_comm(comm))
	{
		if(reduce_scatter_inter == NULL)
		reduce_scatter_inter = (int (*)(void *, void *, int *, MPI_Datatype, 
		MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_inter.reduce_scatter_inter.value);
		res = reduce_scatter_inter(sendbuf, recvbuf, recvcnts, datatype, 
		op, comm);
		if(res != MPI_SUCCESS){return res;}
	}
	else
	{
		/* Intracomm */
		if(reduce_scatter_intra == NULL)
		reduce_scatter_intra = (int (*)(void *, void *, int *, MPI_Datatype, 
		MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_intra.reduce_scatter_intra.value);
		res = reduce_scatter_intra(sendbuf, recvbuf, recvcnts, datatype, 
		op, comm);
		if(res != MPI_SUCCESS){return res;}
	}
	return res;
}

int
__INTERNAL__PMPI_Reduce_scatter_block_intra (void *sendbuf, void *recvbuf, int recvcnt,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
	int rank, size, count, res = MPI_SUCCESS;
    MPI_Aint true_lb, true_extent, lb, extent, buf_size;
    char *recv_buf = NULL, *recv_buf_free = NULL;

    res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
    if(res != MPI_SUCCESS){return res;}
    res = __INTERNAL__PMPI_Comm_size(comm, &size);
    if(res != MPI_SUCCESS){return res;}

    count = recvcnt * size;
    if (0 == count){
        return MPI_SUCCESS;
    }

    res = PMPI_Type_get_extent(datatype, &lb, &extent);
    res = PMPI_Type_get_true_extent(datatype, &true_lb, &true_extent);
    buf_size = true_extent + (count - 1) * extent;

    if (MPI_IN_PLACE == sendbuf) {
        sendbuf = recvbuf;
    }

    if (rank == 0) 
    {
        recv_buf_free = (char*) sctk_malloc(buf_size);
        recv_buf = recv_buf_free - lb;
        if (recv_buf_free == NULL) {
            return MPI_ERR_INTERN;
        }
    }

    res = __INTERNAL__PMPI_Reduce(sendbuf, recv_buf, count, datatype, op, 0, comm);
	if(res != MPI_SUCCESS){return res;}
    
    res = __INTERNAL__PMPI_Scatter(recv_buf, recvcnt, datatype, recvbuf, recvcnt, datatype, 0, comm);
    return res;
}

int
__INTERNAL__PMPI_Reduce_scatter_block_inter (void *sendbuf, void *recvbuf, int recvcnt,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN, i, rank, root = 0, rsize, lsize;
    int totalcounts;
    MPI_Aint extent;
    char *tmpbuf = NULL, *tmpbuf2 = NULL;
    MPI_Request req;
    MPC_Op mpc_op;
	sctk_op_t *mpi_op;
	MPC_Op_f func;

	mpi_op = sctk_convert_to_mpc_op (op);
	mpc_op = mpi_op->op;

    res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
    if(res != MPI_SUCCESS){return res;}
    res = __INTERNAL__PMPI_Comm_size(comm, &lsize);
    if(res != MPI_SUCCESS){return res;}
    res = __INTERNAL__PMPI_Comm_remote_size(comm, &rsize);
    if(res != MPI_SUCCESS){return res;}

    totalcounts = lsize * recvcnt;

    if (rank == root) 
    {
        res = __INTERNAL__PMPI_Type_extent(datatype, &extent);
        if(res != MPI_SUCCESS){return res;}

        tmpbuf = (char *) sctk_malloc(totalcounts * extent);
        tmpbuf2 = (char *) sctk_malloc(totalcounts * extent);
        if (NULL == tmpbuf || NULL == tmpbuf2) {
            return MPI_ERR_INTERN;
        }

        res = __INTERNAL__PMPI_Isend(sendbuf, totalcounts, datatype, 0, 
        MPC_REDUCE_SCATTER_BLOCK_TAG, comm, &req);
        if(res != MPI_SUCCESS){return res;}

        res = __INTERNAL__PMPI_Recv(tmpbuf2, totalcounts, datatype, 0, 
        MPC_REDUCE_SCATTER_BLOCK_TAG, comm, MPI_STATUS_IGNORE);
        if(res != MPI_SUCCESS){return res;}

        res = __INTERNAL__PMPI_Wait( &req, MPI_STATUS_IGNORE);
        if(res != MPI_SUCCESS){return res;}

        for (i = 1; i < rsize; i++) 
        {
            res = __INTERNAL__PMPI_Recv(tmpbuf, totalcounts, datatype, i, 
            MPC_REDUCE_SCATTER_BLOCK_TAG, comm, MPI_STATUS_IGNORE);
            if(res != MPI_SUCCESS){return res;}

            if (mpc_op.u_func != NULL)
            {
                mpc_op.u_func(tmpbuf, tmpbuf2, &totalcounts, &datatype);
            }
            else
            {
                MPC_Op_f func;
                func = sctk_get_common_function(datatype, mpc_op);
                func(tmpbuf, tmpbuf2, totalcounts, datatype);
            }
        }
    } 
    else 
    {
        res = __INTERNAL__PMPI_Send(sendbuf, totalcounts, datatype, root, 
        MPC_REDUCE_SCATTER_BLOCK_TAG, comm);
        if(res != MPI_SUCCESS){return res;}
    }

    res = __INTERNAL__PMPI_Scatter(tmpbuf2, recvcnt, datatype, recvbuf, recvcnt, 
    datatype, 0, sctk_get_local_comm_id(comm));

    return res;
}

int
__INTERNAL__PMPI_Reduce_scatter_block (void *sendbuf, void *recvbuf, int recvcnt,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	
	/* Intercomm */
	if(sctk_is_inter_comm(comm))
	{
		if(reduce_scatter_block_inter == NULL)
		reduce_scatter_block_inter = (int (*)(void *, void *, int, MPI_Datatype, 
		MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_inter.reduce_scatter_block_inter.value);
		res = reduce_scatter_block_inter(sendbuf, recvbuf, recvcnt, datatype, 
		op, comm);
		if(res != MPI_SUCCESS){return res;}
	}
	else
	{
		/* Intracomm */
		if(reduce_scatter_block_intra == NULL)
		reduce_scatter_block_intra = (int (*)(void *, void *, int, MPI_Datatype, 
		MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_intra.reduce_scatter_block_intra.value);
		res = reduce_scatter_block_intra(sendbuf, recvbuf, recvcnt, datatype, 
		op, comm);
		if(res != MPI_SUCCESS){return res;}
	}
	return res;
}

int
__INTERNAL__PMPI_Scan_intra (void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	MPC_Op mpc_op;
	sctk_op_t *mpi_op;
	int size, dsize;
	int rank;
	MPI_Request request;
	char *tmp = NULL;
	char *free_buffer = NULL;
	int res;

  if(sendbuf == MPI_IN_PLACE)
  {
    sendbuf = recvbuf;
  }

	mpi_op = sctk_convert_to_mpc_op (op);
	mpc_op = mpi_op->op;

	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}

	res = __INTERNAL__PMPI_Isend (sendbuf, count, datatype, rank, MPC_SCAN_TAG, comm, &request);
	if(res != MPI_SUCCESS){return res;}

	res = __INTERNAL__PMPI_Recv (recvbuf, count, datatype, rank, MPC_SCAN_TAG, comm, MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS){return res;}
	

	res = __INTERNAL__PMPI_Barrier (comm);
	if(res != MPI_SUCCESS){return res;}

	if (rank != 0)
	{
		res = PMPI_Type_size(datatype, &dsize);
        if(res != MPI_SUCCESS){return res;}

		tmp = sctk_malloc(dsize*count);
		res = __INTERNAL__PMPI_Recv (tmp, count, datatype, rank - 1, MPC_SCAN_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS){return res;}
		
		if (mpc_op.u_func != NULL)
		{
			mpc_op.u_func (tmp, recvbuf, &count, &datatype);
		}
		else
		{
			MPC_Op_f func;
			func = sctk_get_common_function (datatype, mpc_op);
			func (tmp, recvbuf, count, datatype);
		}
	}

	if (rank + 1 < size)
	{
		res = __INTERNAL__PMPI_Send (recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm);
		if(res != MPI_SUCCESS){return res;}
	}

	sctk_free (tmp);
	res = __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS){return res;}

	res = __INTERNAL__PMPI_Barrier (comm);
	return res;
}

int
__INTERNAL__PMPI_Scan (void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	
	int res = MPI_ERR_INTERN;
	
	/* Only on Intracomm */
	if(scan_intra == NULL)
	scan_intra = (int (*)(void *, void *, int, MPI_Datatype, 
	MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_intra.scan_intra.value);
	res = scan_intra(sendbuf, recvbuf, count, datatype, op, comm);

	return res;
}

int
__INTERNAL__PMPI_Exscan_intra (void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	MPC_Op mpc_op;
	sctk_op_t *mpi_op;
	int size;
	int rank;
	MPI_Request request;
	MPI_Aint dsize;
	void *tmp;
	int res = MPI_ERR_INTERN;

  if(sendbuf == MPI_IN_PLACE)
  {
    sendbuf = recvbuf;
  }

	mpi_op = sctk_convert_to_mpc_op (op);
	mpc_op = mpi_op->op;

	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}

	res = __INTERNAL__PMPI_Isend (sendbuf, count, datatype, rank, 
	MPC_EXSCAN_TAG, comm, &request);
	if(res != MPI_SUCCESS){return res;}
	
	res =  __INTERNAL__PMPI_Type_extent (datatype, &dsize);
	if(res != MPI_SUCCESS){return res;}

	tmp = sctk_malloc (dsize*count);

	res = __INTERNAL__PMPI_Recv (tmp, count, datatype, rank, 
	MPC_EXSCAN_TAG, comm, MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS){return res;}

	res = __INTERNAL__PMPI_Barrier (comm);
	if(res != MPI_SUCCESS){return res;}

	if (rank != 0)
	{
		res = __INTERNAL__PMPI_Recv (recvbuf, count, datatype, rank - 1, 
		MPC_EXSCAN_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS){return res;}
		
		if (mpc_op.u_func != NULL)
		{
			mpc_op.u_func (recvbuf, tmp, &count, &datatype);
		}
		else
		{
			MPC_Op_f func;
			func = sctk_get_common_function (datatype, mpc_op);
			func (recvbuf, tmp, count, datatype);
		}
	}

	if (rank + 1 < size)
	{
		res = __INTERNAL__PMPI_Send (tmp, count, datatype, rank + 1, 
		MPC_EXSCAN_TAG, comm);
		if(res != MPI_SUCCESS){return res;}
	}

	sctk_free (tmp);
	res = __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS){return res;}
	
	res = __INTERNAL__PMPI_Barrier (comm);
	return res;
}

int
__INTERNAL__PMPI_Exscan (void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	
	int res = MPI_ERR_INTERN;
	
	/* Only on Intracomm */
	if(exscan_intra == NULL)
	exscan_intra = (int (*)(void *, void *, int, MPI_Datatype, 
	MPI_Op, MPI_Comm))(sctk_runtime_config_get()->modules.collectives_intra.exscan_intra.value);
	res = exscan_intra(sendbuf, recvbuf, count, datatype, op, comm);

	return res;
}

typedef struct MPI_internal_group_s
{
  MPC_Group group;
  int used;
  volatile struct MPI_internal_group_s *next;
  int rank;
} MPI_internal_group_t;

struct MPI_group_struct_s
{
  sctk_spinlock_t lock;
  long max_size;
  MPI_internal_group_t **tab;
  volatile MPI_internal_group_t *free_list;
  sctk_alloc_buffer_t buf;
} ;


static inline MPI_internal_group_t *
__sctk_new_mpc_group_internal (MPI_Group * group)
{
  MPI_group_struct_t *groups;
  MPI_internal_group_t *tmp;

  PMPC_Get_groups ((void *) &groups);

  sctk_spinlock_lock (&(groups->lock));
  if (groups->free_list == NULL)
    {
      long old_size;
      long i;
	  old_size = groups->max_size;
      groups->max_size += 10;
      groups->tab =
	sctk_realloc (groups->tab,
		      groups->max_size * sizeof (MPI_internal_group_t *));
      assume (groups->tab != NULL);
      for (i = groups->max_size - 1; i >= old_size; i--)
	{
	  MPI_internal_group_t *tmp;
	  sctk_nodebug ("%lu", i);
/* 	  tmp = sctk_malloc(sizeof (MPI_internal_group_t)); */
	  tmp =
	    sctk_buffered_malloc (&(groups->buf),
				  sizeof (MPI_internal_group_t));
	  assume (tmp != NULL);
	  groups->tab[i] = tmp;
	  tmp->used = 0;
	  tmp->next = groups->free_list;
	  groups->free_list = tmp;
	  tmp->rank = i;
	}
    }
  tmp = (MPI_internal_group_t *) groups->free_list;
  tmp->used = 1;
  groups->free_list = tmp->next;
  sctk_spinlock_unlock (&(groups->lock));
  *group = tmp->rank;
  sctk_nodebug ("Create group %d", *group);
  return tmp;
}

static inline MPC_Group
__sctk_new_mpc_group (MPI_Group * group)
{
  MPI_internal_group_t *tmp;
  tmp = __sctk_new_mpc_group_internal (group);
  return tmp->group;
}

static inline void
__sctk_init_mpc_group ()
{
  MPI_group_struct_t *groups;
  MPI_Group EMPTY;
  static MPC_Group_t mpc_mpi_group_empty;

  groups = sctk_malloc (sizeof (MPI_group_struct_t));

  groups->lock = 0;
  groups->tab = NULL;
  groups->free_list = NULL;
  groups->max_size = 0;
  sctk_buffered_alloc_create (&(groups->buf), sizeof (MPI_internal_group_t));

  PMPC_Set_groups (groups);

  /* Init group empty */
  memcpy (&mpc_mpi_group_empty, &mpc_group_empty, sizeof (MPC_Group_t));
  __sctk_new_mpc_group_internal (&EMPTY)->group = &mpc_mpi_group_empty;
  assume (EMPTY == MPI_GROUP_EMPTY);

}

static inline MPI_internal_group_t *
__sctk_convert_mpc_group_internal (MPI_Group group)
{
  MPI_internal_group_t *tmp;
  MPI_group_struct_t *groups;
  int int_group;

  int_group = group;
  if (int_group == MPI_GROUP_NULL)
    {
      return NULL;
    }


  PMPC_Get_groups ((void *) &groups);

  assume (groups != NULL);

  sctk_spinlock_lock (&(groups->lock));
  assume (((int_group) >= 0) && ((int_group) < groups->max_size));
  tmp = groups->tab[int_group];
  assume (tmp->rank == group);
  assume (tmp->used);
  sctk_spinlock_unlock (&(groups->lock));
  assume (tmp != NULL);
  return tmp;
}
static inline MPC_Group
__sctk_convert_mpc_group (MPI_Group group)
{
  MPI_internal_group_t *tmp;

  tmp = __sctk_convert_mpc_group_internal (group);
  if (tmp == NULL)
    {
      return MPC_GROUP_NULL;
    }
  assume (tmp->group != NULL);
  return tmp->group;
}

static inline void
__sctk_delete_mpc_group (MPI_Group * group)
{
  MPI_group_struct_t *groups;
  MPI_internal_group_t *tmp;
  int int_group;

  int_group = *group;
  if (int_group == MPI_GROUP_NULL)
    {
      return;
    }

  PMPC_Get_groups ((void *) &groups);
  assume (groups != NULL);

  sctk_nodebug ("Delete group %d", *group);
  sctk_spinlock_lock (&(groups->lock));
  assume (((*group) >= 0) && ((*group) < groups->max_size));
  tmp = groups->tab[*group];
  assume (tmp->rank == *group);
  tmp->used = 0;
  tmp->next = groups->free_list;
  groups->free_list = tmp;
  *group = MPI_GROUP_NULL;
  sctk_spinlock_unlock (&(groups->lock));
}

static inline int
__sctk_is_part_of_group(MPI_Group group)
{
	int rank, i;
	MPC_Group temp_group = __sctk_convert_mpc_group (group);
	__INTERNAL__PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
	for(i = 0 ; i <  temp_group->task_nb ; i++)
	{
		if(temp_group->task_list_in_global_ranks[i] == rank)
		{
			sctk_nodebug("rank = %d, temp_group->task_list_in_global_ranks[%d] == %d", rank, i, temp_group->task_list_in_global_ranks[i]);
			return 1;
		}
	}
	return 0;
}

static int
__INTERNAL__PMPI_Group_size (MPI_Group group, int *size)
{
  *size = __sctk_convert_mpc_group (group)->task_nb;
  return MPI_SUCCESS;
}

int *sctk_group_raw_ranks(MPI_Group group) {
  return __sctk_convert_mpc_group(group)->task_list_in_global_ranks;
}

static int
__INTERNAL__PMPI_Group_rank (MPI_Group mpi_group, int *rank)
{
  int i;
  int grank;
  MPC_Group group;
  group = __sctk_convert_mpc_group (mpi_group);
  __INTERNAL__PMPI_Comm_rank (MPI_COMM_WORLD, &grank);
  *rank = MPI_UNDEFINED;
  for (i = 0; i < group->task_nb; i++)
    {
      if (group->task_list_in_global_ranks[i] == grank)
	{
	  *rank = i;
	  return MPI_SUCCESS;
	}
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_translate_ranks (MPI_Group mpi_group1, int n, int *ranks1, MPI_Group mpi_group2, int *ranks2)
{
	int i,j;
	MPC_Group group1;
	MPC_Group group2;

	if ( (MPI_GROUP_NULL == mpi_group1) || (MPI_GROUP_NULL == mpi_group2) )
	{
		sctk_nodebug("Wrong group 1 : group1 %d, group2 %d", mpi_group1, mpi_group2);
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"");
	}
	if (n > 0 && ((NULL == ranks1) || (NULL == ranks2 )))
	{
		sctk_nodebug("Wrong group n > 0 && ((NULL == ranks1) || (NULL == ranks2 ))");
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"");
	}

	if(n == 0)
		return MPI_SUCCESS;

	if ( MPI_GROUP_EMPTY == mpi_group1 || MPI_GROUP_EMPTY == mpi_group2 )
	{
		for (i = 0; i < n ; i++)
			ranks2[i] = MPI_UNDEFINED;

		return MPI_SUCCESS;
    }

	group1 = __sctk_convert_mpc_group (mpi_group1);
	group2 = __sctk_convert_mpc_group (mpi_group2);

	sctk_nodebug("n = %d", n);
	for(i = 0 ; i < n ; i++)
		sctk_nodebug("ranks1[%d] = %d", i, ranks1[i]);

        for(i = 0 ; i < n ; i++){
                if(ranks1[i]  ==  MPI_UNDEFINED )
					return MPI_ERR_RANK;
                
				if(ranks1[i] >= group1->task_nb)
					return MPI_ERR_RANK;
        }

	/*
	for(i = 0 ; i < group1->task_nb ; i++)
		sctk_error("group1 : task_list_in_global_ranks[%d] = %d", i, group1->task_list_in_global_ranks[i]);

	for(i = 0 ; i < group2->task_nb ; i++)
		sctk_error("group2 : task_list_in_global_ranks[%d] = %d", i, group2->task_list_in_global_ranks[i]);
	*/


	for (j = 0; j < n; j++)
    {
		int i;
		int grank;
		
		/* MPI 2 Errata http://www.mpi-forum.org/docs/mpi-2.0/errata-20-2.pdf */
		if( ranks1[j] == MPI_PROC_NULL )
		{
			ranks2[j] = MPI_PROC_NULL;
			continue;
		}
		
		grank = group1->task_list_in_global_ranks[ranks1[j]];
		ranks2[j] = MPI_UNDEFINED;
		for (i = 0; i < group2->task_nb; i++)
		{
			if (group2->task_list_in_global_ranks[i] == grank)
			{
				ranks2[j] = i;
			}
		}
		sctk_nodebug ("%d is %d", ranks1[j], ranks2[j]);
    }
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_compare (MPI_Group mpi_group1, MPI_Group mpi_group2,
				int *result)
{
	int i;
	int is_ident = 1;
	int is_similar = 1;
	MPC_Group group1;
	MPC_Group group2;

	*result = MPI_UNEQUAL;

	if (mpi_group1 == mpi_group2)
	{
		*result = MPI_IDENT;
		return MPI_SUCCESS;
	}

	if (mpi_group1 == MPI_GROUP_EMPTY || mpi_group2 == MPI_GROUP_EMPTY)
	{
		*result = MPI_UNEQUAL;
		return MPI_SUCCESS;
	}

	group1 = __sctk_convert_mpc_group (mpi_group1);
	group2 = __sctk_convert_mpc_group (mpi_group2);
	
	if (group1->task_nb != group2->task_nb)
	{
		*result = MPI_UNEQUAL;
		return MPI_SUCCESS;
	}
	
	for (i = 0; i < group1->task_nb; i++)
	{
		if (group1->task_list_in_global_ranks[i] != group2->task_list_in_global_ranks[i])
		{
			is_ident = 0;
			break;
		}
	}

	if (is_ident)
	{
		*result = MPI_IDENT;
		return MPI_SUCCESS;
	}

	for (i = 0; i < group1->task_nb; i++)
	{
		int j;
		int found = 0;
		for (j = 0; j < group2->task_nb; j++)
		{
			if (group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
			{
				found = 1;
				break;
			}
		}
		if(found == 0)
		{
			is_similar = 0;
			break;
		}
	}

	if (is_similar)
	{
		*result = MPI_SIMILAR;
		return MPI_SUCCESS;
	}

	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Comm_group (MPI_Comm comm, MPI_Group * mpi_newgroup)
{
  MPI_internal_group_t *newgroup;
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  return PMPC_Comm_group (comm, &(newgroup->group));
}

static int
__INTERNAL__PMPI_Group_union (MPI_Group mpi_group1, MPI_Group mpi_group2,
			      MPI_Group * mpi_newgroup)
{
  int size;
  int j;
  int i;
  MPC_Group group1;
  MPC_Group group2;
  MPC_Group newgroup;
  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);

  for(i = 0 ; i < group1->task_nb ; i++)
	sctk_nodebug("group1 : task_list_in_global_ranks[%d] = %d", i, group1->task_list_in_global_ranks[i]);

  for(i = 0 ; i < group2->task_nb ; i++)
	sctk_nodebug("group2 : task_list_in_global_ranks[%d] = %d", i, group2->task_list_in_global_ranks[i]);

  size = group1->task_nb + group2->task_nb;
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  memcpy ((newgroup)->task_list_in_global_ranks, group1->task_list_in_global_ranks,
	  group1->task_nb * sizeof (int));
  memcpy (&((newgroup)->task_list_in_global_ranks[group1->task_nb]), group2->task_list_in_global_ranks,
	  group2->task_nb * sizeof (int));

  for (i = 0; i < group1->task_nb; i++)
    {
	  for (j = 0; j < group2->task_nb; j++)
		{
		  if (group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
			{
			  (&((newgroup)->task_list_in_global_ranks[group1->task_nb]))[j] = -1;
			  size--;
			}
		}
    }

  for (i = 0; i < (newgroup)->task_nb; i++)
    {
      if ((newgroup)->task_list_in_global_ranks[i] == -1)
		{
		  for (j = i; j < (newgroup)->task_nb - 1; j++)
			{
			  (newgroup)->task_list_in_global_ranks[j] = (newgroup)->task_list_in_global_ranks[j + 1];
			}
		}
    }

  for(i = 0 ; i < size ; i++)
	sctk_nodebug("task_list_in_global_ranks[%d] = %d", i, (newgroup)->task_list_in_global_ranks[i]);
  (newgroup)->task_nb = size;

  if(size == 0)
	*mpi_newgroup = MPI_GROUP_EMPTY;

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_intersection (MPI_Group mpi_group1,
				     MPI_Group mpi_group2,
				     MPI_Group * mpi_newgroup)
{
  int i;
  int size;
  MPC_Group group1;
  MPC_Group group2;
  MPC_Group newgroup;
  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  size = group1->task_nb;

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  size = 0;
  for (i = 0; i < group1->task_nb; i++)
    {
      int j;
      for (j = 0; j < group2->task_nb; j++)
	{
	  if (group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
	    {
	      (newgroup)->task_list_in_global_ranks[size] = group1->task_list_in_global_ranks[i];
	      size++;
	      break;
	    }
	}
    }
  (newgroup)->task_nb = size;
  if(size == 0)
	*mpi_newgroup = MPI_GROUP_EMPTY;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_difference (MPI_Group mpi_group1, MPI_Group mpi_group2,
				   MPI_Group * mpi_newgroup)
{
  int result;
  MPC_Group group1;
  MPC_Group group2;
  MPI_internal_group_t *newgroup;

  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  result = PMPC_Group_difference (group1, group2, &(newgroup->group));
  if(result != MPI_SUCCESS)
	return result;
  if(newgroup->group->task_nb == 0)
	*mpi_newgroup = MPI_GROUP_EMPTY;
  return result;

}

static int
__INTERNAL__PMPI_Group_incl (MPI_Group mpi_group, int n, int *ranks,
			     MPI_Group * mpi_newgroup)
{
  int res;
  MPC_Group group;
  int i;
  int j; 
  MPI_internal_group_t *newgroup;

  if(n < 0)
	return MPI_ERR_ARG;

  if(n == 0)
	{
		(*mpi_newgroup) = MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}


  group = __sctk_convert_mpc_group (mpi_group);


  for(i = 0; i < n ; i++){
    if((ranks[i] < 0) || (ranks[i] >= group->task_nb))
      MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
  }
  for(i = 0; i < n ; i++){
    for(j = 0; j < n ; j++){
	if((j != i) && (ranks[i] == ranks[j]))
	 MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
    }
  }

  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);

  res = PMPC_Group_incl (group, n, ranks, &(newgroup->group));

  return res;
}

static int
__INTERNAL__PMPI_Group_excl (MPI_Group mpi_group, int n, int *ranks,
			     MPI_Group * mpi_newgroup)
{
	int i;
	int j;
	int k;
	int is_out;
	int size;
	MPC_Group group;
	MPC_Group newgroup;

	if ( (MPI_GROUP_NULL == mpi_group) || (NULL == mpi_newgroup) )
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"");
	else if ((NULL == ranks && n > 0) || (n < 0))
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"");

	group = __sctk_convert_mpc_group (mpi_group);
	newgroup = __sctk_new_mpc_group (mpi_newgroup);
	size = group->task_nb - n;

	if(size == 0)
	{
		(*mpi_newgroup) = MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}
	if(n >=  group->task_nb )MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"");

  for(i = 0; i < n ; i++){
    for(j = 0; j < n ; j++){
        if((j != i) && (ranks[i] == ranks[j]))
         MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
    }
  }


	newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
	(newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
	(newgroup)->task_nb = size;
	__sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;
	for(i=0; i<size; i++)
		(newgroup)->task_list_in_global_ranks[i] = -1;

	k=0;
	for(i=0 ; i<group->task_nb; i++)
	{
		is_out = 0;
		for(j=0; j<n; j++)
		{
			if((ranks[j] < 0) || (ranks[j] >= group->task_nb))
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
			if(i == ranks[j])
			{
				is_out = 1;
				break;
			}
		}
		if(!is_out)
		{
			(newgroup)->task_list_in_global_ranks[k] = group->task_list_in_global_ranks[i];
			k++;
		}
	}

	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_range_excl (MPI_Group mpi_group, int n,
				   int ranges[][3], MPI_Group * mpi_newgroup)
{
	TODO("Cette partie a été reprise sur OpenMPI");
	int err, i,index;
    int group_size;
    int * elements_int_list;

    int j,k;
    int *ranks_included=NULL;
    int *ranks_excluded=NULL;
    int first_rank,last_rank,stride;
    int count,result;
	sctk_nodebug("MPI_Group_range_excl");
	MPC_Group group;

/* Error checking */
	group = __sctk_convert_mpc_group (mpi_group);

	if ( (MPI_GROUP_NULL == mpi_group) || (NULL == mpi_newgroup) )
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"group must not be MPI_GROUP_NULL");

	__INTERNAL__PMPI_Group_size (mpi_group, &group_size);
	elements_int_list = (int *) sctk_malloc(sizeof(int) * (group_size+1));

	if (NULL == elements_int_list)
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_INTERN,"");

	for (i = 0; i <= group_size; i++)
		elements_int_list[i] = -1;

	for ( i=0; i < n; i++)
	{
		first_rank = ranges[i][0];
		last_rank  = ranges[i][1];
		stride     = ranges[i][2];
		
		if ((first_rank < 0) || (first_rank >= group_size) || (last_rank < 0) || (last_rank >= group_size) || (stride == 0))
		{
			MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
		}

		if ((first_rank < last_rank))
		{
			if (stride < 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index <= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else if (first_rank > last_rank)
		{
			if (stride > 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index >= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else
		{
			index = first_rank;
			if (elements_int_list[index] != -1)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}
			elements_int_list[index] = i;
		}
	}
	free ( elements_int_list);

/* computing the ranks to be included */
    count = 0;
    /* determine the number of excluded processes for the range-excl-method */
    for(j=0 ; j<n ; j++)
    {
        first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
            index = first_rank;
			while (index <= last_rank)
			{
				count ++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				count ++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
            index = first_rank;
			count ++;
		}
    }
    if (0 != count)
    {
        ranks_excluded = (int *)malloc( group_size*(sizeof(int)));
    }

    /* determine the list of excluded processes for the range-excl method */
    k = 0;
    i = 0;
    for(j=0 ; j<n ; j++)
    {
		first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
			index = first_rank;
			while (index <= last_rank)
			{
				ranks_excluded[i] = index;
				i++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				ranks_excluded[i] = index;
				i++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
			index = first_rank;
			ranks_excluded[i] = index;
			i++;
		}
    }

    if (0 != (group->task_nb - count))
	{
		ranks_included = (int *)malloc( group_size*(sizeof(int)));
	}
	for (j=0 ; j<group->task_nb ; j++)
	{
		for(index=0 ; index<i ; index++)
		{
			if(ranks_excluded[index] == j) break;
		}
		if (index == i)
		{
			ranks_included[k] = j;
			k++;
		}
	}

/* including the ranks in the new group */
    result = __INTERNAL__PMPI_Group_incl(mpi_group, k, ranks_included, mpi_newgroup);

    if (NULL != ranks_included)
    {
        sctk_free(ranks_included);
    }
    if (NULL != ranks_excluded)
    {
        sctk_free(ranks_excluded);
    }

	return result;
}

static int
__INTERNAL__PMPI_Group_range_incl (MPI_Group mpi_group, int n,
				   int ranges[][3], MPI_Group * mpi_newgroup)
{
	TODO("Cette partie a été reprise sur OpenMPI");
	int err, i,index;
    int group_size;
    int * elements_int_list;

    int j,k;
    int *ranks_included=NULL;
    int first_rank,last_rank,stride;
    int count,result;

	MPC_Group group;
	MPC_Group newgroup;

/* Error checking */
	if(n == 0)
	{
		(*mpi_newgroup) = MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}

	if ( (MPI_GROUP_NULL == mpi_group) || (NULL == mpi_newgroup) )
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"group must not be MPI_GROUP_NULL");

	__INTERNAL__PMPI_Group_size (mpi_group, &group_size);
	elements_int_list = (int *) sctk_malloc(sizeof(int) * (group_size+1));

	if (NULL == elements_int_list)
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_INTERN,"");

	for (i = 0; i < group_size; i++)
		elements_int_list[i] = -1;

	for ( i=0; i < n; i++)
	{
		first_rank = ranges[i][0];
		last_rank  = ranges[i][1];
		stride     = ranges[i][2];

		if ((first_rank < 0) || (first_rank >= group_size) || (last_rank < 0) || (last_rank >= group_size) || (stride == 0))
		{
			MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
		}

		if ((first_rank < last_rank))
		{
			if (stride < 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index <= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else if (first_rank > last_rank)
		{
			if (stride > 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index >= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else
		{
			index = first_rank;
			if (elements_int_list[index] != -1)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}
			elements_int_list[index] = i;
		}
	}
	free ( elements_int_list);

/* computing the ranks to be included */
    count = 0;
    /* determine the number of included processes for the range-incl-method */
    k = 0;
    for(j=0 ; j<n ; j++)
    {

        first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
            index = first_rank;
			while (index <= last_rank)
			{
				count ++;
				k++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				count ++;
				k++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
            index = first_rank;
			count ++;
			k++;
		}
    }
    if (0 != count)
    {
        ranks_included = (int *)malloc( (count)*(sizeof(int)));
    }

    /* determine the list of included processes for the range-incl method */
    k = 0;
    for(j=0 ; j<n ; j++)
    {
		first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
			index = first_rank;
			while (index <= last_rank)
			{
				ranks_included[k] = index;
				k++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				ranks_included[k] = index;
				k++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
			index = first_rank;
			ranks_included[k] = index;
			k++;
		}
    }
    /* including the ranks in the new group */
    result =
        __INTERNAL__PMPI_Group_incl(mpi_group, k, ranks_included, mpi_newgroup);

    if (NULL != ranks_included) {
      sctk_free(ranks_included);
    }
    return result;
}

static int __INTERNAL__PMPI_Group_free(MPI_Group *mpi_group) {
  int res;
  MPC_Group group;

  if (*mpi_group == MPI_GROUP_EMPTY) {
    //    *mpi_group=MPI_GROUP_NULL
    return MPI_SUCCESS;
  }

  if (/*(*mpi_group == MPI_GROUP_EMPTY)
  || */ (*mpi_group == MPI_GROUP_NULL))
    return MPI_ERR_ARG;

  group = __sctk_convert_mpc_group(*mpi_group);

  res = PMPC_Group_free(&group);

  __sctk_delete_mpc_group(mpi_group);

  return res;
}

static int __INTERNAL__PMPI_Comm_size(MPI_Comm comm, int *size) {


  static int comm_world_size = -1;
  int ret = MPI_SUCCESS;

  if( (comm == MPI_COMM_WORLD) && (0<comm_world_size) )
  {
    *size = comm_world_size;
  }
  else
  {
    static __thread int last_comm = -1;
    static __thread int last_rank = -1;
    static __thread int last_size = -1;

    if (last_comm == comm) {
      if (last_rank == sctk_get_task_rank()) {
        *size = last_size;
        return ret;
      }
    }

    int ret = PMPC_Comm_size(comm, size);

    last_rank = sctk_get_task_rank();
    last_comm = comm;
    last_size = *size;

    if( comm == MPC_COMM_WORLD )
    {
      comm_world_size = *size;
    }
  }

  return ret;
}


static int __INTERNAL__PMPI_Comm_rank(MPI_Comm comm, int *rank) {
  static __thread int last_comm = -1;
  static __thread int last_rank = -1;
  static __thread int last_crank = -1;

  if (last_comm == comm) {
    if (last_rank == sctk_get_task_rank()) {
        if (!__MPC_Maybe_disguised()) {
            *rank = last_crank;
  	    return MPI_SUCCESS;
        }
     }
  }

  int ret = PMPC_Comm_rank(comm, rank);

  if( (ret == MPI_SUCCESS) && (!__MPC_Maybe_disguised()) )
  {
 	//sctk_error("SAVE %d@%d %p", *rank , comm,  rank); 
  	last_rank = sctk_get_task_rank();
  	last_comm = comm;
  	last_crank = *rank;

  }

  return ret;
}

static int __INTERNAL__PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2,
                                         int *result) {
  int result_group;
  *result = MPI_UNEQUAL;

  if (sctk_is_inter_comm(comm1) != sctk_is_inter_comm(comm2)) {
    *result = MPI_UNEQUAL;
    return MPI_SUCCESS;
  }

  if (comm1 == comm2) {
    *result = MPI_IDENT;
    return MPI_SUCCESS;
  } else {
    if (sctk_get_nb_task_total(comm1) != sctk_get_nb_task_total(comm2)) {
      *result = MPI_UNEQUAL;
      return MPI_SUCCESS;
    }

    if (comm1 == MPI_COMM_SELF || comm2 == MPI_COMM_SELF) {
      if (sctk_get_nb_task_total(comm1) != sctk_get_nb_task_total(comm2))
        *result = MPI_UNEQUAL;
      else
        *result = MPI_CONGRUENT;
      return MPI_SUCCESS;
    }

    MPI_Group comm_group1;
    MPI_Group comm_group2;

    __INTERNAL__PMPI_Comm_group(comm1, &comm_group1);
    __INTERNAL__PMPI_Comm_group(comm2, &comm_group2);

    __INTERNAL__PMPI_Group_compare(comm_group1, comm_group2, &result_group);

    if (result_group == MPI_SIMILAR) {
      *result = MPI_SIMILAR;
      return MPI_SUCCESS;
    } else if (result_group == MPI_IDENT) {
      *result = MPI_CONGRUENT;
      return MPI_SUCCESS;
    } else {
      *result = MPI_UNEQUAL;
      return MPI_SUCCESS;
    }
  }

  return MPI_SUCCESS;
}

static int SCTK__MPI_Comm_communicator_dup(MPI_Comm comm, MPI_Comm newcomm);
static int SCTK__MPI_Comm_communicator_free(MPI_Comm comm);

static int __INTERNAL__PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  int res;
  MPI_Errhandler err;
  res = PMPC_Comm_dup(comm, newcomm);
  if (res != MPI_SUCCESS) {
    *newcomm = MPI_COMM_NULL;
    return res;
  }
  res = __INTERNAL__PMPI_Errhandler_get(comm, &err);
  if (res != MPI_SUCCESS) {
    *newcomm = MPI_COMM_NULL;
    return res;
  }
  res = __INTERNAL__PMPI_Errhandler_set(*newcomm, err);
  if (res != MPI_SUCCESS) {
    *newcomm = MPI_COMM_NULL;
    return res;
  }
  res = SCTK__MPI_Attr_communicator_dup(comm, *newcomm);
  if (res != MPI_SUCCESS) {
    *newcomm = MPI_COMM_NULL;
    return res;
  }
  res = SCTK__MPI_Comm_communicator_dup(comm, *newcomm);
  if (res != MPI_SUCCESS)
    *newcomm = MPI_COMM_NULL;
  return res;
}

static int __INTERNAL__PMPI_Comm_create(MPI_Comm comm, MPI_Group group,
                                        MPI_Comm *newcomm) {
  return PMPC_Comm_create(comm, __sctk_convert_mpc_group(group), newcomm);
}

static int __INTERNAL__PMPI_Comm_create_from_intercomm(MPI_Comm comm,
                                                       MPI_Group group,
                                                       MPI_Comm *newcomm) {
  return PMPC_Comm_create_from_intercomm(comm, __sctk_convert_mpc_group(group),
                                         newcomm);
}

static inline int sctk_MPI_split_on_roots(MPI_Comm comm, int chosen_arity,
                                          MPI_Comm *leaf_comm,
                                          MPI_Comm *root_comm) {
  int size, rank;
  PMPI_Comm_size(comm, &size);
  PMPI_Comm_rank(comm, &rank);

  /* First check the split threshold */
  if (size == 1) {
    *leaf_comm = MPI_COMM_NULL;
    *root_comm = MPI_COMM_NULL;
    return 0;
  }

  /* Generate Leaf Comm */
  PMPC_Comm_split(comm, rank / chosen_arity, rank, leaf_comm);

  int leaf_comm_rank;
  PMPI_Comm_rank(*leaf_comm, &leaf_comm_rank);

  int root_color = MPI_UNDEFINED;

  if (leaf_comm_rank == 0) {
    root_color = 1;
  }

  /* Generate Root Comm */
  PMPC_Comm_split(comm, root_color, rank, root_comm);

  return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Comm_split(MPI_Comm comm, int color, int key,
                                       MPI_Comm *newcomm) {
  int res;
  res = PMPC_Comm_split(comm, color, key, newcomm);
  return res;
}

static int __INTERNAL__PMPI_Comm_free(MPI_Comm *comm) {

  mpc_mpi_per_communicator_t *per_comm = mpc_mpc_get_per_comm_data(*comm);

  int res;
  res = SCTK__MPI_Attr_clean_communicator(*comm);
  if (res != MPI_SUCCESS) {
    return res;
  }
  SCTK__MPI_Comm_communicator_free(*comm);
  if (res != MPI_SUCCESS) {
    return res;
  }

  return PMPC_Comm_free(comm);
}

static int
__INTERNAL__PMPI_Comm_test_inter (MPI_Comm comm, int *flag)
{
  int res = MPI_SUCCESS;
  *flag = sctk_is_inter_comm (comm);
  return res;
}

static int
__INTERNAL__PMPI_Comm_remote_size (MPI_Comm comm, int *size)
{
  return PMPC_Comm_remote_size (comm, size);
}

static int
__INTERNAL__PMPI_Comm_remote_group (MPI_Comm comm, MPI_Group * mpi_newgroup)
{
  MPI_internal_group_t *newgroup;
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  return PMPC_Comm_remote_group (comm, &(newgroup->group));
}
static int
__INTERNAL__PMPI_Intercomm_create (MPI_Comm local_comm, int local_leader,
				   MPI_Comm peer_comm, int remote_leader,
				   int tag, MPI_Comm * newintercomm)
{
  return PMPC_Intercomm_create (local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
}

static int
__INTERNAL__PMPI_Intercomm_merge (MPI_Comm intercomm, int high,
				  MPI_Comm * newintracomm)
{
	int rank, grank, remote_high, remote_leader, local_leader;
	sctk_communicator_t peer_comm;
	MPI_Group local_group;
	MPI_Group remote_group;
	MPI_Group new_group;
	MPC_Status status;

	__INTERNAL__PMPI_Comm_rank (MPI_COMM_WORLD, &rank);
	__INTERNAL__PMPI_Comm_rank (intercomm, &grank);
	__INTERNAL__PMPI_Comm_group (intercomm, &local_group);
	__INTERNAL__PMPI_Comm_remote_group (intercomm, &remote_group);

	remote_leader = sctk_get_remote_leader(intercomm);
	local_leader = sctk_get_local_leader(intercomm);
	peer_comm = sctk_get_peer_comm(intercomm);
	if(grank == local_leader)
	{
		sctk_nodebug("grank = %d, rank %d : local_leader %d send to remote_leader %d", grank, rank, local_leader, remote_leader);
		__INTERNAL__PMPI_Sendrecv(&high, 1, MPC_INT, remote_leader, 629, &remote_high, 1, MPC_INT, remote_leader, 629, peer_comm, &status);
	}

	sctk_nodebug("rank %d, grank %d : boroadcast comm %d", rank, grank, intercomm);
	sctk_broadcast(&remote_high,sizeof(int),0,sctk_get_local_comm_id(intercomm));
	
	sctk_nodebug("rank %d : merge intercomm %d, high = %d, remote_high = %d", rank, intercomm, high, remote_high);
	/* TODO : Finir le merge avec le placement des groupes en fonction des paramètres "high" */
	if(sctk_is_in_local_group(intercomm))
	{
		if(high && !remote_high)
		{
			sctk_nodebug("high && !remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
		else if(!high && remote_high)
		{
			sctk_nodebug("!high && remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
		else if(high && remote_high)
		{
			sctk_nodebug("high && remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
		else
		{
			sctk_nodebug("!high && !remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
	}
	else
	{
		if(high && !remote_high)
		{
			sctk_nodebug("high && !remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
		else if(!high && remote_high)
		{
			sctk_nodebug("!high && remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
		else if(high && remote_high)
		{
			sctk_nodebug("high && remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
		else
		{
			sctk_nodebug("!high && !remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
	}

	return __INTERNAL__PMPI_Comm_create_from_intercomm (intercomm, new_group, newintracomm);
}

/************************************************************************/
/* Attribute Handling                                                   */
/************************************************************************/

static int MPI_TAG_UB_VALUE = 512*1024*1024;
static char *MPI_HOST_VALUE[4096];
static int MPI_IO_VALUE = 0;
static int MPI_WTIME_IS_GLOBAL_VALUE = 0;
static int MPI_APPNUM_VALUE;
static int MPI_UNIVERSE_SIZE_VALUE;
static int MPI_LASTUSEDCODE_VALUE;

typedef int (MPI_Copy_function_fortran) (MPI_Comm *, int *, int *, int *, int *, int *, int *);
typedef int (MPI_Delete_function_fortran) (MPI_Comm *, int *, int *, int *, int *);

static void *defines_attr_tab[MPI_MAX_KEY_DEFINED] = 
{
	(void *) &MPI_TAG_UB_VALUE /*MPI_TAG_UB */ ,
	&MPI_HOST_VALUE,
	&MPI_IO_VALUE,
	&MPI_WTIME_IS_GLOBAL_VALUE,
	&MPI_APPNUM_VALUE,
	&MPI_UNIVERSE_SIZE_VALUE,
	&MPI_LASTUSEDCODE_VALUE

};


static int __INTERNAL__PMPI_Keyval_create (MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn, int *keyval, void *extra_state)
{
	int i;
	mpc_mpi_data_t* tmp;

	tmp = mpc_mpc_get_per_task_data();
	sctk_nodebug("number = %d, max_number = %d", tmp->number, tmp->max_number);
	sctk_spinlock_lock(&(tmp->lock));
	if (tmp->number >= tmp->max_number)
	{
		tmp->max_number += 100;
		tmp->attrs_fn = sctk_realloc ((void *) (tmp->attrs_fn), tmp->max_number * sizeof (MPI_Caching_key_t));
		for (i = tmp->number; i < tmp->max_number; i++)
		{
			tmp->attrs_fn[i].used = 0;
		}
	}

	*keyval = tmp->number;
	tmp->number++;

	tmp->attrs_fn[*keyval].copy_fn = copy_fn;
	tmp->attrs_fn[*keyval].delete_fn = delete_fn;
	tmp->attrs_fn[*keyval].extra_state = extra_state;
	tmp->attrs_fn[*keyval].used = 1;
	tmp->attrs_fn[*keyval].fortran_key = 0;

	sctk_spinlock_unlock(&(tmp->lock));
	*keyval += MPI_MAX_KEY_DEFINED;
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn,
										int *comm_keyval, void *extra_state)
{
		return __INTERNAL__PMPI_Keyval_create( comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
	
}

static int __INTERNAL__PMPI_Comm_free_keyval(int *comm_keyval)
{
		*comm_keyval = MPI_KEYVAL_INVALID;
		return MPI_SUCCESS;
	
}

static int __INTERNAL__PMPI_Keyval_free (int *keyval)
{
	TODO("Optimize to free memory")
	*keyval = MPI_KEYVAL_INVALID;
	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Attr_set_fortran (int keyval)
{
	int res = MPI_SUCCESS;
	MPI_Comm comm = MPI_COMM_WORLD;
	mpc_mpi_data_t* tmp;

	tmp = mpc_mpc_get_per_task_data();

	if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
	{
		MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
	}

	keyval -= MPI_MAX_KEY_DEFINED;

	if (keyval < 0)
	{
		MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
	}

	sctk_spinlock_lock(&(tmp->lock));
	
	if (tmp->attrs_fn[keyval].used == 0)
	{
		sctk_spinlock_unlock(&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
	}

	tmp->attrs_fn[keyval].fortran_key = 1;
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}






static int __INTERNAL__PMPI_Attr_put (MPI_Comm comm, int keyval, void *attr_value)
{
	int res = MPI_SUCCESS;
	mpc_mpi_data_t* tmp;
	mpc_mpi_per_communicator_t* tmp_per_comm;
	int i;

	tmp = mpc_mpc_get_per_task_data();
	keyval -= MPI_MAX_KEY_DEFINED;

	if (keyval < 0)
	{
		sctk_nodebug("wrong keyval");
		MPI_ERROR_REPORT (comm, MPI_ERR_KEYVAL, "");
	}

	sctk_spinlock_lock(&(tmp->lock));
	if (tmp->attrs_fn[keyval].used == 0)
	{
		sctk_nodebug("not used");
		sctk_spinlock_unlock(&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_KEYVAL, "");
	}

	tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
	sctk_spinlock_lock(&(tmp_per_comm->lock));

	if(tmp_per_comm->max_number <= keyval)
	{
		if(tmp_per_comm->key_vals == NULL)
			tmp_per_comm->key_vals = sctk_malloc((keyval+1)*sizeof(MPI_Caching_key_value_t));
		else
			tmp_per_comm->key_vals = sctk_realloc(tmp_per_comm->key_vals,(keyval+1)*sizeof(MPI_Caching_key_value_t));

		for(i = tmp_per_comm->max_number; i <= keyval; i++)
		{
			tmp_per_comm->key_vals[i].flag = 0;
			tmp_per_comm->key_vals[i].attr = NULL;
		}

		tmp_per_comm->max_number = keyval+1;
	}
	
	if((tmp_per_comm->key_vals != NULL) && (tmp_per_comm->key_vals[keyval].flag == 1))
	{
		sctk_spinlock_unlock(&(tmp_per_comm->lock));
		sctk_spinlock_unlock(&(tmp->lock));
		res = __INTERNAL__PMPI_Attr_delete (comm, keyval + MPI_MAX_KEY_DEFINED);
		sctk_spinlock_lock(&(tmp->lock));
		sctk_spinlock_lock(&(tmp_per_comm->lock));
		if(res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock(&(tmp_per_comm->lock));
			sctk_spinlock_unlock(&(tmp->lock));
			return res;
		}
	}

	if (tmp->attrs_fn[keyval].fortran_key == 0)
	{
		sctk_nodebug("put %d for keyval %d", *((int *)attr_value), keyval);
		tmp_per_comm->key_vals[keyval].attr = (void *) attr_value;
	}
	else
	{
		long tmp = 0;
		tmp = tmp + *(long *)attr_value;
		tmp_per_comm->key_vals[keyval].attr = (void *)tmp;
	}

	tmp_per_comm->key_vals[keyval].flag = 1;

	sctk_spinlock_unlock(&(tmp_per_comm->lock));
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}

static int __INTERNAL__PMPI_Attr_get (MPI_Comm comm, int keyval, void *attr_value,   int *flag)
{
	int res = MPI_SUCCESS;
	mpc_mpi_data_t* tmp;
	mpc_mpi_per_communicator_t* tmp_per_comm;
	void **attr;
	int i;

	*flag = 0;
	attr = (void **) attr_value;

	if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
	{
		*flag = 1;
		*attr = defines_attr_tab[keyval];
		return MPI_SUCCESS;
	}

	keyval -= MPI_MAX_KEY_DEFINED;

	/* wrong keyval */
	if (keyval < 0)
	{
		MPI_ERROR_REPORT (comm, MPI_ERR_KEYVAL, "");
	}

	/* get TLS var for checking if keyval exist */
	tmp = mpc_mpc_get_per_task_data();
	sctk_spinlock_lock(&(tmp->lock));

	/* it doesn-t exist */
	if (tmp->attrs_fn[keyval].used == 0)
		{
		*flag = 0;
		sctk_spinlock_unlock(&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
	}

	/* get TLS var to check attributes for keyval */
	tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
	sctk_spinlock_lock(&(tmp_per_comm->lock));

	/* it doesn't have any */
	if(tmp_per_comm->key_vals == NULL )
	{
		*flag = 0;
		*attr = NULL;
	}
	else if(keyval >= tmp_per_comm->max_number )
	{
		*flag = 0;
		*attr = NULL;
	}
    else if(tmp_per_comm->key_vals[keyval].flag == 0)
	{
		*flag = 0;
		*attr = NULL;
	}
	else /* we found one */
	{
		*flag = 1;
		if (tmp->attrs_fn[keyval].fortran_key == 0)
		{
        	*attr = tmp_per_comm->key_vals[keyval].attr;
		}
		else
		{
			long tmp;
			tmp = (long)tmp_per_comm->key_vals[keyval].attr;
			*attr = (void*)tmp;
		}
    }

	sctk_spinlock_unlock(&(tmp_per_comm->lock));
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}

static int __INTERNAL__PMPI_Attr_get_fortran (MPI_Comm comm, int keyval, int *attr_value,   int *flag){
        if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
        {	
		long tmp;
                *flag = 1;
		*attr_value = *((int*)(defines_attr_tab[keyval]));
                return MPI_SUCCESS;
        } else {
                return __INTERNAL__PMPI_Attr_get(comm,keyval,(void*)attr_value,flag);
        }
}

static int __INTERNAL__PMPI_Attr_delete (MPI_Comm comm, int keyval)
{
	int res = MPI_SUCCESS;
	mpc_mpi_data_t* tmp;
	mpc_mpi_per_communicator_t* tmp_per_comm;
	if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
	{
	return MPI_ERR_KEYVAL;
	}
	keyval -= MPI_MAX_KEY_DEFINED;

	tmp = mpc_mpc_get_per_task_data();
	sctk_spinlock_lock(&(tmp->lock));

	if ((tmp == NULL) || (keyval < 0))
	{
	sctk_spinlock_unlock(&(tmp->lock));
	return MPI_ERR_ARG;
	}
	if ((tmp->attrs_fn[keyval].used == 0) )
	{
	sctk_spinlock_unlock(&(tmp->lock));
	return MPI_ERR_ARG;
	}

	tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
	sctk_spinlock_lock(&(tmp_per_comm->lock));

	if (tmp_per_comm->key_vals[keyval].flag == 1)
	{
		if (tmp->attrs_fn[keyval].delete_fn != NULL)
		{
			if (tmp->attrs_fn[keyval].fortran_key == 0)
			{
				sctk_spinlock_unlock(&(tmp_per_comm->lock));
				sctk_spinlock_unlock(&(tmp->lock));
				res = tmp->attrs_fn[keyval].delete_fn (comm, keyval + MPI_MAX_KEY_DEFINED, 
								       tmp_per_comm->key_vals[keyval].attr,
								       tmp->attrs_fn[keyval].extra_state );
				sctk_spinlock_lock(&(tmp_per_comm->lock));
				sctk_spinlock_lock(&(tmp->lock));
			}
			else
			{
				int fort_key;
				int val;
				long long_val;
				int *ext;
				long_val = (long) (tmp_per_comm->key_vals[keyval].attr);
				val = (int) long_val;
				fort_key = keyval + MPI_MAX_KEY_DEFINED;
				ext = (int *) (tmp->attrs_fn[keyval].extra_state);

				sctk_spinlock_unlock(&(tmp_per_comm->lock));
				sctk_spinlock_unlock(&(tmp->lock));
				((MPI_Delete_function_fortran *) tmp->attrs_fn[keyval].delete_fn) (&comm, &fort_key, &val, ext, &res);
				sctk_spinlock_lock(&(tmp_per_comm->lock));
				sctk_spinlock_lock(&(tmp->lock));
			}
		}
	}

	if(res == MPI_SUCCESS)
	{
		tmp_per_comm->key_vals[keyval].attr = NULL;
		tmp_per_comm->key_vals[keyval].flag = 0;
	}

	sctk_spinlock_unlock(&(tmp_per_comm->lock));
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}

static int SCTK__MPI_Attr_clean_communicator (MPI_Comm comm)
{
	int res = MPI_SUCCESS;
	int i;
	mpc_mpi_data_t* tmp;
	mpc_mpi_per_communicator_t* tmp_per_comm;


	tmp = mpc_mpc_get_per_task_data();
	sctk_spinlock_lock(&(tmp->lock));
	tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
	sctk_spinlock_lock(&(tmp_per_comm->lock));

	for (i = 0; i < tmp_per_comm->max_number; i++)
	{
		if (tmp_per_comm->key_vals[i].flag == 1)
		{
			sctk_spinlock_unlock(&(tmp_per_comm->lock));
			sctk_spinlock_unlock(&(tmp->lock));
			res = __INTERNAL__PMPI_Attr_delete (comm, i + MPI_MAX_KEY_DEFINED);
			sctk_spinlock_lock(&(tmp->lock));
			sctk_spinlock_lock(&(tmp_per_comm->lock));

			if (res != MPI_SUCCESS)
			{
				sctk_spinlock_unlock(&(tmp_per_comm->lock));
				sctk_spinlock_unlock(&(tmp->lock));
				return res;
			}

		}
	}
	
	sctk_spinlock_unlock(&(tmp_per_comm->lock));
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}

static int SCTK__MPI_Attr_communicator_dup (MPI_Comm prev , MPI_Comm newcomm)
{
  int res = MPI_SUCCESS;
  mpc_mpi_data_t* tmp;
  mpc_mpi_per_communicator_t* tmp_per_comm_old;
  mpc_mpi_per_communicator_t* tmp_per_comm_new;
  int i;

  tmp = mpc_mpc_get_per_task_data();
  sctk_spinlock_lock(&(tmp->lock));
  tmp_per_comm_old = mpc_mpc_get_per_comm_data(prev);
  sctk_spinlock_lock(&(tmp_per_comm_old->lock));

  tmp_per_comm_new = mpc_mpc_get_per_comm_data(newcomm);
  tmp_per_comm_new->key_vals = sctk_malloc(tmp_per_comm_old->max_number*sizeof(MPI_Caching_key_value_t));
  tmp_per_comm_new->max_number = tmp_per_comm_old->max_number;

  for(i = 0; i < tmp_per_comm_old->max_number; i++){
    tmp_per_comm_new->key_vals[i].flag = 0;
    tmp_per_comm_new->key_vals[i].attr = NULL;
  }

  for (i = 0; i < tmp_per_comm_old->max_number; i++)
    {
      if (tmp->attrs_fn[i].copy_fn != NULL)
	{
	  if (tmp_per_comm_old->key_vals[i].flag == 1)
	    {
	      void *arg;
	      int flag = 0;
	      MPI_Copy_function *cpy;

	      cpy = tmp->attrs_fn[i].copy_fn;

	      if (tmp->attrs_fn[i].fortran_key == 0)
		{
		  res =
		    cpy (prev, i + MPI_MAX_KEY_DEFINED,
			 tmp->attrs_fn[i].extra_state,
			 tmp_per_comm_old->key_vals[i].attr, (void *) (&arg), &flag);
		}
	      else
		{
		  int fort_key;
		  int val;
		  int *ext;
		  int val_out;
		  long long_val;
		  long_val = (long) (tmp_per_comm_old->key_vals[i].attr);
		  val = (int) long_val;
		  fort_key = i + MPI_MAX_KEY_DEFINED;
		  ext = (int *) (tmp->attrs_fn[i].extra_state);
		  sctk_nodebug ("%d val", val);
		  ((MPI_Copy_function_fortran *) cpy) (&prev, &fort_key, ext, &val, &val_out, &flag, &res);
		  sctk_nodebug ("%d val_out", val_out);
		  arg = &val_out;
		}
	      sctk_nodebug ("i = %d Copy %d %ld->%ld flag %d", i, i + MPI_MAX_KEY_DEFINED,
			    (unsigned long) tmp_per_comm_old->key_vals[i].attr,
			    (unsigned long) arg, flag);
	      if (flag)
		{
			sctk_spinlock_unlock(&(tmp_per_comm_old->lock));
			sctk_spinlock_unlock(&(tmp->lock));
			sctk_nodebug("arg = %d", *(((int *)arg)));
			__INTERNAL__PMPI_Attr_put (newcomm, i + MPI_MAX_KEY_DEFINED, arg);
			sctk_spinlock_lock(&(tmp->lock));
			sctk_spinlock_lock(&(tmp_per_comm_old->lock));
		}
	      if (res != MPI_SUCCESS)
		{
		  sctk_spinlock_unlock(&(tmp_per_comm_old->lock));
		  sctk_spinlock_unlock(&(tmp->lock));
		  return res;
		}

	    }
	}
    }
  sctk_spinlock_unlock(&(tmp_per_comm_old->lock));
  sctk_spinlock_unlock(&(tmp->lock));
  return res;
}



static void __sctk_init_mpi_topo ()
{
}

static int SCTK__MPI_Comm_communicator_dup (MPI_Comm comm, MPI_Comm newcomm)
{
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo_old;
	mpi_topology_per_comm_t* topo_new;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo_old = &(tmp->topo);
	tmp = mpc_mpc_get_per_comm_data(newcomm);
	topo_new = &(tmp->topo);

	sctk_spinlock_lock (&(topo_old->lock));

	if (topo_old->type == MPI_CART)
	{
		topo_new->type = MPI_CART;
		topo_new->data.cart.ndims = topo_old->data.cart.ndims;
		topo_new->data.cart.reorder = topo_old->data.cart.reorder;
		topo_new->data.cart.dims = sctk_malloc (topo_old->data.cart.ndims * sizeof (int));
		memcpy (topo_new->data.cart.dims, topo_old->data.cart.dims, topo_old->data.cart.ndims * sizeof (int));
		topo_new->data.cart.periods = sctk_malloc (topo_old->data.cart.ndims * sizeof (int));
		memcpy (topo_new->data.cart.periods, topo_old->data.cart.periods, topo_old->data.cart.ndims * sizeof (int));
	}

	if (topo_old->type == MPI_GRAPH)
	{
		topo_new->type = MPI_GRAPH;

		topo_new->data.graph.nnodes = topo_old->data.graph.nnodes;
		topo_new->data.graph.reorder = topo_old->data.graph.reorder;
		topo_new->data.graph.index = sctk_malloc ( topo_old->data.graph.nindex * sizeof (int));
		memcpy (topo_new->data.graph.index, topo_old->data.graph.index,  topo_old->data.graph.nindex * sizeof (int));
		topo_new->data.graph.edges = sctk_malloc (topo_old->data.graph.nedges * sizeof (int));
		memcpy (topo_new->data.graph.edges, topo_old->data.graph.edges, topo_old->data.graph.nedges * sizeof (int));

		topo_new->data.graph.nedges = topo_old->data.graph.nedges;
		topo_new->data.graph.nindex = topo_old->data.graph.nindex;
	}

	sctk_spinlock_unlock (&(topo_old->lock));

	return MPI_SUCCESS;
}

static int SCTK__MPI_Comm_communicator_free (MPI_Comm comm)
{
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	sctk_spinlock_lock (&(topo->lock));

	if (topo->type == MPI_CART)
	{
		sctk_free (topo->data.cart.dims);
		sctk_free (topo->data.cart.periods);
	}
	
	if (topo->type == MPI_GRAPH)
	{
		sctk_free (topo->data.graph.index);
		sctk_free (topo->data.graph.edges);
	}

        topo->type = MPI_UNDEFINED;

        sctk_spinlock_unlock(&(topo->lock));

        sctk_free(tmp->key_vals);
        sctk_free(tmp);
        return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Comm_get_name (MPI_Comm comm, char *comm_name,
				int *resultlen)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int len;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));
  len = strlen (topo->names);

  if (strcmp(topo->names, "undefined") == 0)
  {
	  if(comm == MPI_COMM_WORLD)
	  {
		  sprintf(comm_name, "MPI_COMM_WORLD");
		  len = strlen (comm_name);
		  *resultlen = len;
	  }
	  else if(comm == MPI_COMM_SELF)
	  {
		  sprintf(comm_name, "MPI_COMM_SELF");
		  len = strlen (comm_name);
		  *resultlen = len;
	  }
	  else
	  {
		  memcpy (comm_name, topo->names, len + 1);
		  *resultlen = len;
	  }
	  sctk_spinlock_unlock (&(topo->lock));
  }
  else
  {
	  memcpy (comm_name, topo->names, len + 1);
	  *resultlen = len;
	  sctk_spinlock_unlock (&(topo->lock));
  }
  return MPI_SUCCESS;
}
static int
__INTERNAL__PMPI_Comm_set_name (MPI_Comm comm, char *comm_name)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int len;
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));
  memset (topo->names, '\0', MPI_MAX_NAME_STRING + 1);

  len = strlen (comm_name);
  if (len > MPI_MAX_NAME_STRING)
    {
      len = MPI_MAX_NAME_STRING;
    }
  memcpy (topo->names, comm_name, len);

  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Topo_test (MPI_Comm comm, int *topo_type)
{
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	sctk_spinlock_lock (&(topo->lock));

	if((topo->type == MPI_CART) || (topo->type == MPI_GRAPH))
		*topo_type = topo->type;
	else
		*topo_type = MPI_UNDEFINED;


	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims,
			      int *periods, int reorder, MPI_Comm * comm_cart)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int res = MPI_ERR_INTERN;
  int nb_tasks = 1;
  int size;
  int i;
  int rank;

  __INTERNAL__PMPI_Comm_rank (comm_old, &rank);

  for (i = 0; i < ndims; i++)
    {
	  if(dims[i] <= 0)
		MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "One of the dimensions is equal or less than zero");
      nb_tasks *= dims[i];
      sctk_nodebug ("dims[%d] = %d", i, dims[i]);
    }

  __INTERNAL__PMPI_Comm_size (comm_old, &size);

  sctk_nodebug ("%d <= %d", nb_tasks, size);
  if (nb_tasks > size)
    {
      MPI_ERROR_REPORT (comm_old, MPI_ERR_COMM, "too small group size");
    }

INFO("Very simple approach never reorder nor take care of hardware topology")

  if (nb_tasks == size)
    {
      res = __INTERNAL__PMPI_Comm_dup (comm_old, comm_cart);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  else
    {
      MPI_Group comm_group;
      MPI_Group new_group;
      int ranges[1][3];

      res = __INTERNAL__PMPI_Comm_group (comm_old, &comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      /*Retire ranks higher than nb_tasks */

      ranges[0][0] = nb_tasks;
      ranges[0][1] = size-1;
      ranges[0][2] = 1;

      res =
	__INTERNAL__PMPI_Group_range_excl (comm_group, 1, ranges, &new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      res = __INTERNAL__PMPI_Comm_create (comm_old, new_group, comm_cart);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }

	if(*comm_cart != MPC_COMM_NULL)
	{
		tmp = mpc_mpc_get_per_comm_data(*comm_cart);
		topo = &(tmp->topo);
		sctk_spinlock_lock (&(topo->lock));
			topo->type = MPI_CART;
			topo->data.cart.ndims = ndims;
			topo->data.cart.reorder = reorder;
			topo->data.cart.dims = sctk_malloc (ndims * sizeof (int));
			memcpy (topo->data.cart.dims, dims, ndims * sizeof (int));
			topo->data.cart.periods = sctk_malloc (ndims * sizeof (int));
			memcpy (topo->data.cart.periods, periods, ndims * sizeof (int));
		sctk_spinlock_unlock (&(topo->lock));
	}
	return res;
}

/*
 *  assignnodes
 *
 *  Function:   - assign processes to dimensions
 *          - get "best-balanced" grid
 *          - greedy bin-packing algorithm used
 *          - sort dimensions in decreasing order
 *          - dimensions array dynamically allocated
 *  Accepts:    - # of dimensions
 *          - # of prime factors
 *          - array of prime factors
 *          - array of factor counts
 *          - ptr to array of dimensions (returned value)
 *  Returns:    - 0 or ERROR
 */
static int
assignnodes(int ndim, int nfactor, int *pfacts, int *counts, int **pdims)
{
    int *bins;
    int i, j;
    int n;
    int f;
    int *p;
    int *pmin;

    if (0 >= ndim) {
       return MPI_ERR_DIMS;
    }

    /* Allocate and initialize the bins */
    bins = (int *) sctk_malloc((unsigned) ndim * sizeof(int));
    if (NULL == bins) {
       return MPI_ERR_INTERN;
    }
    *pdims = bins;

    for (i = 0, p = bins; i < ndim; ++i, ++p) {
        *p = 1;
     }

    /* Loop assigning factors from the highest to the lowest */
    for (j = nfactor - 1; j >= 0; --j) {
       f = pfacts[j];
       for (n = counts[j]; n > 0; --n) {
            /* Assign a factor to the smallest bin */
            pmin = bins;
            for (i = 1, p = pmin + 1; i < ndim; ++i, ++p) {
                if (*p < *pmin) {
                    pmin = p;
                }
            }
            *pmin *= f;
        }
     }

     /* Sort dimensions in decreasing order (O(n^2) for now) */
     for (i = 0, pmin = bins; i < ndim - 1; ++i, ++pmin) {
         for (j = i + 1, p = pmin + 1; j < ndim; ++j, ++p) {
             if (*p > *pmin) {
                n = *p;
                *p = *pmin;
                *pmin = n;
             }
         }
     }

     return MPI_SUCCESS;
}

/*
 *  getfactors
 *
 *  Function:   - factorize a number
 *  Accepts:    - number
 *          - # of primes
 *          - array of primes
 *          - ptr to array of counts (returned value)
 *  Returns:    - 0 or ERROR
 */
static int
getfactors(int num, int nprime, int *primes, int **pcounts)
{
    int *counts;
    int i;
    int *p;
    int *c;

    if (0 >= nprime) {
        return MPI_ERR_INTERN;
    }

    /* Allocate the factor counts array */
    counts = (int *) sctk_malloc((unsigned) nprime * sizeof(int));
    if (NULL == counts) {
       return MPI_ERR_INTERN;
    }

    *pcounts = counts;

    /* Loop over the prime numbers */
    i = nprime - 1;
    p = primes + i;
    c = counts + i;

    for (; i >= 0; --i, --p, --c) {
        *c = 0;
        while ((num % *p) == 0) {
            ++(*c);
            num /= *p;
        }
    }

    if (1 != num) {
        return MPI_ERR_INTERN;
    }

    return MPI_SUCCESS;
}

/*
 *  getprimes
 *
 *  Function:   - get primes smaller than number
 *          - array of primes dynamically allocated
 *  Accepts:    - number
 *          - ptr # of primes (returned value)
 *          - ptr array of primes (returned values)
 *  Returns:    - 0 or ERROR
 */
static int
getprimes(int num, int *pnprime, int **pprimes) {

   int i, j;
   int n;
   int size;
   int *primes;

   /* Allocate the array of primes */
   size = (num / 2) + 1;
   primes = (int *) sctk_malloc((unsigned) size * sizeof(int));
   if (NULL == primes) {
       return MPI_ERR_INTERN;
   }
   *pprimes = primes;

   /* Find the prime numbers */
   i = 0;
   primes[i++] = 2;

   for (n = 3; n <= num; n += 2) {
      for (j = 1; j < i; ++j) {
         if ((n % primes[j]) == 0) {
             break;
          }
      }

      if (j == i) {
        if (i >= size) {
           return MPI_ERR_DIMS;
         }
         primes[i++] = n;
      }
   }

   *pnprime = i;
   return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Dims_create (int nnodes, int ndims, int *dims)
{
    int i;
    int freeprocs;
    int freedims;
    int nprimes;
    int *primes;
    int *factors;
    int *procs;
    int *p;
    int err;

	if (NULL == dims)
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid arg dims");

	if (1 > ndims)
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");

    /* Get # of free-to-be-assigned processes and # of free dimensions */
    freeprocs = nnodes;
    freedims = 0;
    for (i = 0, p = dims; i < ndims; ++i,++p)
    {
        if (*p == 0)
            ++freedims;
        else if ((*p < 0) || ((nnodes % *p) != 0))
            MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
        else
            freeprocs /= *p;
    }

    if (freedims == 0)
    {
		if (freeprocs == 1)
			return MPI_SUCCESS;
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
    }

    if (freeprocs < 1)
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
    else if (freeprocs == 1)
    {
        for (i = 0; i < ndims; ++i, ++dims)
        {
            if (*dims == 0)
               *dims = 1;
        }
        return MPI_SUCCESS;
    }

    /* Compute the relevant prime numbers for factoring */
    if (MPI_SUCCESS != (err = getprimes(freeprocs, &nprimes, &primes)))
		MPI_ERROR_REPORT(MPI_COMM_WORLD, err, "Error getprimes");

    /* Factor the number of free processes */
    if (MPI_SUCCESS != (err = getfactors(freeprocs, nprimes, primes, &factors)))
		MPI_ERROR_REPORT(MPI_COMM_WORLD, err, "Error getfactors");

    /* Assign free processes to free dimensions */
    if (MPI_SUCCESS != (err = assignnodes(freedims, nprimes, primes, factors, &procs)))
		MPI_ERROR_REPORT(MPI_COMM_WORLD, err, "Error assignnodes");

    /* Return assignment results */
    p = procs;
    for (i = 0; i < ndims; ++i, ++dims)
    {
        if (*dims == 0)
			*dims = *p++;
    }

    sctk_free((char *) primes);
    sctk_free((char *) factors);
    sctk_free((char *) procs);

	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_create (MPI_Comm comm_old, int nnodes, int *index,
			       int *edges, int reorder, MPI_Comm * comm_graph)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int res;
  int size;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);

INFO("Very simple approach never reorder nor take care of hardware topology")
  if (nnodes == size)
    {
      res = __INTERNAL__PMPI_Comm_dup (comm_old, comm_graph);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  else
    {
      MPI_Group comm_group;
      MPI_Group new_group;
      int ranges[1][3];

      res = __INTERNAL__PMPI_Comm_group (comm_old, &comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      /*Retire ranks higher than nb_tasks */

      ranges[0][0] = nnodes;
      ranges[0][1] = size-1;
      ranges[0][2] = 1;

      res =
	__INTERNAL__PMPI_Group_range_excl (comm_group, 1, ranges, &new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      res = __INTERNAL__PMPI_Comm_create (comm_old, new_group, comm_graph);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }

	if(*comm_graph >= 0)
	{
		tmp = mpc_mpc_get_per_comm_data(*comm_graph);
		topo = &(tmp->topo);
		sctk_spinlock_lock (&(topo->lock));
			topo->type = MPI_GRAPH;
			topo->data.graph.nnodes = nnodes;
			topo->data.graph.reorder = reorder;
			topo->data.graph.index = sctk_malloc (nnodes * sizeof (int));
			memcpy (topo->data.graph.index, index, nnodes * sizeof (int));
			topo->data.graph.edges = sctk_malloc (index[nnodes - 1] * sizeof (int));
			memcpy (topo->data.graph.edges, edges, index[nnodes - 1] * sizeof (int));
			topo->data.graph.nedges = index[nnodes - 1];
			topo->data.graph.nindex = nnodes;
		sctk_spinlock_unlock (&(topo->lock));
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graphdims_get (MPI_Comm comm, int *nnodes, int *nedges)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  *nnodes = topo->data.graph.nnodes;
  *nedges = topo->data.graph.nedges;
  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_get (MPI_Comm comm, int maxindex, int maxedges,
			    int *index, int *edges)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int i;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  for (i = 0; (i < maxindex) && (i < topo->data.graph.nindex);
       i++)
    {
      index[i] = topo->data.graph.index[i];
    }
  for (i = 0; (i < maxedges) && (i < topo->data.graph.nedges);
       i++)
    {
      edges[i] = topo->data.graph.edges[i];
    }

  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cartdim_get (MPI_Comm comm, int *ndims)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type != MPI_CART)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  *ndims = topo->data.cart.ndims;

  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods, int *coords)
{
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	int res;
	int rank;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);

	sctk_spinlock_lock (&(topo->lock));
		if (topo->type != MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}

		if (maxdims != topo->data.cart.ndims)
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
		}

		memcpy (dims, topo->data.cart.dims, maxdims * sizeof (int));
		memcpy (periods, topo->data.cart.periods, maxdims * sizeof (int));

		res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
	sctk_spinlock_unlock (&(topo->lock));
	return res;
}

static int
__INTERNAL__PMPI_Cart_rank (MPI_Comm comm, int *coords, int *rank, int locked)
{
	int loc_rank = 0;
	int dims_coef = 1;
	int i;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	if (!locked)
		sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_CART)
	{
		if (!locked)
			sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	for (i = topo->data.cart.ndims - 1 ; i >= 0 ; i--)
	{
		loc_rank += coords[i] * dims_coef ;
		dims_coef *= topo->data.cart.dims[i] ;
	}

	*rank = loc_rank;
	sctk_nodebug ("rank %d", loc_rank);
	if (!locked)
		sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_coords (MPI_Comm comm, int init_rank, int maxdims,
			      int *coords, int locked)
{
	int i;
	int pi = 1;
	int qi = 1;
	int rank;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	rank = init_rank;

	if (!locked)
		sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_CART)
	{
		if (!locked)
			sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	if (maxdims != topo->data.cart.ndims)
	{
		if (!locked)
			sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
	}

	for (i = maxdims-1; i >= 0; --i)
	{
		pi *= topo->data.cart.dims[i];
		coords[i] = ( rank % pi ) / qi ;
		qi = pi ;
	}

	if (!locked)
		sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_neighbors_count (MPI_Comm comm, int rank,
					int *nneighbors)
{
	int start;
	int end;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_GRAPH)
    {
		sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

	end = topo->data.graph.index[rank];
	start = 0;
	if (rank)
    {
		start = topo->data.graph.index[rank - 1];
    }

	*nneighbors = end - start;


	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_neighbors (MPI_Comm comm, int rank, int maxneighbors,
				  int *neighbors)
{
	int start;
	int i;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_GRAPH)
    {
		sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

	start = 0;
	if (rank)
    {
		start = topo->data.graph.index[rank - 1];
    }

	for (i = 0; i < maxneighbors && i + start < topo->data.graph.nedges; i++)
    {
		neighbors[i] = topo->data.graph.edges[i + start];
    }

	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_shift (MPI_Comm comm, int direction, int displ,
			     int *source, int *dest)
{
  int *coords;
  int res;
  int rank;
  int maxdims;
  int new_val;
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  __INTERNAL__PMPI_Comm_rank (comm, &rank);


  if (topo->type != MPI_CART)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  if (direction >= topo->data.cart.ndims)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid direction");
    }

  maxdims = topo->data.cart.ndims;

  coords = sctk_malloc (maxdims * sizeof (int));

  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  coords[direction] = coords[direction] + displ;

  sctk_nodebug ("New val + %d", coords[direction]);
  if (coords[direction] < 0)
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] += topo->data.cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else if (coords[direction] >=
	   topo->data.cart.dims[direction])
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] -= topo->data.cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else
    {
      res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
    }


  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  *dest = new_val;

  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  coords[direction] = coords[direction] - displ;

  sctk_nodebug ("New val - %d", coords[direction]);
  if (coords[direction] < 0)
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] += topo->data.cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else if (coords[direction] >=
	   topo->data.cart.dims[direction])
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] -= topo->data.cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else
    {
      res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
    }


  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  *source = new_val;

  sctk_free (coords);

  sctk_nodebug ("dest %d src %d rank %d dir %d", *dest, *source, rank,
		direction);
  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_sub (MPI_Comm comm, int *remain_dims,
			   MPI_Comm * comm_new)
{
	int res;
	int nb_comm;
	int i;
	int *coords_in_new;
	int color;
	int rank;
	int size;
	int ndims = 0;
	int j;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	mpi_topology_per_comm_t* topo_new;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	sctk_spinlock_lock (&(topo->lock));


		if (topo->type != MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}

		coords_in_new = malloc (topo->data.cart.ndims * sizeof (int));

		res = __INTERNAL__PMPI_Cart_coords (comm, rank, topo->data.cart.ndims, coords_in_new, 1);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(topo->lock));
			return res;
		}
		
		
		int has_a_dim_left = 0;
		
		for (i = 0; i < topo->data.cart.ndims; i++)
		{
			if (remain_dims[i])
			{
				has_a_dim_left = 1;
				break;
			}
		}
		
		if( !has_a_dim_left )
		{
			if( rank == 0 )
			{
				MPI_Comm_dup(MPI_COMM_SELF, comm_new);
			}
			else
			{
				*comm_new = MPI_COMM_NULL;
			}
			
			sctk_spinlock_unlock (&(topo->lock));
			return MPI_SUCCESS;
		}
		

		nb_comm = 1;
		for (i = 0; i < topo->data.cart.ndims; i++)
		{
			sctk_nodebug ("Remain %d %d", i, remain_dims[i]);
			if (remain_dims[i] == 0)
			{
				nb_comm *= topo->data.cart.dims[i];
			}
			else
			{
				ndims++;
				coords_in_new[i] = 0;
			}
		}

		sctk_nodebug ("%d new comms", nb_comm);

		res = __INTERNAL__PMPI_Cart_rank (comm, coords_in_new, &color, 1);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(topo->lock));
			return res;
		}

		sctk_nodebug ("%d color", color);
		res = __INTERNAL__PMPI_Comm_split (comm, color, rank, comm_new);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(topo->lock));
			return res;
		}

		sctk_free (coords_in_new);

		tmp = mpc_mpc_get_per_comm_data(*comm_new);
		topo_new = &(tmp->topo);

		topo_new->type = MPI_CART;
		topo_new->data.cart.ndims = ndims;
		topo_new->data.cart.reorder = 0;
		topo_new->lock = SCTK_SPINLOCK_INITIALIZER;
		topo_new->data.cart.dims = sctk_malloc (ndims * sizeof (int));
		topo_new->data.cart.periods = sctk_malloc (ndims * sizeof (int));

		j = 0;
		for (i = 0; i < topo->data.cart.ndims; i++)
		{
			if (remain_dims[i])
			{
				topo_new->data.cart.dims[j] = topo->data.cart.dims[i];
				topo_new->data.cart.periods[j] = topo->data.cart.periods[i];
				j++;
			}
		}

		__INTERNAL__PMPI_Comm_size (*comm_new, &size);
		__INTERNAL__PMPI_Comm_rank (*comm_new, &rank);
		sctk_nodebug ("%d on %d new rank", rank, size);
	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_map (MPI_Comm comm, int ndims, int *dims,
			   int *periods, int *newrank)
{
  int res;
  int nnodes = 1;
  int i;

  for (i = 0; i < ndims; i++)
    {
      nnodes *= dims[i];
    }

TODO("Should be optimized")
  res = __INTERNAL__PMPI_Comm_rank (comm, newrank);

  if (*newrank >= nnodes)
    {
      *newrank = MPI_UNDEFINED;
    }

  return res;
}

static int
__INTERNAL__PMPI_Graph_map (MPI_Comm comm, int nnodes, int *index,
			    int *edges, int *newrank)
{
TODO("Should be optimized")
  __INTERNAL__PMPI_Comm_rank (comm, newrank);

  if (nnodes <= *newrank)
    {
      *newrank = MPI_UNDEFINED;
    }

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Get_processor_name (char *name, int *resultlen)
{
  return PMPC_Get_processor_name (name, resultlen);
}

static int
__INTERNAL__PMPI_Get_version (int *version, int *subversion)
{
	*version = MPI_VERSION;
	*subversion = MPI_SUBVERSION;
  	return MPI_SUCCESS;
}

void
MPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file,
		   int line)
{
/*   char str[1024]; */
/*   int i; */
/*   __INTERNAL__PMPI_Error_string (*error, str, &i); */
/*   if (i != 0) */
/*     sctk_error ("%s in file %s at line %d %s", str, file, line, msg); */
/*   else */
/*     sctk_error ("Unknown error"); */
  /* The lib is not supposed to crash but deliver message */
/*   __INTERNAL__PMPI_Abort (*comm, *error); */
}

void
MPI_Abort_error (MPI_Comm * comm, int *error, char *msg, char *file,
		   int line)
{
  char str[1024];
  int i;
  __INTERNAL__PMPI_Error_string (*error, str, &i);
  if (i != 0)
    sctk_error ("%s in file %s at line %d %s", str, file, line, msg);
  else
    sctk_error ("Unknown error");
  /* The lib is not supposed to crash but deliver message */
  __INTERNAL__PMPI_Abort (*comm, *error);
}

void
MPI_Return_error (MPI_Comm * comm, int *error, ...)
{
}

static int __INTERNAL__PMPI_Errhandler_create(MPI_Handler_function *function,
                                              MPI_Errhandler *errhandler) {
  sctk_errhandler_register(function, errhandler);
  MPI_ERROR_SUCESS();
}

static int
__INTERNAL__PMPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  sctk_handle_set_errhandler((sctk_handle)comm, SCTK_HANDLE_COMM,
                             (sctk_errhandler_t)errhandler);
  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler * errhandler)
{
  *errhandler = (MPI_Errhandler)sctk_handle_get_errhandler((sctk_handle)comm,
                                                           SCTK_HANDLE_COMM);

  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Errhandler_free (MPI_Errhandler * errhandler)
{
  TODO("Refcounting should be implemented for Error handlers");
  // sctk_errhandler_free((sctk_errhandler_t)*errhandler);
  *errhandler = MPI_ERRHANDLER_NULL;
  MPI_ERROR_SUCESS();
}

#define MPI_Error_string_convert(code, msg)                                    \
  case code:                                                                   \
    sprintf(string, msg);                                                      \
    break

static int
__INTERNAL__PMPI_Error_string (int errorcode, char *string, int *resultlen)
{
  size_t lngt;
  string[0] = '\0';
  switch (errorcode)
    {
			MPI_Error_string_convert (MPI_ERR_BUFFER , "Invalid buffer pointer");
			MPI_Error_string_convert (MPI_ERR_COUNT , "Invalid count argument");
			MPI_Error_string_convert (MPI_ERR_TYPE , "Invalid datatype argument");
			MPI_Error_string_convert (MPI_ERR_TAG , "Invalid tag argument");
			MPI_Error_string_convert (MPI_ERR_COMM , "Invalid communicator");
			MPI_Error_string_convert (MPI_ERR_RANK , "Invalid rank");
      MPI_Error_string_convert (MPI_ERR_REQUEST, "Invalid mpi_request handle");
			MPI_Error_string_convert (MPI_ERR_ROOT , "Invalid root");
			MPI_Error_string_convert (MPI_ERR_GROUP , "Invalid group ");
			MPI_Error_string_convert (MPI_ERR_OP , "Invalid operation");
			MPI_Error_string_convert (MPI_ERR_TOPOLOGY , "Invalid topology");
			MPI_Error_string_convert (MPI_ERR_DIMS , "Invalid dimension argument");
			MPI_Error_string_convert (MPI_ERR_ARG , "Invalid argument of some other kind");
			MPI_Error_string_convert (MPI_ERR_UNKNOWN , "Unknown error");
			MPI_Error_string_convert (MPI_ERR_TRUNCATE , "Message truncated on receive");
      MPI_Error_string_convert (MPI_ERR_OTHER, "Other error; use Error_string");
			MPI_Error_string_convert (MPI_ERR_INTERN , "Internal MPI (implementation) error");
			MPI_Error_string_convert (MPI_ERR_IN_STATUS , "Error code in status");
			MPI_Error_string_convert (MPI_ERR_PENDING , "Pending request");
			MPI_Error_string_convert (MPI_ERR_KEYVAL , "Invalid keyval has been passed");
			MPI_Error_string_convert (MPI_ERR_NO_MEM , "MPI_ALLOC_MEM failed because memory is exhausted");
			MPI_Error_string_convert (MPI_ERR_BASE , "Invalid base passed to MPI_FREE_MEM");
			MPI_Error_string_convert (MPI_ERR_INFO_KEY , "Key longer than MPI_MAX_INFO_KEY");
			MPI_Error_string_convert (MPI_ERR_INFO_VALUE , "Value longer than MPI_MAX_INFO_VAL");
			MPI_Error_string_convert (MPI_ERR_INFO_NOKEY , "Invalid key passed to MPI_INFO_DELETE");
			MPI_Error_string_convert (MPI_ERR_SPAWN , "Error in spawning processes");
			MPI_Error_string_convert (MPI_ERR_PORT , "Ivalid port name passed to MPI_COMM_CONNECT");
			MPI_Error_string_convert (MPI_ERR_SERVICE , "Invalid service name passed to MPI_UNPUBLISH_NAME");
			MPI_Error_string_convert (MPI_ERR_NAME , "Invalid service name passed to MPI_LOOKUP_NAME");
			MPI_Error_string_convert (MPI_ERR_WIN , "Invalid win argument");
			MPI_Error_string_convert (MPI_ERR_SIZE , "Invalid size argument");
			MPI_Error_string_convert (MPI_ERR_DISP , "Invalid disp argument");
			MPI_Error_string_convert (MPI_ERR_INFO , "Invalid info argument");
			MPI_Error_string_convert (MPI_ERR_LOCKTYPE , "Invalid locktype argument");
			MPI_Error_string_convert (MPI_ERR_ASSERT , "Invalid assert argument");
			MPI_Error_string_convert (MPI_ERR_RMA_CONFLICT , "Conflicting accesses to window");
			MPI_Error_string_convert (MPI_ERR_RMA_SYNC , "Wrong synchronization of RMA calls");
			MPI_Error_string_convert (MPI_ERR_FILE , "Invalid file handle");
			MPI_Error_string_convert (MPI_ERR_NOT_SAME , "Collective argument not identical on all processes, \
				or collective routines called in a different order by	different processes");
			MPI_Error_string_convert (MPI_ERR_AMODE , "Error related to the amode passed to MPI_FILE_OPEN");
			MPI_Error_string_convert (MPI_ERR_UNSUPPORTED_DATAREP , "Unsupported datarep passed to MPI_FILE_SET_VIEW");
			MPI_Error_string_convert (MPI_ERR_UNSUPPORTED_OPERATION , "Unsupported operation, \
				such as seeking on a file which	supports sequential access only");
			MPI_Error_string_convert (MPI_ERR_NO_SUCH_FILE , "File does not exist");
			MPI_Error_string_convert (MPI_ERR_FILE_EXISTS , "File exists");
			MPI_Error_string_convert (MPI_ERR_BAD_FILE , "Invalid file name (eg, path name too long");
			MPI_Error_string_convert (MPI_ERR_ACCESS , "Permission denied");
			MPI_Error_string_convert (MPI_ERR_NO_SPACE , "Not enough space");
			MPI_Error_string_convert (MPI_ERR_QUOTA , "Quota exceeded");
			MPI_Error_string_convert (MPI_ERR_READ_ONLY , "Read-only file or file system");
			MPI_Error_string_convert (MPI_ERR_FILE_IN_USE , "File operation could not be completed, \
				as the file is currently open by some process");
			MPI_Error_string_convert (MPI_ERR_DUP_DATAREP , "Conversion functions could not be registered because \
				a data representation identifier that was already defined	was passed to MPI_REGISTER_DATAREP");
			MPI_Error_string_convert (MPI_ERR_CONVERSION , "An error occurred in a user supplied data conversion function");
			MPI_Error_string_convert (MPI_ERR_IO , "Other I/O error");
			MPI_Error_string_convert (MPI_ERR_LASTCODE , "Last error code");
    default:
      sctk_warning("%d error code unknown : %s", errorcode,
                   string ? string : "");
    }

  lngt = strlen (string);
  *resultlen = (int) lngt;
  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Error_class (int errorcode, int *errorclass)
{
  *errorclass = errorcode;
  MPI_ERROR_SUCESS ();
}

static double
__INTERNAL__PMPI_Wtime (void)
{
  return PMPC_Wtime ();
}

static double
__INTERNAL__PMPI_Wtick (void)
{
  return PMPC_Wtick ();
}

static int
__INTERNAL__PMPI_Query_thread (int *provided)
{
  return PMPC_Query_thread(provided);
}

static int __INTERNAL__PMPI_Init_thread(int *argc, char ***argv, int required,
                                        int *provided) {
  int res;
  int max_thread_type = MPI_THREAD_MULTIPLE;
  res = __INTERNAL__PMPI_Init(argc, argv);
  if (required < max_thread_type) {
    *provided = required;
  } else {
    *provided = max_thread_type;
  }
  return res;
}




static int __INTERNAL__PMPI_Init(int *argc, char ***argv) {
  int res;
  int flag;
  int flag_finalized;
  __INTERNAL__PMPI_Initialized(&flag);
  __INTERNAL__PMPI_Finalized(&flag_finalized);
  if ((flag != 0) || (flag_finalized != 0)) {
    MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_OTHER, "MPI_Init issue");
  } else {
    int rank;

    sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
    struct sctk_task_specific_s *task_specific;
    mpc_per_communicator_t *per_communicator;
    mpc_per_communicator_t *per_communicator_tmp;

    sctk_init_yield_as_overloaded();

    res = PMPC_Init(argc, argv);
    if (res != MPI_SUCCESS) {
      return res;
    }
    is_initialized = 1;

    task_specific = __MPC_get_task_specific();
    task_specific->mpc_mpi_data = sctk_malloc(sizeof(struct mpc_mpi_data_s));
    memset(task_specific->mpc_mpi_data, 0, sizeof(struct mpc_mpi_data_s));
    task_specific->mpc_mpi_data->lock = SCTK_SPINLOCK_INITIALIZER;
    task_specific->mpc_mpi_data->requests = NULL;
    task_specific->mpc_mpi_data->groups = NULL;
    task_specific->mpc_mpi_data->buffers = NULL;
    task_specific->mpc_mpi_data->ops = NULL;
    if (sctk_runtime_config_get()
            ->modules.nbc.use_progress_thread == 1) {
      task_specific->mpc_mpi_data->NBC_Pthread_handles = NULL;
      sctk_thread_mutex_init(&(task_specific->mpc_mpi_data->list_handles_lock),
                             NULL);
      task_specific->mpc_mpi_data->nbc_initialized_per_task = 0;
      sctk_thread_mutex_init(
          &(task_specific->mpc_mpi_data->nbc_initializer_lock), NULL);
    }
    __sctk_init_mpc_request();
    __sctk_init_mpi_buffer();
    __sctk_init_mpi_errors();
    __sctk_init_mpi_topo();
    __sctk_init_mpi_op();
    __sctk_init_mpc_group();
    fortran_check_binds_resolve();
     __sctk_init_mpc_halo ();

    sctk_spinlock_lock(&(task_specific->per_communicator_lock));
    HASH_ITER(hh, task_specific->per_communicator, per_communicator,
              per_communicator_tmp) {
      mpc_mpi_per_communicator_t *tmp;
      per_communicator->mpc_mpi_per_communicator =
          sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s));
      memset(per_communicator->mpc_mpi_per_communicator, 0,
             sizeof(struct mpc_mpi_per_communicator_s));
      per_communicator->mpc_mpi_per_communicator_copy =
          mpc_mpi_per_communicator_copy_func;
      per_communicator->mpc_mpi_per_communicator_copy_dup =
          mpc_mpi_per_communicator_dup_copy_func;
      per_communicator->mpc_mpi_per_communicator->lock = lock;

      tmp = per_communicator->mpc_mpi_per_communicator;

      __sctk_init_mpi_topo_per_comm(tmp);
      tmp->max_number = 0;
      tmp->topo.lock = lock;
    }
    sctk_spinlock_unlock(&(task_specific->per_communicator_lock));

    mpc_MPI_allocmem_pool_init();

    MPI_APPNUM_VALUE = 0;
    MPI_LASTUSEDCODE_VALUE = MPI_ERR_LASTCODE;
    __INTERNAL__PMPI_Comm_size(MPI_COMM_WORLD, &MPI_UNIVERSE_SIZE_VALUE);
    __INTERNAL__PMPI_Comm_rank(MPI_COMM_WORLD, &rank);

    __INTERNAL__PMPI_Barrier(MPI_COMM_WORLD);
  }
  return res;
}

static int
__INTERNAL__PMPI_Finalize (void)
{
  int res; 

  struct sctk_task_specific_s * task_specific;
  task_specific = __MPC_get_task_specific ();
  if(task_specific->mpc_mpi_data->nbc_initialized_per_task == 1){
    task_specific->mpc_mpi_data->nbc_initialized_per_task = -1;
    sctk_thread_yield();
    NBC_Finalize(&(task_specific->mpc_mpi_data->NBC_Pthread));
  }

  mpc_MPI_allocmem_pool_release();

  /* Clear attributes on COMM_SELF */
  SCTK__MPI_Attr_clean_communicator (MPI_COMM_SELF);

  if(is_finalized != 0){
    MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "MPI_Finalize issue");
  }
  res = PMPC_Finalize ();
  is_finalized = 1;
  return res;
}

static int
__INTERNAL__PMPI_Finalized (int* flag)
{
  *flag = is_finalized;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Initialized (int *flag)
{
  return PMPC_Initialized (flag);
}

static int
__INTERNAL__PMPI_Abort (MPI_Comm comm, int val)
{
  return PMPC_Abort (comm, val);
}



/* Halo */

static int __INTERNAL__PMPIX_Halo_cell_init( MPI_Halo * halo, char * label, MPI_Datatype type, int count )
{
	int new_id = 0;
	struct sctk_mpi_halo_s * new_cell = sctk_mpi_halo_context_new( sctk_halo_context_get(),  &new_id );
	*halo = new_id;
	
	sctk_mpi_halo_init( new_cell , label , type, count );
	
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_cell_release( MPI_Halo * halo )
{
	struct sctk_mpi_halo_s * cell = sctk_mpi_halo_context_get( sctk_halo_context_get(),  *halo );
	
	if( !cell )
		return MPI_ERR_ARG;
	
	/* Remove cell from array */
	sctk_mpi_halo_context_set( sctk_halo_context_get(),  *halo, NULL );
	
	sctk_mpi_halo_release( cell );

	*halo = MPI_HALO_NULL;

	return MPI_SUCCESS;	
}


static int __INTERNAL__PMPIX__Halo_cell_set( MPI_Halo halo, void * ptr )
{
	struct sctk_mpi_halo_s * cell = sctk_mpi_halo_context_get( sctk_halo_context_get(),  halo );
	
	if( !cell )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_set( cell, ptr ) )
	{
		return MPI_ERR_INTERN;
	}
	
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_cell_get( MPI_Halo halo, void ** ptr )
{
	struct sctk_mpi_halo_s * cell = sctk_mpi_halo_context_get( sctk_halo_context_get(),  halo );
	
	if( !cell )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_get( cell, ptr ) )
	{
		return MPI_ERR_INTERN;
	}
	
	return MPI_SUCCESS;
}


/* Halo Exchange */

static int __INTERNAL__PMPIX__Halo_exchange_init( MPI_Halo_exchange * ex )
{
	int new_id = 0;
	struct sctk_mpi_halo_exchange_s * new_ex = sctk_mpi_halo_context_exchange_new( sctk_halo_context_get(),  &new_id );
	*ex = new_id;
	
	sctk_mpi_halo_exchange_init( new_ex );
	
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_exchange_release( MPI_Halo_exchange * ex )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  *ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	sctk_mpi_halo_context_exchange_set( sctk_halo_context_get(),  *ex, NULL );
	
	if( sctk_mpi_halo_exchange_release( exentry ) )
	{
		return MPI_ERR_INTERN;
	}
	
	*ex = MPI_HALO_NULL;
	
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_exchange_commit( MPI_Halo_exchange ex )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_exchange_commit( exentry ) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_exchange( MPI_Halo_exchange ex )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_exchange_blocking( exentry ) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_iexchange( MPI_Halo_exchange ex )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_iexchange( exentry ) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX_Halo_iexchange_wait( MPI_Halo_exchange ex )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_exchange_wait( exentry ) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}


static int __INTERNAL__PMPIX__Halo_cell_bind_local( MPI_Halo_exchange ex, MPI_Halo halo )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	struct sctk_mpi_halo_s * cell = sctk_mpi_halo_context_get( sctk_halo_context_get(),  halo );
	
	if( !cell )
		return MPI_ERR_ARG;
	
	if( sctk_mpi_halo_bind_local( exentry, cell) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

static int __INTERNAL__PMPIX__Halo_cell_bind_remote( MPI_Halo_exchange ex, MPI_Halo halo, int remote, char * remote_label )
{
	struct sctk_mpi_halo_exchange_s * exentry = sctk_mpi_halo_context_exchange_get( sctk_halo_context_get(),  ex );
		
	if( !exentry )
		return MPI_ERR_ARG;
	
	struct sctk_mpi_halo_s * cell = sctk_mpi_halo_context_get( sctk_halo_context_get(),  halo );
	
	if( !cell )
		return MPI_ERR_ARG;

	if( sctk_mpi_halo_bind_remote( exentry, cell, remote, remote_label) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

/* Checkpoint */
static int __INTERNAL__PMPIX_Checkpoint(MPIX_Checkpoint_state* st)
{
	return PMPC_Checkpoint(st);
}

/*
  PMPI FUNCTIONS
*/

/*
 * TODO: RULE FROM THE MPI STANDARD
 * In order to fully support the MPI standard, we cannot fail if the returned value of
 * an MPI function is different than MPI_SUCCESS.
 * We could add an additional MPC mode in order to fail in the case of a wrong returned value.
 */
#define SCTK_MPI_CHECK_RETURN_VAL(res,comm)do{if(res == MPI_SUCCESS){return res;} else {MPI_ERROR_REPORT(comm,res,"Generic error retrun");}}while(0)
//~ #define SCTK_MPI_CHECK_RETURN_VAL(res, comm) return res

static inline void
SCTK__MPI_INIT_STATUS (MPI_Status * status)
{
}

inline void
SCTK__MPI_INIT_REQUEST (MPI_Request * request)
{
  *request = MPI_REQUEST_NULL;
}

/** \brief Swap an ALLOCATED segment in place using zero-copy if possible
 *  \param sendrecv_buf Adress of the pointer to the buffer used to send and receive data
 *  \param remote_rank Rank which is part of the exchange
 *  \param size Total size of the message in bytes
 *  \param comm Target communicator
 */
int PMPIX_Swap (void **sendrecv_buf , int remote_rank, MPI_Count size , MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	if(remote_rank == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		SCTK_MPI_CHECK_RETURN_VAL (res, comm);
	}
	
	mpi_check_comm (comm, comm);

	res = __INTERNAL__PMPIX_Swap(sendrecv_buf , remote_rank, size , comm);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/** \brief Swap between two ALLOCATED segment in place using zero-copy if possible
 *  \param send_buf Adress of the pointer to the buffer used to send data
 *  \param recvbuff Adress of the pointer to the buffer used to receive data
 *  \param remote_rank Rank which is part of the exchange
 *  \param size Total size of the message in bytes
 *  \param comm Target communicator
 */
int PMPIX_Exchange(void **send_buf , void **recvbuff, int remote_rank, MPI_Count size , MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	if(remote_rank == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		SCTK_MPI_CHECK_RETURN_VAL (res, comm);
	}
	
  mpi_check_comm (comm, comm);
	
	res = __INTERNAL__PMPIX_Exchange(send_buf, recvbuff , remote_rank, size , comm);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

static inline int PMPI_Send_p(void *buf, int count, MPI_Datatype datatype,
                              int dest, int tag, MPI_Comm comm) {
  int res = MPI_ERR_INTERN;
  sctk_debug ("SEND buf %p type %d tag %d dest %d count %d", buf, datatype,tag,dest,count);
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
    {
	    mpi_check_buf (buf, comm);
    }
  }
  res = __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) {
  int res;
  SCTK_PROFIL_START(MPI_Send);
  res = PMPI_Send_p(buf, count, datatype, dest, tag, comm);
  SCTK_PROFIL_END(MPI_Send);
  return res;
}

static inline int PMPI_Recv_p(void *buf, int count, MPI_Datatype datatype,
                              int source, int tag, MPI_Comm comm,
                              MPI_Status *status) {
  sctk_nodebug("MPI_Recv count %d, datatype %d, source %d, tag %d, comm %d",
               count, datatype, source, tag, comm);
  int res = MPI_ERR_INTERN;
  if (source == MPC_PROC_NULL) {
    res = MPI_SUCCESS;
    MPI_Status empty_status;
    empty_status.MPC_SOURCE = MPI_PROC_NULL;
    empty_status.MPC_TAG = MPI_ANY_TAG;
    empty_status.MPC_ERROR = MPI_SUCCESS;
    empty_status.cancelled = 0;
    empty_status.size = 0;

    if (status != MPI_STATUS_IGNORE)
      *status = empty_status;

    SCTK_MPI_CHECK_RETURN_VAL(res, comm);
  }
  SCTK__MPI_INIT_STATUS(status);
  {
    int size;
    mpi_check_comm(comm, comm);
    mpi_check_type(datatype, comm);
    mpi_check_count(count, comm);
    sctk_nodebug("tag %d", tag);
    mpi_check_tag(tag, comm);
    __INTERNAL__PMPI_Comm_size(comm, &size);
    if (sctk_is_inter_comm(comm) == 0) {
      mpi_check_rank(source, size, comm);
    }
    if (count != 0) {
      mpi_check_buf(buf, comm);
    }
  }

  res = __INTERNAL__PMPI_Recv(buf, count, datatype, source, tag, comm, status);

  if (status != MPI_STATUS_IGNORE) {
    if (status->MPI_ERROR != MPI_SUCCESS) {
      res = status->MPI_ERROR;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  int res;
  SCTK_PROFIL_START(MPI_Recv);
  res = PMPI_Recv_p(buf, count, datatype, source, tag, comm, status);
  SCTK_PROFIL_END(MPI_Recv);
  return res;
}
int PMPI_Get_count (MPI_Status * status, MPI_Datatype datatype, int *count)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type (datatype, MPI_COMM_WORLD);

	if (status == NULL)
	{
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_IN_STATUS, "");
	}

	if (count == NULL)
	{
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COUNT, "");
	}

	res = __INTERNAL__PMPI_Get_count (status, datatype, count);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Bsend (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Ssend (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Rsend (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Buffer_attach (void *buffer, int size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Buffer_attach (buffer, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Buffer_detach (void *buffer, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Buffer_detach (buffer, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm, MPI_Request * request)
{
	sctk_nodebug("Isend");
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  mpi_check_comm (comm, comm);
  mpi_check_type (datatype, comm);
  res =
    __INTERNAL__PMPI_Isend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Ibsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	  res = MPI_SUCCESS;
	  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Ibsend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	  res = MPI_SUCCESS;
	  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Issend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Irsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Irsend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source,
	    int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(source == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Irecv (buf, count, datatype, source, tag, comm, request);

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Wait (MPI_Request * request, MPI_Status * status)
{
	sctk_nodebug("entering MPI_Wait request = %d", *request);
	MPI_Comm comm = MPI_COMM_WORLD;
	
	int res = MPI_ERR_INTERN;

	if(*request == MPI_REQUEST_NULL)
	{
		res = MPI_SUCCESS;
    if(status)
    {
      status->MPC_SOURCE = MPI_PROC_NULL;
      status->MPC_TAG = MPI_ANY_TAG;
      status->MPC_ERROR = MPI_SUCCESS;
      status->size = 0;
      SCTK_MPI_CHECK_RETURN_VAL (res, comm);
    }
	}

	res = __INTERNAL__PMPI_Wait (request, status);
	
	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = status->MPI_ERROR;
		}
	}

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Test (MPI_Request * request, int *flag, MPI_Status * status)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	if(*request == MPI_REQUEST_NULL)
	{
		res = MPI_SUCCESS;
                if( status )
                {
		    status->MPC_SOURCE = MPI_PROC_NULL;
		    status->MPC_TAG = MPI_ANY_TAG;
		    status->MPC_ERROR = MPI_SUCCESS;
		    status->size = 0;
                }
		*flag = 1;
		return res;
	}
	
	res = __INTERNAL__PMPI_Test (request, flag, status);
	
	if(status != MPI_STATUS_IGNORE)
	{
		if((status->MPI_ERROR != MPI_SUCCESS) && (status->MPI_ERROR != MPI_ERR_PENDING))
		{
			res = status->MPI_ERROR;
		}
	}

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Request_free (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_SUCCESS;
  if (NULL == request) {
    sctk_nodebug("request NULL");
    res = MPI_ERR_REQUEST;
  } else if(MPI_REQUEST_NULL == *request) {
	sctk_nodebug("request MPI_REQUEST_NULL");
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);  
  } else
	res = __INTERNAL__PMPI_Request_free (request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Waitany (int count, MPI_Request array_of_requests[], int *index,
	      MPI_Status * status)
{
	sctk_nodebug("entering PMPI_Waitany");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Waitany (count, array_of_requests, index, status);
  
  if( status != MPI_STATUS_IGNORE ){
    if(status->MPI_ERROR != MPI_SUCCESS){
      res = status->MPI_ERROR;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Testany (int count, MPI_Request array_of_requests[], int *index,
	      int *flag, MPI_Status * status)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Testany (count, array_of_requests, index, flag, status);
 
  if( status != MPI_STATUS_IGNORE ){
    if((status->MPI_ERROR != MPI_SUCCESS) && (status->MPI_ERROR != MPI_ERR_PENDING)){
      res = status->MPI_ERROR;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Waitall (int count, MPI_Request array_of_requests[],
	      MPI_Status array_of_statuses[])
{
	sctk_nodebug("entering PMPI_Waitall");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;

  res =
    __INTERNAL__PMPI_Waitall (count, array_of_requests, array_of_statuses);

  if(array_of_statuses != MPI_STATUSES_IGNORE){
    int i;
    for(i = 0; i < count; i++){
      if(array_of_statuses[i].MPI_ERROR != MPI_SUCCESS){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Testall (int count, MPI_Request array_of_requests[], int *flag,
	      MPI_Status array_of_statuses[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Testall (count, array_of_requests, flag,
			      array_of_statuses);

  if(array_of_statuses != MPI_STATUSES_IGNORE){
    int i;
    for(i =0; i < count; i++){
      if((array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING)){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Waitsome (int incount, MPI_Request array_of_requests[],
	       int *outcount, int array_of_indices[],
	       MPI_Status array_of_statuses[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Waitsome (incount, array_of_requests, outcount,
			       array_of_indices, array_of_statuses);

  if((array_of_statuses != MPI_STATUSES_IGNORE) && (*outcount != MPI_UNDEFINED)){
    int i;
    for(i =0; i < *outcount; i++){
      if((array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING)){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Testsome (int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int index;
	int res = MPI_ERR_INTERN;
	if ((array_of_requests == NULL) && (incount != 0))
	{
		res = MPI_ERR_REQUEST;
	}
/* 	else */
/* 	{ */
/* 		for (index = 0; index < incount; ++index) */
/* 		{ */
/* 			if (array_of_requests[index] == -1) */
/* 			{ */
/* 				res = MPI_ERR_REQUEST; */
/* 				break; */
/* 			} */
/* 		} */
/* 	} */
	if (((outcount == NULL || array_of_indices == NULL) && incount > 0) || incount < 0)
	{
		return MPI_ERR_ARG;
	}
	res = __INTERNAL__PMPI_Testsome (incount, array_of_requests, outcount, array_of_indices, array_of_statuses);

	if((array_of_statuses != MPI_STATUSES_IGNORE) && (*outcount != MPI_UNDEFINED)){
    int i;
    for(i =0; i < *outcount; i++){
      if((array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING)){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
	     MPI_Status * status)
{
    int size;
  int res = MPI_ERR_INTERN;
	mpi_check_comm(comm, comm);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
  res = __INTERNAL__PMPI_Iprobe (source, tag, comm, flag, status);
/*   if((status != MPI_STATUS_IGNORE) && (*flag != 0)){ */
/*     if(status->MPI_ERROR != MPI_SUCCESS){ */
/*       res = MPI_ERR_IN_STATUS; */
/*     } */
/*   }  */

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Probe (int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int size;
  int res = MPI_ERR_INTERN;
	mpi_check_comm(comm, comm);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
  res = __INTERNAL__PMPI_Probe (source, tag, comm, status);
/*   if(status != MPI_STATUS_IGNORE){ */
/*     if(status->MPI_ERROR != MPI_SUCCESS){ */
/*       res = MPI_ERR_IN_STATUS; */
/*     } */
/*   }  */

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cancel (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(MPI_REQUEST_NULL == *request) {
        sctk_nodebug("request MPI_REQUEST_NULL");
        SCTK_MPI_CHECK_RETURN_VAL (MPI_ERR_REQUEST, comm);
  }
  res = __INTERNAL__PMPI_Cancel (request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Send_init (void *buf, int count, MPI_Datatype datatype, int dest,
		int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Send_init (buf, count, datatype, dest, tag, comm,
				request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Bsend_init (void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Bsend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Ssend_init (void *buf, int count, MPI_Datatype datatype, int dest,
		 int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Ssend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Rsend_init (void *buf, int count, MPI_Datatype datatype, int dest,
		 int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Rsend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source,
		int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Recv_init (buf, count, datatype, source, tag, comm,
				request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Start (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if ((request == NULL) || (*request == MPI_REQUEST_NULL)) {
    MPI_ERROR_REPORT(comm,MPI_ERR_REQUEST,"");
  }
  res = __INTERNAL__PMPI_Start (request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Startall (int count, MPI_Request array_of_requests[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  int i; 
  for(i = 0 ; i < count ; i++){
  if (array_of_requests[i] == MPI_REQUEST_NULL) {
	MPI_ERROR_REPORT(comm,MPI_ERR_REQUEST,"");    
  }
  }
  res = __INTERNAL__PMPI_Startall (count, array_of_requests);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Sendrecv (void *sendbuf, int sendcount, MPI_Datatype sendtype,
	       int dest, int sendtag,
	       void *recvbuf, int recvcount, MPI_Datatype recvtype,
	       int source, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  int size;

  mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);

  mpi_check_type (sendtype, comm);
  mpi_check_type (recvtype, comm);
  mpi_check_count (sendcount, comm);
  mpi_check_count (recvcount, comm);
  mpi_check_tag_send (sendtag, comm);
  mpi_check_tag (recvtag, comm);
  mpi_check_rank_send(dest,size,comm);
  mpi_check_rank(source,size,comm);

  if (sendcount != 0)
    {
      mpi_check_buf (sendbuf, comm);
    }
  if (recvcount != 0)
    {
      mpi_check_buf (recvbuf, comm);
    }
  res =
    __INTERNAL__PMPI_Sendrecv (sendbuf, sendcount, sendtype, dest, sendtag,
			       recvbuf, recvcount, recvtype, source, recvtag,
			       comm, status);
  if(status != MPI_STATUS_IGNORE){
    if(status->MPI_ERROR != MPI_SUCCESS){
      res = MPI_ERR_IN_STATUS;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Sendrecv_replace (void *buf, int count, MPI_Datatype datatype,
		       int dest, int sendtag, int source, int recvtag,
		       MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  int size;
  mpi_check_comm (comm, comm);
  mpi_check_type (datatype, comm);
  mpi_check_count (count, comm);
  mpi_check_tag_send (sendtag, comm);
  mpi_check_tag (recvtag, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  mpi_check_rank_send(dest,size,comm);
  mpi_check_rank(source,size,comm);
  if (count != 0)
    {
      mpi_check_buf (buf, comm);
    }
  res =
    __INTERNAL__PMPI_Sendrecv_replace (buf, count, datatype, dest, sendtag,
				       source, recvtag, comm, status);
  if(status != MPI_STATUS_IGNORE){
    if(status->MPI_ERROR != MPI_SUCCESS){
      res = MPI_ERR_IN_STATUS;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/
int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count )
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Status_set_elements(status, datatype, count );
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Status_set_elements_x(status, datatype, count );
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Status_set_cancelled (MPI_Status *status, int cancelled)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Status_set_cancelled (status, cancelled);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Request_get_status (MPI_Request request, int *flag, MPI_Status *status)
{
        MPC_Request *mpc_request =  __sctk_convert_mpc_request(&request,__sctk_internal_get_MPC_requests());
	
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Request_get_status(*mpc_request, flag, status);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Test_cancelled (MPI_Status * status, int *flag)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Test_cancelled (status, flag);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/************************************************************************/
/* MPI Datatype Interface                                               */
/************************************************************************/

int PMPI_Type_contiguous (int count, MPI_Datatype old_type, MPI_Datatype * new_type_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	mpi_check_count(count,comm);
	
	*new_type_p = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	res = __INTERNAL__PMPI_Type_contiguous (count, old_type, new_type_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_vector (int count, int blocklength, int stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	
	*newtype_p = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	if(blocklength < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}
	
	mpi_check_count(count,comm);
	
	res = __INTERNAL__PMPI_Type_vector (count, blocklength, stride, old_type, newtype_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_hvector (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	mpi_check_count(count,comm);
	
	*newtype_p = MPC_DATATYPE_NULL;

	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	
	if(blocklen < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}
	
	res = __INTERNAL__PMPI_Type_hvector (count, blocklen, stride, old_type, newtype_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_hvector (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	mpi_check_count(count,comm);
	
	*newtype_p = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	if(blocklen < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}
	
	res = __INTERNAL__PMPI_Type_hvector (count, blocklen, stride, old_type, newtype_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_indexed (int count, int blocklens[], int indices[], MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	if((blocklens == NULL) || (indices == NULL))
	{
		return MPI_SUCCESS;
	}
	
	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}

	mpi_check_type_create(old_type,comm);
	res =  __INTERNAL__PMPI_Type_indexed (count, blocklens, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_hindexed (int count, int blocklens[],  MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	
	if((blocklens == NULL) || (indices == NULL))
	{
		return MPI_SUCCESS;
	}
	
	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}
	
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}


int PMPI_Type_create_indexed_block(int count, int blocklength, int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );

	
	if( blocklength < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}

	
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_create_indexed_block(count, blocklength, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);	
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	
	*newtype = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	if( blocklength < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}

	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_create_hindexed_block(count, blocklength, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);	
}

int PMPI_Type_create_hindexed (int count, int blocklens[], MPI_Aint indices[],  MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);

	*newtype = MPC_DATATYPE_NULL;
	
	mpi_check_type( old_type, MPI_COMM_WORLD );
	
	if((blocklens == NULL) || (indices == NULL))
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_struct (int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;

	
	if((old_types == NULL) || (indices == NULL) || (blocklens == NULL))
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{

		mpi_check_type( old_types[i], MPI_COMM_WORLD );

		mpi_check_type_create(old_types[i],comm); 

		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}

	res = __INTERNAL__PMPI_Type_struct (count, blocklens, indices, old_types, newtype);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_struct (int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	
	mpi_check_count(count,comm);
	
	
	*newtype = MPC_DATATYPE_NULL;

	for(i = 0; i < count; i++)
	{
		mpi_check_type( old_types[i], MPI_COMM_WORLD );
		mpi_check_type_create(old_types[i],comm); 
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Bad datatype provided to PMPI_Type_create_struct");
		} 
	}
	
	

	res = __INTERNAL__PMPI_Type_struct (count, blocklens, indices, old_types, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Address (void *location, MPI_Aint * address)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Address (location, address);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Get_address( void *location, MPI_Aint *address)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Address (location, address);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
int PMPI_Type_extent (MPI_Datatype datatype, MPI_Aint * extent)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_extent (datatype, extent);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	
	__INTERNAL__PMPI_Type_lb (datatype, lb);
	res = __INTERNAL__PMPI_Type_extent (datatype, extent);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent)
{
	return PMPI_Type_get_extent( datatype, lb, extent );
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent)
{
	return PMPC_Type_get_true_extent( datatype, true_lb, true_extent );
}

int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent)
{
	return PMPC_Type_get_true_extent( datatype, true_lb, true_extent );
}

int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner)
{
	return PMPC_Type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner);
}

int PMPI_Type_get_contents( MPI_Datatype datatype, 
		       	    int max_integers,
			    int max_addresses,
			    int max_datatypes,
			    int array_of_integers[],
			    MPI_Aint array_of_addresses[],
			    MPI_Datatype array_of_datatypes[])
{
	return PMPC_Type_get_contents(datatype, max_integers,max_addresses,max_datatypes,array_of_integers,array_of_addresses,array_of_datatypes);
}

  /* See the 1.1 version of the Standard.  The standard made an
     unfortunate choice here, however, it is the standard.  The size returned
     by MPI_Type_size is specified as an int, not an MPI_Aint */
int PMPI_Type_size (MPI_Datatype datatype, int *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_size (datatype, size);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_size_x(datatype, size);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

  /* MPI_Type_count was withdrawn in MPI 1.1 */
int PMPI_Type_lb (MPI_Datatype datatype, MPI_Aint * displacement)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_lb (datatype, displacement);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_ub (MPI_Datatype datatype, MPI_Aint * displacement)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_ub (datatype, displacement);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(old_type,comm);
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_create_resized( old_type, lb, extent, new_type );
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_commit (MPI_Datatype * datatype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(*datatype,comm);
	mpi_check_type_create(*datatype,comm);
	res = __INTERNAL__PMPI_Type_commit (datatype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_free (MPI_Datatype * datatype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	
	mpi_check_type (*datatype, MPI_COMM_WORLD);

	if( sctk_datatype_is_common(*datatype) )
	{
		MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "");
	}
	
	res = __INTERNAL__PMPI_Type_free (datatype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_dup( MPI_Datatype old_type, MPI_Datatype *newtype )
{
	return PMPC_Type_dup(old_type, newtype);
}

int PMPI_Get_elements (MPI_Status * status, MPI_Datatype datatype, int *elements)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);
	
	MPI_Count tmp_elements = 0;
	
	res = __INTERNAL__PMPI_Get_elements_x (status, datatype, &tmp_elements);
	
	*elements = tmp_elements;
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Get_elements_x (MPI_Status * status, MPI_Datatype datatype, MPI_Count *elements)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);

	res = __INTERNAL__PMPI_Get_elements_x (status, datatype, elements);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_darray (int size,
			     int rank,
			     int ndims,
			     int array_of_gsizes[],
			     int array_of_distribs[],
			     int array_of_dargs[],
			     int array_of_psizes[],
			     int order,
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(oldtype,comm);
	
	int csize;
	__INTERNAL__PMPI_Comm_size (comm, &csize);
	mpi_check_rank(rank, size, comm);
	
	
	res = __INTERNAL__PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs,
						  array_of_dargs, array_of_psizes, order, oldtype, newtype);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_subarray (int ndims,
			       int array_of_sizes[],
			       int array_of_subsizes[],
			       int array_of_starts[],
			       int order,
			       MPI_Datatype oldtype,
			       MPI_Datatype * new_type)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(oldtype,comm);
	
	res = __INTERNAL__PMPI_Type_create_subarray ( ndims, array_of_sizes, array_of_subsizes,
						      array_of_starts, order, oldtype, new_type);
	
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *rtype)
{
	/* This structure directly comes from the standard */
	switch(typeclass) 
	{
		case MPI_TYPECLASS_REAL: 
			switch(size)
			{
				case 4: *rtype = MPI_REAL4; return MPI_SUCCESS;
				case 8: *rtype = MPI_REAL8; return MPI_SUCCESS;
				case 16: *rtype = MPI_REAL16; return MPI_SUCCESS;
				default: MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad size provided to MPI_Type_match_size (MPI_TYPECLASS_REAL)");
			}
		case MPI_TYPECLASS_INTEGER: 
			switch(size)
			{
				case 1: *rtype = MPI_INTEGER1; return MPI_SUCCESS;
				case 2: *rtype = MPI_INTEGER2; return MPI_SUCCESS;
				case 4: *rtype = MPI_INTEGER4; return MPI_SUCCESS;
				case 8: *rtype = MPI_INTEGER8; return MPI_SUCCESS;
				case 16:  *rtype = MPI_INTEGER16; return MPI_SUCCESS;
				default: MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad size provided to MPI_Type_match_size (MPI_TYPECLASS_INTEGER)");
			}
		case MPI_TYPECLASS_COMPLEX: 
			switch(size)
			{
				case 8: *rtype = MPI_COMPLEX8; return MPI_SUCCESS;
				case 16: *rtype = MPI_COMPLEX16; return MPI_SUCCESS;
				case 32: *rtype = MPI_COMPLEX32; return MPI_SUCCESS;
				default: MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad size provided to MPI_Type_match_size (MPI_TYPECLASS_COMPLEX)");
			}
		default:
			MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad Type class provided to MPI_Type_match_size");
	}

	return MPI_SUCCESS;
}

int PMPI_Pack(void *inbuf,
	   int incount,
	   MPI_Datatype datatype,
	   void *outbuf, int outcount, int *position, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	mpi_check_comm(comm,comm);
	mpi_check_type(datatype,comm);
        if ((NULL == outbuf) || (NULL == position)) {
          MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "wrong outbuf or position");
        } else if (incount < 0) {
          MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "wrong incount");
        } else if (outcount < 0) {
          MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "wrong outcount");
        } else if (incount == 0) {
          return MPI_SUCCESS;
        }

        if (datatype < SCTK_USER_DATA_TYPES_MAX) {
          if (outcount < incount) {
            MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
          }
        }
        res = __INTERNAL__PMPI_Pack(inbuf, incount, datatype, outbuf, outcount,
                                    position, comm);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int PMPI_Unpack (void *inbuf,
	     int insize,
	     int *position,
	     void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	mpi_check_comm(comm,comm);
	mpi_check_type(datatype,comm);
        if ((NULL == inbuf) || (NULL == position)) {
          MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "wrong outbuff or position");
        } else if ((outcount < 0) || (insize < 0)) {
          MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "wrong outcount insize");
        } else if (datatype < 0) {
          MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "wrong datatype");
        }
        res = __INTERNAL__PMPI_Unpack(inbuf, insize, position, outbuf, outcount,
                                      datatype, comm);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int PMPI_Pack_size (int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm(comm,comm);
  mpi_check_type(datatype,comm);
  mpi_check_count(incount,comm);
  res = __INTERNAL__PMPI_Pack_size (incount, datatype, comm, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/* DataType Keyval Management */

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                            MPI_Type_delete_attr_function *type_delete_attr_fn,
                            int *type_keyval, void *extra_state) {
  return PMPC_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn,
                                 type_keyval, extra_state);
}

int PMPI_Type_free_keyval(int *type_keyval) {
  return PMPC_Type_free_keyval(type_keyval);
}

int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval,
                       void *attribute_val) {
  return PMPC_Type_set_attr(datatype, type_keyval, attribute_val);
}

int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval,
                       void *attribute_val, int *flag) {
  return PMPC_Type_get_attr(datatype, type_keyval, attribute_val, flag);
}

int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval) {
  return PMPC_Type_delete_attr(datatype, type_keyval);
}

/************************************************************************/
/* MPI Pack external Support                                            */
/************************************************************************/

int PMPI_Pack_external_size (char *datarep , int incount, MPI_Datatype datatype, MPI_Aint *size)
{
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Pack_external_size(datarep, incount, datatype, size );
	
	SCTK_MPI_CHECK_RETURN_VAL (res, MPI_COMM_SELF);
}

int PMPI_Pack_external (char *datarep , void *inbuf, int incount, MPI_Datatype datatype, void * outbuf, MPI_Aint outsize, MPI_Aint * position)
{
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Pack_external (datarep , inbuf, incount, datatype, outbuf, outsize,  position);
	SCTK_MPI_CHECK_RETURN_VAL (res, MPI_COMM_SELF);
}

int PMPI_Unpack_external (char * datarep, void * inbuf, MPI_Aint insize, MPI_Aint * position, void * outbuf, int outcount, MPI_Datatype datatype)
{
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Unpack_external (datarep, inbuf, insize, position, outbuf, outcount, datatype);
	SCTK_MPI_CHECK_RETURN_VAL (res, MPI_COMM_SELF);
}




int PMPI_Type_set_name( MPI_Datatype datatype, char *name )
{
	return PMPC_Type_set_name(datatype, name);
}

int PMPI_Type_get_name( MPI_Datatype datatype, char *name, int * resultlen )
{
	return PMPC_Type_get_name(datatype, name, resultlen);
}


int PMPI_Barrier (MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Barrier);

        /* Error checking */
        mpi_check_comm(comm, comm);

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Barrier(comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Barrier(comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Barrier);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int PMPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Bcast);
	
	/* Error checking */
    mpi_check_comm(comm, comm);
        
	if (MPI_IN_PLACE == buffer) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}
      
	__INTERNAL__PMPI_Comm_size (comm, &size);
	mpi_check_root(root,size,comm);
	mpi_check_rank_send(root,size,comm);
	mpi_check_buf (buffer, comm);
	mpi_check_count (count, comm);
	mpi_check_type (datatype, comm);

	/* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res =
              __INTERNAL__PMPI_Bcast(buffer, count, datatype, root, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res =
              __INTERNAL__PMPI_Bcast(buffer, count, datatype, root, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Bcast);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	     void *recvbuf, int recvcnt, MPI_Datatype recvtype,
	     int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */

	SCTK_PROFIL_START (MPI_Gather);
	
	mpi_check_comm (comm, comm);
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}

  if((sendcnt == 0 || recvcnt == 0) && !sctk_is_inter_comm(comm)) {
     return MPI_SUCCESS;
   } 

  if(sendbuf == recvbuf && rank == root) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }


	/* Error checking */
	mpi_check_root(root,size,comm);
    if(rank != root)
    {
		mpi_check_buf (sendbuf, comm);
		mpi_check_count (sendcnt, comm);
		mpi_check_type (sendtype, comm);
    } 
    else 
    {
		if(MPI_IN_PLACE != sendbuf)
		{
			mpi_check_buf (sendbuf, comm);
			mpi_check_count (sendcnt, comm);
			mpi_check_type (sendtype, comm);
		}
		mpi_check_buf (recvbuf, comm);
		mpi_check_count (recvcnt, comm);
		mpi_check_type (recvtype, comm);
    }
    
	if ((0 == sendcnt && MPI_ROOT != root && 
			(rank != root ||
				(rank == root && MPI_IN_PLACE != sendbuf)
			)
		) ||
		(rank == root && MPI_IN_PLACE == sendbuf && 0 == recvcnt) || 
			(0 == recvcnt && 
				(MPI_ROOT == root || MPI_PROC_NULL == root)
			)
		) 
	{
		return MPI_SUCCESS;
	}
	
		
	/* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf,
                                        recvcnt, recvtype, root, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {

          res = __INTERNAL__PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf,
                                        recvcnt, recvtype, root, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Gather);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	      void *recvbuf, int *recvcnts, int *displs,
	      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Gatherv);
	
	mpi_check_comm (comm, comm);
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	
	/* Error checking */
	mpi_check_root(root,size,comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcnt, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_type (recvtype, comm);

  if(sendbuf == recvbuf && rank == root) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
	if ((rank != root && MPI_IN_PLACE == sendbuf) ||
		(rank == root && MPI_IN_PLACE == recvbuf)) 
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res =
              __INTERNAL__PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf,
                                       recvcnts, displs, recvtype, root, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res =
              __INTERNAL__PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf,
                                       recvcnts, displs, recvtype, root, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Gatherv);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	      void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
	      MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Scatter);
	
	mpi_check_comm (comm, comm);
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	
	/* Error checking */
	mpi_check_root(root,size,comm);
	if (rank == root)
	{
		mpi_check_buf (sendbuf, comm);
		mpi_check_count (sendcnt, comm);
		mpi_check_type (sendtype, comm);
		if(recvbuf == sendbuf){
		  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
		}
	}
	else
	{
		mpi_check_buf (recvbuf, comm);
		mpi_check_count (recvcnt, comm);
		mpi_check_type (recvtype, comm);
	}

	if ((rank != root && MPI_IN_PLACE == recvbuf) ||
		(rank == root && MPI_IN_PLACE == sendbuf)) 
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}

	if ((0 == recvcnt && MPI_ROOT != root &&
			(rank != root ||
				(rank == root && MPI_IN_PLACE != recvbuf)
			)
		) ||
		(rank == root && MPI_IN_PLACE == recvbuf && 0 == sendcnt) ||
			(0 == sendcnt && 
				(MPI_ROOT == root || MPI_PROC_NULL == root)
			)
		) 
	{
		return MPI_SUCCESS;
	}

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf,
                                         recvcnt, recvtype, root, comm);
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
        } else {
          res = __INTERNAL__PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf,
                                         recvcnt, recvtype, root, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Scatter);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Scatterv (void *sendbuf, int *sendcnts, int *displs,
	       MPI_Datatype sendtype, void *recvbuf, int recvcnt,
	       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rsize, rank, i;
	MPI_Aint extent;
	
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Scatterv);
	
	res = __INTERNAL__PMPI_Type_extent(sendtype, &extent);
	if(res != MPI_SUCCESS){return res;}
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_remote_size (comm, &rsize);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
	 
  if(sendbuf == recvbuf && rank == root) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
	if(sctk_is_inter_comm(comm))
	{
		mpi_check_root(root,rsize,comm);
		if(root == MPI_ROOT)
		{
			mpi_check_type (sendtype, comm);
			for(i=0; i<size; i++)
			{
				mpi_check_buf ((char *)(sendbuf)+displs[i]*extent, comm);
				mpi_check_count(sendcnts[i], comm);
			}
		}
		else if (root != MPI_PROC_NULL)
		{
			mpi_check_buf (recvbuf, comm);
			mpi_check_count (recvcnt, comm);
			mpi_check_type (recvtype, comm);
		}
	}
	else
	{
		mpi_check_root(root,size,comm);
		if(rank == root)
		{
			mpi_check_type (sendtype, comm);
			for (i = 0; i < size; i++) 
			{
				mpi_check_buf ((char *)(sendbuf)+displs[i]*extent, comm);
				mpi_check_count(sendcnts[i], comm);
			}
			if(recvbuf != MPI_IN_PLACE)
			{
				mpi_check_buf (recvbuf, comm);
				mpi_check_count (recvcnt, comm);
				mpi_check_type (recvtype, comm);
			}
		}
		else
		{
			mpi_check_buf (recvbuf, comm);
			mpi_check_count (recvcnt, comm);
			mpi_check_type (recvtype, comm);
		}
	}
	
	if ((rank != root && MPI_IN_PLACE == recvbuf) ||
		(rank == root && MPI_IN_PLACE == sendbuf)) 
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res =
              __INTERNAL__PMPI_Scatterv(sendbuf, sendcnts, displs, sendtype,
                                        recvbuf, recvcnt, recvtype, root, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res =
              __INTERNAL__PMPI_Scatterv(sendbuf, sendcnts, displs, sendtype,
                                        recvbuf, recvcnt, recvtype, root, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Scatterv);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Allgather (void *sendbuf, int sendcount, MPI_Datatype sendtype,
		void *recvbuf, int recvcount, MPI_Datatype recvtype,
		MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	/* Profiling */
	SCTK_PROFIL_START (MPI_Allgather);
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	if(sendbuf != MPI_IN_PLACE)
	{
		mpi_check_buf (sendbuf, comm);
		mpi_check_count (sendcount, comm);
		mpi_check_type (sendtype, comm);
	}
  if(sendbuf == recvbuf) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcount, comm);
	mpi_check_type (recvtype, comm);
	if(sctk_is_inter_comm (comm))
	{
		if ( 0 == sendcount && 0 == recvcount ) 
		{
			return MPI_SUCCESS;
		}
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
		else
			MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
#endif
	}
	else
	{
		if ((MPI_IN_PLACE != sendbuf && 0 == sendcount) || (0 == recvcount)) 
		{
			return MPI_SUCCESS;
		}
	}

	if (MPI_IN_PLACE == recvbuf) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Allgather);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Allgatherv (void *sendbuf, int sendcount, MPI_Datatype sendtype,
		 void *recvbuf, int *recvcounts, int *displs,
		 MPI_Datatype recvtype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Allgatherv);
	
	mpi_check_comm (comm, comm);
    if(sendbuf != MPI_IN_PLACE)
    {
		mpi_check_buf (sendbuf, comm);
    	mpi_check_count (sendcount, comm);
    	mpi_check_type (sendtype, comm);
    }
  if(sendbuf == recvbuf) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
	mpi_check_buf (recvbuf, comm);
	mpi_check_type (recvtype, comm);

	if (MPI_IN_PLACE == recvbuf) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res =
              __INTERNAL__PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcounts, displs, recvtype, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res =
              __INTERNAL__PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcounts, displs, recvtype, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Allgatherv);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Alltoall (void *sendbuf, int sendcount, MPI_Datatype sendtype,
	       void *recvbuf, int recvcount, MPI_Datatype recvtype,
	       MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Alltoall);
	
	/* Error checking */
	if (sendcount == 0 && recvcount == 0) {
		return MPI_SUCCESS;
    }
	mpi_check_comm (comm, comm);
	if(sendbuf != MPI_IN_PLACE)
	{
		mpi_check_buf (sendbuf, comm);
		mpi_check_count (sendcount, comm);
		mpi_check_type (sendtype, comm);
	}
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcount, comm);
	mpi_check_type (recvtype, comm);
    
    if (MPI_IN_PLACE == sendbuf) {
		sendcount = recvcount;
		sendtype = recvtype;
	}
        
	if (recvbuf == MPI_IN_PLACE  || sendbuf == recvbuf) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}
    /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Alltoall);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Alltoallv (void *sendbuf, int *sendcnts, int *sdispls,
		MPI_Datatype sendtype, void *recvbuf, int *recvcnts,
		int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size;
	int i; 
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Alltoallv);
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}

  if (MPI_IN_PLACE == sendbuf) 
	{
		sendcnts = recvcnts;
		sdispls = rdispls;
		sendtype = recvtype;
	}

	for(i = 0; i < size; i++)
	{
		mpi_check_count (sendcnts[i], comm);
		mpi_check_count (recvcnts[i], comm);
	}
	mpi_check_buf (sendbuf, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
  mpi_check_type (recvtype, comm);

  if (MPI_IN_PLACE == recvbuf || recvbuf == sendbuf)
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");

	/* Internal */
    if (sctk_new_scheduler_engine_enabled) {
      sctk_thread_generic_scheduler_t *sched;
      sched = &(sctk_thread_generic_self()->sched);
      sched->th->attr.basic_priority +=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sched->th->attr.current_priority +=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
      sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
      res = __INTERNAL__PMPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype,
                                       recvbuf, recvcnts, rdispls, recvtype,
                                       comm);
      sched->th->attr.basic_priority -=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sched->th->attr.current_priority -=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sctk_thread_generic_setkind_mask_self(kind_mask_save);
    } else {
      res = __INTERNAL__PMPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype,
                                       recvbuf, recvcnts, rdispls, recvtype,
                                       comm);
    }

    /* Profiling */
    SCTK_PROFIL_END(MPI_Alltoallv);
    SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int PMPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes,
				   void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rsize, rank;
	int i; 
	MPI_Aint sextent, rextent;
	
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Alltoallw);
	
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_remote_size (comm, &rsize);
	if(res != MPI_SUCCESS){return res;}
	res = __INTERNAL__PMPI_Comm_rank (comm, &rank);
	if(res != MPI_SUCCESS){return res;}
  if(sendbuf == recvbuf) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
	/* Error checking */
	mpi_check_comm (comm, comm);
	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	
	for ( i = 0; i < size; i++ ) 
	{
		if (sendbuf != MPI_IN_PLACE) 
		{
			mpi_check_type (sendtypes[i], comm);
			res = __INTERNAL__PMPI_Type_extent(sendtypes[i], &sextent);
			if(res != MPI_SUCCESS){return res;}
			mpi_check_buf((char *)(sendbuf)+sdispls[i]*sextent, comm);
			mpi_check_count (sendcnts[i], comm);
		}
		mpi_check_type (recvtypes[i], comm);
		res = __INTERNAL__PMPI_Type_extent(recvtypes[i], &rextent);
		if(res != MPI_SUCCESS){return res;}
		mpi_check_buf((char *)(recvbuf)+rdispls[i]*rextent, comm);
		mpi_check_count (recvcnts[i], comm);
	}
	
	if (MPI_IN_PLACE == sendbuf) 
	{
		sendcnts = recvcnts;
		sdispls    = rdispls;
		sendtypes  = recvtypes;
	}
	
	if ((NULL == sendcnts) || (NULL == sdispls) || (NULL == sendtypes) ||
        (NULL == recvcnts) || (NULL == rdispls) || (NULL == recvtypes) ||
        recvbuf == MPI_IN_PLACE) 
    {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
    }
		
		
	/* Internal */
    if (sctk_new_scheduler_engine_enabled) {
      sctk_thread_generic_scheduler_t *sched;
      sched = &(sctk_thread_generic_self()->sched);
      sched->th->attr.basic_priority +=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sched->th->attr.current_priority +=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
      sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
      res = __INTERNAL__PMPI_Alltoallw(sendbuf, sendcnts, sdispls, sendtypes,
                                       recvbuf, recvcnts, rdispls, recvtypes,
                                       comm);
      sched->th->attr.basic_priority -=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sched->th->attr.current_priority -=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sctk_thread_generic_setkind_mask_self(kind_mask_save);
    } else {
      res = __INTERNAL__PMPI_Alltoallw(sendbuf, sendcnts, sdispls, sendtypes,
                                       recvbuf, recvcnts, rdispls, recvtypes,
                                       comm);
    }

    /* Profiling */
    SCTK_PROFIL_END(MPI_Alltoallw);
    SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

/* Neighbors collectives */
int PMPI_Neighbor_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int rank;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (recvtype == MPI_DATATYPE_NULL) {	
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
	} else if (recvcount < 0) {
		MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcount must be superior or equal to zero");
	} 
    
    if(sctk_is_inter_comm (comm)){
		if (sendcount == 0 && recvcount == 0)
			return MPI_SUCCESS;
	} else {
		if (recvcount == 0)
			return MPI_SUCCESS;
	}
	
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart");
			res = __INTERNAL__PMPI_Neighbor_allgather_cart(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_graph");
			res = __INTERNAL__PMPI_Neighbor_allgather_graph(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	__INTERNAL__PMPI_Comm_size (comm, &size);
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (!(topo->type == MPI_CART || topo->type == MPI_GRAPH)) {
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"Topology must be MPI_CART or MPI_GRAPH");
	} else if (recvtype == MPI_DATATYPE_NULL) {	
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
	}
	
	for (i = 0; i < size; ++i) {
	  if (recvcounts[i] < 0) {
		MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcounts must be superior or equal to zero");
	  }
	}

	if (NULL == displs) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_BUFFER,"displs must be valid");
	}
       
    if ( !sctk_is_inter_comm (comm) ) {
        for (i = 0; i < size; ++i) {
            if (recvcounts[i] != 0) {
                break;
            }
        }
        if (i >= size) {
            return MPI_SUCCESS;
        }
    }
    
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart");
			res = __INTERNAL__PMPI_Neighbor_allgatherv_cart(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_graph");
			res = __INTERNAL__PMPI_Neighbor_allgatherv_graph(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int rank;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (!(topo->type == MPI_CART || topo->type == MPI_GRAPH)) {
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"Topology must be MPI_CART or MPI_GRAPH");
	} else if (recvtype == MPI_DATATYPE_NULL) {	
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
	} else if (recvcount < 0) {
		MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcount must be superior or equal to zero");
	} 
	
	if (sendcount == 0 && recvcount == 0) {
        return MPI_SUCCESS;
    }
    
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoall_cart(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoall_graph(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_alltoallv(void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (!(topo->type == MPI_CART || topo->type == MPI_GRAPH)) {
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"Topology must be MPI_CART or MPI_GRAPH");
	} else if ((NULL == sendcounts) || (NULL == sdispls) || (NULL == recvcounts) || (NULL == rdispls)) {
		MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "Invalid arg");
	}
    
    __INTERNAL__PMPI_Comm_rank (comm, &size);
    for (i = 0; i < size; i++) 
    {
		if (recvcounts[i] < 0) {
			MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcounts must be superior or equal to zero");
		} else if (MPI_DATATYPE_NULL == recvtype) {
			MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
		}
	}
           
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallv_cart(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallv_graph(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_alltoallw(void *sendbuf, int sendcounts[], MPI_Aint sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if ((NULL == sendcounts) || (NULL == sdispls) || (NULL == sendtypes) || 
	    (NULL == recvcounts) || (NULL == rdispls) || (NULL == recvtypes)) {
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "Invalid arg");
    }
    
    __INTERNAL__PMPI_Comm_rank (comm, &size);
    for (i = 0; i < size; i++) 
    {
		if (recvcounts[i] < 0) {
			MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcounts must be superior or equal to zero");
		} else if (MPI_DATATYPE_NULL == recvtypes[i] || recvtypes == NULL) {
			MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtypes must be valid types");
		}
	}
	
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallw_cart(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallw_graph(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}


int PMPI_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Reduce);

        /* Error checking */
        mpi_check_comm(comm, comm);
        res = __INTERNAL__PMPI_Comm_size(comm, &size);
        if (res != MPI_SUCCESS) {
          return res;
        }
        res = __INTERNAL__PMPI_Comm_rank(comm, &rank);
        if (res != MPI_SUCCESS) {
          return res;
        }

        mpi_check_root(root, size, comm);
        mpi_check_count(count, comm);
        mpi_check_type(datatype, comm);
        mpi_check_op(op, datatype, comm);
        if (sctk_is_inter_comm(comm)) {
          if (root == MPI_ROOT) {
            mpi_check_buf(recvbuf, comm);
          } else if (root != MPI_PROC_NULL) {
            mpi_check_buf(sendbuf, comm);
          }
        } else {
          if (rank == root) {
            if (sendbuf == MPI_IN_PLACE) {
              mpi_check_buf(recvbuf, comm);
            } else {
              mpi_check_buf(sendbuf, comm);
            }
            mpi_check_buf(recvbuf, comm);
          } else {
            mpi_check_buf(sendbuf, comm);
          }
        }

        if (rank == root &&
             ((MPI_IN_PLACE == recvbuf) || (sendbuf == recvbuf))) {
          MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
        }

        if (0 == count) {
          return MPI_SUCCESS;
        }

        /* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);

          res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype,
                                             op, root, comm);

          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {

          res = __INTERNAL__PMPI_Reduce(sendbuf, recvbuf, count, datatype,
                                             op, root, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Reduce);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Op_create (MPI_User_function * function, int commute, MPI_Op * op)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Op_create (function, commute, op);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Op_free (MPI_Op * op)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((*op < 0) || (*op < MAX_MPI_DEFINED_OP)){
    MPI_ERROR_REPORT(comm,MPI_ERR_OP,"");
  }
  res = __INTERNAL__PMPI_Op_free (op);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Op_commutative(MPI_Op op, int * commute) { not_implemented(); return MPI_ERR_INTERN;}

int
PMPI_Allreduce (void *sendbuf, void *recvbuf, int count,
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Allreduce);
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (count, comm);
	mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);
	
	if( MPI_IN_PLACE == recvbuf ) 
    {
	    MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "");
    }
    if (0 == count) {
        return MPI_SUCCESS;
    }
    
    if(recvbuf != MPI_BOTTOM){
      if(recvbuf == sendbuf){
	MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
      }
    }
    
    /* Internal */
    if (sctk_new_scheduler_engine_enabled) {
      sctk_thread_generic_scheduler_t *sched;
      sched = &(sctk_thread_generic_self()->sched);
      sched->th->attr.basic_priority +=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sched->th->attr.current_priority +=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
      sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
      res = __INTERNAL__PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op,
                                       comm);
      sched->th->attr.basic_priority -=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sched->th->attr.current_priority -=
          sctk_runtime_config_get()->modules.scheduler.progress_basic_priority;
      sctk_thread_generic_setkind_mask_self(kind_mask_save);
    } else {
      res = __INTERNAL__PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op,
                                       comm);
    }

    /* Profiling */
    SCTK_PROFIL_END(MPI_Allreduce);
    SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts,
		     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int i;
	int size; 
	int res = MPI_ERR_INTERN;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Reduce_scatter);
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	if (MPI_IN_PLACE == recvbuf || sendbuf == recvbuf) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}
	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);

	res = __INTERNAL__PMPI_Comm_size (comm, &size);
	if(res != MPI_SUCCESS){return res;}
	
	for(i = 0; i < size; i++){
		mpi_check_count (recvcnts[i], comm);
	}
	mpi_check_type (datatype, comm);
	mpi_check_op (op,datatype, comm);
	
	/* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Reduce_scatter(sendbuf, recvbuf, recvcnts,
                                                datatype, op, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Reduce_scatter(sendbuf, recvbuf, recvcnts,
                                                datatype, op, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Reduce_scatter);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Reduce_scatter_block (void *sendbuf, void *recvbuf, int recvcnt,
		     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int i;
  int size; 
  int res = MPI_ERR_INTERN;
  int * recvcnts;
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START (MPI_Reduce_scatter_block);
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
  
  __INTERNAL__PMPI_Comm_size (comm, &size);

	if (MPI_IN_PLACE == recvbuf || sendbuf == recvbuf) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}

	mpi_check_count (recvcnts, comm);
	mpi_check_type (datatype, comm);
	mpi_check_op (op,datatype, comm);

	/* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcnt,
                                                      datatype, op, comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcnt,
                                                      datatype, op, comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Reduce_scatter_block);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Scan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	   MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	
	/* Error checking */
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (count, comm);
	mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);

	/* Invalid operation for intercommunicators */
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
	/* Profiling */
	SCTK_PROFIL_START (MPI_Scan);
	
 if(sendbuf == recvbuf) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

	
	/* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Scan(sendbuf, recvbuf, count, datatype, op,
                                      comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Scan(sendbuf, recvbuf, count, datatype, op,
                                      comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Scan);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Exscan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	   MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	
	if(sctk_is_inter_comm (comm)){
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
	/* Profiling */
	SCTK_PROFIL_START (MPI_Exscan);
	 
  if(sendbuf == recvbuf) {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

	/* Error checking */
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (count, comm);
	mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);
	
	/* Internal */
        if (sctk_new_scheduler_engine_enabled) {
          sctk_thread_generic_scheduler_t *sched;
          sched = &(sctk_thread_generic_self()->sched);
          sched->th->attr.basic_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority +=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          unsigned int kind_mask_save = sctk_thread_generic_getkind_mask_self();
          sctk_thread_generic_addkind_mask_self(KIND_MASK_PROGRESS_THREAD);
          res = __INTERNAL__PMPI_Exscan(sendbuf, recvbuf, count, datatype, op,
                                        comm);
          sched->th->attr.basic_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sched->th->attr.current_priority -=
              sctk_runtime_config_get()
                  ->modules.scheduler.progress_basic_priority;
          sctk_thread_generic_setkind_mask_self(kind_mask_save);
        } else {
          res = __INTERNAL__PMPI_Exscan(sendbuf, recvbuf, count, datatype, op,
                                        comm);
        }

        /* Profiling */
        SCTK_PROFIL_END(MPI_Exscan);
        SCTK_MPI_CHECK_RETURN_VAL(res, comm);
}

int
PMPI_Group_size (MPI_Group group, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_size (group, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_rank (MPI_Group group, int *rank)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_rank (group, rank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1,
			    MPI_Group group2, int *ranks2)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(n < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Group_translate_ranks (group1, n, ranks1, group2, ranks2);
  sctk_nodebug("RES = %d", res);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_compare (MPI_Group group1, MPI_Group group2, int *result)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_compare (group1, group2, result);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_group (MPI_Comm comm, MPI_Group * group)
{
	sctk_nodebug("Enter Comm_group");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm); 
  res = __INTERNAL__PMPI_Comm_group (comm, group);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_union (MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_union (group1, group2, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_intersection (MPI_Group group1, MPI_Group group2,
			 MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_intersection (group1, group2, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_difference (MPI_Group group1, MPI_Group group2,
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
	MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }
  res = __INTERNAL__PMPI_Group_difference (group1, group2, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_incl (MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  int i; 
  int size; 
  __INTERNAL__PMPI_Comm_size (comm, &size);
  for(i = 0; i < n ; i++){
    if((ranks[i] < 0) || (ranks[i] >= size)) MPI_ERROR_REPORT (comm, MPI_ERR_RANK, "");
  }
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_incl (group, n, ranks, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_excl (MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_excl (group, n, ranks, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_range_incl (MPI_Group group, int n, int ranges[][3],
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  if(n < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Group_range_incl (group, n, ranges, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_range_excl (MPI_Group group, int n, int ranges[][3],
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }
  if(n < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Group_range_excl (group, n, ranges, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_free (MPI_Group * group)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(*group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_free (group);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_size (MPI_Comm comm, int *size)
{
	sctk_nodebug("Enter Comm_size comm %d", comm);
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_size (comm, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}



int
PMPI_Comm_rank (MPI_Comm comm, int *rank)
{
	sctk_nodebug("Enter Comm_rank comm %d", comm);
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_rank (comm, rank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result)
{
	sctk_nodebug("Enter Comm_compare");
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;

	if(comm1 < 0 || comm2 < 0)
	{
		if(comm1 != comm2)
			*result = MPI_UNEQUAL;
		else
			*result = MPI_IDENT;
		return MPI_ERR_ARG;
	}

	mpi_check_comm (comm1, comm1);
	mpi_check_comm (comm2, comm2);
	res = __INTERNAL__PMPI_Comm_compare (comm1, comm2, result);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_dup (MPI_Comm comm, MPI_Comm * newcomm)
{
	sctk_nodebug("Enter Comm_dup");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Comm_dup (comm, newcomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_create (MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
	sctk_nodebug("Enter Comm_create");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  if (group == MPI_GROUP_NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
    }
  res = __INTERNAL__PMPI_Comm_create (comm, group, newcomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Intercomm_create (MPI_Comm local_comm, int local_leader,
		       MPI_Comm peer_comm, int remote_leader, int tag,
		       MPI_Comm * newintercomm)
{
	sctk_nodebug("Enter Intercomm_create");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  int size;

  mpi_check_comm(local_comm,local_comm);
  mpi_check_comm(peer_comm,peer_comm);
  
    __INTERNAL__PMPI_Comm_size (local_comm, &size);
    if(sctk_is_inter_comm(local_comm) == 0){
      mpi_check_rank_send (local_leader, size, comm);
    }
    __INTERNAL__PMPI_Comm_size (peer_comm, &size);
    if(sctk_is_inter_comm(peer_comm) == 0){
      mpi_check_rank_send (remote_leader, size, comm);
    }

  if (newintercomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Intercomm_create (local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_split (MPI_Comm comm, int color, int key, MPI_Comm * newcomm)
{
	sctk_nodebug("Enter Comm_split");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Comm_split (comm, color, key, newcomm);
  sctk_nodebug ("SPLIT Com %d Color %d, key %d newcomm %d", comm, color, key, *newcomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info,
                         MPI_Comm *newcomm) {
  int color = 0;

  if (split_type == MPI_COMM_TYPE_SHARED) {
    color = sctk_get_node_rank();
    /* char hname[200];
    gethostname(hname, 200);
    sctk_error("Color %d on %s", color, hname); */
  }

  TODO("Handle info in Comm_split_type");
  return PMPI_Comm_split(comm, color, key, newcomm);
}

int
PMPI_Comm_free (MPI_Comm * comm)
{
	sctk_nodebug("Enter Comm_free");
  int res = MPI_ERR_INTERN;
  if (comm == NULL)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COMM, "");
    }
 
  if ((*comm == MPI_COMM_WORLD) || (*comm == MPI_COMM_SELF))
    {
       MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COMM, "");
    }

  mpi_check_comm (*comm, *comm);
  res = __INTERNAL__PMPI_Comm_free (comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, *comm);
}

int
PMPI_Comm_test_inter (MPI_Comm comm, int *flag)
{
	sctk_nodebug("Enter Comm_test_inter");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_test_inter (comm, flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_remote_size (MPI_Comm comm, int *size)
{
	sctk_nodebug("Enter Comm_remote_size comm %d", comm);
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if(sctk_is_inter_comm (comm) == 0){
    MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
  }
  res = __INTERNAL__PMPI_Comm_remote_size (comm, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_remote_group (MPI_Comm comm, MPI_Group * group)
{
	sctk_nodebug("Enter Comm_remote_group");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if(sctk_is_inter_comm (comm) == 0){
    MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
  }

  res = __INTERNAL__PMPI_Comm_remote_group (comm, group);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Intercomm_merge (MPI_Comm intercomm, int high, MPI_Comm * newintracomm)
{
	sctk_nodebug("Enter Intercomm_merge");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
   mpi_check_comm (intercomm,comm);

  if(sctk_is_inter_comm (intercomm) == 0){
    MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
  }

  res = __INTERNAL__PMPI_Intercomm_merge (intercomm, high, newintracomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}



int
PMPI_Keyval_create (MPI_Copy_function * copy_fn,
		    MPI_Delete_function * delete_fn,
		    int *keyval, void *extra_state)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Keyval_create (copy_fn, delete_fn, keyval, extra_state);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Keyval_free (int *keyval)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;

  if(*keyval == MPI_KEYVAL_INVALID){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Keyval_free (keyval);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_put (MPI_Comm comm, int keyval, void *attr_value)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_put (comm, keyval, attr_value);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_get (MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_get (comm, keyval, attr_value, flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_get_fortran (MPI_Comm comm, int keyval, int *attr_value, int *flag)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_get_fortran (comm, keyval, attr_value, flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_delete (MPI_Comm comm, int keyval)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_delete (comm, keyval);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Topo_test (MPI_Comm comm, int *topo_type)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Topo_test (comm, topo_type);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims, int *periods,
		  int reorder, MPI_Comm * comm_cart)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  {
  int i;
  int size;
  int sum = 1;
  
  if (comm_old == MPI_COMM_NULL)
      MPI_ERROR_REPORT (comm_old, MPI_ERR_COMM, "");

  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  
  if(ndims <= 0 || dims == NULL)
  {
    MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
  }
  
  if( size <= ndims )
  {
	  *comm_cart = MPI_COMM_NULL;
	  MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
  }
  
  else if (ndims >= 1 && 
	  (periods == NULL || comm_cart == NULL))
  {
	MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }

  for(i = 0; i < ndims; i++)
  {
          if(dims[i] < 0){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
          if(dims[i] > size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
        sum *= dims[i];
  }
  if(sum > size)
  {
        MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }

  }

  res =
    __INTERNAL__PMPI_Cart_create (comm_old, ndims, dims, periods, reorder,
				  comm_cart);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Dims_create (int nnodes, int ndims, int *dims)
{
	sctk_nodebug("Enter PMPI_Dims_create");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;

  if(nnodes <= 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Dims_create (nnodes, ndims, dims);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_create (MPI_Comm comm_old, int nnodes, int *index, int *edges,
		   int reorder, MPI_Comm * comm_graph)
{
	sctk_nodebug("Enter PMPI_Graph_create");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  {
  int i;
  int size;
  int nb_edge = 0;
  int first_edge = 0;
  __INTERNAL__PMPI_Comm_size (comm_old, &size); 

  if((nnodes < 0) || (nnodes > size)){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }
  
  if( nnodes == 0 )
  {
	*comm_graph = MPI_COMM_NULL;  
	return MPI_SUCCESS;
  }
  
  
  nb_edge = index[nnodes -1];
  for(i = 0; i < nb_edge; i++){
	  if(edges[i] < 0){
        	MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
	  }
          if(edges[i] >= size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
  }
  for (i = 0; i < nnodes; i++){
    int j;
    first_edge = index[i];
  }
  }
  res =
    __INTERNAL__PMPI_Graph_create (comm_old, nnodes, index, edges, reorder,
				   comm_graph);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Graphdims_get (MPI_Comm comm, int *nnodes, int *nedges)
{
	sctk_nodebug("Enter PMPI_Graphdims_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graphdims_get (comm, nnodes, nedges);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_get (MPI_Comm comm, int maxindex, int maxedges,
		int *index, int *edges)
{
	sctk_nodebug("Enter PMPI_Graph_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graph_get (comm, maxindex, maxedges, index, edges);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cartdim_get (MPI_Comm comm, int *ndims)
{
	sctk_nodebug("Enter PMPI_Cartdim_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cartdim_get (comm, ndims);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods,
	       int *coords)
{
	sctk_nodebug("Enter PMPI_Cart_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);  {
  int i;
  int size;
  __INTERNAL__PMPI_Comm_size (comm, &size);
  if((maxdims <= 0) || (maxdims > size)){
        MPI_ERROR_REPORT (comm, MPI_ERR_DIMS, "");
  }
  }

  res = __INTERNAL__PMPI_Cart_get (comm, maxdims, dims, periods, coords);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_rank (MPI_Comm comm, int *coords, int *rank)
{
	sctk_nodebug("Enter PMPI_Cart_rank");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_rank (comm, coords, rank, 0);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_coords (MPI_Comm comm, int rank, int maxdims, int *coords)
{
	sctk_nodebug("Enter PMPI_Cart_coords");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 0);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_neighbors_count (MPI_Comm comm, int rank, int *nneighbors)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graph_neighbors_count (comm, rank, nneighbors);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_neighbors (MPI_Comm comm, int rank, int maxneighbors,
		      int *neighbors)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res =
    __INTERNAL__PMPI_Graph_neighbors (comm, rank, maxneighbors, neighbors);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_shift (MPI_Comm comm, int direction, int displ, int *source,
		 int *dest)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if(direction < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_DIMS, "");
  }

  if (source == NULL || dest == NULL)
  {
	  MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Cart_shift (comm, direction, displ, source, dest);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_sub (MPI_Comm comm, int *remain_dims, MPI_Comm * comm_new)
{
	sctk_nodebug("Enter PMPI_Cart_sub");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_sub (comm, remain_dims, comm_new);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_map (MPI_Comm comm_old, int ndims, int *dims, int *periods,
	       int *newrank)
{
	sctk_nodebug("Enter PMPI_Cart_map");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  {
  int i;
  int size;
  int sum = 1;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  if((ndims <= 0) || (ndims > size)){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
  }
  for(i = 0; i < ndims; i++){
          if(dims[i] <= 0){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
          if(dims[i] >= size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
	sum *= dims[i];
  }
  if(sum > size){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
  }

  }

  res = __INTERNAL__PMPI_Cart_map (comm_old, ndims, dims, periods, newrank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Graph_map (MPI_Comm comm_old, int nnodes, int *index, int *edges,
		int *newrank)
{
	sctk_nodebug("Enter PMPI_Graph_map");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  if(nnodes <= 0){
	MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }  
{
  int i;
  int size;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  if((nnodes < 0) || (nnodes > size)){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }
  for(i = 0; i < nnodes; i++){
          if(edges[i] < 0){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
          if(edges[i] >= size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
  }
  }

  res = __INTERNAL__PMPI_Graph_map (comm_old, nnodes, index, edges, newrank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Get_processor_name (char *name, int *resultlen)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_processor_name (name, resultlen);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Get_version (int *version, int *subversion)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_version (version, subversion);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_create (MPI_Handler_function * function,
			MPI_Errhandler * errhandler)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Errhandler_create (function, errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  }

  if (errhandler == MPI_ERRHANDLER_NULL) {
    return MPI_ERR_ARG;
  }

  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Errhandler_set (comm, errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler * errhandler)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Errhandler_get (comm, errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_free (MPI_Errhandler * errhandler)
{
  if (!errhandler) {
    return MPI_ERR_ARG;
  }

  if (*errhandler == MPI_ERRHANDLER_NULL) {
    return MPI_ERR_ARG;
  }

  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Errhandler_free (errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Error_string (int errorcode, char *string, int *resultlen)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Error_string (errorcode, string, resultlen);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Error_class (int errorcode, int *errorclass)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Error_class (errorcode, errorclass);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

double
PMPI_Wtime (void)
{
  double res = 0;
  res = __INTERNAL__PMPI_Wtime ();
  return res;
}

double
PMPI_Wtick (void)
{
  double res = 0;
  res = __INTERNAL__PMPI_Wtick ();
  return res;
}

int
PMPI_Query_thread (int *provided)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Query_thread (provided);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}


int
PMPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Init_thread (argc, argv, required, provided);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Init (int *argc, char ***argv)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Init (argc, argv);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Finalize (void)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Finalize ();
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Finalized (int *flag)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Finalized (flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Initialized (int *flag)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Initialized (flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Abort (MPI_Comm comm, int errorcode)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);

  res = __INTERNAL__PMPI_Abort (comm, errorcode);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}


int PMPI_Is_thread_main(int *flag)
{
	sctk_task_specific_t *task_specific;
	task_specific = __MPC_get_task_specific ();
	*flag = task_specific->init_done;
	return MPI_SUCCESS;
}
  /* Note that we may need to define a @PCONTROL_LIST@ depending on whether
     stdargs are supported */
int
PMPI_Pcontrol (const int level, ...)
{
  return MPI_SUCCESS;
}

/*************************************
*  MPI-3 : Matched Probe             *
**************************************/

/** This lock protects message handles attribution */
static sctk_spinlock_t __message_handle_lock = 0;

/** This is where message handle ids are generated */
static int __message_handle_id = 1;

/** This is a value telling if message handles have
    been initialized (HT in particular) */
static volatile int __message_handle_initialized = 0;

/** This is the HT where handle conversion is done */
struct MPCHT __message_handle_ht;

/** This is the message handle data-structure */
struct MPC_Message {
  int message_id;
  void *buff;
  size_t size;
  MPI_Comm comm;
  MPI_Status status;
  MPI_Request request;
};

/** This is how you init the handle table once */
static void MPC_Message_handle_init_once() {
  if (__message_handle_initialized)
    return;

  sctk_spinlock_lock_yield(&__message_handle_lock);
  if (__message_handle_initialized == 0) {
    __message_handle_initialized = 1;
    MPCHT_init(&__message_handle_ht, 64);
    sctk_m_probe_matching_init();
  }

  sctk_spinlock_unlock(&__message_handle_lock);
}

/** This is how you release the handle table once */
static void MPC_Message_handle_release_once() {

  sctk_spinlock_lock(&__message_handle_lock);

  if (__message_handle_initialized == 1) {
    __message_handle_initialized = 0;
    MPCHT_release(&__message_handle_ht);
  }

  sctk_spinlock_unlock(&__message_handle_lock);
}

/** This is how you create a new mesage */
struct MPC_Message *MPC_Message_new() {
  MPC_Message_handle_init_once();

  struct MPC_Message *new = sctk_malloc(sizeof(struct MPC_Message));

  assume(new != NULL);

  sctk_spinlock_lock(&__message_handle_lock);

  new->message_id = __message_handle_id++;
  new->request = MPI_REQUEST_NULL;
  new->buff = NULL;

  sctk_spinlock_unlock(&__message_handle_lock);

  MPCHT_set(&__message_handle_ht, new->message_id, (void *)new);

  return new;
}

/** This is how you free a received message */
void MPC_Message_free(struct MPC_Message *m) {
  MPC_Message_handle_init_once();
  sctk_free(m->buff);
  memset(m, 0, sizeof(struct MPC_Message));
  sctk_free(m);
}

/** This is how you resolve and delete a message */
struct MPC_Message *MPC_Message_retrieve(MPI_Message message) {

  struct MPC_Message *ret =
      (struct MPC_Message *)MPCHT_get(&__message_handle_ht, message);

  if (ret) {
    MPCHT_delete(&__message_handle_ht, message);
  }

  return ret;
}

/* probe and cancel */
int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message,
                MPI_Status *status) {
  MPI_Status st;

  if (status == MPI_STATUS_IGNORE) {
    status = &st;
  }

  if (source == MPI_PROC_NULL) {
    *message = MPI_MESSAGE_NO_PROC;
    status->MPI_SOURCE = MPI_PROC_NULL;
    status->MPI_TAG = MPI_ANY_TAG;
    return MPI_SUCCESS;
  }

  struct MPC_Message *m = MPC_Message_new();

  /* Do a probe & pick */

  /* Here we set the probing value */
  sctk_m_probe_matching_set(m->message_id);

  int ret = PMPI_Probe(source, tag, comm, status);

  if (ret != MPI_SUCCESS) {
    return ret;
  }

  /* Allocate Memory */
  int count = 0;
  MPI_Get_count(status, MPI_CHAR, &count);
  m->buff = sctk_malloc(count);
  assume(m->buff);
  m->size = count;

  m->comm = comm;

  /* Post Irecv */
  PMPI_Irecv(m->buff, count, MPI_CHAR, status->MPI_SOURCE, tag, comm,
             &m->request);

  /* Return the message ID */
  *((int *)message) = m->message_id;

  sctk_m_probe_matching_reset();

  return MPI_SUCCESS;
}

int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag,
                 MPI_Message *message, MPI_Status *status) {
  MPI_Status st;

  if (status == MPI_STATUS_IGNORE) {
    status = &st;
  }

  if (source == MPI_PROC_NULL) {
    *flag = 1;
    *message = MPI_MESSAGE_NO_PROC;
    status->MPI_SOURCE = MPI_PROC_NULL;
    status->MPI_TAG = MPI_ANY_TAG;
    return MPI_SUCCESS;
  }

  struct MPC_Message *m = MPC_Message_new();

  sctk_m_probe_matching_set(m->message_id);
  /* Do a probe & pick */
  int ret = PMPI_Iprobe(source, tag, comm, flag, status);

  if (ret != MPI_SUCCESS) {
    return ret;
  }

  if (*flag) {

    /* Allocate Memory */
    int count = 0;
    MPI_Get_count(status, MPI_CHAR, &count);
    m->buff = sctk_malloc(count);
    assume(m->buff);
    m->size = count;

    m->comm = comm;

    /* Post Irecv */
    PMPI_Irecv(m->buff, count, MPI_CHAR, status->MPI_SOURCE, tag, comm,
               &m->request);

    /* Return the message ID */
    *((int *)message) = m->message_id;
  } else {
    MPC_Message_free(m);
    *message = MPI_MESSAGE_NULL;
  }

  sctk_m_probe_matching_reset();

  return MPI_SUCCESS;
}

int PMPI_Mrecv(void *buf, int count, MPI_Datatype datatype,
               MPI_Message *message, MPI_Status *status) {
  MPI_Status st;

  if (status == MPI_STATUS_IGNORE) {
    status = &st;
  }

  if (*message == MPI_MESSAGE_NO_PROC) {
    *message = MPI_MESSAGE_NULL;
    status->MPI_SOURCE = MPI_PROC_NULL;
    status->MPI_TAG = MPI_ANY_TAG;
    return MPI_SUCCESS;
  }

  struct MPC_Message *m = MPC_Message_retrieve(*message);

  if (!m) {
    return MPI_ERR_ARG;
  }

  /* This message is about to be consumed */
  *message = MPI_MESSAGE_NULL;

  sctk_nodebug(">>WAITINT FOR MESSAGE");
  PMPI_Wait(&m->request, MPI_STATUS_IGNORE);

  int pos = 0;

  sctk_nodebug("<< DONE WAITINT FOR MESSAGE");

  sctk_nodebug("UNPACK to %p", buf);
  PMPI_Unpack(m->buff, m->size, &pos, buf, count, datatype, m->comm);

  memcpy(status, &m->status, sizeof(MPI_Status));

  MPC_Message_free(m);

  return MPI_SUCCESS;
}

struct MPC_Message_poll {
  struct MPC_Message *m;
  MPI_Request *req;
  void *buff;
  int count;
  MPI_Datatype datatype;
};

int MPI_Grequest_imrecv_query(void *extra_state, MPI_Status *status) {
  return 0;
}

int MPI_Grequest_imrecv_poll(void *extra_state, MPI_Status *status) {
  struct MPC_Message_poll *p = (struct MPC_Message_poll *)extra_state;

  assert(p != NULL);

  int flag = 0;
  PMPI_Test(&p->m->request, &flag, MPI_STATUS_IGNORE);

  if (flag == 1) {
    /* Message here then unpack */
    int pos = 0;
    PMPI_Unpack(p->m->buff, p->m->size, &pos, p->buff, p->count, p->datatype,
                p->m->comm);

    memcpy(status, &p->m->status, sizeof(MPI_Status));

    PMPI_Grequest_complete(*p->req);
  }

  return MPI_SUCCESS;
}

int MPI_Grequest_imrecv_cancel(void *extra_state, int complete) {
  return MPI_SUCCESS;
}

int MPI_Grequest_imrecv_free(void *extra_state) {
  struct MPC_Message_poll *p = (struct MPC_Message_poll *)extra_state;

  MPC_Message_free(p->m);
  sctk_free(p);

  return MPI_SUCCESS;
}

int PMPI_Imrecv(void *buf, int count, MPI_Datatype datatype,
                MPI_Message *message, MPI_Request *request) {

  if (*message == MPI_MESSAGE_NO_PROC) {
    *message = MPI_MESSAGE_NULL;
    return MPI_SUCCESS;
  }

  struct MPC_Message *m = MPC_Message_retrieve(*message);

  if (!m) {
    return MPI_ERR_ARG;
  }

  /* This message is about to be consumed */
  *message = MPI_MESSAGE_NULL;

  /* Prepare a polling structure for generalized requests */
  struct MPC_Message_poll *poll = sctk_malloc(sizeof(struct MPC_Message_poll));
  assume(poll != NULL);

  poll->req = request;
  poll->m = m;
  poll->buff = buf;
  poll->count = count;
  poll->datatype = datatype;

  /* Start a generalized request to do the work */
  PMPIX_Grequest_start(MPI_Grequest_imrecv_query, MPI_Grequest_imrecv_free,
                       MPI_Grequest_imrecv_cancel, MPI_Grequest_imrecv_poll,
                       (void *)poll, request);

  return MPI_SUCCESS;
}

/*************************************
*  Communicator Naming                *
**************************************/

int
PMPI_Comm_get_name (MPI_Comm comm, char *comm_name, int *resultlen)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_get_name (comm, comm_name, resultlen);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_set_name (MPI_Comm comm, char *comm_name)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_set_name (comm, comm_name);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/*************************************
*  NULL Delete handlers              *
**************************************/

int MPC_Mpi_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* type */
int MPC_Mpi_type_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_type_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_type_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_type_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_type_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_type_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* comm */
int MPC_Mpi_comm_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_comm_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_comm_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_comm_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_comm_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_comm_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* win */
int MPC_Mpi_win_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_win_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_win_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_win_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_win_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_win_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/*************************************
*  MPI-2 : Fortran handle conversion *
**************************************/

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm)
{
	return (MPI_Comm)comm;
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm)
{
	return (MPI_Fint)comm;
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype)
{
	return (MPI_Datatype)datatype;
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype)
{
	return (MPI_Fint)datatype;
}

MPI_Group PMPI_Group_f2c(MPI_Fint group)
{
	return (MPI_Group)group;
}

MPI_Fint PMPI_Group_c2f(MPI_Group group)
{
	return (MPI_Fint)group;
}

MPI_Request PMPI_Request_f2c(MPI_Fint request)
{
	return (MPI_Request)request;
}

MPI_Fint PMPI_Request_c2f(MPI_Request request)
{
	return (MPI_Fint)request;
}


MPI_Win PMPI_Win_f2c(MPI_Fint win)
{
	return (MPI_Win)win;
}

MPI_Fint PMPI_Win_c2f(MPI_Win win)
{
	return (MPI_Fint)win;
}

MPI_Op PMPI_Op_f2c(MPI_Fint op)
{
	return (MPI_Op)op;
}

MPI_Fint PMPI_Op_c2f(MPI_Op op)
{
	return (MPI_Fint)op;
}

MPI_Info PMPI_Info_f2c(MPI_Fint info)
{
	return (MPI_Info)info;
}

MPI_Fint PMPI_Info_c2f(MPI_Info info)
{
	return (MPI_Fint)info;
}

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler)
{
	return (MPI_Errhandler)errhandler;
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
	return (MPI_Fint)errhandler;
}

/*********************************** 
*  Dummy One-Sided Communicationst *
************************************/

int PMPI_Free_mem (void *ptr)
{
  mpc_MPI_allocmem_pool_free(ptr);

  return MPI_SUCCESS;
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
  *((void **)baseptr) = mpc_MPI_allocmem_pool_alloc(size);

  if (*((void **)baseptr) == NULL) {
    return MPI_ERR_INTERN;
  }

  return MPI_SUCCESS;
}



/*********************** 
*  MPI Info Management *
***********************/


int PMPI_Info_set( MPI_Info info, const char *key, const char *value )
{
	return PMPC_Info_set( (MPC_Info)info, key, value );
}

int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag)
{
	return PMPC_Info_get( (MPC_Info)info, key, valuelen, value, flag );
}

int PMPI_Info_free( MPI_Info *info )
{
	return PMPC_Info_free( (MPC_Info *)info );
}

int PMPI_Info_dup( MPI_Info info, MPI_Info *newinfo )
{
	return PMPC_Info_dup( (MPC_Info) info , (MPC_Info * ) newinfo );
}

int PMPI_Info_delete( MPI_Info info, const char *key )
{
	return PMPC_Info_delete( (MPC_Info) info , key );
}

int PMPI_Info_create( MPI_Info *info )
{
	return PMPC_Info_create( (MPC_Info *) info );
}

int PMPI_Info_get_nkeys( MPI_Info info, int *nkeys )
{
	return PMPC_Info_get_nkeys( (MPC_Info) info, nkeys );
}

int PMPI_Info_get_nthkey( MPI_Info info, int n, char *key )
{
	return PMPC_Info_get_nthkey( (MPC_Info) info, n , key );
}

int PMPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, int *flag)
{
	return PMPC_Info_get_valuelen( (MPC_Info) info , key , valuelen , flag );
}

/*************************
*  MPI Keyval Management *
*************************/

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn,
							int *comm_keyval, void *extra_state)
{
	return  __INTERNAL__PMPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
}

int PMPI_Comm_free_keyval(int *comm_keyval)
{
	return __INTERNAL__PMPI_Comm_free_keyval (comm_keyval);
}


int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
	return MPI_SUCCESS;

}

int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
	return MPI_SUCCESS;


}


int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
	return MPI_SUCCESS;

}

/********************************* 
*  MPI_Halo Extra halo interface *
*********************************/
int PMPIX_Halo_cell_init( MPI_Halo * halo, char * label, MPI_Datatype type, int count )
{
	return __INTERNAL__PMPIX_Halo_cell_init( halo, label, type, count );
}

int PMPIX_Halo_cell_release( MPI_Halo * halo )
{
	return __INTERNAL__PMPIX__Halo_cell_release( halo );
}


int PMPIX_Halo_cell_set( MPI_Halo halo, void * ptr )
{
	return __INTERNAL__PMPIX__Halo_cell_set( halo , ptr );
}

int PMPIX_Halo_cell_get( MPI_Halo halo, void ** ptr )
{
	return __INTERNAL__PMPIX__Halo_cell_get( halo, ptr );
}

/* Halo Exchange */

int PMPIX_Halo_exchange_init( MPI_Halo_exchange * ex )
{
	return __INTERNAL__PMPIX__Halo_exchange_init( ex );
}

int PMPIX_Halo_exchange_release( MPI_Halo_exchange * ex )
{
	return __INTERNAL__PMPIX__Halo_exchange_release( ex );
}



int PMPIX_Halo_exchange_commit( MPI_Halo_exchange ex )
{
	return __INTERNAL__PMPIX__Halo_exchange_commit( ex );
}

int PMPIX_Halo_exchange( MPI_Halo_exchange ex )
{
	return __INTERNAL__PMPIX__Halo_exchange( ex );
}

int PMPIX_Halo_iexchange( MPI_Halo_exchange ex )
{
	return __INTERNAL__PMPIX__Halo_iexchange( ex );
}

int PMPIX_Halo_iexchange_wait( MPI_Halo_exchange ex )
{
	return __INTERNAL__PMPIX_Halo_iexchange_wait( ex );
}


int PMPIX_Halo_cell_bind_local( MPI_Halo_exchange ex, MPI_Halo halo )
{
	return __INTERNAL__PMPIX__Halo_cell_bind_local( ex, halo );
}

int PMPIX_Halo_cell_bind_remote( MPI_Halo_exchange ex, MPI_Halo halo, int remote, char * remote_label )
{
	return __INTERNAL__PMPIX__Halo_cell_bind_remote( ex, halo, remote, remote_label );
}


/************************************************************************/
/*  The MPI Tools Inteface                                              */
/************************************************************************/

int PMPI_T_init_thread(int required, int *provided) {
  return mpc_MPI_T_init_thread(required, provided);
}

int PMPI_T_finalize(void) { return mpc_MPI_T_finalize(); }

int PMPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name,
                         int *name_len) {
  return mpc_MPI_T_enum_get_info(enumtype, num, name, name_len);
}

int PMPI_T_enum_get_item(MPI_T_enum enumtype, int index, int *value, char *name,
                         int *name_len) {
  return mpc_MPI_T_enum_get_item(enumtype, index, value, name, name_len);
}

int PMPI_T_cvar_get_num(int *num_cvar) {
  return mpc_MPI_T_cvar_get_num(num_cvar);
}

int PMPI_T_cvar_get_info(int cvar_index, char *name, int *name_len,
                         int *verbosity, MPI_Datatype *datatype,
                         MPI_T_enum *enumtype, char *desc, int *desc_len,
                         int *bind, int *scope) {
  return mpc_MPI_T_cvar_get_info(cvar_index, name, name_len, verbosity,
                                 datatype, enumtype, desc, desc_len, bind,
                                 scope);
}

int PMPI_T_cvar_get_index(const char *name, int *cvar_index) {
  return mpc_MPI_T_cvar_get_index(name, cvar_index);
}

int PMPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle,
                             MPI_T_cvar_handle *handle, int *count) {
  return mpc_MPI_T_cvar_handle_alloc(cvar_index, obj_handle, handle, count);
}

int PMPI_T_cvar_handle_free(MPI_T_cvar_handle *handle) {
  return mpc_MPI_T_cvar_handle_free(handle);
}

int PMPI_T_cvar_read(MPI_T_cvar_handle handle, void *buff) {
  return mpc_MPI_T_cvar_read(handle, buff);
}

int PMPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buff) {
  return mpc_MPI_T_cvar_write(handle, buff);
}

int PMPI_T_pvar_get_num(int *num_pvar) {
  return mpc_MPI_T_pvar_get_num(num_pvar);
}

int PMPI_T_pvar_get_info(int pvar_index, char *name, int *name_len,
                         int *verbosity, int *var_class, MPI_Datatype *datatype,
                         MPI_T_enum *enumtype, char *desc, int *desc_len,
                         int *bind, int *readonly, int *continuous,
                         int *atomic) {
  return mpc_MPI_T_pvar_get_info(pvar_index, name, name_len, verbosity,
                                 var_class, datatype, enumtype, desc, desc_len,
                                 bind, readonly, continuous, atomic);
}

int PMPI_T_pvar_get_index(char *name, int *pvar_class, int *pvar_index) {
  return mpc_MPI_T_pvar_get_index(name, pvar_class, pvar_index);
}

int PMPI_T_pvar_session_create(MPI_T_pvar_session *session) {
  return mpc_MPI_T_pvar_session_create(session);
}

int PMPI_T_pvar_session_free(MPI_T_pvar_session *session) {
  return mpc_MPI_T_pvar_session_free(session);
}

int PMPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index,
                             void *obj_handle, MPI_T_pvar_handle *handle,
                             int *count) {
  return mpc_MPI_T_pvar_handle_alloc(session, pvar_index, obj_handle, handle,
                                     count);
}

int PMPI_T_pvar_handle_free(MPI_T_pvar_session session,
                            MPI_T_pvar_handle *handle) {
  return mpc_MPI_T_pvar_handle_free(session, handle);
}

int PMPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle) {
  return mpc_MPI_T_pvar_start(session, handle);
}

int PMPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle) {
  return mpc_MPI_T_pvar_stop(session, handle);
}

int PMPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                     void *buff) {
  return mpc_MPI_T_pvar_read(session, handle, buff);
}

int PMPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                          void *buff) {
  return mpc_MPI_T_pvar_readreset(session, handle, buff);
}

int PMPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                      const void *buff) {
  return mpc_MPI_T_pvar_write(session, handle, buff);
}

int PMPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle) {
  return mpc_MPI_T_pvar_reset(session, handle);
}

int PMPI_T_category_get_num(int *num_cat) {
  return mpc_MPI_T_category_get_num(num_cat);
}

int PMPI_T_category_get_info(int cat_index, char *name, int *name_len,
                             char *desc, int *desc_len, int *num_cvars,
                             int *num_pvars, int *num_categories) {
  return mpc_MPI_T_category_get_info(cat_index, name, name_len, desc, desc_len,
                                     num_cvars, num_pvars, num_categories);
}
int PMPI_T_category_get_index(char *name, int *cat_index) {
  return mpc_MPI_T_category_get_index(name, cat_index);
}
int PMPI_T_category_get_cvars(int cat_index, int len, int indices[]) {
  return mpc_MPI_T_category_get_cvars(cat_index, len, indices);
}

int PMPI_T_category_get_pvars(int cat_index, int len, int indices[]) {
  return mpc_MPI_T_category_get_pvars(cat_index, len, indices);
}

int PMPI_T_category_get_categories(int cat_index, int len, int indices[]) {
  return mpc_MPI_T_category_get_categories(cat_index, len, indices);
}

int PMPI_T_category_changed(int *stamp) {
  return mpc_MPI_T_category_changed(stamp);
}

/* Error Handling */

int PMPI_Add_error_class(int *errorclass) {
  not_implemented();
  return MPI_ERR_INTERN;
}

int PMPI_Add_error_code(int errorclass, int *errorcode) {
  not_implemented();
  return MPI_ERR_INTERN;
}

int PMPI_Add_error_string(int errorcode, char *string) {
  not_implemented();
  return MPI_ERR_INTERN;
}

int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *function,
                                MPI_Errhandler *errhandler) {
  return PMPC_Errhandler_create((MPC_Handler_function *)function, errhandler);
}

int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler) {
  return PMPC_Errhandler_get(comm, errhandler);
}

int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler) {
  return PMPC_Errhandler_set(comm, errhandler);
}

int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode) {
  sctk_errhandler_t errh =
      sctk_handle_get_errhandler((sctk_handle)comm, SCTK_HANDLE_COMM);
  sctk_generic_handler errf = sctk_errhandler_resolve(errh);

  if (errf) {
    (errf)((void *)&comm, &errorcode);
  }

  return MPI_SUCCESS;
}

int PMPI_Win_call_errhandler(MPI_Win win, int errorcode) {
  sctk_errhandler_t errh =
      sctk_handle_get_errhandler((sctk_handle)win, SCTK_HANDLE_WIN);
  sctk_generic_handler errf = sctk_errhandler_resolve(errh);

  if (errf) {
    (errf)((void *)&win, &errorcode);
  }

  return MPI_ERR_INTERN;
}

/*************************
*  MPI One-SIDED         *
*************************/

#include "mpi_rma.h"

/* MPI Info storage in WIN                                              */

int PMPI_Win_set_info(MPI_Win win, MPI_Info info) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);
  if (!desc)
    return MPI_ERR_ARG;

  desc->info = info;

  return MPI_SUCCESS;
}

int PMPI_Win_get_info(MPI_Win win, MPI_Info *info) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  if (!desc)
    return MPI_ERR_ARG;

  PMPI_Info_dup(desc->info, info);

  return MPI_SUCCESS;
}

/* MPI Window Naming                                                    */

int PMPI_Win_set_name(MPI_Win win, const char *name) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  if (!desc) {
    return MPI_ERR_ARG;
  }

  if (MPI_MAX_OBJECT_NAME <= strlen(name)) {
    return MPI_ERR_ARG;
  }

  sctk_free(desc->win_name);
  desc->win_name = strdup(name);

  return MPI_SUCCESS;
}

int PMPI_Win_get_name(MPI_Win win, char *name, int *len) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  if (!desc)
    return MPI_ERR_ARG;

  sprintf(name, "%s", desc->win_name);
  *len = strlen(desc->win_name);

  return MPI_SUCCESS;
}

/* MPI Window Creation/Release                                          */

int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                    MPI_Comm comm, MPI_Win *win) {
  /* MPI Windows need more progress
   * we must give up on agressive collectives */
  __do_yield |= 1;
  return mpc_MPI_Win_create(base, size, disp_unit, info, comm, win);
}

int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                      MPI_Comm comm, void *base, MPI_Win *win) {
  /* MPI Windows need more progress
   * we must give up on agressive collectives */
  __do_yield |= 1;
  return mpc_MPI_Win_allocate(size, disp_unit, info, comm, base, win);
}

int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
                             MPI_Comm comm, void *base, MPI_Win *win) {
  /* MPI Windows need more progress
   * we must give up on agressive collectives */
  __do_yield |= 1;
  return mpc_MPI_Win_allocate_shared(size, disp_unit, info, comm, base, win);
}

int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win) {
  /* MPI Windows need more progress
   * we must give up on agressive collectives */
  __do_yield |= 1;
  return mpc_MPI_Win_create_dynamic(info, comm, win);
}

int PMPI_Win_attach(MPI_Win, void *, MPI_Aint);
int PMPI_Win_detach(MPI_Win, const void *);

int PMPI_Win_free(MPI_Win *win) { return mpc_MPI_Win_free(win); }

/* RDMA Operations */

int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win) {
  return mpc_MPI_Get(origin_addr, origin_count, origin_datatype, target_rank,
                     target_disp, target_count, target_datatype, win);
}

int PMPI_Put(const void *origin_addr, int origin_count,
             MPI_Datatype origin_datatype, int target_rank,
             MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win) {
  return mpc_MPI_Put(origin_addr, origin_count, origin_datatype, target_rank,
                     target_disp, target_count, target_datatype, win);
}

int PMPI_Rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request) {
  return mpc_MPI_Rput(origin_addr, origin_count, origin_datatype, target_rank,
                      target_disp, target_count, target_datatype, win, request);
}

int PMPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
              int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request) {
  return mpc_MPI_Rget(origin_addr, origin_count, origin_datatype, target_rank,
                      target_disp, target_count, target_datatype, win, request);
}

/* Synchronization calls Epochs */

int PMPI_Win_fence(int assert, MPI_Win win) {
  return mpc_MPI_Win_fence(assert, win);
}

/* Active target sync */

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win) {
  return mpc_MPI_Win_start(group, assert, win);
}

int PMPI_Win_complete(MPI_Win win) { return mpc_MPI_Win_complete(win); }

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win) {
  return mpc_MPI_Win_post(group, assert, win);
}

int PMPI_Win_wait(MPI_Win win) { return mpc_MPI_Win_wait(win); }

int PMPI_Win_test(MPI_Win win, int *flag) {
  return mpc_MPI_Win_test(win, flag);
}

/* Passive Target Sync */

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win) {
  return mpc_MPI_Win_lock(lock_type, rank, assert, win);
}

int PMPI_Win_unlock(int rank, MPI_Win win) {
  return mpc_MPI_Win_unlock(rank, win);
}

int PMPI_Win_lock_all(int assert, MPI_Win win) {
  return mpc_MPI_Win_lock_all(assert, win);
}

int PMPI_Win_unlock_all(MPI_Win win) { return mpc_MPI_Win_unlock_all(win); }

/* Error Handling */

int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                               MPI_Errhandler *errhandler) {
  return PMPC_Errhandler_create((MPC_Handler_function *)win_errhandler_fn,
                                errhandler);
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler) {
  sctk_handle_set_errhandler((sctk_handle)win, SCTK_HANDLE_WIN,
                             (sctk_errhandler_t)errhandler);
  return MPI_SUCCESS;
}

int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler) {
  *errhandler = (MPC_Errhandler)sctk_handle_get_errhandler((sctk_handle)win,
                                                           SCTK_HANDLE_WIN);
  return MPI_SUCCESS;
}

/* Window Flush Calls */

int PMPI_Win_sync(MPI_Win win) { return mpc_MPI_Win_sync(win); }

int PMPI_Win_flush(int rank, MPI_Win win) {
  return mpc_MPI_Win_flush(rank, win);
}

int PMPI_Win_flush_local(int rank, MPI_Win win) {
  return mpc_MPI_Win_flush_local(rank, win);
}

int PMPI_Win_flush_all(MPI_Win win) { return mpc_MPI_Win_flush_all(win); }

int PMPI_Win_flush_local_all(MPI_Win win) {
  return mpc_MPI_Win_flush_local_all(win);
}

int PMPI_Win_get_group(MPI_Win win, MPI_Group *group) {
  return mpc_PMPI_Win_get_group(win, group);
}

int PMPI_Accumulate(const void *origin_addr, int origin_count,
                    MPI_Datatype origin_datatype, int target_rank,
                    MPI_Aint target_disp, int target_count,
                    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win) {
  return mpc_MPI_Accumulate(origin_addr, origin_count, origin_datatype,
                            target_rank, target_disp, target_count,
                            target_datatype, op, win);
}

int PMPI_Raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request *request) {
  return mpc_MPI_Raccumulate(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count,
                             target_datatype, op, win, request);
}

int PMPI_Get_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr,
                        int result_count, MPI_Datatype result_datatype,
                        int target_rank, MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win) {
  return mpc_MPI_Get_accumulate(origin_addr, origin_count, origin_datatype,
                                result_addr, result_count, result_datatype,
                                target_rank, target_disp, target_count,
                                target_datatype, op, win);
}

int PMPI_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr,
                         int result_count, MPI_Datatype result_datatype,
                         int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype,
                         MPI_Op op, MPI_Win win, MPI_Request *request) {
  return mpc_MPI_Rget_accumulate(origin_addr, origin_count, origin_datatype,
                                 result_addr, result_count, result_datatype,
                                 target_rank, target_disp, target_count,
                                 target_datatype, op, win, request);
}

int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank,
                      MPI_Aint target_disp, MPI_Op op, MPI_Win win) {
  return mpc_MPI_Fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                              target_disp, op, win);
}

int PMPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype,
                          int target_rank, MPI_Aint target_disp, MPI_Win win) {
  return mpc_MPI_Compare_and_swap(origin_addr, compare_addr, result_addr,
                                  datatype, target_rank, target_disp, win);
}

/* ATTR handling */

int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val) {
  return mpc_MPI_Win_set_attr(win, win_keyval, attribute_val);
}

int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val,
                      int *flag) {
  return mpc_MPI_Win_get_attr(win, win_keyval, attribute_val, flag);
}

int PMPI_Win_free_keyval(int *win_keyval) {
  return mpc_MPI_Win_free_keyval(win_keyval);
}

int PMPI_Win_delete_attr(MPI_Win win, int win_keyval) {
  return mpc_MPI_Win_delete_attr(win, win_keyval);
}

int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                           MPI_Win_delete_attr_function *win_delete_attr_fn,
                           int *win_keyval, void *extra_state) {
  return mpc_MPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn,
                                   win_keyval, extra_state);
}

/* Win shared */

int PMPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit,
                          void *baseptr) {
  return mpc_MPI_Win_shared_query(win, rank, size, disp_unit, baseptr);
}

int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size) {
  return MPI_SUCCESS;
}

int PMPI_Win_detach(MPI_Win win, const void *base) { return MPI_SUCCESS; }

/* Checkpointing */
int PMPIX_Checkpoint(MPIX_Checkpoint_state* state)
{
	return __INTERNAL__PMPIX_Checkpoint(state);
}

/************************************************************************/
/*  NOT IMPLEMENTED                                                     */
/************************************************************************/

/* Communicator Management */
int PMPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info * info_used){not_implemented();return MPI_ERR_INTERN;}

int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm){not_implemented();return MPI_ERR_INTERN;}

int PMPI_Get_library_version( char *version, int *resultlen )
{
	snprintf( version, MPI_MAX_LIBRARY_VERSION_STRING - 1,
			  "MPC version %d.%d.%d%s %s %s",
			  SCTK_VERSION_MAJOR, SCTK_VERSION_MINOR, SCTK_VERSION_REVISION,
			  SCTK_VERSION_PRE,
			  sctk_alloc_mode(), get_debug_mode() );
	*resultlen = strlen( version );
	return MPI_SUCCESS;
}

/* Process Creation and Management */
int PMPI_Close_port(const char *port_name){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm,MPI_Comm *newcomm){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_disconnect(MPI_Comm * comm){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_get_parent(MPI_Comm *parent){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_join(int fd, MPI_Comm *intercomm){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info,int root, MPI_Comm comm, MPI_Comm *intercomm,int array_of_errcodes[]){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[],char **array_of_argv[], const int array_of_maxprocs[],const MPI_Info array_of_info[], int root, MPI_Comm comm,MPI_Comm *intercomm, int array_of_errcodes[]){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Open_port(MPI_Info info, char *port_name){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name){not_implemented();return MPI_ERR_INTERN;}


/* Dist graph operations */
int PMPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[]){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[],const int degrees[], const int destinations[],const int weights[],MPI_Info info, int reorder, MPI_Comm *comm_dist_graph){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Dist_graph_create_adjacent(MPI_Comm comm_old,int indegree, const int sources[],const int sourceweights[],int outdegree, const int destinations[],const int destweights[],MPI_Info info, int reorder, MPI_Comm *comm_dist_graph){not_implemented();return MPI_ERR_INTERN;}

/* collectives */
int PMPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){not_implemented();return MPI_ERR_INTERN;}

/* Error handling */
int PMPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn, MPI_Errhandler *errhandler){not_implemented();return MPI_ERR_INTERN;}
int PMPI_File_call_errhandler(void * fh, int errorcode){not_implemented();return MPI_ERR_INTERN;}


/* FORTRAN TYPE */
int PMPI_Type_create_f90_complex(int precision, int range, MPI_Datatype *newtype){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Type_create_f90_integer(int range, MPI_Datatype *newtype){not_implemented();return MPI_ERR_INTERN;}
int PMPI_Type_create_f90_real(int precision, int range, MPI_Datatype *newtype){not_implemented();return MPI_ERR_INTERN;}

/* MPIX methods */
int PMPIX_Comm_failure_ack( MPI_Comm comm ){not_implemented();return MPI_ERR_INTERN;}
int PMPIX_Comm_failure_get_acked( MPI_Comm comm, MPI_Group *failedgrp ){not_implemented();return MPI_ERR_INTERN;}
int PMPIX_Comm_agree(MPI_Comm comm, int *flag){not_implemented();return MPI_ERR_INTERN;}
int PMPIX_Comm_revoke(MPI_Comm comm){not_implemented();return MPI_ERR_INTERN;}
int PMPIX_Comm_shrink(MPI_Comm comm, MPI_Comm *newcomm){not_implemented();return MPI_ERR_INTERN;}

/************************************************************************/
/* END NOT IMPLEMENTED                                                     */
/************************************************************************/

