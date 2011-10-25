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
/* #   - PERACHE Marc     marc.perache@cea.fr                             # */
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                 # */
/* #                                                                      # */
/* ######################################################################## */


#ifndef __SCTK__SHM_MEM_STRUCT_H_
#define __SCTK__SHM_MEM_STRUCT_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "sctk_spinlock.h"
#include "sctk_asm.h"
#include <string.h>
#include <assert.h>
#include <time.h>
#include <mpc.h>
#include <math.h>

#include "sctk_shm_consts.h"
#include "sctk_collective_communications.h"
#include "sctk_net_tools.h"
#include "sctk_shm_lib.h"

#define HEADER_QUEUE \
	/* queue type */ \
	int queue_type; \
	/* queue size */\
	int queue_size;\
	/* lock on first slot */\
	sctk_spinlock_t lock_read;\
	/* ptr to queue's first and last slots */\
	int ptr_msg_last_slot;\
	int ptr_msg_first_slot;\
	/* lock on writers */\
	sctk_spinlock_t lock_write;\


/*-----------------------------------------------------------
 *  SHM - Structures definitions
 *----------------------------------------------------------*/
struct sctk_shm_mem_struct_s;
struct sctk_shm_ptp_queue_s;
struct sctk_shm_fastmsg_queue_s;
struct sctk_shm_bigmsg_slot_s;
struct sctk_shm_rpc_slot_s;
struct sctk_shm_broadcast_slot_s;
struct sctk_shm_com_list_s;
struct sctk_shm_barrier_slot_s;
struct sctk_shm_fastmsg_broadcast_slot_s;
struct sctk_shm_bigmsg_broadcast_slot_s;


/*------------------------------------------------------
 *  Structure which is returned when a new msg is gotten
 *-----------------------------------------------------*/
struct sctk_shm_ret_get_ptp_s {
	/* pointer to queue */
	void *queue;
	/* ptr to slot selected */
	void *selected_slot;
	/* ptr to the payload of the slot */
	void **selected_slot_content;
	/* type of queue */
	int queue_type;
	/* type of msg */
	int msg_type;
};

/*-----------------------------------------------------------
 *  SHM - REDUCE slot
 *----------------------------------------------------------*/
struct sctk_shm_reduce_slot_s {
	uint64_t msg_size[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
	char msg_content[SCTK_SHM_MAX_NB_LOCAL_PROCESSES][SCTK_SHM_FASTMSG_REDUCE_MAXLEN];
	int shm_msg_type;
	int shm_is_msg_ready[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
	int shm_is_broadcast_ready;
	sctk_spinlock_t lock;
};



/*------------------------------------------------------
 *  SHM - BARRIER slot
 *-----------------------------------------------------*/
struct sctk_shm_barrier_slot_s {
	sctk_collective_communications_t *com;
	unsigned int rank[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
	unsigned int lock;
	sctk_spinlock_t spin_done;
	char dummy[SCTK_SHM_PADDING];
	sctk_spinlock_t spin_total;
	unsigned int total;
	unsigned int done[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
};



/*-----------------------------------------------------------
 * SHM - BROADCAST queues
 *----------------------------------------------------------*/
struct sctk_shm_bigmsg_broadcast_slot_s {
	uint64_t slot_number;
	char msg_content[SCTK_SHM_BIGMSG_BROADCAST_MAXLEN];
	size_t msg_size;
	int ptr_to_another_slot;
	int msg_remaining;
	int is_msg_ready_to_read;
	unsigned int current_elements;
	unsigned int total_elements;
	sctk_spinlock_t shm_lock_current_elements;
};


/*-----------------------------------------------------------
 *  SHM - BROADCAST slot
 *----------------------------------------------------------*/
struct sctk_shm_broadcast_slot_s {
	uint64_t msg_size;
	char msg_content[SCTK_SHM_FASTMSG_BROADCAST_MAXLEN];
	unsigned int rank[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
	int shm_msg_type;
	int shm_is_msg_ready;
};



/*-----------------------------------------------------------
 *  SHM - COMMUNICATORS structures
 *----------------------------------------------------------*/
struct sctk_shm_com_list_s {
	/* id of the communicator registered */
	int com_id;
	/* nb of tasks involved in the communicator */
	int nb_task_involved;
	/* nb of processes involved in the communicator */
	int nb_process_involved;
	/* list of tasks involved in the communicator */
	int involved_task_list[SCTK_SHM_MAX_NB_TOTAL_PROCESSES];
	/* processus matched for a specified task */
	int match_task_process[SCTK_SHM_MAX_NB_TOTAL_PROCESSES];
	/* list of prcesses */
	int involved_process_list[SCTK_SHM_MAX_NB_TOTAL_PROCESSES];

	/* slots for broadcast, barrier and reduce */
	struct sctk_shm_broadcast_slot_s shm_broadcast_slot;
	struct sctk_shm_barrier_slot_s shm_barrier_slot;
	struct sctk_shm_reduce_slot_s shm_reduce_slot;

	/* reduce bigmsg_queue */
	struct sctk_shm_bigmsg_queue_s *broadcast_bigmsg_queue;
	struct sctk_shm_bigmsg_queue_s *reduce_bigmsg_queue;
};



/*-----------------------------------------------------------
 *  SHM - Main structures
 *----------------------------------------------------------*/
struct sctk_shm_mem_struct_s {
	struct sctk_shm_ptp_queue_s *shm_ptp_queue[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];

	/* communicator lock */
	sctk_spinlock_t com_list_lock;
	/* list of communicators */
	struct sctk_shm_com_list_s sctk_shm_com_list[SCTK_SHM_MAX_COMMUNICATORS];

	/* translation tables */
	int shm_local_to_global_translation_table[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
	int shm_global_to_local_translation_table[SCTK_SHM_MAX_NB_TOTAL_PROCESSES];

	sctk_spinlock_t translation_table_lock;
	int ptr_translation_table;
};

struct sctk_shm_ptp_queue_s {
	/* 0=DATA, 1=RPC, 2=RDMA */
	struct sctk_shm_fastmsg_queue_s *shm_fastmsg_queue[3];
	struct sctk_shm_bigmsg_queue_s *shm_bigmsg_queue;
	struct sctk_shm_bigmsg_queue_s* dma_bigmsg_queue;
};



/*-----------------------------------------------------------
 *  SHM - DATA queues
 *----------------------------------------------------------*/

/* generic structure for queues */
struct sctk_shm_generic_queue_s {
	HEADER_QUEUE;
	int next_slots[];
};




/* fast msg queue structure */
struct sctk_shm_fastmsg_queue_s {
	HEADER_QUEUE;
	int next_slots[SCTK_SHM_MAX_SLOTS_FASTMSG_QUEUE];

	void *msg_slot[SCTK_SHM_MAX_SLOTS_FASTMSG_QUEUE];

	/* flags and lock when a new msg is received */
	volatile unsigned int flag_new_msg_received;
	volatile unsigned int count_msg_received;
	sctk_spinlock_t lock_count_msg_received;
};

/* big msg queue structure */
struct sctk_shm_bigmsg_queue_s {
	HEADER_QUEUE;
	int next_slots[SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE];

	void *msg_slot[SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE];
};


/* reduce big msg queue structure */
struct sctk_shm_bigmsg_reduce_queue_s {
	HEADER_QUEUE;
	int next_slots[SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE];

	void *msg_slot[SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE];
};


struct sctk_shm_fastmsg_slot_s {
	uint64_t msg_size;
	uint64_t slot_number;
	char msg_content[SCTK_SHM_FASTMSG_DATA_MAXLEN];
	unsigned char is_msg_ready_to_read;
	unsigned int shm_msg_type;
};

struct sctk_shm_bigmsg_header_s {
	int nb_slots_allocated;

	/* size of the msg */
	size_t size;

	/* if msg to be extracted are remaining */
	int remaining;

	/* if the msg is ready to be read */
	int ready;
}
#if (defined(__GNUC__))
__attribute__ ( ( aligned ( 16 ) ) )
#endif
;


struct sctk_shm_bigmsg_reduce_slot_s {
	uint64_t slot_number;
	char msg_content[SCTK_SHM_BIGMSG_REDUCE_MAXLEN];
	uint64_t shm_bigmsg_slot_size;
	size_t msg_size;
	int msg_remaining;
};


/*-----------------------------------------------------------
 *  SHM - RPC slot
 *----------------------------------------------------------*/

/* RPC message */
struct sctk_shm_rpc_s {
	void ( *shm_rpc_function ) ( void * );
	char shm_rpc_args[SCTK_SHM_RPC_ARGS_MAXLEN];
	size_t shm_rpc_size_args;
};


/* standard RPC slot */
struct sctk_shm_rpc_slot_s {
	uint64_t msg_src;
	uint64_t msg_size;
	uint64_t slot_number;
	char msg_content[SCTK_SHM_RPC_MAXLEN];
	unsigned int shm_msg_type;
	int is_rpc_acked;
};


/*------------------------------------------------------
 *  SHM - RDMA
 *-----------------------------------------------------*/
/* fastmsg RDMA slot */
struct sctk_shm_fastmsg_dma_slot_s {
	uint64_t msg_src;
	uint64_t msg_size;
	uint64_t slot_number;
	char msg_content[SCTK_SHM_FASTMSG_RDMA_MAXLEN];
	unsigned int shm_msg_type;
};

struct sctk_shm_bigmsg_dma_slot_s {
	uint64_t msg_src;
	uint64_t msg_size;
	uint64_t slot_number;
	char msg_content[SCTK_SHM_BIGMSG_RDMA_MAXLEN];
	unsigned int shm_msg_type;
};


struct sctk_shm_rdma_read_s {
	/* addr of the variable to read */
	void *rdma_addr;
	/*  size of the variable to read */
	size_t rdma_size;
	/* rank of the src process */
	int src_rank;
};

struct sctk_shm_rdma_write_s {
	/* addr of the variable to read */
	void *rdma_addr;
	int rdma_msg;
	/* size of the variable to read */
	size_t rdma_size;
};


/*-----------------------------------------------------------
 *  SHM - Functions definitions
 *----------------------------------------------------------*/

/* select a communicator structure */
struct sctk_shm_com_list_s
			*sctk_shm_select_right_com_list ( const sctk_collective_communications_t * __com );

/* return the size allocated in the SHM */
size_t sctk_shm_get_allocated_mem_size ();

/* function which inits MPC_COM_WORLD */
struct sctk_shm_com_list_s *sctk_shm_init_world_com ( int init );

/* Init collective structures */
void sctk_shm_init_collective_structs ( int init );

void sctk_shm_init_new_com ( const sctk_internal_communicator_t * __com,
                             const int nb_involved, const int *task_list );

void sctk_shm_free_com ( int com_id );

struct sctk_shm_mem_struct_s *sctk_shm_get_memstruct_base ();

/* init the shared memory structure */
struct sctk_shm_mem_struct_s *sctk_shm_init_mem_struct ( void *__ptr_shm_base, void* __malloc_base );

/* put a ptp msg in a queue */
inline void sctk_shm_put_ptp_msg ( const unsigned int __dest,
                                   void *__payload,
                                   const size_t __size_header,
                                   const size_t __size_payload );

/* put a new message in the fast msg queue */
inline void *sctk_shm_put_fastmsg ( const unsigned int __dest,
                                    const void *__shm_msg,
                                    const size_t __size, const int ptr_to_queue_type );

/* put a broadcast msg */
inline void
sctk_shm_put_broadcast_msg ( const void *__shm_msg,
                             const size_t __size,
                             struct sctk_shm_com_list_s *__com_list,
                             const int is_reduce );

/* get a message from the fast msg queue */
inline int sctk_shm_get_ptp_msg ( unsigned int __rank,
                                  void *__msg_content,
                                  size_t __size,
                                  size_t * __message_size,
                                  int __queue_type,
                                  struct sctk_shm_ret_get_ptp_s *__ret );

/* get a broadcast message (broadcast or reduce) */
inline void sctk_shm_get_broadcast_msg ( unsigned int __rank,
  void *__msg_content,
  size_t __size,
  struct sctk_shm_com_list_s
  *__com_list );

/* put a reduce msg in the queue */
inline void
sctk_shm_put_reduce_msg ( const unsigned int __dest,
                          const void *__shm_msg,
                          const size_t __size,
                          struct sctk_shm_com_list_s *__com_list );

/* get a reduce msg from a queue */
inline void
sctk_get_reduce_msg ( const unsigned int __dest,
                      const size_t __size,
                      void *__shm_msg, struct sctk_shm_com_list_s *__com_list );

inline void *sctk_shm_get_ptp_fastmsg ( const unsigned int __rank, const int __mode );

inline void *
sctk_shm_put_dma_bigmsg (
 const int __dest,
 const void *__shm_msg,
 const size_t __size,
 int* ack );

void *
sctk_shm_get_dma_bigmsg ( const unsigned int __rank,
                          void *__msg_content,
                          const size_t __size );


#endif //__SCTK__SHM_MEM_STRUCT_H_
