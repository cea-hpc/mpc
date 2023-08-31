#include "romio_ctx.h"

#include <string.h>
#include <sctk_alloc.h>
#include <mpc_common_debug.h>

__thread struct mpc_thread_romio_ctx_s *mpc_thread_romio_ctx_storage = NULL;


struct mpc_thread_romio_ctx_s *_mpc_thread_romio_ctx_init(void)
{
	struct mpc_thread_romio_ctx_s *ret = sctk_malloc(sizeof(struct mpc_thread_romio_ctx_s) );

	assume(ret != NULL);

	/* All default to 0 / NULL */
	memset(ret, 0, sizeof(struct mpc_thread_romio_ctx_s) );

	ret->ADIOI_Flattened_type_keyval = -7 /* MPC_KEYVAL_INVALID */;
	ret->ADIOI_cb_config_list_keyval = -7; /* MPC_KEYVAL INVALID */
	ret->ADIO_Init_keyval            = -7 /* MPC_KEYVAL_INVALID */;

	ret->ADIOI_DFLT_ERR_HANDLER = -6 /* MPC_ERRORS_RETURN */;

	ret->ADIOI_syshints  = -1 /* MPI_INFO_NULL */;
	ret->ADIO_same_amode = 0 /* MPI_OP_NULL */;

	return ret;
}
