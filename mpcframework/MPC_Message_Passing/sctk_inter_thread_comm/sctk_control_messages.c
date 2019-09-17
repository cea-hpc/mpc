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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_control_messages.h"

#include <stdio.h>

#include <sctk_low_level_comm.h>
#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <sctk_asm.h>
#include <sctk_checksum.h>
#include <sctk_reorder.h>
#include <sctk_rail.h>
#include <sctk_alloc.h>
#include <sctk_window.h>
#include "sctk_topological_polling.h"


/************************************************************************/
/* Control Messages Context                                             */
/************************************************************************/

static struct sctk_control_message_context __ctrl_msg_context = { NULL };

static inline struct sctk_control_message_context * sctk_control_message_ctx()
{
	return &__ctrl_msg_context;
}

void sctk_control_message_context_set_user( void (*fn)( int , int , char , char , void *, size_t ) )
{
	sctk_control_message_ctx()->sctk_user_control_message = fn;
}


/************************************************************************/
/* Process Level Messages                                               */
/************************************************************************/

#ifdef MPC_MPI
/* MPI Level win handler */
void mpc_MPI_Win_control_message_handler(void *data, size_t size);

#else
/* Dummy substitute implemenentation */
void mpc_MPI_Win_control_message_handler(void *data, size_t size) {}
#endif

void sctk_control_message_task_level(__UNUSED__ int source_process, __UNUSED__ int source_rank,
                                     __UNUSED__ char subtype, __UNUSED__ char param, __UNUSED__ void *data,
                                     __UNUSED__ size_t size) {
  switch (subtype) {

  // case SCTK_PROCESS_RDMA_CONTROL_MESSAGE:
  // mpc_MPI_Win_control_message_handler(data, size);
  // break;
  default:
    not_implemented();
  }
}

void sctk_control_message_process_level(__UNUSED__ int source_process, __UNUSED__ int source_rank,
                                        char subtype, __UNUSED__ char param, void *data,
                                        __UNUSED__ size_t size) {
  struct sctk_window_map_request *mr = NULL;
  struct sctk_window_emulated_RDMA *erma = NULL;
  __UNUSED__ struct sctk_control_message_fence_ctx *fence = NULL;
  struct sctk_window_emulated_fetch_and_op_RDMA *fop = NULL;
  struct sctk_window_emulated_CAS_RDMA *fcas = NULL;
  int win_id = -1;

  switch (subtype) {
  case SCTK_PROCESS_FENCE:
    //fence = (struct sctk_control_message_fence_ctx *)data;
    // sctk_control_message_fence_handler(fence);
    break;

  case SCTK_PROCESS_RDMA_WIN_MAPTO:
    sctk_nodebug("Received a MAP remote from %d", source_rank);
    mr = (struct sctk_window_map_request *)data;
    sctk_window_map_remote_ctrl_msg_handler(mr);
    break;

  case SCTK_PROCESS_RDMA_WIN_RELAX:
    memcpy(&win_id, data, sizeof(int));
    sctk_nodebug("Received a  WIN relax from %d on %d", source_rank, win_id);
    sctk_window_relax_ctrl_msg_handler(win_id);
    break;

  case SCTK_PROCESS_RDMA_EMULATED_WRITE:
    erma = (struct sctk_window_emulated_RDMA *)data;
    sctk_window_RDMA_emulated_write_ctrl_msg_handler(erma);
    break;

  case SCTK_PROCESS_RDMA_EMULATED_READ:
    erma = (struct sctk_window_emulated_RDMA *)data;
    sctk_window_RDMA_emulated_read_ctrl_msg_handler(erma);
    break;

  case SCTK_PROCESS_RDMA_EMULATED_FETCH_AND_OP:
    fop = (struct sctk_window_emulated_fetch_and_op_RDMA *)data;
    sctk_window_RDMA_fetch_and_op_ctrl_msg_handler(fop);
    break;

  case SCTK_PROCESS_RDMA_EMULATED_CAS:
    fcas = (struct sctk_window_emulated_CAS_RDMA *)data;
    sctk_window_RDMA_CAS_ctrl_msg_handler(fcas);
    break;
  default:
    not_implemented();
  }
}

/************************************************************************/
/* Control Messages Send Recv                                           */
/************************************************************************/

static void sctk_free_control_messages (__UNUSED__  void *ptr )
{

}


void printpayload( void * pl , size_t size )
{
	size_t i;

	sctk_info("======== %ld ========", size);
	for( i = 0 ; i < size; i++ )
	{
		sctk_info("%d = [%hu]  ",i,  ((char *)pl)[i] );
	}
	sctk_info("===================");

}

void __sctk_control_messages_send(int dest, int dest_task,
                                  sctk_message_class_t message_class,
                                  sctk_communicator_t comm, int subtype,
                                  int param, void *buffer, size_t size,
                                  int rail_id) {
  sctk_communicator_t communicator = comm;
  sctk_communicator_t tag = 16000;

  sctk_request_t request;

  sctk_thread_ptp_message_t msg;

  /* Add a dummy payload */
  static int dummy_data = 42;

  if (!buffer && !size) {
    buffer = &dummy_data;
    size = sizeof(int);
  }

  if (!sctk_message_class_is_control_message(message_class)) {
    sctk_fatal("Cannot send a non-contol message using this function");
  }

  int source = -1;

  if (dest < 0) {
    int cw_rank = sctk_get_comm_world_rank(comm, dest_task);
    dest = sctk_get_process_rank_from_task_rank(cw_rank);
    source = sctk_get_rank(communicator, get_task_rank());
  } else {
    source = get_process_rank();
  }

  /* Are we sending to our own process ?? */
  //~ if( dest == get_process_rank() )
  //~ {
  //~ /* If so we call directly */
  //~ control_message_submit( message_class, rail_id, get_process_rank(),
  // sctk_get_rank(communicator, get_task_rank() ), subtype, param, buffer,
  // size );
  //~ return;
  //~ }

  sctk_init_header(&msg, SCTK_MESSAGE_CONTIGUOUS, sctk_free_control_messages,
                   sctk_message_copy);

  /* Fill in control message context (note that class is handled by
   * set_header_in_message) */
  SCTK_MSG_SPECIFIC_CLASS_SET_SUBTYPE((&msg), subtype);
  SCTK_MSG_SPECIFIC_CLASS_SET_PARAM((&msg), param);
  SCTK_MSG_SPECIFIC_CLASS_SET_RAILID((&msg), rail_id);

  sctk_add_adress_in_message(&msg, buffer, size);

  // printpayload( buffer, size );
  sctk_set_header_in_message(&msg, tag, communicator, source,
                             (0 <= dest_task) ? dest_task : dest, &request,
                             size, message_class, SCTK_DATATYPE_IGNORE, REQUEST_SEND);

  sctk_send_message_try_check(&msg, 1);

  sctk_wait_message(&request);
}

void sctk_control_messages_send_process(int dest_process, int subtype,
                                        char param, void *buffer, size_t size) {
  __sctk_control_messages_send(dest_process, -1, SCTK_CONTROL_MESSAGE_PROCESS,
                               SCTK_COMM_WORLD, subtype, param, buffer, size,
                               -1);
}

void sctk_control_messages_send_to_task(int dest_task, sctk_communicator_t comm,
                                        int subtype, char param, void *buffer,
                                        size_t size) {
  sctk_info("Send task to %d (subtype %d)", dest_task, subtype);
  __sctk_control_messages_send(-1, dest_task, SCTK_CONTROL_MESSAGE_TASK, comm,
                               subtype, param, buffer, size, -1);
}

void sctk_control_messages_send_rail( int dest, int subtype, char  param, void *buffer, size_t size, int  rail_id ) 
{
  __sctk_control_messages_send(dest, -1, SCTK_CONTROL_MESSAGE_RAIL,
                               SCTK_COMM_WORLD, subtype, param, buffer, size,
                               rail_id);
}

void sctk_control_messages_incoming( sctk_thread_ptp_message_t * msg )
{
	sctk_message_class_t class = SCTK_MSG_SPECIFIC_CLASS( msg );

        switch (class) {
        /* Direct processing possibly outside of MPI context */
        case SCTK_CONTROL_MESSAGE_RAIL:
          sctk_control_messages_perform(msg, 0);
          break;

        /* Delayed processing but freeing the network progress threads
         * from the handler (avoid deadlocks) */
        case SCTK_CONTROL_MESSAGE_PROCESS:
        case SCTK_CONTROL_MESSAGE_USER:
          sctk_control_message_push(msg);
          break;

        default:
          not_reachable();
        }
}

void control_message_submit(sctk_message_class_t class, int rail_id,
                            int source_process, int source_rank, int subtype,
                            int param, void *data, size_t msg_size) {
  switch (class) {
  case SCTK_CONTROL_MESSAGE_RAIL: {
    sctk_rail_info_t *rail = sctk_rail_get_by_id(rail_id);

    if (!rail) {
      sctk_fatal("No such rail %d", rail_id);
    }

    if (!rail->control_message_handler) {
      sctk_warning("No handler was set to rail (%d = %s) control messages, set "
                   "it prior to send such messages",
                   rail_id, rail->network_name);
      return;
    }

    // printpayload( data, SCTK_MSG_SIZE ( msg ) );

    (rail->control_message_handler)(rail, source_process, source_rank, subtype,
                                    param, data, msg_size);
  } break;
  case SCTK_CONTROL_MESSAGE_TASK: {
    sctk_control_message_task_level(source_process, source_rank, subtype, param,
                                    data, msg_size);
  } break;
  case SCTK_CONTROL_MESSAGE_PROCESS: {
    sctk_control_message_process_level(source_process, source_rank, subtype,
                                       param, data, msg_size);
  } break;
  case SCTK_CONTROL_MESSAGE_USER: {
    struct sctk_control_message_context *ctx = sctk_control_message_ctx();

    if (!ctx->sctk_user_control_message) {
      sctk_warning("No handler was set to process user level control messages, "
                   "set it prior to send such messages");
      return;
    }

    (ctx->sctk_user_control_message)(source_process, source_rank, subtype,
                                     param, data, msg_size);
  } break;
  default:
    not_reachable();
  }
}

#define SCTK_CTRL_MSG_STATICBUFFER_SIZE 2048

void sctk_control_messages_perform(sctk_thread_ptp_message_t *msg, int force) {
  sctk_message_class_t class = SCTK_MSG_SPECIFIC_CLASS(msg);

  if (!sctk_message_class_is_control_message(class)) {
    sctk_fatal("Cannot process a non-control message using this function (%s)",
               sctk_message_class_name[class]);
  }

  /* Retrieve message info here as they are freed after match */
  int source_process = SCTK_MSG_SRC_PROCESS(msg);
  int source_rank = SCTK_MSG_SRC_TASK(msg);
  size_t msg_size = SCTK_MSG_SIZE(msg);
  sctk_communicator_t msg_comm = SCTK_MSG_COMMUNICATOR(msg);
  short subtype = SCTK_MSG_SPECIFIC_CLASS_SUBTYPE(msg);
  char param = SCTK_MSG_SPECIFIC_CLASS_PARAM(msg);
  char rail_id = SCTK_MSG_SPECIFIC_CLASS_RAILID(msg);

  int did_allocate = 0;
  void *tmp_contol_buffer = NULL;
  char __the_static_buffer_used_instead_of_doing_static_allocation
      [SCTK_CTRL_MSG_STATICBUFFER_SIZE];

  /* Do we need to rely on a dynamic allocation */
  if (SCTK_CTRL_MSG_STATICBUFFER_SIZE <= SCTK_MSG_SIZE(msg)) {
    /* Yes we will have to free */
    did_allocate = 1;
    /* Allocate */
    tmp_contol_buffer = sctk_calloc(SCTK_MSG_SIZE(msg), sizeof(char));
  } else {
    /* No need to free */
    did_allocate = 0;
    /* Refference the static buffer */
    tmp_contol_buffer =
        __the_static_buffer_used_instead_of_doing_static_allocation;
  }

  assume(tmp_contol_buffer != NULL);

  sctk_request_t request;

  if ((SCTK_MSG_DEST_TASK(msg) < 0) || force) {
    /* Generate the paired recv message to fill the buffer in
    * a portable way (idependently from driver) */

    sctk_thread_ptp_message_t recvmsg;

    sctk_init_header(&recvmsg, SCTK_MESSAGE_CONTIGUOUS,
                     sctk_free_control_messages, sctk_message_copy);
    sctk_add_adress_in_message(&recvmsg, tmp_contol_buffer, msg_size);
    sctk_set_header_in_message(&recvmsg, 0, msg_comm, SCTK_ANY_SOURCE,
                               get_process_rank(), &request,
                               SCTK_MSG_SIZE(msg), class, SCTK_DATATYPE_IGNORE, REQUEST_RECV);

    /* Trigger the receive task (as if we matched) */
    sctk_message_to_copy_t copy_task;
    copy_task.msg_send = msg;
    copy_task.msg_recv = &recvmsg;
    copy_task.prev = NULL;
    copy_task.next = NULL;
    /* Trigger the copy from network task */
    copy_task.msg_send->tail.message_copy(&copy_task);
  } else {
    sctk_message_irecv_class(source_rank, tmp_contol_buffer, msg_size, 16000,
                             msg_comm, class, &request);
    sctk_wait_message(&request);
  }

  void *data = tmp_contol_buffer;

  control_message_submit(class, rail_id, source_process, source_rank, subtype,
                         param, data, msg_size);

  /* Free only if the message was too large
   * for the static buffer */
  if (did_allocate) {
    /* Free the TMP buffer */
    sctk_free(tmp_contol_buffer);
  }
}

void sctk_control_message_fence_req(int target_task, sctk_communicator_t comm,
                                    sctk_request_t *req) {
  struct sctk_control_message_fence_ctx ctx;

  return;

  ctx.source = sctk_get_rank(comm, get_task_rank());
  ctx.remote = sctk_get_rank(comm, target_task);
  ctx.comm = comm;

  static int dummy = 0;

  sctk_message_irecv_class_dest(ctx.remote, ctx.source, &dummy, sizeof(int),
                                ctx.source, comm, SCTK_CONTROL_MESSAGE_FENCE,
                                req);

  sctk_control_messages_send_process(
      sctk_get_process_rank_from_task_rank(
          sctk_get_comm_world_rank(target_task, comm)),
      SCTK_PROCESS_FENCE, 0, &ctx,
      sizeof(struct sctk_control_message_fence_ctx));
}

void sctk_control_message_fence(int target_task, sctk_communicator_t comm) {
  return;
  sctk_request_t fence_req;

  sctk_control_message_fence_req(target_task, comm, &fence_req);

  sctk_wait_message(&fence_req);
}

void sctk_control_message_fence_handler( struct sctk_control_message_fence_ctx *ctx )
{
  static int dummy = 0;
  sctk_error("FENCE SEND %d -> %d", ctx->remote, ctx->source);

  sctk_control_message_process_local(get_task_rank());

  sctk_message_isend_class_src(ctx->remote, ctx->source, &dummy, sizeof(int),
                               ctx->source, ctx->comm,
                               SCTK_CONTROL_MESSAGE_FENCE, NULL);
}

/************************************************************************/
/* Control Messages List                                                */
/************************************************************************/

struct sctk_topological_polling_tree ___control_message_list_polling_tree;

struct sctk_ctrl_msg_cell
{
	sctk_thread_ptp_message_t * msg;
	
	struct sctk_ctrl_msg_cell *prev;
	struct sctk_ctrl_msg_cell *next;
};

struct sctk_ctrl_msg_cell * __ctrl_msg_list = NULL;
sctk_spinlock_t __ctrl_msg_list_lock = SCTK_SPINLOCK_INITIALIZER;

void sctk_control_message_push( sctk_thread_ptp_message_t * msg )
{
	struct sctk_ctrl_msg_cell * cell = sctk_malloc( sizeof( struct sctk_ctrl_msg_cell ) );
	cell->msg = msg;
	
	sctk_spinlock_lock( &__ctrl_msg_list_lock );
	DL_PREPEND( __ctrl_msg_list, cell );
	sctk_spinlock_unlock( &__ctrl_msg_list_lock );
}

void sctk_control_message_process_all()
{
	struct sctk_ctrl_msg_cell * cell = NULL;

        int present = 1;
        while (present) {


            if (__ctrl_msg_list) {
                sctk_spinlock_lock_yield(&__ctrl_msg_list_lock);

                if (__ctrl_msg_list) {
                    /* In UTLIST head->prev is the tail */
                    cell = __ctrl_msg_list->prev;
                    DL_DELETE(__ctrl_msg_list, cell);
                }

                sctk_spinlock_unlock(&__ctrl_msg_list_lock);
            }

          if (cell) {
            sctk_control_messages_perform(cell->msg, 0);
            sctk_free(cell);
            cell = NULL;
          } else {
            present = 0;
          }
        }
}

__thread int ___is_in_sctk_control_message_process_local = 0;

int sctk_control_message_process_local(int rank) {
  int ret = 0;

  if (rank < 0)
    return ret;

  if (___is_in_sctk_control_message_process_local)
    return ret;

  ___is_in_sctk_control_message_process_local = 1;

  int st;
  sctk_thread_message_header_t msg;
  memset(&msg, 0, sizeof(sctk_thread_message_header_t));

  sctk_probe_any_source_tag_class_comm(rank, 16000, SCTK_P2P_MESSAGE,
                                       SCTK_COMM_WORLD, &st, &msg);

  ___is_in_sctk_control_message_process_local = 0;

  return ret;
}

void __sctk_control_message_process(__UNUSED__ void *dummy) {
  struct sctk_ctrl_msg_cell *cell = NULL;

  if (__ctrl_msg_list != NULL) {
      sctk_spinlock_lock_yield(&__ctrl_msg_list_lock);

      if (__ctrl_msg_list != NULL) {

          /* In UTLIST head->prev is the tail */
          cell = __ctrl_msg_list->prev;
          DL_DELETE(__ctrl_msg_list, cell);
      }

      sctk_spinlock_unlock(&__ctrl_msg_list_lock);

      if (cell) {
          sctk_control_messages_perform(cell->msg, 0);
          sctk_free(cell);
      }
  }
}

static __thread int __inside_sctk_control_message_process = 0;

void sctk_control_message_process()
{

  __inside_sctk_control_message_process = 1;

  sctk_topological_polling_tree_poll(&___control_message_list_polling_tree,
                                     __sctk_control_message_process, NULL);

  __inside_sctk_control_message_process = 0;
}



void sctk_control_message_init()
{
	sctk_topological_polling_tree_init(  &___control_message_list_polling_tree, SCTK_POLL_SOCKET, SCTK_POLL_MACHINE,  0 );
}
