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
#include <string.h>
#include <stdlib.h>
#include <sctk_debug.h>
#include "sctk_libxml_helper.h"
#include "sctk_runtime_config_mapper.h"
#include "sctk_runtime_config_printer.h"
#include "sctk_runtime_config_struct_defaults.h"

extern bool sctk_crash_on_symbol_load;

/********************************* CONSTS **********************************/
/** Special string value to set string pointer to NULL in config management. **/
const char * sctk_cst_string_undefined = "undefined";

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to a boolean value.
 * @param node XML entry to map.
 * @return The boolean mapped value.
 */
bool sctk_runtime_config_map_entry_to_bool(xmlNodePtr node)
{
	bool res;
	xmlChar * value = xmlNodeGetContent(node);
	if (xmlStrcmp(value,BAD_CAST("true")) == 0 || xmlStrcmp(value,BAD_CAST("0")) == 0 ) {
		res = true;
	} else if (xmlStrcmp(value,BAD_CAST("false")) == 0 || xmlStrcmp(value,BAD_CAST("1")) == 0) {
		res = false;
	} else {
		sctk_fatal("Invalid boolean value (true|false) : %s",value);
	}
	xmlFree(value);
	return res;
}

/******************************** FUNCTION *********************************/
/** @todo avoid to usr xmlNodeGetContent which made a copy. **/
/**
 * Map the given entry to a integer value.
 * @param node XML entry to map.
 * @return The integer mapped value.
 */
int sctk_runtime_config_map_entry_to_int(xmlNodePtr node)
{
	int res;
  	xmlChar * value = xmlNodeGetContent(node);
	res = atoi((char*)value);
	free(value);
	return res;
}

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to a double value.
 * @param node XML entry to map.
 * @return The double mapped value.
 */
double sctk_runtime_config_map_entry_to_double(xmlNodePtr node)
{
	double res;
	xmlChar * value = xmlNodeGetContent(node);
	res = atof((char*)value);
	xmlFree(value);
	return res;
}

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to a float value.
 * @param node XML entry to map.
 * @return The float mapped value.
 */
float sctk_runtime_config_map_entry_to_float(xmlNodePtr node)
{
	return (float)sctk_runtime_config_map_entry_to_double(node);
}

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to a string value.
 * @param node XML entry to map.
 * @return The string mapped value.
 */
char * sctk_runtime_config_map_entry_to_string(xmlNodePtr node)
{
	char *ret = NULL;
	xmlChar * value = xmlNodeGetContent(node);
	ret = strdup( (char*)value );
	xmlFree(value);

	return ret;
}

/******************************** FUNCTION *********************************/
/**
 * Compare two characters ignoring their cases.
 * @param aa First character to compare.
 * @param bb Second character to compare.
 * @return 1 if characters are equal, 0 otherwise.
 */
int sctk_runtime_config_map_case_cmp( const char aa, const char bb)
{
	char str[4];
	str[0]=aa;
	str[1]='\0';
	str[2]=bb;
	str[3]='\0';

	if( !strcasecmp(str, str+2 ) )
		return 1;
	else
		return 0;
}

/******************************** FUNCTION *********************************/
/**
 * Convert a string value to a size one.
 * @param pval The token to convert (i.e. 50MB, 1KB, etc.).
 * @return The converted string as a size value.
 */
size_t sctk_runtime_config_map_entry_parse_size( const char *pval )
{
	int len;
	char * value = NULL;
	size_t ret = 0;

	len = strlen( pval );
	if( len < 2 ) {
		return ret;
	}

	value = strdup( pval );

	if( len == 2 && sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
		value[len - 1 ]= '\0';
		ret = atoll(value);

	} else {
		if( sctk_runtime_config_map_case_cmp(value[ len - 2 ],'K')
		    && sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
			value[len - 2 ]= '\0';
			ret = atoll(value) * 1024;
		} else if( sctk_runtime_config_map_case_cmp(value[ len - 2 ],'M')
		           && sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
			value[len - 2 ]= '\0';
			ret = atoll(value) * 1024 * 1024;
		} else if( sctk_runtime_config_map_case_cmp(value[ len - 2 ],'G')
		           && sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
			value[len - 2 ]= '\0';
			ret = atoll(value) * 1073741824llu;
		} else if( sctk_runtime_config_map_case_cmp(value[ len - 2 ],'T')
		           && sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
			value[len - 2 ]= '\0';
			ret = atoll(value) * 1099511627776llu;
		} else if( sctk_runtime_config_map_case_cmp(value[ len - 2 ],'P')
		           && sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
			value[len - 2 ]= '\0';
			ret = atoll(value);
			if( 16384 <= ret ) {
				sctk_fatal("%llu PB is close to overflow 64bits values please choose a lower value", (unsigned long long int)ret);
			}
			ret = ret * 1125899906842624llu;
		} else if( sctk_runtime_config_map_case_cmp(value[ len - 1 ],'B') ) {
			value[len - 1 ]= '\0';
			ret = atoll(value);
		} else {
			sctk_fatal("Could not parse size : %s\n", pval);
		}
	}

	free( value );
	return ret;
}

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to a size value.
 * @param node XML entry to map.
 * @return The size mapped value.
 */
size_t sctk_runtime_config_map_entry_to_size(xmlNodePtr node)
{
  size_t ret = 0;
  xmlChar * value = xmlNodeGetContent(node);

  ret = sctk_runtime_config_map_entry_parse_size( (char *)value );

  xmlFree(value);
  return ret;
}

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to a function pointer value.
 * @param node XML entry to map.
 * @return The function pointer mapped value.
 */
struct sctk_runtime_config_funcptr sctk_runtime_config_map_entry_to_funcptr(xmlNodePtr node)
{
  struct sctk_runtime_config_funcptr ret;
  xmlChar * value = xmlNodeGetContent(node);

  ret.name = sctk_runtime_config_map_entry_to_string(node);
  *(void **) &(ret.value) = dlsym(sctk_handler, ret.name);

  xmlFree(value);
  return ret;
}

/******************************** FUNCTION *********************************/
/**
 * Map the given entry to an enum value.
 * @param node XML entry to map.
 * @param type_name Enum which the entry must be converted on.
 * @return The enum mapped value.
 */
int sctk_runtime_config_map_entry_to_enum(xmlNodePtr node, const char * type_name)
{
  int res;
  struct enum_type * current_enum;
  struct enum_value * current_value;

  xmlChar * value = xmlNodeGetContent(node);

  HASH_FIND_STR(enums_types, type_name, current_enum);
  if (current_enum) {
    HASH_FIND_STR(current_enum->values, (char *) value, current_value);
    if (current_value) {
      res = current_value->value;
    }
    else {
      sctk_fatal("Invalid enum value.");
    }
  }
  else {
    sctk_fatal("Invalid enum type.");
  }

  free(value);
  return res;
}

/******************************** FUNCTION *********************************/
/**
 * Get the first child of a struct element, only if it's a param or an array.
 * @param current Struct element on which to get the first child.
 * @return The child if it's a param or an array, NULL otherwise.
 */
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_meta_get_first_child(const struct sctk_runtime_config_entry_meta * current) {
	/* error */
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_STRUCT);

	/* current level */
	current++;

	/* if new level is same, we found next child, otherwise we reach end */
	if (current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY)
		return current;
	else
		return NULL;
}

/******************************** FUNCTION *********************************/
/**
 * Get the next child in a struct element, only if it's a param or an array.
 * @param current Previous child in the struct element on which to get next.
 * @return The child if it's a param or an array, NULL otherwise.
 */
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_meta_get_next_child(const struct sctk_runtime_config_entry_meta * current) {
	/* error */
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY);

	/* current level */
	current++;

	/* if new level is same, we found next child, otherwise we reach end */
	if (current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY)
		return current;
	else
		return NULL;
}

/******************************** FUNCTION *********************************/
/**
 * Get a child (param or array) of a struct element, matching to a name.
 * @param current Struct element on which to get the child.
 * @param name Name of the child to get.
 * @return The child if found, NULL otherwise.
 */
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_get_child_meta(const struct sctk_runtime_config_entry_meta * current,const xmlChar * name) {
	/* errors */
	assert(current != NULL);
	assert(name != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_STRUCT);

	/* loop on childs to get entry */
	current = sctk_runtime_config_meta_get_first_child(current);
	while (current != NULL) {
		if (xmlStrcmp(name,BAD_CAST(current->name)) == 0)
			break;
		current = sctk_runtime_config_meta_get_next_child(current);
	}

	return current;
}

/******************************** FUNCTION *********************************/
/**
 * Get an entry within a struct.
 * @param struct_ptr Struct element on which to get the entry.
 * @param current The meta entry.
 * @return The entry value.
 */
void * sctk_runtime_config_get_entry(sctk_runtime_config_struct_ptr struct_ptr,const struct sctk_runtime_config_entry_meta * current)
{
	/* errors */
	assert(current->type == SCTK_CONFIG_META_TYPE_PARAM || current->type == SCTK_CONFIG_META_TYPE_ARRAY || current->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY);

	/* compute address and return */
	if (current->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY)
		return struct_ptr;
	else
		return ((void*)struct_ptr)+current->offset;
}

/******************************** FUNCTION *********************************/
/**
 * Apply an init handler on a meta entry.
 * @param config_meta Main meta config struct storing meta data.
 * @param struct_ptr Element on which to apply the init handler.
 * @param type_name
 */
void sctk_runtime_config_apply_init_handler(const struct sctk_runtime_config_entry_meta *config_meta, sctk_runtime_config_struct_ptr struct_ptr,const char * type_name)
{
	/* error */
	assert(struct_ptr != NULL);
	assert(type_name != NULL);

	/* search definition */
	const struct sctk_runtime_config_entry_meta * entry;
	entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
	if (entry != NULL && entry->type == SCTK_CONFIG_META_TYPE_STRUCT && entry->extra != NULL)
		((sctk_runtime_config_struct_init_handler)entry->extra)(struct_ptr);
}

/******************************** FUNCTION *********************************/
/**
 * Map an XML node and childs to a C array.
 * @param config_meta Define the root element of the meta description table.
 * @param array Define the pointer to the array pointer in C struct.
 * @param current Define the meta description of the entry, must be a type SCTK_CONFIG_META_TYPE_ARRAY.
 * @param node Define the XML node to map.
**/
void sctk_runtime_config_map_array(const struct sctk_runtime_config_entry_meta *config_meta,
                                   void ** array,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node)
{
	/* vars */
	const xmlChar * entry_name;
	int cnt;
	int * array_size;
	void * element;

	/* errors */
	assert(array != NULL);
	assert(current != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_ARRAY);
	assert(node != NULL);
	assert(xmlStrcmp(node->name,BAD_CAST(current->name)) == 0);
	assert(current->inner_type != NULL);
	assert(current->extra != NULL);

	/* we know that the size counter is next element by definition */
	array_size = (int*)(array+1);

	/* get element size */
	assert(current->size > 0 && current->size < 4096);

	/* check coherency */
	assert((*array_size == 0 && *array == NULL) || (*array_size != 0 && *array != NULL));

	/* delete old one */
	if (*array != NULL)
		free(*array);

	/* get name of sub-elements */
	entry_name = current->extra;

	/* counter number of such child to allocate array memory */
	cnt = sctk_libxml_count_child_nodes(node,entry_name);

	/* if no child, set to NULL */
	if (cnt == 0) {
		*array = NULL;
		*array_size = 0;
	} else {
		/* allocate new one */
		*array = calloc(cnt,current->size);
		*array_size = cnt;

		/* loop on all child nodes */
		int i = 0;
		node = xmlFirstElementChild(node);
		while (node != NULL) {
			/* calc address of current element */
			element = (*array)+i*current->size;

			/* must only find nodes with tage_name = entry_name */
			assume_m(xmlStrcmp(node->name,entry_name) == 0,"Invalid child node in array alement, expect %s -> %s but get %s.",current->name,(const char*)current->extra,node->name);

			/* init the element before mapping the xml node */
			sctk_runtime_config_apply_init_handler(config_meta, element,current->inner_type);

			/* map the xml node on current element */
			sctk_runtime_config_map_value(config_meta, element,current->inner_type,node);

			/* move to next one */
			node = xmlNextElementSibling(node);
			i++;
		}
	}
}

/******************************** FUNCTION *********************************/
/**
 * Map the given XML node to a union type element.
 * @param config_meta Define the root element of the meta description table.
 * @param value Define the pointer to the union value to fill in C struct.
 * @param current Define the meta description of the entry, must be a type SCTK_CONFIG_META_TYPE_UNION.
 * @param node Define the XML node to map.
**/
void sctk_runtime_config_map_union( const struct sctk_runtime_config_entry_meta *config_meta,
                                    void * value,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node)
{
	/* vars */
	const struct sctk_runtime_config_entry_meta * entry;
	xmlNodePtr child;

	/* errors */
	assert(current != NULL);
	assert(node != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_UNION);
	/* by default enum is based on int, checking this in case of .... */
	assert(sizeof(enum sctk_runtime_config_meta_entry_type) == sizeof(int));

	/* get first child */
	child = xmlFirstElementChild(node);

	/* not found */
	if (child == NULL) {
		sctk_warning("Invalid child in union node %s.",current->name);
		return;
	}

	/* skip the union entry itself */
	entry = current+1;
	//search corresponding type in union acceped list
	while (entry->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY && xmlStrcmp(child->name,BAD_CAST(entry->name)) != 0)
		entry++;

	/* not found */
	if (entry->type != SCTK_CONFIG_META_TYPE_UNION_ENTRY)
		sctk_fatal("Invalid entry type in enum %s : %s.",current->name,child->name);

	/* setup type ID */
	*(int*)value = entry->offset;

	/* skip enum to go to bas adress of next element */
	value = sctk_runtime_config_get_union_value_offset(current->name, value );

	/* call function to apply to good type */
	sctk_runtime_config_map_value(config_meta,value,entry->inner_type,child);
}

/******************************** FUNCTION *********************************/
/**
 * Map a value on C structure by using the good type.
 * @param config_meta Define the root element of the meta description table.
 * @param value Define the pointer to the value to fill in C struct.
 * @param type_name Define the type name of the value.
 * @param node Define the XML node to map.
**/
void sctk_runtime_config_map_value( const struct sctk_runtime_config_entry_meta *config_meta, void * value, const char * type_name,xmlNodePtr node)
{
	/* vars */
	bool is_plain_type;
	const struct sctk_runtime_config_entry_meta * entry;

	/* errors */
	assert(value != NULL);
	assert(node != NULL);
	assert(type_name != NULL);

	/* try with plain types */
	is_plain_type = sctk_runtime_config_map_plain_type(value,type_name,node);

	/* if not it's a composed type */
	if ( ! is_plain_type )
	{
		if (!strcmp(type_name, "funcptr"))
		{
			
			*(struct sctk_runtime_config_funcptr*) value = sctk_runtime_config_map_entry_to_funcptr(node);
		}
		else if (!strncmp(type_name, "enum", 4))
		{
			*(int*) value = sctk_runtime_config_map_entry_to_enum(node, type_name);
		}
		else
		{
			/* get the meta description of the type */
			entry = sctk_runtime_config_get_meta_type(config_meta, type_name);

			/* check for errors and types */
			if (entry == NULL)
			{
				sctk_fatal("Can't find type information for : %s.",type_name);
			}
			else if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT)
			{
				sctk_runtime_config_map_struct(config_meta,value,entry,node);
			}
			else if (entry->type == SCTK_CONFIG_META_TYPE_UNION)
			{
				sctk_runtime_config_map_union(config_meta,value,entry,node);
			}
			else
			{
			sctk_fatal("Unknown custom type : %s (%d)",type_name,entry->type);
			}
		}
	}
}

/******************************** FUNCTION *********************************/
/**
 * Try to map as a plain type, if not return false.
 * @param value Define the address of the value to map.
 * @param type_name Define the type name of the value to know how to map.
 * @param node Define the XML node in which to take the value.
**/
bool sctk_runtime_config_map_plain_type(void * value, const char * type_name,xmlNodePtr node)
{
	/* errors */
	assert(value != NULL);
	assert(type_name != NULL);
	assert(node != NULL);

	/* test all plain types
	   check type name */
	if (strcmp(type_name,"int") == 0) {
		*(int*)value = sctk_runtime_config_map_entry_to_int(node);
		return true;
	} else if (strcmp(type_name,"bool") == 0) {
		*(bool*)value = sctk_runtime_config_map_entry_to_bool(node);
		return true;
	} else if (strcmp(type_name,"double") == 0) {
		*(double*)value = sctk_runtime_config_map_entry_to_double(node);
		return true;
	} else if (strcmp(type_name,"float") == 0) {
		*(float*)value = sctk_runtime_config_map_entry_to_float(node);
		return true;
	} else if (strcmp(type_name,"char *") == 0) {
		*((char **)value) = sctk_runtime_config_map_entry_to_string(node);
		return true;
	} else if (strcmp(type_name,"size_t") == 0) {
		*((size_t *)value) = sctk_runtime_config_map_entry_to_size(node);
		return true;
	} else {
		return false;
	}
}

/******************************** FUNCTION *********************************/
/**
 * Map a given XML node structure to the C config struct.
 * @param config_meta Define the root element of the meta description table.
 * @param struct_ptr Define the pointer of the C structure to fill.
 * @param current Define the meta description related to current node.
 * @param node Define the XML node to map.
**/
void sctk_runtime_config_map_struct( const struct sctk_runtime_config_entry_meta *config_meta,
                                     void * struct_ptr,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node)
{
	/* child */
	const struct sctk_runtime_config_entry_meta * child;
	void * child_ptr;

	/* errors */
	assert(current != NULL);
	assert(node != NULL);
	assert(current->type == SCTK_CONFIG_META_TYPE_STRUCT);

	/* Here we make sure that every struct we encounter has already been initialized
	 * once with the default content. This is needed as for example rails and
	 * drivers are defined dynamically, and cannot be filled statically */
	sctk_runtime_config_reset_struct_default_if_needed(current->name, struct_ptr );

	/* loop on all parameters of the struct */
	node = xmlFirstElementChild(node);
	while (node != NULL) {
		/* get meta data of child */
		child = sctk_runtime_config_get_child_meta(current,node->name);
		assume_m(child != NULL,"Can't find meta data for node : %s",node->name);

		/* get pointer of entry in c struct */
		child_ptr = sctk_runtime_config_get_entry(struct_ptr,child);
		assume_m(child_ptr != NULL,"Can't find meta data for node entry : %s",node->name);

		/* apply the good function */
		switch(child->type) {
			case SCTK_CONFIG_META_TYPE_ARRAY:
				sctk_runtime_config_map_array(config_meta,child_ptr,child,node);
				break;
			case SCTK_CONFIG_META_TYPE_PARAM:
				sctk_runtime_config_map_value(config_meta,child_ptr,child->inner_type,node);
				break;
			default:
				sctk_fatal("Invalid current meta entry type : %d",current->type);
		}

		/* move to next lement */
		node = xmlNextElementSibling(node);
	}
}

/******************************** FUNCTION *********************************/
/**
 * Search and return the entry in meta description table which correspond the given type name.
 * It work only for root type definition, so for structures and union.
 * @param config_meta Root element of the meta description table.
 * @param name Define the name of the type to search in the table.
**/
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_get_meta_type( const struct sctk_runtime_config_entry_meta *config_meta, const char * name) {
	/* vars */
	const struct sctk_runtime_config_entry_meta * entry = config_meta;

	/* error */
	assert(name != NULL);

	/* search */
	while (entry->type != SCTK_CONFIG_META_TYPE_END &&
	       ((entry->type != SCTK_CONFIG_META_TYPE_STRUCT && entry->type != SCTK_CONFIG_META_TYPE_UNION)
	        || strcmp(name,entry->name) != 0))
		entry++;

	/* if end, not found */
	if (entry->type == SCTK_CONFIG_META_TYPE_END) {
		return NULL;
	} else {
		assert(strcmp(name,entry->name) == 0);
		assert(entry->type == SCTK_CONFIG_META_TYPE_STRUCT || entry->type == SCTK_CONFIG_META_TYPE_UNION);
		return entry;
	}
}

/******************************** FUNCTION *********************************/
/**
 * Retrieve symbol from dynamic library.
 * @param symbol_name The name of the symbol to load.
 * @return If no error, return the loaded symbol.
 */
void* sctk_runtime_config_get_symbol(char * symbol_name)
{
  void * symbol = dlsym(sctk_handler, symbol_name);
  if (symbol == NULL && sctk_crash_on_symbol_load) {
    char * msg = dlerror();
    sctk_warning("Fail to load config symbol %s : %s", symbol_name, msg);
  }
  
  return symbol;
}

/******************************** FUNCTION *********************************/
/** @TODO Generate this methode from XML files. **/
void sctk_runtime_config_do_cleanup(__UNUSED__ struct sctk_runtime_config* config)
{

}
