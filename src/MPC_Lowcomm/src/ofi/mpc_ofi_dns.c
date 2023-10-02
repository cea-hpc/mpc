#include "mpc_ofi_dns.h"

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <mpc_common_spinlock.h>

#include "mpc_common_datastructure.h"
#include "mpc_ofi_helpers.h"
#include "rdma/fabric.h"

//#define DEBUG_DNS_ADDR

#ifdef DEBUG_DNS_ADDR
static void __dump_addr(char *p, size_t len)
{
	size_t i = 0;

	for(i = 0; i < len; i++)
	{
		printf("%x", p[i]);
	}
	printf("\n");
}
#endif

/*******************
 * THE CENTRAL DNS *
 *******************/

int mpc_ofi_dns_init(struct mpc_ofi_dns_t * dns)
{
   mpc_common_hashtable_init(&dns->cache, MPC_OFI_DNS_DEFAULT_HT_SIZE);

   return 0;
}
int mpc_ofi_dns_release(struct mpc_ofi_dns_t * dns)
{
   struct mpc_ofi_dns_name_entry_t * tmp = NULL;

   MPC_HT_ITER(&dns->cache, tmp )
   {
      mpc_ofi_dns_name_entry_release(tmp);
   }
   MPC_HT_ITER_END(&dns->cache)

   mpc_common_hashtable_release(&dns->cache);

   return 0;
}


struct mpc_ofi_dns_name_entry_t * mpc_ofi_dns_name_entry(char * buff, size_t len, struct fid_ep * endpoint)
{
   struct mpc_ofi_dns_name_entry_t *entry = malloc(sizeof(struct mpc_ofi_dns_name_entry_t));

   if(!entry)
   {
      perror("malloc");
      return NULL;
   }

   entry->value = malloc(len * sizeof(char));

   if(!entry->value)
   {
      perror("malloc");
      return NULL;
   }

   memcpy(entry->value, buff, len);
   entry->len = len;
   entry->endpoint = endpoint;

   return entry;
}

void mpc_ofi_dns_name_entry_release(void *pentry)
{
   struct mpc_ofi_dns_name_entry_t * entry = (struct mpc_ofi_dns_name_entry_t *)pentry;

   free(entry->value);
   free(entry);
}



struct fid_ep * mpc_ofi_dns_resolve(struct mpc_ofi_dns_t * dns, uint64_t rank, char * outbuff, size_t *outlen, int * found)
{
   /* First look in local cache */
   struct mpc_ofi_dns_name_entry_t *entry = (struct mpc_ofi_dns_name_entry_t *)mpc_common_hashtable_get(&dns->cache, rank);

   if(entry)
   {
      /* Cache HIT */
      if(*outlen < entry->len)
      {
         mpc_common_errorpoint("Address truncated");
         return NULL;
      }

      memcpy(outbuff, entry->value, entry->len);
      *outlen = entry->len;


      #ifdef DEBUG_DNS_ADDR
         (void)fprintf(stderr, "Resovled address for %lu @", rank);
         __dump_addr(outbuff, *outlen);
      #endif

      *found = 1;

      return entry->endpoint;
   }

   *found = 0;

#ifdef DEBUG_DNS_ADDR
      mpc_common_debug_warning("Failed to resolve %lu", rank);
#endif

   return NULL;
}

int mpc_ofi_dns_register(struct mpc_ofi_dns_t * dns, uint64_t rank, char * buff, size_t len, struct fid_ep * endpoint)
{
   /* Copy address out of the input buff for persistence */
   struct mpc_ofi_dns_name_entry_t *entry = mpc_ofi_dns_name_entry(buff, len, endpoint);

#ifdef DEBUG_DNS_ADDR
   (void)fprintf(stderr, "New address for %lu @", rank);
   __dump_addr(buff, len);
#endif

   /* Add to cache */
   mpc_common_hashtable_set(&dns->cache, rank, (void *)entry);

   return 0;
}


/**********************
 * THE PER DOMAIN DNS *
 **********************/



int mpc_ofi_domain_dns_init(struct mpc_ofi_domain_dns_t *ddns,
                           struct mpc_ofi_dns_t * main_dns,
                           struct fid_domain *domain,
                           bool is_passive)
{
   ddns->main_dns = main_dns;

   /* Create hashtable */
   mpc_common_hashtable_init(&ddns->cache, MPC_OFI_DNS_DEFAULT_HT_SIZE);

   ddns->is_passive = is_passive;

   if(!is_passive)
   {
      /* Init the AV */
      struct fi_av_attr av_attr = {0};
      av_attr.type = FI_AV_MAP;
      MPC_OFI_CHECK_RET(fi_av_open(domain, &av_attr, &ddns->av, NULL));
   }
   else
   {
      ddns->av = NULL;
   }

   return 0;
}


struct fid * mpc_ofi_domain_dns_av(struct mpc_ofi_domain_dns_t *ddns)
{
   if(ddns->is_passive)
   {
      mpc_common_debug_fatal("No address vector in pasive endpoint mode");
   }
   return &ddns->av->fid;
}

int mpc_ofi_domain_dns_release(struct mpc_ofi_domain_dns_t *ddns)
{
   if(!ddns->is_passive)
   {
      MPC_OFI_CHECK_RET(fi_close(&ddns->av->fid));
   }

   struct mpc_ofi_dns_name_entry_t * tmp = NULL;

   MPC_HT_ITER(&ddns->cache, tmp )
   {
      if(tmp->endpoint && ddns->is_passive)
      {
         TODO("Understand why we get -EBUSY");
         fi_close(&tmp->endpoint->fid);
      }
      mpc_ofi_dns_name_entry_release(tmp);
   }
   MPC_HT_ITER_END(&ddns->cache)

   mpc_common_hashtable_release(&ddns->cache);
   return 0;
}

struct fid_ep * mpc_ofi_domain_dns_resolve(struct mpc_ofi_domain_dns_t * ddns, uint64_t rank, fi_addr_t *addr)
{

   struct mpc_ofi_dns_name_entry_t * entry = mpc_common_hashtable_get(&ddns->cache, rank);

   if(!entry)
   {
      char _buff[MPC_OFI_ADDRESS_LEN];
      char *buff = _buff;
      size_t len = MPC_OFI_ADDRESS_LEN;

      int addr_found = 0;

      /* We need to insert it */
      struct fid_ep * ep = mpc_ofi_dns_resolve(ddns->main_dns, rank, buff, &len, &addr_found);

      /* We need to make sure the endpoint exists prior to trying to sending messages on it
         it means the central DNS MUST know this endpoint */
      assume(addr_found == 1);

      if(!ddns->is_passive)
      {
         /* Now insert in AV */
         MPC_OFI_DUMP_ERROR(fi_av_insert(ddns->av, buff, 1, addr, 0, NULL));
         buff = (char*)addr;
         len = sizeof(fi_addr_t);
      }

      struct mpc_ofi_dns_name_entry_t * new_entry = mpc_ofi_dns_name_entry(buff, len, ep);

      /* and now save in HT for next lookup */
      mpc_common_hashtable_set(&ddns->cache, rank, new_entry);

      return ep;
   }

   if(!ddns->is_passive)
   {
      assume(entry->len == sizeof(fi_addr_t));
      *addr = *((fi_addr_t*)entry->value);
   }
   else
   {
      *addr = 0;
   }

   return entry->endpoint;

}