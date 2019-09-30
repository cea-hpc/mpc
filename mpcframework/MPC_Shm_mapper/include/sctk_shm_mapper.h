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
/* #   - VALAT Sébastien sebastien.valat@cea.fr                           # */
/* #   - CAPRA Antoine capra@paratools.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_SHM_MAPPER_H
#define SCTK_SHM_MAPPER_H

/************************** HEADERS ************************/
#include "mpc_common_asm.h"
#include "sctk_bool.h"
#include <stdio.h>
#include <stdlib.h>
//#define bool unsigned char
//#define true 1
//#define false 0

#ifdef __cplusplus
extern "C"
{
#endif

/************************** MACROS *************************/
#ifndef SCTK_STATIC
	#define SCTK_STATIC
	#define SCTK_PUBLIC
#endif //SCTK_STATIC

#define SCTK_SHM_MAPPER_PAGE_SIZE 4096lu

/**************************  TYPES  ************************/
/**
 * Define a handler to broadcast the name of the shm file to all participant (except the master).
 * It didn't require to wait after the send as master will sync after this.
 * @param filename Define the name of the file used to find the SHM segment.
 * @param option Optional parameter to transmit to handler implementation.
**/
typedef bool (*sctk_shm_mapper_send_handler)(const char *filename, void *option,
                                             void *option1);
/**
 * Define a handler to receive the name of shm file on all participant (except the master).
 * Must wait unit receive.
 * @param option Optional parameter to transmit to handler implementation.
 * @return Return a copy of the filename, the caller must use free on it when finish.
**/
typedef char *(*sctk_shm_mapper_recv_handler)(void *option, void *option1);
/**
 * Function used to generate the SHM filename. Their is one by default, but can be changed
 * depending on the situation.
 * @param option Optional parameter to transmit to handler implementation.
**/
typedef char *(*sctk_shm_mapper_filename_handler)(void *option, void *option1);
typedef size_t sctk_size_t;
typedef size_t sctk_addr_t;

/************************** STRUCT *************************/
/**
 * Permit to get a memorey representation of the /proc/[ALL_PIDS]/maps by
 *merging all tables of participants.
 * It will contain the free segments, not mapped one, so can directely search a
 *good candidate after filling it.
**/
struct sctk_shm_mapper_segment_s {
  /** Starting address of the free segment. **/
  sctk_addr_t start;
  /** End address of the free segment. **/
  sctk_addr_t end;
  /** Next free segment if not NULL. **/
  struct sctk_shm_mapper_segment_s *next;
};
typedef struct sctk_shm_mapper_segment_s sctk_shm_mapper_segment_t;

/************************** STRUCT *************************/
/**
 * Structure to store the handler to exchange the name of SHM file between all
 *participant.
 * It provide tha send method used by master and receive one used by slaves.
 * By this way we can support init on top of various communication system (MPI,
 *PMI,...)
**/
struct sctk_alloc_mapper_handler_s
{
	/** The handler used by master to send the shm filename. **/
	sctk_shm_mapper_send_handler send_handler;
	/** The handler used by slaves to receive the shm filename. **/
	sctk_shm_mapper_recv_handler recv_handler;
	/** Handler to generate the shm filename. **/
	sctk_shm_mapper_filename_handler gen_filename;
	/** An opaque option to transmit the handlers when calling them. **/
	void * option;
        /** An opaque option to transmit the handlers when calling them. **/
        void *option1;
};
typedef struct sctk_alloc_mapper_handler_s sctk_alloc_mapper_handler_t;

/*************************** ENUM **************************/
/**
 * Role of the current caller.
**/
enum sctk_shm_mapper_role_e
{
	/** The master will create the shm file and choose the address if needed. **/
	SCTK_SHM_MAPPER_ROLE_MASTER,
	/** The slaves wait order from master, try to map the files and wait final mapping address from it. **/
	SCTK_SHM_MAPPER_ROLE_SLAVE
};
typedef enum sctk_shm_mapper_role_e sctk_shm_mapper_role_t;

/*************************** ENUM **************************/
/**
 * This is for easier debugging to mark the different steps in communication.
**/
enum sctk_shm_mapper_steps_e
{
	/** Master create the shm file and map it. **/
	SCTK_SHM_MAPPER_STEP_INIT,
	/** Master send the filename to slaves. **/
	SCTK_SHM_MAPPER_SYNC_FILENAME,
	/** Slaves has mapped the file and return anwser to master. **/
	SCTK_SHM_MAPPER_STEP_FIRST_SLAVE_MAPPING,
	/** Master as taken decision for final address and wait anwser of slaves before returing.**/
	SCTK_SHM_MAPPER_STEP_FINAL_CHECK
};
typedef enum sctk_shm_mapper_steps_e sctk_shm_mapper_steps_t;

/************************** STRUCT *************************/
/**
 * Main structure placed at beggining of the shared segment to sync master and slaves.
**/
struct sctk_shm_mapper_sync_header_s
{
	/**
	 * Counter to sync slaves operations. It start to number of participant and decrement until all are ready (0).
	 * Slaves and master will spin on this until 0.
	**/
        OPA_int_t barrier_cnt;
        /**
         * Counter to sync barrier operations.
        **/
        OPA_int_t barrier_gen;
        /** Counter to known where slave must write their PID (need atomic inc
         * and read), master put at 0. **/
        OPA_int_t cnt_pid;
        /**
         * Number of participants which didn't map the segment to same address
         *at first step, so need more complexe
         * decision from master if not 0.
        **/
        OPA_int_t cnt_invalid;
        /** Initial address obtained by master, if all slaves map with this one,
         * it's fine, don't need more work. **/
        volatile void *master_initial_address;
        /**
         * After the first mapping try, the master write the final address
         *choice here for slaves.
         * Slavles will spin on this until non NULL.
        **/
        OPA_ptr_t final_address;
        /** Just to assert the size on each participant and ensure all request
         * the same one. **/
        sctk_size_t size;
        /** To debug by ensuring the current step. **/
        sctk_shm_mapper_steps_t step;
        /** List of pids of all participant (writed before decrement sync_cnt),
         *so master can use it for more complexe
         * decision if required (to read /proc/[PID]/maps).
        **/
        int *pids;
};
typedef struct sctk_shm_mapper_sync_header_s sctk_shm_mapper_sync_header_t;


/************************* FUNCTION ************************/
//Entry point for user
SCTK_PUBLIC void * sctk_shm_mapper_create(sctk_size_t size,sctk_shm_mapper_role_t role,int participants,sctk_alloc_mapper_handler_t  * handler);
SCTK_PUBLIC void sctk_alloc_shm_remove(void * ptr,sctk_size_t size);

/************************* FUNCTION ************************/
//internal mapper main functions
SCTK_STATIC void * sctk_shm_mapper_slave(sctk_size_t size,int nb_participants,sctk_alloc_mapper_handler_t * handler);
SCTK_STATIC void * sctk_shm_mapper_master(sctk_size_t size,int nb_participants,sctk_alloc_mapper_handler_t * handler);
SCTK_STATIC void * sctk_shm_mapper_master_trivial(sctk_size_t size);

/************************* FUNCTION ************************/
//shm file management
SCTK_STATIC char *sctk_shm_mapper_get_filename(void *option, void *option1);
SCTK_STATIC int sctk_shm_mapper_create_shm_file(const char * filename,sctk_size_t size);
SCTK_STATIC int sctk_shm_mapper_slave_open(const char * filename);
SCTK_STATIC void sctk_shm_mapper_unlink(const char * filename);

/************************* FUNCTION ************************/
//shm memory mapping
SCTK_STATIC void * sctk_shm_mapper_mmap(void * addr,int fd,sctk_size_t size);
SCTK_STATIC void sctk_shm_mapper_remap(int fd, void* old_addr, void* new_addr, sctk_size_t size);
SCTK_STATIC void * sctk_shm_mapper_find_common_addr(sctk_size_t size, int* pids, int participants, void* start);

/************************* FUNCTION ************************/
//shm sync
SCTK_STATIC sctk_shm_mapper_sync_header_t * sctk_shm_mapper_sync_header_init(void * ptr,sctk_size_t size,int participants);
SCTK_STATIC sctk_shm_mapper_sync_header_t * sctk_shm_mapper_sync_header_slave_update(void * ptr,sctk_size_t size,int participants);
SCTK_STATIC void sctk_shm_mapper_barrier( sctk_shm_mapper_sync_header_t * sync_header,sctk_shm_mapper_role_t role,int participants);

/************************* FUNCTION ************************/
//construction of common mapping table to find common free segment
SCTK_STATIC sctk_shm_mapper_segment_t * sctk_alloc_shm_create_free_segment(sctk_addr_t start,sctk_addr_t end);
SCTK_STATIC void sctk_shm_mapper_merge_map(sctk_shm_mapper_segment_t ** free_segments,int pid,void * start);
SCTK_STATIC void sctk_shm_mapper_update_prev_segment(sctk_shm_mapper_segment_t ** free_segments,sctk_shm_mapper_segment_t * prev,sctk_shm_mapper_segment_t * new_next);
SCTK_STATIC void sctk_alloc_shm_create_merge_used_segment(sctk_shm_mapper_segment_t** free_segments, sctk_addr_t start, sctk_addr_t end);
SCTK_STATIC void * sctk_shm_mapper_find_common_addr_in_table(sctk_shm_mapper_segment_t * free_segments,sctk_size_t size);
SCTK_STATIC void sctk_shm_mapper_free_table(sctk_shm_mapper_segment_t * free_segments);
SCTK_STATIC void * sctk_shm_mapper_find_common_addr(sctk_size_t size,int * pids,int participants,void * start);

/************************* FUNCTION ************************/
//fake handler for test with a global variable, please not use this
SCTK_STATIC void sctk_shm_mapper_fake_handler_reset(void);
SCTK_STATIC bool sctk_shm_mapper_fake_handler_send(const char *filename,
                                                   void *option, void *option1);
SCTK_STATIC char *sctk_shm_mapper_fake_handler_recv(void *option,
                                                    void *option1);
extern sctk_alloc_mapper_handler_t sctk_shm_mapper_fake_handler;

/************************* FUNCTION ************************/
//provide a previous fork based hanlder

/************************* FUNCTION ************************/
//provide a MPI based handler

/************************* FUNCTION ************************/
//provide a PMI based handler

#ifdef __cplusplus
}
#endif

#endif //SCTK_SHM_MAPPER
