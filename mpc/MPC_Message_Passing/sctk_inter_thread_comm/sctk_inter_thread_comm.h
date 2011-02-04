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
/*  #   - DIDELOT Sylvain didelot.sylvain@gmail.com                       # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_INTER_THREAD_COMMUNICATIONS_H_
#define __SCTK_INTER_THREAD_COMMUNICATIONS_H_

#include <stdio.h>
#include <string.h>

#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_communicator.h"
#include "sctk_thread.h"
#include "sctk_alloc.h"


#ifndef NO_INTERNAL_ASSERT
#define USE_OWNER_FLAG
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct{
    sctk_alloc_thread_data_t *alloc;
    sctk_spinlock_t lock;
  } sctk_messages_alloc_thread_data_t;

  typedef unsigned int sctk_pack_indexes_t;
  typedef unsigned long sctk_pack_absolute_indexes_t;

  typedef int sctk_count_t;

  typedef unsigned long sctk_message_numbering;
  struct sctk_request_s;

#define SCTK_INIT_NUMBER_OF_WAITTING_MESSAGES 20

  typedef struct sctk_thread_message_header_s
  {
    int message_tag;
    sctk_communicator_t communicator;

    int source;
    int local_source;
    int destination;

    int myself;
    int volatile *  request;
    size_t volatile *req_msg_size;
    long rank;
    unsigned long rank_recv;
    size_t msg_size;
  } sctk_thread_message_header_t;
#define SCTK_INIT_HEADER_MESSAGE {\
    0,0,			  \
      0,0,0,			  \
      0,NULL,NULL,0,0,0}

  typedef struct sctk_thread_message_description_s
  {
    size_t nb_items;
    size_t nb_items_max;
    size_t message_size;

    size_t max_count;

    sctk_pack_indexes_t **begins;
    sctk_pack_indexes_t **ends;

    sctk_pack_absolute_indexes_t **begins_absolute;
    sctk_pack_absolute_indexes_t **ends_absolute;

    size_t *sizes;
    size_t *elem_sizes;

    void **adresses;
    /*Part for simples message */
  } sctk_thread_message_description_t;

  struct sctk_task_ptp_data_s;

  typedef struct sctk_thread_ptp_message_s
  {
    volatile struct sctk_thread_ptp_message_s *next;
    volatile struct sctk_thread_ptp_message_s *wait_next;
    volatile struct sctk_thread_ptp_message_s *pair;

    sctk_messages_alloc_thread_data_t* allocator_p;

    sctk_thread_message_description_t message;
    volatile int completion_flag;
    struct sctk_task_ptp_data_s *data;

    sctk_thread_message_header_t header;
    void *net_mesg;
    void *addr_to_free;
    int process_src;
    struct sctk_request_s *request;
    /* if SHM has been used to send ths msg */
    int sent_by_shm;
    /* TODO : surround by macro */
    /* additional information for infiniband */
    int channel_type;
    void* struct_ptr;
  } sctk_thread_ptp_message_t;


  typedef struct
  {
    volatile sctk_thread_ptp_message_t *head;
    volatile sctk_thread_ptp_message_t *tail;
    sctk_spinlock_t lock;
#ifdef USE_OWNER_FLAG
    volatile sctk_thread_t owner;
#endif
  } sctk_thread_ptp_message_list_t;

  typedef struct sctk_task_ptp_data_s
  {
    sctk_spinlock_t spinlock;
    int source_task;

    volatile int busy;
    volatile sctk_thread_ptp_message_t* matched;
    sctk_spinlock_t spinlock_matched;

    /*Cyrcular table */
    sctk_thread_ptp_message_list_t sctk_send_list;
    volatile long rank_send;

    /*Message list */
    sctk_thread_ptp_message_list_t sctk_recv_list;
    volatile long rank_recv;

    /*Equal 1 if the task is here and 0 if not */
    volatile int is_usable;
  } sctk_task_ptp_data_t;

  typedef struct sctk_per_communicator_ptp_data_s
  {
    sctk_task_ptp_data_t *tasks;
    int nb_tasks;

    sctk_thread_ptp_message_list_t sctk_wait_list;

    sctk_thread_ptp_message_list_t sctk_wait_send_list;

    sctk_thread_ptp_message_list_t sctk_any_source_list;
    unsigned long rank_recv;

  } sctk_per_communicator_ptp_data_t;

  typedef struct
  {
    sctk_spinlock_t lock;
    volatile sctk_thread_ptp_message_t *free_list;
  } sctk_thread_ptp_message_free_list_t;

  typedef struct sctk_ptp_data_s
  {
    sctk_per_communicator_ptp_data_t
      communicators[SCTK_MAX_COMMUNICATOR_NUMBER];

    sctk_messages_alloc_thread_data_t allocator;

    sctk_thread_ptp_message_free_list_t free_list;
    /*Equal 1 if the task is here and 0 if not */
    volatile int is_usable;
  } sctk_ptp_data_t;

  static inline int sctk_ptp_message_equal (const
					    sctk_thread_ptp_message_t * a,
					    const sctk_thread_ptp_message_t *
					    b)
  {
    sctk_nodebug ("check dest %d and src %d", a->header.message_tag,
		  b->header.message_tag);
    return a->header.message_tag == b->header.message_tag;
  }
  void sctk_ptp_init (const int nb_task);
  void sctk_ptp_delete (void);

  typedef struct
  {
    sctk_count_t count;
    sctk_pack_indexes_t *begins;
    sctk_pack_indexes_t *ends;
    sctk_pack_absolute_indexes_t *begins_absolute;
    sctk_pack_absolute_indexes_t *ends_absolute;
  } sctk_default_pack_t;

  typedef struct sctk_request_s
  {
    volatile int completion_flag;
    sctk_thread_ptp_message_t *msg;
    sctk_thread_message_header_t header;
    volatile size_t msg_size;
    sctk_default_pack_t default_pack;
    int is_null;
    void *ptr;
  } sctk_request_t;

  static inline int sctk_translate_to_global_rank_local (const int i, const
							 sctk_communicator_t
							 communicator)
  {
    int res;
    sctk_internal_communicator_t *tmp;

    if (communicator == 0)
      {
	return i;
      }
    if(i == -1) {
      return i;
    }

    tmp = sctk_get_communicator (communicator);
    sctk_nodebug ("tmp->global_in_communicator %p",
		  tmp->global_in_communicator_local);
    if (tmp->global_in_communicator_local != NULL)
      {
	res = tmp->global_in_communicator_local[i];
      }
    else
      res = i;
    sctk_nodebug ("translate to global %d->%d, %d comm", i, res,
		  communicator);
    assert (res >= 0);
    return res;
  }
  static inline int sctk_translate_to_global_rank_remote (const int i, const
							  sctk_communicator_t
							  communicator)
  {
    int res;
    sctk_internal_communicator_t *tmp;

    if (communicator == 0)
      {
	return i;
      }
    if(i == -1) {
      return i;
    }

    tmp = sctk_get_communicator (communicator);
    sctk_nodebug ("tmp->global_in_communicator %p",
		  tmp->global_in_communicator_remote);
    if (tmp->global_in_communicator_remote != NULL)
      {
	res = tmp->global_in_communicator_remote[i];
      }
    else
      res = i;
    sctk_nodebug ("translate to global %d->%d, %d comm", i, res,
		  communicator);
    assert (res >= 0);
    return res;
  }

  static inline void sctk_set_header_in_message (sctk_thread_ptp_message_t *
						 msg, const int message_tag,
						 const sctk_communicator_t
						 communicator,
						 const int source,
						 const int destination,
						 sctk_request_t * request,
						 const size_t count)
  {
    msg->header.msg_size = count;
    msg->header.rank = 0;
    msg->header.message_tag = message_tag;
    msg->header.communicator = communicator;
    msg->header.local_source = source;

    msg->header.source =
      sctk_translate_to_global_rank_local (source, communicator);
    msg->header.destination =
      sctk_translate_to_global_rank_remote (destination, communicator);

    msg->completion_flag = 0;
    msg->data = NULL;
    msg->header.myself =
      sctk_translate_to_global_rank_local (msg->header.myself, communicator);

    msg->request = request;

    if (request != NULL)
      {
	/* Prepare link in header for request*/
	msg->header.req_msg_size = &(request->msg_size);
	msg->header.request = &(request->completion_flag);
	request->header = msg->header;
	request->msg = msg;
	request->completion_flag = 0;
	request->msg_size = count;
	request->is_null = 0;
      }
    else
      {
	msg->header.request = NULL;
      }
    msg->net_mesg = NULL;

    sctk_nodebug ("message from %d to %d created on %d",
		  source, destination, msg->header.myself);
  }
  sctk_thread_ptp_message_t *sctk_create_header (const int myself);
  sctk_thread_ptp_message_t
    * sctk_add_adress_in_message (sctk_thread_ptp_message_t *
				  restrict msg, void *restrict adr,
				  const size_t size);
  sctk_thread_ptp_message_t
    * sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg,
				void *adr, const sctk_count_t nb_items,
				const size_t elem_size,
				sctk_pack_indexes_t * begins,
				sctk_pack_indexes_t * ends);
  sctk_thread_ptp_message_t
    * sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t *
					 msg, void *adr,
					 const sctk_count_t nb_items,
					 const size_t elem_size,
					 sctk_pack_absolute_indexes_t *
					 begins,
					 sctk_pack_absolute_indexes_t * ends);

  int sctk_is_net_message (int dest);
  void sctk_send_message (sctk_thread_ptp_message_t * msg);
  void sctk_recv_message (sctk_thread_ptp_message_t * msg);

  void sctk_register_thread_initial (const int i);
  void sctk_register_thread (const int i);
  void sctk_unregister_thread (const int i);
  void sctk_register_distant_thread (const int i, const int pos);
  void sctk_register_restart_thread (const int i, const int pos);
  void sctk_unregister_distant_thread (const int i);

/*  void sctk_check_for_communicator(const int task,const sctk_communicator_t comm);*/
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
  void sctk_perform_messages (const int task, const sctk_communicator_t com);
  void sctk_ptp_per_task_init (int i);
  void sctk_check_messages (int destination, int source,
			    sctk_communicator_t communicator);
  void sctk_cancel_message (sctk_request_t * msg);

  static inline
    void sctk_init_thread_ptp_message (sctk_thread_ptp_message_t * tmp,
				       const int myself)
  {
    tmp->next = NULL;
    tmp->wait_next = NULL;
    /*tmp->tls */

    /*tmp->message */
    tmp->message.nb_items = 0;
    tmp->message.max_count = 0;
    tmp->message.message_size = 0;
    tmp->completion_flag = 0;
    /*tmp->data */

    tmp->header.myself = myself;
    tmp->request = NULL;
  }

  int sctk_get_ptp_process_localisation(int i);


#define SCTK_MESSAGE_UNIT_NUMBER 10
#define SCTK_MESSAGE_UNIT_SIZE(this_size)				\
  (sctk_aligned_sizeof(sctk_thread_ptp_message_t) +			\
   sctk_aligned_size((this_size)*sizeof(sctk_pack_indexes_t*)) +	\
   sctk_aligned_size((this_size)*sizeof(sctk_pack_indexes_t*)) +	\
   sctk_aligned_size((this_size)*sizeof(sctk_pack_absolute_indexes_t*)) + \
   sctk_aligned_size((this_size)*sizeof(sctk_pack_absolute_indexes_t*)) + \
   sctk_aligned_size((this_size)*sizeof(size_t)) +			\
   sctk_aligned_size((this_size)*sizeof(size_t)) +			\
   sctk_aligned_size((this_size)*sizeof(void*)))

#ifdef __cplusplus
}
#endif
#endif
