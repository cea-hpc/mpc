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
#include <mpc_common_types.h>
#include <mpc_common_debug.h>
#include <runtime_config_walk.h>
#include <runtime_config_printer.h>
#include "mpc_print_config_sh.h"

/********************************* FUNCTION *********************************/
/**
 * Replace ascii lower case letter by upper case onces.
**/
void to_upper_case(char * value)
{
	while (*value != '\0') {
		if (*value >= 'a' && *value <= 'z')
			*value += 'Z' - 'z';
		value++;
	}
}

/********************************* FUNCTION *********************************/
/**
 * Print the name of the variable by reusing the current state to get the full path
 * to access to the current node. It will used path entries in upper case separated
 * by underscore.
**/
void print_var_name_sh(struct display_state_sh * state,const char * name,int level)
{
	char buffer[64];
	int i = 0;
	int size = 0;
	for ( i = 0 ; i < level ; i++) {
		size += sprintf(buffer+size,"%s_",state->names[i]);
	}
	sprintf(buffer+size,"%s=\"",name);
	to_upper_case(buffer);
	printf("%s",buffer);
}

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
void display_handler_sh(enum sctk_runtime_config_walk_type type,
                        const char * name,
                        const char * type_name,
                        void * value,
                        enum sctk_runtime_config_walk_status status,
                        __UNUSED__ const struct sctk_runtime_config_entry_meta * type_meta,
                        int level,
                        void * opt)
{
	/* get the current state */
	struct display_state_sh * state = opt;

	/* check open/close */
	if (status == SCTK_RUNTIME_CONFIG_WALK_CLOSE) {
		/* close only simple array as it was name: {v1,v2,v3} */
		if (state->is_simple_array && type == SCTK_RUNTIME_CONFIG_WALK_ARRAY) {
			state->is_simple_array = 0;
		} else if (type == SCTK_RUNTIME_CONFIG_WALK_VALUE) {
			printf("\"\n");
		}
	} else {
		switch(type) {
			case SCTK_RUNTIME_CONFIG_WALK_VALUE:
				/* push current name */
				print_var_name_sh(state,name,level);

				/* print the value */
				assume_m(sctk_runtime_config_display_plain_type(type_name,value),"Invalid plain type : %s.",type_name);

				/* separator if simple array, line break otherwise */
				if (state->is_simple_array)
					printf(" ");
				break;
			case SCTK_RUNTIME_CONFIG_WALK_ARRAY:
				/* detect simple array to change the display mode of values to be more compact. */
				if (sctk_runtime_config_is_basic_type(type_name)) {
					print_var_name_sh(state,name,level);
					state->is_simple_array = 1;
				} else {
					mpc_common_debug_warning("Can't display array of struct in sh compatible output mode.");
				}
				break;
			case SCTK_RUNTIME_CONFIG_WALK_STRUCT:
				state->names[level] = name;
				break;
			case SCTK_RUNTIME_CONFIG_WALK_UNION:
				mpc_common_debug_warning("Can't display union in sh compatible output mode.");
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
void display_tree_sh(const struct sctk_runtime_config_entry_meta * config_meta,
                     const char * root_name,
                     const char * root_struct_name,
                     void * root_struct)
{
	struct display_state_sh state;
	state.is_simple_array = 0;
	memset(state.names,0,sizeof(state.names));
	sctk_runtime_config_walk_tree(config_meta,display_handler_sh,root_name,root_struct_name,root_struct,&state);
}
