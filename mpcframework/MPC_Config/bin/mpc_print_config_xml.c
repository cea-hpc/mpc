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

/********************************* INCLUDES *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sctk_bool.h>
#include <sctk_debug.h>
#include <sctk_runtime_config_walk.h>
#include <sctk_runtime_config_printer.h>
#include "mpc_print_config_xml.h"

/********************************  CONSTS  **********************************/
/** Header of XML output format. **/
const char * mpc_print_config_xml_header = "<?xml version='1.0'?>\n\
<mpc xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"mpc-config.xsd\">\n\
\t<profiles>\n";
/** Footer of XML output format. **/
const char * mpc_print_config_xml_footer = "\
\t</profiles>\n\
\t<mappings>\n\
\t</mappings>\n\
</mpc>\n";

/********************************* FUNCTION *********************************/
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
                                  __UNUSED__ const struct sctk_runtime_config_entry_meta * type_meta,
                                  int level,
                                  __UNUSED__ void * opt)
{
	/* erase name of root node */
	if (level == 0 && strcmp(name,"config") == 0)
		name = "profile";

	/* check open/close */
	if (status == SCTK_RUNTIME_CONFIG_WALK_CLOSE) {
		switch(type) {
			case SCTK_RUNTIME_CONFIG_WALK_ARRAY:
				sctk_runtime_config_display_indent(level+2);
				printf("</%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_VALUE:
				printf("</%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_STRUCT:
				sctk_runtime_config_display_indent(level+2);
				printf("</%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_UNION:
				/* print only if the content is not SCTK_RTCFG_..._NONE */
				if (*(int*)value != 0) {
					sctk_runtime_config_display_indent(level+2);
					printf("</%s>\n",name);
				}
				break;
		}
	} else {
		switch(type) {
			case SCTK_RUNTIME_CONFIG_WALK_VALUE:
				/* print the name if not simple array */
				sctk_runtime_config_display_indent(level+2);
				printf("<%s>",name);
				/* print the value */
				assume_m(sctk_runtime_config_display_plain_type(type_name,value),"Invalid plain type : %s.",type_name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_ARRAY:
				sctk_runtime_config_display_indent(level+2);
				printf("<%s>\n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_STRUCT:
				sctk_runtime_config_display_indent(level+2);
				printf("<%s>\n",name);
				/* if root node, add the <default> tag */
				if (level == 0) {
					sctk_runtime_config_display_indent(level+2+1);
					printf("<name>default</name>\n");
				}
				break;
			case SCTK_RUNTIME_CONFIG_WALK_UNION:
				/* print only if the content is not SCTK_RTCFG_..._NONE */
				if (*(int*)value != 0) {
					sctk_runtime_config_display_indent(level+2);
					printf("<%s>\n",name);
				}
				break;
		}
	}
}

/********************************* FUNCTION *********************************/
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
	puts(mpc_print_config_xml_header);
	sctk_runtime_config_walk_tree(config_meta,mpc_print_config_xml_handler,root_name,root_struct_name,root_struct,NULL);
	puts(mpc_print_config_xml_footer);
}
