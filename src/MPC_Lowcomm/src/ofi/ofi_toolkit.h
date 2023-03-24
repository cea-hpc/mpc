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
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_LOWCOMM_OFI_TOOLKIT_H_
#define __MPC_LOWCOMM_OFI_TOOLKIT_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include <rail.h>
#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>
#include "ofi_types.h"
#include <mpc_lowcomm_monitor.h>

#define MPC_LOWCOMM_OFI_EQ_SIZE 64
#define MPC_LOWCOMM_OFI_CQ_SIZE 64
#define MPC_LOWCOMM_OFI_CQ_POLL_MAX 10
#define MPC_LOWCOMM_OFI_MAX_INTERLEAVE 16


void mpc_lowcomm_ofi_setup_hints_from_config(struct fi_info* hint, struct _mpc_lowcomm_config_struct_net_driver_ofi config);
void mpc_lowcomm_ofi_init_provider(mpc_lowcomm_ofi_rail_info_t* rail, struct fi_info* hint);
_mpc_lowcomm_endpoint_t* mpc_lowcomm_ofi_add_route(mpc_lowcomm_peer_uid_t dest, void* ctx, sctk_rail_info_t* rail, _mpc_lowcomm_endpoint_type_t origin, _mpc_lowcomm_endpoint_state_t state);

/**
 * @brief elect the proper fabric datatype from MPC datatype
 * 
 * @param type the MPC-specific datatype for RDMA
 * @return enum fi_datatype the libfabric datatype value
 */
static inline enum fi_datatype mpc_lowcomm_ofi_convert_type(RDMA_type type)
{
	switch(type)
	{
		case RDMA_TYPE_CHAR               : return FI_INT8      ; break ;
		case RDMA_TYPE_DOUBLE             : return FI_DOUBLE      ; break ;
		case RDMA_TYPE_FLOAT              : return FI_FLOAT       ; break ;
		case RDMA_TYPE_INT                : return FI_INT32     ; break ;
		case RDMA_TYPE_LONG_DOUBLE        : return FI_LONG_DOUBLE ; break ;
		case RDMA_TYPE_LONG               : return FI_INT64     ; break ;
		case RDMA_TYPE_LONG_LONG          : return FI_INT64     ; break ;
		case RDMA_TYPE_LONG_LONG_INT      : return FI_INT64     ; break ;
		case RDMA_TYPE_SHORT              : return FI_INT16     ; break ;
		case RDMA_TYPE_SIGNED_CHAR        : return FI_INT8      ; break ;
		case RDMA_TYPE_UNSIGNED_CHAR      : return FI_UINT8     ; break ;
		case RDMA_TYPE_UNSIGNED           : return FI_UINT32    ; break ;
		case RDMA_TYPE_UNSIGNED_LONG      : return FI_UINT64    ; break ;
		case RDMA_TYPE_UNSIGNED_LONG_LONG : return FI_UINT64    ; break ;
		case RDMA_TYPE_UNSIGNED_SHORT     : return FI_UINT16    ; break ;
		case RDMA_TYPE_WCHAR              : return FI_INT16     ; break ;
		default: 
			mpc_common_debug_fatal("Type not handled by OFI: %d", type);
	}
	return 0;
}

/**
 * Convert an MPC operation to libfabric one.
 * \param[in] op the MPC op
 * \return the libfabric op
 */
static inline enum fi_op mpc_lowcomm_ofi_convert_op(RDMA_op op)
{
	switch(op)
	{
		case RDMA_SUM  : return FI_SUM  ; break ;
		case RDMA_MIN  : return FI_MIN  ; break ;
		case RDMA_MAX  : return FI_MAX  ; break ;
		case RDMA_PROD : return FI_PROD ; break ;
		case RDMA_LAND : return FI_LAND ; break ;
		case RDMA_BAND : return FI_BAND ; break ;
		case RDMA_LOR  : return FI_LOR  ; break ;
		case RDMA_BOR  : return FI_BOR  ; break ;
		case RDMA_LXOR : return FI_LXOR ; break ;
		case RDMA_BXOR : return FI_BXOR ; break ;
		default:
			mpc_common_debug_fatal("Operation not supported by OFI %d", op);
	}
	return 0;
}

/**
 * Check if the given operation is unary (for instance INC | DEC).
 * If so, prepare a buffer with the adequat increment/decrement.
 * \param[in] op the MPC RDMA operation
 * \param[in] type the MPC RDMA type
 * \param[out] buf the buffer to store the increment/decrement.
 * \param[out] size buf size.
 */
static inline short mpc_lowcomm_ofi_is_unary_op(RDMA_op op, RDMA_type type, char* buf, size_t size)
{
	if(op != RDMA_INC && op != RDMA_DEC)
	{
		return 0;
	}

	assert(buf);
	assert(size >= RDMA_type_size(type));
	
	memset(buf, 0, size);
	if(op == RDMA_INC)
	{
		switch(type)
		{
			case RDMA_TYPE_CHAR:
			case RDMA_TYPE_UNSIGNED_CHAR:
			case RDMA_TYPE_SIGNED_CHAR:
			{
				int8_t x = 1;
				memcpy(buf, &x, sizeof(int8_t));
				break;
			}
			case RDMA_TYPE_SHORT:
			case RDMA_TYPE_WCHAR:
			case RDMA_TYPE_UNSIGNED_SHORT:
			{
				int16_t x = 1;
				memcpy(buf, &x, sizeof(int16_t));
				break;
			}
			case RDMA_TYPE_DOUBLE:
			{
				double x = 1.0;
				memcpy(buf, &x, sizeof(double));
				break;
			}
			case RDMA_TYPE_FLOAT:
			{
				float x = 1.0;
				memcpy(buf, &x, sizeof(float));
				break;
			}
			case RDMA_TYPE_INT:
			case RDMA_TYPE_UNSIGNED:
			{
				int32_t x = 1;
				memcpy(buf, &x, sizeof(int32_t));
				break;
			}
			case RDMA_TYPE_LONG_LONG:
			case RDMA_TYPE_LONG:
			case RDMA_TYPE_LONG_LONG_INT:
			case RDMA_TYPE_UNSIGNED_LONG:
			case RDMA_TYPE_UNSIGNED_LONG_LONG:
			{
				int64_t x = 1;
				memcpy(buf, &x, sizeof(int64_t));
				break;
			}
			case RDMA_TYPE_LONG_DOUBLE:
			{
				long double x = 1.0;
				memcpy(buf, &x, sizeof(long double));
				break;
			}
			default:
				not_reachable();
		}
	}
	else if( op == RDMA_DEC)
	{
		switch(type)
		{
			case RDMA_TYPE_CHAR:
			case RDMA_TYPE_UNSIGNED_CHAR:
			case RDMA_TYPE_SIGNED_CHAR:
			{
				int8_t x = -1;
				memcpy(buf, &x, sizeof(int8_t));
				break;
			}
			case RDMA_TYPE_SHORT:
			case RDMA_TYPE_WCHAR:
			case RDMA_TYPE_UNSIGNED_SHORT:
			{
				int16_t x = -1;
				memcpy(buf, &x, sizeof(int16_t));
				break;
			}
			case RDMA_TYPE_DOUBLE:
			{
				double x = -1.0;
				memcpy(buf, &x, sizeof(double));
				break;
			}
			case RDMA_TYPE_FLOAT:
			{
				float x = -1.0;
				memcpy(buf, &x, sizeof(float));
				break;
			}
			case RDMA_TYPE_INT:
			case RDMA_TYPE_UNSIGNED:
			{
				int32_t x = -1;
				memcpy(buf, &x, sizeof(int32_t));
				break;
			}
			case RDMA_TYPE_LONG_LONG:
			case RDMA_TYPE_LONG:
			case RDMA_TYPE_LONG_LONG_INT:
			case RDMA_TYPE_UNSIGNED_LONG:
			case RDMA_TYPE_UNSIGNED_LONG_LONG:
			{
				int64_t x = -1;
				memcpy(buf, &x, sizeof(int64_t));
				break;
			}
			case RDMA_TYPE_LONG_DOUBLE:
			{
				long double x = -1.0;
				memcpy(buf, &x, sizeof(long double));
				break;
			}
			default:
				not_reachable();
		}
	}
	return 1;
}

#ifdef __cplusplus
}
#endif
#endif
