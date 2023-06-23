#ifndef MPC_OFI_DNS
#define MPC_OFI_DNS

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>

#include <mpc_common_spinlock.h>

#include "rdma/fi_domain.h"

#include "mpc_ofi_helpers.h"

/*
Here we need to persist two kind of info

   RANK -> OPAQUE Ofi ADDR (central in main DNS)
   RANK -> fi_addr (per domain)

We see the address resolution as:


Per domain -> Lookup Rank
   * MISS:
      * In central:
         ->Central Lookup OPAQUE
         -> Possibly PMI / SOCKET
         -> Add to central
      * Register to local translation:
         * Add to AV
         * Store RANK -> fi_addr_t
*/
/************************************
 * HASH TABLE TO UINT64_T TO VOID * *
 ************************************/

#define MPC_OFI_DNS_DEFAULT_HT_SIZE 512

struct mpc_ofi_dns_ht_cell_t
{
   uint64_t rank;
   void * value;
   struct mpc_ofi_dns_ht_cell_t * next;
};


struct mpc_ofi_dns_ht_t
{
   mpc_common_spinlock_t lock;
   uint32_t size;
   struct mpc_ofi_dns_ht_cell_t ** heads;
};

int mpc_ofi_dns_ht_init(struct mpc_ofi_dns_ht_t * ht, uint32_t size);
int mpc_ofi_dns_ht_release(struct mpc_ofi_dns_ht_t * ht, void (*release)(void *pentry));

int mpc_ofi_dns_ht_put(struct mpc_ofi_dns_ht_t * ht, uint64_t rank, void * value);
void * mpc_ofi_dns_ht_get(struct mpc_ofi_dns_ht_t * ht, uint64_t rank, int * found);


/***************************
 * SOCKET-BASED RESOLUTION *
 ***************************/

struct mpc_ofi_dns_t;

typedef enum
{
   MPC_OFI_DNS_SOCKET_RESOLUTION_NONE = 0,
   MPC_OFI_DNS_SOCKET_RESOLUTION_GET,
   MPC_OFI_DNS_SOCKET_RESOLUTION_PUT,
   MPC_OFI_DNS_SOCKET_RESOLUTION_COUNT
}mpc_ofi_dns_socket_resolution_request_type_t;


struct mpc_ofi_dns_socket_resolution_request_t
{
   mpc_ofi_dns_socket_resolution_request_type_t type;
   char address[MPC_OFI_ADDRESS_LEN];
   size_t len;
   uint64_t rank;
};

#define MPC_OFI_DNS_PORT_LEN 16

struct mpc_ofi_dns_socket_resolution_t
{
   struct mpc_ofi_dns_t * dns;
   int is_master_process;

   /* Server side */
   pthread_t server_thread;
   volatile int is_running;
   int server_socket;

   /* Client side */
   char server_addr[MPC_OFI_ADDRESS_LEN];
   char port[MPC_OFI_DNS_PORT_LEN];
};

int mpc_ofi_dns_socket_resolution_init(struct mpc_ofi_dns_socket_resolution_t*rsvl, struct mpc_ofi_dns_t * dns);
int mpc_ofi_dns_socket_resolution_release(struct mpc_ofi_dns_socket_resolution_t*rsvl);

int mpc_ofi_dns_socket_resolution_register(void* prsvl, uint64_t rank, char *addr, size_t *len);
int mpc_ofi_dns_socket_resolution_lookup(void* prsvl, uint64_t rank, char *addr, size_t *len);


/*******************
 * THE CENTRAL DNS *
 *******************/

typedef enum
{
   MPC_OFI_DNS_RESOLUTION_SOCKET,
   MPC_OFI_DNS_RESOLUTION_CPLANE,
   MPC_OFI_DNS_RESOLUTION_COUNT
}mpc_ofi_dns_resolution_t;

struct mpc_ofi_dns_name_entry_t
{
   char * value;
   size_t len;
};

struct mpc_ofi_dns_name_entry_t * mpc_ofi_dns_name_entry(char * buff, size_t len);
void mpc_ofi_dns_name_entry_release(void *pentry);


typedef int (*mpc_ofi_dns_operation_t)(void * first_arg, uint64_t rank, char *addr, size_t *len);

struct mpc_ofi_dns_t
{
   struct mpc_ofi_dns_ht_t cache;
   /* External resolver*/
   mpc_ofi_dns_resolution_t resolution_type;
   void *operation_first_arg;
   mpc_ofi_dns_operation_t op_lookup;
   mpc_ofi_dns_operation_t op_register;
   /* MPC_OFI_RESOLUTION_SOCKET */
   struct mpc_ofi_dns_socket_resolution_t socket_rsvl;
};


int mpc_ofi_dns_init(struct mpc_ofi_dns_t * dns, mpc_ofi_dns_resolution_t resolution_type);
int mpc_ofi_dns_release(struct mpc_ofi_dns_t * dns);

int mpc_ofi_dns_resolve(struct mpc_ofi_dns_t * dns, uint64_t rank, char * outbuff, size_t *outlen);
int mpc_ofi_dns_register(struct mpc_ofi_dns_t * dns, uint64_t rank, char * buff, size_t len);

/**********************
 * THE PER DOMAIN DNS *
 **********************/

struct mpc_ofi_domain_dns_t
{
   struct mpc_ofi_dns_t * main_dns;
   /* Local fi_addr_t cache */
   struct mpc_ofi_dns_ht_t cache;
   /* The addess vector */
   struct fid_av *av;
};

int mpc_ofi_domain_dns_init(struct mpc_ofi_domain_dns_t *ddns,
                           struct mpc_ofi_dns_t * main_dns,
                           struct fid_domain *domain);

struct fid * mpc_ofi_domain_dns_av(struct mpc_ofi_domain_dns_t *ddns);

int mpc_ofi_domain_dns_resolve(struct mpc_ofi_domain_dns_t * dns, uint64_t rank, fi_addr_t *addr);

int mpc_ofi_domain_dns_release(struct mpc_ofi_domain_dns_t *ddns);



#endif /* MPC_OFI_DNS */