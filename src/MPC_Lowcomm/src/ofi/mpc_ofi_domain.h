#ifndef MPC_OFI_DOMAIN
#define MPC_OFI_DOMAIN

#include "mpc_ofi_context.h"
#include "mpc_ofi_request.h"

#include <mpc_common_spinlock.h>

#include <rdma/fi_domain.h>
#include <rdma/fi_eq.h>
#include <stdint.h>
#include <sys/types.h>

/*********************
 * THE OFI CL DOMAIN *
 *********************/

/* TODO: move to config */
#define MPC_OFI_DOMAIN_EAGER_SIZE (size_t)8192
#define MPC_OFI_DOMAIN_EAGER_PER_BUFF 512
#define MPC_OFI_NUM_MULTIRECV_BUFF 8

#define MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL 8


struct mpc_ofi_domain_t;

struct mpc_ofi_domain_buffer_t
{
   mpc_common_spinlock_t lock;
   uint32_t pending_operations;
   volatile int is_posted;

   void * buffer;
   size_t size;

   struct fid_mr * mr;
   struct fi_context context;

   struct mpc_ofi_domain_t * domain;
};

int mpc_ofi_domain_buffer_init(struct mpc_ofi_domain_buffer_t * buff, struct mpc_ofi_domain_t * domain);
void mpc_ofi_domain_buffer_acquire(struct mpc_ofi_domain_buffer_t * buff);
int mpc_ofi_domain_buffer_relax(struct mpc_ofi_domain_buffer_t * buff);
int mpc_ofi_domain_buffer_post(struct mpc_ofi_domain_buffer_t * buff);
void mpc_ofi_domain_buffer_set_unposted(struct mpc_ofi_domain_buffer_t * buff);



int mpc_ofi_domain_buffer_release(struct mpc_ofi_domain_buffer_t * buff);

struct mpc_ofi_domain_buffer_manager_t
{
   struct mpc_ofi_domain_buffer_t rx_buffers[MPC_OFI_NUM_MULTIRECV_BUFF];

   struct mpc_ofi_domain_t * domain;

   volatile int pending_repost_count;
   mpc_common_spinlock_t pending_repost_count_lock;

};

int mpc_ofi_domain_buffer_manager_init( struct mpc_ofi_domain_buffer_manager_t * buffs, struct mpc_ofi_domain_t * domain);

int mpc_ofi_domain_buffer_manager_release(struct mpc_ofi_domain_buffer_manager_t * buffs);




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
   /* Endpoint */
   struct fid_ep * ep;
   /* Address for this EP */
   char address[MPC_OFI_ADDRESS_LEN];
   size_t address_len;
   /* Buffers */
   struct mpc_ofi_domain_buffer_manager_t buffers;
   /* Requests */
   struct mpc_ofi_request_cache_t rcache;
};

int mpc_ofi_domain_init(struct mpc_ofi_domain_t * domain, struct mpc_ofi_context_t *ctx);
int mpc_ofi_domain_release(struct mpc_ofi_domain_t * domain);
int mpc_ofi_domain_poll(struct mpc_ofi_domain_t * domain, int type);

int mpc_ofi_domain_memory_register(struct mpc_ofi_domain_t * domain,
                                  void *buff,
                                  size_t size,
                                  uint64_t acs,
                                  struct fid_mr **mr);

int mpc_ofi_domain_memory_unregister(struct fid_mr *mr);


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


#endif /* MPC_OFI_DOMAIN */