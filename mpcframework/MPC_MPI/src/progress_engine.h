#ifndef PROGRESS_ENGINE_H
#define PROGRESS_ENGINE_H

#include "sctk_spinlock.h"
#include "sctk_thread.h"

typedef enum
{
    PWU_NO_PROGRESS = 0,
    PWU_DID_PROGRESS = 1,
    PWU_WORK_DONE = 2
}progressWorkState;

struct progressWorkUnit
{
    int (*fn)( void * param);
    void * param;
    volatile int done;
    int is_free;
    sctk_spinlock_t unit_lock;
};

int progressWorkUnit_init( struct progressWorkUnit *pwu );
int progressWorkUnit_release( struct progressWorkUnit *pwu );

int progressWorkUnit_acquire( struct progressWorkUnit *pwu, int (*fn)( void * param), void * param );
int progressWorkUnit_relax( struct progressWorkUnit *pwu );

int progressWorkUnit_poll( struct progressWorkUnit *pwu );
int progressWorkUnit_complete( struct progressWorkUnit *pwu );


#define PROGRESS_PWU_STATIC_ARRAY 32

struct progressList
{
    struct progressWorkUnit works[PROGRESS_PWU_STATIC_ARRAY];
    volatile unsigned int work_count;
    sctk_spinlock_t list_lock;
    int is_free;
    unsigned int no_work_count;
    char __pad[64];
};

int progressList_init( struct progressList * pl );
int progressList_release( struct progressList * pl );

struct progressWorkUnit * progressList_add( struct progressList * pl, int (*fn)( void * param), void * param );
int progressList_poll( struct progressList * pl );


#define PROGRESS_POLL_ENGINE_STATIC_ARRAY 4

struct progressEnginePool
{
    struct progressList __lists[PROGRESS_PWU_STATIC_ARRAY];
    struct progressList *lists;
    sctk_spinlock_t pool_lock;
    unsigned int size;
    unsigned int booked;
};


int progressEnginePool_init( struct progressEnginePool * p, unsigned int size );
int progressEnginePool_release( struct progressEnginePool * p);

struct progressList * progressEnginePool_join( struct progressEnginePool * p );
int progressEnginePool_poll( struct progressEnginePool * p, int my_id );




#endif /* PROGRESS_ENGINE_H */
