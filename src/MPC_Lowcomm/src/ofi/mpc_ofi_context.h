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
/* # - Paul Canat <pcanat@paratools.com>                                  # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_OFI_CONTEXT
#define MPC_OFI_CONTEXT

/* Public */
#include <stdint.h>

#include <mpc_common_datastructure.h>
#include <mpc_common_spinlock.h>


/* INTERNAL */
#include "mpc_lowcomm_monitor.h"
#include "lowcomm_config.h"
#include "mpc_mempool.h"
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
}_mpc_ofi_context_policy_t;


typedef int (*_mpc_ofi_context_recv_callback_t)(void *buffer, size_t len, struct _mpc_ofi_request_t * req, void * callback_arg);
typedef int (*_mpc_ofi_context_accept_callback_t)(mpc_lowcomm_peer_uid_t uid, void * accept_cb_arg);


/* Opaque declaration of the domain */
struct _mpc_ofi_domain_t;

struct _mpc_ofi_context_t
{
   mpc_common_spinlock_t lock;

   /* Topology */
   uint16_t numa_count;
   uint16_t current_domain;
   _mpc_ofi_context_policy_t ctx_policy;

   /* Domains */
   struct _mpc_ofi_domain_t * domain;

   /* Provider Name */
   char *provider;

   /* Main Fabric */
   struct fi_info *config;
   struct fid_fabric *fabric;

   /* Main DNS */
   struct _mpc_ofi_dns_t dns;

   /* Central Callback for Recv */
   _mpc_ofi_context_recv_callback_t recv_callback;
   void * callback_arg;

   /* Callback for accepting connections (for passive endpoints only)*/
   _mpc_ofi_context_accept_callback_t accept_cb;
   void *accept_cb_arg;

   struct _mpc_lowcomm_config_struct_net_driver_ofi * rail_config;
};

int _mpc_ofi_context_init(struct _mpc_ofi_context_t *ctx,
                        char * provider,
                        _mpc_ofi_context_policy_t policy,
                        _mpc_ofi_context_recv_callback_t recv_callback,
                        void * callback_arg,
                        _mpc_ofi_context_accept_callback_t accept_cb,
                        void *accept_cb_arg,
                        struct _mpc_lowcomm_config_struct_net_driver_ofi * config);

int _mpc_ofi_context_release(struct _mpc_ofi_context_t *ctx);



#endif /* MPC_OFI_CONTEXT */
