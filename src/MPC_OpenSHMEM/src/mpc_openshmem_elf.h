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
/* #   - BEAULIEU Corentin corentin.beaulieu@cea.fr                       # */
/* #                                                                      # */
/* ######################################################################## */

/**@file
 * OpenSHMEM internal ELF parser API
 */

#ifndef OSHMEM_ELF_H_
#define OSHMEM_ELF_H_
#include <stddef.h>

/** @defgroup openshmem_internals INTERNALS
 * @ingroup MPC_OPENSHMEM
 * @{
 * @brief MPC OpenSHMEM Internal datastructures and functions
 */

/** @brief Address and size of a Elf section */
typedef struct _mpc_oshmem_elf_info_s
{
	void * address; /**< Address of the first entry in the section */
	size_t size;    /**< Size of the all section */
} _mpc_oshmem_elf_info_t;

/** @brief Address and size of the needed sections*/
typedef struct _mpc_oshmem_section_info_s
{
	_mpc_oshmem_elf_info_t bss;  /**< Information on the .bss section */
	_mpc_oshmem_elf_info_t data; /**< Information on the .data section */
} _mpc_oshmem_section_info_t;

/** @brief Return information on the needed section from the user binary
 *
 * This function parse the Elf of the "exename" (@e i.g. the user executable).
 *
 * @note It only works on the main executable
 *
 * @important If the OpenSHMEM library is called from a dynamically linked
 *	library it won't work as expected
 *
 * @param[out] section_info Information retrieved by the function
 */
void _mpc_oshmem_get_elf_info(_mpc_oshmem_section_info_t *const section_info);

/** @} */  // openshmem_internals

#endif // OSHMEM_ELF_H_
