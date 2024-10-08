/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:44:31 CEST 2023                                        # */
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
#include "mpc_ofi_dns.h"

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <mpc_common_spinlock.h>
#include <stdio.h>
#include <string.h>


#include "mpc_common_datastructure.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_ofi_helpers.h"
#include "rdma/fabric.h"

#define MPC_MODULE "Lowcomm/Ofi/Dns"

//#define DEBUG_DNS_ADDR

#define SMALL_BUFF 8

static char * __dump_addr(char * buff, int outlen, char *p, size_t len)
{
	size_t i = 0;

   buff[0] = '\0';

	for(i = 0; i < len; i++)
	{
      char tmp[SMALL_BUFF];
		(void)snprintf(tmp, SMALL_BUFF, "%x", p[i]);
      strncat(buff, tmp, outlen - 1);
	}

   return buff;
}

#define LARGE_BUFF 512

void _mpc_ofi_dns_dump_addr(char * context, char * buff, size_t len)
{
   char outbuff[LARGE_BUFF];
   mpc_common_debug_error("[OFI DNS] %s OFI ADDR len %d : %s", context, len, __dump_addr(outbuff, LARGE_BUFF, buff, len));
}


/*******************
 * THE CENTRAL DNS *
 *******************/

int _mpc_ofi_dns_init(struct _mpc_ofi_dns_t * dns)
{
   mpc_common_hashtable_init(&dns->cache, MPC_OFI_DNS_DEFAULT_HT_SIZE);

   return 0;
}
int _mpc_ofi_dns_release(struct _mpc_ofi_dns_t * dns)
{
   struct _mpc_ofi_dns_name_entry_t * tmp = NULL;

   MPC_HT_ITER(&dns->cache, tmp )
   {
      _mpc_ofi_dns_name_entry_release(tmp);
   }
   MPC_HT_ITER_END(&dns->cache)

   mpc_common_hashtable_release(&dns->cache);

   return 0;
}


struct _mpc_ofi_dns_name_entry_t * _mpc_ofi_dns_name_entry(char * buff, size_t len, struct fid_ep * endpoint)
{
   struct _mpc_ofi_dns_name_entry_t *entry = malloc(sizeof(struct _mpc_ofi_dns_name_entry_t));

   if(!entry)
   {
      perror("malloc");
      return NULL;
   }

   entry->value = malloc(len * sizeof(char));

   if(!entry->value)
   {
      free(entry);
      perror("malloc");
      return NULL;
   }

   memcpy(entry->value, buff, len);
   entry->len = len;
   entry->endpoint = endpoint;

   return entry;
}

void _mpc_ofi_dns_name_entry_release(void *pentry)
{
   struct _mpc_ofi_dns_name_entry_t * entry = (struct _mpc_ofi_dns_name_entry_t *)pentry;

   free(entry->value);
   free(entry);
}



struct fid_ep * _mpc_ofi_dns_resolve(struct _mpc_ofi_dns_t * dns, uint64_t rank, char * outbuff, size_t *outlen, int * found)
{
   /* First look in local cache */
   struct _mpc_ofi_dns_name_entry_t *entry = (struct _mpc_ofi_dns_name_entry_t *)mpc_common_hashtable_get(&dns->cache, rank);

   if(entry)
   {
      /* Cache HIT */
      if(*outlen < entry->len)
      {
         mpc_common_errorpoint("Address truncated");
         return NULL;
      }

      assume(entry->len < *outlen);

      memcpy(outbuff, entry->value, entry->len);
      *outlen = entry->len;

      #ifdef DEBUG_DNS_ADDR
         char buff[512];
         mpc_common_debug_error("[OFI DNS]  Resolved address for %s len: %ld @ %s in %p", mpc_lowcomm_peer_format(rank), *outlen, __dump_addr(buff, 512, outbuff, *outlen), outbuff);
      #endif

      *found = 1;

      return entry->endpoint;
   }

   *found = 0;

#ifdef DEBUG_DNS_ADDR
      mpc_common_debug_warning("[OFI DNS]  Failed to resolve %s", mpc_lowcomm_peer_format(rank));
#endif

   return NULL;
}

int _mpc_ofi_dns_set_endpoint(struct _mpc_ofi_dns_t * dns, uint64_t rank, struct fid_ep * endpoint)
{
   struct _mpc_ofi_dns_name_entry_t *entry = (struct _mpc_ofi_dns_name_entry_t *)mpc_common_hashtable_get(&dns->cache, rank);

   assume(entry != NULL);
   assume(entry->endpoint == NULL);

   entry->endpoint = endpoint;

   return 0;
}


int _mpc_ofi_dns_register(struct _mpc_ofi_dns_t * dns, uint64_t rank, char * buff, size_t len, struct fid_ep * endpoint)
{
   /* Copy address out of the input buff for persistence */
   struct _mpc_ofi_dns_name_entry_t *entry = _mpc_ofi_dns_name_entry(buff, len, endpoint);

#ifdef DEBUG_DNS_ADDR
   char outbuff[512];
   mpc_common_debug_error("[OFI DNS] New address for %s len: %ld @ %s", mpc_lowcomm_peer_format(rank), len, __dump_addr(outbuff, 512, buff, len));
#endif

   /* Add to cache */
   mpc_common_hashtable_set(&dns->cache, rank, (void *)entry);

   return 0;
}


/**********************
 * THE PER DOMAIN DNS *
 **********************/



int _mpc_ofi_domain_dns_init(struct _mpc_ofi_domain_dns_t *ddns,
                           struct _mpc_ofi_dns_t * main_dns,
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


struct fid * _mpc_ofi_domain_dns_av(struct _mpc_ofi_domain_dns_t *ddns)
{
   if(ddns->is_passive)
   {
      mpc_common_debug_fatal("No address vector in pasive endpoint mode");
   }
   return &ddns->av->fid;
}

int _mpc_ofi_domain_dns_release(struct _mpc_ofi_domain_dns_t *ddns)
{
   if(!ddns->is_passive)
   {
      MPC_OFI_CHECK_RET(fi_close(&ddns->av->fid));
   }

   struct _mpc_ofi_dns_name_entry_t * tmp = NULL;

   MPC_HT_ITER(&ddns->cache, tmp )
   {
      if(tmp->endpoint && ddns->is_passive)
      {
         TODO("Understand why we get -EBUSY");
         fi_close(&tmp->endpoint->fid);
      }
      _mpc_ofi_dns_name_entry_release(tmp);
   }
   MPC_HT_ITER_END(&ddns->cache)

   mpc_common_hashtable_release(&ddns->cache);
   return 0;
}

struct fid_ep * _mpc_ofi_domain_dns_resolve(struct _mpc_ofi_domain_dns_t * ddns, uint64_t rank, fi_addr_t *addr)
{

   struct _mpc_ofi_dns_name_entry_t * entry = mpc_common_hashtable_get(&ddns->cache, rank);

   if(!entry)
   {
      char _buff[MPC_OFI_ADDRESS_LEN];
      char *buff = _buff;
      size_t len = MPC_OFI_ADDRESS_LEN;

      int addr_found = 0;

      /* We need to insert it */
      struct fid_ep * ep = _mpc_ofi_dns_resolve(ddns->main_dns, rank, buff, &len, &addr_found);

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

      struct _mpc_ofi_dns_name_entry_t * new_entry = _mpc_ofi_dns_name_entry(buff, len, ep);

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
