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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

/********************************* INCLUDES *********************************/
#include <stdio.h>
#include <string.h>
#include <sctk_debug.h>
#include "runtime_config_printer.h"
#include "runtime_config_mapper.h"
#include "runtime_config_walk.h"

/********************************* MACROS ***********************************/
#define SCTK_MB_SIZE 1048576llu
#define SCTK_GB_SIZE 1073741824llu
#define SCTK_TB_SIZE 1099511627776llu
#define SCTK_PB_SIZE 1125899906842624llu

/********************************* FUNCTION *********************************/
/**
 * Display tabulation for indentation.
 * @param level Number indentation step.
**/
void sctk_runtime_config_display_indent(int level)
{
	while (level > 0) {
		printf("\t");
		level--;
	}
}

/********************************* FUNCTION *********************************/
/**
 * Display size in B, KB, MB, GB, TB or PB.
 * @param value The size to display.
 * @return The value converted using defined units.
 */
char * sctk_runtime_config_display_size(size_t value)
{
	static char size[500];
	unsigned long long int t_size = 0;

	size[0]='\0';

	if( value < 1024 ) {
		/* Bits */
		sprintf(size,"%llu B", (unsigned long long int)value);
	} else if( value < SCTK_MB_SIZE ) {
		/* KB */
		t_size = value/(1024);
		sprintf(size,"%llu KB", t_size);
	} else if( value < SCTK_GB_SIZE ) {
		/* MB */
		t_size = value/(SCTK_MB_SIZE);
		sprintf(size,"%llu MB", t_size);
	} else if( value < SCTK_TB_SIZE ) {
		/* GB */
		t_size = value/(SCTK_GB_SIZE);
		sprintf(size,"%llu GB", t_size);
	} else if( value < SCTK_PB_SIZE ) {
		/* TB */
		t_size = value/(SCTK_TB_SIZE);
		sprintf(size,"%llu TB", t_size);
	} else {
		/* PB */
		t_size = value/(SCTK_PB_SIZE);
		sprintf(size,"%llu PB", t_size);
	}

	return size;
}



/********************************* FUNCTION *********************************/
/**
 * Display simple values.
 * @param type_name Type of the value to display.
 * @param value Value to display.
 * @return true if the type value is a basic type, false otherwise.
**/
bool sctk_runtime_config_display_plain_type( const char * type_name,void *value)
{
	if (strcmp(type_name,"int") == 0) {
		printf("%d",*(int*)value);
		return true;
	} else if (strcmp(type_name,"bool") == 0) {
		printf("%s",(*(bool*)value)?"true":"false");
		return true;
	} else if (strcmp(type_name,"float") == 0) {
		printf("%f",(double)*(float*)value);
		return true;
	} else if (strcmp(type_name,"double") == 0) {
		printf("%g",*(double*)value);
		return true;
	} else if (strcmp(type_name,"char *") == 0) {
		if (*((char **)value) == NULL)
			printf("%s",sctk_cst_string_undefined);
		else
			printf("%s",*((char **)value));
		return true;
	} else if (strcmp(type_name,"size_t") == 0) {
		printf("%s", sctk_runtime_config_display_size( *( (size_t *)value ) ) );
		return true;
	} else {
		return false;
	}
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
void sctk_runtime_config_display_handler(enum sctk_runtime_config_walk_type type,
        const char * name,
        const char * type_name,
        void * value,
        enum sctk_runtime_config_walk_status status,
        __UNUSED__ const struct sctk_runtime_config_entry_meta * type_meta,
        int level,
        void * opt)
{
	printf("%s == %s\n", name, type_name);
	/* get the current state */
	struct sctk_runtime_config_display_state * state = opt;

	/* check open/close */
	if (status == SCTK_RUNTIME_CONFIG_WALK_CLOSE) {
		/* close only simple array as it was name: {v1,v2,v3} */
		if (state->is_simple_array && type == SCTK_RUNTIME_CONFIG_WALK_ARRAY) {
			printf("}\n");
			state->is_simple_array = false;
		}
	} else {
		switch(type) {
			case SCTK_RUNTIME_CONFIG_WALK_VALUE:
				/* print the name if not simple array */
				if (!state->is_simple_array) {
					sctk_runtime_config_display_indent(level);
					printf("%s : ",name);
				}

				/* print the value */
				assume_m(sctk_runtime_config_display_plain_type(type_name,value),"Invalid plain type : %s.",type_name);

				/* separator if simple array, line break otherwise */
				if (state->is_simple_array)
					printf(",");
				else
					printf("\n");
				break;
			case SCTK_RUNTIME_CONFIG_WALK_ARRAY:
				sctk_runtime_config_display_indent(level);
				/* detect siple array to change the display mode of values to be more compact. */
				if (sctk_runtime_config_is_basic_type(type_name)) {
					printf("%s : {",name);
					state->is_simple_array = true;
				} else {
					printf("%s : \n",name);
				}
				break;
			case SCTK_RUNTIME_CONFIG_WALK_STRUCT:
				sctk_runtime_config_display_indent(level);
				printf("%s : \n",name);
				break;
			case SCTK_RUNTIME_CONFIG_WALK_UNION:
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
void sctk_runtime_config_display_tree(const struct sctk_runtime_config_entry_meta * config_meta,
                                      const char * root_name,
                                      const char * root_struct_name,
                                      void * root_struct)
{
	struct sctk_runtime_config_display_state state;
	state.is_simple_array = false;
	sctk_runtime_config_walk_tree(config_meta,sctk_runtime_config_display_handler,root_name,root_struct_name,root_struct,&state);
}
