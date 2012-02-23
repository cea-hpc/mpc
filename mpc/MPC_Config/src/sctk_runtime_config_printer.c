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
 * @param config Define the configuration root structure.
 * @param struct_ptr Define the current element pointer.
 * @param current Define the meta description for current element.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_runtime_config_display_union(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,sctk_runtime_config_struct_ptr struct_ptr,const char * type_name,int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	int * union_type_id = struct_ptr;
	void * union_data = struct_ptr + sizeof(int);

	//error
	assert(config != NULL);
	assert(type_name != NULL);
	assert(struct_ptr != NULL);
	assert(sizeof(enum sctk_runtime_config_meta_entry_type) == sizeof(int));
	assert(*union_type_id < 64);

	//find meta entry for type
	entry = sctk_runtime_config_get_meta_type_entry(config_meta, type_name);
	assert(entry != NULL);
	assert(entry->type == SCTK_CONFIG_META_TYPE_UNION);

	//get real child
	entry += *union_type_id;

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
		//display it
		sctk_runtime_config_display_entry(config_meta,config,union_data,entry,level);
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
void sctk_runtime_config_display_struct(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,sctk_runtime_config_struct_ptr struct_ptr,const char * type_name,int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	const struct sctk_runtime_config_entry_meta * child;
	void * value = struct_ptr;

	//error
	assert(config != NULL);
	assert(type_name != NULL);
	assert(struct_ptr != NULL);

	//find meta entry for type
	entry = sctk_runtime_config_get_meta_type_entry(config_meta, type_name);
	assert(entry != NULL);
	assert(entry->type == SCTK_CONFIG_META_TYPE_STRUCT);

	//display childs if any
	//only struct has child and we have child only if next entry has level+1
	if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT)
	{
		child = sctk_runtime_config_meta_get_first_child(entry);
		while (child != NULL)
		{
			//get value
			value = sctk_runtime_config_get_entry(struct_ptr,child);
			assert(value != NULL);
			//display name
			printf("\n");
			sctk_runtime_config_display_indent(level);
			printf("%s: ",child->name);
			//display value
			if (child->type == SCTK_CONFIG_META_TYPE_ARRAY)
				sctk_runtime_config_display_array(config_meta, config,value,child,level);
			else
				sctk_runtime_config_display_entry(config_meta, config,value,child,level);
			//move to next
			child = sctk_runtime_config_meta_get_next_child(child);
		}
	}
}

static int _sctk_runtime_config_display_plain_type( const struct sctk_runtime_config_entry_meta * current,void *value, int level);

/*******************  FUNCTION  *********************/
/**
 * Display element of an array from configuration structure.
 * @param config Define the pointer to root configuration structure.
 * @param value Define the pointer to the entry in structure, so pointer to the pointer which contain the address of the array.
 * @param current Define the current meta description, must by of type SCTK_CONFIG_TYPE_ARRAY.
 * @param level Define the indentation level to apply to current element.
**/
void sctk_runtime_config_display_array(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,
                               void ** value,const struct sctk_runtime_config_entry_meta * current,int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;
	int size;
	int element_size;
	int i;
	bool is_basic_type;

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
		printf("{}");
	} else {
		assert(size >= 0);

		is_basic_type = sctk_runtime_config_is_basic_type(current->inner_type);
		if (is_basic_type)
			printf("{ ");

		element_size = current->size;
		for (i = 0 ; i < size ; i++)
		{
			void * element = ((char*)(*value))+i*element_size;

			//add some separators
			if (is_basic_type == false)
			{
				printf("\n");
				sctk_runtime_config_display_indent(level+1);
				printf("%s :",(const char *)current->extra);
			} else if (i > 0) {
				printf(", ");
			}

			//display the value
			sctk_runtime_config_display_entry(config_meta,config,element,current,level+1);
		}

		if (is_basic_type)
			printf(" }");
	}
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_display_entry(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,
                                       sctk_runtime_config_struct_ptr value,
                                       const struct sctk_runtime_config_entry_meta * current, int level)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;

	//error
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY || current->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY);

	if ( !sctk_runtime_config_display_plain_type( current->inner_type, value, level) )
	{
		entry = sctk_runtime_config_get_meta_type_entry(config_meta, current->inner_type);
		assert(entry != NULL);
		/** @TODO avoid this duplication of code **/
		if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT)
			sctk_runtime_config_display_struct(config_meta, config,value,current->inner_type,level+1);
		else if (entry->type == SCTK_CONFIG_META_TYPE_UNION)
			sctk_runtime_config_display_union(config_meta, config,value,current->inner_type,level+1);
		else
			fatal("Invalid type.");
	}
}

/*******************  FUNCTION  *********************/
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

