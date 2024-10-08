/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *										University Research and Technology
 *										Corporation.	All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All
 *										rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */
#ifndef __NBC_H__
#define __NBC_H__

/* the debug level */
//#define NBC_DLEVEL 11

#define HAVE_PROGRESS_THREAD    1

/********************* end of LibNBC tuning parameters ************************/

/* correct fortran bindings */

#include <mpc_mpi.h>
#include <mpc_thread.h>
#include <datatype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>


#define USE_MPI    /* set by the buildsystem! */

#ifdef __cplusplus
extern "C" {
#endif

/* Function return codes	*/
#define NBC_OK                        MPI_SUCCESS /* everything went fine */
#define NBC_SUCCESS                   MPI_SUCCESS /* everything went fine (MPI compliant :) */
#define NBC_OOR                       1           /* out of resources */
#define NBC_BAD_SCHED                 2           /* bad schedule */
#define NBC_CONTINUE                  3           /* progress not done */
#define NBC_DATATYPE_NOT_SUPPORTED    4           /* datatype not supported or not valid */
#define NBC_OP_NOT_SUPPORTED          5           /* operation not supported or not valid */
#define NBC_NOT_IMPLEMENTED           6
#define NBC_INVALID_PARAM             7           /* invalid parameters */
#define NBC_INVALID_TOPOLOGY_COMM     8           /* invalid topology attached to communicator */

// safeguard for non-MPI-2.2 compliant MPIs
#ifndef MPI_UNWEIGHTED
#define MPI_UNWEIGHTED    0
#endif

/* number of implemented collective functions */
#define NBC_NUM_COLL    19


/* a schedule is basically a pointer to some memory location where the
 * schedule array resides */
typedef void *NBC_Schedule;

/* used to hang off a communicator */
typedef struct
{
	MPI_Comm mycomm; /* save the shadow communicator here */
	int      tag;
} NBC_Comminfo;

/* thread specific data */
typedef struct
{
	MPI_Comm               comm;
	MPI_Comm               mycomm;
	long                   row_offset;
	int                    tag;
	volatile int           req_count;
	mpc_thread_mutex_t     lock;
	mpc_thread_sem_t       semid;
	volatile int           actual_req_count;
	MPI_Request *          req_array;
	int *                  req_count_persistent; /* number of requests for each round
	                                              *      used by persistent */
	volatile int           array_offset;         /* current offset in req_array for progress
	                                              * used by persistent */
	int                    num_rounds;           /* numero of round for progress used by persistent */
	int                    is_persistent;        /* to differentiate nbc from persistent using nbc */
	NBC_Comminfo *         comminfo;
	volatile NBC_Schedule *schedule;
	void *                 tmpbuf; /* temporary buffer e.g. used for Reduce */

/* TODO: we should make a handle pointer to a state later (that the user
 * can move request handles) */
	void *                 tmpbuf_gather; /* temporary buffer e.g. used for Gather */
} NBC_Handle;
typedef NBC_Handle NBC_Request;


/*******************************************************
****** external NBC functions are defined here *******
*******************************************************/


/* external function prototypes */
extern int NBC_Wait(NBC_Handle *handle, MPI_Status *status);
int NBC_Test(NBC_Handle *handle, int *flag, MPI_Status *status);

/* TODO: some hacks */



/* all MPI functions used in LibNBC have to be defined here */


/* if we use MPI-1, MPI_IN_PLACE is not defined :-( */
#ifndef MPI_IN_PLACE
#define MPI_IN_PLACE    (void *)1
#endif

/* restore inline behavior for non-gcc compilers */
#ifndef __GNUC__
#define inline    inline
#endif

/* log(2) */
#define LOG2                   0.69314718055994530941

/* all collectives */
#define NBC_ALLGATHER          0
#define NBC_ALLGATHERV         1
#define NBC_ALLREDUCE          2
#define NBC_ALLTOALL           3
#define NBC_ALLTOALLV          4
#define NBC_ALLTOALLW          5
#define NBC_BARRIER            6
#define NBC_BCAST              7
#define NBC_EXSCAN             8
#define NBC_GATHER             9
#define NBC_GATHERV            10
#define NBC_REDUCE             11
#define NBC_REDUCESCAT         12
#define NBC_REDUCESCATBLOCK    13
#define NBC_SCAN               14
#define NBC_SCATTER            15
#define NBC_SCATTERV           16
#define NBC_CART_SHIFT_XCHG    17
#define NBC_NEIGHBOR_XCHG      18
/* set the number of collectives in nbc.h !!!! */

/* several typedefs for NBC */

/* the function type enum */
typedef enum
{
	SEND,
	RECV,
	OP,
	COPY,
	MOVE,
	UNPACK
} NBC_Fn_type;

/* the send argument struct */
typedef struct
{
	void *       buf;
	void *       buf1;
	void *       buf2;
	void *       buf3;
	MPI_Datatype datatype;
	void *       src;
	MPI_Datatype srctype;
	void *       tgt;
	MPI_Datatype tgttype;
	void *       inbuf;
	void *       outbuf;
	MPI_Comm     comm;
	MPI_Op       op;
	int          count;
	int          dest;
	int          source;
	int          srccount;
	int          tgtcount;
	char         tmpbuf;
	char         tmpbuf1;
	char         tmpbuf2;
	char         tmpbuf3;
	char         tmpsrc;
	char         tmptgt;
	char         tmpinbuf;
	char         tmpoutbuf;
} NBC_Args;

/* internal function prototypes */
int NBC_Start(NBC_Handle *handle, NBC_Schedule *schedule);
int NBC_Init_handle(NBC_Handle *handle, MPI_Comm comm, int tag);

/* some macros */

/* a schedule has the following format:
 * [schedule] ::= [size][round-schedule][delimiter][round-schedule][delimiter]...[end]
 * [size] ::= size of the schedule (int)
 * [round-schedule] ::= [num][type][type-args][type][type-args]...
 * [num] ::= number of elements in round (int)
 * [type] ::= function type (NBC_Fn_type)
 * [type-args] ::= type specific arguments (NBC_Args, NBC_Args or, NBC_Args)
 * [delimiter] ::= 1 (char) - indicates that a round follows
 * [end] ::= 0 (char) - indicates that this is the last round
 */

/* NBC_GET_ROUND_SIZE returns the size in bytes of a round of a NBC_Schedule
 * schedule. A round has the format:
 * [num]{[type][type-args]}
 * e.g. [(int)2][(NBC_Fn_type)SEND][(NBC_Args)SEND-ARGS][(NBC_Fn_type)RECV][(NBC_Args)RECV-ARGS] */
#define NBC_GET_ROUND_SIZE(schedule, size)                                                  \
	{                                                                                   \
		int *numptr;                                                                \
		numptr = (int *)schedule;                                                   \
		size   = *numptr * (sizeof(NBC_Fn_type) + sizeof(NBC_Args) ) + sizeof(int); \
	}

/* returns the size of a schedule in bytes */
#define NBC_GET_SIZE(schedule, size)     \
	{                                \
		size = *(int *)schedule; \
	}

/* NBC_PRINT_ROUND prints a round in a schedule. A round has the format:
 * [num]{[op][op-data]} types: [int]{[enum][op-type]}
 * e.g. [2][SEND][SEND-ARGS][RECV][RECV-ARGS] */
#define NBC_PRINT_ROUND(schedule)                                                                                                                                                                                                                                                                          \
	{                                                                                                                                                                                                                                                                                                  \
		int          myrank, *numptr;                                                                                                                                                                                                                                                              \
		NBC_Fn_type *typeptr;                                                                                                                                                                                                                                                                      \
		NBC_Args *   sendargs;                                                                                                                                                                                                                                                                     \
		NBC_Args *   recvargs;                                                                                                                                                                                                                                                                     \
		NBC_Args *   opargs;                                                                                                                                                                                                                                                                       \
		NBC_Args *   copyargs;                                                                                                                                                                                                                                                                     \
		NBC_Args *   unpackargs;                                                                                                                                                                                                                                                                   \
		int          i;                                                                                                                                                                                                                                                                            \
                                                                                                                                                                                                                                                                                                           \
		numptr = (int *)schedule;                                                                                                                                                                                                                                                                  \
		MPI_Comm_rank(MPI_COMM_WORLD, &myrank);                                                                                                                                                                                                                                                    \
		printf("has %i actions: \n", *numptr);                                                                                                                                                                                                                                                     \
		/* end is increased by sizeof(int) bytes to point to type */                                                                                                                                                                                                                               \
		typeptr = (NBC_Fn_type *)( (int *)(schedule) + 1);                                                                                                                                                                                                                                         \
		for(i = 0; i < *numptr; i++){                                                                                                                                                                                                                                                              \
			/* go sizeof op-data forward */                                                                                                                                                                                                                                                    \
			switch(*typeptr){                                                                                                                                                                                                                                                                  \
				case SEND:                                                                                                                                                                                                                                                                 \
					printf("[%i]	SEND (offset %li) ", myrank, (long)typeptr - (long)schedule);                                                                                                                                                                                         \
					sendargs = (NBC_Args *)(typeptr + 1);                                                                                                                                                                                                                              \
					printf("*buf: %lu, count: %i, type: %lu, dest: %i)\n", (unsigned long)sendargs->buf, sendargs->count, (unsigned long)sendargs->datatype, sendargs->dest);                                                                                                          \
					typeptr = (NBC_Fn_type *)( (NBC_Args *)typeptr + 1);                                                                                                                                                                                                               \
					break;                                                                                                                                                                                                                                                             \
				case RECV:                                                                                                                                                                                                                                                                 \
					printf("[%i]	RECV (offset %li) ", myrank, (long)typeptr - (long)schedule);                                                                                                                                                                                         \
					recvargs = (NBC_Args *)(typeptr + 1);                                                                                                                                                                                                                              \
					printf("*buf: %lu, count: %i, type: %lu, source: %i)\n", (unsigned long)recvargs->buf, recvargs->count, (unsigned long)recvargs->datatype, recvargs->source);                                                                                                      \
					typeptr = (NBC_Fn_type *)( (NBC_Args *)typeptr + 1);                                                                                                                                                                                                               \
					break;                                                                                                                                                                                                                                                             \
				case OP:                                                                                                                                                                                                                                                                   \
					printf("[%i]	OP	 (offset %li) ", myrank, (long)typeptr - (long)schedule);                                                                                                                                                                                          \
					opargs = (NBC_Args *)(typeptr + 1);                                                                                                                                                                                                                                \
					printf("*buf1: %lu, buf2: %lu, count: %i, type: %lu)\n", (unsigned long)opargs->buf1, (unsigned long)opargs->buf2, opargs->count, (unsigned long)opargs->datatype);                                                                                                \
					typeptr = (NBC_Fn_type *)( (NBC_Args *)typeptr + 1);                                                                                                                                                                                                               \
					break;                                                                                                                                                                                                                                                             \
				case COPY:                                                                                                                                                                                                                                                                 \
					printf("[%i]	COPY	 (offset %li) ", myrank, (long)typeptr - (long)schedule);                                                                                                                                                                                        \
					copyargs = (NBC_Args *)(typeptr + 1);                                                                                                                                                                                                                              \
					printf("*src: %lu, srccount: %i, srctype: %lu, *tgt: %lu, tgtcount: %i, tgttype: %lu)\n", (unsigned long)copyargs->src, copyargs->srccount, (unsigned long)copyargs->srctype, (unsigned long)copyargs->tgt, copyargs->tgtcount, (unsigned long)copyargs->tgttype); \
					typeptr = (NBC_Fn_type *)( (NBC_Args *)typeptr + 1);                                                                                                                                                                                                               \
					break;                                                                                                                                                                                                                                                             \
				case UNPACK:                                                                                                                                                                                                                                                               \
					printf("[%i]	UNPACK	 (offset %li) ", myrank, (long)typeptr - (long)schedule);                                                                                                                                                                                      \
					unpackargs = (NBC_Args *)(typeptr + 1);                                                                                                                                                                                                                            \
					printf("*src: %lu, srccount: %i, srctype: %lu, *tgt: %lu\n", (unsigned long)unpackargs->inbuf, unpackargs->count, (unsigned long)unpackargs->datatype, (unsigned long)unpackargs->outbuf);                                                                         \
					typeptr = (NBC_Fn_type *)( (NBC_Args *)typeptr + 1);                                                                                                                                                                                                               \
					break;                                                                                                                                                                                                                                                             \
				default:                                                                                                                                                                                                                                                                   \
					printf("[%i] NBC_PRINT_ROUND: bad type %li at offset %li\n", myrank, (long)*typeptr, (long)typeptr - (long)schedule);                                                                                                                                              \
					return NBC_BAD_SCHED;                                                                                                                                                                                                                                              \
			}                                                                                                                                                                                                                                                                                  \
			/* increase ptr by size of fn_type enum */                                                                                                                                                                                                                                         \
			typeptr = (NBC_Fn_type *)( (NBC_Fn_type *)typeptr + 1);                                                                                                                                                                                                                            \
		}                                                                                                                                                                                                                                                                                          \
		printf("\n");                                                                                                                                                                                                                                                                              \
	}

#define NBC_CHECK_NULL(ptr)          \
	{                            \
		if(ptr == NULL){     \
			MPC_CRASH(); \
		}                    \
	}


/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */
static inline void NBC_DEBUG(__UNUSED__ int level, __UNUSED__ const char *fmt, ...)
{
#if NBC_DLEVEL > 0
	va_list ap;
	int     rank;

	if(NBC_DLEVEL >= level)
	{
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		printf("[LibNBC - %i] ", rank);
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
		fflush(stdout);
		fflush(stderr);
	}
#endif
}

/* returns true (1) or false (0) if type is intrinsic or not */
static inline int NBC_Type_intrinsic(MPI_Datatype type)
{
	if( (type == MPI_INT) ||
	    (type == MPI_LONG) ||
	    (type == MPI_SHORT) ||
	    (type == MPI_UNSIGNED) ||
	    (type == MPI_UNSIGNED_SHORT) ||
	    (type == MPI_UNSIGNED_LONG) ||
	    (type == MPI_FLOAT) ||
	    (type == MPI_DOUBLE) ||
	    (type == MPI_LONG_DOUBLE) ||
	    (type == MPI_BYTE) ||
	    (type == MPI_FLOAT_INT) ||
	    (type == MPI_DOUBLE_INT) ||
	    (type == MPI_LONG_INT) ||
	    (type == MPI_2INT) ||
	    (type == MPI_SHORT_INT) ||
	    (type == MPI_LONG_DOUBLE_INT) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* let's give a try to inline functions */
static inline int NBC_Copy(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm)
{
	int      size, pos, res;
	MPI_Aint ext;
	void *   packbuf;

	if( (srctype == tgttype) && _mpc_dt_is_contig_mem(srctype) )
	{
		/* if we have the same types and they are contiguous (intrinsic
		 * types are contiguous), we can just use a single memcpy */
		res = MPI_Type_extent(srctype, &ext);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Type_extent() (%i)\n", res); return res;
		}
		memcpy(tgt, src, srccount * ext);
	}
	else
	{
		/* we have to pack and unpack */
		res = MPI_Pack_size(srccount, srctype, comm, &size);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Pack_size() (%i)\n", res); return res;
		}
		packbuf = malloc(size);
		if(NULL == packbuf)
		{
			printf("Error in malloc()\n"); return res;
		}
		pos = 0;
		res = MPI_Pack(src, srccount, srctype, packbuf, size, &pos, comm);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Pack() (%i)\n", res); return res;
		}
		pos = 0;
		res = MPI_Unpack(packbuf, size, &pos, tgt, tgtcount, tgttype, comm);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Unpack() (%i)\n", res); return res;
		}
		free(packbuf);
	}

	return NBC_OK;
}

/* let's give a try to inline functions */
static inline int NBC_Move(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm)
{
	int      size, pos, res;
	MPI_Aint ext;
	void *   packbuf;

	if( (srctype == tgttype) && NBC_Type_intrinsic(srctype) )
	{
		/* if we have the same types and they are contiguous (intrinsic
		 * types are contiguous), we can just use a single memcpy */
		res = MPI_Type_extent(srctype, &ext);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Type_extent() (%i)\n", res); return res;
		}
		memmove(tgt, src, srccount * ext);
	}
	else
	{
		/* we have to pack and unpack */
		res = MPI_Pack_size(srccount, srctype, comm, &size);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Pack_size() (%i)\n", res); return res;
		}
		packbuf = malloc(size);
		if(NULL == packbuf)
		{
			printf("Error in malloc()\n"); return res;
		}
		pos = 0;
		res = MPI_Pack(src, srccount, srctype, packbuf, size, &pos, comm);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Pack() (%i)\n", res); return res;
		}
		pos = 0;
		res = MPI_Unpack(packbuf, size, &pos, tgt, tgtcount, tgttype, comm);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Unpack() (%i)\n", res); return res;
		}
		free(packbuf);
	}

	return NBC_OK;
}

static inline int NBC_Unpack(void *src, int srccount, MPI_Datatype srctype, void *tgt, MPI_Comm comm)
{
	int      size, pos, res;
	MPI_Aint ext;

	if(NBC_Type_intrinsic(srctype) )
	{
		/* if we have the same types and they are contiguous (intrinsic
		 * types are contiguous), we can just use a single memcpy */
		res = MPI_Type_extent(srctype, &ext);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Type_extent() (%i)\n", res); return res;
		}
		memcpy(tgt, src, srccount * ext);
	}
	else
	{
		/* we have to unpack */
		res = MPI_Pack_size(srccount, srctype, comm, &size);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Pack_size() (%i)\n", res); return res;
		}
		pos = 0;
		res = MPI_Unpack(src, size, &pos, tgt, srccount, srctype, comm);
		if(MPI_SUCCESS != res)
		{
			printf("MPI Error in MPI_Unpack() (%i)\n", res); return res;
		}
	}

	return NBC_OK;
}
/* NOLINTEND(clang-diagnostic-unused-function) */

#ifdef __cplusplus
}
#endif

#endif
