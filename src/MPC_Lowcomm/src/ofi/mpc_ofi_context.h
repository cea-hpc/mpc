#ifndef MPC_OFI_CONTEXT
#define MPC_OFI_CONTEXT

/* Public */
#include <stdint.h>

#include <mpc_common_spinlock.h>

/* INTERNAL */
#include "mpc_ofi_dns.h"
#include "mpc_ofi_request.h"

/* OFI */
#include <rdma/fabric.h>


typedef enum
{
   MPC_OFI_POLICY_RR,
   MPC_OFI_POLICY_NUMA_RR,
   MPC_OFI_POLICY_RANDOM,
   MPC_OFI_POLICY_COUNT
}mpc_ofi_context_policy_t;


typedef int (*mpc_ofi_context_recv_callback_t)(void *buffer, size_t len, struct mpc_ofi_request_t * req);


/* Opaque declaration of the domain */
struct mpc_ofi_domain_t;

struct mpc_ofi_context_t
{
   mpc_common_spinlock_t lock;

   /* Topology */
   uint16_t numa_count;
   uint16_t current_domain;
   mpc_ofi_context_policy_t ctx_policy;

   /* Domains */
   uint32_t domain_count;
   struct mpc_ofi_domain_t * domains;

   /* Provider Name */
   char *provider;

   /* Main Fabric */
   struct fi_info *config;
   struct fid_fabric *fabric;

   /* Main DNS */
   struct mpc_ofi_dns_t dns;

   /* Central Callback for Recv */
   mpc_ofi_context_recv_callback_t recv_callback;
};

int mpc_ofi_context_init(struct mpc_ofi_context_t *ctx,
                        uint32_t domain_count,
                        char * provider,
                        mpc_ofi_context_policy_t policy,
                        mpc_ofi_context_recv_callback_t recv_callback);

int mpc_ofi_context_release(struct mpc_ofi_context_t *ctx);


/**************************
 * THE OFI CL VIEW *
 **************************/

struct mpc_ofi_view_t
{
   struct mpc_ofi_domain_t * domain;
   uint64_t rank;
};

int mpc_ofi_view_init(struct mpc_ofi_view_t *view, struct mpc_ofi_context_t *ctx, uint64_t rank);
int mpc_ofi_view_release(struct mpc_ofi_view_t *view);


int mpc_ofi_view_send(struct mpc_ofi_view_t *view, void * buffer, size_t size, uint64_t dest, struct mpc_ofi_request_t **req);

int mpc_ofi_view_test(struct mpc_ofi_view_t *view,  struct mpc_ofi_request_t *req, int *done);

int mpc_ofi_view_wait(struct mpc_ofi_view_t *view,  struct mpc_ofi_request_t *req);


int mpc_ofi_view_wait_for_rank_registration(struct mpc_ofi_view_t *view, uint64_t rank, uint32_t timeout_sec);


#endif /* MPC_OFI_CONTEXT */