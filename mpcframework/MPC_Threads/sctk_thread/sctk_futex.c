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
#include "sctk_futex.h"
#include "sctk_debug.h"
#include "sctk_thread.h"

#include <stdio.h>
#include <pthread.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdlib.h>

/************************************************************************/
/* Futex Queues                                                         */
/************************************************************************/
 
 
struct futex_queue * futex_queue_new( int * futex_key )
{
	struct futex_queue * ret = malloc( sizeof( struct futex_queue ) );
	
	if( !ret )
	{
		perror("malloc");
		return NULL;
	}
	
	sctk_atomics_store_int( &ret->queue_is_wake_tainted, 0 );

	Buffered_FIFO_init(&ret->wait_list, 32, sizeof(int *));
	
	ret->futex_key = futex_key;
	
	return ret;
}

void * futex_queue_new_from_key( sctk_uint64_t key )
{
	return (void *)futex_queue_new( (int *) key );
}

int futex_queue_release( struct futex_queue * fq )
{
	Buffered_FIFO_release( &fq->wait_list );
	
	free( fq ); 
	
	return 0;
}

int ** futex_queue_push( struct futex_queue * fq )
{
	/* Make sure we do not push when poping
	 * to avoid repopping previously popped threads */
	while( sctk_atomics_load_int( &fq->queue_is_wake_tainted ) )
	{
		sched_yield();
	}
	
	int  * do_wait = malloc( sizeof( int ) );
	
	if( !do_wait )
	{
		perror("malloc");
		return NULL;
	}
	
	*do_wait = 1;
	
	return (int **) Buffered_FIFO_push( &fq->wait_list , &do_wait );
}

int futex_queue_wake( struct futex_queue * fq, int count )
{
	int popped = 0;
	
	sctk_atomics_store_int( &fq->queue_is_wake_tainted, 1 );
	
	while( count )
	{
		int * to_wake = NULL;
		
		
		if( !Buffered_FIFO_pop(&fq->wait_list, (void *)&to_wake ) )
		{
			/* No more elem to pop */
			break;
		}
		else
		{
			/* Wake up the thread */
			if( to_wake )
			{
				*to_wake = 0;
				
				/* Here we yield to avoid
				 * releasing all threads at
				 * once if they are contending on the same lock
				 * we will do a scheduling round before releasing
				 * the next, then being more fair in case
				 * of competition  */
				sched_yield();
				popped++;
			}
			else
			{
				sctk_error("Popped an empty elem ??");
			}
		}
		
		count--;
	}
	
	sctk_atomics_store_int( &fq->queue_is_wake_tainted, 0 );
	
	return popped;
}

/************************************************************************/
/* Futex HT                                                             */
/************************************************************************/

int futex_queue_HT_init( struct futex_queue_HT * ht )
{
	MPCHT_init( &ht->queue_hash_table, 128 );
	ht->queue_count = 0;
	ht->queue_cleanup_ratio = 128;
	sctk_atomics_store_int( &ht->queue_table_is_being_manipulated, 0);
	
	return 0;
}

int futex_queue_HT_release( struct futex_queue_HT * ht )
{
	MPCHT_release( &ht->queue_hash_table );
	ht->queue_count = 0;
	ht->queue_cleanup_ratio = 0;
	sctk_atomics_store_int( &ht->queue_table_is_being_manipulated, 0);

	return 0;
}


int * futex_queue_HT_register_thread( struct futex_queue_HT * ht , int * futex_key )
{
	/* Here we are protecting a table manipulation */
	while( sctk_atomics_load_int( &ht->queue_table_is_being_manipulated )  < 0 )
	{
		sched_yield();
	}
	
	/* This is the Recycling policy to avoid wasting memory 
	 * if the user keeps creating different futexes */
	if( ht->queue_cleanup_ratio < ht->queue_count )
	{
		/* Flag the table as being manipulated dy decrementing by one */
		sctk_atomics_decr_int( &ht->queue_table_is_being_manipulated);
		
		/* Wait of all the threads in the critical section to leave */
		while( 0 <= sctk_atomics_load_int( &ht->queue_table_is_being_manipulated ) )
		{
			sched_yield();
		}
		
		/* At this point no thread shall enter */
		void * pfutex_queue = NULL;
		MPC_HT_ITER( &ht->queue_hash_table, pfutex_queue )
		{
			struct futex_queue * queue = (struct futex_queue *)pfutex_queue;
			
			if( !Buffered_FIFO_count( &queue->wait_list ) )
			{
				/* This is an empty wait list then POP */
				sctk_uint64_t key = (sctk_uint64_t) queue->futex_key;
				MPCHT_delete( &ht->queue_hash_table, key );
				ht->queue_count--;
				
				if( ht->queue_count < 0 )
				{
					sctk_error("Underflow in futex queues");
				}
				
				/* Free it */
				futex_queue_release( queue );
			}
		}
		MPC_HT_ITER_END
		
		
		/* This done we leave the negative locking phase */
		sctk_atomics_incr_int( &ht->queue_table_is_being_manipulated);
		
		/* As we are not sure to be the first one entering
		 * we simply recursively call the function */
		return futex_queue_HT_register_thread( ht , futex_key );
	}
	
	
	/* Now that resource exhaustion has been managed
	 * lets focus on ressource allocation */
	 
	int * ret = NULL;

	/* Acquire the read lock */
	sctk_atomics_incr_int( &ht->queue_table_is_being_manipulated);
	
	/* Here we first create a futex queue if 
	 * it does not exists yet */
	sctk_uint64_t new_key = (sctk_uint64_t) futex_key;
	
	int new_queue_created = 0;
	struct futex_queue *fq = (struct futex_queue *) 
	          MPCHT_get_or_create(&ht->queue_hash_table,
								   new_key ,
								   futex_queue_new_from_key,
								   &new_queue_created);
	/* Was a new queue created ? */
	if( new_queue_created )
		ht->queue_count++;

	/* Did it work ? */
	if( fq )
	{
		/* Here we are now manipulating a futex queue */
		
		/* Push a new wait entry */
		int ** new_do_wait = futex_queue_push( fq );
		
		if( new_do_wait )
		{
			/* It worked */
			ret = *new_do_wait;
		}
		else
		{
			/* Something ODD happened */
			sctk_error("FUTEX insert in wait queue failed");
		}
		
	}
	
	/* Release the read lock */
	sctk_atomics_decr_int( &ht->queue_table_is_being_manipulated);
	
	return ret;
}


int futex_queue_HT_wake_threads( struct futex_queue_HT * ht , int * futex_key, int count )
{
	/* Here we are protecting a table manipulation */
	while( sctk_atomics_load_int( &ht->queue_table_is_being_manipulated )  < 0 )
	{
		sched_yield();
	}
 
	int ret = 0;

	/* Acquire the read lock */
	sctk_atomics_incr_int( &ht->queue_table_is_being_manipulated);
		
	sctk_uint64_t key = (sctk_uint64_t) futex_key;
	struct futex_queue *fq = (struct futex_queue *) 
							   MPCHT_get( &ht->queue_hash_table,key );
	
	if( !fq )
	{
		/* No such queue */
		errno = EINVAL;
		sctk_atomics_decr_int( &ht->queue_table_is_being_manipulated);
		return -1;
	}
	else
	{
		/* Perform the wake */
		ret = futex_queue_wake( fq, count );
	}
	
	/* Release the read lock */
	sctk_atomics_decr_int( &ht->queue_table_is_being_manipulated);
	
	sctk_info("Waking %d threads\n", ret );
	
	return ret;
}


/************************************************************************/
/* Futex Context                                                        */
/************************************************************************/

static struct futex_queue_HT ___futex_queue_HT;

void sctk_futex_context_init()
{
	futex_queue_HT_init( &___futex_queue_HT );
}

void sctk_futex_context_release()
{
	futex_queue_HT_release( &___futex_queue_HT );
}

/************************************************************************/
/* Futex Ops                                               	            */
/************************************************************************/

int sctk_futex_WAIT_no_timer(void *addr1, int op, int val1 )
{
	int * wait_ticket = futex_queue_HT_register_thread( &___futex_queue_HT, addr1 );
	
	if(!wait_ticket)
	{
		return -1;
	}
	
	sctk_thread_wait_for_value ( wait_ticket, 0);
	
	/* If we are here we are done waiting */
	free( wait_ticket );
	
	return 0;
}


int sctk_futex_WAIT(void *addr1, int op, int val1, struct timespec *timeout )
{
	/* First check the value */
	OPA_int_t * pold_val = (OPA_int_t *)addr1;
	
	int old_val = sctk_atomics_load_int( pold_val );
	
	/* Val is different directly return */
	if( old_val != val1 )
	{
		errno = EWOULDBLOCK;
		return -1;
	}
	
	
	/* Do we need to jump to the blocking implementation ? */
	if( !timeout )
	{
		return sctk_futex_WAIT_no_timer(addr1, op, val1 );
	}
	
	/* Here we consider the timed implementation */
	
	int * wait_ticket = futex_queue_HT_register_thread( &___futex_queue_HT, addr1 );
	
	if(!wait_ticket)
	{
		return -1;
	}

	unsigned int time_to_wait = timeout->tv_sec * 1e6 + timeout->tv_nsec;

	if( sctk_thread_timed_wait_for_value (wait_ticket, 0, time_to_wait) )
	{
		/* We timed out */
		errno = ETIMEDOUT;
		free( wait_ticket );
		return -1;
	}
	
	/* If we are here we are done waiting */
	free( wait_ticket );

	return 0;
}


int sctk_futex_WAKE(void *addr1, int op, int val1 )
{
	return futex_queue_HT_wake_threads(  &___futex_queue_HT, addr1,  val1 );
}


int sctk_futex(void *addr1, int op, int val1, 
               struct timespec *timeout, void *addr2, int val3)
{
	int ret = 0;
	
	switch( op )
	{
		case FUTEX_WAIT :
			ret = sctk_futex_WAIT(addr1, op, val1, timeout );
		break;
		case FUTEX_WAKE :
			ret = sctk_futex_WAKE(addr1, op, val1 );
		break;
		default:
			sctk_error("Not implemented YET :=)");
	}

	
	sched_yield();
	
	return ret;
}
