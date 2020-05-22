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
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK___DEBUGGGER__
#define __SCTK___DEBUGGGER__

#ifdef __cplusplus
extern "C"
{
#endif

#include <mpc_common_debug.h>
#include <mpc_common_debugger.h>
#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

typedef union
{
	Elf32_Ehdr h32;
	Elf64_Ehdr h64;
} Elf_Ehdr;

typedef union
{
	Elf32_Shdr h32;
	Elf64_Shdr h64;
} Elf_Shdr;

typedef struct
{
	char *file;
	char *dir;
	char *absolute;
} file_t;

typedef struct
{
	char *dir;
} debug_info_t;

typedef struct
{
	unsigned long comp_dir_off;
	int comp_dir_direct;
} abbrev_t;

typedef struct elf_class_s
{
	char *name;
	FILE *fd;
	void ( *read_elf_header ) ( struct elf_class_s * );
	Elf_Ehdr header;
	void ( *read_elf_sections ) ( struct elf_class_s * );
	Elf_Shdr *sections;
	char *string_table;
	int symtab_idx;
	char *sym_table;
	int dynsymtab_idx;
	char *dynsym_table;
	void ( *read_elf_symbols ) ( struct elf_class_s * );
	void *symbols;
	void ( *read_elf_sym ) ( struct elf_class_s *, mpc_addr2line_t *, int );
	void *debug_line;
	unsigned long debug_line_size;
	void *debug_aranges;
	file_t *file_list;
	char **dir_list;
	int is_lib;

	void *debug_info;
	unsigned long debug_info_size;
	debug_info_t *dbg_list;
	int dbg_nb;

	void *debug_str;
	unsigned long debug_str_size;
} elf_class_t;

void sctk_read_elf_header_32 ( elf_class_t *c );
void sctk_read_elf_section_32 ( elf_class_t *c );
void sctk_read_elf_symbols_32 ( elf_class_t *c );
void sctk_read_elf_sym_32 ( elf_class_t *c, mpc_addr2line_t *ptrs, int nb );
void sctk_read_elf_header_64 ( elf_class_t *c );
void sctk_read_elf_section_64 ( elf_class_t *c );
void sctk_read_elf_symbols_64 ( elf_class_t *c );
void sctk_read_elf_sym_64 ( elf_class_t *c, mpc_addr2line_t *ptrs, int nb );
void sctk_vprint_backtrace ( const char *format, va_list ap );

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
