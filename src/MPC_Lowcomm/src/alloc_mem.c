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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "alloc_mem.h"

#include "lowcomm_config.h"

#include <mpc_common_rank.h>

#include <sctk_alloc.h>

#include "mpc_common_spinlock.h"
#include "mpc_lowcomm.h"
#include <mpc_common_helper.h>
#include <mpc_common_rank.h>
#include <mpc_common_debug.h>
#include <mpc_common_flags.h>
#include <mpc_launch_shm.h>
#include <stdint.h>


#ifdef MPC_Threads
#include "mpc_thread.h"
#include "thread.h"
#endif

#include "string.h"

struct mpc_lowcomm_allocmem_pool __mpc_lowcomm_memory_pool;

static int _pool_init_done  = 0;
static int _pool_only_local = 0;
static mpc_common_spinlock_t _pool_init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

static size_t _forced_pool_size = 0;

static void __pool_size_adapt(char *exename)
{
	if(!exename)
	{
		return;
	}

	if(_forced_pool_size != 0)
	{
		return;
	}


	struct _mpc_lowcomm_config_mem_pool *mempool_conf = &(_mpc_lowcomm_conf_get()->memorypool);

	int is_linear = mempool_conf->force_process_linear;


	if(mempool_conf->autodetect)
	{
		if(strstr(exename, "IMB"))
		{
			is_linear = 1;
		}
	}

	if(is_linear)
	{
		_forced_pool_size = mempool_conf->per_proc_size *
		                    mpc_common_get_local_process_count();
		mpc_common_debug("Info : setting MPI_Alloc_mem pool size to %d MB",
		                 _forced_pool_size / (1024 * 1024) );
	}
}

static size_t __get_pool_size()
{
	if(_forced_pool_size != 0)
	{
		return _forced_pool_size;
	}

	return _mpc_lowcomm_conf_get()->memorypool.size;
}

static inline int __barrier(void *pcomm)
{
	mpc_lowcomm_communicator_t comm = (mpc_lowcomm_communicator_t)pcomm;

	mpc_lowcomm_barrier(comm);
	return 0;
}

static inline int __rank(void *pcomm)
{
	mpc_lowcomm_communicator_t comm = (mpc_lowcomm_communicator_t)pcomm;
	int ret;

	return mpc_lowcomm_communicator_rank(comm);
}

static inline int __bcast(void *buff, size_t len, void *pcomm)
{
	mpc_lowcomm_communicator_t comm = (mpc_lowcomm_communicator_t)pcomm;

	mpc_lowcomm_bcast(buff, len, 0, comm);
	return 0;
}

void alloc_workshare(mpc_lowcomm_communicator_t comm)
{
	int            nb_tasks = mpc_common_get_local_task_count();
	mpc_workshare *workshare, *tmp;
	void *         addr;

	if(mpc_common_get_local_task_rank() == 0)
	{
		tmp  = sctk_malloc(sizeof(mpc_workshare) * nb_tasks);
		addr = tmp;
	}

	mpc_lowcomm_bcast(&addr, sizeof(void *), 0, comm);

	workshare = (mpc_workshare *)addr;
#ifdef MPC_Threads
	mpc_thread_data_get()->workshare = workshare;
#endif

	if(mpc_common_get_local_task_rank() == 0)
	{
		mpc_lowcomm_workshare_init_func_pointers();
		int i;
		for(i = 0; i < nb_tasks; i++)
		{
			OPA_store_int(&(workshare[i].is_last_iter), 1);
			mpc_common_spinlock_init(&workshare[i].lock, 0);
			mpc_common_spinlock_init(&workshare[i].lock2, 0);
			mpc_common_spinlock_init(&workshare[i].atomic_lock, 0);
			mpc_common_spinlock_init(&workshare[i].critical_lock, 0);
			workshare[i].lb = 0;
			workshare[i].ub = 0;
			OPA_store_int(&(workshare[i].nb_threads_stealing), 0);
		}
		for(i = 0; i < nb_tasks; i++)
		{
			workshare[i].is_allowed_to_steal = 1;
		}
	}
	mpc_lowcomm_barrier(comm);
}

/* This is the accumulate pool protection */

static mpc_common_spinlock_t *__accululate_master_lock = NULL;
static mpc_common_spinlock_t  __static_lock_for_acc    = MPC_COMMON_SPINLOCK_INITIALIZER;

static void __accumulate_op_lock_init_shared()
{
	__accululate_master_lock = &__static_lock_for_acc;
}

static void __accumulate_op_lock_init(mpc_lowcomm_communicator_t per_node_comm)
{
	int my_node_level_rank, my_node_size;

	my_node_level_rank = mpc_lowcomm_communicator_rank(per_node_comm);
	my_node_size       = mpc_lowcomm_communicator_size(per_node_comm);

	void * p        = NULL;
	size_t rel_addr = 0;

	int is_shared = 0;

	if(!my_node_level_rank)
	{
		p        = mpc_lowcomm_allocmem_pool_alloc_check(sizeof(mpc_common_spinlock_t), &is_shared);
		rel_addr = mpc_lowcomm_allocmem_relative_addr(p);
		mpc_common_spinlock_init(p, 0);
	}


	mpc_lowcomm_bcast(&p, sizeof(void *), 0, per_node_comm);


	__accululate_master_lock = ( mpc_common_spinlock_t * )mpc_lowcomm_allocmem_abs_addr(rel_addr);
}

void mpc_lowcomm_accumulate_op_lock()
{
	//mpc_common_debug_error("LOCK");

	assert(__accululate_master_lock != NULL);
	mpc_common_spinlock_lock_yield(__accululate_master_lock);
}

void mpc_lowcomm_accumulate_op_unlock()
{
	//mpc_common_debug_error("UNLOCK");
	assert(__accululate_master_lock != NULL);
	mpc_common_spinlock_unlock(__accululate_master_lock);
}

static inline void __setup_pool(mpc_lowcomm_communicator_t per_node_comm)
{
	size_t pool_size = __get_pool_size();

	/* Compute pool size */
	size_t inner_size = pool_size;

	/* Book space for the bit array */
	inner_size += SCTK_PAGE_SIZE;
	/* Book space for the spinlock */
	inner_size += sizeof(mpc_common_spinlock_t);
	/* Book space for the canary */
	inner_size += sizeof(int);

	int cw_rank = mpc_lowcomm_communicator_rank(MPC_COMM_WORLD);

	/* disabling shm allocator for C/R (temp) */
	if(mpc_common_get_flags()->checkpoint_enabled)
	{
		if(!cw_rank)
		{
			mpc_common_debug_warning("Shared-Node memory RMA are not supported when C/R is enabled");
		}


		_mpc_lowcomm_conf_get()->memorypool.enabled = 0;
	}

	int alloc_mem_enabled = _mpc_lowcomm_conf_get()->memorypool.enabled;

	if(alloc_mem_enabled == 0)
	{
		_pool_only_local = 1;
		return;
	}

	mpc_lowcomm_communicator_t process_master_comm;
	int is_master = 0;

#if defined(MPC_IN_PROCESS_MODE)
	is_master           = 1;
	process_master_comm = per_node_comm;
#else
	/* Now only local rank 0 in each process will work */

	if(mpc_common_get_local_task_rank() == 0)
	{
		is_master = 1;
	}

	process_master_comm = mpc_lowcomm_communicator_split(per_node_comm, is_master, cw_rank);
#endif

	if(is_master)
	{
		/* Round size up to the next page */
		size_t to_map_size = ( (inner_size / SCTK_PAGE_SIZE) + 1) * SCTK_PAGE_SIZE;
		__mpc_lowcomm_memory_pool.mapped_size = to_map_size;

		mpc_launch_shm_exchange_params_t params;

		params.mpi.barrier = __barrier;
		params.mpi.bcast   = __bcast;
		params.mpi.rank    = __rank;
		params.mpi.pcomm   = process_master_comm;

		//mpc_common_debug_warning("POOL is %ld large", to_map_size);

		__mpc_lowcomm_memory_pool._pool = mpc_launch_shm_map(to_map_size, MPC_LAUNCH_SHM_USE_MPI, &params);

		//mpc_common_debug_error("POOL @ %p", __mpc_lowcomm_memory_pool._pool);

		assume(__mpc_lowcomm_memory_pool._pool != NULL);

#if !defined(MPC_IN_PROCESS_MODE)
		mpc_lowcomm_communicator_free(&process_master_comm);
#endif
	
		/* First mark segment start for debug */
		int * canary =  (int *)__mpc_lowcomm_memory_pool._pool;
		*canary = 1377;

		/* After we have a lock */
		void * after_canary = canary + 1;
		
		__mpc_lowcomm_memory_pool.lock = after_canary;

		/* A bit array */		
		void * bit_array = __mpc_lowcomm_memory_pool.lock + 1;

		/* And to finish the actual memory pool */
		__mpc_lowcomm_memory_pool.pool = bit_array + SCTK_PAGE_SIZE;

		mpc_common_nodebug("BASE %p LOCK %p POOL %p", bit_array_page,
		                   __mpc_lowcomm_memory_pool.lock,
		                   __mpc_lowcomm_memory_pool.pool);

		__mpc_lowcomm_memory_pool.size      = pool_size;
		__mpc_lowcomm_memory_pool.size_left = pool_size;

		mpc_common_spinlock_init(__mpc_lowcomm_memory_pool.lock, 0);

		mpc_common_bit_array_init_buff(&__mpc_lowcomm_memory_pool.mask,
		                               SCTK_PAGE_SIZE * 8, bit_array,
		                               SCTK_PAGE_SIZE);

		__mpc_lowcomm_memory_pool.space_per_bit = (pool_size / (SCTK_PAGE_SIZE * 8) );
		mpc_common_hashtable_init(&__mpc_lowcomm_memory_pool.size_ht, 32);
	}
}

int mpc_lowcomm_allocmem_pool_init()
{
	/* Check for particular programs */
	__pool_size_adapt(mpc_common_get_flags()->exename);

	/* Make sure only one process does the internal
	 * pool init in the shared memory segment */
	mpc_common_spinlock_lock(&_pool_init_lock);

	if(_pool_init_done == 0)
	{
		/* Pool was globally initialized */
		_pool_init_done = 1;
	}

	mpc_common_spinlock_unlock(&_pool_init_lock);

	int cw_rank = mpc_lowcomm_communicator_rank(MPC_COMM_WORLD);

	/* First build a comm for each node */
	mpc_lowcomm_communicator_t per_node_comm = mpc_lowcomm_communicator_split(MPC_COMM_WORLD, mpc_common_get_node_rank(), cw_rank);

	int my_node_size = mpc_lowcomm_communicator_size(per_node_comm);


	/* Allocate Workshare */
	int split_ws = mpc_common_get_process_rank();
	mpc_lowcomm_communicator_t per_process_comm = mpc_lowcomm_communicator_split(per_node_comm, split_ws, cw_rank);

	alloc_workshare(per_process_comm);
	mpc_lowcomm_communicator_free(&per_process_comm);


	/* Are all the tasks in the same process ? */
	if( (my_node_size == 1) || (mpc_lowcomm_get_process_count() == 1))
	{
		/* Memory pool is only local */
		_pool_only_local = 1;
		/* Set accumulate lock in shared */
		__accumulate_op_lock_init_shared();
		return 0;
	}
	else
	{
		__setup_pool(per_node_comm);

		mpc_lowcomm_barrier(per_node_comm);

		__accumulate_op_lock_init(per_node_comm);

		if(_mpc_lowcomm_conf_get()->memorypool.enabled)
		{
			int * canary = (int *)__mpc_lowcomm_memory_pool._pool;
			assume(*canary == 1377);
		}
	}

	mpc_lowcomm_communicator_free(&per_node_comm);


	return 0;
}

int mpc_lowcomm_allocmem_pool_release()
{
	/* Are all the tasks in the same process ? */
	if(_pool_only_local)
	{
		return 0;
	}

	int is_master = 0;

	mpc_common_spinlock_lock(&_pool_init_lock);

	if(_pool_init_done)
	{
		is_master       = 1;
		_pool_init_done = 0;
	}

	mpc_common_spinlock_unlock(&_pool_init_lock);

	mpc_launch_shm_unmap(__mpc_lowcomm_memory_pool._pool,
	                     __mpc_lowcomm_memory_pool.mapped_size);

	if(is_master)
	{
		mpc_common_hashtable_release(&__mpc_lowcomm_memory_pool.size_ht);
		/* All the rest is static */
		memset(&__mpc_lowcomm_memory_pool, 0,
		       sizeof(struct mpc_lowcomm_allocmem_pool) );
	}

	return 0;
}

static inline void mpc_lowcomm_allocmem_pool_lock()
{
	assert(__mpc_lowcomm_memory_pool.lock);

	mpc_common_spinlock_lock_yield(__mpc_lowcomm_memory_pool.lock);
}

static inline void mpc_lowcomm_allocmem_pool_unlock()
{
	assert(__mpc_lowcomm_memory_pool.lock);
	mpc_common_spinlock_unlock(__mpc_lowcomm_memory_pool.lock);

}

void *mpc_lowcomm_allocmem_pool_alloc_check(size_t size, int *is_shared)
{
	*is_shared = 0;

	/* Are all the tasks in the same process ? */
	if(_pool_only_local)
	{
		*is_shared = 1;
		return sctk_malloc(size);
	}

	/* We are sure that it does not fit */
	if( (__mpc_lowcomm_memory_pool.size_left < size) )
	{
		return sctk_malloc(size);
	}

	mpc_lowcomm_allocmem_pool_lock();

	uint64_t number_of_bits = 1 + (size + (size % __mpc_lowcomm_memory_pool.space_per_bit) ) / __mpc_lowcomm_memory_pool.space_per_bit;

	/* Now try to find this number of contiguous free bits */
	size_t i, j;
	struct mpc_common_bit_array *ba = &__mpc_lowcomm_memory_pool.mask;


	for(i = 0; i < ba->real_size; i++)
	{
		if(mpc_common_bit_array_get(ba, i) )
		{
			/* This bit is taken */
			continue;
		}

		uint64_t end_of_seg_off = i + number_of_bits;

		if(ba->real_size <= end_of_seg_off)
		{
			/* We are too close from the end */
			break;
		}

		if(mpc_common_bit_array_get(ba, end_of_seg_off) == 0)
		{
			int found_taken_bit = 0;
			/* Last bit is free it is a good candidate */

			/* Now scan the whole segment */
			for(j = i; j <= end_of_seg_off ; j++)
			{
				if(mpc_common_bit_array_get(ba, j) )
				{
					found_taken_bit = 1;
					/* No need to rescan the same area */
					i = j+1;
					break;
				}
			}

			if(found_taken_bit == 0)
			{
				/* We found enough space */
				/* Book the bits */
				size_t k;

				for(k = i; k <= end_of_seg_off; k++)
				{
					mpc_common_bit_array_set(ba, k, 1);
				}


				/* Compute address */
				void *addr = __mpc_lowcomm_memory_pool.pool + (i * __mpc_lowcomm_memory_pool.space_per_bit);
				__mpc_lowcomm_memory_pool.size_left -= number_of_bits * __mpc_lowcomm_memory_pool.space_per_bit;
				mpc_common_nodebug("Alloc %p [%d to %d]", addr, i, end_of_seg_off);


				/* Store bit size for free for address */
				mpc_common_hashtable_set(&__mpc_lowcomm_memory_pool.size_ht, ( uint64_t )addr,
				                         ( void * )number_of_bits);

				mpc_lowcomm_allocmem_pool_unlock();
				*is_shared = 1;
				/* Proudly return the address */
				return addr;
			}
		}
	}

	mpc_lowcomm_allocmem_pool_unlock();
	/* We failed */

	return sctk_malloc(size);
}

void *mpc_lowcomm_allocmem_pool_alloc(size_t size)
{
	int dummy_shared;

	void * ret = mpc_lowcomm_allocmem_pool_alloc_check(size, &dummy_shared);

	return ret;
}

int mpc_lowcomm_allocmem_pool_free_size(void *ptr, ssize_t known_size)
{
	/* Are all the tasks in the same process ? */
	if(_pool_only_local)
	{
		sctk_free(ptr);
		return 0;
	}

	mpc_lowcomm_allocmem_pool_lock();
	void *size_in_ptr = NULL;



	if(known_size < 0)
	{
		size_in_ptr = mpc_common_hashtable_get(&__mpc_lowcomm_memory_pool.size_ht, ( uint64_t )ptr);
	}
	else
	{
		size_in_ptr = ( void * )known_size;
	}

	if(size_in_ptr != NULL)
	{
		/* Compute bit_offset */
		size_t off = (ptr - __mpc_lowcomm_memory_pool.pool) /
		             __mpc_lowcomm_memory_pool.space_per_bit;
		size_t number_of_bits = (size_t)size_in_ptr;
		size_t i;


		mpc_common_nodebug("Free %p [%d to %d]", ptr, off, (off + number_of_bits));


		for(i = off; i <= (off + number_of_bits); i++)
		{
			mpc_common_bit_array_set(&__mpc_lowcomm_memory_pool.mask, i, 0);
		}

		__mpc_lowcomm_memory_pool.size_left += number_of_bits * __mpc_lowcomm_memory_pool.space_per_bit;
		mpc_common_hashtable_delete(&__mpc_lowcomm_memory_pool.size_ht, ( uint64_t )ptr);
		mpc_lowcomm_allocmem_pool_unlock();
	}
	else
	{
		mpc_lowcomm_allocmem_pool_unlock();
		/* If we are here the pointer was allocated */
		sctk_free(ptr);
	}

	return 0;
}

int mpc_lowcomm_allocmem_pool_free(void *ptr)
{
	return mpc_lowcomm_allocmem_pool_free_size(ptr, -1);
}

int mpc_lowcomm_allocmem_is_in_pool(void *ptr)
{
	struct mpc_lowcomm_allocmem_pool *p = &__mpc_lowcomm_memory_pool;

	if( (p->pool <= ptr) && (ptr < (p->pool + p->size) ) )
	{
		return 1;
	}

	return 0;
}

size_t mpc_lowcomm_allocmem_relative_addr(void *in_pool_addr)
{
	return in_pool_addr - __mpc_lowcomm_memory_pool.pool;
}

void *mpc_lowcomm_allocmem_abs_addr(size_t relative_addr)
{
	return (char *)__mpc_lowcomm_memory_pool.pool + relative_addr;
}
