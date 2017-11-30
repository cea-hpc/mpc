#include "progress_engine.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
/*
 * The Work Unit
 */


static inline int progressWorkUnit_isfree( struct progressWorkUnit * pwu )
{
    int ret = 0;
    sctk_spinlock_lock( &pwu->unit_lock );
    ret = pwu->is_free;
    sctk_spinlock_unlock( &pwu->unit_lock );
    return ret;
}


static inline void progressWorkUnit_setfree( struct progressWorkUnit * pwu, int is_free)
{
    sctk_spinlock_lock( &pwu->unit_lock );
    pwu->is_free = is_free;
    sctk_spinlock_unlock( &pwu->unit_lock );
}



static inline void progressWorkUnit_clear( struct progressWorkUnit *pwu )
{
    pwu->fn = NULL;
    pwu->param = NULL;
    progressWorkUnit_setfree( pwu, 1 );
}


int progressWorkUnit_init( struct progressWorkUnit *pwu )
{
    pwu->unit_lock = 0;
    progressWorkUnit_clear( pwu );

    return 0;
}


int progressWorkUnit_release( struct progressWorkUnit *pwu )
{
    progressWorkUnit_clear( pwu );
    pwu->unit_lock = 0;

    return 0;
}



int progressWorkUnit_acquire( struct progressWorkUnit *pwu, int (*fn)(  void * param), void * param )
{
    if( progressWorkUnit_isfree(pwu) == 0 )
        return 1;

    pwu->fn = fn;
    pwu->param = param;

    pwu->done = 0;
    progressWorkUnit_setfree( pwu, 0 );

    return 0;
}

int progressWorkUnit_relax( struct progressWorkUnit *pwu )
{
    progressWorkUnit_clear( pwu );
    return 0;
}

int progressWorkUnit_poll( struct progressWorkUnit *pwu )
{
    int ret = PWU_NO_PROGRESS;


    if( sctk_spinlock_trylock(&pwu->unit_lock) == 0 ) {

        if( !pwu->done && !pwu->is_free)
        {
            pwu->done = (pwu->fn)( pwu->param);
        
            ret = PWU_DID_PROGRESS;
    
            if( pwu->done )
            {
                ret = PWU_WORK_DONE;
            }
        }
    
        sctk_spinlock_unlock(&pwu->unit_lock);
    }

    return ret;
}


int progressWorkUnit_complete( struct progressWorkUnit *pwu )
{
    if( progressWorkUnit_isfree(pwu) )
        return 0;

    while( pwu->done == 0 )
    {
        progressWorkUnit_poll( pwu );
    }

    return 0;
}

/*
 * The Work List
 */

int progressList_init( struct progressList * pl )
{
    /* Init works */
    int i;
    for (i = 0; i < PROGRESS_PWU_STATIC_ARRAY; ++i) {
        progressWorkUnit_init( &pl->works[i] );
    }

    pl->work_count = 0;
    pl->no_work_count = 0;

    pl->list_lock = 0;

    pl->is_free = 1;

    return 0;
}


int progressList_release( struct progressList * pl )
{
    int i;
    for (i = 0; i < PROGRESS_PWU_STATIC_ARRAY; ++i) {
        progressWorkUnit_complete( &pl->works[i] );
        progressWorkUnit_release( &pl->works[i] );
    }

    pl->list_lock = 0;

    pl->work_count = 0;
    pl->no_work_count = 0;

    return 0;
}


struct progressWorkUnit * progressList_acquire( struct progressList * pl, int (*fn)(void * param), void * param  )
{
    int acquired = 0;
    struct progressWorkUnit * ret = NULL;

    do
    {
        int current_w_count = 0;
        do
        {
            sctk_spinlock_lock( &pl->list_lock );
            current_w_count = pl->work_count;
            sctk_spinlock_unlock( &pl->list_lock );

            if( PROGRESS_PWU_STATIC_ARRAY == current_w_count )
            {
                /* Poll to try to free a slot */
                progressList_poll( pl );
            }

        }while( PROGRESS_PWU_STATIC_ARRAY == current_w_count );


        /* We have a window now try to acquire */
        sctk_spinlock_lock( &pl->list_lock );

        if( pl->work_count < PROGRESS_PWU_STATIC_ARRAY )
        {
            /* There is a free slot where is it ? */
            int i;
            for( i = 0 ; i < PROGRESS_PWU_STATIC_ARRAY; i++)
            {
                if( pl->works[i].is_free )
                {
                    progressWorkUnit_acquire( &pl->works[i], fn, param );
                    ret=  &pl->works[i];
                    pl->work_count++;

                    /* All done */
                    acquired = 1;
                }
            }
        }

        sctk_spinlock_unlock( &pl->list_lock );

    }while( !acquired );

    return ret;
}

struct progressWorkUnit * progressList_add( struct progressList * pl, int (*fn)( void * param), void * param )
{
    struct progressWorkUnit * pwu = progressList_acquire( pl, fn, param );
    return pwu;
}


#define PROGRESS_THRESH 3


int progressList_poll( struct progressList * pl )
{
    int did_work = PWU_NO_PROGRESS;

    if( pl->work_count == 0 )
    {
        pl->no_work_count++;
        return did_work;
    }

    int i;
    int progress_count = 0;


    for (i = 0; i < PROGRESS_PWU_STATIC_ARRAY; ++i) {
        
        int is_free = 0;

        if( progressWorkUnit_isfree( &pl->works[i]) )
            continue;

        int ret = progressWorkUnit_poll( &pl->works[i] );

        if( ret == PWU_DID_PROGRESS )
        {
            did_work |= PWU_DID_PROGRESS;
            progress_count++;
        

            if( PROGRESS_THRESH <= progress_count )
            {
                break;
            }
        }
        else if( ret == PWU_WORK_DONE )
        {
            did_work |= 1;
            pl->no_work_count=0;

            sctk_spinlock_lock( &pl->list_lock );
            /* We need to free this slot */
            progressWorkUnit_relax( &pl->works[i] );
            pl->work_count--;
            sctk_spinlock_unlock( &pl->list_lock );

            return PWU_WORK_DONE;
        }
    }

    return did_work;
}

/*
 * The Progress Pool
 */


int progressEnginePool_init( struct progressEnginePool * p, unsigned int size )
{

    if( size <= PROGRESS_POLL_ENGINE_STATIC_ARRAY )
    {
        p->lists = p->__lists;
    }
    else
    {
        p->lists = malloc( sizeof(struct progressList) * size );

        if( !p->lists )
        {
            perror("malloc");
            return 1;
        }
    }


    p->size = size;
    p->booked = 0;

    int i;

    for( i = 0 ; i < p->size ; i++ )
    {
        if( progressList_init(&p->lists[i]) )
        {
            return 1;
        }
    }

    p->pool_lock = 0;


    return 0;
}


int progressEnginePool_release( struct progressEnginePool * p)
{
    p->size = 0;
    p->booked = 0;

    int i;

    for( i = 0 ; i < p->size ; i++ )
    {
        if( progressList_release(&p->lists[i]) )
        {
            return 1;
        }
    }


    p->pool_lock = 0;

    if( p->lists != p->__lists )
    {
        free( p->lists );
        p->lists = NULL;
    }

    return 0;
}

struct progressList * progressEnginePool_join( struct progressEnginePool * p )
{

    struct progressList * ret = NULL;

    sctk_spinlock_lock( &p->pool_lock );

    if( p->booked == p->size )
    {
        fprintf(stderr, "Error no free pool\n");
        abort();
    }
    
    ret = &p->lists[ p->booked ];
    p->booked++;

    sctk_spinlock_unlock( &p->pool_lock );


    return ret;
}


int progressEnginePool_poll( struct progressEnginePool * p, int my_id )
{
    if( !p->booked )
        return PWU_NO_PROGRESS;

    /* Try locally */

    int targ = -1;

    if( my_id < 0 )
    {
        /* I have no attachment lets poll randomly */
        targ = rand() % p->booked;
    }
    else
    {
        /* I'm space located poll myself first */
        targ = my_id % p->booked;
    }


    int ret = progressList_poll( &p->lists[targ] );

    if( ret != PWU_NO_PROGRESS )
    {
        /* I did progress return */
        return ret;
    }

    if( my_id < 0 )
    {
        /* Not local don't steal */
        return ret;
    }

    /* If I'm here I did no progress */

    if( p->lists[targ].no_work_count < 1024 )
        return ret;

    /* Try to steal progress neighbor */
    targ = rand() % p->booked;

    ret = progressList_poll( &p->lists[targ] );

    if( ret != PWU_NO_PROGRESS )
    {
        p->lists[targ].no_work_count = 0;
        /* I did progress return */
    }

    return ret;
}



#include <omp.h>

#define NUM_WORK 10

int vals[NUM_WORK] = {0};

int do_work( void * pval )
{
    int * val = (int *)pval;

    int id = omp_get_thread_num();
    //printf("%d done a quantum @ %d\n", id, *val);
    usleep(100);
    *val = *val + 1;

    if( 10000 <= *val )
    {
        printf("BLOCK DONE %d\n", id);
        /* Done */
        return 1;
    }

    /* Not done */
    return 0;
}



#if 0
int main(int argc, char *argv[])
{
    struct progressEnginePool pool;

    int num = -1;
#pragma omp parallel
{
#pragma omp master
    num = omp_get_num_threads();
}
    printf("%d\n", num);

    progressEnginePool_init(&pool, num);

#pragma omp parallel
{

    struct progressList * pl = progressEnginePool_join( &pool );

    int id = omp_get_thread_num();

    printf("Hello %d\n", id);

    int i;
    
    if( id == 0)
    {
        for (i = 0; i < NUM_WORK; ++i) {
            progressList_add( pl, do_work, (void*)&vals[i] );
            
        }
    }

    for (i = 0; i < 1e6; ++i) {
        progressEnginePool_poll( &pool, id );
    }

}



    progressEnginePool_release(&pool);


    return 0;
}
#endif

