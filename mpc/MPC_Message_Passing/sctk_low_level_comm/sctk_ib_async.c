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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND

#include "sctk_ib_config.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "ASYNC"
#include "sctk_ib_toolkit.h"

#include "sctk_ib.h"
#include "sctk_ib_async.h"
#include "sctk_ib_qp.h"

/*-----------------------------------------------------------
 *  CONSTS
 *----------------------------------------------------------*/
#define DESC_EVENT(config, event, desc, level, fatal)  do { \
  if ( (level <= (config)->ibv_verbose_level) || fatal) \
    sctk_ib_debug(event":\t"desc); \
  if (fatal) sctk_abort(); \
  } while(0)

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void* async_thread(void* arg)
{
  sctk_ib_rail_info_t *rail_ib = (sctk_ib_rail_info_t*) arg;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);
  struct ibv_async_event event;
  struct ibv_srq_attr mod_attr;
  int rc;

  sctk_ib_debug("Async thread running on context %p", device->context);
  while(1) {
    if(ibv_get_async_event((struct ibv_context*) device->context, &event))
    {
      sctk_error("[async thread] cannot get event");
    }

    switch (event.event_type)
    {
      case IBV_EVENT_CQ_ERR:
        DESC_EVENT(config, "IBV_EVENT_CQ_ERR", "CQ is in error (CQ overrun)", 1, 1);
        break;

      case IBV_EVENT_QP_FATAL:
        DESC_EVENT(config, "IBV_EVENT_QP_FATAL", "Error occurred on a QP and it transitioned to error state", 1, 1);
        break;

      case	IBV_EVENT_QP_REQ_ERR:
        DESC_EVENT(config, "IBV_EVENT_QP_REQ_ERR", "Invalid Request Local Work Queue Error", 1, 1);
        break;

      case	IBV_EVENT_QP_ACCESS_ERR:
        DESC_EVENT(config, "IBV_EVENT_QP_ACCESS_ERR", "Local access violation error", 1, 1);
        break;

      case IBV_EVENT_COMM_EST:
        DESC_EVENT(config, "IBV_EVENT_COMM_EST", "Communication was established on a QP", 2, 0);
        break;

      case IBV_EVENT_SQ_DRAINED:
        DESC_EVENT(config, "IBV_EVENT_SQ_DRAINED", "Send Queue was drained of outstanding messages in progress", 1, 0);
        break;

      case IBV_EVENT_PATH_MIG:
        DESC_EVENT(config, "IBV_EVENT_PATH_MIG", "A connection failed to migrate to the alternate path", 1, 0);
        break;

      case IBV_EVENT_PATH_MIG_ERR:
        DESC_EVENT(config,"IBV_EVENT_PATH_MIG_ERR", "A connection failed to migrate to the alternate path", 1, 0);
        break;

      case	IBV_EVENT_DEVICE_FATAL:
        DESC_EVENT(config, "IBV_EVENT_DEVICE_FATAL", "Error occurred on a QP and it transitioned to error state", 1, 1);
        break;

      case IBV_EVENT_PORT_ACTIVE:
        DESC_EVENT(config, "IBV_EVENT_PORT_ACTIVE", "Link became active on a port", 1, 0);
        break;

      case IBV_EVENT_PORT_ERR:
        DESC_EVENT(config, "IBV_EVENT_PORT_ERR", "Link became unavailable on a port ", 1, 1);
        break;

      case	IBV_EVENT_LID_CHANGE:
        DESC_EVENT(config, "IBV_EVENT_LID_CHANGE", "LID was changed on a port", 1, 0);
        break;

      case IBV_EVENT_PKEY_CHANGE:
        DESC_EVENT(config, "IBV_EVENT_PKEY_CHANGE", "P_Key table was changed on a port", 1, 0);
        break;

      case IBV_EVENT_SM_CHANGE:
        DESC_EVENT(config, "IBV_EVENT_SM_CHANGE", "SM was changed on a port", 1, 0);
        break;

      case IBV_EVENT_SRQ_ERR:
        DESC_EVENT(config, "IBV_EVENT_SRQ_ERR", "Error occurred on an SRQ", 1, 1);
        break;

        /* event triggered when the limit given by ibv_srq_credit_thread_limit is reached */
      case IBV_EVENT_SRQ_LIMIT_REACHED:
        DESC_EVENT(config, "IBV_EVENT_SRQ_LIMIT_REACHED","SRQ limit was reached", 2, 0);
        not_implemented();
        /* We re-arm the limit for the SRQ. */
        mod_attr.srq_limit  = config->ibv_srq_credit_thread_limit;
        mod_attr.max_wr     = config->ibv_max_srq_ibufs_posted;
        rc = ibv_modify_srq(device->srq, &mod_attr, IBV_SRQ_LIMIT);
        assume(rc == 0);
        break;

      case IBV_EVENT_QP_LAST_WQE_REACHED:
        DESC_EVENT(config, "IBV_EVENT_QP_LAST_WQE_REACHED","Last WQE Reached on a QP associated with an SRQ CQ events", 1, 1);
        break;

      case IBV_EVENT_CLIENT_REREGISTER:
        DESC_EVENT(config, "IBV_EVENT_CLIENT_REREGISTER","SM sent a CLIENT_REREGISTER request to a port CA events", 1, 0);
        break;

      default:
        DESC_EVENT(config, "UNKNOWN_EVENT","unknown event received", 1, 0);
        break;
    }

    ibv_ack_async_event(&event);
  }
  sctk_nodebug("Async thread exits...");
  return NULL;
}


void sctk_ib_async_init(sctk_ib_rail_info_t *rail_ib)
{
  sctk_thread_attr_t attr;
  sctk_thread_t pidt;

  sctk_thread_attr_init (&attr);
  /* The thread *MUST* be in a system scope */
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, async_thread, rail_ib);
}

#endif
