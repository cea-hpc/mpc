#ifndef MPC_MPI_EGREQ_PROGRESS_H_
#define MPC_MPI_EGREQ_PROGRESS_H_

#include "mpc_common_spinlock.h"

typedef enum {
	PWU_NO_PROGRESS = 0,
	PWU_DID_PROGRESS = 1,
	PWU_WORK_DONE = 2
} sctk_progress_workState;

struct _mpc_egreq_progress_work_unit
{
	int ( *fn )( void *param );
	void *param;
	volatile int done;
	int is_free;
	mpc_common_spinlock_t unit_lock;
};

int _mpc_egreq_progress_work_unit_poll( struct _mpc_egreq_progress_work_unit *pwu );

#define MPC_EGREQ_PWU_STATIC_ARRAY 16

struct _mpc_egreq_progress_list
{
	struct _mpc_egreq_progress_work_unit works[MPC_EGREQ_PWU_STATIC_ARRAY];
	volatile unsigned int work_count;
	mpc_common_spinlock_t list_lock;
	int is_free;
	unsigned int no_work_count;
	int id;
};

struct _mpc_egreq_progress_work_unit *_mpc_egreq_progress_list_add( struct _mpc_egreq_progress_list *pl, int ( *fn )( void *param ), void *param );
int _mpc_egreq_progress_list_poll( struct _mpc_egreq_progress_list *pl );

struct _mpc_egreq_progress_pool
{
	struct _mpc_egreq_progress_list *lists;
	mpc_common_spinlock_t pool_lock;
	unsigned int size;
	unsigned int booked;
};

int _mpc_egreq_progress_pool_init( struct _mpc_egreq_progress_pool *p, unsigned int size );
int _mpc_egreq_progress_pool_release( struct _mpc_egreq_progress_pool *p );

struct _mpc_egreq_progress_list *_mpc_egreq_progress_pool_join( struct _mpc_egreq_progress_pool *p );
int _mpc_egreq_progress_pool_poll( struct _mpc_egreq_progress_pool *p, int my_id );

#endif /* MPC_MPI_EGREQ_PROGRESS_H_ */
