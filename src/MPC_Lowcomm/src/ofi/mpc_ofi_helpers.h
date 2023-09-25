#ifndef MPC_OFI_HELPERS_H
#define MPC_OFI_HELPERS_H

#include <stdint.h>
#include <stdio.h>

#include <mpc_common_debug.h>

#include <rdma/fabric.h>
#include <rdma/fi_errno.h>

/***********
* DEFINES *
***********/

#define MPC_OFI_ADDRESS_LEN    1024

/*****************
 * ALLOC ALIGNED *
 *****************/

typedef struct mpc_ofi_aligned_mem_s
{
	void * orig;
	void * ret;
}mpc_ofi_aligned_mem_t;

mpc_ofi_aligned_mem_t mpc_ofi_alloc_aligned(size_t size);
void mpc_ofi_free_aligned(mpc_ofi_aligned_mem_t * mem);

/*************
* PRINTINGS *
*************/

int mpc_ofi_decode_cq_flags(uint64_t flags);
int mpc_ofi_decode_mr_mode(uint64_t flags);

/************
* RETCODES *
************/

#define MPC_OFI_DUMP_ERROR(a)    do{                                                                                                \
		int __ret = (a);                                                                                                    \
		if(__ret < 0){                                                                                                      \
			mpc_common_errorpoint_fmt("[MPC_OFI]@%s:%d: %s\n%s(%d)\n", __FILE__, __LINE__, #a, fi_strerror(-__ret), __ret); \
		}                                                                                                                   \
}while(0)

#define MPC_OFI_CHECK_RET(a)    do{                                                                                                \
		int __ret = (a);                                                                                                    \
		if(__ret < 0){                                                                                                      \
			mpc_common_errorpoint_fmt("[MPC_OFI]@%s:%d: %s\n%s(%d)\n", __FILE__, __LINE__, #a, fi_strerror(-__ret), __ret); \
			return __ret;                                                                                                    \
		}                                                                                                                   \
}while(0)

/*********
* HINTS *
*********/

struct fi_info *mpc_ofi_get_requested_hints(char *provider);

#endif /* MPC_OFI_HELPERS_H */
