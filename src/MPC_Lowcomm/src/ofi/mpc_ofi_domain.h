#ifndef MPC_OFI_DOMAIN
#define MPC_OFI_DOMAIN

#include "mpc_ofi_context.h"
#include "mpc_ofi_request.h"

#include <mpc_common_spinlock.h>

#include "lowcomm_config.h"

#include <rdma/fi_domain.h>
#include <rdma/fi_eq.h>
#include <stdint.h>
#include <sys/types.h>

/*********************
 * THE OFI CL DOMAIN *
 *********************/

#define MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL 4


struct mpc_ofi_domain_t;

struct mpc_ofi_domain_buffer_t
{
   mpc_common_spinlock_t lock;
   uint32_t pending_operations;
   volatile int is_posted;
   bool has_multi_recv;

   mpc_ofi_aligned_mem_t aligned_mem;

   void * buffer;
   size_t size;

   struct fid_mr * mr;
   struct fid_ep * endpoint;
   struct fi_context context;
};

int mpc_ofi_domain_buffer_init(struct mpc_ofi_domain_buffer_t * buff,
                               struct mpc_ofi_domain_t * domain,
                               struct fid_ep * endpoint,
                               unsigned int eager_size,
                               unsigned int eager_per_buff,
                               bool has_multi_recv);
void mpc_ofi_domain_buffer_acquire(struct mpc_ofi_domain_buffer_t * buff);
int mpc_ofi_domain_buffer_relax(struct mpc_ofi_domain_buffer_t * buff);
int mpc_ofi_domain_buffer_post(struct mpc_ofi_domain_buffer_t * buff);
void mpc_ofi_domain_buffer_set_unposted(struct mpc_ofi_domain_buffer_t * buff);



int mpc_ofi_domain_buffer_release(struct mpc_ofi_domain_buffer_t * buff);

struct mpc_ofi_domain_buffer_manager_t
{
   unsigned int buffer_count;
   unsigned int eager_size;
   unsigned int eager_per_buff;
   struct mpc_ofi_domain_buffer_t *rx_buffers;

   struct mpc_ofi_domain_t * domain;

   volatile unsigned int pending_repost_count;
   mpc_common_spinlock_t pending_repost_count_lock;

};

int mpc_ofi_domain_buffer_manager_init(struct mpc_ofi_domain_buffer_manager_t * buffs,
                                       struct mpc_ofi_domain_t * domain,
                                       struct fid_ep * endpoint,
                                       unsigned int eager_size,
                                       unsigned int eager_per_buff,
                                       unsigned int num_recv_buff,
                                       bool has_multi_recv);

int mpc_ofi_domain_buffer_manager_release(struct mpc_ofi_domain_buffer_manager_t * buffs);


/*********************
 * THE ENDPOINT LIST *
 *********************/





/**************
 * THE DOMAIN *
 **************/

struct mpc_ofi_domain_t{
   mpc_common_spinlock_t lock;
   volatile int being_polled;
   /* Pointer to config and fabric */
   struct mpc_ofi_context_t *ctx;
   /* The OFI domain */
   struct fid_domain *domain;
   /* Event queue */
   struct fid_eq * eq;
   /* Completion queues */
   struct fid_cq * rx_cq;
   struct fid_cq * tx_cq;
   /* Local DNS */
   struct mpc_ofi_domain_dns_t ddns;

   /* Is multirecv supported ?*/
   bool has_multi_recv;

   /* Is it a passive endpoint mode*/
   bool is_passive_endpoint;

   /* Connectionless Endpoint */
   struct fid_ep * ep;

   /* Passive endpoint */
   struct fid_pep * pep;


   /* Address for this EP */
   char address[MPC_OFI_ADDRESS_LEN];
   size_t address_len;
   /* Buffers */
   struct mpc_ofi_domain_buffer_manager_t buffers;
   /* Requests */
   struct mpc_ofi_request_cache_t rcache;
   struct _mpc_lowcomm_config_struct_net_driver_ofi * config;
};

int mpc_ofi_domain_init(struct mpc_ofi_domain_t * domain, struct mpc_ofi_context_t *ctx,    struct _mpc_lowcomm_config_struct_net_driver_ofi * config);
int mpc_ofi_domain_release(struct mpc_ofi_domain_t * domain);
int mpc_ofi_domain_poll(struct mpc_ofi_domain_t * domain, int type);

int mpc_ofi_domain_memory_register(struct mpc_ofi_domain_t * domain,
                                  void *buff,
                                  size_t size,
                                  uint64_t acs,
                                  struct fid_mr **mr);

int mpc_ofi_domain_memory_unregister(struct mpc_ofi_domain_t * domain, struct fid_mr *mr);


int mpc_ofi_domain_send(struct mpc_ofi_domain_t * domain,
                       uint64_t dest,
                       void *buff,
                       size_t size,
                       struct mpc_ofi_request_t **req,
                       int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                       void *arg_ext);

int mpc_ofi_domain_sendv(struct mpc_ofi_domain_t * domain,
                        uint64_t dest,
                        const struct iovec *iov,
                        size_t iovcnt,
                        struct mpc_ofi_request_t **req,
                        int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                        void *arg_ext);

int mpc_ofi_domain_get(struct mpc_ofi_domain_t *domain,
                       void *buf,
                       size_t len,
                       uint64_t dest,
                       uint64_t remote_addr,
                       uint64_t key,
                       struct mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                       void *arg_ext);

int mpc_ofi_domain_put(struct mpc_ofi_domain_t *domain,
                       void *buf,
                       size_t len,
                       uint64_t dest,
                       uint64_t remote_addr,
                       uint64_t key,
                       struct mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                       void *arg_ext);

struct fid_ep * mpc_ofi_domain_connect(struct mpc_ofi_domain_t *domain, void *addr);

struct fid_ep * mpc_ofi_domain_accept(struct mpc_ofi_domain_t *domain, void *addr);


#endif /* MPC_OFI_DOMAIN */