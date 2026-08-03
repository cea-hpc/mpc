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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

/**@file
 * Public API of the MPC MPI one sided communications routines.
 * It should be the only header needed to use the top level abstraction of OSC.
 */

#ifndef MPC_MPI_OSC_H
#define MPC_MPI_OSC_H

#include <mpc_mpi.h>                  // For MPI_MAX_OBJECT_NAME, MPI_Info...
#include "mpc_mpi_internal.h"         // For MPI_internal_request_t

/** @defgroup MPI_OSC MPI One-Sided MPC API
 * @{
 * @ingroup MPC_MPI
 * @brief API of the MPI module to perform RMA operations
 * @}
 */

/**@ingroup MPI_OSC
 * MPI Window opaque handle */
typedef struct mpc_mpi_win_s *mpc_mpi_win_h;

/*********************
 * Window Management *
 *********************/

/**@ingroup MPI_OSC
 * @brief Creates a window from a user provided buffer
 *
 * @param[in]  base      Address of the underlying buffer
 * @param[in]  size      Minimum size requested for the window
 * @param[in]  disp_unit Displacement unit to use in the window
 * @param[in]  info      Info object to store in the window at creation
 * @param[in]  comm      Communicator associated with the created window
 * @param[out] win_p     Pointer to store the resulting handle on the created window
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_win_create(void **base, const size_t size, const int disp_unit, MPI_Info info,
                           mpc_lowcomm_communicator_t comm, mpc_mpi_win_h *win_p);

/**@ingroup MPI_OSC
 * @brief Creates a window by allocating a buffer for the user
 *
 * @param[in]  base      Address of the allocated buffer
 * @param[in]  size      Minimum size requested for the window
 * @param[in]  disp_unit Displacement unit to use in the window
 * @param[in]  info      Info object to store in the window at creation
 * @param[in]  comm      Communicator associated with the created window
 * @param[out] win_p     Pointer to store the resulting handle on the created window
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_win_allocate(void **base, const size_t size, const int disp_unit, MPI_Info info,
                             mpc_lowcomm_communicator_t comm, mpc_mpi_win_h *win_p);

/**@ingroup MPI_OSC
 * @brief Creates an empty dynamic window
 *
 * @param[in]  info  Info object to store in the window at creation
 * @param[in]  comm  Communicator associated with the created window
 * @param[out] win_p Pointer to store the resulting handle on the created window
 *
 * @return           MPI errorcode
 */
int mpc_mpi_osc_win_create_dynamic(MPI_Info info, mpc_lowcomm_communicator_t comm, mpc_mpi_win_h *win_p);

/**@ingroup MPI_OSC
 * @brief Frees a window
 *
 * @param[in] win Handle on the window to free
 *
 * @return        MPI errorcode
 */
int mpc_mpi_osc_win_free(mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Attaches a buffer to a dynamic window
 *
 * @warning Trying to attach memory to a created or allocated window will fail
 *
 * @param[in] win  Handle of the window the memory will be attached to
 * @param[in] base Address of the buffer to attach
 * @param[in] len  Size of the buffer to attach
 *
 * @return         MPI errorcode
 */
int mpc_mpi_osc_win_attach(mpc_mpi_win_h win, void *base, const size_t len);

/**@ingroup MPI_OSC
 * @brief Attaches a buffer to a dynamic window
 *
 * @warning Trying to detach memory from a created or allocated window will fail
 *
 * @param[in] win  Handle of the window the memory will be attached to
 * @param[in] base Address of the buffer to attach
 * @param[in] len  Size of the buffer to attach
 *
 * @return         MPI errorcode
 */
int mpc_mpi_osc_win_detach(mpc_mpi_win_h win, const void *base);


/******************
 * Communications *
 ******************/

/**@ingroup MPI_OSC
 * @brief Performs a put one-sided operation
 *
 * @param[in] origin_addr  Address of the buffer to read from
 * @param[in] origin_count Number of elements to read at the origin
 * @param[in] origin_dt    Datatype of the elements to read at the origin
 * @param[in] target       Rank of the target task
 * @param[in] target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in] target_count Number of elements to write at the target
 * @param[in] target_dt    Datatype of the elements to write at the target
 * @param[in] win          Handle of the window used for the communication
 *
 * @return                 MPI errorcode
 */
int mpc_mpi_osc_put(const void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                    const int target, const ptrdiff_t target_disp, const int target_count,
                    mpc_lowcomm_datatype_t target_dt, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Performs a get one-sided operation
 *
 * @param[in] origin_addr  Address of the buffer to write to
 * @param[in] origin_count Number of elements to write at the origin
 * @param[in] origin_dt    Datatype of the elements to write at the origin
 * @param[in] target       Rank of the target task
 * @param[in] target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in] target_count Number of elements to read at the target
 * @param[in] target_dt    Datatype of the elements to read at the target
 * @param[in] win          Handle of the window used for the communication
 *
 * @return                 MPI errorcode
 */
int mpc_mpi_osc_get(void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                    const int target, const ptrdiff_t target_disp, const int target_count,
                    mpc_lowcomm_datatype_t target_dt, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Performs an accumulate one-sided operation
 *
 * @param[in] origin_addr  Address of the buffer to read from
 * @param[in] origin_count Number of elements to read at the origin
 * @param[in] origin_dt    Datatype of the elements to read at the origin
 * @param[in] target       Rank of the target task
 * @param[in] target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in] target_count Number of elements to write at the target
 * @param[in] target_dt    Datatype of the elements to write at the target
 * @param[in] op           Operation to execute
 * @param[in] win          Handle of the window used for the communication
 *
 * @return                 MPI errorcode
 */
int mpc_mpi_osc_accumulate(const void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                           const int target, const ptrdiff_t target_disp, const int target_count,
                           mpc_lowcomm_datatype_t target_dt, MPI_Op op, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Performs a get and accumulate one-sided operation
 *
 * @param[in] origin_addr  Address of the buffer to read from
 * @param[in] origin_count Number of elements to read at the origin
 * @param[in] origin_dt    Datatype of the elements to read at the origin
 * @param[in] result_addr  Address of the buffer to write to
 * @param[in] result_count Number of elements to write at the origin
 * @param[in] result_dt    Datatype of the elements to write at the origin
 * @param[in] target       Rank of the target task
 * @param[in] target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in] target_count Number of elements to write at the target
 * @param[in] target_dt    Datatype of the elements to write at the target
 * @param[in] op           Operation to execute
 * @param[in] win          Handle of the window used for the communication
 *
 * @return                 MPI errorcode
 */
int mpc_mpi_osc_get_accumulate(const void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                               void *result_addr, const int result_count, mpc_lowcomm_datatype_t result_dt,
                               const int target_rank, const ptrdiff_t target_disp, const int target_count,
                               mpc_lowcomm_datatype_t target_dt, MPI_Op op, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Performs an atomic compare and swap one-sided operation
 *
 * @param[in] origin_addr  Address of the buffer to read from to replace
 * @param[in] compare_addr Address of the buffer to read from to have the compare element
 * @param[in] result_addr  Address of the buffer to write to
 * @param[in] dt           Datatype of the element
 * @param[in] target       Rank of the target task
 * @param[in] target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in] win          Handle of the window used for the communication
 *
 * @return                 MPI errorcode
 */
int mpc_mpi_osc_compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr,
                                 mpc_lowcomm_datatype_t dt, const int target, const ptrdiff_t target_disp,
                                 mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Performs an atomic fetch and operation one-sided operation
 *
 * @param[in] origin_addr Address of the buffer to read from to replace
 * @param[in] result_addr Address of the buffer to write to
 * @param[in] dt          Datatype of the element
 * @param[in] target      Rank of the target task
 * @param[in] target_disp Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in] op          Operation to execute
 * @param[in] win         Handle of the window used for the communication
 *
 * @return                MPI errorcode
 */
int mpc_mpi_osc_fetch_and_op(const void *origin_addr, void *result_addr, mpc_lowcomm_datatype_t dt,
                             const int target, const ptrdiff_t target_disp, MPI_Op op, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Performs a put one-sided operation with a request to be completed
 *
 * @param[in]  origin_addr  Address of the buffer to read from
 * @param[in]  origin_count Number of elements to read at the origin
 * @param[in]  origin_dt    Datatype of the elements to read at the origin
 * @param[in]  target       Rank of the target task
 * @param[in]  target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in]  target_count Number of elements to write at the target
 * @param[in]  target_dt    Datatype of the elements to write at the target
 * @param[in]  win          Handle of the window used for the communication
 * @param[out] req          Request associated with the communication
 *
 * @return                  MPI errorcode
 */
int mpc_mpi_osc_rput(const void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                     const int target, const ptrdiff_t target_disp, const int target_count,
                     mpc_lowcomm_datatype_t target_dt, mpc_mpi_win_h win, MPI_internal_request_t **req);

/**@ingroup MPI_OSC
 * @brief Performs a get one-sided operation with a request to be completed
 *
 * @param[in]  origin_addr  Address of the buffer to write to
 * @param[in]  origin_count Number of elements to write at the origin
 * @param[in]  origin_dt    Datatype of the elements to write at the origin
 * @param[in]  target       Rank of the target task
 * @param[in]  target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in]  target_count Number of elements to read at the target
 * @param[in]  target_dt    Datatype of the elements to read at the target
 * @param[in]  win          Handle of the window used for the communication
 * @param[out] req          Request associated with the communication
 *
 * @return                  MPI errorcode
 */
int mpc_mpi_osc_rget(void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                     const int target, const ptrdiff_t target_disp, const int target_count,
                     mpc_lowcomm_datatype_t target_dt, mpc_mpi_win_h win, MPI_internal_request_t **req);

/**@ingroup MPI_OSC
 * @brief Performs an accumulate one-sided operation with a request to be completed
 *
 * @param[in]  origin_addr  Address of the buffer to read from
 * @param[in]  origin_count Number of elements to read at the origin
 * @param[in]  origin_dt    Datatype of the elements to read at the origin
 * @param[in]  target       Rank of the target task
 * @param[in]  target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in]  target_count Number of elements to write at the target
 * @param[in]  target_dt    Datatype of the elements to write at the target
 * @param[in]  op           Operation to execute
 * @param[in]  win          Handle of the window used for the communication
 * @param[out] req          Request associated with the communication
 *
 * @return                  MPI errorcode
 */
int mpc_mpi_osc_raccumulate(const void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                            const int target, const ptrdiff_t target_disp, const int target_count,
                            mpc_lowcomm_datatype_t target_dt, MPI_Op op,
                            mpc_mpi_win_h win, MPI_internal_request_t **request);

/**@ingroup MPI_OSC
 * @brief Performs a get and accumulate one-sided operation with a request to be completed
 *
 * @param[in]  origin_addr  Address of the buffer to read from
 * @param[in]  origin_count Number of elements to read at the origin
 * @param[in]  origin_dt    Datatype of the elements to read at the origin
 * @param[in]  result_addr  Address of the buffer to write to
 * @param[in]  result_count Number of elements to write at the origin
 * @param[in]  result_dt    Datatype of the elements to write at the origin
 * @param[in]  target       Rank of the target task
 * @param[in]  target_disp  Offset at the target location (multiplied by the disp_unit of the target)
 * @param[in]  target_count Number of elements to write at the target
 * @param[in]  target_dt    Datatype of the elements to write at the target
 * @param[in]  op           Operation to execute
 * @param[in]  win          Handle of the window used for the communication
 * @param[out] req          Request associated with the communication
 *
 * @return                  MPI errorcode
 */
int mpc_mpi_osc_rget_accumulate(const void *origin_addr, const int origin_count, mpc_lowcomm_datatype_t origin_dt,
                                void *result_addr, const int result_count, mpc_lowcomm_datatype_t result_dt,
                                const int target_rank, const ptrdiff_t target_disp, const int target_count,
                                mpc_lowcomm_datatype_t target_dt, MPI_Op op,
                                mpc_mpi_win_h win, MPI_internal_request_t **request);


/******************
 * Passive Target *
 ******************/

/**@ingroup MPI_OSC
 * @brief Locks a window either to be share or to have exclusive access
 *
 * @param[in] lock_type  Type of locking to perform (SHARED or EXCLUSIVE)
 * @param[in] target     Rank of the target task
 * @param[in] mpi_assert Special bitfield to atler the lock behaviour
 * @param[in] win        Handle of the window used
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_lock(const int lock_type, const int target, const int mpi_assert, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Unlocks a window previously locked
 *
 * @param[in] target Rank of the target task
 * @param[in] win    Handle of the window used
 *
 * @return           MPI errorcode
 */
int mpc_mpi_osc_unlock(const int target, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Locks all the target in the window for shared access
 *
 * @param[in] mpi_assert Special bitfield to atler the lock behaviour
 * @param[in] win        Handle of the window used
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_lock_all(const int mpi_assert, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Unlocks all the target in the window
 *
 * @note The window should be locked with @ref mpc_mpi_osc_lock_all
 *
 * @param[in] win Handle of the window used
 *
 * @return        MPI errorcode
 */
int mpc_mpi_osc_unlock_all(mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Synchronizes the outstanding communications with the network
 *
 * @param[in] win Handle of the window used
 *
 * @return        MPI errorcode
 */
int mpc_mpi_osc_sync(mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Flushes the outstanding communications
 *
 * @note This routine assess local and distant completion of the communications upon return
 *
 * @param[in] target Rank of the target task
 * @param[in] win    Handle of the window used
 *
 * @return           MPI errorcode
 */
int mpc_mpi_osc_flush(const int target, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Flushes the outstanding communications for all targets in the window
 *
 * @note This routine assess local and distant completion of the communications upon return
 *
 * @param[in] win Handle of the window used
 *
 * @return        MPI errorcode
 */
int mpc_mpi_osc_flush_all(mpc_mpi_win_h win);


/*****************
 * Active Target *
 *****************/

/**@ingroup MPI_OSC
 * @brief Synchronizes the target in the window
 *
 * @note This routine assess local and distant completion of the communications upon return
 *
 * @param[in] mpi_assert Special bitfield to atler the lock behaviour
 * @param[in] win        Handle of the window used
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_fence(const int mpi_assert, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Opens an access epoch on the specified group
 *
 * @param[in] group      Group on which the access epoch will be opened
 * @param[in] mpi_assert Special bitfield to atler the lock behaviour
 * @param[in] win        Handle of the window used
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_start(mpc_lowcomm_group_t *group, const int mpi_assert, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Closes an access epoch on the specified group
 *
 * @note This routine assess local completion of the communications upon return
 *
 * @param[in] win Handle of the window used
 *
 * @return        MPI errorcode
 */
int mpc_mpi_osc_complete(mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Opens an exposure epoch on the specified group
 *
 * @param[in] group      Group on which the exposure epoch will be opened
 * @param[in] mpi_assert Special bitfield to atler the lock behaviour
 * @param[in] win        Handle of the window used
 *
 * @return               MPI errorcode
 */
int mpc_mpi_osc_post(mpc_lowcomm_group_t *group, const int mpi_assert, mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Closes an exposure epoch on the specified group
 *
 * @note This routine assess distant completion of the communications upon return
 *
 * @param[in] win Handle of the window used
 *
 * @return        MPI errorcode
 */
int mpc_mpi_osc_wait(mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Tries to close an exposure epoch on the specified group
 *
 * @param[in]  win  Handle of the window used
 * @param[out] flag Success status (1 if the epoch was closed)
 *
 * @return          MPI errorcode
 */
int mpc_mpi_osc_test(mpc_mpi_win_h win, int *flag);


/**************
 * Attributes *
 **************/

/**@ingroup
 * @brief Generates a new entry in the attribute table
 *
 * @param[in]  copy_fn     Copy function of the new attribute
 * @param[in]  delete_fn   Deletion function of the new attribute
 * @param[out] keyval      Identifier of the new attribute
 * @param[in]  extra_state Extra information to store in the new attribute
 *
 * @return                 MPI errorcode
 */
int mpc_mpi_osc_win_create_keyval(MPI_Win_copy_attr_function *copy_fn, MPI_Win_delete_attr_function *delete_fn,
                                  int *keyval, void *extra_state);

/**@ingroup
 * @brief Modifies an attribute stored in a window
 *
 * @param[in] win      Handle of the window
 * @param[in] keyval   Identifier of the attribute to modify
 * @param[in] attr_val New value of the attribute
 *
 * @return             MPI errorcode
 */
int mpc_mpi_osc_win_set_attr(mpc_mpi_win_h win, const int keyval, void *const attr_val);

/**@ingroup
 * @brief Retrieves an attribute from a window
 *
 * @param[in]  win      Handle of the window
 * @param[in]  keyval   Identifier of the attribute
 * @param[out] attr_val Pointer to store this attribute
 * @param[out] flag     Success status (1 if the attribute exists and has been retrieved)
 *
 * @return              MPI errorcode
 */
int mpc_mpi_osc_win_get_attr(mpc_mpi_win_h win, const int keyval, void *attr_val, int *flag);

/**@ingroup
 * @brief Removes an attribute from a window
 *
 * @param[in] win    Handle of the window
 * @param[in] keyval Identifier of the attribute to remove
 *
 * @return           MPI errorcode
 */
int mpc_mpi_osc_win_delete_attr(mpc_mpi_win_h win, const int keyval);

/**@ingroup
 * @brief Removes an entry from the attribute table
 *
 * @param[in] keyval Identifier of the entry to remove
 *
 * @return           MPI errorcode
 */
int mpc_mpi_osc_win_free_keyval(const int keyval);


/***********
 * Special *
 ***********/

/**@ingroup MPI_OSC
 * @brief Retrieves the communicator associated with a win
 *
 * @param[in] win Handle of the window to retrieve from
 *
 * @return        A reference on the communicator
 */
MPI_Comm mpc_mpi_osc_win_get_comm(const mpc_mpi_win_h win);

/**@ingroup MPI_OSC
 * @brief Retrieves the group associated with a win
 *
 * @param[in] win Handle of the window to retrieve from
 *
 * @return        A copy of the group
 */
MPI_Group mpc_mpi_osc_win_get_group(const mpc_mpi_win_h win);

#endif // MPC_MPI_OSC_H
