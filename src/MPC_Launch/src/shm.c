#include <mpc_launch_shm.h>

#include <mpc_common_rank.h>
#include <mpc_common_debug.h>
#include <mpc_launch_pmi.h>

#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

/*********************
 * PMI DATA EXCHANGE *
 *********************/

static inline char * __get_per_node_unique_name(char * buff, int len)
{
	buff[0] = '\0';

	char local_buff[128];
	snprintf(local_buff, 128, "/tmp/mpc-shm-XXXXXX");
	int fd = mkstemp(local_buff);

	if(fd < 0)
	{
		perror("mkstemp");
		return NULL;
	}

	close(fd);
	unlink(local_buff);

	char * slash = strrchr(local_buff, '/');

	if(slash)
	{
		snprintf(buff, len, "%s", slash);
		return buff;
	}
	else
	{
		return NULL;
	}
}

static inline char * __get_per_node_segment_key(char *buff, int len)
{
	static unsigned int segment_id = 0;
	snprintf(buff, len, "mpc-shm-filename-%d-%d", mpc_common_get_node_rank(), ++segment_id);
    return buff;
}


static inline void * __map_shm_segment_pmi(size_t size)
{
	char segment_name[128];
	char segment_key[32];
	__get_per_node_segment_key(segment_key, 32);

	int shm_fd = -1;

	/* We need an unique name let 0 in each process define it */
	if(mpc_common_get_local_process_rank() == 0)
	{
		__get_per_node_unique_name(segment_name, 128);

		mpc_launch_pmi_put(segment_name, segment_key, 1 /* local to node */);

		/* Time to create the segment */
		shm_fd = shm_open(segment_name, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0600);

		if(shm_fd < 0)
		{
			perror("shm_open");
			mpc_common_debug_fatal("Failed to open shm segment (as leading processes)");
		}

		/* Truncate to size */
		int ret = ftruncate(shm_fd, size);

		if(ret < 0)
		{
			perror("ftruncate");
			mpc_common_debug_fatal("Failed to truncate shm segment to size %ld", size);
		}
	}

	mpc_launch_pmi_barrier();

	/* Now time to retrieve the local segments in non leaders */
	if(mpc_common_get_local_process_rank() != 0)
	{
        segment_name[0] = '\0';

		/* First get the segment name */
        mpc_launch_pmi_get(segment_name, 128, segment_key, 1 /* local to node */);

        if(!strlen(segment_name))
        {
            mpc_common_debug_fatal("Failed to retrieve shm segment name");
        }

		shm_fd = shm_open(segment_name, O_RDWR, 0600);

		if(shm_fd < 0)
		{
			perror("shm_open");
			mpc_common_debug_fatal("Failed to open shm segment (as secondary processes)");
		}
	}

    assume(0 < shm_fd);

	void * ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if(ret == NULL)
    {
        perror("mmap");
		mpc_common_debug_fatal("Failed to map shm segment");
    }

	mpc_launch_pmi_barrier();

	if(mpc_common_get_local_process_rank() == 0)
	{
		shm_unlink(segment_name);
	}

    return ret;
}

/*********************
 * MPI DATA EXCHANGE *
 *********************/

static inline void * __map_shm_segment_mpi(size_t size, mpc_launch_shm_exchange_params_t * params)
{
	assume(params != NULL);
	assume(params->mpi.rank != NULL);
	assume(params->mpi.barrier != NULL);
	assume(params->mpi.bcast != NULL);

	int my_rank = (params->mpi.rank)(params->mpi.pcomm);

	char segment_name[128];
	segment_name[0] = '\0';

	int shm_fd = -1;

	if(my_rank == 0)
	{
		__get_per_node_unique_name(segment_name, 128);

		/* Time to create the segment */
		shm_fd = shm_open(segment_name, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0600);

		if(shm_fd < 0)
		{
			perror("shm_open");
			mpc_common_debug_fatal("Failed to open shm segment (as leading processes)");
		}

		/* Truncate to size */
		int ret = ftruncate(shm_fd, size);

		if(ret < 0)
		{
			perror("ftruncate");
			mpc_common_debug_fatal("Failed to truncate shm segment to size %ld", size);
		}
	}

	(params->mpi.bcast)(segment_name, 128, params->mpi.pcomm);

	assume(strlen(segment_name) != 0);

	if(my_rank != 0)
	{
		shm_fd = shm_open(segment_name, O_RDWR, 0600);

		if(shm_fd < 0)
		{
			perror("shm_open");
			mpc_common_debug_fatal("Failed to open shm segment (as secondary processes)");
		}
	}

	assume(0 < shm_fd);

	void * ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if(ret == NULL)
    {
        perror("mmap");
		mpc_common_debug_fatal("Failed to map shm segment");
    }

	(params->mpi.barrier)(params->mpi.pcomm);

	if(my_rank == 0)
	{
		shm_unlink(segment_name);
	}

	return ret;
}

/********************************
 * MPC LAUNCH SHM MAP INTERFACE *
 ********************************/

void * mpc_launch_shm_map(size_t size, mpc_launch_shm_exchange_method_t method, mpc_launch_shm_exchange_params_t * params)
{
    switch (method)
    {
		case MPC_LAUNCH_SHM_USE_MPI:
			return __map_shm_segment_mpi(size, params);
		case MPC_LAUNCH_SHM_USE_PMI:
			return __map_shm_segment_pmi(size);
    }

    return NULL;
}

int mpc_launch_shm_unmap(void *ptr, size_t size)
{
	return munmap(ptr, size);
}