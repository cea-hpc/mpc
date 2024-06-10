#ifndef MPC_THREAD_ROMIO_CTX_H_
#define MPC_THREAD_ROMIO_CTX_H_

#include <stdlib.h>

struct mpc_thread_romio_ctx_s
{
	/* Pointers */

	void * ADIOI_Datarep_head;
	void * ADIOI_Ftable;
	void * ADIOI_Reqtable;
	void * MPIR_Infotable;
	void * flatBuf_global;

	void * stripeWriteLens;
	void * stripeWriteOffsets;

	void * token_ptr;
	void * yylval;

	/* Integers */

	int ADIOI_Ftable_ptr;
	int ADIOI_Ftable_max;
	int ADIOI_Reqtable_ptr;
	int ADIOI_Reqtable_max;
	int MPIR_Infotable_ptr;
	int MPIR_Infotable_max;
	int ADIOI_Direct_read;
	int ADIOI_Direct_write;
	int ADIOI_cb_config_list_keyval;
	int ADIOI_Flattened_type_keyval;
	int ADIO_Init_keyval;

	int amountOfStripedDataExpected;

	int gpfsmpio_balancecontig;
	int gpfsmpio_bg_nagg_pset;
	int gpfsmpio_bridgeringagg;
	int gpfsmpio_comm;
	int gpfsmpio_devnullio;
	int gpfsmpio_p2pcontig;
	int gpfsmpio_pthreadio;
	int gpfsmpio_timing;
	int gpfsmpio_timing2;
	int gpfsmpio_timing_cw_level;
	int gpfsmpio_tuneblocking;

	int iWasUsedStripingAgg;

	int lastDataTypeExtent;
	int lastFlatBufIndice;

	int numStripesUsed;

	int romio_onesided_always_rmw;
	int romio_onesided_inform_rmw;
	int romio_onesided_no_rmw;
	int romio_read_aggmethod;
	int romio_tunegather;
	int romio_write_aggmethod;

	/* MPI_Err_handler == INT */
	int ADIOI_DFLT_ERR_HANDLER;

	/* MPC_Info == INT */
	int ADIOI_syshints;

	/* MPI_Op^== INT */
	int ADIO_same_amode;

	/* MPIX_Grequest_class == INT */
	int ADIOI_GEN_greq_class;

	/* Long */
	long bglocklessmpio_f_type;

	/* size_t */
	size_t bufTypeExtent_global;
	size_t lastIndiceOffset;

	/* Double */
	double gpfsmpio_prof_cw[21];
	double gpfsmpio_prof_cr[21];
};

extern __thread struct mpc_thread_romio_ctx_s * mpc_thread_romio_ctx_storage;


struct mpc_thread_romio_ctx_s *_mpc_thread_romio_ctx_init(void);

static inline struct mpc_thread_romio_ctx_s * mpc_thread_romio_ctx_storage_get()
{
	if(!mpc_thread_romio_ctx_storage)
	{
		mpc_thread_romio_ctx_storage = _mpc_thread_romio_ctx_init();
	}

	return mpc_thread_romio_ctx_storage;
}


#define _MPC_THREAD_ROMIO_CTX_GET(member) (void*)(&(mpc_thread_romio_ctx_storage_get())->member)
#define MPC_THREAD_ROMIO_CTX_GET(member, type) *((type *)_MPC_THREAD_ROMIO_CTX_GET(member))

#endif /* MPC_THREAD_ROMIO_CTX_H_ */
