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
#include <stdint.h>
#include <mpcmp.h>

  struct sctk_ibuf_pool_s;
  struct sctk_ibuf_s;
  struct sctk_ib_mmu_s;
  struct sctk_ib_config_s;
  struct sctk_ib_device_s;
  struct sctk_ib_qp_s;
  struct sctk_ib_cp_s;
  struct sctk_thread_ptp_message_s;
  struct sctk_rail_info_s;
  struct sctk_message_to_copy_s;
  struct sctk_ib_buffered_entry_s;

  typedef struct sctk_ib_rail_info_s {
    struct sctk_ibuf_pool_s *pool_buffers;
    struct sctk_ib_mmu_s    *mmu;
    struct sctk_ib_config_s *config;
    struct sctk_ib_device_s *device;
    struct sctk_ib_prof_s   *profiler;
    /* Collaborative polling */
    struct sctk_ib_cp_s *cp;
    /* HashTable where all QPs are stored */
    struct sctk_ib_qp_s *qps;
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

  /* 2 first bits: not_set, zerocopy, recopy */
  /* third bit: if matched */
  /* forth bit: if done  */
  /* free from 1<<4 */
  typedef volatile enum sctk_ib_rdma_status_e {
    not_set = 1,
    zerocopy = 2,
    recopy = 3,
    match = 4,
    done = 8,
    allocating = 1<<4,
  } sctk_ib_rdma_status_t;
#define MASK_BASE   0x03
#define MASK_MATCH  0x04
#define MASK_DONE   0x08

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
        struct sctk_reorder_table_s* reorder_table;
        struct sctk_ib_buffered_entry_s* entry;
        struct sctk_rail_info_s* rail;
        int ready;
      } buffered;
      struct {
        size_t requested_size;
        sctk_spinlock_t lock;
        struct sctk_rail_info_s *rail;
        struct sctk_route_table_s* route_table;
        struct sctk_message_to_copy_s *copy_ptr;
        /* For collaborative polling: src and dest of msg */
        int glob_source;
        int glob_destination;
        /* Local structure */
        struct {
          sctk_ib_rdma_status_t status;
          struct sctk_ib_mmu_entry_s *mmu_entry;
          void  *addr;
          size_t size;
          void  *aligned_addr;
          size_t aligned_size;
          /* Local structure ready to be read */
          volatile int ready;
        } local;
        /* Remote structure */
        struct {
          /* msg_header of the remote peer */
          struct sctk_thread_ptp_message_s *msg_header;
          void  *addr;
          size_t size;
          uint32_t rkey;
          void  *aligned_addr;
          size_t aligned_size;
        } remote;
      } rdma;
    };
  } sctk_ib_msg_header_t;

  void sctk_network_init_ib_all(struct sctk_rail_info_s* rail,
			       int (*route)(int , struct sctk_rail_info_s* ),
			       void(*route_init)(struct sctk_rail_info_s*));

  /* XXX: Should not be declared here but in CM */
  struct sctk_route_table_s *
    sctk_ib_create_remote(int dest, struct sctk_rail_info_s* rail);
  void sctk_ib_add_static_route(int dest, struct sctk_route_table_s *tmp, struct sctk_rail_info_s* rail);
  void sctk_ib_add_dynamic_route(int dest, struct sctk_route_table_s *tmp, struct sctk_rail_info_s* rail);
  int sctk_ib_route_dynamic_is_connected(struct sctk_route_table_s *tmp);
  void sctk_ib_route_dynamic_set_connected(struct sctk_route_table_s *tmp, int connected);

  /* IB header: generic */
#define IBUF_GET_EAGER_HEADER(buffer) \
  (sctk_ib_eager_t*) ((char*) buffer + sizeof(sctk_ibuf_header_t))
#define IBUF_GET_BUFFERED_HEADER(buffer) \
  (sctk_ib_buffered_t*) ((char*) buffer + sizeof(sctk_ibuf_header_t))
#define IBUF_GET_RDMA_HEADER(buffer) \
  (sctk_ib_rdma_t*) ((char*) buffer + sizeof(sctk_ibuf_header_t))

/* IB payload: generic */
#define IBUF_GET_EAGER_PAYLOAD(buffer) \
  (void*) ((char*) IBUF_GET_EAGER_HEADER(buffer) + sizeof(sctk_ib_eager_t))
#define IBUF_GET_BUFFERED_PAYLOAD(buffer) \
  (void*) ((char*) IBUF_GET_BUFFERED_HEADER(buffer) + sizeof(sctk_ib_buffered_t))
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
#define IBUF_GET_RDMA_DONE(buffer) ((sctk_ib_rdma_done_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_DATA_READ(buffer) ((sctk_ib_rdma_data_read_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_DATA_WRITE(buffer) ((sctk_ib_rdma_data_write_t*) IBUF_GET_RDMA_PAYLOAD(buffer))

/* Size */
#define IBUF_GET_EAGER_SIZE (IBUF_GET_HEADER_SIZE + sizeof(sctk_ib_eager_t))

#define IBUF_GET_BUFFERED_SIZE (IBUF_GET_HEADER_SIZE + sizeof(sctk_ib_buffered_t))

#define IBUF_GET_RDMA_SIZE (IBUF_GET_HEADER_SIZE + sizeof(sctk_ib_rdma_t))
#define IBUF_GET_RDMA_REQ_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_req_t))
#define IBUF_GET_RDMA_ACK_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_ack_t))
#define IBUF_GET_RDMA_DONE_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_done_t))
#define IBUF_GET_RDMA_DATA_READ_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_data_read_t))
#define IBUF_GET_RDMA_DATA_WRITE_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_data_write_t))

  void sctk_network_free_msg(struct sctk_thread_ptp_message_s *msg);
  /* For getting stats from network usage */
  void sctk_network_stats_ib (struct MPC_Network_stats_s* stats);

/* For not used fuctions (disable compiler warning */
#ifdef __GNUC__
#define __UNUSED__ __attribute__ ((__unused__))
#else
#define __UNUSED__
#endif

#ifdef __cplusplus
}
#endif
#endif
