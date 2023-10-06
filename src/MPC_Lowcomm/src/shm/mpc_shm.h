/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:51:04 CEST 2023                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_SHM_H_
#define MPC_SHM_H_

#include <mpc_common_spinlock.h>
#include <mpc_common_datastructure.h>
#include <mpc_mempool.h>
#include <stdint.h>
#include <mpc_lowcomm_monitor.h>
#include <sys/types.h>

#define MPC_SHM_EAGER_SIZE 4200
#define MPC_SHM_BCOPY_SIZE MPC_SHM_EAGER_SIZE
#define SHM_PROCESS_ARITY 2ull
#define SHM_FREELIST_PER_PROC 2ull
#define SHM_CELL_COUNT_PER_PROC 4096ull

/**************
 * AM HEADERS *
 **************/

typedef enum
{
   MPC_SHM_AM_EAGER,
   MPC_SHM_AM_GET,
   MPC_SHM_AM_PUT,
   MPC_SHM_AM_FRAG
}_mpc_shm_am_type_t;

typedef struct
{
   _mpc_shm_am_type_t op;
	uint8_t am_id;
	size_t length;
   char * data[0];
}_mpc_shm_am_hdr_t;


typedef struct
{
   mpc_lowcomm_peer_uid_t source;
   uint64_t frag_id;
   uint64_t segment_id;
   uint64_t offset;
   size_t size;
}_mpc_shm_am_get;

typedef struct
{
   uint64_t frag_id;
   uint64_t segment_id;
   size_t size;
   uint64_t offset;
   char data[0];
}_mpc_shm_frag;

/**************
 * CELL LISTS *
 **************/

struct _mpc_shm_cell
{
   char data[MPC_SHM_EAGER_SIZE];
   struct _mpc_shm_cell * next;
   struct _mpc_shm_cell * prev;
   unsigned int free_list;
};

struct _mpc_shm_list_head
{
   mpc_common_spinlock_t lock;
   struct _mpc_shm_cell * head;
   struct _mpc_shm_cell * tail;
   char __pad[512];
};

typedef enum
{
   MPC_SHM_CMA_NOK = 0,
   MPC_SHM_CMA_OK = 1,
   MPC_SHM_CMA_UNCHECKED = 2   
}mpc_shm_cma_state_t;


struct _mpc_shm_storage
{
   struct _mpc_shm_list_head * per_process;
   struct _mpc_shm_list_head * free_lists;
   mpc_lowcomm_peer_uid_t ** remote_uid_addresses;
   pid_t * pids;
   mpc_lowcomm_peer_uid_t * uids;
   unsigned int process_count;
   unsigned int process_arity;
   unsigned int freelist_count;
   unsigned int cell_count;
   void * shm_buffer;
   size_t segment_size;
   mpc_lowcomm_peer_uid_t my_uid;
   mpc_shm_cma_state_t cma_state;
};

/**************
 * FRAGMENTED *
 **************/

struct _mpc_shm_region
{
   uint64_t id;
   void * addr;
   size_t size;
};

struct _mpc_shm_fragment
{
   mpc_common_spinlock_t lock;
   uint64_t id;
   uint64_t segment_id;
   void * base_addr;
   void * arg;
   void (*completion)(void * arg);
   size_t sizeleft;
};

struct _mpc_shm_fragment_factory
{
   mpc_common_spinlock_t lock;
   uint64_t fragment_id;
   uint64_t segment_id;
   struct mpc_common_hashtable frags;
   mpc_mempool frag_pool;
   struct mpc_common_hashtable regs;
   mpc_mempool reg_pool;
};

/*******************
 * PINNING CONTEXT *
 *******************/

typedef struct
{
   uint64_t id;
   void * addr;
}_mpc_lowcomm_shm_pinning_ctx_t;

/************************
 * SHM ENDPOINT CONTEXT *
 ************************/

typedef struct _mpc_lowcomm_shm_endpoint_info_s
{
   pid_t cma_pid;
   unsigned int local_rank;
}_mpc_lowcomm_shm_endpoint_info_t;

void _mpc_lowcomm_shm_endpoint_info_init(_mpc_lowcomm_shm_endpoint_info_t * infos, mpc_lowcomm_peer_uid_t uid, struct _mpc_shm_storage *storage);

/********************
 * SHM RAIL CONTEXT *
 ********************/

typedef struct{
   struct _mpc_shm_storage storage;
   struct _mpc_shm_fragment_factory frag_factory;
}_mpc_lowcomm_shm_rail_info_t;


#endif /* MPC_SHM_H_ */
