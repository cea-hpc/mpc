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
#ifndef MPC_LOWCOMM_IB_DEVICE
#define MPC_LOWCOMM_IB_DEVICE

#include <infiniband/verbs.h>
#include <inttypes.h>

#include "mpc_common_spinlock.h"
#include "ib.h"
#include "mpc_conf.h"

struct sctk_ib_qp_s;

/** Structure related to ondemand connexions */
typedef struct sctk_ib_qp_ondemand_s
{
	struct sctk_ib_qp_s *qp_list;
	struct sctk_ib_qp_s *qp_list_ptr; /** 'Hand' of the clock */
	mpc_common_spinlock_t lock;
} sctk_ib_qp_ondemand_t;


/** Structure associated to a device */
typedef struct sctk_ib_device_s
{

	struct ibv_device **dev_list; 		/**< Devices */
	int                     dev_nb;		/**< Number of devices */
	struct ibv_device       *dev;			/**< Selected device */
	int                     dev_index;		/**< Selected device index */
	struct ibv_context      *context;  		/**< context */
	struct ibv_device_attr  dev_attr; 		/**< Attributes of the device */
	struct ibv_port_attr    port_attr;		/**< Port attributes */
	/* XXX: do not add other fields here or the code segfaults.
	* Maybe a restriction of IB in the memory alignement */

	unsigned int id;		     /**< ID of the device */
	struct ibv_pd           *pd;       /**< protection domain */
	struct ibv_srq          *srq;      /**< shared received quue */
	struct ibv_cq           *send_cq;  /**< outgoing completion queues */
	struct ibv_cq           *recv_cq;  /**< incoming completion queues */

	struct ibv_comp_channel *send_comp_channel;
	struct ibv_comp_channel *recv_comp_channel;

	struct sctk_ib_qp_ondemand_s ondemand;

	char pad1[64];
	mpc_common_spinlock_t cq_polling_lock;
	char pad2[64];

	/* Link rate & data rate*/
	char link_rate[64];
	int link_width;
	int data_rate;
	int index_pkey;

	int eager_rdma_connections; 		/**< For eager RDMA channel */
} sctk_ib_device_t;



sctk_ib_device_t *sctk_ib_device_init ( struct sctk_ib_rail_info_s *rail_ib );

sctk_ib_device_t *sctk_ib_device_open ( struct sctk_ib_rail_info_s *rail_ib, char * device_name );
void sctk_ib_device_close( struct sctk_ib_rail_info_s *);

struct ibv_pd *sctk_ib_pd_init ( sctk_ib_device_t *device );
void sctk_ib_pd_free(sctk_ib_device_t *device);

struct ibv_comp_channel *sctk_ib_comp_channel_init ( sctk_ib_device_t *device );

struct ibv_cq *sctk_ib_cq_init ( sctk_ib_device_t *device,
                                 struct _mpc_lowcomm_config_struct_net_driver_infiniband *config,
                                 struct ibv_comp_channel *comp_chan );
void sctk_ib_cq_free(struct ibv_cq*);

int sctk_ib_device_found();

#endif /* MPC_LOWCOMM_IB_DEVICE */
