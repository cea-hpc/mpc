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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_INTER_THREAD_COMMUNICATIONS_H_
#define __SCTK_INTER_THREAD_COMMUNICATIONS_H_

#include <sctk_config.h>
#include <sctk_debug.h>
#include <sctk_communicator.h>
#include <sctk_collective_communications.h>
#include <mpcmp.h>

#ifdef __cplusplus
extern "C"
{
#endif

  struct sctk_request_s;

#define SCTK_MESSAGE_PENDING 0
#define SCTK_MESSAGE_DONE 1
#define SCTK_MESSAGE_CANCELED 2

  typedef enum {
    pt2pt_specific_message_tag,
    barrier_specific_message_tag,
    broadcast_specific_message_tag,
    allreduce_specific_message_tag
  }specific_message_tag_t;

  typedef struct sctk_thread_message_header_s
  {
    int source;
    int destination;
    int glob_source;
    int glob_destination;
    sctk_communicator_t communicator;
    int message_tag;
    specific_message_tag_t specific_message_tag;
    size_t msg_size;
  } sctk_thread_message_header_t;

  typedef unsigned int sctk_pack_indexes_t;
  typedef unsigned long sctk_pack_absolute_indexes_t;
  typedef int sctk_count_t;

  typedef struct{
    size_t size;
    void* addr;
  }sctk_message_contiguous_t;

  typedef struct{
    sctk_count_t count;
    sctk_pack_indexes_t *begins;
    sctk_pack_indexes_t *ends;
    void* addr;
    size_t elem_size;
  }sctk_message_pack_std_list_t;

  typedef struct{
    sctk_count_t count;
    sctk_pack_absolute_indexes_t *begins;
    sctk_pack_absolute_indexes_t *ends;
    void* addr;
    size_t elem_size;
  }sctk_message_pack_absolute_list_t;

  typedef union{
    sctk_message_pack_absolute_list_t* absolute;
    sctk_message_pack_std_list_t* std;
  }sctk_message_pack_list_t;

  typedef union{
    sctk_message_pack_absolute_list_t absolute;
    sctk_message_pack_std_list_t std;
  }sctk_message_pack_default_t;

  typedef struct{
    size_t count;
    size_t max_count;
    sctk_message_pack_list_t list;
  }sctk_message_pack_t;

  typedef enum {
    sctk_message_contiguous,
    sctk_message_pack,
    sctk_message_pack_absolute,
    sctk_message_pack_undefined
  } sctk_message_type_t;

  typedef union {
    sctk_message_contiguous_t contiguous;
    sctk_message_pack_t pack;
  } sctk_message_t;

  typedef MPC_Request sctk_request_t;

  struct sctk_thread_ptp_message_s;

  typedef struct sctk_msg_list_s{
    struct sctk_thread_ptp_message_s * msg;
    struct sctk_msg_list_s *prev, *next;
  }sctk_msg_list_t;

typedef struct sctk_message_to_copy_s{
  struct sctk_thread_ptp_message_s * msg_send;
  struct sctk_thread_ptp_message_s * msg_recv;  
  struct sctk_message_to_copy_s * prev, *next;
}sctk_message_to_copy_t;

  typedef struct sctk_thread_ptp_message_s{
    sctk_thread_message_header_t header;
    volatile int* completion_flag; 
    sctk_request_t * request;

    /*Message data*/
    sctk_message_type_t message_type;
    sctk_message_t message;
    sctk_message_pack_default_t default_pack;

    /*Storage structs*/
    sctk_msg_list_t distant_list;
    sctk_message_to_copy_t copy_list;

    /*Destructor*/
    void (*free_memory)(void*);

    /*Copy operator*/
    void (*message_copy)(sctk_message_to_copy_t*);
  }sctk_thread_ptp_message_t;


  /**
     Check if the message if completed according to the message passed as a request
  */
  void sctk_perform_messages(sctk_request_t* request);

  void sctk_init_header (sctk_thread_ptp_message_t *tmp, const int myself,
			 sctk_message_type_t msg_type, void (*free_memory)(void*),
			 void (*message_copy)(sctk_message_to_copy_t*));
  void sctk_reinit_header (sctk_thread_ptp_message_t *tmp, void (*free_memory)(void*),
			   void (*message_copy)(sctk_message_to_copy_t*));
  sctk_thread_ptp_message_t *sctk_create_header (const int myself,sctk_message_type_t msg_type);
  void sctk_add_adress_in_message (sctk_thread_ptp_message_t *
				  restrict msg, void *restrict addr,
				  const size_t size);
  void sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg,
				void *adr, const sctk_count_t nb_items,
				const size_t elem_size,
				sctk_pack_indexes_t * begins,
				sctk_pack_indexes_t * ends);
  void sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t *
					 msg, void *adr,
					 const sctk_count_t nb_items,
					 const size_t elem_size,
					 sctk_pack_absolute_indexes_t *
					 begins,
					 sctk_pack_absolute_indexes_t * ends);
  void sctk_set_header_in_message (sctk_thread_ptp_message_t *
				   msg, const int message_tag,
				   const sctk_communicator_t
				   communicator,
				   const int source,
				   const int destination,
				   sctk_request_t * request,
				   const size_t count,
				   specific_message_tag_t specific_message_tag);
  void sctk_wait_message (sctk_request_t * request);
  void sctk_wait_all (const int task, const sctk_communicator_t com);
  void sctk_probe_source_any_tag (int destination, int source,
				  const sctk_communicator_t comm,
				  int *status,
				  sctk_thread_message_header_t * msg);
  void sctk_probe_any_source_any_tag (int destination,
				      const sctk_communicator_t comm,
				      int *status,
				      sctk_thread_message_header_t * msg);
  void sctk_probe_source_tag (int destination, int source,
			      const sctk_communicator_t comm, int *status,
			      sctk_thread_message_header_t * msg);
  void sctk_probe_any_source_tag (int destination,
				  const sctk_communicator_t comm,
				  int *status,
				  sctk_thread_message_header_t * msg);
  void sctk_send_message (sctk_thread_ptp_message_t * msg);
  void sctk_recv_message (sctk_thread_ptp_message_t * msg);
  void sctk_cancel_message (sctk_request_t * msg);
  void sctk_ptp_per_task_init (int i);
  void sctk_unregister_thread (const int i);

  void sctk_message_copy(sctk_message_to_copy_t* tmp);
  void sctk_message_copy_pack(sctk_message_to_copy_t* tmp);
  void sctk_message_copy_pack_absolute(sctk_message_to_copy_t* tmp);

#ifdef __cplusplus
}
#endif

#endif
