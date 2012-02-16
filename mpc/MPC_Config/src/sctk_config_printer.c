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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sctk_config_printer.h"
#include "sctk_config_mapper.h"

/*******************  FUNCTION  *********************/
/**
 * Display tabulation for indentation.
 * @param level Number indentation step.
**/
void sctk_config_display_indent(int level)
{
	while (level > 0)
	{
		printf("\t");
		level--;
	}
}

/*******************  FUNCTION  *********************/
/**
 * Display all child of a composed configuration structure.
 * @param config Define the configuration root structure.
 * @param struct_ptr Define the current element pointer.
 * @param current Define the meta description for current element.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_config_display_struct(struct sctk_config * config,sctk_config_struct_ptr struct_ptr,const char * type_name,int level)
{
	//vars
	const struct sctk_config_entry_meta * entry;
	const struct sctk_config_entry_meta * child;
// 	void * value = struct_ptr;

	//error
	assert(config != NULL);
	assert(type_name != NULL);
	assert(struct_ptr != NULL);

	//find meta entry for type
	entry = sctk_config_get_meta_type_entry(type_name);
	assert(entry != NULL);

	//display childs if any
	//only struct has child and we have child only if next entry has level+1
	if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT)
	{
		child = sctk_config_meta_get_first_child(entry);
		while (child != NULL)
		{
			//get value
// 			value = sctk_config_get_entry(struct_ptr,child);
// 			assert(value != NULL);
			sctk_config_display_entry(config,struct_ptr,child,level);
			//move to next
			child = sctk_config_meta_get_next_child(child);
		}
	}
}

/*******************  FUNCTION  *********************/
/**
 * Display element of an array from configuration structure.
 * @param config Define the pointer to root configuration structure.
 * @param value Define the pointer to the entry in structure, so pointer to the pointer which contain the address of the array.
 * @param current Define the current meta description, must by of type SCTK_CONFIG_TYPE_ARRAY.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_config_display_array(struct sctk_config * config,void ** value,const struct sctk_config_entry_meta * current,int level)
{
	int size;
	int element_size;
	int i;

	//error
	assert(config != NULL);
	assert(value != NULL);
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_ARRAY);
	assert(current->inner_type != NULL);
	assert(current->extra != NULL);

	//size is next element by definition so :
	size = *(int*)(value+1);

	//if empty
	if (*value == NULL)
	{
		assert(size == 0);
		sctk_config_display_indent(level);
		printf("%s: {}\n",current->name);
	} else {
		assert(size >= 0);
		//loop on all values, we know that next meta entry define the type.
		sctk_config_display_indent(level);
		if (strcmp(current->inner_type,"int") == 0 || strcmp(current->inner_type,"bool") == 0)
			printf("%s: {",current->name);
		else
			printf("%s:\n",current->name);
		for (i = 0 ; i < size ; i++)
		{
			/** @TODO Avoid code duplication here with next method. **/
			if (strcmp(current->inner_type,"int") == 0)
			{
				printf("%d, ",((int*)(*value))[i]);
			} else if (strcmp(current->inner_type,"bool") == 0) {
				printf("%s, ",(((bool*)(*value))[i])?"true":"false");
			} else {
				//get element size, we know that it's the next element
				sctk_config_display_indent(level+1);
				printf("%s :\n",(const char*)current->extra);
				element_size = current->size;
				assert(element_size > 0);
				sctk_config_display_struct(config,((char*)(*value))+i*element_size,current->inner_type,level+2);
			}		
		}
		if (strcmp(current->inner_type,"int") == 0 || strcmp(current->inner_type,"bool") == 0)
			printf("}\n");
	}
}

/*******************  FUNCTION  *********************/
void sctk_config_display_entry(struct sctk_config * config,sctk_config_struct_ptr struct_ptr,const struct sctk_config_entry_meta * current,int level)
{
	//vars
	void * value;

	//error
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY);

	//get value
	value = sctk_config_get_entry(struct_ptr,current);
	assert(value != NULL);

	if (current->type == SCTK_CONFIG_META_TYPE_ARRAY)
	{
		sctk_config_display_array(config,value,current,level);
	} else if (strcmp(current->inner_type,"int") == 0) {
		sctk_config_display_indent(level);
		printf("%s: %d\n",current->name,*(int*)value);
	} else if (strcmp(current->inner_type,"bool") == 0) {
		sctk_config_display_indent(level);
		printf("%s: %s\n",current->name,(*(bool*)value)?"true":"false");
	} else {
		sctk_config_display_indent(level);
		printf("%s: \n",current->name);
		sctk_config_display_struct(config,value,current->inner_type,level+1);
	}
}

/*******************  FUNCTION  *********************/
void sctk_config_display(struct sctk_config * config)
{
	//error
	assert(config != NULL);

	//separator
	printf("====================== MPC CONFIG ======================\n");
	sctk_config_display_struct(config,&config->modules,"sctk_config_modules",0);
	printf("========================================================\n");
}
