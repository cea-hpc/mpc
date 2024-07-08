#ifndef SHM_COLL_H
#define SHM_COLL_H

#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>
#include <mpc_lowcomm_types.h>

/******************************** STRUCTURE *********************************/
struct mpc_lowcomm_coll_s;

struct shared_mem_barrier_sig
{
	OPA_ptr_t *   sig_points;
	volatile int *tollgate;
	OPA_int_t     fare;
	OPA_int_t     counter;
};

int sctk_shared_mem_barrier_sig_init(struct shared_mem_barrier_sig *shmb,
                                     int nb_task);
int sctk_shared_mem_barrier_sig_release(struct shared_mem_barrier_sig *shmb);

struct shared_mem_barrier
{
	OPA_int_t counter;
	OPA_int_t phase;
};

int sctk_shared_mem_barrier_init(struct shared_mem_barrier *shmb, int nb_task);
int sctk_shared_mem_barrier_release(struct shared_mem_barrier *shmb);

#define MPC_LOWCOMM_COMM_SHM_COLL_BUFF_MAX_SIZE    1024

union shared_mem_buffer
{
	float  f[MPC_LOWCOMM_COMM_SHM_COLL_BUFF_MAX_SIZE];
	int    i[MPC_LOWCOMM_COMM_SHM_COLL_BUFF_MAX_SIZE];
	double d[MPC_LOWCOMM_COMM_SHM_COLL_BUFF_MAX_SIZE];
	char   c[MPC_LOWCOMM_COMM_SHM_COLL_BUFF_MAX_SIZE];
};

struct shared_mem_reduce
{
	volatile int *           tollgate;
	OPA_int_t                fare;
	mpc_common_spinlock_t *  buff_lock;
	OPA_int_t                owner;
	OPA_int_t                left_to_push;
	volatile void *          target_buff;
	union shared_mem_buffer *buffer;
	int                      pipelined_blocks;
};

int sctk_shared_mem_reduce_init(struct shared_mem_reduce *shmr, int nb_task);
int sctk_shared_mem_reduce_release(struct shared_mem_reduce *shmr);

struct shared_mem_bcast
{
	OPA_int_t               owner;
	OPA_int_t               left_to_pop;

	volatile int *          tollgate;
	OPA_int_t               fare;

	union shared_mem_buffer buffer;

	OPA_ptr_t               to_free;
	void *                  target_buff;
	int                     scount;
	size_t                  stype_size;
	int                     root_in_buff;
};

int sctk_shared_mem_bcast_init(struct shared_mem_bcast *shmb, int nb_task);
int sctk_shared_mem_bcast_release(struct shared_mem_bcast *shmb);

struct shared_mem_gatherv
{
	OPA_int_t     owner;
	OPA_int_t     left_to_push;

	volatile int *tollgate;
	OPA_int_t     fare;

	/* Leaf infos */
	OPA_ptr_t *   src_buffs;

	/* Root infos */
	void *        target_buff;
	const int *   counts;
	int *         send_count;
	size_t *      send_type_size;
	const int *   disps;
	size_t        rtype_size;
	int           rcount;
	int           let_me_unpack;
};

int sctk_shared_mem_gatherv_init(struct shared_mem_gatherv *shmgv, int nb_task);
int sctk_shared_mem_gatherv_release(struct shared_mem_gatherv *shmgv);

struct shared_mem_scatterv
{
	OPA_int_t               owner;
	OPA_int_t               left_to_pop;

	volatile int *          tollgate;
	OPA_int_t               fare;
	mpc_lowcomm_datatype_t *recv_types;

	/* Root infos */
	OPA_ptr_t *             src_buffs;
	int                     was_packed;
	size_t                  stype_size;
	mpc_lowcomm_datatype_t  send_type;
	int *                   counts;
	int *                   disps;
};

int sctk_shared_mem_scatterv_init(struct shared_mem_scatterv *shmgv,
                                  int nb_task);
int sctk_shared_mem_scatterv_release(struct shared_mem_scatterv *shmgv);

struct sctk_shared_mem_a2a_infos
{
	mpc_lowcomm_datatype_t sendtype;
	mpc_lowcomm_datatype_t recvtype;

	ssize_t                stype_size;
	const void *           source_buff;
	void **                packed_buff;
	const int *            disps;
	const int *            counts;
};

struct shared_mem_a2a
{
	struct sctk_shared_mem_a2a_infos **infos;
	volatile int                       has_in_place;
};

int sctk_shared_mem_a2a_init(struct shared_mem_a2a *shmaa, int nb_task);
int sctk_shared_mem_a2a_release(struct shared_mem_a2a *shmaa);


/**
 *  \brief This structure describes the pool allocated context for node local coll
 */

struct sctk_per_node_comm_context
{
	struct shared_mem_barrier shm_barrier;
};


struct sctk_comm_coll
{
	int                                init_done;
	volatile int *                     coll_id;
	struct shared_mem_barrier          shm_barrier;
	struct shared_mem_barrier_sig      shm_barrier_sig;
	int                                reduce_interleave;
	struct shared_mem_reduce *         shm_reduce;
	int                                bcast_interleave;
	struct shared_mem_bcast *          shm_bcast;
	struct shared_mem_gatherv          shm_gatherv;
	struct shared_mem_scatterv         shm_scatterv;
	struct shared_mem_a2a              shm_a2a;
	int                                comm_size;
	struct sctk_per_node_comm_context *node_ctx;
};

struct sctk_comm_coll * sctk_comm_coll_init(int nb_task);
int sctk_comm_coll_release(struct sctk_comm_coll *coll);


/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */
static inline int __sctk_comm_coll_get_id(struct sctk_comm_coll *coll,
                                          int rank)
{
	return coll->coll_id[rank]++;
}

static inline int sctk_comm_coll_get_id_red(struct sctk_comm_coll *coll,
                                            int rank)
{
	return __sctk_comm_coll_get_id(coll, rank) & (coll->reduce_interleave - 1);
}

static inline struct shared_mem_reduce *sctk_comm_coll_get_red(struct sctk_comm_coll *coll, __UNUSED__ int rank)
{
	int xid = sctk_comm_coll_get_id_red(coll, rank);

	return &coll->shm_reduce[xid];
}

static inline int sctk_comm_coll_get_id_bcast(struct sctk_comm_coll *coll,
                                              int rank)
{
	return __sctk_comm_coll_get_id(coll, rank) & (coll->bcast_interleave - 1);
}

static inline struct shared_mem_bcast *sctk_comm_coll_get_bcast(struct sctk_comm_coll *coll, __UNUSED__ int rank)
{
	int xid = sctk_comm_coll_get_id_bcast(coll, rank);

	return &coll->shm_bcast[xid];
}
/* NOLINTEND(clang-diagnostic-unused-function) */

int sctk_per_node_comm_context_init(struct sctk_per_node_comm_context *ctx,
                                    mpc_lowcomm_communicator_t comm, int nb_task);

int sctk_per_node_comm_context_release(struct sctk_per_node_comm_context *ctx);



#endif /* SHM_COLL_H */
