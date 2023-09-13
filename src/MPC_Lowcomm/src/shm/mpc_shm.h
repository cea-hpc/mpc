#ifndef MPC_SHM_H_
#define MPC_SHM_H_

#include <mpc_common_spinlock.h>
#include <mpc_common_datastructure.h>
#include <mpc_mempool.h>
#include <stdint.h>
#include <mpc_lowcomm_monitor.h>
#include <sys/types.h>

#define MPC_SHM_EAGER_SIZE 8300

#define MPC_SHM_BCOPY_SIZE MPC_SHM_EAGER_SIZE
#define SHM_PROCESS_ARITY 16ull
#define SHM_FREELIST_PER_PROC 4ull
#define SHM_CELL_COUNT 4096

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
   unsigned char free_list;
};

struct _mpc_shm_list_head
{
   mpc_common_spinlock_t lock;
   struct _mpc_shm_cell * head;
   struct _mpc_shm_cell * tail;
   char __pad[512];
};

struct _mpc_shm_storage
{
   struct _mpc_shm_list_head * per_process;
   struct _mpc_shm_list_head * free_lists;
   unsigned int process_count;
   unsigned int process_arity;
   unsigned int freelist_count;
   unsigned int cell_count;
   void * shm_buffer;
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


/************************
 * SHM ENDPOINT CONTEXT *
 ************************/

typedef struct _mpc_lowcomm_shm_endpoint_info_s
{
   //mpc_mempool zcopy;
}_mpc_lowcomm_shm_endpoint_info_t;

/********************
 * SHM RAIL CONTEXT *
 ********************/

typedef struct{
   struct _mpc_shm_storage storage;
   struct _mpc_shm_fragment_factory frag_factory;
}_mpc_lowcomm_shm_rail_info_t;


#endif /* MPC_SHM_H_ */