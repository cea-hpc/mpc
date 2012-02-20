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
#include <string.h>
#include "sctk_runtime_config_debug.h"
#include "sctk_libxml_helper.h"
#include "sctk_runtime_config_mapper.h"
#include "sctk_runtime_config_printer.h"

/*******************  FUNCTION  *********************/
bool sctk_runtime_config_map_entry_to_bool(xmlNodePtr node)
{
	bool res;
	xmlChar * value = xmlNodeGetContent(node);
	if (xmlStrcmp(value,BAD_CAST("true")) == 0)
	{
		res = true;
	} else if (xmlStrcmp(value,BAD_CAST("false")) == 0) {
		res = false;
	} else {
		fprintf(stderr,"Invalid boolean value (true|false) : %s\n",value);
		abort();
	}
	free(value);
	return res;
}

/*******************  FUNCTION  *********************/
/** @todo avoid to usr xmlNodeGetContent which made a copy. **/
int sctk_runtime_config_map_entry_to_int(xmlNodePtr node)
{
	int res;
	xmlChar * value = xmlNodeGetContent(node);
	res = atoi((char*)value);
	free(value);
	return res;
}

/*******************  FUNCTION  *********************/
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_meta_get_first_child(const struct sctk_runtime_config_entry_meta * current)
{
	//error
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_STRUCT);

	//current level
	current++;

	//if new level is same, we found next child, otherwise we reach end
	if (current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY)
		return current;
	else
		return NULL;
}

/*******************  FUNCTION  *********************/
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_meta_get_next_child(const struct sctk_runtime_config_entry_meta * current)
{
	//error
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY);

	//current level
	current++;

	//if new level is same, we found next child, otherwise we reach end
	if (current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY)
		return current;
	else
		return NULL;
}

/*******************  FUNCTION  *********************/
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_get_child_meta(const struct sctk_runtime_config_entry_meta * current,const xmlChar * name)
{
	//errors
	assert(current != NULL);
	assert(name != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_STRUCT);

	//loop on childs to get entry
	current = sctk_runtime_config_meta_get_first_child(current);
	while (current != NULL)
	{
		if (xmlStrcmp(name,BAD_CAST(current->name)) == 0)
			break;
		current = sctk_runtime_config_meta_get_next_child(current);
	}

	return current;
}

/*******************  FUNCTION  *********************/
void * sctk_runtime_config_get_entry(sctk_runtime_config_struct_ptr struct_ptr,const struct sctk_runtime_config_entry_meta * current)
{
	//errors
	assert(current->offset >= 0);
	assert(current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY);

	//compute address and return
	return ((void*)struct_ptr)+current->offset;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_apply_init_handler(const struct sctk_runtime_config_entry_meta *config_meta, sctk_runtime_config_struct_ptr struct_ptr,const char * type_name)
{
	//error
	assert(struct_ptr != NULL);
	assert(type_name != NULL);

	//search definition
	const struct sctk_runtime_config_entry_meta * entry;
	entry = sctk_runtime_config_get_meta_type_entry(config_meta, type_name);
	if (entry != NULL 
            && entry->type == SCTK_CONFIG_META_TYPE_STRUCT 
            && entry->extra != NULL)
		((sctk_runtime_config_struct_init_handler)entry->extra)(struct_ptr);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_apply_node_array(const struct sctk_runtime_config_entry_meta *config_meta, struct sctk_runtime_config * config,
                                  sctk_runtime_config_struct_ptr struct_ptr,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node)
{
	//vars
	const xmlChar * entry_name;
	int cnt;
	void ** array = struct_ptr;
	int * array_size;

	//errors
	assert(config != NULL);
	assert(struct_ptr != NULL);
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_ARRAY);
	assert(node != NULL);
	assert(xmlStrcmp(node->name,BAD_CAST(current->name)) == 0);
	assert(current->inner_type != NULL);
	assert(current->extra != NULL);

	//get entry
	//we know that the size counter is next element by definition
	array_size = (int*)(array+1);
	//get element size
	assert(current->size > 0 && current->size < 4096);

	//check coherency
	assert((*array_size == 0 && *array == NULL) || (*array_size != 0 && *array != NULL));

	//delete old one
	if (*array != NULL)
		free(*array);

	//get name of sub-elements
	entry_name = current->extra;

	//counter number of such child to allocate array memory
	cnt = sctk_libxml_count_child_nodes(node,entry_name);

	//if no child, set to NULL
	if (cnt == 0)
	{
		*array = NULL;
		*array_size = 0;
	} else {
		*array = calloc(cnt,current->size);
		*array_size = cnt;
		//loop on all child nodes
		int i = 0;
		node = xmlFirstElementChild(node);
		while (node != NULL)
		{
			//must only find nodes with tage_name = entry_name
			assume(xmlStrcmp(node->name,entry_name) == 0,"Invalid child node in array alement, expect %s -> %s but get %s.",current->name,(const char*)current->extra,node->name);
			sctk_runtime_config_apply_init_handler(config_meta, (*array)+i*current->size,current->inner_type);
			sctk_runtime_config_apply_node_value(config_meta, config,(*array)+i*current->size,current->inner_type,node);
			node = xmlNextElementSibling(node);
			i++;
		}
	}
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_apply_node_value( const struct sctk_runtime_config_entry_meta *config_meta, struct sctk_runtime_config * config,sctk_runtime_config_struct_ptr struct_ptr, const char * type_name,xmlNodePtr node)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry;

	//errors
	assert(config != NULL);
	assert(struct_ptr != NULL);
	assert(node != NULL);
	assert(type_name != NULL);

	//check type name
	if (strcmp(type_name,"int") == 0)
	{
		*(int*)struct_ptr = sctk_runtime_config_map_entry_to_int(node);
	} else if (strcmp(type_name,"bool") == 0) {
		*(bool*)struct_ptr = sctk_runtime_config_map_entry_to_bool(node);
	} else {
		entry = sctk_runtime_config_get_meta_type_entry(config_meta, type_name);
		if (entry == NULL)
		{
			fatal("Can't find type information for : %s.",type_name);
		} else if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT) {
			sctk_runtime_config_map_node_to_c_struct(config_meta,config,struct_ptr,entry,node);
		} else {
			fatal("Unknown custom type : %s (%d)",type_name,entry->type);
		}
	}
}

/*******************  FUNCTION  *********************/
/** @TODO cleanup this method. **/
void sctk_runtime_config_map_node_to_c_struct( const struct sctk_runtime_config_entry_meta *config_meta, struct sctk_runtime_config * config,
                                       sctk_runtime_config_struct_ptr struct_ptr,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node)
{
	//child
	const struct sctk_runtime_config_entry_meta * child;
	sctk_runtime_config_struct_ptr child_ptr;

	//errors
	assert(config != NULL);
	assert(current != NULL);
	assert(node != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_STRUCT);

	//setup value
	node = xmlFirstElementChild(node);
	while (node != NULL)
	{
		child = sctk_runtime_config_get_child_meta(current,node->name);
		assume(child != NULL,"Can't find meta data for node : %s",node->name);
		child_ptr = sctk_runtime_config_get_entry(struct_ptr,child);
		assume(child_ptr != NULL,"Can't find meta data for node entry : %s",node->name);
		switch(child->type)
		{
			case SCTK_CONFIG_META_TYPE_ARRAY:
				sctk_runtime_config_apply_node_array(config_meta,config,child_ptr,child,node);
				break;
			case SCTK_CONFIG_META_TYPE_PARAM:
				sctk_runtime_config_apply_node_value(config_meta,config,child_ptr,child->inner_type,node);
				break;
			default:
				fatal("Invalid current meta entry type : %d",current->type);				
		}
		node = xmlNextElementSibling(node);
	}
}

/*******************  FUNCTION  *********************/
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_get_meta_type_entry( const struct sctk_runtime_config_entry_meta *config_meta, const char * name)
{
	//vars
	const struct sctk_runtime_config_entry_meta * entry = config_meta;

	//error
	assert(name != NULL);

	//search
	while (entry->type != SCTK_CONFIG_META_TYPE_END && (entry->type != SCTK_CONFIG_META_TYPE_STRUCT || strcmp(name,entry->name) != 0))
		entry++;

	//if end, not found
	if (entry->type == SCTK_CONFIG_META_TYPE_END)
	{
		return NULL;
	} else {
		assert(strcmp(name,entry->name) == 0);
		assert(entry->type == SCTK_CONFIG_META_TYPE_STRUCT);
		return entry;
	}
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_do_cleanup(struct sctk_runtime_config* config)
{

}

