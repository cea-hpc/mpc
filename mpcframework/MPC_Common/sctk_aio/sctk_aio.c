/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr					  # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_aio.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "sctk_debug.h"
#include "pthread.h"


#define MAX_SCTK_AIO_THREADS 16

/************************************************************************/
/* AIO Job List                                                         */
/************************************************************************/

struct sctk_aio_jobcell
{
	struct aiocb * job;
	struct sctk_aio_jobcell * prev;
};

struct sctk_aio_joblist
{
	struct sctk_aio_jobcell * head;
	struct sctk_aio_jobcell * tail;
	long long int counter;
	pthread_mutex_t joblistlock;
};

static inline int sctk_aio_joblist_empty( struct sctk_aio_joblist * jl )
{
	if( jl->tail || jl->head )
	{
		return 0;
	}
	
	return 1;
}



int sctk_aio_joblist_init( struct sctk_aio_joblist * jl )
{
	jl->head = NULL;
	jl->tail = NULL;
	jl->counter = 0;
	
	if( pthread_mutex_init( &jl->joblistlock , NULL) < 0 )
	{
		perror("pthread_mutex_init");
		return 1;
	}
	
	return 0;
}

int sctk_aio_joblist_release( struct sctk_aio_joblist * jl )
{
	if( !sctk_aio_joblist_empty( jl ) )
	{
		printf("Errror You are freing an unempty AIO joblist !");
		abort();
	}
	
	if( pthread_mutex_destroy( &jl->joblistlock ) < 0 )
	{
		perror("pthread_mutex_init");
		return 1;
	}
	
	jl->counter = 0;
	
	return 0;
}

int sctk_aio_joblist_cancel_fd( struct sctk_aio_joblist * jl, int fd )
{
	pthread_mutex_lock( &jl->joblistlock );
	
	struct sctk_aio_jobcell * current = jl->tail;
	
	/* Here any block in the queue has not 
	 * been processed yet as blocks are popped before being processed
	 * we can cancel them all without worying
	 * of the EINPROGRESS status */
	
	int did_cancel = 0;
	
	while( current )
	{
		if( current->job->aio_fildes == fd )
		{
			current->job->aio_lio_opcode = ECANCELED;
			did_cancel = 1;
		}
		
		current = current->prev;
	}
	
	pthread_mutex_unlock( &jl->joblistlock );
	
	return did_cancel?AIO_ALLDONE:AIO_CANCELED;
}

int sctk_aio_joblist_push( struct sctk_aio_joblist * jl, struct aiocb * job )
{
	/* Prepare the cell */
	struct sctk_aio_jobcell * new_cell = malloc( sizeof( struct sctk_aio_jobcell ) );
	
	if( !new_cell )
	{
		perror("malloc");
		return 1;
	}
	
	new_cell->job = job;
	
	pthread_mutex_lock( &jl->joblistlock );
	
			new_cell->prev = NULL;
			
			/* Enqueue in prev elem */
			if( jl->head )
			{
				jl->head->prev = new_cell;
			}
			
			jl->head = new_cell;
			
			/* First Elem */
			if( !jl->tail )
			{
				jl->tail = new_cell;
			}
			
			jl->counter++;
	
	pthread_mutex_unlock( &jl->joblistlock );
	
	return 0;
}

struct aiocb *  sctk_aio_joblist_pop( struct sctk_aio_joblist * jl )
{
	struct aiocb * ret = NULL;
	struct sctk_aio_jobcell * to_pop = NULL;
	
	pthread_mutex_lock( &jl->joblistlock );
	
		if( !jl->tail )
			goto sctk_aio_POP_END;
		
		to_pop = jl->tail;

		jl->tail = to_pop->prev;
		
		/* Last_Elem */
		if( !jl->tail )
			jl->head = NULL;
		
		jl->counter--;

sctk_aio_POP_END:
	pthread_mutex_unlock( &jl->joblistlock );
	
	if( to_pop )
	{
		ret = to_pop->job;
		free( to_pop );
	}
	
	return ret;
}





/************************************************************************/
/* AIO helpers                                                          */
/************************************************************************/
void sctk_aio_set_error( struct aiocb * cb, int error )
{
	if( !cb )
	{
		errno = EINVAL;
		return;
	}
	
	/* As the error field is not standardized
	 * we rely on the priority field which is 
	 * not used in our implementation */
	volatile int * errorfield = &cb->aio_reqprio;
	*errorfield = error;
}

void sctk_aio_get_error( struct aiocb * cb, int * error )
{
	if( !cb )
	{
		errno = EINVAL;
		return;
	}

	/* As the error field is not standardized
	 * we rely on the priority field which is 
	 * not used in our implementation */
	volatile int * errorfield = &cb->aio_reqprio;
	*error = *errorfield;
}


int sctk_aio_Write( struct aiocb * cb )
{
	ssize_t to_write = cb->aio_nbytes;
	ssize_t written = 0;
	
	while( 1 )
	{
		if( to_write <= 0 )
		{
			/* DONE */
			return 0;
		}
		
		errno = 0;

		ssize_t ret = pwrite(cb->aio_fildes, (const void *)cb->aio_buf, to_write, cb->aio_offset + written);
		
		if( ret < 0 )
		{
			if( errno == EINTR )
				continue;
			
			perror("pwrite");
			sctk_aio_set_error( cb, errno );
			return 1;
		}
		
		to_write -= ret;
		written += ret;
	}
	
	return 0;
}

int sctk_aio_Read( struct aiocb * cb )
{
	ssize_t to_read = cb->aio_nbytes;
	ssize_t read = 0;
	
	while( 1 )
	{
		if( to_read <= 0 )
		{
			/* DONE */
			return 0;
		}
		
		errno = 0;
		ssize_t ret = pread(cb->aio_fildes, (void *)cb->aio_buf, to_read, cb->aio_offset + read);
		
		if( ret < 0 )
		{
			if( errno == EINTR )
				continue;
			
			perror("pread");
			sctk_aio_set_error( cb, errno );
			return 1;
		}
		
		to_read -= ret;
		read += ret;
	}
	
	return 0;
}


struct sctk_aio_sigevent_wrapper
{
	union sigval sv;
	void  (*func)(union sigval);
};


void * sctk_aio_sigevent_detachfunction( void * pwrapper )
{
	/* This thread will not be joined */
	pthread_detach( pthread_self() );
	
	struct sctk_aio_sigevent_wrapper static_wrapper = *( (struct sctk_aio_sigevent_wrapper *) pwrapper );
	free( pwrapper );
	
	(static_wrapper.func)(static_wrapper.sv);
	
	return NULL;
}


int sctk_aio_sigevent( struct sigevent * sig )
{

	if( !sig )
	{
		return 0;
	}
	
	switch( sig->sigev_notify )
	{
		case SIGEV_NONE:
			return 0;
		break;
		
		case SIGEV_THREAD:
			/* A clean implementation should create a thread here 
			 * but this would not be so efficient so we just call
			 * the target function if no attr was set */
			 
			 if( !sig->sigev_notify_function )
			 {
				errno = EINVAL;
				return 1;
			 }
			 
			 if( sig->sigev_notify_attributes )
			 {
				pthread_attr_t * attr = (pthread_attr_t *)sig->sigev_notify_attributes;
				pthread_t th;
				struct sctk_aio_sigevent_wrapper * wrapper = malloc( sizeof( struct sctk_aio_sigevent_wrapper ) );
				
				if( !wrapper )
				{
					perror("malloc");
					return 1;
				}
				
				wrapper->sv = sig->sigev_value;
				wrapper->func = sig->sigev_notify_function;
				
				if( pthread_create( &th, attr, sctk_aio_sigevent_detachfunction, (void *)wrapper ) < 0 )
				{
					free( wrapper );
					perror("pthread_create");
					return 1;
				}
			 }
			 else
			 {
				/* Just call */
				(sig->sigev_notify_function)(sig->sigev_value);
			 }
		break;
		
		case SIGEV_SIGNAL:
			/* Signal the process with the value in siginfo */
			if( sigqueue( getpid(), sig->sigev_signo, sig->sigev_value ) < 0 )
			{
				perror("sigqueue");
				return 1;
			}
		break;
		
	}
	
	return 0;
}


int sctk_aio_Process( struct aiocb * cb )
{
	
	if( !cb )
	{
		errno = EINVAL;
		return 1;
	}
	
	int do_append = 0;
	
	int fcntlret = fcntl( cb->aio_fildes, F_GETFD );
	
	if( fcntlret < 0 )
	{
		perror("fcntl");
		sctk_aio_set_error( cb, errno );
		return 1;
	}
	
	do_append = fcntlret & O_APPEND;
	
	if( do_append )
	{
		/* Here we set the offset in the aiocb
		 * structure accordingly to the append policy */
		 
		 /* Get current offset */
		 long long int current_off = lseek( cb->aio_fildes, 0, SEEK_CUR);
		 
		if( current_off < 0 )
		{
			perror("lseek");
			sctk_aio_set_error( cb, errno );
			return 1;
		}
		
		/* Register it for a p(read|write) operation */
		cb->aio_offset = current_off;
		
		/* Move the head */
		lseek( cb->aio_nbytes, 0, SEEK_CUR);
	}

	int ret = 1;
	volatile int * popcode = &cb->aio_lio_opcode;
	
	switch( *popcode )
	{
		case LIO_READ:
			ret = sctk_aio_Read( cb );
		break;
		case LIO_WRITE:
			ret = sctk_aio_Write( cb );
		break;
		case O_SYNC:
			if( fsync( cb->aio_fildes ) < 0 )
			{
				perror("fsync");
				sctk_aio_set_error( cb, errno );
				return 1;
			}
		break;
#ifndef MPC_AIO_SYNC_IS_DUPLICATED
		case O_DSYNC:
			if( fdatasync( cb->aio_fildes ) < 0 )
			{
				perror("fdatasync");
				sctk_aio_set_error( cb, errno );
				return 1;
			}
		break;
#endif
		case LIO_NOP:
			/* Nothing to do */
			ret = 0;
		break;
		case ECANCELED:
			/* Here we were canceled */
			sctk_aio_set_error( cb, ECANCELED );
		break;
	}
	
	if( ret == 0 )
	{
		/* No error set complete */
		sctk_aio_set_error( cb, 0 );
		/* Signal the end if needed */
		sctk_aio_sigevent( &cb->aio_sigevent );
	}
	
	return ret;
}


/************************************************************************/
/* AIO Threads                                                          */
/************************************************************************/

struct sctk_aio_threads
{
	pthread_t aio_threads[ MAX_SCTK_AIO_THREADS ];
	
	/* Condition variable for processing */
	pthread_cond_t cond;
	pthread_mutex_t condlock;
	
	/* Condition variable for suspend */
	pthread_cond_t scond;
	pthread_mutex_t scondlock;
	
	/* Joblist */
	struct sctk_aio_joblist jobs;
};

static inline int sctk_aio_numthreads()
{
	return 4;
}

static volatile int sctk_aio_threads_initialized = 0;
pthread_mutex_t sctk_aio_threads_initialized_lock = PTHREAD_MUTEX_INITIALIZER;



static struct sctk_aio_threads sctk_aio_threads;
static volatile int sctk_aio_thread_stop_condition = 1;

void * aio_thread_main_loop( __UNUSED__ void * arg )
{
	int trial_counter = 0;

	while( 1 )
	{
		if( sctk_aio_thread_stop_condition == 1 )
			break;
		
		struct aiocb * job = sctk_aio_joblist_pop( &sctk_aio_threads.jobs );
		
		if( !job )
		{
			trial_counter++;
			
			/* Try to take other jobs before blocking */
			if( 42 <= trial_counter )
			{
				/* Enter a condwait */
				pthread_mutex_lock( &sctk_aio_threads.condlock );
				pthread_cond_wait( &sctk_aio_threads.cond, &sctk_aio_threads.condlock );
				pthread_mutex_unlock( &sctk_aio_threads.condlock );
			}
		}
		else
		{
			trial_counter = 0;
			sctk_aio_Process( job );
			
			/* Signal the suspend condition as we completed a job */
			pthread_cond_signal(  &sctk_aio_threads.scond );
		}		
	}
	
	return NULL;
}



int sctk_aio_threads_init()
{
	pthread_mutex_lock( &sctk_aio_threads_initialized_lock );
	
	if( sctk_aio_threads_initialized )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 0;
	}
	
	if( pthread_cond_init( &sctk_aio_threads.cond, NULL ) < 0 )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	} 
	
	if( pthread_mutex_init( &sctk_aio_threads.condlock, NULL ) < 0 )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	} 
	
	
	if( pthread_cond_init( &sctk_aio_threads.scond, NULL ) < 0 )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	} 
	
	if( pthread_mutex_init( &sctk_aio_threads.scondlock, NULL ) < 0 )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	} 
	
	/* Flag threads as running */
	sctk_aio_thread_stop_condition = 0;
	
	/* Initialize the joblist */
	sctk_aio_joblist_init( &sctk_aio_threads.jobs );
	
	if(  MAX_SCTK_AIO_THREADS < sctk_aio_numthreads() )
	{
		printf("Error overflow in AIO thread array\n");
		abort();
	}
	
	/* Launch threads */
	int i;
	for( i = 0 ; i <  sctk_aio_numthreads() ; i++ )
	{
		if( pthread_create( &sctk_aio_threads.aio_threads[i], NULL, aio_thread_main_loop, NULL ) )
		{
			pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
			return 1;
		}
	}
	
	sctk_aio_threads_initialized = 1;
	pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
	return 0;
}

int sctk_aio_threads_release()
{
	pthread_mutex_lock( &sctk_aio_threads_initialized_lock );
	
	if( !sctk_aio_threads_initialized )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 0;
	}
	
	if( pthread_cond_destroy( &sctk_aio_threads.cond ) < 0 )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	} 
	
	if( pthread_mutex_destroy( &sctk_aio_threads.condlock ) < 0 )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	} 
	
	/* Flag threads as running */
	sctk_aio_thread_stop_condition = 1;

	/* Unblock waiting threads */
	pthread_cond_broadcast( &sctk_aio_threads.cond );

	/* Launch threads */
	int i;
	for( i = 0 ; i <  sctk_aio_numthreads() ; i++ )
	{
		if( pthread_join( sctk_aio_threads.aio_threads[i], NULL ) )
		{
			pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
			return 1;
		}
	}
	
	/* Make sure joblist is empty */
	struct aiocb * ret;
	
	while( (ret = sctk_aio_joblist_pop( &sctk_aio_threads.jobs ) ) )
	{
		sctk_aio_Process( ret );
	}
		
	/* Initialize the joblist */
	if( sctk_aio_joblist_release( &sctk_aio_threads.jobs ) )
	{
		pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
		return 1;
	}

	sctk_aio_threads_initialized = 0;
	pthread_mutex_unlock( &sctk_aio_threads_initialized_lock );
	
	return 0;
}

/************************************************************************/
/* AIO Interface                                                        */
/************************************************************************/

int sctk_aio_check_request( struct aiocb * cb )
{
	if( !cb )
	{
		errno = EINVAL;
		return 1;
	}
	
	if( fcntl( cb->aio_fildes, F_GETFD ) < 0 )
	{
		errno = EBADF;
		return 1;
	}
	
	/* No need for a buff when syncing */
	if( (cb->aio_lio_opcode == LIO_READ ) || (cb->aio_lio_opcode == LIO_WRITE ) )
	{
		if( cb->aio_buf == NULL )
		{
			errno = EINVAL;
			return 1;
		}
	}
	
	return 0;
}

int sctk_aio_submit( struct aiocb * cb, int type )
{
	/* Set job type */
	cb->aio_lio_opcode = type;
	
	if( sctk_aio_check_request( cb ) )
	{
		return -1;
	}
	
	/* Initialize if needed */
	if( !sctk_aio_threads_initialized )
	{
		if( sctk_aio_threads_init() )
		{
			return -1;
		}
	}
	
	/* Tell aio_error that we started working */
	sctk_aio_set_error( cb, EINPROGRESS );
	
	if( sctk_aio_joblist_push( &sctk_aio_threads.jobs, cb ) )
	{
		sctk_aio_set_error( cb, ENOMEM );
		return -1;
	}
	
	/* Launch a thread */
	if( pthread_cond_signal( &sctk_aio_threads.cond ) )
		return -1;
	
	return 0;
}

int sctk_aio_read( struct aiocb * cb )
{
	return sctk_aio_submit( cb, LIO_READ );
}

int sctk_aio_write( struct aiocb * cb )
{
	return sctk_aio_submit( cb, LIO_WRITE );
}

int sctk_aio_fsync( int op, struct aiocb * cb )
{
	if( ( op != O_SYNC ) && ( op != O_DSYNC ) )
	{
		errno = EINVAL;
		return -1;
	}

	return sctk_aio_submit( cb, op );
}

int sctk_aio_error( struct aiocb * cb )
{
	int ret = -1;
	sctk_aio_get_error( cb, &ret );
	return ret;
}

ssize_t sctk_aio_return( struct aiocb * cb )
{
	int ret = -1;
	sctk_aio_get_error( cb, &ret );
	
	if( ret == 0 )
	{
		/* Complete, return status */
		if( (cb->aio_lio_opcode == LIO_READ ) || (cb->aio_lio_opcode == LIO_WRITE ) )
		{
			return cb->aio_nbytes;
		}
		else
		{
			return 0;
		}
	}
	
	if( ret == ECANCELED )
		return -1;
	
	/* Otherwise return the error which was set */
	
	return ret;
}

int sctk_aio_cancel( int fd, struct aiocb * cb )
{
	if( !sctk_aio_threads_initialized )
	{
		errno = EINVAL;
		return -1;
	}
	
	if( cb )
	{
		int status;
		sctk_aio_get_error( cb, &status );
		
		/* Already DONE */
		if( status == 0 )
		{
			return AIO_ALLDONE;
		}
		
		cb->aio_lio_opcode = ECANCELED;
		
		return AIO_CANCELED;
	}
	else
	{
		return sctk_aio_joblist_cancel_fd( &sctk_aio_threads.jobs, fd );
	}
}

int sctk_aio_suspend( const struct aiocb * const aiocb_list[], int nitems, const struct timespec * timeout )
{
	int ret = 0;

	/* Just in case (not to deadlock)
	 * as there could be a RC between the job counter
	 * and the moment we enter the condition wait */
	
	struct timespec loc_timeout;
	loc_timeout.tv_sec = 0;
	loc_timeout.tv_nsec = 10000000; /* 10 ms spinning */
	
	if( !timeout )
		timeout = &loc_timeout;

	while( 1 )
	{
		int i;
		int status;
		
		/* Check if is one of the request of interest has ended */
		for( i = 0 ; i  < nitems; i++ )
		{
			sctk_aio_get_error( (struct aiocb *)&aiocb_list[i], &status );
			
			if( status == 0 )
			{
				return 0;
			}
		}

		pthread_mutex_lock( &sctk_aio_threads.scondlock );
		ret = pthread_cond_timedwait( &sctk_aio_threads.scond, &sctk_aio_threads.scondlock, timeout );
		pthread_mutex_unlock( &sctk_aio_threads.scondlock );
		
		if( (ret == ETIMEDOUT) && (timeout != &loc_timeout) )
			return EAGAIN;
		
		/* Note that here we do not handle signals (no EINTR error) */
	}
	
	if( ret == 0 )
		return 0;
	
	return -1;
}

struct sctk_aio_lio_listio_ctx
{
	struct sigevent * sevp;
	struct sigevent * individual_sevp;

	volatile int counter;
	volatile int request_count;
	pthread_mutex_t lock;
};

void lio_listio_ctx_notify( union sigval pctx )
{
	struct sctk_aio_lio_listio_ctx * ctx = (struct sctk_aio_lio_listio_ctx *) pctx.sival_ptr;
	
	int do_free = 0;
	
	pthread_mutex_lock( &ctx->lock );
	
	ctx->counter++;

	if( ctx->counter == ctx->request_count )
	{
		/* I'm the last so I free */
		do_free = 1;
	}
		
	pthread_mutex_unlock( &ctx->lock );

	if( do_free )
	{
		/* First the last one does the notification as we overrided
		 * it to get here, we first do this before signaling the 
		 * total processing end */
		
		/* Notify Individual sigevent from each aiocb */
		int i;
		for( i = 0 ; i < ctx->request_count ; i++ )
			sctk_aio_sigevent( &ctx->individual_sevp[i] );
		
		free( ctx->individual_sevp );
		
		/* Notify END (listio sigevent) */
		sctk_aio_sigevent( ctx->sevp );
		
		pthread_mutex_destroy( &ctx->lock );
		
		free( ctx );
	}
}



int sctk_aio_lio_listio( int mode , struct aiocb * const aiocb_list[], int nitems, struct sigevent *sevp )
{
	/* Check mode */
	if( ( mode != LIO_WAIT ) && (mode != LIO_NOWAIT ) )
	{
		errno = EINVAL;
		return -1;
	} 
	
	/* Particular case */
	if( nitems == 0 )
	{
		if( sctk_aio_sigevent( sevp ) )
			return -1;
		
		return 0;
	}
	
	/* Allocate the termination handle */
	struct sctk_aio_lio_listio_ctx * ctx = malloc( sizeof( struct sctk_aio_lio_listio_ctx ) );
	
	if( !ctx )
	{
		perror("malloc");
		return -1;
	}
	
	ctx->sevp = sevp;
	
	if( pthread_mutex_init( &ctx->lock, NULL ) )
	{
		perror("pthread_mutex_init");
		return -1;
	}
	
	ctx->counter = 0;
	ctx->request_count = nitems;
	
	/* Prepare a structure for individual notifications */
	ctx->individual_sevp = malloc( nitems * sizeof( struct sigevent ));
	
	if( !ctx->individual_sevp )
	{
		perror("malloc");
		return -1;
	}
	
	int i;
	for( i = 0 ; i < nitems; i++ )
	{
		if( aiocb_list[i] )
		{
			ctx->individual_sevp[i] = aiocb_list[i]->aio_sigevent;
		}
		else
		{
			memset( &ctx->individual_sevp[i], 0, sizeof( struct sigevent ) );
			ctx->individual_sevp[i].sigev_notify = SIGEV_NONE;
		}
	}
	

	/* Override requests signaling with the termination handler */
	struct sigevent request_sevp_override;
	memset( &request_sevp_override, 0, sizeof( struct sigevent ) );
	
	request_sevp_override.sigev_notify = SIGEV_THREAD;
	request_sevp_override.sigev_notify_function = lio_listio_ctx_notify;
	request_sevp_override.sigev_value.sival_ptr = ctx;
	
	for( i = 0 ; i < nitems; i++ )
	{
		/* Some can be NULL */
		if( !aiocb_list[i] )
			continue;
		
		/* Override */
		aiocb_list[i]->aio_sigevent = request_sevp_override;
		
		/* Submit */
		if( sctk_aio_submit( aiocb_list[i], aiocb_list[i]->aio_lio_opcode ) )
		{
			/* Failed to queue this OP */
			errno = EIO;
			return -1;
		}
	}

	/* Launch all threads */
	pthread_cond_broadcast( &sctk_aio_threads.cond );


	if( mode == LIO_WAIT )
	{
		while( 1 )
		{
			/* Just in case (not to deadlock) */
			struct timespec timeout;
			timeout.tv_sec = 1;
			timeout.tv_nsec = 0;
			
			/* Here we use this not to spin */
			sctk_aio_suspend( (const struct aiocb * const*)aiocb_list, nitems, &timeout );
			
			int done = 0;
			
			/* Check for completion */
			for( i = 0 ; i < nitems ; i++ )
			{
				/* Some can be NULL */
				if( !aiocb_list[i] )
				{
					done++;
					continue;
				}
					
				int status;
				
				sctk_aio_get_error( aiocb_list[i], &status );

				if( status != EINPROGRESS )
				{
					done++;
				}
				
			}
			
			if( done == nitems )
			{
				return 0;
			}
		}
	}

	return 0;
}
