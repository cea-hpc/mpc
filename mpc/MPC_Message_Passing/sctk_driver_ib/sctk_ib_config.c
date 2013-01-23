/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND

#include "sctk_ib_config.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_NAME "CONFIG"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_runtime_config.h"

/*-----------------------------------------------------------
 *  CONSTS
 *----------------------------------------------------------*/
/* For RDMA connection */
/* FOR PAPER */
#define IBV_RDMA_MIN_SIZE (2 * 1024)
#define IBV_RDMA_MAX_SIZE (12 * 1024)
#define IBV_RDMA_MIN_NB (16)
#define IBV_RDMA_MAX_NB (1024)

/* For RDMA resizing */
/* FOR PAPER */
#define IBV_RDMA_RESIZING_MIN_SIZE (2 * 1024)
#define IBV_RDMA_RESIZING_MAX_SIZE (12 * 1024)
#define IBV_RDMA_RESIZING_MIN_NB (16)
#define IBV_RDMA_RESIZING_MAX_NB (1024)

char* steal_names[2] = {
  "Normal mode",
  "Collaborative-polling mode"};

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_config_check(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);

  /* TODO: MMU cache is buggy and do not intercept free calls */
  if ( (sctk_process_rank == 0)
      && (config->ibv_mmu_cache_enabled == 1) ) {
    sctk_error("MMU cache enabled: use it at your own risk!");
  }

  if ( (sctk_process_rank == 0)
      && (config->ibv_low_memory == 1) ) {
    sctk_error("LOW mem module enabled: use it at your own risk!");
  }

  /* Good conf, we return */
  return;
}

#define EXPERIMENTAL(str) #str" (experimental)"
void sctk_ib_config_print(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  if (sctk_process_rank == 0) {
    fprintf(stderr,
        "############# IB configuration for %s\n"
        "ibv_eager_limit      = %d\n"
        "ibv_buffered_limit = %d\n"
        "ibv_max_rdma_connections = %d\n"
        "ibv_qp_tx_depth      = %d\n"
        "ibv_qp_rx_depth      = %d\n"
        "ibv_max_sg_sq        = %d\n"
        "ibv_max_sg_rq        = %d\n"
        "ibv_max_inline       = %d\n"
        "ibv_init_ibufs        = %d\n"
        "ibv_max_srq_ibufs_posted    = %d\n"
        "ibv_max_srq_ibufs    = %d\n"
        "ibv_size_ibufs_chunk = %d\n"
        "ibv_srq_credit_limit = %d\n"
        "ibv_srq_credit_thread_limit = %d\n"
        "ibv_rdvz_protocol    = %d\n"
        "ibv_wc_in_number     = %d\n"
        "ibv_wc_out_number    = %d\n"
        "ibv_init_mr          = %d\n"
        "ibv_mmu_cache_enabled = %d\n"
        "ibv_mmu_cache_entries = %d\n"
        "ibv_adm_port         = %d\n"
        "ibv_rdma_depth       = %d\n"
        "ibv_rdma_dest_depth  = %d\n"
        "ibv_adaptive_polling = %d\n"
        "ibv_quiet_crash      = %d\n"
        EXPERIMENTAL(ibv_steal)"            = %d\n"
        "Stealing desc        = %s\n"
        EXPERIMENTAL(ibv_low_memory)"            = %d\n"
        "#############\n",
        config->network_name,
        config->ibv_eager_limit,
        config->ibv_buffered_limit,
        config->ibv_max_rdma_connections,
        config->ibv_qp_tx_depth,
        config->ibv_qp_rx_depth,
        config->ibv_max_sg_sq,
        config->ibv_max_sg_rq,
        config->ibv_max_inline,
        config->ibv_init_ibufs,
        config->ibv_max_srq_ibufs_posted,
        config->ibv_max_srq_ibufs,
        config->ibv_size_ibufs_chunk,
        config->ibv_srq_credit_limit,
        config->ibv_srq_credit_thread_limit,
        config->ibv_rdvz_protocol,
        config->ibv_wc_in_number,
        config->ibv_wc_out_number,
        config->ibv_init_mr,
        config->ibv_mmu_cache_enabled,
        config->ibv_mmu_cache_entries,
        config->ibv_adm_port,
        config->ibv_rdma_depth,
        config->ibv_rdma_dest_depth,
        config->ibv_adaptive_polling,
        config->ibv_quiet_crash,
        config->ibv_steal, steal_names[config->ibv_steal],
        config->ibv_low_memory);
  }
}

#define SET_RUNTIME_CONFIG(name) config->ibv_##name = runtime_config->name

static void load_ib_load_config(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  struct sctk_runtime_config_struct_net_driver_infiniband * runtime_config =
    &rail_ib->rail->runtime_config_driver_config->driver.value.infiniband;

  SET_RUNTIME_CONFIG(size_mr_chunk);
  SET_RUNTIME_CONFIG(init_ibufs);
  SET_RUNTIME_CONFIG(max_rdma_connections);
  SET_RUNTIME_CONFIG(qp_tx_depth);
  SET_RUNTIME_CONFIG(qp_rx_depth);
  SET_RUNTIME_CONFIG(cq_depth);
  SET_RUNTIME_CONFIG(max_sg_sq);
  SET_RUNTIME_CONFIG(max_sg_rq);
  SET_RUNTIME_CONFIG(max_inline);
  SET_RUNTIME_CONFIG(max_srq_ibufs_posted);
  SET_RUNTIME_CONFIG(max_srq_ibufs);
  SET_RUNTIME_CONFIG(srq_credit_limit);
  SET_RUNTIME_CONFIG(srq_credit_thread_limit);
  SET_RUNTIME_CONFIG(verbose_level);
  SET_RUNTIME_CONFIG(wc_in_number);
  SET_RUNTIME_CONFIG(wc_out_number);
  SET_RUNTIME_CONFIG(init_mr);
  SET_RUNTIME_CONFIG(size_ibufs_chunk);
  SET_RUNTIME_CONFIG(mmu_cache_enabled);
  SET_RUNTIME_CONFIG(mmu_cache_entries);
  SET_RUNTIME_CONFIG(adm_port);
  SET_RUNTIME_CONFIG(rdma_depth);
  SET_RUNTIME_CONFIG(rdma_dest_depth);
  SET_RUNTIME_CONFIG(steal);
  SET_RUNTIME_CONFIG(quiet_crash);

  config->ibv_eager_limit       = ALIGN_ON (runtime_config->eager_limit + IBUF_GET_EAGER_SIZE, 64);
  config->ibv_buffered_limit  = (runtime_config->buffered_limit + sizeof(sctk_thread_ptp_message_body_t));
  config->ibv_rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
}

#if 0
/* Set IB configure with env variables */
void set_ib_env(sctk_ib_rail_info_t *rail_ib)
{
  /* Format: "x:x:x:x-x:x:x:x". Buffer sizes are in KB */
  if ( (value = getenv("MPC_IBV_RDMA_EAGER")) != NULL) {
    sscanf(value,"%d-(%d:%d:%d:%d)-%d-(%d:%d:%d:%d)",
        &c->ibv_max_rdma_connections,
        &c->ibv_rdma_min_size,
        &c->ibv_rdma_max_size,
        &c->ibv_rdma_min_nb,
        &c->ibv_rdma_max_nb,
        &c->ibv_rdma_resizing,
        &c->ibv_rdma_resizing_min_size,
        &c->ibv_rdma_resizing_max_size,
        &c->ibv_rdma_resizing_min_nb,
        &c->ibv_rdma_resizing_max_nb);

    c->ibv_rdma_min_size          *=1024;
    c->ibv_rdma_max_size          *=1024;
    c->ibv_rdma_resizing_min_size *=1024;
    c->ibv_rdma_resizing_max_size *=1024;
  }
}
#endif

void sctk_ib_config_init(sctk_ib_rail_info_t *rail_ib, char* network_name)
{
  LOAD_CONFIG(rail_ib);

  config = sctk_malloc(sizeof(sctk_ib_config_t));
  assume(config);
  memset(config, 0, sizeof(sctk_ib_config_t));
  rail_ib->config = config;
  rail_ib->config->network_name = strdup(network_name);

  load_ib_load_config(rail_ib);

  /*
   * Check if the variables are well set
   * */
  sctk_ib_config_check(rail_ib);
}

#endif
