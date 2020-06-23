#include <mpc_common_spinlock.h>


/*******************************
 * MPC SPINLOCK IMPLEMENTATION *
 *******************************/


int mpc_common_spinlock_lock_yield( mpc_common_spinlock_t *lock )
{
	OPA_int_t *p = (OPA_int_t *) lock;
	while ( expect_true( sctk_test_and_set( p ) ) )
	{
		while ( OPA_load_int(p) )
		{
			int i;

			mpc_thread_yield();

			for ( i = 0; ( OPA_load_int(p) ) && ( i < 100 ); i++ )
			{
				sctk_cpu_relax();
			}
		}
	}


	return 0;
}

int mpc_common_spinlock_lock( mpc_common_spinlock_t *lock )
{
	OPA_int_t *p =  (OPA_int_t *) lock;
	while ( expect_true( sctk_test_and_set( p ) ) )
	{
		do
		{
			sctk_cpu_relax();
		} while ( OPA_load_int(p) );
	}


	return 0;
}

int _mpc_common_spinlock_trylock( mpc_common_spinlock_t *lock )
{
	OPA_int_t *p =  (OPA_int_t *) lock;
	return sctk_test_and_set( p );

	return 1;
}

int mpc_common_spinlock_unlock( mpc_common_spinlock_t *lock )
{
	mpc_common_spinlock_init(lock, 0);
    return 0;
}

/**********************************
 * MPC RW SPINLOCK IMPLEMENTATION *
 **********************************/

int mpc_common_spinlock_read_lock( mpc_common_rwlock_t *lock )
{
	mpc_common_spinlock_lock( &( lock->writer_lock ) );
	OPA_incr_int( &( lock->reader_number ) );
	mpc_common_spinlock_unlock( &( lock->writer_lock ) );
	return 0;
}

int mpc_common_spinlock_read_lock_yield( mpc_common_rwlock_t *lock )
{
	mpc_common_spinlock_lock_yield( &( lock->writer_lock ) );
	OPA_incr_int( &( lock->reader_number ) );
	mpc_common_spinlock_unlock( &( lock->writer_lock ) );
	return 0;
}

int mpc_common_spinlock_read_trylock( mpc_common_rwlock_t *lock )
{
	if ( mpc_common_spinlock_trylock( &( lock->writer_lock ) ) == 0 )
	{
		OPA_incr_int( &( lock->reader_number ) );
		mpc_common_spinlock_unlock( &( lock->writer_lock ) );
		return 0;
	}

	return 1;
}

int mpc_common_spinlock_write_lock( mpc_common_rwlock_t *lock )
{
	mpc_common_spinlock_lock( &( lock->writer_lock ) );
	while ( expect_true( OPA_load_int( &( lock->reader_number ) ) != 0 ) )
	{
		sctk_cpu_relax();
	}
	return 0;
}

int mpc_common_spinlock_write_trylock( mpc_common_rwlock_t *lock )
{
	if ( mpc_common_spinlock_trylock( &( lock->writer_lock ) ) == 0 )
	{

		while ( expect_true( OPA_load_int( &( lock->reader_number ) ) != 0 ) )
		{
			sctk_cpu_relax();
		}

		return 0;
	}

	return 1;
}

int mpc_common_spinlock_write_lock_yield( mpc_common_rwlock_t *lock )
{
	int cnt = 0;
	mpc_common_spinlock_lock_yield( &( lock->writer_lock ) );
	while ( expect_true( OPA_load_int( &( lock->reader_number ) ) != 0 ) )
	{
		sctk_cpu_relax();
		if ( !( ++cnt % 1000 ) )
		{
			mpc_thread_yield();
		}
	}
	return 0;
}

int mpc_common_spinlock_read_unlock( mpc_common_rwlock_t *lock )
{
	return OPA_fetch_and_add_int( &( lock->reader_number ), -1 ) - 1;
}

int mpc_common_spinlock_read_unlock_state( mpc_common_rwlock_t *lock )
{
	return OPA_load_int( &( lock->reader_number ) );
}

int mpc_common_spinlock_write_unlock( mpc_common_rwlock_t *lock )
{
	mpc_common_spinlock_unlock( &( lock->writer_lock ) );
	return 0;
}