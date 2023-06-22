#ifndef MPC_OFI_HELPERS_H
#define MPC_OFI_HELPERS_H

#include <stdint.h>
#include <stdio.h>

#include <rdma/fabric.h>
#include <rdma/fi_errno.h>

/***********
 * DEFINES *
 ***********/

#define MPC_OFI_ADDRESS_LEN 512


/*************
 * PRINTINGS *
 *************/

int mpc_ofi_decode_cq_flags(uint64_t flags);
int mpc_ofi_decode_mr_mode(uint64_t flags);

/************
 * RETCODES *
 ************/


#define MPC_OFI_CHECK_RET(a)    do{                                                                                      \
		int __ret = (a);                                                                                 \
		if(__ret < 0){                                                                                   \
			(void)fprintf(stderr, "[MPC_OFI]@%s:%d: %s\n%s(%d)\n", __FILE__, __LINE__, #a, fi_strerror(-__ret), __ret); \
			return -FI_EOTHER;                                                                                \
		}                                                                                                \
}while(0)

#endif /* MPC_OFI_HELPERS_H */