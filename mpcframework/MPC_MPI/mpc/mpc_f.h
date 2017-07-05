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
#if !defined(__MPC_F_H__) || defined(__MPC_F_H__FORCE)
#define __MPC_F_H__

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(SCTK_USER_FORTRAN_EXT_)
#undef ffunc
#define ffunc(a) a##_
#else

#undef ffunc
#define ffunc(a) a##__
#endif

  extern const void *ffunc (mpc_sum);
  extern const void *ffunc (mpc_max);
  extern const void *ffunc (mpc_min);
  extern const void *ffunc (mpc_prod);
  extern const void *ffunc (mpc_land);
  extern const void *ffunc (mpc_band);
  extern const void *ffunc (mpc_lor);
  extern const void *ffunc (mpc_bor);
  extern const void *ffunc (mpc_lxor);
  extern const void *ffunc (mpc_bxor);
  extern const void *ffunc (mpc_minloc);
  extern const void *ffunc (mpc_maxloc);

  extern int sctk_is_in_fortran;

  void ffunc (mpc_init) (int *err);
  void ffunc (mpc_init_thread) (int *required, int *provided, int *err);
  void ffunc (mpc_finalize) (int *err);
  void ffunc (mpc_abort) (MPC_Comm * comm, int *errorcode, int *err);

  void ffunc (mpc_comm_rank) (MPC_Comm * comm, int *rank, int *err);
  void ffunc (mpc_comm_size) (MPC_Comm * comm, int *size, int *err);
  void ffunc (mpc_node_rank) (int *rank, int *err);
  void ffunc (mpc_node_number) (int *number, int *err);
  void ffunc (mpc_processor_rank) (int *rank, int *err);
  void ffunc (mpc_processor_number) (int *number, int *err);
  void ffunc (mpc_process_rank) (int *rank, int *err);
  void ffunc (mpc_process_number) (int *number, int *err);
  void ffunc (mpc_get_version) (int *version, int *subversion, int *err);
  void ffunc (mpc_get_multithreading) (char *name, int *size, int *err);
  void ffunc (mpc_get_networking) (char *name, int *size, int *err);

  void ffunc (mpc_barrier) (MPC_Comm * comm, int *err);
  void ffunc (mpc_bcast) (void *buffer, int *count,
			  MPC_Datatype * datatype, int *root,
			  MPC_Comm * comm, int *err);
#if defined(SCTK_ia64_ARCH_SCTK)
  void ffunc (mpc_allreduce) (void *sendbuf, void *recvbuf, int *count,
			      MPC_Datatype * datatype, MPC_Op *** op,
			      MPC_Comm * comm, int *err);
  void ffunc (mpc_reduce) (void *sendbuf, void *recvbuf, int *count,
			   MPC_Datatype * datatype, MPC_Op *** op,
			   int *root, MPC_Comm * comm, int *err);
#else
  void ffunc (mpc_allreduce) (void *sendbuf, void *recvbuf, int *count,
			      MPC_Datatype * datatype, MPC_Op ** op,
			      MPC_Comm * comm, int *err);
  void ffunc (mpc_reduce) (void *sendbuf, void *recvbuf, int *count,
			   MPC_Datatype * datatype, MPC_Op ** op,
			   int *root, MPC_Comm * comm, int *err);
#endif


/*P-t-P Communications */
  void ffunc (mpc_isend) (void *buf, int *count, MPC_Datatype * datatype,
			  int *dest, int *tag, MPC_Comm * comm,
			  MPC_Request * req, int *err);
  void ffunc (mpc_ibsend) (void *buf, int *count, MPC_Datatype * datatype,
			   int *dest, int *tag, MPC_Comm * comm,
			   MPC_Request * req, int *err);
  void ffunc (mpc_issend) (void *buf, int *count, MPC_Datatype * datatype,
			   int *dest, int *tag, MPC_Comm * comm,
			   MPC_Request * req, int *err);
  void ffunc (mpc_irsend) (void *buf, int *count, MPC_Datatype * datatype,
			   int *dest, int *tag, MPC_Comm * comm,
			   MPC_Request * req, int *err);
  void ffunc (mpc_irecv) (void *buf, int *count, MPC_Datatype * datatype,
			  int *source, int *tag, MPC_Comm * comm,
			  MPC_Request * request, int *err);
  void ffunc (mpc_wait) (MPC_Request * request, MPC_Status * status,
			 int *err);
  void ffunc (mpc_waitall) (int *count, MPC_Request * request,
			    MPC_Status * status, int *err);
  void ffunc (mpc_waitsome) (int *a, MPC_Request * b, int *c, int *d,
			     MPC_Status * e, int *err);
  void ffunc (mpc_waitany) (mpc_msg_count * count,
			    MPC_Request array_of_requests[],
			    mpc_msg_count * index, MPC_Status * status,
			    int *err);
  void ffunc (mpc_wait_pending) (MPC_Comm * comm, int *err);
  void ffunc (mpc_wait_pending_all_comm) (int *err);
  void ffunc (mpc_test) (MPC_Request * req, int *flag,
			 MPC_Status * status, int *err);
  void ffunc (mpc_iprobe) (int *a, int *b, MPC_Comm * c, int *d,
			   MPC_Status * e, int *err);
  void ffunc (mpc_probe) (int *a, int *b, MPC_Comm * c, MPC_Status * d,
			  int *err);
  void ffunc (mpc_get_count) (MPC_Status * status, MPC_Datatype * dtype,
			      mpc_msg_count * count, int *err);
  void ffunc (mpc_send) (void *buf, int *count, MPC_Datatype * datatype,
			 int *dest, int *tag, MPC_Comm * comm, int *err);
  void ffunc (mpc_ssend) (void *buf, int *count, MPC_Datatype * datatype,
			  int *dest, int *tag, MPC_Comm * comm, int *err);
  void ffunc (mpc_rsend) (void *buf, int *count, MPC_Datatype * datatype,
			  int *dest, int *tag, MPC_Comm * comm, int *err);
  void ffunc (mpc_bsend) (void *buf, int *count, MPC_Datatype * datatype,
			  int *dest, int *tag, MPC_Comm * comm, int *err);
  void ffunc (mpc_recv) (void *buf, int *count, MPC_Datatype * datatype,
			 int *source, int *tag, MPC_Comm * comm,
			 MPC_Status * request, int *err);
  void ffunc (mpc_sendrecv) (void *sendbuf, mpc_msg_count * sendcount,
			     MPC_Datatype * sendtype, int *dest,
			     int *sendtag, void *recvbuf,
			     mpc_msg_count * recvcount,
			     MPC_Datatype * recvtype, int *source,
			     int *recvtag, MPC_Comm * comm,
			     MPC_Status * status, int *err);

/*Status */
  void ffunc (mpc_test_cancelled) (MPC_Status * a, int *b, int *err);

/*Gather */
  void ffunc (mpc_gather) (void *sendbuf, mpc_msg_count * sendcnt,
			   MPC_Datatype * sendtype, void *recvbuf,
			   mpc_msg_count * recvcount,
			   MPC_Datatype * recvtype, int *root,
			   MPC_Comm * comm, int *err);
  void ffunc (mpc_gatherv) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			    void *d, mpc_msg_count * e, mpc_msg_count * f,
			    MPC_Datatype * g, int *h, MPC_Comm * i, int *err);
  void ffunc (mpc_allgather) (void *a, mpc_msg_count * b,
			      MPC_Datatype * c, void *d,
			      mpc_msg_count * e, MPC_Datatype * f,
			      MPC_Comm * g, int *err);
  void ffunc (mpc_allgatherv) (void *a, mpc_msg_count * b,
			       MPC_Datatype * c, void *d,
			       mpc_msg_count * e, mpc_msg_count * f,
			       MPC_Datatype * g, MPC_Comm * h, int *err);

/*Scatter */
  void ffunc (mpc_scatter) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			    void *d, mpc_msg_count * e, MPC_Datatype * g,
			    int *h, MPC_Comm * i, int *err);
  void ffunc (mpc_scatterv) (void *a, mpc_msg_count * b,
			     mpc_msg_count * c, MPC_Datatype * d, void *e,
			     mpc_msg_count * f, MPC_Datatype * g, int *h,
			     MPC_Comm * i, int *err);

/*Alltoall */
  void ffunc (mpc_alltoall) (void *a, mpc_msg_count * b, MPC_Datatype * c,
			     void *d, mpc_msg_count * e, MPC_Datatype * f,
			     MPC_Comm * g, int *err);
  void ffunc (mpc_alltoallv) (void *a, mpc_msg_count * b,
			      mpc_msg_count * c, MPC_Datatype * d,
			      void *e, mpc_msg_count * f,
			      mpc_msg_count * g, MPC_Datatype * h,
			      MPC_Comm * i, int *err);

/*Informations */
  void ffunc (mpc_get_processor_name) (char *a, int *b, int *err);

/*Groups */
  void ffunc (mpc_comm_group) (MPC_Comm * a, MPC_Group * b, int *err);
  void ffunc (mpc_group_free) (MPC_Group * a, int *err);
  void ffunc (mpc_group_incl) (MPC_Group * a, int *b, int *c,
			       MPC_Group * d, int *err);
  void ffunc (mpc_group_difference) (MPC_Group * a, MPC_Group * b,
				     MPC_Group * c, int *err);

/*Communicators */
  void ffunc (mpc_comm_create_list) (MPC_Comm * a, int *list,
				     int *nb_elem, MPC_Comm * b, int *err);
  void ffunc (mpc_comm_create) (MPC_Comm * a, MPC_Group * b, MPC_Comm * c,
				int *err);
  void ffunc (mpc_comm_free) (MPC_Comm * a, int *err);
  void ffunc (mpc_comm_dup) (MPC_Comm * a, MPC_Comm * b, int *err);
  void ffunc (mpc_comm_split) (MPC_Comm * a, int *b, int *c, MPC_Comm * d,
			       int *err);

/*Error_handler */
  void ffunc (mpc_default_error) (MPC_Comm * comm, int *error, char *msg,
				  char *file, int *line);
#if 0
  void ffunc (mpc_return_error) (, int *err);;
#endif
  void ffunc (mpc_errhandler_create) (MPC_Handler_function * a,
				      MPC_Errhandler * b, int *err);
  void ffunc (mpc_errhandler_set) (MPC_Comm * a, MPC_Errhandler * b,
				   int *err);
  void ffunc (mpc_errhandler_get) (MPC_Comm * a, MPC_Errhandler * b,
				   int *err);
  void ffunc (mpc_errhandler_free) (MPC_Errhandler * a, int *err);
  void ffunc (mpc_error_string) (int *a, char *b, int *c, int *err);
  void ffunc (mpc_error_class) (int *a, int *b, int *err);

/*Timing */
  double *ffunc (mpc_wtime) (void);
  double *ffunc (mpc_wtick) (void);
/*Types */
  void ffunc (mpc_type_size) (MPC_Datatype * a, size_t * b, int *err);
  void ffunc (mpc_sizeof_datatype) (MPC_Datatype * a, size_t * b, int *err);
  void ffunc (mpc_type_free) (MPC_Datatype * datatype, int *err);

/*Requests */
  void ffunc (mpc_request_free) (MPC_Request * a, int *err);

/*Scheduling */
  void ffunc (mpc_proceed) (int *err);
  void ffunc (mpc_checkpoint) (MPC_Checkpoint_state* st, int *err);
  void ffunc (mpc_checkpoint_timed) (unsigned int *sec, MPC_Comm * comm,
				     int *err);
  void ffunc (mpc_migrate) (int *err);
  void ffunc (mpc_restart) (int *rank, int *err);
  void ffunc (mpc_restarted) (int *flag, int *err);
  void ffunc (mpc_get_activity) (int *nb_item, MPC_Activity_t * tab,
				 double *process_act, int *err);
  void ffunc (mpc_move_to) (int *process, int *cpuid, int *err);

  /*Packs */
  void ffunc (mpc_open_pack) (MPC_Request * request, int *err);
  void ffunc (mpc_default_pack) (mpc_msg_count * count,
				 mpc_pack_indexes_t * begins,
				 mpc_pack_indexes_t * ends,
				 MPC_Request * request, int *err);
  void ffunc (mpc_add_pack) (void *buf, mpc_msg_count * count,
			     mpc_pack_indexes_t * begins,
			     mpc_pack_indexes_t * ends,
			     MPC_Datatype * datatype,
			     MPC_Request * request, int *err);
  void ffunc (mpc_add_pack_absolute) (void *buf, mpc_msg_count * count,
				      mpc_pack_absolute_indexes_t *
				      begins,
				      mpc_pack_absolute_indexes_t * ends,
				      MPC_Datatype * datatype,
				      MPC_Request * request, int *err);
  void ffunc (mpc_add_pack_default) (void *buf, MPC_Datatype datatype,
				     MPC_Request * request, int *err);
  void ffunc (mpc_isend_pack) (int *dest, int *tag, MPC_Comm * comm,
			       MPC_Request * request, int *err);
  void ffunc (mpc_irecv_pack) (int *source, int *tag, MPC_Comm * comm,
			       MPC_Request * request, int *err);

  int sctk_launch_main (int argc, char **argv);
  void ffunc (mpc_start) (void);


/*
  IO
*/
  void ffunc (mpc_io_lock) (void);
  void ffunc (mpc_io_unlock) (void);
  void ffunc (mpc_io_get_unit) (int *unit);
  void ffunc (mpc_io_free_unit) (int *unit);

  /* Other */
  void ffunc (mpc_network_stats) (int *matched, int *not_matched, int *poll_own, int *poll_stolen, int *poll_steals, double *time_stolen, double *time_steals, double *time_own);
#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
