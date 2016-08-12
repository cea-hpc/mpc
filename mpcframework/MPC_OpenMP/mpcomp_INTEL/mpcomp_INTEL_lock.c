#include "mpcomp_INTEL_types.h"
#include "mpcomp_internal.h"

/*
 * Lock structure
 */
typedef struct omp_lock_t {
  void* lk;
} iomp_lock_t;


/*
 *  * Lock interface routines (fast versions with gtid passed in)
 *   */
void
__kmpc_init_lock( ident_t *loc, kmp_int32 gtid,  void **user_lock )
{
  sctk_debug( "[%d] __kmpc_init_lock: "
      "Addr %p",
      ((mpcomp_thread_t *) sctk_openmp_thread_tls)->rank, user_lock ) ;

  if( user_lock == NULL ) {
    sctk_error("__kmpc_init_lock: Null lock reference");
    sctk_abort();
  }

  ((iomp_lock_t*) user_lock)->lk = (void*) sctk_malloc( sizeof( omp_lock_t ));
  omp_init_lock( (omp_lock_t *) ((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_init_nest_lock: Null lock reference");
    sctk_abort();
  }

  ((iomp_lock_t*) user_lock)->lk = (void*) sctk_malloc( sizeof( omp_nest_lock_t ));
  omp_init_nest_lock( (omp_nest_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_destroy_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  omp_lock_t* llk;

  if( user_lock == NULL ) {
    sctk_error("__kmpc_destroy_lock: Null lock reference");
    sctk_abort();
  }

  llk = ((iomp_lock_t*) user_lock)->lk;
  omp_destroy_lock( llk );
  free( llk );
  llk = NULL;
}

void
__kmpc_destroy_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  omp_nest_lock_t* llk;

  if( user_lock == NULL ) {
    sctk_error("__kmpc_destroy_nest_lock: Null lock reference");
    sctk_abort();
  }

  llk = ((iomp_lock_t*) user_lock)->lk;
  omp_destroy_nest_lock( llk );
  free( llk );
  llk = NULL;
}

void
__kmpc_set_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_set_lock: Null lock reference");
    sctk_abort();
  }

  omp_set_lock( (omp_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_set_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_set_nest_lock: Null lock reference");
    sctk_abort();
  }

  omp_set_nest_lock( (omp_nest_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_unset_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_unset_lock: Null lock reference");
    sctk_abort();
  }

  omp_unset_lock( (omp_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}

void
__kmpc_unset_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_unset_nest_lock: Null lock reference");
    sctk_abort();
  }

  omp_unset_nest_lock( (omp_nest_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}

int
__kmpc_test_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_test_lock: Null lock reference");
    sctk_abort();
  }

  return omp_test_lock( (omp_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}

int
__kmpc_test_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock )
{
  if( user_lock == NULL ) {
    sctk_error("__kmpc_test_nest_lock: Null lock reference");
    sctk_abort();
  }
  return omp_test_nest_lock( (omp_nest_lock_t*) ((iomp_lock_t*) user_lock)->lk );
}
