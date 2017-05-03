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

#include "mpi_rma.h"
#include "mpc_datatypes.h"
#include "mpc_mpi_internal.h"
#include "mpcmp.h"
#include "sctk_communicator.h"
#include "sctk_control_messages.h"
#include <string.h>

#include "mpi_rma_ctrl_msg.h"

/************************************************************************/
/* MPI_Get                                                              */
/************************************************************************/

static inline int mpc_MPI_Get_RMA(struct mpc_MPI_Win *desc, void *origin_addr,
                                  int origin_count,
                                  MPI_Datatype origin_datatype, int target_rank,
                                  MPI_Aint target_disp, int target_count,
                                  MPI_Datatype target_datatype, MPI_Win win,
                                  sctk_request_t *ref_request) {
  int can_read_rma = 0;
  int can_write_rma = 0;

  /* Do the optimized RMA only if target datatype is contiguous */
  if (sctk_datatype_is_common(target_datatype) ||
      sctk_datatype_is_contiguous(target_datatype) ||
      sctk_datatype_is_struct_datatype(target_datatype)) {
    can_read_rma = 1;
  }

  /* Can the RMA directly target the local buffer (ie contig) ? */
  if (sctk_datatype_is_common(origin_datatype) ||
      sctk_datatype_is_contiguous(origin_datatype) ||
      sctk_datatype_is_struct_datatype(origin_datatype)) {
    can_write_rma = 1;
  }

  if (target_rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  /* At this point we deal with an RMA capable RDMA */

  /* Now compute & check boundaries */

  MPI_Count remote_size = 0;
  PMPI_Type_size_x(target_datatype, &remote_size);
  remote_size *= target_count;

  MPI_Count local_size = 0;
  PMPI_Type_size_x(origin_datatype, &local_size);
  local_size *= origin_count;

  if ((remote_size == 0) || (local_size == 0)) {
    /* We may have nothing to do */
    return MPI_SUCCESS;
  }

  /* Pick Target window */
  if (desc->comm_size <= target_rank) {
    return MPI_ERR_ARG;
  }

  if (desc->remote_wins[target_rank] < 0) {
    return MPI_ERR_ARG;
  }

  int pack_size = local_size;
  void *tmp_buff = NULL;

  /* Local datatype is not contiguous */
  if (!can_write_rma) {
    /* In this case only remote is contig */

    if (PMPI_Pack_size(origin_count, origin_datatype, desc->comm, &pack_size) !=
        MPI_SUCCESS) {
      return MPI_ERR_INTERN;
    }

    tmp_buff = mpc_MPI_Win_tmp_alloc(&desc->source.tmp_buffs, pack_size);
  }

  /* Basic Check */
  if (local_size < remote_size) {
    return MPI_ERR_ARG;
  }

  int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 0);

  if (can_write_rma && can_read_rma) {

    struct sctk_window *low_remote_win = sctk_win_translate(target_win);

    void *target_addr =
        low_remote_win->start_addr + target_disp * desc->win_disp;

    if (mpc_MPI_win_can_write_directly(desc, target_addr, target_rank)) {

      memcpy(origin_addr, target_addr, remote_size);
    } else {

      /* Pick a Request */
      sctk_request_t *request =
          mpc_MPI_Win_request_array_pick(&desc->source.requests);

      if (ref_request) {
        request->pointer_to_source_request = ref_request;
        ref_request->pointer_to_shadow_request = request;
      }

      sctk_window_RDMA_read(target_win, origin_addr, remote_size, target_disp,
                            request);
    }

  } else {

    /* One of the two segments is not contiguous */
    /* Local non-contig, remote contig */
    if (!can_write_rma && can_read_rma) {

      sctk_request_t req;
      sctk_init_request(&req, desc->comm, REQUEST_SEND);
      sctk_window_RDMA_read(target_win, tmp_buff, remote_size, target_disp,
                            &req);
      sctk_wait_message(&req);

      int pos = 0;
      __INTERNAL__PMPI_Unpack(tmp_buff, pack_size, &pos, origin_addr,
                              origin_count, origin_datatype, desc->comm);

    } else if (!can_read_rma) {
      int target_pack_size = 0;

      if (PMPI_Pack_size(target_count, target_datatype, desc->comm,
                         &target_pack_size) != MPI_SUCCESS) {
        return MPI_ERR_INTERN;
      }

      sctk_info("REMOTE PACK THEN UNPACK (%d to %d)", desc->comm_rank,
                target_rank);
      /* Emit fractionned Read */
      size_t stsize;
      void *serialized_type = sctk_derived_datatype_serialize(
          target_datatype, &stsize, sizeof(struct mpc_MPI_Win_ctrl_message));

      struct mpc_MPI_Win_ctrl_message *message =
          (struct mpc_MPI_Win_ctrl_message *)serialized_type;
      mpc_MPI_Win_tmp_register(&desc->source.tmp_buffs, message, stsize);

      mpc_MPI_Win_init_derived_recv(message, win, target_rank, target_disp,
                                    target_pack_size, target_count);

      /* Already submit the packed data */
      sctk_request_t *request = NULL;
      sctk_request_t local_req;
      void *dest_buff = NULL;

      if (!can_write_rma) {
        request = &local_req;
        dest_buff =
            mpc_MPI_Win_tmp_alloc(&desc->source.tmp_buffs, target_pack_size);

      } else {
        request = mpc_MPI_Win_request_array_pick(&desc->source.requests);
        dest_buff = origin_addr;
      }

      sctk_message_irecv_class_dest(target_rank, desc->comm_rank, dest_buff,
                                    target_pack_size, TAG_RDMA_READ, desc->comm,
                                    SCTK_RDMA_MESSAGE, request);

      /* Now notify remote */
      mpc_MPI_Win_control_message_send_piggybacked(win, target_rank, message,
                                                   stsize, 1);

      if (!can_write_rma) {
        sctk_wait_message(request);
        int pos = 0;
        __INTERNAL__PMPI_Unpack(dest_buff, target_pack_size, &pos, origin_addr,
                                origin_count, origin_datatype, desc->comm);
      }

    } else {
      not_reachable();
    }
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Get(void *origin_addr, int origin_count,
                MPI_Datatype origin_datatype, int target_rank,
                MPI_Aint target_disp, int target_count,
                MPI_Datatype target_data_type, MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  int ret = MPI_SUCCESS;

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  ret = mpc_MPI_Get_RMA(desc, origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count,
                        target_data_type, win, NULL);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

int mpc_MPI_Rget(void *origin_addr, int origin_count,
                 MPI_Datatype origin_datatype, int target_rank,
                 MPI_Aint target_disp, int target_count,
                 MPI_Datatype target_data_type, MPI_Win win,
                 MPI_Request *request) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  /* Create a new MPI request */
  sctk_request_t *new_request =
      __sctk_new_mpc_request(request, __sctk_internal_get_MPC_requests());

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Get_RMA(desc, origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count,
                        target_data_type, win, new_request);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

/************************************************************************/
/* MPI_Put                                                              */
/************************************************************************/

static inline int mpc_MPI_Put_RMA(struct mpc_MPI_Win *desc,
                                  const void *origin_addr, int origin_count,
                                  MPI_Datatype origin_datatype, int target_rank,
                                  MPI_Aint target_disp, int target_count,
                                  MPI_Datatype target_datatype, MPI_Win win,
                                  sctk_request_t *ref_request) {
  int can_read_rma = 0;
  int can_write_rma = 0;

  void *local_buff = NULL;
  void *remote_buff = NULL;

  if (target_rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  /* Do the optimized RMA only if target datatype is contiguous */
  if (sctk_datatype_is_common(target_datatype) ||
      sctk_datatype_is_contiguous(target_datatype) ||
      sctk_datatype_is_struct_datatype(target_datatype)) {
    can_write_rma = 1;
  }

  /* Can the RMA directly target the local buffer (ie contig) ? */
  if (sctk_datatype_is_common(origin_datatype) ||
      sctk_datatype_is_contiguous(origin_datatype) ||
      sctk_datatype_is_struct_datatype(origin_datatype)) {
    can_read_rma = 1;
  }

  /* Now compute & check boundaries */

  MPI_Count remote_size = 0;
  PMPI_Type_size_x(target_datatype, &remote_size);
  remote_size *= target_count;

  MPI_Count local_size = 0;
  PMPI_Type_size_x(origin_datatype, &local_size);
  local_size *= origin_count;

  if ((remote_size == 0) || (local_size == 0)) {
    /* We may have nothing to do */
    return MPI_SUCCESS;
  }

  int pack_size = local_size;

  // sctk_error("SOURCE S %d DEST S %d", local_size, remote_size );

  /* Basic Check */
  if (remote_size < local_size) {
    return MPI_ERR_ARG;
  }

  /* Pick Target window */
  if (desc->comm_size <= target_rank) {
    return MPI_ERR_ARG;
  }

  if (desc->remote_wins[target_rank] < 0) {
    return MPI_ERR_ARG;
  }

  /* Do we need to pack the local data-type ? */
  if (!can_read_rma) {

    if (PMPI_Pack_size(origin_count, origin_datatype, desc->comm, &pack_size) !=
        MPI_SUCCESS) {
      return MPI_ERR_INTERN;
    }

    /* We need a temporary pack buffer */

    void *tmp_buff = mpc_MPI_Win_tmp_alloc(&desc->source.tmp_buffs, pack_size);

    /* Pack in TMP buffer */
    int pos = 0;

    if (MPI_Pack((char *)origin_addr, origin_count, origin_datatype, tmp_buff,
                 pack_size, &pos, desc->comm) != MPI_SUCCESS) {
      return MPI_ERR_INTERN;
    }

    /* Now we override the input to consider a contiguous pack like if nothing
     * happened
     * note that the TMP buff will be freed at the end of the epoch */
    origin_addr = tmp_buff;

    /* Now that we packed flag read */
    can_read_rma = 1;
  }

  /* Most Optimal case, all contig on dest side */
  if (can_write_rma && can_read_rma) {

    int target_win_direct = mpc_MPI_win_get_remote_win(desc, target_rank, 0);
    struct sctk_window *low_remote_win = sctk_win_translate(target_win_direct);

    void *target_addr =
        low_remote_win->start_addr + target_disp * desc->win_disp;

    if (mpc_MPI_win_can_write_directly(desc, target_addr, target_rank)) {
      //	sctk_error("DIRECT");
      memcpy(target_addr, origin_addr, remote_size);
    } else {

      /* Pick a Request */
      sctk_request_t *request =
          mpc_MPI_Win_request_array_pick(&desc->source.requests);

      if (ref_request) {
        request->pointer_to_source_request = ref_request;
        ref_request->pointer_to_shadow_request = request;
      }

      int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 1);
      sctk_nodebug("--> %d %p write %d to off %d", target_rank, origin_addr,
                   remote_size, target_disp);
      sctk_window_RDMA_write(target_win, (void *)origin_addr, remote_size,
                             target_disp, request);

      // sctk_atomics_incr_int( &desc->source.ctrl_message_counter );
    }
  } else {

    /* Emit fractionned Write */
    size_t stsize;
    void *serialized_type = sctk_derived_datatype_serialize(
        target_datatype, &stsize, sizeof(struct mpc_MPI_Win_ctrl_message));

    struct mpc_MPI_Win_ctrl_message *message =
        (struct mpc_MPI_Win_ctrl_message *)serialized_type;

    mpc_MPI_Win_tmp_register(&desc->source.tmp_buffs, message, stsize);

    mpc_MPI_Win_init_derived_send(message, win, target_rank, target_disp,
                                  pack_size, target_count);

    /* Already submit the packed data */
    sctk_request_t *request1 =
        mpc_MPI_Win_request_array_pick(&desc->source.requests);

    sctk_message_isend_class_src(desc->comm_rank, target_rank,
                                 (char *)origin_addr, pack_size, TAG_RDMA_WRITE,
                                 desc->comm, SCTK_RDMA_MESSAGE, request1);

    int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 1);
    struct sctk_window *low_remote_win = sctk_win_translate(target_win);
    sctk_window_inc_outgoing(low_remote_win);

    sctk_atomics_incr_int(&desc->source.ctrl_message_counter);

    /* Now notify remote */
    mpc_MPI_Win_control_message_send_piggybacked(win, target_rank, message,
                                                 stsize, 1);
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Put(const void *origin_addr, int origin_count,
                MPI_Datatype origin_datatype, int target_rank,
                MPI_Aint target_disp, int target_count,
                MPI_Datatype target_datatype, MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  int ret = MPI_SUCCESS;

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  ret = mpc_MPI_Put_RMA(desc, origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count, target_datatype,
                        win, NULL);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

int mpc_MPI_Rput(const void *origin_addr, int origin_count,
                 MPI_Datatype origin_datatype, int target_rank,
                 MPI_Aint target_disp, int target_count,
                 MPI_Datatype target_datatype, MPI_Win win,
                 MPI_Request *request) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  /* Create a new MPI request */
  sctk_request_t *new_request =
      __sctk_new_mpc_request(request, __sctk_internal_get_MPC_requests());

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Put_RMA(desc, origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count, target_datatype,
                        win, new_request);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

/************************************************************************/
/* MPI_Accumulate                                                       */
/************************************************************************/

/* MPC_Op_f sctk_get_common_function (MPC_Datatype datatype, MPC_Op op); */

/*
 * The algorithm is the following:
 *
 * We pack the local buffer (if needed) and we send it then we send
 * a control message which allocates and receive it.
 *
 * Then we pack the remote data (if needed) and we apply the operation
 * between the two buffers. Eventually we unpack the target buffer at
 * destination to put data in place.
 *
 */

static inline int
mpc_MPI_Accumulate_RMA(struct mpc_MPI_Win *desc, void *origin_addr,
                       int origin_count, MPI_Datatype origin_datatype,
                       int target_rank, MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                       sctk_request_t *ref_request) {
  if (target_rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  /* First of all reroute to Put if the OP is MPI_REPLACE */
  if (op == MPI_REPLACE) {
    return mpc_MPI_Put_RMA(desc, origin_addr, origin_count, origin_datatype,
                           target_rank, target_disp, target_count,
                           target_datatype, win, ref_request);
  }

  /* Otherwise proceed with Accumulate */

  int can_read_rma = 0;
  int can_write_rma = 0;

  void *local_buff = NULL;
  void *remote_buff = NULL;

  /* Do the optimized RMA only if target datatype is contiguous */
  if (sctk_datatype_is_common(target_datatype) ||
      sctk_datatype_is_contiguous(target_datatype) ||
      sctk_datatype_is_struct_datatype(target_datatype)) {
    can_write_rma = 1;
  }

  /* Can the RMA directly target the local buffer (ie contig) ? */
  if (sctk_datatype_is_common(origin_datatype) ||
      sctk_datatype_is_contiguous(origin_datatype) ||
      sctk_datatype_is_struct_datatype(origin_datatype)) {
    can_read_rma = 1;
  }

  /* Now compute & check boundaries */

  MPI_Count remote_size = 0;
  PMPI_Type_size_x(target_datatype, &remote_size);
  remote_size *= target_count;

  MPI_Count local_size = 0;
  PMPI_Type_size_x(origin_datatype, &local_size);
  local_size *= origin_count;

  if ((remote_size == 0) || (local_size == 0)) {
    /* We may have nothing to do */
    return MPI_SUCCESS;
  }

  int pack_size = local_size;
  int pos = 0;

  /* Basic Check */
  if (remote_size < local_size) {
    return MPI_ERR_ARG;
  }

  /* Pick Target window */
  if (desc->comm_size <= target_rank) {
    return MPI_ERR_ARG;
  }

  if (desc->remote_wins[target_rank] < 0) {
    return MPI_ERR_ARG;
  }

  mpc_MPI_accumulate_op_lock();
  int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 1);

  /* Do we need to pack the local data-type ? */
  if (!can_read_rma) {

    /* In this case only remote is contig */

    if (PMPI_Pack_size(origin_count, origin_datatype, desc->comm, &pack_size) !=
        MPI_SUCCESS) {
      mpc_MPI_accumulate_op_unlock();
      return MPI_ERR_INTERN;
    }

    void *tmp_buff = mpc_MPI_Win_tmp_alloc(&desc->source.tmp_buffs, pack_size);

    /* Pack in TMP buffer */
    pos = 0;
    if (MPI_Pack(origin_addr, origin_count, origin_datatype, tmp_buff,
                 pack_size, &pos, desc->comm) != MPI_SUCCESS) {
      mpc_MPI_accumulate_op_unlock();
      return MPI_ERR_INTERN;
    }

    /* Now we override the input to consider a contiguous pack like if nothing
     * happened
     * note that the TMP buff will be freed at the end of the epoch */
    origin_addr = tmp_buff;

    /* Now that we packed flag read */
    can_read_rma = 1;
  }

  /* Retrieve the Inner target datatype */

  MPI_Datatype inner_type = -1;

  if (sctk_datatype_is_common(target_datatype)) {
    inner_type = target_datatype;
  } else {
    inner_type = sctk_datatype_get_inner_type(target_datatype);
  }

  if (!sctk_datatype_is_common(inner_type) &&
      !sctk_datatype_is_struct_datatype(inner_type)) {
    sctk_warning("MPI_Accumulate : cannot accumulate a derived datatype which "
                 "is not made of a single predefined type");
    mpc_MPI_accumulate_op_unlock();
    return MPI_ERR_ARG;
  }

  struct sctk_window *low_win = sctk_win_translate(target_win);
  assume(low_win);

  void *start_addr =
      (char *)low_win->start_addr + low_win->disp_unit * target_disp;

  /* Check is the remote window is not local */
  if (mpc_MPI_win_can_write_directly(desc, start_addr, target_rank)) {
    /*
     *
     * THIS IS THE LOCAL CASE (Shared memory)
     *
     */
    MPI_Count remote_size = 0;
    PMPI_Type_size_x(target_datatype, &remote_size);
    remote_size *= target_count;

    int target_pack_size = remote_size;

    void *target_address = NULL;

    if (!can_write_rma) {
      /* ##!
       * If we are here we are targetting a local buffer but the
       * target is not contiguous we need to pack it */
      if (PMPI_Pack_size(target_count, target_datatype, desc->comm,
                         &target_pack_size) != MPI_SUCCESS) {
        mpc_MPI_accumulate_op_unlock();
        return MPI_ERR_INTERN;
      }

      void *tmp_target_buff =
          mpc_MPI_Win_tmp_alloc(&desc->source.tmp_buffs, target_pack_size);

      /* Pack in TMP buffer */
      pos = 0;
      if (MPI_Pack(start_addr, target_count, target_datatype, tmp_target_buff,
                   target_pack_size, &pos, desc->comm) != MPI_SUCCESS) {
        mpc_MPI_accumulate_op_unlock();
        return MPI_ERR_INTERN;
      }

      target_address = tmp_target_buff;
    } else {
      target_address = start_addr;
    }

    /* Now do the OP directly */
    sctk_op_t *mpi_op = sctk_convert_to_mpc_op(op);
    MPC_Op mpc_op = mpi_op->op;
    MPC_Op_f func = sctk_get_common_function(inner_type, mpc_op);

    MPI_Count per_elem_size = 0;
    PMPI_Type_size_x(inner_type, &per_elem_size);

    size_t max_size =
        (target_pack_size < pack_size) ? target_pack_size : pack_size;

    sctk_nodebug("F %p ORIG %p TARGET %p SI %ld INT %d PE %d", func,
                 origin_addr, target_address, max_size / per_elem_size,
                 inner_type, per_elem_size);
    func(origin_addr, target_address, max_size / per_elem_size, inner_type);

    if (!can_write_rma) {

      /* We enter here only if the target buffer is a packed one see ##! */
      pos = 0;
      __INTERNAL__PMPI_Unpack(target_address, target_pack_size, &pos,
                              start_addr, target_count, target_datatype,
                              desc->comm);
    }

  } else {

    /*
     *
     * This is the NET case
     *
     */

    /* We now send the buffer inside a control message */

    /* Most Optimal case, all contig on dest side */
    int do_free_target_type = 0;

    if (can_write_rma) {
      /* If the target type is a contiguous one
       * now create a derived one to simplify code
       * branching (removing cases on target) */
      PMPC_Type_convert_to_derived(target_datatype, &target_datatype);
      do_free_target_type = 1;
    }

    /* Serialize the remote type */
    size_t stsize;
    void *serialized_type = sctk_derived_datatype_serialize(
        target_datatype, &stsize, sizeof(struct mpc_MPI_Win_ctrl_message));

    struct mpc_MPI_Win_ctrl_message *message =
        (struct mpc_MPI_Win_ctrl_message *)serialized_type;
    mpc_MPI_Win_tmp_register(&desc->source.tmp_buffs, message, stsize);

    mpc_MPI_Win_init_accumulate_send(message, win, target_rank, target_disp,
                                     pack_size, target_count, inner_type, op);

    /* Already submit the packed data */
    sctk_request_t *request =
        mpc_MPI_Win_request_array_pick(&desc->source.requests);

    if (ref_request) {
      request->pointer_to_source_request = ref_request;
      ref_request->pointer_to_shadow_request = request;
    }

    sctk_message_isend_class_src(desc->comm_rank, target_rank, origin_addr,
                                 pack_size, TAG_RDMA_ACCUMULATE, desc->comm,
                                 SCTK_RDMA_MESSAGE, request);

    int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 1);
    struct sctk_window *low_remote_win = sctk_win_translate(target_win);
    sctk_window_inc_outgoing(low_remote_win);

    sctk_atomics_incr_int(&desc->source.ctrl_message_counter);

    /* Now notify remote */
    mpc_MPI_Win_control_message_send_piggybacked(win, target_rank, message,
                                                 stsize, 1);

    if (do_free_target_type &&
        !sctk_datatype_is_struct_datatype(target_datatype)) {
      PMPC_Type_free(&target_datatype);
    }
  }

  mpc_MPI_accumulate_op_unlock();

  return MPI_SUCCESS;
}

int mpc_MPI_Accumulate(void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPI_Op op, MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  int ret = MPI_SUCCESS;

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  ret = mpc_MPI_Accumulate_RMA(desc, origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count,
                               target_datatype, op, win, NULL);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

int mpc_MPI_Raccumulate(void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, int target_rank,
                        MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                        MPI_Request *request) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  /* Create a new MPI request */
  sctk_request_t *new_request =
      __sctk_new_mpc_request(request, __sctk_internal_get_MPC_requests());

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Accumulate_RMA(desc, origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count,
                               target_datatype, op, win, new_request);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

/************************************************************************/
/* Conversion to low-level RMA                                          */
/************************************************************************/

static inline RDMA_op mpc_RMA_convert_op(MPI_Op op) {
  switch (op) {
  case MPI_SUM:
    return RDMA_SUM;
    break;
  case MPI_MIN:
    return RDMA_SUM;
    break;
  case MPI_MAX:
    return RDMA_SUM;
    break;
  case MPI_PROD:
    return RDMA_SUM;
    break;
  case MPI_LAND:
    return RDMA_SUM;
    break;
  case MPI_BAND:
    return RDMA_SUM;
    break;
  case MPI_LOR:
    return RDMA_SUM;
    break;
  case MPI_BOR:
    return RDMA_SUM;
    break;
  case MPI_LXOR:
    return RDMA_SUM;
    break;
  case MPI_BXOR:
    return RDMA_SUM;
    break;
  default:
    return -1;
  }

  return -1;
}

static inline RDMA_type mpc_RMA_convert_type(MPI_Datatype type) {

  switch (type) {
  case MPI_CHAR:
    return RDMA_TYPE_CHAR;
    break;
  case MPI_DOUBLE:
    return RDMA_TYPE_DOUBLE;
    break;
  case MPI_FLOAT:
    return RDMA_TYPE_FLOAT;
    break;
  case MPI_INT:
    return RDMA_TYPE_INT;
    break;
  case MPI_LONG:
    return RDMA_TYPE_LONG;
    break;
  case MPI_LONG_DOUBLE:
    return RDMA_TYPE_LONG_DOUBLE;
    break;
  case MPI_LONG_LONG:
    return RDMA_TYPE_LONG_LONG;
    break;
  case MPI_LONG_LONG_INT:
    return RDMA_TYPE_LONG_LONG_INT;
    break;
  case MPI_SHORT:
    return RDMA_TYPE_SHORT;
    break;
  case MPI_SIGNED_CHAR:
    return RDMA_TYPE_SIGNED_CHAR;
    break;
  case MPI_UNSIGNED:
    return RDMA_TYPE_UNSIGNED;
    break;
  case MPI_UNSIGNED_CHAR:
    return RDMA_TYPE_UNSIGNED_CHAR;
    break;
  case MPI_UNSIGNED_LONG:
    return RDMA_TYPE_UNSIGNED_LONG;
    break;
  case MPI_UNSIGNED_LONG_LONG:
    return RDMA_TYPE_UNSIGNED_LONG_LONG;
    break;
  case MPI_UNSIGNED_SHORT:
    return RDMA_TYPE_UNSIGNED_SHORT;
    break;
  case MPI_WCHAR:
    return RDMA_TYPE_WCHAR;
    break;
  default:
    return -1;
  }

  return -1;
}

/************************************************************************/
/* INTERNAL MPI_Get_accumulate                                          */
/************************************************************************/

static inline int
mpc_MPI_Fetch_and_op_RMA(struct mpc_MPI_Win *desc, const void *origin_addr,
                         void *result_addr, MPI_Datatype datatype,
                         int target_rank, MPI_Aint target_disp, MPI_Op op,
                         MPI_Win win, sctk_request_t *ref_request);

static inline int mpc_MPI_GACC_fast_path(RDMA_op op, RDMA_type type) {
  if (op != RDMA_SUM) {
    return 0;
  }

  switch (type) {
  case RDMA_TYPE_INT:
  case RDMA_TYPE_FLOAT:
    return 1;
  default:
    return 0;
  }
}

static inline int mpc_MPI_Get_accumulate_RMA(
    struct mpc_MPI_Win *desc, const void *origin_addr, int origin_count,
    MPI_Datatype origin_datatype, void *result_addr, int result_count,
    MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
    int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
    sctk_request_t *ref_request) {
  if (target_rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  if ((origin_count == result_count) && (origin_count == target_count) &&
      (origin_count == 0)) {
    return MPI_SUCCESS;
  }

  /* Check if we can use the optimized case (fallback to FOP) */
  if (sctk_datatype_is_common(origin_datatype) &&
      sctk_datatype_is_common(result_datatype) &&
      sctk_datatype_is_common(target_datatype)) {
    if ((origin_datatype == result_datatype) &&
        (origin_datatype == target_datatype)) {
      RDMA_op rmaop = mpc_RMA_convert_op(op);
      RDMA_type rmatype = mpc_RMA_convert_type(target_datatype);

      if ((0 <= rmaop) && (0 <= rmatype) && (origin_count == result_count) &&
          (origin_count == target_count)) {

        int target_win_direct =
            mpc_MPI_win_get_remote_win(desc, target_rank, 0);
        struct sctk_window *low_remote_win =
            sctk_win_translate(target_win_direct);

        void *ptarget =
            low_remote_win->start_addr + target_disp * desc->win_disp;

        /* Fast Path */
        if ((!desc->is_over_network ||
             mpc_MPI_win_can_write_directly(desc, ptarget, target_rank)) &&
            mpc_MPI_GACC_fast_path(rmaop, rmatype)) {

          int i;

          switch (rmatype) {
          case RDMA_TYPE_INT:
            memcpy(result_addr, ptarget, origin_count * sizeof(int));

            for (i = 0; i < origin_count; i++) {
              *((int *)ptarget + i) += *((int *)origin_addr + i);
            }
            break;
          case RDMA_TYPE_FLOAT:
            memcpy(result_addr, ptarget, origin_count * sizeof(float));

            for (i = 0; i < origin_count; i++) {
              *((float *)ptarget + i) += *((float *)origin_addr + i);
            }

            break;
          default:
            not_reachable();
          }

          return MPI_SUCCESS;
        } else if (origin_count == 1) {
          /* If we are here we can use FOP */
          return mpc_MPI_Fetch_and_op_RMA(desc, origin_addr, result_addr,
                                          origin_datatype, target_rank,
                                          target_disp, op, win, ref_request);
        }
      }
    }
  }

  /* First retrieve the target buffer in the result buffer */

  int ret = mpc_MPI_Get(result_addr, result_count, result_datatype, target_rank,
                        target_disp, target_count, target_datatype, win);

  if (op != MPI_NO_OP) {
    mpc_MPI_Win_request_array_fence_no_ops(&desc->source.requests);

    /* Now accumulate origin in the target */

    ret = mpc_MPI_Accumulate((char *)origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count,
                             target_datatype, op, win);

    mpc_MPI_Win_request_array_fence_no_ops(&desc->source.requests);
  }

  return ret;
}

int mpc_MPI_Get_accumulate(const void *origin_addr, int origin_count,
                           MPI_Datatype origin_datatype, void *result_addr,
                           int result_count, MPI_Datatype result_datatype,
                           int target_rank, MPI_Aint target_disp,
                           int target_count, MPI_Datatype target_datatype,
                           MPI_Op op, MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Get_accumulate_RMA(
      desc, origin_addr, origin_count, origin_datatype, result_addr,
      result_count, result_datatype, target_rank, target_disp, target_count,
      target_datatype, op, win, NULL);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

int mpc_MPI_Rget_accumulate(const void *origin_addr, int origin_count,
                            MPI_Datatype origin_datatype, void *result_addr,
                            int result_count, MPI_Datatype result_datatype,
                            int target_rank, MPI_Aint target_disp,
                            int target_count, MPI_Datatype target_datatype,
                            MPI_Op op, MPI_Win win, MPI_Request *request) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  /* Create a new MPI request */
  sctk_request_t *new_request =
      __sctk_new_mpc_request(request, __sctk_internal_get_MPC_requests());

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Get_accumulate_RMA(
      desc, origin_addr, origin_count, origin_datatype, result_addr,
      result_count, result_datatype, target_rank, target_disp, target_count,
      target_datatype, op, win, new_request);

  new_request->completion_flag = SCTK_MESSAGE_DONE;

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

/************************************************************************/
/* INTERNAL MPI_Fetch_and_op                                            */
/************************************************************************/

static inline int mpc_MPI_FOP_fast_path(RDMA_op op, RDMA_type type) {
  if (op != RDMA_SUM) {
    return 0;
  }

  switch (type) {
  case RDMA_TYPE_INT:
  case RDMA_TYPE_FLOAT:
    return 1;
  default:
    return 0;
  }

  return 0;
}

static inline int
mpc_MPI_Fetch_and_op_RMA(struct mpc_MPI_Win *desc, const void *origin_addr,
                         void *result_addr, MPI_Datatype datatype,
                         int target_rank, MPI_Aint target_disp, MPI_Op op,
                         MPI_Win win, sctk_request_t *ref_request) {
  if (target_rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  /* Handle special operations */
  if (op == MPI_NO_OP) {
    /* Use accumulate to handle this case */
    mpc_MPI_Get(result_addr, 1, datatype, target_rank, target_disp, 1, datatype,
                win);

    return MPI_SUCCESS;
  }

  /* Handle special operations */
  if (op == MPI_REPLACE) {
    /* Use accumulate to handle this case */

    mpc_MPI_Get(result_addr, 1, datatype, target_rank, target_disp, 1, datatype,
                win);

    mpc_MPI_Put_RMA(desc, origin_addr, 1, datatype, target_rank, target_disp, 1,
                    datatype, win, ref_request);

    return MPI_SUCCESS;
  }

  RDMA_op rmaop = mpc_RMA_convert_op(op);

  if (0 <= rmaop) {
    RDMA_type rmatype = mpc_RMA_convert_type(datatype);

    if (0 <= rmatype) {
      int target_win_direct = mpc_MPI_win_get_remote_win(desc, target_rank, 0);
      struct sctk_window *low_remote_win =
          sctk_win_translate(target_win_direct);

      void *ptarg = low_remote_win->start_addr + target_disp * desc->win_disp;

      /* fast path */
      if ((!desc->is_over_network ||
           mpc_MPI_win_can_write_directly(desc, ptarg, target_rank)) &&
          (mpc_MPI_FOP_fast_path(rmaop, rmatype))) {

        switch (rmatype) {
        case RDMA_TYPE_FLOAT:
          *((float *)result_addr) = *((float *)ptarg);
          *((float *)ptarg) += *((float *)origin_addr);
          break;

        case RDMA_TYPE_INT:
          *((int *)result_addr) = *((int *)ptarg);
          *((int *)ptarg) += *((int *)(origin_addr));
          break;
        default:
          not_reachable();
        }

        return MPI_SUCCESS;
      }

      /* If we are here we can use the low-level fetch and OP */
      /* Pick a Request */
      sctk_request_t *request =
          mpc_MPI_Win_request_array_pick(&desc->source.requests);

      if (ref_request) {
        request->pointer_to_source_request = ref_request;
        ref_request->pointer_to_shadow_request = request;
      }

      int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 1);

      sctk_window_RDMA_fetch_and_op(target_win, target_disp, result_addr,
                                    (void *)origin_addr, rmaop, rmatype,
                                    request);

      return MPI_SUCCESS;
    }
  }

  /* If we are here we were not able to use the optmized FOP now use
   * getaccumulate which is more general */
  sctk_error("ACC");
  return mpc_MPI_Get_accumulate_RMA(desc, origin_addr, 1, datatype, result_addr,
                                    1, datatype, target_rank, target_disp, 1,
                                    datatype, op, win, ref_request);
}

int mpc_MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                         MPI_Datatype datatype, int target_rank,
                         MPI_Aint target_disp, MPI_Op op, MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Fetch_and_op_RMA(desc, origin_addr, result_addr, datatype,
                                 target_rank, target_disp, op, win, NULL);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

/************************************************************************/
/* INTERNAL MPI_Compare_and_swap                                        */
/************************************************************************/

static inline int mpc_MPI_CAS_fast_path(size_t type_size) {
  switch (type_size) {
  case 4:
  case 8:
    return 1;
  }

  return 0;
}

static inline int mpc_MPI_Compare_and_swap_RMA(
    struct mpc_MPI_Win *desc, const void *origin_addr, const void *compare_addr,
    void *result_addr, MPI_Datatype datatype, int target_rank,
    MPI_Aint target_disp, MPI_Win win, sctk_request_t *ref_request) {
  if (target_rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  RDMA_type rmatype = mpc_RMA_convert_type(datatype);

  if (0 <= rmatype) {
    size_t rmatsize = RDMA_type_size(rmatype);

    int target_win_direct = mpc_MPI_win_get_remote_win(desc, target_rank, 0);
    struct sctk_window *low_remote_win = sctk_win_translate(target_win_direct);

    void *ptarget = low_remote_win->start_addr + target_disp * desc->win_disp;

    /* fast path */
    if ((!desc->is_over_network ||
         mpc_MPI_win_can_write_directly(desc, ptarget, target_rank)) &&
        (mpc_MPI_CAS_fast_path(rmatsize))) {
      switch (rmatsize) {
      case 4:
        *((int *)result_addr) = *((int *)ptarget);
        sctk_atomics_cas_int(ptarget, *((int *)compare_addr),
                             *((int *)origin_addr));
        break;

      case 8:
        *((void **)result_addr) = *((void **)ptarget);
        sctk_atomics_cas_ptr(ptarget, *((void **)compare_addr),
                             *((void **)origin_addr));
        break;
      }

      return MPI_SUCCESS;
    }

    /* If we are here we can use the low-level fetch and OP */
    /* Pick a Request */
    sctk_request_t *request =
        mpc_MPI_Win_request_array_pick(&desc->source.requests);

    if (ref_request) {
      request->pointer_to_source_request = ref_request;
      ref_request->pointer_to_shadow_request = request;
    }

    int target_win = mpc_MPI_win_get_remote_win(desc, target_rank, 1);

    sctk_window_RDMA_CAS(target_win, target_disp, (char *)compare_addr,
                         (char *)origin_addr, result_addr, rmatype, request);
  } else {
    return MPI_ERR_ARG;
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                             void *result_addr, MPI_Datatype datatype,
                             int target_rank, MPI_Aint target_disp,
                             MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)sctk_window_get_payload(win);

  mpc_MPI_Win_request_array_add_pending(&desc->source.requests);

  int ret = MPI_SUCCESS;

  ret = mpc_MPI_Compare_and_swap_RMA(desc, origin_addr, compare_addr,
                                     result_addr, datatype, target_rank,
                                     target_disp, win, NULL);

  mpc_MPI_Win_request_array_add_done(&desc->source.requests);

  return ret;
}

/** Win ErrHandler */

int PMPI_Win_set_errhandler(MPI_Win, MPI_Errhandler);
int PMPI_Win_get_errhandler(MPI_Win, MPI_Errhandler *);
int PMPI_Win_call_errhandler(MPI_Win win, int errorcode);
int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *, MPI_Errhandler *);

/* Win Ops */

int PMPI_Win_shared_query(MPI_Win, int, MPI_Aint *, int *, void *);
