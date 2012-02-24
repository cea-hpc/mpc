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
#include "sctk_runtime_config_debug.h"
#include "sctk_runtime_config_printer.h"
#include "sctk_runtime_config_mapper.h"

/*******************  FUNCTION  *********************/
/**
 * Display tabulation for indentation.
 * @param level Number indentation step.
**/
void sctk_runtime_config_display_indent(int level)
{
	while (level > 0)
	{
		printf("\t");
		level--;
	}
}

/*******************  FUNCTION  *********************/
/**
 * Display all child of a composed configuration union.
 * @param config_meta Define the address of base element of meta description table.
 * @param value Define the current element pointer.
 * @param current Define the meta description for current element.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_runtime_config_display_union(const struct sctk_runtime_config_entry_meta * config_meta, void * value,const char * type_name,int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	int * union_type_id = value;
	void * union_data = value + sizeof(int);

	//error
	assert(type_name != NULL);
	assert(value != NULL);
	assert(sizeof(enum sctk_runtime_config_meta_entry_type) == sizeof(int));
	assert(*union_type_id < 64);

	//find meta entry for type
	entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
	assert(entry != NULL);
	assert(entry->type == SCTK_CONFIG_META_TYPE_UNION);

	//get real child, id is given by enum value, and as we were on SCTK_CONFIG_META_TYPE_UNION entry
	//of meta description table, we known that we need to increment to get the good one.
	entry += *union_type_id;

	//if non type id, get empty value
	if (*union_type_id == 0)
	{
		printf("NONE\n");
	} else {
		//check
		assume(entry->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY,"Invalid union entry type : %d.",entry->type);

		//display entry name
		printf("\n");
		sctk_runtime_config_display_indent(level);
		printf("%s :",entry->name);

		//display the value by using good type
		sctk_runtime_config_display_value(config_meta,union_data,entry->inner_type,level);
	}
}

/*******************  FUNCTION  *********************/
/**
 * Display all child of a composed configuration structure.
 * @param config_meta Define the address of base element of meta description table.
 * @param struct_ptr Define the current struct root pointer.
 * @param current Define the meta description for current element.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_runtime_config_display_struct(const struct sctk_runtime_config_entry_meta * config_meta, void * struct_ptr,const char * type_name,int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	const struct sctk_runtime_config_entry_meta * child;
	void * value = struct_ptr;

	//error
	assert(config_meta != NULL);
	assert(type_name != NULL);
	assert(struct_ptr != NULL);

	//find meta entry for type
	entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
	assert(entry != NULL);
	assert(entry->type == SCTK_CONFIG_META_TYPE_STRUCT);

	//get first child (params or arrays)
	child = sctk_runtime_config_meta_get_first_child(entry);

	//loop unit last one
	while (child != NULL)
	{
		//get value
		value = sctk_runtime_config_get_entry(struct_ptr,child);
		assert(value != NULL);

		//display param name
		printf("\n");
		sctk_runtime_config_display_indent(level);
		printf("%s: ",child->name);

		//display value depending on type
		if (child->type == SCTK_CONFIG_META_TYPE_ARRAY)
		{
			sctk_runtime_config_display_array(config_meta, value,child,level);
		} else {
			assert(child->type == SCTK_CONFIG_META_TYPE_PARAM || child->type == SCTK_CONFIG_META_TYPE_ARRAY || child->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY);
			sctk_runtime_config_display_value(config_meta,value,child->inner_type,level);
		}

		//move to next
		child = sctk_runtime_config_meta_get_next_child(child);
	}
}

/*******************  FUNCTION  *********************/
/**
 * Display element of an array from configuration structure.
 * @param config_meta Define the address of base element of meta description table.
 * @param value Define the pointer to the entry in structure, so pointer to the pointer which contain the address of the array.
 * @param current Define the current meta description, must by of type SCTK_CONFIG_TYPE_ARRAY.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_runtime_config_display_array(const struct sctk_runtime_config_entry_meta * config_meta,
                               void ** value,const struct sctk_runtime_config_entry_meta * current,int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	int size;
	int i;
	bool is_basic_type;

	//error
	assert(config_meta != NULL);
	assert(value != NULL);
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_ARRAY);
	assert(current->inner_type != NULL);
	assert(current->extra != NULL);

	//size is next element by construction (see generator of struct file) so :
	size = *(int*)(value+1);

	//if empty
	if (*value == NULL)
	{
		assert(size == 0);
		printf("{}");
	} else {
		assert(size >= 0);

		//check if basic type for special display
		is_basic_type = sctk_runtime_config_is_basic_type(current->inner_type);
		if (is_basic_type)
			printf("{ ");

		//loop on all elements
		for (i = 0 ; i < size ; i++)
		{
			//compute address of current one
			void * element = ((char*)(*value)) + i * current->size;

			//add some separators in display
			if (is_basic_type == false)
			{
				printf("\n");
				sctk_runtime_config_display_indent(level+1);
				printf("%s :",(const char *)current->extra);
			} else if (i > 0) {
				printf(", ");
			}

			//display the value
			sctk_runtime_config_display_value(config_meta,element,current->inner_type,level+1);
		}

		//some closure string.
		if (is_basic_type)
			printf(" }");
	}
}

/*******************  FUNCTION  *********************/
/**
 * Display a given value by using the meta description to known which type to apply.
 * @param config_meta Define the root element of meta description table to use.
 * @param value Define the base adresse of the value to display.
 * @param type_name Define the type name of value to display.
 * @param level Define the indentation level.
**/
void sctk_runtime_config_display_value(const struct sctk_runtime_config_entry_meta * config_meta,
                                       void * value,
                                       const char * type_name, int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	bool is_basic_type;

	//error
	assert(config_meta != NULL);
	assert(type_name != NULL);
	assert(value != NULL);

	//try to display as a basic type
	is_basic_type = sctk_runtime_config_display_plain_type( type_name, value, level);

	//if not, need some work.
	if ( ! is_basic_type )
	{
		//get metta data of the entry
		entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
		assert(entry != NULL);
		
		//apply related display method.
		if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT)
			sctk_runtime_config_display_struct(config_meta, value,type_name,level+1);
		else if (entry->type == SCTK_CONFIG_META_TYPE_UNION)
			sctk_runtime_config_display_union(config_meta, value,type_name,level+1);
		else
			fatal("Invalid type.");
	}
}

/*******************  FUNCTION  *********************/
/**
 * Test if the given type name is a basic one or not.
 * @param type_name The type name to test (int, double ...)
**/
bool sctk_runtime_config_is_basic_type(const char * type_name)
{
	return (strcmp(type_name,"int")    == 0   || strcmp(type_name,"bool")   == 0
	    ||  strcmp(type_name,"double") == 0   || strcmp(type_name,"char *") == 0 );
}

/*******************  FUNCTION  *********************/
/**
 * Display simple values.
 * @param current Define the current meta entry to display.
 * @param value Define the indentation level.
**/
bool sctk_runtime_config_display_plain_type( const char * type_name,void *value, int level)
{
	if (strcmp(type_name,"int") == 0)
	{
		printf("%d",*(int*)value);
		return true;
	} else if (strcmp(type_name,"bool") == 0)
	{
		printf("%s",(*(bool*)value)?"true":"false");
		return true;
	} else if (strcmp(type_name,"float") == 0)
	{
		printf("%f",*(float*)value);
		return true;
	} else if (strcmp(type_name,"double") == 0)
	{
		printf("%g",*(double*)value);
		return true;
	} else if (strcmp(type_name,"char *") == 0)
	{
		printf("%s",*((char **)value));
		return true;
	} else {
		return false;
	}
}

