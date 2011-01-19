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
/* #   - PERACHE Marc     marc.perache@cea.fr                             # */
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                 # */
/* #                                                                      # */
/* ######################################################################## */


#ifndef __SCTK__SHM_CONSTS_H_
#define __SCTK__SHM_CONSTS_H_

/* size of the shared memory */
#define SCTK_SHM_LEN (400 * 1024 * 1024)

/* version of the module. Just for fun */
#define SHM_VERSION "1.0"

/* we consider that there is no more processes than this value */
#define SCTK_SHM_MAX_NB_LOCAL_PROCESSES 32
#define SCTK_SHM_MAX_NB_TOTAL_PROCESSES SCTK_MAX_NODES * SCTK_SHM_MAX_NB_LOCAL_PROCESSES
/* max number of nodes */
#define SCTK_MAX_NODES 50

/* max slots allowed in a queue. If more slots are
 * registered in a queue, the program aborts*/
#define SCTK_SHM_MAX_SLOTS_FASTMSG_QUEUE  512
#define SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE   2048

/* max size of a slot.
 * Beware that a msg doesn't excess this value */
#define SCTK_SHM_BIGMSG_DATA_MAXLEN         ( 12 * 1024)
#define SCTK_SHM_BIGMSG_RPC_MAXLEN          ( 4 * 1024)
#define SCTK_SHM_BIGMSG_REDUCE_MAXLEN       ( 4 * 1024)
#define SCTK_SHM_BIGMSG_BROADCAST_MAXLEN    ( 4 * 1024)

#define SCTK_SHM_FASTMSG_DATA_MAXLEN        ( 4 * 1024)
#define SCTK_SHM_FASTMSG_BROADCAST_MAXLEN   ( 4 * 1024)

#define SCTK_SHM_BARRIER_MAXLEN         1024
#define SCTK_SHM_RPC_MAXLEN             1024
#define SCTK_SHM_FASTMSG_RDMA_MAXLEN    1024
#define SCTK_SHM_BIGMSG_RDMA_MAXLEN     1024
#define SCTK_SHM_FASTMSG_REDUCE_MAXLEN  1024

/* size of queues */
#define SCTK_SHM_BIGMSG_QUEUE_LEN            1024
#define SCTK_SHM_BIGMSG_BROADCAST_QUEUE_LEN  64
#define SCTK_SHM_BIGMSG_REDUCE_QUEUE_LEN     64
#define SCTK_SHM_BIGMSG_RPC_QUEUE_LEN        4

#define SCTK_SHM_FASTMSG_QUEUE_LEN           128
#define SCTK_SHM_FASTMSG_BROADCAST_QUEUE_LEN 128

#define SCTK_SHM_RPC_QUEUE_LEN      16
#define SCTK_SHM_BARRIER_QUEUE_LEN  16
#define SCTK_SHM_RDMA_QUEUE_LEN     16

/* max value of functions args */
#define SCTK_SHM_RPC_ARGS_MAXLEN   128
/* max number of pointers to functions */
#define SCTK_SHM_RPC_MAX_FUNCTIONS 64
/* max of communicators */
#define SCTK_SHM_MAX_COMMUNICATORS 10
/* padding */
#define SCTK_SHM_PADDING 64

/* queues types */
/* don't change the order. Types 0 to 2 are used
 * in queues creations. */
#define SCTK_SHM_DATA_FAST      ((int)0)
#define SCTK_SHM_RPC_FAST       ((int)1)
#define SCTK_SHM_RDMA_FAST      ((int)2)
#define SCTK_SHM_DATA_BIG       ((int)3)
#define SCTK_SHM_RPC_BIG        ((int)4)
#define SCTK_SHM_RDMA_BIG       ((int)5)
#define SCTK_SHM_REDUCE         ((int)6)
#define SCTK_SHM_BROADCAST      ((int)7)

/* macros which define the type of msg received  */
#define SCTK_SHM_SLOT_TYPE_COLLECTIVE_BROADCAST 1
#define SCTK_SHM_SLOT_TYPE_PTP_REDUCE           2
#define SCTK_SHM_SLOT_TYPE_PTP_MESSAGE          4
#define SCTK_SHM_SLOT_TYPE_PTP_RPC              8
#define SCTK_SHM_SLOT_TYPE_PTP_BARRIER          16
#define SCTK_SHM_SLOT_TYPE_PTP_RDMA_READ        32
#define SCTK_SHM_SLOT_TYPE_PTP_RDMA_WRITE       64

#define SCTK_SHM_SLOT_TYPE_FASTMSG              128
#define SCTK_SHM_SLOT_TYPE_BIGMSG               256

/* Type of communication */
#define SCTK_SHM_INTRA_NODE_COMM ((int)0) /* in node */
#define SCTK_SHM_INTER_NODE_COMM ((int)1) /* outside */
#define SCTK_SHM_ERR             ((int)-1)

#endif //__SCTK__SHM_CONSTS_H
