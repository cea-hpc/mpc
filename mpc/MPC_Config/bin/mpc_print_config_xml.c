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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

/********************  HEADERS  *********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "sctk_debug.h"
#include "mpc_print_config_xml.h"
#include "sctk_runtime_config_walk.h"

/*******************  FUNCTION  *********************/
/**
 * Type for user handler function to call on each node of the configuration structure.
 * @param type Define the type of the node (union, struct, array, simple value).
 * @param name Define the name of the field.
 * @param type_name Define the type name of the value.
 * @param value Pointer to the value.
 * @param status Open/Close status.
 * @param type_meta Pointer to the meta description of the field value. NULL for simple values (int, float...).
 * @param level Inclusion level in the tree.
 * @param opt An optional pointer to transmit to the handler while calling it.
 */
void mpc_print_config_xml_handler(enum sctk_runtime_config_walk_type type,
                                         const char * name,
                                         const char * type_name,
                                         void * value,
                                         enum sctk_runtime_config_walk_status status,
                                         const struct sctk_runtime_config_entry_meta * type_meta,
                                         int level,
                                         void * opt)
{
	//check open/close
	if (status == SCTK_RUNTIME_CONFIG_WALK_CLOSE)
	{
		switch(type)
		{
			case SCTK_RUNTIME_CONFIG_WALK_ARRAY:
				sctk_runtime_config_display_indent(level);
				printf("</%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_VALUE:
				printf("</%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_STRUCT:
				sctk_runtime_config_display_indent(level);
				printf("</%s>\n",name);
				break;
		}
	} else {
		switch(type)
		{
			case SCTK_RUNTIME_CONFIG_WALK_VALUE:
				//print the name if not simple array
				sctk_runtime_config_display_indent(level);
				printf("<%s>",name);
				//print the value
				assume_m(sctk_runtime_config_display_plain_type(type_name,value),"Invalid plain type : %s.",type_name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_ARRAY:
				sctk_runtime_config_display_indent(level);
				printf("<%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_STRUCT:
				sctk_runtime_config_display_indent(level);
				printf("<%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_UNION:
				break;
		}
	}
}

/*******************  FUNCTION  *********************/
/**
 * Method to display the structure.
 * @param config_meta Pointer to the root element of the meta description of the structure.
 * @param root_name Name of the root node.
 * @param root_struct_name Type name of the root structure (must be a struct, not a union).
 * @param root_struct Pointer to the C structure to display.
 */
void mpc_print_config_xml(const struct sctk_runtime_config_entry_meta * config_meta,
                                      const char * root_name,
                                      const char * root_struct_name,
                                      void * root_struct)
{
	printf("<?xml version='1.0'?>\n");
	sctk_runtime_config_walk_tree(config_meta,mpc_print_config_xml_handler,root_name,root_struct_name,root_struct,NULL);
}
