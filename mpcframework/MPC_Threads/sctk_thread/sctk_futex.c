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
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

#if SCTK_FUTEX_SUPPORTED

#include <linux/futex.h>
#include <sys/syscall.h>

int sctk_do_translate_futex_op( int OPCODE )
{
	/* Always remove the private flag 
	 * as it does not mean anything in our context */
	OPCODE &= ~(FUTEX_PRIVATE_FLAG);
	
	switch( OPCODE )
	{
		case FUTEX_WAIT :
			return SCTK_FUTEX_WAIT;
		break;
		case FUTEX_WAKE :
			return SCTK_FUTEX_WAKE;
		break;
		case FUTEX_WAKE_OP :
			return SCTK_FUTEX_WAKE_OP;
		break;
		case FUTEX_REQUEUE :
			return SCTK_FUTEX_REQUEUE;
		break;
		case FUTEX_CMP_REQUEUE :
			return SCTK_FUTEX_CMP_REQUEUE;
		break;
		default:
			sctk_error("Not implemented YET :=)");
	}

	return OPCODE;
}

/* For _OP operations */
int sctk_do_translate_futex_opcode( int opcode )
{
	switch( opcode )
	{
		case FUTEX_OP_SET :
			return SCTK_FUTEX_OP_SET;
		break;
		case FUTEX_OP_ADD :
			return SCTK_FUTEX_OP_ADD;
		break;
		case FUTEX_OP_OR :
			return SCTK_FUTEX_OP_OR;
		break;
		case FUTEX_OP_ANDN :
			return SCTK_FUTEX_OP_ANDN;
		break;
		case FUTEX_OP_XOR :
			return SCTK_FUTEX_OP_XOR;
		break;
		default:
			sctk_error("Failled to convert FUTEX OP");
	}

	return opcode;
}

/* May not be defined */
#ifndef FUTEX_OP_ARG_SHIFT
#define FUTEX_OP_ARG_SHIFT 8
#endif

int sctk_do_translate_futex_opcode_for_shift( int opcode )
{
	return ( opcode & FUTEX_OP_ARG_SHIFT );
}

int sctk_do_translate_cmp( int cmp )
{
	switch( cmp )
	{
		case FUTEX_OP_CMP_EQ:
			return SCTK_FUTEX_OP_CMP_EQ;
		break;
		case FUTEX_OP_CMP_NE:
			return SCTK_FUTEX_OP_CMP_NE;
		break;
		case FUTEX_OP_CMP_LT:
			return SCTK_FUTEX_OP_CMP_LT;
		break;
		case FUTEX_OP_CMP_LE:
			return SCTK_FUTEX_OP_CMP_LE;
		break;
		case FUTEX_OP_CMP_GT:
			return SCTK_FUTEX_OP_CMP_GT;
		break;
		case FUTEX_OP_CMP_GE:
			return  SCTK_FUTEX_OP_CMP_GE;
		break;
		default:
				sctk_fatal("No such cmp operation");
	}

	return cmp;
}
#else

/* No need to translate as values are already
 * internal symbols */

int sctk_do_translate_futex_op( int OPCODE )
{
	return OPCODE;
}

/* For _OP operations */
static inline int sctk_do_translate_futex_opcode( int opcode )
{
	return opcode;
}

int sctk_do_translate_futex_opcode_for_shift( int opcode )
{
	return ( opcode & SCTK_FUTEX_OP_ARG_SHIFT );
}

int sctk_do_translate_cmp( int cmp )
{
	return cmp;
}

#endif


/************************************************************************/
/* Futex Cell                                                         */
/************************************************************************/

struct futex_cell *  futex_cell_new( int bitmask )
{
	struct futex_cell * cell  = calloc( 1, sizeof( struct futex_cell ) );
	
	cell->do_wait = malloc( sizeof( int ) );
	
	if( !cell->do_wait )
	{
		perror("calloc");
		sctk_fatal("alloc failed");
	}
	
	*cell->do_wait = 1;
	
	cell->bitmask = bitmask;
	
	cell->skip = 0;
	
	return cell;
}

int  futex_cell_match( struct futex_cell * cell  , int bitmask )
{
	if( !cell )
		return 0;
		
	return ( cell->bitmask & bitmask );
}

int *  futex_cell_detach( struct futex_cell * cell )
{
	int * ret = cell->do_wait;
	*ret = 0;
	free( cell );
	return ret;
}


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
	
	ret->queue_is_wake_tainted = 0;

	Buffered_FIFO_init(&ret->wait_list, 32, sizeof(struct futex_cell *));
	
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

int * futex_queue_push( struct futex_queue * fq , int bitmask )
{
	/* Make sure we do not push when poping
	 * to avoid repopping previously popped threads */
	sctk_spinlock_lock( &fq->queue_is_wake_tainted );
	
	struct futex_cell * cell = futex_cell_new(0);

	Buffered_FIFO_push( &fq->wait_list , &cell );
	
	sctk_spinlock_unlock( &fq->queue_is_wake_tainted );
	
	return cell->do_wait;
}

int * futex_queue_repush( struct futex_queue * fq , struct futex_cell * to_repush )
{
	sctk_spinlock_lock( &fq->queue_is_wake_tainted );
	
	Buffered_FIFO_push( &fq->wait_list , to_repush );
	
	sctk_spinlock_unlock( &fq->queue_is_wake_tainted );
	
	return to_repush->do_wait;
}

int futex_queue_wake( struct futex_queue * fq , int bitmask, int use_mask, int count )
{
	int popped = 0;
	
	sctk_spinlock_lock( &fq->queue_is_wake_tainted );
	
	/* Wake indiferently from the mask */
	if( use_mask == 0 )
	{
	
		while( count )
		{
			struct futex_cell * to_wake = NULL;
			
			
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
					if( !to_wake->skip )
					{
						*to_wake->do_wait = 0;
						popped++;
						count--;
					}
				}
				else
				{
					sctk_error("Popped an empty elem ??");
				}
			}
			
			
		}
	
	}
	else
	{
		/* We only wake those which are matching the mask */
		
	}
	
	sctk_spinlock_unlock( &fq->queue_is_wake_tainted );
		
	return popped;
}

int futex_queue_requeue( struct futex_queue * fq, struct futex_queue * out, int count )
{
	int popped = 0;
	
	sctk_spinlock_lock( &fq->queue_is_wake_tainted );

	struct futex_cell * to_wake = NULL;
	
	while( count )
	{
		
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
				*to_wake->do_wait = 0;
				popped++;
			}
			else
			{
				sctk_error("Popped an empty elem ??");
			}
		}
		
		count--;
	}
	
	if( fq != out )
	{
		/* Queues are different, lets push on the new one */
		while( Buffered_FIFO_pop(&fq->wait_list, (void *)&to_wake ) )
		{
			futex_queue_repush( out, to_wake );
			sched_yield();
		}
	}
	/* ELSE Nothing to do we are already on the same queue */

	sctk_spinlock_unlock( &fq->queue_is_wake_tainted );
	
	
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


int * futex_queue_HT_register_thread( struct futex_queue_HT * ht , int * futex_key  , int bitmask )
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
		return futex_queue_HT_register_thread( ht , futex_key, bitmask );
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
		ret = futex_queue_push( fq , bitmask );
	}
	
	/* Release the read lock */
	sctk_atomics_decr_int( &ht->queue_table_is_being_manipulated);
	
	sched_yield();
	
	return ret;
}


int futex_queue_HT_requeue_threads( struct futex_queue_HT * ht , 
									int * in_futex_key,
									int * out_futex_key,
									int count )
{	
	int ret = 0;

	/* Acquire the read lock */
	sctk_atomics_incr_int( &ht->queue_table_is_being_manipulated);
	
	/* Here we first create a futex queue if 
	 * it does not exists yet */
	sctk_uint64_t in_key = (sctk_uint64_t) in_futex_key;
	sctk_uint64_t out_key = (sctk_uint64_t) out_futex_key;
	
	int new_queue_created = 0;
	struct futex_queue *in_fq = (struct futex_queue *) 
	          MPCHT_get_or_create(&ht->queue_hash_table,
								   in_key ,
								   futex_queue_new_from_key,
								   &new_queue_created);
	/* Was a new queue created ? */
	if( new_queue_created )
		ht->queue_count++;
	
	new_queue_created = 0;
	struct futex_queue *out_fq = (struct futex_queue *) 
	          MPCHT_get_or_create(&ht->queue_hash_table,
								   out_key ,
								   futex_queue_new_from_key,
								   &new_queue_created);
	/* Was a new queue created ? */
	if( new_queue_created )
		ht->queue_count++;

	/* Did it work ? */
	if( in_fq && out_fq )
	{
		/* Here we are now manipulating a futex queue */
		ret = futex_queue_requeue( in_fq, out_fq, count );
	}
	
	/* Release the read lock */
	sctk_atomics_decr_int( &ht->queue_table_is_being_manipulated);
	
	return ret;	
	
}



int futex_queue_HT_wake_threads( struct futex_queue_HT * ht , int * futex_key , int bitmask, int use_mask, int count )
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
		ret = futex_queue_wake( fq, bitmask, use_mask, count );
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
static volatile int context_init_done = 0;

void sctk_futex_context_init()
{
	if( context_init_done )
		return;
	
	context_init_done = 1;
	
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
	int * wait_ticket = futex_queue_HT_register_thread( &___futex_queue_HT, addr1, 0 );
	
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
	
	int * wait_ticket = futex_queue_HT_register_thread( &___futex_queue_HT, addr1 , 0);
	
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
	return futex_queue_HT_wake_threads(  &___futex_queue_HT, addr1, 0, 0 /* No bitmask */,  val1 );
}


void sctk_futex_WAKE_OP_decode( int val3, int * op, int * cmp, int * oparg, int * cmparg )
{
	int opcode, cmpcode, opargcode, cmpargcode;

	opcode = (val3 >> 28 ) & 0xf;
	cmpcode = (val3 >> 24 ) & 0xf;
	opargcode = (val3 >> 12 ) & 0xfff;
	cmpargcode = val3 & 0xfff;
	
	*cmp = sctk_do_translate_cmp( cmpargcode );
	*op = sctk_do_translate_futex_opcode( opcode );
	
	if( sctk_do_translate_futex_opcode_for_shift( opcode ) == 0 )
	{
		*oparg = opargcode;
	}
	else
	{
		/* FUTEX_OP_ARG_SHIFT was set */
		*oparg = (1 << opargcode);
	}
}


int sctk_futex_WAKE_OP_do_op( int oldval, int val3 )
{
	int op, cmp, oparg, cmparg;
	
	sctk_futex_WAKE_OP_decode( val3, &op, &cmp, &oparg, &cmparg );
	
	switch( op )
	{
		case SCTK_FUTEX_OP_SET:
			return oparg;
		break;
		case SCTK_FUTEX_OP_ADD:
			return oldval + oparg;
		break;
		case SCTK_FUTEX_OP_OR:
			return oldval | oparg;
		break;
		case SCTK_FUTEX_OP_ANDN:
			return oldval & oparg;
		break;
		case SCTK_FUTEX_OP_XOR:
			return oldval ^ oparg;
		break;
		default:
				sctk_fatal("No such operation");
	}

	return oparg;
}

int sctk_futex_WAKE_OP_do_cmp( int oldval, int val3 )
{
	int op, cmp, oparg, cmparg;
	
	sctk_futex_WAKE_OP_decode( val3, &op, &cmp, &oparg, &cmparg );
	
	switch( op )
	{
		case SCTK_FUTEX_OP_CMP_EQ:
			return (oldval == oparg);
		break;
		case SCTK_FUTEX_OP_CMP_NE:
			return (oldval != oparg);
		break;
		case SCTK_FUTEX_OP_CMP_LT:
			return (oldval < oparg);
		break;
		case SCTK_FUTEX_OP_CMP_LE:
			return (oldval <= oparg);
		break;
		case SCTK_FUTEX_OP_CMP_GT:
			return (oparg < oldval);
		break;
		case SCTK_FUTEX_OP_CMP_GE:
			return  ( oparg <= oldval);
		break;
		default:
				sctk_fatal("No such operation");
	}

	return oparg;
}

int sctk_futex_WAKE_OP(void *addr1, int op, void *addr2, int val1, int val2, int val3  )
{
	int oldval = *(int *) addr2;
	
	*((int *) addr2) = sctk_futex_WAKE_OP_do_op( oldval, val3 );
	
	sctk_futex_WAKE(addr1, SCTK_FUTEX_WAKE, val1 );

	if( sctk_futex_WAKE_OP_do_cmp( oldval, val3 ) )
		sctk_futex_WAKE(addr2, SCTK_FUTEX_WAKE, val2 );

}


int sctk_futex_REQUEUE(void *addr1, int op, int val1, void * addr2 )
{
	return futex_queue_HT_requeue_threads( &___futex_queue_HT, 
										   addr1,
										   addr2,
										   val1 );
}

int sctk_futex_CMPREQUEUE(void *addr1, int op, int val1, void * addr2, int val3 )
{
	/* First check the value */
	OPA_int_t * pold_val = (OPA_int_t *)addr1;
	
	int old_val = sctk_atomics_load_int( pold_val );
	
	/* Val is different directly return */
	if( old_val != val3 )
	{
		errno = EWOULDBLOCK;
		return -1;
	}
	
	return futex_queue_HT_requeue_threads( &___futex_queue_HT, 
										   addr1,
										   addr2,
										   val1 );
}


int sctk_futex(void *addr1, int op, int val1, 
               struct timespec *timeout, void *addr2, int val3)
{
	int ret = 0;

	/* We need to convert only if futexes
	 * are supported by the system
	 * othewise the value is already the
	 * internal symbol this is handled
	 * inside this function */
	op = sctk_do_translate_futex_op( op );
	
	unsigned long tmp_val2 = (unsigned long)timeout;
	int val2 = (int)tmp_val2;
			
	switch( op )
	{
		case SCTK_FUTEX_WAIT :
			ret = sctk_futex_WAIT(addr1, op, val1, timeout );
		break;
		case SCTK_FUTEX_WAKE :
			ret = sctk_futex_WAKE(addr1, op, val1 );
		break;
		case SCTK_FUTEX_WAKE_OP :
			ret = sctk_futex_WAKE_OP(addr1, op, addr2, val1, val2, val3 );
		break;
		case SCTK_FUTEX_REQUEUE :
			ret = sctk_futex_REQUEUE(addr1, op, val1, addr2 );
		break;
		case SCTK_FUTEX_CMP_REQUEUE :
			ret = sctk_futex_CMPREQUEUE(addr1, op, val1, addr2, val3 );
		break;
		default:
			sctk_fatal("Not implemented YET");
	}

	
	sched_yield();
	
	return ret;
}



