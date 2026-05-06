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
 * OpenSHMEM internal ELF parser implementation
 */

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpc_common_debug.h>
#include <mpc_common_flags.h>
#include <sctk_alloc.h>

#include "mpc_openshmem_elf.h"

#ifdef MPC_MODULE
	#undef MPC_MODULE
#endif
#define MPC_MODULE "OpenSHMEM/elf"

/** @ingroup opnenshmem_internals
 * @brief Retrieve the wanted information from a 32 bits Elf
 *
 * @note This function is unlikely to be called as 32 bits Elf tends to be pretty rare outside
 * embedded
 *
 * @param[out] info   Information retrieved by the function
 * @param[in]  fd     File descriptor of the opened binary to read from
 * @param[in]  header Elf Header already read
 */
__attribute__((cold))
static void __oshmem_get_elf32_info(_mpc_oshmem_section_info_t *const info, FILE *fd,
                                    const Elf32_Ehdr *const header)
{
	unsigned long read = 0;

	assert(header->e_shnum <= 64);
	Elf64_Shdr section_headers[64];

	// Get Section Header String table index
	unsigned char strtab_index = header->e_shstrndx;

	// Go to the Section Header Table
	fseek(fd, header->e_shoff, SEEK_SET);

	// Get every section descriptor
	for (unsigned char i = 0; i < header->e_shnum; i++)
	{
		read = fread(&section_headers[i], 1, header->e_shentsize, fd);
		assert(read == header->e_shentsize);
	}

	// Retrieve the Section Header String table
	char *string_table = sctk_malloc(section_headers[strtab_index].sh_size);
	fseek(fd, section_headers[strtab_index].sh_offset, SEEK_SET);
	read = fread(string_table, 1, section_headers[strtab_index].sh_size, fd);
	assert(read == section_headers[strtab_index].sh_size);

	// Here, we compare the name of the sections with the ones we are searching for
	for (unsigned char i = 0; i < header->e_shnum; i++)
	{
		const size_t name_index = section_headers[i].sh_name;
		mpc_common_nodebug("Section %2d, name: %16s, name_index: %3lu offset: %8d size: %8d\n",
			i, &(string_table[name_index]), name_index,
			section_headers[i].sh_offset, section_headers[i].sh_size);

		if (strcmp(&(string_table[name_index]), ".bss") == 0)
		{
			info->bss.address = (void *)section_headers[i].sh_addr;
			info->bss.size    = section_headers[i].sh_size;
		}
		else if (strcmp(&(string_table[name_index]), ".data") == 0)
		{
			info->data.address = (void *)section_headers[i].sh_addr;
			info->data.size    = section_headers[i].sh_size;
		}
	}

	sctk_free(string_table);
}

/** @ingroup opnenshmem_internals
 * @brief Retrieve the wanted information from a 64 bits Elf
 *
 * @param[out] info Information retrieved by the function
 * @param[in]  fd   File descriptor of the opened binary to read from
 */
__attribute__((hot))
static void __oshmem_get_elf64_info(_mpc_oshmem_section_info_t *const info, FILE *fd)
{
	unsigned long read = 0;

	// Go back to the beginning and get the well sized header
	fseek(fd, 0, SEEK_SET);
	Elf64_Ehdr header64;
	read = fread(&header64, 1, sizeof(header64), fd);
	mpc_common_nodebug("Read %10d B from the header\n", read);
	assert(read == sizeof(header64));

	assert(header64.e_shnum <= 64);
	Elf64_Shdr section_headers[64];

	// Get Section Header String table index
	unsigned char strtab_index = header64.e_shstrndx;

	// Go to the Section Header Table
	fseek(fd, header64.e_shoff, SEEK_SET);

	// Get every section descriptor
	for (unsigned char i = 0; i < header64.e_shnum; i++)
	{
		read = fread(&section_headers[i], 1, header64.e_shentsize, fd);
		assert(read == header64.e_shentsize);
	}

	// Retrieve the Section Header String table
	char *string_table = sctk_malloc(section_headers[strtab_index].sh_size);
	fseek(fd, section_headers[strtab_index].sh_offset, SEEK_SET);
	read = fread(string_table, 1, section_headers[strtab_index].sh_size, fd);
	assert(read == section_headers[strtab_index].sh_size);

	// Here, we compare the name of the sections with the ones we are searching for
	for (unsigned char i = 0; i < header64.e_shnum; i++)
	{
		const size_t name_index = section_headers[i].sh_name;
		mpc_common_nodebug("Section %2d, name: %16s, name_index: %3lu, offset: %8d size: %8d\n",
			i, &(string_table[name_index]), name_index,
			section_headers[i].sh_offset, section_headers[i].sh_size);

		if (strcmp(&(string_table[name_index]), ".bss") == 0)
		{
			info->bss.address = (void *)section_headers[i].sh_addr;
			info->bss.size    = section_headers[i].sh_size;
		}
		else if (strcmp(&(string_table[name_index]), ".data") == 0)
		{
			info->data.address = (void *)section_headers[i].sh_addr;
			info->data.size    = section_headers[i].sh_size;
		}
	}

	sctk_free(string_table);
}

void _mpc_oshmem_get_elf_info(_mpc_oshmem_section_info_t *const info)
{
	// First, we need to open the binary
	// NOTE: As explained in the documentation of the function, we are only interested in the main executable
	// This function does not work on dynamically linked library.
	// The support for it may be necessary for KRS and will be implemented in the future
	const char *const exename = mpc_common_get_flags()->exename;
	Elf32_Ehdr        header;
	FILE *            fd = fopen(exename, "r");

	if (fd == NULL)
	{
		mpc_common_debug_fatal("Enable to open the binary file\n");
	}

	int read = 0;

	// As we don't know the type of elf (32 or 64 bits), we default to 32 to retrieve this information
	read = fread(&header, 1, sizeof(Elf32_Ehdr), fd);
	mpc_common_nodebug("Read %10d B from the header\n", read);
	assert(read == sizeof(Elf32_Ehdr));

	// Most common case (I hope)
	if (header.e_ident[EI_CLASS] == ELFCLASS64)
	{
		__oshmem_get_elf64_info(info, fd);
	}

	// Here it is cold in theory
	else if (header.e_ident[EI_CLASS] == ELFCLASS32)
	{
		__oshmem_get_elf32_info(info, fd, &header);
	}

	// Even colder
	else if (header.e_ident[EI_CLASS] == ELFCLASSNONE)
	{
		mpc_common_debug_fatal("Invalid ELF class file");
	}

	fclose(fd);

	mpc_common_nodebug(
		"Retrieved infos: .bss_addr=%p, .bss_size=%lu, .data_start=%p, .data_size=%lu\n",
		info->bss.address, info->bss.size, info->data.address, info->data.size);
}
