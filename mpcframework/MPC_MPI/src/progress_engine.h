#ifndef PROGRESS_ENGINE_H
#define PROGRESS_ENGINE_H

#include "mpc_common_spinlock.h"
#include "sctk_thread.h"

typedef enum
{
    PWU_NO_PROGRESS = 0,
    PWU_DID_PROGRESS = 1,
    PWU_WORK_DONE = 2
}sctk_progress_workState;

struct sctk_progress_work_unit
{
    int (*fn)( void * param);
    void * param;
    volatile int done;
    int is_free;
    mpc_common_spinlock_t unit_lock;
};

int sctk_progress_work_unit_init( struct sctk_progress_work_unit *pwu );
int sctk_progress_work_unit_release( struct sctk_progress_work_unit *pwu );

int sctk_progress_work_unit_poll( struct sctk_progress_work_unit *pwu );
int sctk_progress_work_unit_complete( struct sctk_progress_work_unit *pwu );


#define PROGRESS_PWU_STATIC_ARRAY 16

struct sctk_progress_list
{
    struct sctk_progress_work_unit works[PROGRESS_PWU_STATIC_ARRAY];
    volatile unsigned int work_count;
    mpc_common_spinlock_t list_lock;
    int is_free;
    unsigned int no_work_count;
    int id;
};

int sctk_progress_list_init( struct sctk_progress_list * pl );
int sctk_progress_list_release( struct sctk_progress_list * pl );

struct sctk_progress_work_unit * sctk_progress_list_add( struct sctk_progress_list * pl, int (*fn)( void * param), void * param );
int sctk_progress_list_poll( struct sctk_progress_list * pl );


struct sctk_progress_engine_pool
{
    struct sctk_progress_list *lists;
    mpc_common_spinlock_t pool_lock;
    unsigned int size;
    unsigned int booked;
};


int sctk_progress_engine_pool_init( struct sctk_progress_engine_pool * p, unsigned int size );
int sctk_progress_engine_pool_release( struct sctk_progress_engine_pool * p);

struct sctk_progress_list * sctk_progress_engine_pool_join( struct sctk_progress_engine_pool * p );
int sctk_progress_engine_pool_poll( struct sctk_progress_engine_pool * p, int my_id );




#endif /* PROGRESS_ENGINE_H */
