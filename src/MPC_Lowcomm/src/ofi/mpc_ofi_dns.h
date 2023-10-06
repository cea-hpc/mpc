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

struct _mpc_ofi_dns_name_entry_t
{
   char * value;
   struct fid_ep * endpoint;
   size_t len;
};

struct _mpc_ofi_dns_name_entry_t * _mpc_ofi_dns_name_entry(char * buff, size_t len, struct fid_ep * endpoint);
void _mpc_ofi_dns_name_entry_release(void *pentry);


typedef struct fid_ep * (*_mpc_ofi_dns_operation_t)(void * first_arg, uint64_t rank, char *addr, size_t *len);

struct _mpc_ofi_dns_t
{
   struct mpc_common_hashtable cache;
};


int _mpc_ofi_dns_init(struct _mpc_ofi_dns_t * dns);
int _mpc_ofi_dns_release(struct _mpc_ofi_dns_t * dns);

struct fid_ep * _mpc_ofi_dns_resolve(struct _mpc_ofi_dns_t * dns, uint64_t rank, char * outbuff, size_t *outlen, int * found);
int _mpc_ofi_dns_register(struct _mpc_ofi_dns_t * dns, uint64_t rank, char * buff, size_t len, struct fid_ep * endpoint);
void _mpc_ofi_dns_dump_addr(char * context, char * buff, size_t len);
int _mpc_ofi_dns_set_endpoint(struct _mpc_ofi_dns_t * dns, uint64_t rank, struct fid_ep * endpoint);

/**********************
 * THE PER DOMAIN DNS *
 **********************/

struct _mpc_ofi_domain_dns_t
{
   struct _mpc_ofi_dns_t * main_dns;
   /* Local fi_addr_t cache */
   struct mpc_common_hashtable cache;
   /* The addess vector */
   struct fid_av *av;
   /* Is the DNS in passive endpoint mode */
   bool is_passive;
};

int _mpc_ofi_domain_dns_init(struct _mpc_ofi_domain_dns_t *ddns,
                           struct _mpc_ofi_dns_t * main_dns,
                           struct fid_domain *domain,
                           bool is_passive);

struct fid * _mpc_ofi_domain_dns_av(struct _mpc_ofi_domain_dns_t *ddns);

struct fid_ep * _mpc_ofi_domain_dns_resolve(struct _mpc_ofi_domain_dns_t * ddns, uint64_t rank, fi_addr_t *addr);

int _mpc_ofi_domain_dns_release(struct _mpc_ofi_domain_dns_t *ddns);



#endif /* MPC_OFI_DNS */
