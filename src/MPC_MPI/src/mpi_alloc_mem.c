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

#include "mpi_alloc_mem.h"

#include "mpi_conf.h"

#include <mpc_common_rank.h>

#include <sctk_alloc.h>

#include "mpc_lowcomm.h"
#include <mpc_common_helper.h>
#include <mpc_common_rank.h>
#include "mpc_mpi.h"
#include <mpc_common_debug.h>
#include <mpc_common_flags.h>
#include <mpc_launch_shm.h>


#ifdef MPC_Threads
#include "mpc_thread.h"
#endif

#include "mpc_thread.h"
#include "string.h"

struct mpc_MPI_allocmem_pool ____mpc_sctk_mpi_alloc_mem_pool;

static int _pool_init_done = 0;
static int _pool_only_local = 0;
static mpc_common_spinlock_t _pool_init_lock = SCTK_SPINLOCK_INITIALIZER;

static size_t _forced_pool_size = 0;

void mpc_MPI_allocmem_adapt( char *exename )
{
	if ( !exename )
	{
		return;
	}

	if ( _forced_pool_size != 0 )
	{
		return;
	}

	struct _mpc_mpi_config_mem_pool * mempool_conf = &(_mpc_mpi_config()->mempool);


	int is_linear = mempool_conf->force_process_linear;

	if ( mempool_conf->autodetect )
	{
		if ( !strstr( exename, "IMB" ) )
		{
			/* Force linear */
			is_linear = 1;
		}
	}

	if ( is_linear )
	{
		_forced_pool_size = mempool_conf->per_proc_size *
		    mpc_common_get_local_process_count();
		mpc_common_debug( "Info : setting MPI_Alloc_mem pool size to %d MB",
		           _forced_pool_size / ( 1024 * 1024 ) );
	}
}

size_t mpc_MPI_allocmem_get_pool_size()
{
	if ( _forced_pool_size != 0 )
	{
		return _forced_pool_size;
	}

	return _mpc_mpi_config()->mempool.size;
}

static inline int __barrier(void *pcomm)
{
	MPI_Comm comm = (MPI_Comm)pcomm;
	PMPI_Barrier(comm);
	return 0;
}

static inline int __rank(void *pcomm)
{
	MPI_Comm comm = (MPI_Comm)pcomm;
	int ret;
	PMPI_Comm_rank(comm, &ret);
	return ret;
}

static inline int __bcast(void *buff, size_t len, void *pcomm)
{
	MPI_Comm comm = (MPI_Comm)pcomm;
	
	PMPI_Bcast(buff,len, MPI_BYTE, 0, comm);
	return 0;
}


int mpc_MPI_allocmem_pool_init()
{
        /* Check for particular programs */
        mpc_MPI_allocmem_adapt(mpc_common_get_flags()->exename);

	size_t pool_size = mpc_MPI_allocmem_get_pool_size();
	int do_init = 0;
	mpc_common_spinlock_lock( &_pool_init_lock );

	if ( _pool_init_done == 0 )
	{
		_pool_init_done = 1;
		do_init = 1;
	}

	mpc_common_spinlock_unlock( &_pool_init_lock );
	int cw_rank;
	PMPI_Comm_rank( MPI_COMM_WORLD, &cw_rank );
	/* This code is one task per MPC process */
	size_t inner_size = pool_size;
	/* Book space for the bit array at buffer start */
	inner_size += SCTK_ALLOC_PAGE_SIZE;
	/* Book space for the spinlock */
	inner_size += sizeof( OPA_int_t );

	/* Are all the tasks in the same process ? */
	if ( mpc_common_get_task_count() == mpc_common_get_local_task_count() )
	{
		mpc_MPI_accumulate_op_lock_init_shared();
		return 0;
	}
	else
	{
		/* First build a comm for each node */
		MPI_Comm node_comm;
		PMPI_Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, cw_rank,
		                      MPI_INFO_NULL, &node_comm );
		int my_rank, comm_size;
		PMPI_Comm_rank( node_comm, &my_rank );
		PMPI_Comm_size( node_comm, &comm_size );

		/* disabling shm allocator for C/R (temp) */
		if ( mpc_common_get_flags()->checkpoint_enabled )
		{
			if ( !cw_rank )
			{
				mpc_common_debug_warning( "Shared-Node memory RMA are not supported when C/R is enabled" );
			}

			
			_mpc_mpi_config()->mempool.enabled = 0;
		}

		int alloc_mem_enabled = _mpc_mpi_config()->mempool.enabled;
		int tot_size = 0;

		PMPI_Allreduce( &comm_size, &tot_size, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );

		if ( ( tot_size == mpc_common_get_task_count() ) || ( alloc_mem_enabled == 0 ) )
		{
			mpc_MPI_accumulate_op_lock_init_shared();
			_pool_only_local = 1;
			PMPI_Comm_free( &node_comm );
			return 0;
		}

		/* Now count number of processes by counting masters */
		int is_master = 0;

		if ( mpc_common_get_local_task_rank() == 0 )
		{
			is_master = 1;
		}

		MPI_Comm process_master_comm;

		PMPI_Comm_split( node_comm, is_master, cw_rank, &process_master_comm );

		if ( is_master )
		{
			size_t to_map_size = ( ( inner_size / SCTK_ALLOC_PAGE_SIZE ) + 1 ) *
			                     SCTK_ALLOC_PAGE_SIZE;
			____mpc_sctk_mpi_alloc_mem_pool.mapped_size = to_map_size;

			mpc_launch_shm_exchange_params_t params;

			params.mpi.barrier = __barrier;
			params.mpi.bcast = __bcast;
			params.mpi.rank = __rank;
			params.mpi.pcomm = process_master_comm;

			____mpc_sctk_mpi_alloc_mem_pool._pool = mpc_launch_shm_map(to_map_size, MPC_LAUNCH_SHM_USE_MPI, &params);

			assume(____mpc_sctk_mpi_alloc_mem_pool._pool != NULL);

			PMPI_Comm_free( &process_master_comm );
		}

		PMPI_Comm_free( &node_comm );

	}

	mpc_lowcomm_barrier( MPI_COMM_WORLD );
	assume( ____mpc_sctk_mpi_alloc_mem_pool._pool != NULL );

	if ( do_init )
	{
		/* Keep pointer to the allocated pool  */
		void *bit_array_page = ____mpc_sctk_mpi_alloc_mem_pool._pool;
		/* After we have a lock */
		____mpc_sctk_mpi_alloc_mem_pool.lock =
		    bit_array_page + SCTK_ALLOC_PAGE_SIZE;
		/* And to finish the actual memory pool */
		____mpc_sctk_mpi_alloc_mem_pool.pool =
		    ____mpc_sctk_mpi_alloc_mem_pool.lock + 1;
		mpc_common_nodebug( "BASE %p LOCK %p POOL %p", bit_array_page,
		              ____mpc_sctk_mpi_alloc_mem_pool.lock,
		              ____mpc_sctk_mpi_alloc_mem_pool.pool );
		____mpc_sctk_mpi_alloc_mem_pool.size = pool_size;
		____mpc_sctk_mpi_alloc_mem_pool.size_left = pool_size;
		OPA_store_int( ____mpc_sctk_mpi_alloc_mem_pool.lock, 0 );
		mpc_common_bit_array_init_buff( &____mpc_sctk_mpi_alloc_mem_pool.mask,
		                                SCTK_ALLOC_PAGE_SIZE * 8, bit_array_page,
		                                SCTK_ALLOC_PAGE_SIZE );
		____mpc_sctk_mpi_alloc_mem_pool.space_per_bit =
		    ( pool_size / ( SCTK_ALLOC_PAGE_SIZE * 8 ) );
		mpc_common_hashtable_init( &____mpc_sctk_mpi_alloc_mem_pool.size_ht, 32 );
	}

	mpc_MPI_accumulate_op_lock_init();

	mpc_lowcomm_barrier( MPI_COMM_WORLD );
	return 0;
}

int mpc_MPI_allocmem_pool_release()
{
	mpc_lowcomm_barrier( MPI_COMM_WORLD );

	/* Are all the tasks in the same process ? */
	if ( _pool_only_local ||
	     ( mpc_common_get_task_count() == mpc_common_get_local_task_count() ) )
	{
		return 0;
	}

	int is_master = 0;
	mpc_common_spinlock_lock( &_pool_init_lock );

	if ( _pool_init_done )
	{
		is_master = 1;
		_pool_init_done = 0;
	}

	mpc_common_spinlock_unlock( &_pool_init_lock );

	if ( _pool_only_local ||
	     ( mpc_common_get_task_count() == mpc_common_get_local_task_count() ) )
	{
		if ( is_master )
		{
			sctk_free( ____mpc_sctk_mpi_alloc_mem_pool._pool );
		}
	}
	else
	{
		mpc_launch_shm_unmap( ____mpc_sctk_mpi_alloc_mem_pool._pool,
		                       ____mpc_sctk_mpi_alloc_mem_pool.mapped_size );
	}

	mpc_lowcomm_barrier( MPI_COMM_WORLD );

	if ( is_master )
	{
		mpc_common_hashtable_release( &____mpc_sctk_mpi_alloc_mem_pool.size_ht );
		/* All the rest is static */
		memset( &____mpc_sctk_mpi_alloc_mem_pool, 0,
		        sizeof( struct mpc_MPI_allocmem_pool ) );
	}

	return 0;
}

static inline void mpc_MPI_allocmem_pool_lock()
{
	assert( ____mpc_sctk_mpi_alloc_mem_pool.lock );

	while ( OPA_cas_int( ____mpc_sctk_mpi_alloc_mem_pool.lock, 0, 1 ) )
	{
		mpc_thread_yield();
	}
}

static inline void mpc_MPI_allocmem_pool_unlock()
{
	assert( ____mpc_sctk_mpi_alloc_mem_pool.lock );
	OPA_store_int( ____mpc_sctk_mpi_alloc_mem_pool.lock, 0 );
}


void *mpc_MPI_allocmem_pool_alloc_check( size_t size, int *is_shared )
{
	*is_shared = 0;

	/* Are all the tasks in the same process ? */
	if ( _pool_only_local ||
	     ( mpc_common_get_task_count() == mpc_common_get_local_task_count() ) )
	{
		*is_shared = 1;
		return sctk_malloc( size );
	}

	/* We are sure that it does not fit */
	if ( ( ____mpc_sctk_mpi_alloc_mem_pool.size < size ) )
	{
		return sctk_malloc( size );
	}

	mpc_MPI_allocmem_pool_lock();
	size_t number_of_bits =
	    1 +
	    ( size + ( size % ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit ) ) /
	    ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit;
	/* Now try to find this number of contiguous free bits */
	size_t i, j;
	struct mpc_common_bit_array *ba = &____mpc_sctk_mpi_alloc_mem_pool.mask;

	for ( i = 0; i < ba->real_size; i++ )
	{
		if ( mpc_common_bit_array_get( ba, i ) )
		{
			/* This bit is taken */
			continue;
		}

		unsigned long long end_of_seg_off = i + number_of_bits;

		if ( ba->real_size <= end_of_seg_off )
		{
			/* We are too close from the end */
			break;
		}

		if ( mpc_common_bit_array_get( ba, end_of_seg_off ) == 0 )
		{
			int found_taken_bit = 0;
			/* Last bit is free it is a good candidate */

			/* Now scan the whole segment */
			for ( j = i; ( j < ( i + number_of_bits ) ); j++ )
			{
				if ( mpc_common_bit_array_get( ba, j ) )
				{
					found_taken_bit = 1;
					break;
				}
			}

			if ( found_taken_bit == 0 )
			{
				/* We found enough space */
				/* Book the bits */
				size_t k;

				for ( k = i; k < ( i + number_of_bits ); k++ )
				{
					mpc_common_bit_array_set( ba, k, 1 );
				}

				/* Compute address */
				void *addr = ____mpc_sctk_mpi_alloc_mem_pool.pool +
				             ( i * ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit );
				/* Store bit size for free for address */
				mpc_common_hashtable_set( &____mpc_sctk_mpi_alloc_mem_pool.size_ht, ( uint64_t )addr,
				                          ( void * )number_of_bits );
				mpc_common_nodebug( "ALLOC %ld", number_of_bits );
				mpc_MPI_allocmem_pool_unlock();
				*is_shared = 1;
				/* Proudly return the address */
				return addr;
			}
		}

		// else
		//{
		//	/* Last bit is taken skip to next */
		//	i = end_of_seg_off + 1;
		//}
	}

	mpc_MPI_allocmem_pool_unlock();
	/* We failed */
	return sctk_malloc( size );
}


void *mpc_MPI_allocmem_pool_alloc( size_t size )
{
	int dummy_shared;
	return mpc_MPI_allocmem_pool_alloc_check( size, &dummy_shared );
}


int mpc_MPI_allocmem_pool_free_size( void *ptr, ssize_t known_size )
{
	/* Are all the tasks in the same process ? */
	if ( _pool_only_local ||
	     ( mpc_common_get_task_count() == mpc_common_get_local_task_count() ) )
	{
		sctk_free( ptr );
		return 0;
	}

	mpc_MPI_allocmem_pool_lock();
	void *size_in_ptr = NULL;

	if ( known_size < 0 )
	{
		size_in_ptr = mpc_common_hashtable_get( &____mpc_sctk_mpi_alloc_mem_pool.size_ht, ( uint64_t )ptr );
	}
	else
	{
		size_in_ptr = ( void * )known_size;
	}

	if ( size_in_ptr != NULL )
	{
		size_t size = ( size_t )size_in_ptr;
		/* Compute bit_offset */
		size_t off = ( ptr - ____mpc_sctk_mpi_alloc_mem_pool.pool ) /
		             ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit;
		size_t number_of_bits = size;
		size_t i;

		for ( i = off; i < ( off + number_of_bits ); i++ )
		{
			mpc_common_bit_array_set( &____mpc_sctk_mpi_alloc_mem_pool.mask, i, 0 );
		}

		mpc_common_hashtable_delete( &____mpc_sctk_mpi_alloc_mem_pool.size_ht, ( uint64_t )ptr );
		mpc_MPI_allocmem_pool_unlock();
	}
	else
	{
		mpc_MPI_allocmem_pool_unlock();
		/* If we are here the pointer was allocated */
		sctk_free( ptr );
	}

	return 0;
}


int mpc_MPI_allocmem_pool_free( void *ptr )
{
	return mpc_MPI_allocmem_pool_free_size( ptr, -1 );
}

int mpc_MPI_allocmem_is_in_pool( void *ptr )
{
	return _mpc_MPI_allocmem_is_in_pool( ptr );
}

/* This is the accumulate pool protection */

static mpc_common_spinlock_t *__accululate_master_lock = NULL;
static mpc_common_spinlock_t __static_lock_for_acc = SCTK_SPINLOCK_INITIALIZER;

void mpc_MPI_accumulate_op_lock_init_shared()
{
	__accululate_master_lock = &__static_lock_for_acc;
}

void mpc_MPI_accumulate_op_lock_init()
{
	/* First build a comm for each node */
	MPI_Comm node_comm;
	int cw_rank;
	PMPI_Comm_rank( MPI_COMM_WORLD, &cw_rank );

	PMPI_Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, cw_rank,
	                      MPI_INFO_NULL, &node_comm );
	int my_rank, comm_size;
	PMPI_Comm_rank( node_comm, &my_rank );
	PMPI_Comm_size( node_comm, &comm_size );
	void *p = NULL;

	if ( !my_rank )
	{
		p = mpc_MPI_allocmem_pool_alloc( sizeof( mpc_common_spinlock_t ) );
		OPA_store_int(p, 0);
	}

	mpc_lowcomm_bcast( &p, sizeof( MPI_Aint ), 0, node_comm );
	__accululate_master_lock = ( mpc_common_spinlock_t * )p;
	PMPI_Comm_free( &node_comm );
}

void mpc_MPI_accumulate_op_lock()
{
	assert( __accululate_master_lock != NULL );
	mpc_common_spinlock_lock_yield( __accululate_master_lock );
}

void mpc_MPI_accumulate_op_unlock()
{
	assert( __accululate_master_lock != NULL );
	mpc_common_spinlock_unlock( __accululate_master_lock );
}
