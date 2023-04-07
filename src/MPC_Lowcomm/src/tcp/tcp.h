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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_TCP_H_
#define __SCTK_TCP_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <mpc_common_spinlock.h>
#include <mpc_common_helper.h>
#include "lowcomm_types_internal.h"

/* \brief TCP header
 *
 */
typedef struct lcr_tcp_am_hdr {
	uint8_t am_id;
	size_t length;
} lcr_tcp_am_hdr_t;

typedef struct lcr_tcp_am_zcopy_hdr {
	lcr_tcp_am_hdr_t base;
	int iovcnt;
	struct iovec iov[0];
} lcr_tcp_am_zcopy_hdr_t;

/** \brief ROUTE level data structure for TCP
*
*   This structure is stored in each \ref _mpc_lowcomm_endpoint_s structure
*   using the \ref _mpc_lowcomm_endpoint_info_t union
*/
typedef struct
{
	mpc_common_spinlock_t lock; /**< Client socket write lock */
	int fd;               /**< Client socket */
} _mpc_lowcomm_endpoint_info_tcp_t;

/** \brief RAIL level info data structure for TCP
 *
 *  This structure is stored in each \ref sctk_rail_info_s structure
 *  using the \ref sctk_rail_info_spec_t union
 */
typedef struct
{
	char * interface;                               /**< name of the net interface to try first */
	int sockfd;                                          /**< Listening socket file descriptor */
	int portno;                                          /**< Listening socket port number */
	char connection_infos[MPC_COMMON_MAX_STRING_SIZE];              /**< Connection info for this listening socket */
	size_t connection_infos_size;                        /**< Length of the connection_info field */
	void * ( *tcp_thread_loop ) ( struct _mpc_lowcomm_endpoint_s * ); /**< Function to call when registering a route (RDMA/MULTIRAIL/TCP) */

	mpc_common_spinlock_t lock; /**< Add route lock */
	/* Config */
	int bcopy_buf_size;
	int zcopy_buf_size;

	/* Buffered protocol */
	size_t bcopy_thresh;

	/* Zero copy protocol */
	int max_iov;
} _mpc_lowcomm_tcp_rail_info_t;


void sctk_network_finalize_tcp(sctk_rail_info_t *rail);
void sctk_network_init_tcp ( sctk_rail_info_t *rail );

#ifdef __cplusplus
}
#endif
#endif
