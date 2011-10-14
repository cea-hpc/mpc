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

#include <sctk_inter_thread_comm.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>

void sctk_perform_messages(sctk_request_t* request){not_implemented();}
sctk_thread_ptp_message_t
* sctk_add_adress_in_message (sctk_thread_ptp_message_t *
			      restrict msg, void *restrict adr,
			      const size_t size){not_implemented();}
sctk_thread_ptp_message_t
* sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg,
			    void *adr, const sctk_count_t nb_items,
			    const size_t elem_size,
			    sctk_pack_indexes_t * begins,
			    sctk_pack_indexes_t * ends){not_implemented();}
sctk_thread_ptp_message_t
* sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t *
				     msg, void *adr,
				     const sctk_count_t nb_items,
				     const size_t elem_size,
				     sctk_pack_absolute_indexes_t *
				     begins,
				     sctk_pack_absolute_indexes_t * ends){not_implemented();}
void sctk_set_header_in_message (sctk_thread_ptp_message_t *
				 msg, const int message_tag,
				 const sctk_communicator_t
				 communicator,
				 const int source,
				 const int destination,
				 sctk_request_t * request,
				 const size_t count){not_implemented();}
void sctk_wait_message (sctk_request_t * request){not_implemented();}
void sctk_wait_all (const int task, const sctk_communicator_t com){not_implemented();}
void sctk_probe_source_any_tag (int destination, int source,
				const sctk_communicator_t comm,
				int *status,
				sctk_thread_message_header_t * msg){not_implemented();}
void sctk_probe_any_source_any_tag (int destination,
				    const sctk_communicator_t comm,
				    int *status,
				    sctk_thread_message_header_t * msg){not_implemented();}
void sctk_probe_source_tag (int destination, int source,
			    const sctk_communicator_t comm, int *status,
			    sctk_thread_message_header_t * msg){not_implemented();}
void sctk_probe_any_source_tag (int destination,
				const sctk_communicator_t comm,
				int *status,
				sctk_thread_message_header_t * msg){not_implemented();}
  void sctk_send_message (sctk_thread_ptp_message_t * msg){not_implemented();}
  void sctk_recv_message (sctk_thread_ptp_message_t * msg){not_implemented();}

  void sctk_cancel_message (sctk_request_t * msg){not_implemented();}

typedef struct{
  sctk_communicator_t comm;
  int destination;
}sctk_comm_dest_key_t;

typedef struct sctk_any_source_s{
  sctk_thread_ptp_message_t * msg;
  struct sctk_any_source_s *prev, *next;
}sctk_any_source_t;

typedef struct{
  sctk_comm_dest_key_t key;

  sctk_spinlock_t lock;
  /*list used for any source matching*/
  sctk_any_source_t any_source_list;
  /*Table used when source is defined*/
/*   sctk_defined_source_t define_source_list; */

  UT_hash_handle hh;
} sctk_internal_ptp_t;

static sctk_internal_ptp_t* sctk_ptp_table = NULL;
static sctk_spin_rwlock_t sctk_ptp_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/*Init data structures used for task i*/
void sctk_ptp_per_task_init (int i){
  sctk_internal_ptp_t * tmp;
  tmp = sctk_malloc(sizeof(sctk_internal_ptp_t));
  memset(tmp,0,sizeof(sctk_internal_ptp_t));
  tmp->key.comm = SCTK_COMM_WORLD;
  tmp->key.destination = i;

  sctk_spinlock_write_lock(&sctk_ptp_table_lock);
  HASH_ADD(hh,sctk_ptp_table,key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_ptp_table_lock);
}

void sctk_unregister_thread (const int i){
  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks--;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);  
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/


sctk_thread_ptp_message_t *sctk_create_header (const int myself){
  sctk_thread_ptp_message_t * tmp;
  
#warning "Optimize allocation"
  tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t));

  /*Init message struct*/
  memset(tmp,0,sizeof(sctk_thread_ptp_message_t));
  

  return tmp;
}
