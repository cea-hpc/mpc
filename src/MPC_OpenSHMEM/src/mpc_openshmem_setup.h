#pragma once
#include <stddef.h>

/** @defgroup openshmem_internals INTERNALS
 * @ingroup MPC_OPENSHMEM
 * @{
 * @brief MPC OpenSHMEM Internal datastructures and functions
 */

/** @brief Address and size of a Elf section */
typedef struct _mpc_openshmem_elf_info_s
{
	void * address; /**< Address of the first entry in the section */
	size_t size;    /**< Size of the all section */
} _mpc_openshmem_elf_info_t;

/** @brief Address and size of the needed sections*/
typedef struct _mpc_openshmem_section_info_s
{
	_mpc_openshmem_elf_info_t bss;  /**< Information on the .bss section */
	_mpc_openshmem_elf_info_t data; /**< Information on the .data section */
} _mpc_openshmem_section_info_t;

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
void _mpc_openshmem_get_elf_info(_mpc_openshmem_section_info_t *const section_info);

/** @} */  // openshmem_internals
