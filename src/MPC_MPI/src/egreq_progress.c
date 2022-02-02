#include "egreq_progress.h"

#include "mpc_common_debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sctk_alloc.h>

/*
 * The Work Unit
 */

static inline int __mpc_egreq_progress_work_unit_isfree( struct _mpc_egreq_progress_work_unit *pwu )
{
	int ret = 0;
	if ( mpc_common_spinlock_trylock( &pwu->unit_lock ) == 0 )
	{
		ret = pwu->is_free;
		mpc_common_spinlock_unlock( &pwu->unit_lock );
	}
	return ret;
}

static inline void __mpc_egreq_progress_work_unit_setfree( struct _mpc_egreq_progress_work_unit *pwu, int is_free )
{
	pwu->is_free = is_free;
}

static inline void __mpc_egreq_progress_work_unit_clear( struct _mpc_egreq_progress_work_unit *pwu )
{
	pwu->fn = NULL;
	pwu->param = NULL;
	__mpc_egreq_progress_work_unit_setfree( pwu, 1 );
}

static inline int __mpc_egreq_progress_work_unit_init( struct _mpc_egreq_progress_work_unit *pwu )
{
	mpc_common_spinlock_init(&pwu->unit_lock, 0);
	__mpc_egreq_progress_work_unit_clear( pwu );

	return 0;
}

static inline int __mpc_egreq_progress_work_unit_release( struct _mpc_egreq_progress_work_unit *pwu )
{
	__mpc_egreq_progress_work_unit_clear( pwu );
	mpc_common_spinlock_init(&pwu->unit_lock, 0);

	return 0;
}

static inline int __mpc_egreq_progress_work_unit_acquire( struct _mpc_egreq_progress_work_unit *pwu, int ( *fn )( void *param ), void *param )
{
	if ( __mpc_egreq_progress_work_unit_isfree( pwu ) == 0 )
		return 1;

	pwu->fn = fn;
	pwu->param = param;

	pwu->done = 0;
	__mpc_egreq_progress_work_unit_setfree( pwu, 0 );

	return 0;
}

static inline int __mpc_egreq_progress_work_unit_relax( struct _mpc_egreq_progress_work_unit *pwu )
{
	__mpc_egreq_progress_work_unit_clear( pwu );
	return 0;
}

int _mpc_egreq_progress_work_unit_poll( struct _mpc_egreq_progress_work_unit *pwu )
{
	int ret = PWU_NO_PROGRESS;

	if ( mpc_common_spinlock_trylock( &pwu->unit_lock ) == 0 )
	{

		if ( !pwu->is_free )
		{
			if ( !pwu->done )
				pwu->done = ( pwu->fn )( pwu->param );

			ret = PWU_DID_PROGRESS;

			if ( pwu->done )
			{
				__mpc_egreq_progress_work_unit_relax( pwu );
				ret = PWU_WORK_DONE;
			}
		}

		mpc_common_spinlock_unlock( &pwu->unit_lock );
	}

	return ret;
}

static inline int __mpc_egreq_progress_work_unit_complete( struct _mpc_egreq_progress_work_unit *pwu )
{
	if ( __mpc_egreq_progress_work_unit_isfree( pwu ) )
		return 0;

	while ( pwu->done == 0 )
	{
		_mpc_egreq_progress_work_unit_poll( pwu );
	}

	return 0;
}

/*
 * The Work List
 */

static inline int __mpc_egreq_progress_list_init( struct _mpc_egreq_progress_list *pl )
{
	/* Init works */
	int i;
	for ( i = 0; i < MPC_EGREQ_PWU_STATIC_ARRAY; ++i )
	{
		__mpc_egreq_progress_work_unit_init( &pl->works[i] );
	}

	pl->work_count = 0;
	pl->no_work_count = 0;

	mpc_common_spinlock_init(&pl->list_lock, 0);

	pl->is_free = 1;

	return 0;
}

static inline int __mpc_egreq_progress_list_release( struct _mpc_egreq_progress_list *pl )
{
	int i;
	for ( i = 0; i < MPC_EGREQ_PWU_STATIC_ARRAY; ++i )
	{
		__mpc_egreq_progress_work_unit_complete( &pl->works[i] );
		__mpc_egreq_progress_work_unit_release( &pl->works[i] );
	}

	mpc_common_spinlock_init(&pl->list_lock, 0);

	pl->work_count = 0;
	pl->no_work_count = 0;

	return 0;
}

static inline struct _mpc_egreq_progress_work_unit *__mpc_egreq_progress_list_acquire( struct _mpc_egreq_progress_list *pl, int ( *fn )( void *param ), void *param )
{
	int acquired = 0;
	struct _mpc_egreq_progress_work_unit *ret = NULL;

	do
	{
		int current_w_count = 0;
		do
		{
			mpc_common_spinlock_lock( &pl->list_lock );
			current_w_count = pl->work_count;
			mpc_common_spinlock_unlock( &pl->list_lock );

			if ( MPC_EGREQ_PWU_STATIC_ARRAY == current_w_count )
			{
				/* Poll to try to free a slot */
				_mpc_egreq_progress_list_poll( pl );
			}

		} while ( MPC_EGREQ_PWU_STATIC_ARRAY == current_w_count );

		/* We have a window now try to acquire */
		mpc_common_spinlock_lock( &pl->list_lock );

		if ( pl->work_count < MPC_EGREQ_PWU_STATIC_ARRAY )
		{
			/* There is a free slot where is it ? */
			int i;
			for ( i = 0; i < MPC_EGREQ_PWU_STATIC_ARRAY; i++ )
			{
				if ( pl->works[i].is_free )
				{
					__mpc_egreq_progress_work_unit_acquire( &pl->works[i], fn, param );
					ret = &pl->works[i];
					pl->work_count++;

					/* All done */
					acquired = 1;
					break;
				}
			}
		}

		mpc_common_spinlock_unlock( &pl->list_lock );

	} while ( !acquired );

	return ret;
}

struct _mpc_egreq_progress_work_unit *_mpc_egreq_progress_list_add( struct _mpc_egreq_progress_list *pl, int ( *fn )( void *param ), void *param )
{
	struct _mpc_egreq_progress_work_unit *pwu = __mpc_egreq_progress_list_acquire( pl, fn, param );
	return pwu;
}

#define PROGRESS_THRESH 5

int _mpc_egreq_progress_list_poll( struct _mpc_egreq_progress_list *pl )
{
	int did_work = PWU_NO_PROGRESS;

	if ( pl->work_count == 0 )
	{
		pl->no_work_count++;
		return did_work;
	}

	int i;
	int progress_count = 0;

	for ( i = 0; i < MPC_EGREQ_PWU_STATIC_ARRAY; ++i )
	{

		if ( __mpc_egreq_progress_work_unit_isfree( &pl->works[i] ) )
			continue;

		int ret = _mpc_egreq_progress_work_unit_poll( &pl->works[i] );

		if ( ret == PWU_DID_PROGRESS )
		{
			did_work |= PWU_DID_PROGRESS;
			progress_count++;

			if ( PROGRESS_THRESH <= progress_count )
			{
				break;
			}
		}
		else if ( ret == PWU_WORK_DONE )
		{
			did_work |= 1;
			pl->no_work_count--;

			mpc_common_spinlock_lock( &pl->list_lock );
			/* We need to free this slot */
			pl->work_count--;
			mpc_common_spinlock_unlock( &pl->list_lock );

			return PWU_WORK_DONE;
		}
	}

	return did_work;
}

/*
 * The Progress Pool
 */

int _mpc_egreq_progress_pool_init( struct _mpc_egreq_progress_pool *p, unsigned int size )
{

	p->lists = sctk_malloc( sizeof( struct _mpc_egreq_progress_list ) * size );

	if ( !p->lists )
	{
		perror( "sctk_malloc" );
		return 1;
	}

	p->size = size;
	p->booked = 0;

	unsigned int i;

	for ( i = 0; i < p->size; i++ )
	{
		if ( __mpc_egreq_progress_list_init( &p->lists[i] ) )
		{
			return 1;
		}
	}

	mpc_common_spinlock_init(&p->pool_lock, 0);

	return 0;
}

int _mpc_egreq_progress_pool_release( struct _mpc_egreq_progress_pool *p )
{
	p->size = 0;
	p->booked = 0;

	unsigned int i;

	for ( i = 0; i < p->size; i++ )
	{
		if ( __mpc_egreq_progress_list_release( &p->lists[i] ) )
		{
			return 1;
		}
	}

	mpc_common_spinlock_init(&p->pool_lock, 0);

	sctk_free( p->lists );
	p->lists = NULL;

	return 0;
}

struct _mpc_egreq_progress_list *_mpc_egreq_progress_pool_join( struct _mpc_egreq_progress_pool *p )
{
	struct _mpc_egreq_progress_list *ret = NULL;

	mpc_common_spinlock_lock( &p->pool_lock );

	if ( p->booked == p->size )
	{
		fprintf( stderr, "Error no free pool\n" );
		mpc_common_debug_abort();
	}

	ret = &p->lists[p->booked];
	ret->id = p->booked;
	p->booked++;

	mpc_common_spinlock_unlock( &p->pool_lock );

	return ret;
}

int _mpc_egreq_progress_pool_poll( struct _mpc_egreq_progress_pool *p, int my_id )
{
	//mpc_common_debug_error("POLL as %d", my_id);

	if ( !p->booked )
		return PWU_NO_PROGRESS;

	/* Try locally */

	int targ = -1;

	if ( my_id < 0 )
	{
		/* I have no attachment lets poll randomly */
		targ = rand() % p->booked;
	}
	else
	{
		/* I'm space located poll myself first */
		targ = my_id % p->booked;
	}

	int ret = _mpc_egreq_progress_list_poll( &p->lists[targ] );

	if ( ret != PWU_NO_PROGRESS )
	{
		/* I did progress return */
		return ret;
	}

	if ( my_id < 0 )
	{
		/* Not local don't steal */
		return ret;
	}

	/* If I'm here I did no progress */

	//if( p->lists[targ].no_work_count < 8192 )
	//    return ret;
	/* Try to steal progress neighbor */
	targ = rand() % p->booked;

	ret = _mpc_egreq_progress_list_poll( &p->lists[targ] );

	if ( ret != PWU_NO_PROGRESS )
	{
		p->lists[targ].no_work_count = 0;
		/* I did progress return */
	}

	return ret;
}
