#ifndef MPC_OFI_DNS
#define MPC_OFI_DNS

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include <mpc_common_datastructure.h>
#include <mpc_common_spinlock.h>

#include "mpc_ofi_helpers.h"

#include "rdma/fi_endpoint.h"

/*
Here we need to persist two kind of info

   RANK -> OPAQUE Ofi ADDR (central in main DNS)
   RANK -> fi_addr (per domain)ep
 * HASH TABLE TO UINT64_T TO VOID * *
 ************************************/

#define MPC_OFI_DNS_DEFAULT_HT_SIZE 512

/*******************
 * THE CENTRAL DNS *
 *******************/

struct mpc_ofi_dns_name_entry_t
{
   char * value;
   struct fid_ep * endpoint;
   size_t len;
};

struct mpc_ofi_dns_name_entry_t * mpc_ofi_dns_name_entry(char * buff, size_t len, struct fid_ep * endpoint);
void mpc_ofi_dns_name_entry_release(void *pentry);


typedef struct fid_ep * (*mpc_ofi_dns_operation_t)(void * first_arg, uint64_t rank, char *addr, size_t *len);

struct mpc_ofi_dns_t
{
   struct mpc_common_hashtable cache;
};


int mpc_ofi_dns_init(struct mpc_ofi_dns_t * dns);
int mpc_ofi_dns_release(struct mpc_ofi_dns_t * dns);

struct fid_ep * mpc_ofi_dns_resolve(struct mpc_ofi_dns_t * dns, uint64_t rank, char * outbuff, size_t *outlen, int * found);
int mpc_ofi_dns_register(struct mpc_ofi_dns_t * dns, uint64_t rank, char * buff, size_t len, struct fid_ep * endpoint);

/**********************
 * THE PER DOMAIN DNS *
 **********************/

struct mpc_ofi_domain_dns_t
{
   struct mpc_ofi_dns_t * main_dns;
   /* Local fi_addr_t cache */
   struct mpc_common_hashtable cache;
   /* The addess vector */
   struct fid_av *av;
   /* Is the DNS in passive endpoint mode */
   bool is_passive;
};

int mpc_ofi_domain_dns_init(struct mpc_ofi_domain_dns_t *ddns,
                           struct mpc_ofi_dns_t * main_dns,
                           struct fid_domain *domain,
                           bool is_passive);

struct fid * mpc_ofi_domain_dns_av(struct mpc_ofi_domain_dns_t *ddns);

struct fid_ep * mpc_ofi_domain_dns_resolve(struct mpc_ofi_domain_dns_t * ddns, uint64_t rank, fi_addr_t *addr);

int mpc_ofi_domain_dns_release(struct mpc_ofi_domain_dns_t *ddns);



#endif /* MPC_OFI_DNS */