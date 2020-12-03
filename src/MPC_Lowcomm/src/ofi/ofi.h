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

#ifndef __MPC_LOWCOMM_OFI_H_
#define __MPC_LOWCOMM_OFI_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "ofi_types.h"
#include "lowcomm_config.h"

/* avoid header inclusion cycle */
typedef struct sctk_rail_info_s sctk_rail_info_t;

/**
 * @brief Decode protocol type from string to enum (for config)
 * 
 * @param value input value
 * @return enum _mpc_lowcomm_ofi_rm_type corresponding enum type
 */
enum _mpc_lowcomm_ofi_mode mpc_lowcomm_ofi_decode_mode(char *value);

/**
 * @brief Decode ressource management type from string to enum (for config)
 * 
 * @param value input value
 * @return enum _mpc_lowcomm_ofi_rm_type corresponding enum type
 */
enum _mpc_lowcomm_ofi_rm_type mpc_lowcomm_ofi_decode_rmtype(char *value);

/**
 * @brief Decode address vector type from string to enum (for config)
 * 
 * @param value input value
 * @return enum _mpc_lowcomm_ofi_av_type corresponding enum type
 */
enum _mpc_lowcomm_ofi_av_type mpc_lowcomm_ofi_decode_avtype(char *value);

/**
 * @brief Decode endpoint type from string to enum (for config)
 * 
 * @param value input value
 * @return enum _mpc_lowcomm_ofi_ep_type corresponding enum type
 */
enum _mpc_lowcomm_ofi_ep_type mpc_lowcomm_ofi_decode_eptype(char *value);

/**
 * @brief Decode progress type from string to enum (for config)
 * 
 * @param value input value
 * @return enum _mpc_lowcomm_ofi_progress corresponding enum type
 */
enum _mpc_lowcomm_ofi_progress mpc_lowcomm_ofi_decode_progress(char *value);

#ifdef __cplusplus
}
#endif

void sctk_network_init_ofi( sctk_rail_info_t *rail );

#endif