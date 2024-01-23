/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:44:33 CEST 2023                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_OFI_DOMAIN
#define MPC_OFI_DOMAIN

#include "mpc_ofi_context.h"
#include "mpc_ofi_request.h"

#include <mpc_common_spinlock.h>
#include <mpc_lowcomm_monitor.h>

#include "lowcomm_config.h"

#include <rdma/fi_domain.h>
#include <rdma/fi_eq.h>
#include <stdint.h>
#include <sys/types.h>

/*********************
* THE OFI CL DOMAIN *
*********************/

#define MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL    4


struct _mpc_ofi_domain_t;

struct _mpc_ofi_domain_buffer_t
{
	mpc_common_spinlock_t lock;
	uint32_t              pending_operations;
	volatile int          is_posted;
	bool                  has_multi_recv;

	_mpc_ofi_aligned_mem_t aligned_mem;

	void *                buffer;
	size_t                size;

	struct fid_mr *       mr;
	struct fid_ep *       endpoint;
	struct fi_context     context;
};

int _mpc_ofi_domain_buffer_init(struct _mpc_ofi_domain_buffer_t *buff,
                               struct _mpc_ofi_domain_t *domain,
                               struct fid_ep *endpoint,
                               unsigned int eager_size,
                               unsigned int eager_per_buff,
                               bool has_multi_recv);
void _mpc_ofi_domain_buffer_acquire(struct _mpc_ofi_domain_buffer_t *buff);
int _mpc_ofi_domain_buffer_relax(struct _mpc_ofi_domain_buffer_t *buff);
int _mpc_ofi_domain_buffer_post(struct _mpc_ofi_domain_buffer_t *buff);
void _mpc_ofi_domain_buffer_set_unposted(struct _mpc_ofi_domain_buffer_t *buff);



int _mpc_ofi_domain_buffer_release(struct _mpc_ofi_domain_buffer_t *buff);

struct _mpc_ofi_domain_buffer_manager_t
{
	unsigned int                    buffer_count;
	unsigned int                    eager_size;
	unsigned int                    eager_per_buff;
	struct _mpc_ofi_domain_buffer_t *rx_buffers;

	struct _mpc_ofi_domain_t *       domain;

	volatile unsigned int           pending_repost_count;
	mpc_common_spinlock_t           pending_repost_count_lock;
};

int _mpc_ofi_domain_buffer_manager_init(struct _mpc_ofi_domain_buffer_manager_t *buffs,
                                       struct _mpc_ofi_domain_t *domain,
                                       struct fid_ep *endpoint,
                                       unsigned int eager_size,
                                       unsigned int eager_per_buff,
                                       unsigned int num_recv_buff,
                                       bool has_multi_recv);

int _mpc_ofi_domain_buffer_manager_release(struct _mpc_ofi_domain_buffer_manager_t *buffs);


/*********************
* THE BUFFER   LIST *
*********************/

struct _mpc_ofi_domain_t;

struct _mpc_ofi_domain_buffer_entry_t
{
	struct _mpc_ofi_domain_buffer_manager_t buff;
	struct _mpc_ofi_domain_buffer_entry_t * next;
};

int _mpc_ofi_domain_buffer_entry_add(struct _mpc_ofi_domain_t *domain, struct fid_ep *endpoint);
int _mpc_ofi_domain_buffer_entry_release(struct _mpc_ofi_domain_t *domain);

/*************************
* CONNECTION MANAGEMENT *
*************************/
typedef enum
{
	MPC_OFI_DOMAIN_ENDPOINT_NO_STATE,
	MPC_OFI_DOMAIN_ENDPOINT_BOOTSTRAP,
	MPC_OFI_DOMAIN_ENDPOINT_CONNECTING,
	MPC_OFI_DOMAIN_ENDPOINT_ACCEPTING,
	MPC_OFI_DOMAIN_ENDPOINT_CONNECTED
}_mpc_ofi_domain_conn_state_t;

struct _mpc_ofi_domain_conn
{
	mpc_common_spinlock_t                   lock;
	uint64_t                                key;
	mpc_lowcomm_peer_uid_t                  remote_uid;
	struct fid_ep *                         endpoint;
	_mpc_ofi_domain_conn_state_t state;

	struct _mpc_ofi_domain_conn *next;
};

struct _mpc_ofi_domain_conn *_mpc_ofi_domain_conn_new(uint64_t key, mpc_lowcomm_peer_uid_t remote_uid, _mpc_ofi_domain_conn_state_t state);
int _mpc_ofi_domain_conn_release(struct _mpc_ofi_domain_conn *state);

_mpc_ofi_domain_conn_state_t _mpc_ofi_domain_conn_get(struct _mpc_ofi_domain_conn * cstate);
int _mpc_ofi_domain_conn_set(struct _mpc_ofi_domain_conn *cstate, _mpc_ofi_domain_conn_state_t state);

struct fid_ep * _mpc_ofi_domain_conn_endpoint_get(struct _mpc_ofi_domain_conn * cstate);
int _mpc_ofi_domain_conn_endpoint_set(struct _mpc_ofi_domain_conn * cstate, struct fid_ep * ep);
int _mpc_ofi_domain_conn_key_set(struct _mpc_ofi_domain_conn * cstate, uint64_t key);
struct _mpc_ofi_domain_conntrack
{
	struct _mpc_ofi_domain_conn *connections;
	mpc_common_spinlock_t                   lock;
};

int _mpc_ofi_domain_conntrack_init(struct _mpc_ofi_domain_conntrack *tracker);
int _mpc_ofi_domain_conntrack_release(struct _mpc_ofi_domain_conntrack *tracker);

int _mpc_ofi_domain_conntrack_pop_endpoint(struct _mpc_ofi_domain_conntrack * tracker, struct fid_ep * ep);

struct _mpc_ofi_domain_conn *_mpc_ofi_domain_conntrack_add(struct _mpc_ofi_domain_conntrack *tracker,
                                                                              uint64_t key,
                                                                              mpc_lowcomm_peer_uid_t remote_uid,
                                                                              _mpc_ofi_domain_conn_state_t state,
                                                                              int *is_first);

struct _mpc_ofi_domain_conn * _mpc_ofi_domain_conntrack_get_by_endpoint(struct _mpc_ofi_domain_conntrack * tracker, struct fid_ep * ep);
struct _mpc_ofi_domain_conn *_mpc_ofi_domain_conntrack_get_by_remote(struct _mpc_ofi_domain_conntrack *tracker,
                                                                                        mpc_lowcomm_peer_uid_t remote_uid);



/**************
* THE DOMAIN *
**************/

struct _mpc_ofi_domain_t
{
	mpc_common_spinlock_t                             lock;
	/* Pointer to config and fabric */
	struct _mpc_ofi_context_t *                        ctx;
	/* The OFI domain */
	struct fid_domain *                               domain;
	/* Event queue */
	struct fid_eq *                                   eq;
	/* Completion queues */
	struct fid_cq *                                   rx_cq;
	struct fid_cq *                                   tx_cq;
	/* Local DNS */
	struct _mpc_ofi_domain_dns_t                       ddns;

	/* State tracking for ongoing connections */
	struct _mpc_ofi_domain_conntrack          conntrack;

	/* Is multirecv supported ?*/
	bool                                              has_multi_recv;

	/* Is it a passive endpoint mode*/
	bool                                              is_passive_endpoint;

	/* Connectionless Endpoint */
	struct fid_ep *                                   ep;

	/* Passive endpoint */
	struct fid_pep *                                  pep;


	/* Address for this EP */
	char                                              address[MPC_OFI_ADDRESS_LEN];
	size_t                                            address_len;
	/* Buffers */
	mpc_common_spinlock_t                             buffer_lock;
	struct _mpc_ofi_domain_buffer_entry_t *            buffers;
	/* Requests */
	struct _mpc_ofi_request_cache_t                    rcache;
	struct _mpc_lowcomm_config_struct_net_driver_ofi *config;
};

int _mpc_ofi_domain_init(struct _mpc_ofi_domain_t *domain, struct _mpc_ofi_context_t *ctx, struct _mpc_lowcomm_config_struct_net_driver_ofi *config);
int _mpc_ofi_domain_release(struct _mpc_ofi_domain_t *domain);
int _mpc_ofi_domain_poll(struct _mpc_ofi_domain_t *domain, int type);

int _mpc_ofi_domain_memory_register(struct _mpc_ofi_domain_t *domain,
                                   void *buff,
                                   size_t size,
                                   uint64_t acs,
                                   struct fid_mr **mr);

int _mpc_ofi_domain_memory_unregister(struct _mpc_ofi_domain_t *domain, struct fid_mr *mr);


int _mpc_ofi_domain_send(struct _mpc_ofi_domain_t *domain,
                        uint64_t dest,
                        void *buff,
                        size_t size,
                        struct _mpc_ofi_request_t **req,
                        int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
                        void *arg_ext);

int _mpc_ofi_domain_sendv(struct _mpc_ofi_domain_t *domain,
                         uint64_t dest,
                         const struct iovec *iov,
                         size_t iovcnt,
                         struct _mpc_ofi_request_t **req,
                         int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
                         void *arg_ext);

int _mpc_ofi_domain_get(struct _mpc_ofi_domain_t *domain,
                       void *buf,
                       size_t len,
                       uint64_t dest,
                       uint64_t remote_addr,
                       uint64_t key,
                       struct _mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
                       void *arg_ext);

int _mpc_ofi_domain_put(struct _mpc_ofi_domain_t *domain,
                       void *buf,
                       size_t len,
                       uint64_t dest,
                       uint64_t remote_addr,
                       uint64_t key,
                       struct _mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
                       void *arg_ext);

struct fid_ep *_mpc_ofi_domain_connect(struct _mpc_ofi_domain_t *domain, mpc_lowcomm_peer_uid_t uid, void *addr, size_t addrlen);



#endif /* MPC_OFI_DOMAIN */
