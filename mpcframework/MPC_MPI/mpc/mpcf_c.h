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

const void *ffunc (mpc_sum) = &MPC_SUM;
const void *ffunc (mpc_max) = &MPC_MAX;
const void *ffunc (mpc_min) = &MPC_MIN;
const void *ffunc (mpc_prod) = &MPC_PROD;
const void *ffunc (mpc_land) = &MPC_LAND;
const void *ffunc (mpc_band) = &MPC_BAND;
const void *ffunc (mpc_lor) = &MPC_LOR;
const void *ffunc (mpc_bor) = &MPC_BOR;
const void *ffunc (mpc_lxor) = &MPC_LXOR;
const void *ffunc (mpc_bxor) = &MPC_BXOR;
const void *ffunc (mpc_minloc) = &MPC_MINLOC;
const void *ffunc (mpc_maxloc) = &MPC_MAXLOC;


/*Initialisation*/
void ffunc (mpc_init) (int *err)
{
  *err = MPC_SUCCESS;
}

void ffunc (mpc_init_thread) (int *required, int *provided, int *err)
{
  int argc;
  char **argv;

  *err = MPC_Init_thread (&argc, &argv, *required, provided);
}

void ffunc (mpc_finalize) (int *err)
{
  *err = MPC_Finalize ();
}

void ffunc (mpc_abort) (MPC_Comm * comm, int *errorcode, int *err)
{
  *err = MPC_Abort (*comm, *errorcode);
}

/*Topology informations */
void ffunc (mpc_comm_rank) (MPC_Comm * comm, int *rank, int *err)
{
  *err = MPC_Comm_rank (*comm, rank);
}

void ffunc (mpc_comm_size) (MPC_Comm * comm, int *size, int *err)
{
  *err = MPC_Comm_size (*comm, size);
}

void ffunc (mpc_node_rank) (int *rank, int *err)
{
  *err = MPC_Node_rank (rank);
}

void ffunc (mpc_node_number) (int *number, int *err)
{
  *err = MPC_Node_number (number);
}

void ffunc (mpc_processor_rank) (int *rank, int *err)
{
  *err = MPC_Processor_rank (rank);
}

void ffunc (mpc_processor_number) (int *number, int *err)
{
  *err = MPC_Processor_number (number);
}

void ffunc (mpc_process_rank) (int *rank, int *err)
{
  *err = MPC_Process_rank (rank);
}

void ffunc (mpc_process_number) (int *number, int *err)
{
  *err = MPC_Process_number (number);
}

void ffunc (mpc_get_multithreading) (char *name, int *size, int *err)
{
  *err = MPC_Get_multithreading (name, *size);
}

void ffunc (mpc_get_networking) (char *name, int *size, int *err)
{
  *err = MPC_Get_networking (name, *size);
}

/*Collective operations */
void ffunc (mpc_barrier) (MPC_Comm * comm, int *err)
{
  *err = MPC_Barrier (*comm);
}

void ffunc (mpc_bcast) (void *buffer, int *count, MPC_Datatype * datatype,
			int *root, MPC_Comm * comm, int *err)
{
  *err = MPC_Bcast (buffer, *count, *datatype, *root, *comm);
}

/*Il y a un petit pb avec icc et ia64 sur le passage en argument des op il faut d?r?f?rencer une fois de plus*/
#if defined(SCTK_ia64_ARCH_SCTK)
void ffunc (mpc_allreduce) (void *sendbuf, void *recvbuf, int *count,
			    MPC_Datatype * datatype, MPC_Op *** op,
			    MPC_Comm * comm, int *err)
{
/*   int rank; */
/*   MPC_Comm_rank(MPC_COMM_WORLD,&rank); */
/*   fprintf(stderr,"Rank %d %p %p\n",rank,sendbuf,recvbuf); */
  *err = MPC_Allreduce (sendbuf, recvbuf, *count, *datatype, ***op, *comm);
}

void ffunc (mpc_reduce) (void *sendbuf, void *recvbuf, int *count,
			 MPC_Datatype * datatype, MPC_Op *** op, int *root,
			 MPC_Comm * comm, int *err)
{
  *err = MPC_Reduce (sendbuf, recvbuf, *count,
		     *datatype, ***op, *root, *comm);
}
#else
void ffunc (mpc_allreduce) (void *sendbuf, void *recvbuf, int *count,
			    MPC_Datatype * datatype, MPC_Op ** op,
			    MPC_Comm * comm, int *err)
{
/*   int rank; */
/*   MPC_Comm_rank(MPC_COMM_WORLD,&rank); */
/*   fprintf(stderr,"Rank %d %p %p\n",rank,sendbuf,recvbuf); */
  *err = MPC_Allreduce (sendbuf, recvbuf, *count, *datatype, **op, *comm);
}

void ffunc (mpc_reduce) (void *sendbuf, void *recvbuf, int *count,
			 MPC_Datatype * datatype, MPC_Op ** op, int *root,
			 MPC_Comm * comm, int *err)
{
  *err = MPC_Reduce (sendbuf, recvbuf, *count, *datatype, **op, *root, *comm);
}
#endif

/*P-t-P Communications */
void ffunc (mpc_isend) (void *buf, int *count, MPC_Datatype * datatype,
			int *dest, int *tag, MPC_Comm * comm,
			MPC_Request * req, int *err)
{
  *err = MPC_Isend (buf, *count, *datatype, *dest, *tag, *comm, req);
}

void ffunc (mpc_ibsend) (void *buf, int *count, MPC_Datatype * datatype,
			 int *dest, int *tag, MPC_Comm * comm,
			 MPC_Request * req, int *err)
{
  *err = MPC_Isend (buf, *count, *datatype, *dest, *tag, *comm, req);
}

void ffunc (mpc_issend) (void *buf, int *count, MPC_Datatype * datatype,
			 int *dest, int *tag, MPC_Comm * comm,
			 MPC_Request * req, int *err)
{
  *err = MPC_Isend (buf, *count, *datatype, *dest, *tag, *comm, req);
}

void ffunc (mpc_irsend) (void *buf, int *count, MPC_Datatype * datatype,
			 int *dest, int *tag, MPC_Comm * comm,
			 MPC_Request * req, int *err)
{
  *err = MPC_Isend (buf, *count, *datatype, *dest, *tag, *comm, req);
}

void ffunc (mpc_irecv) (void *buf, int *count, MPC_Datatype * datatype,
			int *source, int *tag, MPC_Comm * comm,
			MPC_Request * request, int *err)
{
  *err = MPC_Irecv (buf, *count, *datatype, *source, *tag, *comm, request);
}

void ffunc (mpc_wait) (MPC_Request * request, MPC_Status * status, int *err)
{
  *err = MPC_Wait (request, status);
}

void ffunc (mpc_waitall) (int *count, MPC_Request * request,
			  MPC_Status * status, int *err)
{
  *err = MPC_Waitall (*count, request, status);
}

void ffunc (mpc_waitsome) (int *a, MPC_Request * b, int *c, int *d,
			   MPC_Status * e, int *err)
{
  *err = MPC_Waitsome (*a, b, c, d, e);
}

void ffunc (mpc_waitany) (mpc_msg_count * count,
			  MPC_Request array_of_requests[],
			  mpc_msg_count * index, MPC_Status * status,
			  int *err)
{
  *err = MPC_Waitany (*count, array_of_requests, index, status);
}

void ffunc (mpc_wait_pending) (MPC_Comm * comm, int *err)
{
  *err = MPC_Wait_pending (*comm);
}

void ffunc (mpc_wait_pending_all_comm) (int *err)
{
  *err = MPC_Wait_pending_all_comm ();
}

void ffunc (mpc_test) (MPC_Request * req, int *flag, MPC_Status * status,
		       int *err)
{
  *err = MPC_Test (req, flag, status);
}

void ffunc (mpc_iprobe) (int *a, int *b, MPC_Comm * c, int *d,
			 MPC_Status * e, int *err)
{
  *err = MPC_Iprobe (*a, *b, *c, d, e);
}

void ffunc (mpc_probe) (int *a, int *b, MPC_Comm * c, MPC_Status * d,
			int *err)
{
  *err = MPC_Probe (*a, *b, *c, d);
}

void ffunc (mpc_get_count) (MPC_Status * status, MPC_Datatype * dtype,
			    mpc_msg_count * count, int *err)
{
  *err = MPC_Get_count (status, *dtype, count);
}

void ffunc (mpc_send) (void *buf, int *count, MPC_Datatype * datatype,
		       int *dest, int *tag, MPC_Comm * comm, int *err)
{
  *err = MPC_Send (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (mpc_ssend) (void *buf, int *count, MPC_Datatype * datatype,
			int *dest, int *tag, MPC_Comm * comm, int *err)
{
  *err = MPC_Send (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (mpc_rsend) (void *buf, int *count, MPC_Datatype * datatype,
			int *dest, int *tag, MPC_Comm * comm, int *err)
{
  *err = MPC_Send (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (mpc_bsend) (void *buf, int *count, MPC_Datatype * datatype,
			int *dest, int *tag, MPC_Comm * comm, int *err)
{
  *err = MPC_Send (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (mpc_recv) (void *buf, int *count, MPC_Datatype * datatype,
		       int *source, int *tag, MPC_Comm * comm,
		       MPC_Status * request, int *err)
{
  *err = MPC_Recv (buf, *count, *datatype, *source, *tag, *comm, request);
}

void ffunc (mpc_sendrecv) (void *sendbuf, mpc_msg_count * sendcount,
			   MPC_Datatype * sendtype, int *dest, int *sendtag,
			   void *recvbuf, mpc_msg_count * recvcount,
			   MPC_Datatype * recvtype, int *source,
			   int *recvtag, MPC_Comm * comm,
			   MPC_Status * status, int *err)
{
  *err =
    MPC_Sendrecv (sendbuf, *sendcount, *sendtype, *dest, *sendtag,
		  recvbuf, *recvcount, *recvtype, *source, *recvtag,
		  *comm, status);
}

/*Status */
void ffunc (mpc_test_cancelled) (MPC_Status * a, int *b, int *err)
{
  *err = MPC_Test_cancelled (a, b);
}

/*Gather */
void ffunc (mpc_gather) (void *sendbuf, mpc_msg_count * sendcnt,
			 MPC_Datatype * sendtype, void *recvbuf,
			 mpc_msg_count * recvcount, MPC_Datatype * recvtype,
			 int *root, MPC_Comm * comm, int *err)
{
  *err =
    MPC_Gather (sendbuf, *sendcnt, *sendtype, recvbuf, *recvcount,
		*recvtype, *root, *comm);
}

void ffunc (mpc_gatherv) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			  void *d, mpc_msg_count * e, mpc_msg_count * f,
			  MPC_Datatype * g, int *h, MPC_Comm * i, int *err)
{
  *err = MPC_Gatherv (a, *b, *c, d, e, f, *g, *h, *i);
}

void ffunc (mpc_allgather) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			    void *d, mpc_msg_count * e, MPC_Datatype * f,
			    MPC_Comm * g, int *err)
{
  *err = MPC_Allgather (a, *b, *c, d, *e, *f, *g);
}

void ffunc (mpc_allgatherv) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			     void *d, mpc_msg_count * e, mpc_msg_count * f,
			     MPC_Datatype * g, MPC_Comm * h, int *err)
{
  *err = MPC_Allgatherv (a, *b, *c, d, e, f, *g, *h);
}

/*Scatter */
void ffunc (mpc_scatter) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			  void *d, mpc_msg_count * e, MPC_Datatype * g,
			  int *h, MPC_Comm * i, int *err)
{
  *err = MPC_Scatter (a, *b, *c, d, *e, *g, *h, *i);
}

void ffunc (mpc_scatterv) (void *a, mpc_msg_count * b, mpc_msg_count * c,
			   MPC_Datatype * d, void *e, mpc_msg_count * f,
			   MPC_Datatype * g, int *h, MPC_Comm * i, int *err)
{
  *err = MPC_Scatterv (a, b, c, *d, e, *f, *g, *h, *i);
}

/*Alltoall */
void ffunc (mpc_alltoall) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			   void *d, mpc_msg_count * e, MPC_Datatype * f,
			   MPC_Comm * g, int *err)
{
  *err = MPC_Alltoall (a, *b, *c, d, *e, *f, *g);
}

void ffunc (mpc_alltoallv) (void *a, mpc_msg_count * b, mpc_msg_count * c,
			    MPC_Datatype * d, void *e, mpc_msg_count * f,
			    mpc_msg_count * g, MPC_Datatype * h,
			    MPC_Comm * i, int *err)
{
  *err = MPC_Alltoallv (a, b, c, *d, e, f, g, *h, *i);
}

/*Informations */
void ffunc (mpc_get_processor_name) (char *a, int *b, int *err)
{
  *err = MPC_Get_processor_name (a, b);
}

/*Groups */
void ffunc (mpc_comm_group) (MPC_Comm * a, MPC_Group * b, int *err)
{
  *err = MPC_Comm_group (*a, b);
}

void ffunc (mpc_group_free) (MPC_Group * a, int *err)
{
  *err = MPC_Group_free (a);
}

void ffunc (mpc_group_incl) (MPC_Group * a, int *b, int *c, MPC_Group * d,
			     int *err)
{
  *err = MPC_Group_incl (*a, *b, c, d);
}

void ffunc (mpc_group_difference) (MPC_Group * a, MPC_Group * b,
				   MPC_Group * c, int *err)
{
  *err = MPC_Group_difference (*a, *b, c);
}


/*Communicators */
void ffunc (mpc_comm_create_list) (MPC_Comm * a, int *list, int *nb_elem,
				   MPC_Comm * b, int *err)
{
  *err = MPC_Comm_create_list (*a, list, *nb_elem, b);
}

void ffunc (mpc_comm_create) (MPC_Comm * a, MPC_Group * b, MPC_Comm * c,
			      int *err)
{
  *err = MPC_Comm_create (*a, *b, c);
}

void ffunc (mpc_comm_free) (MPC_Comm * a, int *err)
{
  *err = MPC_Comm_free (a);
}

void ffunc (mpc_comm_dup) (MPC_Comm * a, MPC_Comm * b, int *err)
{
  *err = MPC_Comm_dup (*a, b);
}

void ffunc (mpc_comm_split) (MPC_Comm * a, int *b, int *c, MPC_Comm * d,
			     int *err)
{
  *err = MPC_Comm_split (*a, *b, *c, d);
}

/*Error_handler */
void ffunc (mpc_default_error) (MPC_Comm * comm, int *error, char *msg,
				char *file, int *line)
{
  MPC_Default_error (comm, error, msg, file, *line);
}

#if 0
void ffunc (mpc_return_error) (, int *err)
{
  *err = ();
}
#endif
void ffunc (mpc_errhandler_create) (MPC_Handler_function * a,
				    MPC_Errhandler * b, int *err)
{
  *err = MPC_Errhandler_create (a, b);
}

void ffunc (mpc_errhandler_set) (MPC_Comm * a, MPC_Errhandler * b, int *err)
{
  *err = MPC_Errhandler_set (*a, *b);
}

void ffunc (mpc_errhandler_get) (MPC_Comm * a, MPC_Errhandler * b, int *err)
{
  *err = MPC_Errhandler_get (*a, b);
}

void ffunc (mpc_errhandler_free) (MPC_Errhandler * a, int *err)
{
  *err = MPC_Errhandler_free (a);
}

void ffunc (mpc_error_string) (int *a, char *b, int *c, int *err)
{
  *err = MPC_Error_string (*a, b, c);
}

void ffunc (mpc_error_class) (int *a, int *b, int *err)
{
  *err = MPC_Error_class (*a, b);
}

/*Timing */
double *ffunc (mpc_wtime) (void)
{
  static double res;
  res = MPC_Wtime ();
  return &res;
}

double *ffunc (mpc_wtick) (void)
{
  static double res;
  res = MPC_Wtick ();
  return &res;
}

/*Types */
void ffunc (mpc_type_size) (MPC_Datatype * a, size_t * b, int *err)
{
  *err = MPC_Type_size (*a, b);
}

void ffunc (MPC_Type_hcontiguous) (MPC_Datatype * a, size_t * b, int *err)
{
  *err = MPC_Type_hcontiguous (a, *b);
}

void ffunc (mpc_type_free) (MPC_Datatype * datatype, int *err)
{
  *err = MPC_Type_free (datatype);
}

/*Requests */
void ffunc (mpc_request_free) (MPC_Request * a, int *err)
{
  *err = MPC_Request_free (a);
}

/*Scheduling */
void ffunc (mpc_proceed) (int *err)
{
  *err = MPC_Proceed ();
}

void ffunc (mpc_checkpoint) (MPC_Checkpoint_state* st, int *err)
{
  *err = MPC_Checkpoint (st);
}

void ffunc (mpc_migrate) (int *err)
{
  *err = MPC_Migrate ();
}

void ffunc (mpc_restart) (int *rank, int *err)
{
  *err = MPC_Restart (*rank);
}

void ffunc (mpc_restarted) (int *flag, int *err)
{
  *err = MPC_Restarted (flag);
}

void ffunc (mpc_get_activity) (int *nb_item, MPC_Activity_t * tab,
			       double *process_act, int *err)
{
  *err = MPC_Get_activity (*nb_item, tab, process_act);
}

void ffunc (mpc_move_to) (int *process, int *cpuid, int *err)
{
  *err = MPC_Move_to (*process, *cpuid);
}

/*Packs */
void ffunc (mpc_open_pack) (MPC_Request * request, int *err)
{
  *err = MPC_Open_pack (request);
}

void ffunc (mpc_default_pack) (mpc_msg_count * count,
			       mpc_pack_indexes_t * begins,
			       mpc_pack_indexes_t * ends,
			       MPC_Request * request, int *err)
{
  *err = MPC_Default_pack (*count, begins, ends, request);
}

void ffunc (mpc_add_pack) (void *buf, mpc_msg_count * count,
			   mpc_pack_indexes_t * begins,
			   mpc_pack_indexes_t * ends,
			   MPC_Datatype * datatype, MPC_Request * request,
			   int *err)
{
  *err = MPC_Add_pack (buf, *count, begins, ends, *datatype, request);
}

void ffunc (mpc_add_pack_absolute) (void *buf, mpc_msg_count * count,
				    mpc_pack_absolute_indexes_t * begins,
				    mpc_pack_absolute_indexes_t * ends,
				    MPC_Datatype * datatype,
				    MPC_Request * request, int *err)
{
  *err =
    MPC_Add_pack_absolute (buf, *count, begins, ends, *datatype, request);
}

void ffunc (mpc_add_pack_default) (void *buf, MPC_Datatype datatype,
				   MPC_Request * request, int *err)
{
  *err = MPC_Add_pack_default (buf, datatype, request);
}

void ffunc (mpc_isend_pack) (int *dest, int *tag, MPC_Comm * comm,
			     MPC_Request * request, int *err)
{
  *err = MPC_Isend_pack (*dest, *tag, *comm, request);
}

void ffunc (mpc_irecv_pack) (int *source, int *tag, MPC_Comm * comm,
			     MPC_Request * request, int *err)
{
  *err = MPC_Irecv_pack (*source, *tag, *comm, request);
}

