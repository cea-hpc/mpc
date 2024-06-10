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

#ifndef SCTK_RDMA_H
#define SCTK_RDMA_H

#include "mpc_lowcomm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This is the MPC low-level RDMA interface defining both windows
 * and operations on remote memory areas this interface has been
 * designed to be a support for an MPI one-sided layer hence
 * some desing choices are similar (for example in offsetting).
 */

/************************************************************************/
/* Definition of an RDMA window                                         */
/************************************************************************/

/** This defines the window interface, any RDMA operation needs
 * to be done on a window in the remote process. Indeed, most RMA
 * networks suppose that the remote memory is registered to comply
 * with security issues, making the use of windows compulsory to
 * fall in this semantic. In MPC windows can be manipulated in three
 * ways in function of their locality:
 *  - local : (in same UNIX process) operations are direct memory accesses
 *  - emulated : the network layer does not support RDMA then RMA are emulated
 *  - RDMA : the network support RDMA then the HCA is called
 *
 * This switching is fully automatic in MPC also note that some HCA
 * have particular constraints on RDMA capable types MPC might then
 * emulate some RMA depending on the target type be careful to use
 * the right data-type in order to take advantage of your hardware
 */

/** Initialize a local window for RMA
 * @arg addr Start address of the window
 * @arg size Size of the region ot be mapped
 * @arg disp_unit Offsetting unit (used to compute displacements)
 * @arg comm Communicator used by this window (MPC_COMM_WORLD is a godd default)
 *
 * @return The ID of the newly created window (is an integer)
 */
mpc_lowcomm_rdma_window_t mpc_lowcomm_rdma_window_init( void *addr, size_t size, size_t disp_unit, mpc_lowcomm_communicator_t comm );

/** Map to a remote window in order to initiate RDMAs
 *
 * When mapping to a remote window you need to know the ID
 * which can be exchanged using point ot point messages MPI_INT
 * also be careful that a new window is created and that operations
 * on this new window are actually acting on the remote memory.
 *
 * @arg remote_rank ID of the remote process hosting the target window
 * @arg comm The communicator to be used
 * @arg win_id ID of the remote window to be mapped to
 *
 * @return a NEW window ID mirrorring the remote window on which operations are
 * remote
 */

mpc_lowcomm_rdma_window_t mpc_lowcomm_rdma_window_map_remote(int remote_rank, mpc_lowcomm_communicator_t comm,
                                     mpc_lowcomm_rdma_window_t win_id);

/** Release a window (either remote or local)
 *
 * Note that windows are using a ref-counting mechanism
 * meaning that the remote area is dereferrenced only
 * when all the mapped windows are released however
 * in all cases the programmer is in charge of the validity
 * of the mapped segment.
 *
 * @arg win ID of the window to be released
 */
void mpc_lowcomm_rdma_window_release( mpc_lowcomm_rdma_window_t win );

/** Emit a fence on a window
 *
 * Note that in most case this call does nothing
 * as the wait on RDMA is sufficent to gurantee data-coherency
 * at the granularity of a single RMA. However for emulated
 * calls we need to make sure that all control messages
 * are processed by flussing the event queue this is the
 * role of this call. It is then good practice to call
 * it before switching between two epochs.
 *
 * @warning This call does not replace the waiting of individual calls
 *
 * @arg win_id The window to emit a fence on (generally remote)
 */
void mpc_lowcomm_rdma_window_RDMA_fence(mpc_lowcomm_rdma_window_t win_id, mpc_lowcomm_request_t *req);

/************************************************************************/
/* Window Wait Operation                                                */
/************************************************************************/

/*
 * RDMA Wait
 */

/** Wait an RDMA request
 *
 * @warning It is compulsory to wait ALL RDMA requests
 *
 * @arg request Request returned by an RMA call
 */
void mpc_lowcomm_rdma_window_RDMA_wait( mpc_lowcomm_request_t *request );

/************************************************************************/
/* Window RDMA Operations                                               */
/************************************************************************/

/*
 * RDMA Write
 */

/** These are the RDMA operations defined in the RDMA interface
 * all these operations are taking a request in parameter and it
 * is required to wait all the RMA operations this the most optimal
 * and fine-grained wait of guaranteeing data-coherency. Note that
 * the state of data (either local or remote) is undefined as long
 * as a given request has not been waited in order to be as close
 * as possible to the MPI window semantic, it is required to call
 * @ref mpc_lowcomm_rdma_window_RDMA_fence between each access epoch
 *
 * The use of the interface is then as follows:
 *
 * 1 - Init a window
 * 2 - Set/Read data
 * 3 - Emit RDMA
 * 4 - Wait individual RDMAs
 * 5 - Emit a fence on the window
 * 6 - Set/Read data
 * 7 - ...
 * 8 - Release the window
 * */

/** Write a contiguous data-segment in a window from an arbitrary address
 * @arg win_id ID of the target window
 * @arg src_addr Address from which to read the data
 * @arg size Size of the data to be read
 * @arg dest_offset Offset in the target window (note that it is dependent from the disp_offset given at window initialization)
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_write( mpc_lowcomm_rdma_window_t win_id, void * src_addr, size_t size, size_t dest_offset, mpc_lowcomm_request_t  * req  );

/** Write a contiguous data-segment in a window from another window
 *
 * @note This call is more optimal than the arbitrary address one
 * as the pinning process can be avoided. It is recommended to
 * use it when possible to achieve the best performance.
 *
 * @arg src_win_id ID of the source local window
 * @arg src_offset offset in the local source window (dependent from disp_unit)
 * @arg size Size of the data to be written (be careful of window size)
 * @arg dest_win_id the window to write to
 * @arg dest_offset the offset to write to in the dest window
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_write_win( mpc_lowcomm_rdma_window_t src_win_id, size_t src_offset, size_t size,  mpc_lowcomm_rdma_window_t dest_win_id, size_t dest_offset, mpc_lowcomm_request_t  * req  );

/*
 * RDMA Read
 */

/** Read data from a remote window to an arbitrary address
 *
 * @arg win_id ID of the window to read from
 * @arg dest_addr Address where to write the data (arbitrary)
 * @arg size Size of the data to be read
 * @arg src_offset Offset in the target window where to read from (function of disp_unit)
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_read( mpc_lowcomm_rdma_window_t win_id, void * dest_addr, size_t size, size_t src_offset, mpc_lowcomm_request_t  * req  );

/** Read data from a window to a window
 *
 * @note This call is more optimal than the arbitrary address one
 * as the pinning process can be avoided. It is recommended to
 * use it when possible to achieve the best performance.
 *
 * @arg src_win_id ID of the window to read from
 * @arg src_offset Offset in the window to read from (function of disp_unit)
 * @arg size Size to read from the window
 * @arg dest_win ID of the destination window
 * @arg dest_offset Dest offset in the destination window
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_read_win( mpc_lowcomm_rdma_window_t src_win_id, size_t src_offset, size_t size, mpc_lowcomm_rdma_window_t dest_win_id, size_t dest_offset, mpc_lowcomm_request_t  * req );

/************************************************************************/
/* Window Atomic Operations                                             */
/************************************************************************/

/** This defines the atomic operations all these atomic operations are
 * typed with the following common data-types ( @ref RDMA_type ):
  	RDMA_TYPE_CHAR
	RDMA_TYPE_DOUBLE
	RDMA_TYPE_FLOAT
	RDMA_TYPE_INT
	RDMA_TYPE_LONG
	RDMA_TYPE_LONG_DOUBLE
	RDMA_TYPE_LONG_LONG
	RDMA_TYPE_LONG_LONG_INT
	RDMA_TYPE_SHORT
	RDMA_TYPE_SIGNED_CHAR
	RDMA_TYPE_UNSIGNED
	RDMA_TYPE_UNSIGNED_CHAR
	RDMA_TYPE_UNSIGNED_LONG
	RDMA_TYPE_UNSIGNED_LONG_LONG
	RDMA_TYPE_UNSIGNED_SHORT
	RDMA_TYPE_WCHAR

* Care must be taken to match the data-type with the actual memory
* region targetted otherwise leading to memory corruptions. As for
* READ and WRITE there requests exist in two versions one with
* arbitrary address and the other in win to win yielding better
* performance (avoiding memory registration overhead).
*/

/*
 * RDMA Fetch and op
 */

/** Emit an RDMA fetch and op in a remote memory window with an
 * arbitrary fetch address
 *
 * @note Feth and op returns the original memory segment (in fetch_addr)
 * then atomically accumulate the data segment (add) using the
 * provided operation (Op) the size being given by the type.
 * Supported operations are the following:
	RDMA_SUM  -> The region is the addition of local and add
	RDMA_INC  -> The target is incremented by 1
	RDMA_DEC  -> The target is decremented by 1
	RDMA_MIN  -> The target is replaced if add is lower than target
	RDMA_MAX  -> The target is replaced is add is higher than target
	RDMA_PROD -> The target is multiplied by add
	RDMA_LAND -> The target goes through a logical and with add
	RDMA_BAND -> The target goes through a bitwise and with add
	RDMA_LOR -> The target goes through a logical or with add
	RDMA_BOR -> The target goes through a bitwise or with add
	RDMA_LXOR -> The target goes through a logical xor with add
	RDMA_BXOR -> The target goes through a bitwise xor with add
 *
 * @arg remote_win_id Remote window id where to operate
 * @arg remote_offset Offset in the remote window (function of disp_unit)
 * @arg fetch_addr Arbitrary fetch address where the original data is written
 * @arg add Pointer to a segment of @ref type where to read the operand
 * @arg op Operation to be carried on the target with add
 * @arg type Type of the @ref fetch_addr, window target and @ref add segments
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_fetch_and_op( mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void * fetch_addr, void * add, RDMA_op op,  RDMA_type type, mpc_lowcomm_request_t  * req );

/** Emit an RDMA fetch and op to a window with a fetch address in another window
 *
 * @note This call is more optimal than the arbitrary address one
 * as the pinning process can be avoided. It is recommended to
 * use it when possible to achieve the best performance.
 *
 * @arg remote_win_id Window where to operate
 * @arg remote_offset  Offset in the remote window (function of disp_unit)
 * @arg local_win_id Local window where to read from
 * @arg fetch_offset Local offset where to fetch the remote data  (dependent from disp_unit)
 * @arg add Pointer to the local data to be added
 * @arg op Operation to be done with add on the target
 * @arg type Type of the operands (fetch, remote and add)
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_fetch_and_op_win( mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, mpc_lowcomm_rdma_window_t local_win_id, size_t fetch_offset, void * add, RDMA_op op,  RDMA_type type, mpc_lowcomm_request_t  * req );

/*
 * RDMA Cas
 */

/** Emit a compare and swap (CAS) on a remote window with an arbitrary fetch address
 *
 * @note A compare an swap replaces the target data is it is equal to the
 * comp value while returning the original data to the caller. The data
 * are typed, the size of the operand being derived from this types
 *
 * @arg remote_win_id ID of the target window where to do the CAS
 * @arg remote_offset Offset in the target window  (dependent from disp_unit)
 * @arg comp Pointer to the memory region to compare
 * @arg new_data Replacement data if comp matches
 * @arg res Pointer to a memory region where the original target memory segment is stored
 * @arg type Type of the operants (res, new_data, target)
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_CAS( mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset, void * comp, void * new_data, void  * res, RDMA_type type, mpc_lowcomm_request_t  * req );

/** Emit a compare and swap (CAS) on a remote window with a fetch address
 * in a local window
 *
 * @note This call is more optimal than the arbitrary address one
 * as the pinning process can be avoided. It is recommended to
 * use it when possible to achieve the best performance.
 *
 * @arg remote_win_id ID of the target window where to do the CAS
 * @arg remote_offset Offset in the target window
 * @arg local_win_id Id of the local window to fetch to
 * @arg res_offset Offset where to store the original data (dependent from disp_unit)
 * @arg comp Pointer to the memory region to compare
 * @arg new_data Replacement data if comp matches
 * @arg type Type of the operants (res, new_data, target)
 * @arg req A request to wait the RMA with @ref mpc_lowcomm_rdma_window_RDMA_wait
 */
void mpc_lowcomm_rdma_window_RDMA_CAS_win( mpc_lowcomm_rdma_window_t remote_win_id, size_t remote_offset,  mpc_lowcomm_rdma_window_t local_win_id, size_t res_offset, void * comp, void * new_data, RDMA_type type, mpc_lowcomm_request_t  * req );

#ifdef __cplusplus
}
#endif

#endif /* SCTK_RDMA_H */
