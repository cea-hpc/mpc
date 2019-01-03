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
/************************** HEADERS ************************/
#include "sctk_shm_mapper.h"
#include "sctk_spinlock.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef MPC_THREAD
#include "mpcthread.h"
#endif


#define assume_m(x,...) if (!(x)) { printf("ERROR: ");printf(__VA_ARGS__);printf("\n");abort(); }
#define fatal(...) {printf("ERROR: ");printf(__VA_ARGS__);printf("\n");abort();}
#define assume_perror(test,perror_string,...) {if (!(test)) {perror(perror_string);fatal(__VA_ARGS__);}}

/************************** MACROS *************************/
/** Provide some MPC macros to work out of MPC for unit tests. **/

/************************* GLOBALS *************************/
/** Static variable used to communicate between slaves and master for fake comm mode (only for test). **/
static volatile const char * sctk_shm_mapper_fake_glob = NULL;
/** Define the struct of fake comm handler (only for test). **/
sctk_alloc_mapper_handler_t sctk_shm_mapper_fake_handler = {
	sctk_shm_mapper_fake_handler_send,
	sctk_shm_mapper_fake_handler_recv,
	sctk_shm_mapper_get_filename,
	NULL,
	NULL
};

/************************* FUNCTION ************************/
/** Reset the global variable of fake handler, need to be done each time before using it. **/
SCTK_STATIC void sctk_shm_mapper_fake_handler_reset(void)
{
	sctk_shm_mapper_fake_glob = 0;
}

/************************* FUNCTION ************************/
/** Fake sync handler used for unit test only as it count on a shared (in sense of thread) global pointer. **/
SCTK_STATIC bool sctk_shm_mapper_fake_handler_send(const char *filename,
                                                   void *option,
                                                   void *option1) {
  assert(filename != NULL);
  sctk_shm_mapper_fake_glob = filename;
  return true;
}

/************************* FUNCTION ************************/
/** Fake sync handler used for unit test only as it count on a shared (in sense of thread) global pointer. **/
SCTK_STATIC char *sctk_shm_mapper_fake_handler_recv(void *option,
                                                    void *option1) {
  while (sctk_shm_mapper_fake_glob == NULL) {
  };
  return strdup((const char *)sctk_shm_mapper_fake_glob);
}

/************************* FUNCTION ************************/
/**
 * Default function used to generate the SHM filename. It use the pid of master.
 * The caller need to free the string it return.
 * @TODO ensure to try another filename if file already exist (with a warning)
**/
SCTK_STATIC char *sctk_shm_mapper_get_filename(void *option, void *option1) {
  char *buffer = malloc(64);
  int res;
  res = sprintf(buffer, "sctk_shm_mapper_%06d.raw", getpid());
  assert(res < 64);
  return buffer;
}

/************************* FUNCTION ************************/
/**
 * Init the structure placed at the begenning of the shared segment the sync slaves and master during
 * the steps required to finish with a mapping on same addresses for all participants.
 * This function must be used only by master.
 * @param ptr Define the base pointer of shared segment mapped on master.
 * @param size Define the size of the shared segment.
 * @param participants Define the number of participants to allocate ensure enough place to store pids.
**/
SCTK_STATIC sctk_shm_mapper_sync_header_t *
sctk_shm_mapper_sync_header_init(void *ptr, sctk_size_t size,
                                 int participants) {
  // vars
  sctk_shm_mapper_sync_header_t *sync_header = ptr;

  // errors
  assert(ptr != NULL);
  assert(participants > 0);

  // debug
  // sctk_nodebug("sctk_shm_mapper_sync_header_init(%p,%d,%lu)",ptr,participants,size);

  // setup entries
  sctk_atomics_store_int(&sync_header->cnt_invalid, 0);
  sctk_atomics_store_int(&sync_header->barrier_cnt, participants);
  sctk_atomics_store_int(&sync_header->barrier_gen, 0);
  sctk_atomics_store_int(&sync_header->cnt_pid, 1);
  sctk_atomics_store_ptr(&sync_header->final_address, NULL);
  sync_header->size = size;
  sync_header->master_initial_address = sync_header;
  sync_header->pids = (int *)(sync_header + 1);
  sync_header->step = SCTK_SHM_MAPPER_STEP_INIT;

  // check alignement of thread/process sync entries
  // Required alignement on their size to be used with atomics or to count on
  // cache synchro
  assert((sctk_addr_t)(&sync_header->cnt_invalid) %
             sizeof(sync_header->cnt_invalid) ==
         0);
  assert((sctk_addr_t)(&sync_header->barrier_cnt) %
             sizeof(sync_header->barrier_cnt) ==
         0);
  assert((sctk_addr_t)(&sync_header->final_address) %
             sizeof(sync_header->final_address) ==
         0);

  // setup pids
  assume_m(sizeof(int) * participants + sizeof(*sync_header) <= size,
           "Not enought size in shared segment to store all pids.");

  memset(sync_header->pids, 0, sizeof(int) * participants);
  sync_header->pids[0] = getpid();

  return sync_header;
}

/************************* FUNCTION ************************/
/**
 * Quick and dirty barrier to sync all participant via the sync_header.
 * @param participants Number of participant to setup the wait counter.
 * @param role Define the role of the caller, must be slaves and strictly 1 master.
 * @TODO rewrite with MPC functions and remove need of role.
**/

static sctk_atomics_int local_gen;
static sctk_spinlock_t gen_lock = 0;
static int gen_init_done = 0;

SCTK_STATIC void sctk_shm_mapper_barrier( sctk_shm_mapper_sync_header_t * sync_header,sctk_shm_mapper_role_t role,int participants)
{
	//errors
	assert(sync_header != NULL);

        sctk_spinlock_lock(&gen_lock);
        if (gen_init_done == 0) {
          sctk_atomics_store_int(&local_gen, -1);
          gen_init_done = 1;
        }
        sctk_spinlock_unlock(&gen_lock);

        sctk_atomics_incr_int(&local_gen);

        // sctk_error("ROLE %d PART %d (GEN %d VAL %d)", role, participants,
        // local_gen, sctk_atomics_load_int
        //				(&sync_header->barrier_cnt));

        while (sctk_atomics_load_int(&local_gen) !=
               sctk_atomics_load_int(&sync_header->barrier_gen)) {
          sctk_atomics_read_write_barrier();
        }

        // sctk_error("VAL %d",sctk_atomics_load_int( &sync_header->barrier_cnt
        // ) );

        // loop until pids equal to 0

        if (sctk_atomics_fetch_and_add_int(&sync_header->barrier_cnt, -1) ==
            1) {
          sctk_atomics_incr_int(&sync_header->barrier_gen);
          sctk_atomics_store_int(&sync_header->barrier_cnt, participants);
        }

        while (sctk_atomics_load_int(&sync_header->barrier_gen) !=
               (sctk_atomics_load_int(&local_gen) + 1)) {
          sctk_atomics_read_write_barrier();
        }
}


/************************* FUNCTION ************************/
/**
 * Function used by slave to try to map the shm segment and notify the master if
 * failed to get the same address.
 * @param ptr Base address of the shared segment mapped on slave.
 * @param size Size of the segment (for check).
 * @param participant Number of participants (for check).
**/
SCTK_STATIC sctk_shm_mapper_sync_header_t * sctk_shm_mapper_sync_header_slave_update(void * ptr,sctk_size_t size,int participants)
{
	//vars
	sctk_shm_mapper_sync_header_t * sync_header = ptr;
	//as we are to sure to have same mapping, need to recompute the pids address.
	int * pids = (int*)(sync_header + 1);
	int cnt_pid;

	//errors
	assert(ptr != NULL);
	assume_m(sync_header->size == size,"Some slaves have a different size than master : %ld != %ld.",sync_header->size,size);

	//update the PID list
	/** TODO USE ATOMICS **/
        cnt_pid = sctk_atomics_fetch_and_add_int(&sync_header->cnt_pid, 1);
        assert(cnt_pid > 0 && cnt_pid < participants);
        pids[cnt_pid] = getpid();

        // if didn't has the same address need to notifiy
        if (ptr != sync_header->master_initial_address)
          sctk_atomics_store_int(&sync_header->cnt_invalid, 1);

        return sync_header;
}

/************************* FUNCTION ************************/
/**
 * User entry point to create a shared memory segment mapped on same address range on each participants.
 * The function will abord on failure.
 * @param size Define the size of the requested shared segment. Must be multiple of page size (SCTK_SHM_MAPPER_PAGE_SIZE).
 * @param role Define the role of the caller. Must be strictly 1 master and any number of slaves.
 * @param participants Total number of participants (1 master + slaves).
 * @param handler List of functions to use the initial exchange of SHM filename. By this way one can use MPI, PMI, ....
**/
SCTK_PUBLIC void * sctk_shm_mapper_create(sctk_size_t size,sctk_shm_mapper_role_t role,int participants,sctk_alloc_mapper_handler_t  * handler)
{
	//errors
	assume_m(participants > 0,"Number of participant must be greater than 0.");
	assume_m(handler != NULL,"You must provide a valid handler to exchange the SHM filename.");
	if(size == 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE != 0){
        printf("Size must be non NULL and multiple of page size (%lu bytes), get %lu\n",SCTK_SHM_MAPPER_PAGE_SIZE,size);
        abort();
    }

    // Flush the barrier generation value
    sctk_spinlock_lock(&gen_lock);
    gen_init_done = 0;
    sctk_spinlock_unlock(&gen_lock);

    // select the method
    switch (role) {
    case SCTK_SHM_MAPPER_ROLE_MASTER:
      if (participants == 1)
        return sctk_shm_mapper_master_trivial(size);
      else
        return sctk_shm_mapper_master(size, participants, handler);
    case SCTK_SHM_MAPPER_ROLE_SLAVE:
      return sctk_shm_mapper_slave(size, participants, handler);
    default:
      fatal("Invalid role on sctk_shm_mapper.");
    }
}

/************************* FUNCTION ************************/
/**
 * User function to unmap the memory segment it obtain.
 * @param ptr Base address returned by sctk_shm_mapper_create().
 * @param size Size of the segment (same than the one passed to sctk_shm_mapper_create()).
**/
SCTK_PUBLIC void sctk_alloc_shm_remove(void * ptr,sctk_size_t size)
{
	//vars
	int status;

	//errors
	assert(ptr != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	status = munmap(ptr,size);
	assume_perror(status == 0,"munmap","Failed to use munmap on shm segment to remap it to another address.");
}

/************************* FUNCTION ************************/
/**
 * Function to implement the operations done by all slaves.
 * It will :
 *    - Use handler to wait and get filename from master.
 *    - Open and try a first mapping of the shm segment.
 *    - Mark their PID into sync structure for master.
 *    - If segment is not on same address than master, notify to request change.
 *    - Wait final address from master.
 *    - If address changed, remap the segment on final one.
 *    - Close the shm file.
 * For parameteres, see sctk_shm_mapper_create().
**/
SCTK_STATIC void * sctk_shm_mapper_slave(sctk_size_t size,int participants,sctk_alloc_mapper_handler_t * handler)
{
	//vars
	char * filename;
	int fd;
	void * ptr;
	sctk_shm_mapper_sync_header_t * sync_header;

	//check args
	assert(handler != NULL);
	assert(handler->recv_handler != NULL);
	assert(handler->send_handler != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);
	assume_m(participants > 1,"Required more than 1 participant.");

	//get filename
        filename = handler->recv_handler(handler->option, handler->option1);
        assume_m(filename != NULL, "Failed to get the SHM filename.");

        // firt try to map
        fd = sctk_shm_mapper_slave_open(filename);
        ptr = sctk_shm_mapper_mmap(NULL, fd, size);
        sync_header =
            sctk_shm_mapper_sync_header_slave_update(ptr, size, participants);

        // wait all
        sctk_shm_mapper_barrier(sync_header, SCTK_SHM_MAPPER_ROLE_SLAVE,
                                participants);

        // wait final decision of master
        while (sctk_atomics_load_ptr(&sync_header->final_address) == NULL) {
	#ifdef MPC_THREAD
          mpc_thread_yield();
	#endif
        }

        sctk_shm_mapper_barrier(sync_header, SCTK_SHM_MAPPER_ROLE_SLAVE,
                                participants);

        // remap if required
        ptr = (void *)sctk_atomics_load_ptr(&sync_header->final_address);
        sctk_shm_mapper_remap(fd, sync_header, ptr, size);
        sync_header = ptr;

        // final sync
        sctk_shm_mapper_barrier(sync_header, SCTK_SHM_MAPPER_ROLE_SLAVE,
                                participants);

        // can close
        close(fd);

        // free temp memory and return
        free(filename);
        return sync_header;
}

/************************* FUNCTION ************************/
/**
 * Manage the trivial case with participant equal to 1, so master only. In that case, don't need
 * to create shm..... directly map a new segment with MPC_SHARED property in case of fork.
 * @param size Define the size of the requested segment. Required multiple of page size.
**/
SCTK_STATIC void * sctk_shm_mapper_master_trivial(sctk_size_t size)
{
	//vars
	void * ptr;

	//errors
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//use mmap directly
	ptr = mmap(NULL , size , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS , 0, 0);
	assume_perror(ptr != MAP_FAILED,"mmap","Failed to mmap the SHM file into memory with size = %ld.",size);

	return ptr;
}

/************************* FUNCTION ************************/
/**
 * Function to implement the operations done by the master
 * It will :
 *    - Use handler generate a filename.
 *    - Create and open SHM file.
 *    - Map it on memory.
 *    - Setup the sync structure on the bottom of shared segment for next operations.
 *    - Send the filename to everybody.
 *    - Wait mapping answer of all slaves.
 *    - If at least on slave didn't naturally obtain the same address, read /proc/[PID]/maps of all
 *      participant to find a common address (starting after the current one to avoid falling on
 *      conflicting range).
 *    - Remap if needed
 *    - Notify all slaves of the final address.
 *    - Wait final mapping of everybody.
 *    - Remove the SHM file and close the file descriptor.
 * For parameteres, see sctk_shm_mapper_create().
**/
SCTK_STATIC void * sctk_shm_mapper_master(sctk_size_t size,int participants,sctk_alloc_mapper_handler_t * handler)
{
	//vars
	char * filename;
	int id;
	int fd;
	bool status;
	void * ptr;
	sctk_shm_mapper_sync_header_t * sync_header;

        // check args
        assert(handler != NULL);
        assert(handler->recv_handler != NULL);
        assert(handler->send_handler != NULL);
        assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);
        assume_m(participants > 1, "Required more than 1 participant.");

        // debug
        // sctk_nodebug("sctk_shm_mapper_master(%lu,%d,handler)",size,participants);

        // get id
        id = getpid();

        // get filename
        filename = handler->gen_filename(handler->option, handler->option1);

        // create file and map it
        fd = sctk_shm_mapper_create_shm_file(filename, size);
        ptr = sctk_shm_mapper_mmap(NULL, fd, size);
        sync_header = sctk_shm_mapper_sync_header_init(ptr, size, participants);

        // sync filename
        sync_header->step = SCTK_SHM_MAPPER_SYNC_FILENAME;
        status =
            handler->send_handler(filename, handler->option, handler->option1);
        assume_m(status,
                 "Fail to send the SHM filename to other participants.");

        // wait all slaves
        sync_header->step = SCTK_SHM_MAPPER_STEP_FIRST_SLAVE_MAPPING;
        sctk_shm_mapper_barrier(sync_header, SCTK_SHM_MAPPER_ROLE_MASTER,
                                participants);

        // if some are not with same address,
        if (sctk_atomics_load_int(&sync_header->cnt_invalid) > 0) {
          // search common free space
          ptr = sctk_shm_mapper_find_common_addr(size, sync_header->pids,
                                                 participants, ptr);
          // remap locally
          sctk_shm_mapper_remap(fd, sync_header, ptr, size);
          sync_header = ptr;
          sync_header->pids = (int *)(sync_header + 1);
        }
        // mark final address and wait all before exit
        sync_header->step = SCTK_SHM_MAPPER_STEP_FINAL_CHECK;
        sctk_atomics_store_ptr(&sync_header->final_address, sync_header);

        sctk_shm_mapper_barrier(sync_header, SCTK_SHM_MAPPER_ROLE_MASTER,
                                participants);
        sctk_shm_mapper_barrier(sync_header, SCTK_SHM_MAPPER_ROLE_MASTER,
                                participants);
        // can close and remove the file
        close(fd);
        sctk_shm_mapper_unlink(filename);

        // free temp memory and return
        free(filename);
        return sync_header;
}

/************************* FUNCTION ************************/
/**
 * Just a helper function to setup a new entry for free segment's table.
**/
SCTK_STATIC sctk_shm_mapper_segment_t * sctk_alloc_shm_create_free_segment(sctk_addr_t start, sctk_addr_t end)
{
	//vars
	sctk_shm_mapper_segment_t * segment;

	//errors
	assert(start < end);

	//init
	segment = malloc(sizeof(sctk_shm_mapper_segment_t));
	segment->next = NULL;
	segment->start = start;
	segment->end = end;

	return segment;
}

/************************* FUNCTION ************************/
/**
 * Update the previous free segment and if NULL, update the head pointer of the list.
**/
SCTK_STATIC void sctk_shm_mapper_update_prev_segment(sctk_shm_mapper_segment_t ** free_segments,sctk_shm_mapper_segment_t * prev,sctk_shm_mapper_segment_t * new_next)
{
	if (prev == NULL)
		*free_segments = new_next;
	else
		prev->next = new_next;
}

/************************* FUNCTION ************************/
/**
 * Merge used segment on the list of free onces.
 * It will split or remove the overlapping free segments.
 * @param free_segments List of free segments.
 * @param start Starting address of the mapped segment.
 * @param start End address of the mapped segment.
**/
SCTK_STATIC void sctk_alloc_shm_create_merge_used_segment(sctk_shm_mapper_segment_t ** free_segments,sctk_addr_t start, sctk_addr_t end)
{
	//vars
	sctk_shm_mapper_segment_t * seg = *free_segments;
	sctk_shm_mapper_segment_t * prev_seg = NULL;
	sctk_shm_mapper_segment_t * tmp = NULL;
	
	//errors
	assert(*free_segments != NULL);
	assert(start < end);

	//simple move until reach collision
	while (seg != NULL && seg->end <= start)
	{
		prev_seg = seg;
		seg = seg->next;
	}

	//loop over conflicting free segments
	while (seg != NULL && seg->start < end)
	{
		//error
		assert(seg->start < seg->end);
		//the new segment totaly overlap the curent one => remove it
		if (start <= seg->start && end >= seg->end)
		{
			sctk_shm_mapper_update_prev_segment(free_segments,prev_seg,seg->next);
			free(seg);
			if (prev_seg == NULL)
				seg = *free_segments;
			else
				seg = prev_seg;
		} else if (start > seg->start && end < seg->end) {
			//the new segment is contained by the current one => split
			//create a new one for the left part, same start but cut at "start"
			tmp = sctk_alloc_shm_create_free_segment(seg->start,start);
			tmp->next = seg;
			sctk_shm_mapper_update_prev_segment(free_segments,prev_seg,tmp);
			prev_seg = tmp;
			//resize the old one to form right part (move start to "end")
			seg->start = end;
		} else if (start <= seg->start && end > seg->start && end < seg->end) {
			//the new segment is on the left part of the current one
			//cut the left part by moving start
			seg->start = end;
		} else if (end >= seg->end && start < seg->end && start > seg->start) {
			//the new segment is on the right part of the current one
			//cut the left part by moving end
			seg->end = start;
		} else {
			fatal("Their is a mistake in computation of segment overlapping, we may never execute this line.");
		}
		//move to next
		prev_seg = seg;
		if (seg != NULL)
			seg = seg->next;
	}
}

/************************* FUNCTION ************************/
/**
 * Read the /proc/[PID]/maps of a given process and merge on the given free table, by this way
 * we can iteratively construct a global free table to find a good place for our segment.
 * @param free_segment List of free segment to merge on.
 * @param pid Pid for which to read the table.
 * @param start Ignore all addresses between this one (avoid to fall on head and other strange mappings).
**/
SCTK_STATIC void sctk_shm_mapper_merge_map(sctk_shm_mapper_segment_t ** free_segments,int pid,void * start)
{
	//vars
	char filename[128];
	char buffer[4096];
	int res;
	sctk_addr_t segment_start;
	sctk_addr_t segment_end;
	FILE * fp;

	//errors
	assert(free_segments != NULL);
	assert(pid > 0);

	//compute name of segments mapping file (/dev/{PID}/maps
	res = sprintf(filename,"/proc/%d/maps",pid);
	assert(res < (int)sizeof(filename));
	
	//open the segment mapping file (/dev/{PID}/maps)
	fp = fopen(filename,"r");
	assume_perror(fp != NULL,filename,"Error while opening memory mapping file in /proc to find common free address range.");

	//read all rentries
	while (fgets(buffer,sizeof(buffer),fp))
	{
		//extract addresses
		res = sscanf(buffer,"%lx-%lx ",&segment_start,&segment_end);
		assume_m(res == 2,"Failed to read format of /proc/[PID]/maps.");
		assert(segment_end > segment_start);

		//if after start, merge
		if (segment_end >= (sctk_addr_t)start)
			sctk_alloc_shm_create_merge_used_segment(free_segments,segment_start,segment_end);
	}

	//close the file
	fclose(fp);
}

/************************* FUNCTION ************************/
/**
 * Once the global table is fully merged, we can search for a free segment in it with a simple loop
 * over free segments.
**/
SCTK_STATIC void * sctk_shm_mapper_find_common_addr_in_table(sctk_shm_mapper_segment_t * free_segments,sctk_size_t size)
{
	//vars
	sctk_shm_mapper_segment_t * seg = free_segments;

	//loop until find one of good size
	while (seg != NULL && (seg->end - seg->start) < size)
		seg = seg->next;

	//error
	assume_m(seg != NULL,"Can't find a common segment for the given size.");

	//ok found
	assert((seg->end - seg->start) >= size);
	return (void*)seg->start;
}

/************************* FUNCTION ************************/
/**
 * Free the memmory used by the table.
**/
SCTK_STATIC void sctk_shm_mapper_free_table(sctk_shm_mapper_segment_t* free_segments)
{
	//vars
	sctk_shm_mapper_segment_t * seg = free_segments;
	sctk_shm_mapper_segment_t * next = NULL;

	//loop until find one of good size
	while (seg != NULL)
	{
		next = seg->next;
		free(seg);
		seg = next;
	}
}

/************************* FUNCTION ************************/
/**
 * Main function to read /proc/[PID]/maps of all participants and to merge them on a global free
 * table. At the end it search a common free segment and return its base address.
 * @param start Ignore all addresses smaller than this one to avoid falling on heap and others. Can use
 * the default address we obtain with first non forced mapping.
**/
SCTK_STATIC void * sctk_shm_mapper_find_common_addr(sctk_size_t size,int * pids,int participants,void * start)
{
	//vars
	sctk_shm_mapper_segment_t * free_segments;
	int i;
	void * res;

	//errors
	assert(size > 0);
	assert(pids != NULL);
	assert(participants > 0);
	
	//init the global table of free segments, by default all the memory is free
	//to speedup we can safetely ignore the first part before the address we obtained
	//it avoid to fall on the heap
	free_segments = sctk_alloc_shm_create_free_segment((sctk_addr_t)start,(sctk_addr_t)(-1));

	//merge maps of all pids
        for (i = 0; i < participants; i++) {
          sctk_shm_mapper_merge_map(&free_segments, pids[i], start);
        }

        // now search a solution
        res = sctk_shm_mapper_find_common_addr_in_table(free_segments, size);
        assert(res >= start);

        // free the temporary table
        sctk_shm_mapper_free_table(free_segments);

        return res;
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_shm_mapper_remap(int fd,void * old_addr,void * new_addr,sctk_size_t size)
{
	//vars
	void * res;
	//errors
	assert(fd != -1);
	assert(old_addr != NULL);
	assert(new_addr != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//trivial
	if (old_addr == new_addr)
		return;

	//unmap an re-mmap
	sctk_alloc_shm_remove(old_addr,size);
	res = sctk_shm_mapper_mmap(new_addr,fd,size);
}

/************************* FUNCTION ************************/
SCTK_STATIC void * sctk_shm_mapper_mmap(void * addr,int fd,sctk_size_t size)
{
	//vars
	void * ptr;

	//errors
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//map
	if (addr != NULL)
		ptr = mmap(addr , size , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_FIXED , fd, 0);
	else
		ptr = mmap(NULL , size , PROT_READ | PROT_WRITE , MAP_SHARED , fd, 0);

	//errors
	if (ptr == MAP_FAILED)
	{
		perror("mmap");
		fatal("Failed to mmap the SHM file into memory.");
	} else if (addr != NULL && ptr != addr) {
		fatal("Failed to map the shm segment to the requested address, get another one in return to mmap.");
	}

	return ptr;
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_shm_mapper_unlink(const char * filename)
{
	//vars
	int res;

	//errors
	assert(filename != NULL);

	//remove the file
	res = shm_unlink(filename);
	assume_perror(res == 0,filename,"Failed to remove the SHM file.");
}

/************************* FUNCTION ************************/
SCTK_STATIC int sctk_shm_mapper_slave_open(const char * filename)
{
	//vars
	int fd;

	//errors
	assert(filename != NULL);

	//open
	fd = shm_open(filename, O_RDWR,S_IRUSR | S_IWUSR);
	assume_perror(fd != -1,filename,"Error while opening file for SHM mapping.");

	return fd;
}

/************************* FUNCTION ************************/
SCTK_STATIC int sctk_shm_mapper_create_shm_file(const char * filename,sctk_size_t size)
{
	//vars
	int fd;
	int res;
    
	//errors
	assert(filename != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//open
	fd = shm_open(filename,O_CREAT | O_RDWR | O_EXCL,S_IRUSR | S_IWUSR);
	assume_perror(fd != -1,filename,"Error while creating file for SHM mapping.");

	//force mapping memory
	res = ftruncate(fd,size);
	assume_perror(fd != -1,filename,"Error while resizing the file for SHM mapping with size : %ld.",size);

	return fd;
}
