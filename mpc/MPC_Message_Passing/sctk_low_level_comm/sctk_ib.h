/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_IB_H_
#define __SCTK_IB_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "sctk_spinlock.h"

  struct sctk_ibuf_pool_s;
  struct sctk_ibuf_s;
  struct sctk_ib_mmu_s;
  struct sctk_ib_config_s;
  struct sctk_ib_device_s;
  struct sctk_ib_qp_s;
  struct sctk_thread_ptp_message_s;
  struct sctk_rail_info_s;

  typedef struct sctk_ib_rail_info_s {
    struct sctk_ibuf_pool_s *pool_buffers;
    struct sctk_ib_mmu_s    *mmu;
    struct sctk_ib_config_s *config;
    struct sctk_ib_device_s *device;
  } sctk_ib_rail_info_t;

  typedef struct sctk_ib_data_s {
    struct sctk_ib_qp_s* remote;
  } sctk_ib_data_t;

  /* ib protocol used */
  typedef enum sctk_ib_protocol_e{
    eager_protocol        = 111,
    buffered_protocol     = 222,
    rdma_protocol   = 333,
  } sctk_ib_protocol_t;

  typedef enum  sctk_ib_rdma_status_e {
    not_set = 111,
    zerocopy = 222,
    recopy = 333,
  } sctk_ib_rdma_status_t;

  /* Structure included in msg header */
  typedef struct sctk_ib_msg_header_s{
    sctk_ib_protocol_t protocol;

    union
    {
      struct {
        /* If msg has been recopied (do not read msg
         * from network buffer */
        int recopied;
        /* Ibuf to release */
        struct sctk_ibuf_s *ibuf;
      } eager;
      struct {
        sctk_ib_rdma_status_t status;
        void *addr_remote;
        void *aligned_addr_remote;
        size_t size_remote;
        size_t aligned_size_remote;
        sctk_spinlock_t lock;
      } rdma;
    };
  } sctk_ib_msg_header_t;

  void sctk_network_init_ib_all(struct sctk_rail_info_s* rail,
			       int (*route)(int , struct sctk_rail_info_s* ),
			       void(*route_init)(struct sctk_rail_info_s*));

/* IB header: generic */
#define IBUF_GET_EAGER_HEADER(buffer) \
  (sctk_ib_eager_t*) ((char*) buffer + sizeof(sctk_ibuf_header_t))
#define IBUF_GET_RDMA_HEADER(buffer) \
  (sctk_ib_rdma_t*) ((char*) buffer + sizeof(sctk_ibuf_header_t))

/* IB payload: generic */
#define IBUF_GET_EAGER_PAYLOAD(buffer) \
  (void*) ((char*) IBUF_GET_EAGER_HEADER(buffer) + sizeof(sctk_ib_eager_t))
#define IBUF_GET_RDMA_PAYLOAD(buffer) \
  (void*) ((char*) IBUF_GET_RDMA_HEADER(buffer) + sizeof(sctk_ib_rdma_t))

/* MPC header */
#define IBUF_GET_EAGER_MSG_HEADER(buffer) \
  (sctk_thread_ptp_message_body_t*) IBUF_GET_EAGER_PAYLOAD(buffer)

/* MPC payload */
#define IBUF_GET_EAGER_MSG_PAYLOAD(buffer) \
  (void*) ((char*) IBUF_GET_EAGER_PAYLOAD(buffer) + sizeof(sctk_thread_ptp_message_body_t))
#define IBUF_GET_RDMA_MSG_PAYLOAD(buffer) \
  (void*) ((char*) IBUF_GET_RDMA_PAYLOAD(buffer) + sizeof(sctk_thread_ptp_message_body_t))

  /* Different types of messages */
#define IBUF_GET_RDMA_REQ(buffer) ((sctk_ib_rdma_req_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_ACK(buffer) ((sctk_ib_rdma_ack_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_DONE(buffer) ((sctk_ib_rdma_ack_t*) IBUF_GET_RDMA_PAYLOAD(buffer))

/* Size */
#define IBUF_GET_EAGER_SIZE (sizeof(IBUF_GET_HEADER_SIZE) + sizeof(sctk_ib_eager_t))
#define IBUF_GET_RDMA_SIZE (sizeof(IBUF_GET_HEADER_SIZE) + sizeof(sctk_ib_rdma_t))
#define IBUF_GET_RDMA_REQ_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_req_t))

#define IBUF

  void sctk_network_free_msg(struct sctk_thread_ptp_message_s *msg);
#ifdef __cplusplus
}
#endif
#endif
