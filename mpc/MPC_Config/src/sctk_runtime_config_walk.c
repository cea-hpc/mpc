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
#include <string.h>
#include <sctk_debug.h>
#include "sctk_runtime_config_walk.h"

/********************************* FUNCTION *********************************/
/**
 * Walk over all child of a composed configuration union.
 * @param config_meta Define the address of base element of meta description table.
 * @param handler User function to call on each node of the structure.
 * @param name Name of the field.
 * @param type_name Type name of the field (name of the union).
 * @param union_ptr Pointer to the union value.
 * @param level Inclusion level in the structure tree.
 * @param opt Optional value to transmit to the user handler.
 */
void sctk_runtime_config_walk_union(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                    const char * name,const char * type_name,void * union_ptr,int level,void * opt)
{
	/* vars */
	const struct sctk_runtime_config_entry_meta * entry;
	int * union_type_id = union_ptr;
	void * union_data = union_ptr + sizeof(int);

	/* error */
	assert(type_name != NULL);
	assert(union_ptr != NULL);
	assert(sizeof(enum sctk_runtime_config_meta_entry_type) == sizeof(int));
	assert(*union_type_id < 64);

	/* find meta entry for type */
	entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
	assert(entry != NULL);
	assert(entry->type == SCTK_CONFIG_META_TYPE_UNION);

	/* get real child, id is given by enum value, and as we were on SCTK_CONFIG_META_TYPE_UNION entry
	   of meta description table, we known that we need to increment to get the good one. */
	entry += *union_type_id;

	/* call handler for open */
	handler(SCTK_RUNTIME_CONFIG_WALK_UNION,name,type_name,union_ptr,SCTK_RUNTIME_CONFIG_WALK_OPEN,entry,level,opt);

	/* if non type id, get empty value */
	if (*union_type_id != 0) {
		/* check */
		assume_m(entry->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY,"Invalid union entry type : %d.",entry->type,opt);

		/* walk on the value by using good type */
		sctk_runtime_config_walk_value(config_meta,handler,entry->name,entry->inner_type,union_data,level,opt);
	}

	/* call handler for open */
	handler(SCTK_RUNTIME_CONFIG_WALK_UNION,name,type_name,union_ptr,SCTK_RUNTIME_CONFIG_WALK_CLOSE,entry,level,opt);
}

/********************************* FUNCTION *********************************/
/**
 * Walk over all child of a composed configuration struct.
 * @param config_meta Define the address of base element of meta description table.
 * @param handler User function to call on each node of the structure.
 * @param name Name of the field.
 * @param type_name Type name of the field (name of the union).
 * @param struct_ptr Pointer to the structure value.
 * @param level Inclusion level in the structure tree.
 * @param opt Optional value to transmit to the user handler.
 */
void sctk_runtime_config_walk_struct(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                     const char * name,const char * type_name,void * struct_ptr,int level,void * opt)
{
	/* vars */
	const struct sctk_runtime_config_entry_meta * entry;
	const struct sctk_runtime_config_entry_meta * child;
	void * value = struct_ptr;

	/* error */
	assert(config_meta != NULL);
	assert(type_name != NULL);
	assert(struct_ptr != NULL);

	/* find meta entry for type */
	entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
	assert(entry != NULL);
	assert(entry->type == SCTK_CONFIG_META_TYPE_STRUCT);

	/* call handler for open */
	handler(SCTK_RUNTIME_CONFIG_WALK_STRUCT,name,type_name,struct_ptr,SCTK_RUNTIME_CONFIG_WALK_OPEN,entry,level,opt);

	/* get first child (params or arrays) */
	child = sctk_runtime_config_meta_get_first_child(entry);

	/* loop unit last one */
	while (child != NULL) {
		/* get value */
		value = sctk_runtime_config_get_entry(struct_ptr,child);
		assert(value != NULL);

		/* walk on value depending on type */
		if (child->type == SCTK_CONFIG_META_TYPE_ARRAY) {
			sctk_runtime_config_walk_array(config_meta, handler, child->name,child,value,level+1,opt);
		} else {
			assert(child->type == SCTK_CONFIG_META_TYPE_PARAM || child->type == SCTK_CONFIG_META_TYPE_UNION_ENTRY);
			sctk_runtime_config_walk_value(config_meta, handler, child->name, child->inner_type,value,level+1,opt);
		}

		/* move to next */
		child = sctk_runtime_config_meta_get_next_child(child);
	}

	/* call handler for close */
	handler(SCTK_RUNTIME_CONFIG_WALK_STRUCT,name,type_name,struct_ptr,SCTK_RUNTIME_CONFIG_WALK_CLOSE,entry,level,opt);
}

/********************************* FUNCTION *********************************/
/**
 * Walk over all child of a composed configuration array.
 * @param config_meta Define the address of base element of meta description table.
 * @param handler User function to call on each node of the array.
 * @param name Name of the field.
 * @param current_meta Meta description of the array.
 * @param struct_ptr Pointer to the structure value.
 * @param level Inclusion level in the structure tree.
 * @param opt Optional value to transmit to the user handler.
 */
void sctk_runtime_config_walk_array(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                    const char * name,const struct sctk_runtime_config_entry_meta * current_meta,void ** value,int level,void * opt)
{
	/* vars */
	int size;
	int i;

	/* error */
	assert(config_meta != NULL);
	assert(value != NULL);
	assert(current_meta != NULL);
	assert(current_meta->type == SCTK_CONFIG_META_TYPE_ARRAY);
	assert(current_meta->inner_type != NULL);
	assert(current_meta->extra != NULL);

	/* size is next element by construction (see generator of struct file) so : */
	size = *(int*)(value+1);

	/* call handler open */
	handler(SCTK_RUNTIME_CONFIG_WALK_ARRAY,name,current_meta->inner_type,value,SCTK_RUNTIME_CONFIG_WALK_OPEN,current_meta,level,opt);

	/* if empty */
	if (*value == NULL) {
		assert(size == 0);
	} else {
		assert(size >= 0);

		/* loop on all elements */
		for (i = 0 ; i < size ; i++) {
			/* compute address of current one */
			void * element = ((char*)(*value)) + i * current_meta->size;

			/* walk on the value */
			sctk_runtime_config_walk_value(config_meta,handler,(const char*)current_meta->extra,current_meta->inner_type,element,level+1,opt);
		}
	}

	/* call handler close */
	handler(SCTK_RUNTIME_CONFIG_WALK_ARRAY,name,current_meta->inner_type,value,SCTK_RUNTIME_CONFIG_WALK_CLOSE,current_meta,level,opt);
}

/********************************* FUNCTION *********************************/
/**
 * Walk over all child of a composed configuration value if composed, otherwise simply call the user handler.
 * @param config_meta Define the address of base element of meta description table.
 * @param handler User function to call on each node of the value.
 * @param name Name of the field.
 * @param type_name Type name of the field.
 * @param struct_ptr Pointer to the structure value.
 * @param level Inclusion level in the structure tree.
 * @param opt Optional value to transmit to the user handler.
 */
void sctk_runtime_config_walk_value(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                    const char * name, const char * type_name,void * value, int level,void * opt)
{
  /* vars */
  const struct sctk_runtime_config_entry_meta * entry;

  /* error */
  assert(config_meta != NULL);
  assert(type_name != NULL);
  assert(value != NULL);

  /* handle the basic type case */
  if ( sctk_runtime_config_is_basic_type( type_name ) ) {
    handler(SCTK_RUNTIME_CONFIG_WALK_VALUE,name,type_name,value,SCTK_RUNTIME_CONFIG_WALK_OPEN,NULL,level,opt);
    handler(SCTK_RUNTIME_CONFIG_WALK_VALUE,name,type_name,value,SCTK_RUNTIME_CONFIG_WALK_CLOSE,NULL,level,opt);
  } else if (!strcmp(type_name, "funcptr")) {
    struct sctk_runtime_config_funcptr * funcptr = (struct sctk_runtime_config_funcptr *) value;
    handler(SCTK_RUNTIME_CONFIG_WALK_VALUE,name,"char *",&funcptr->name,SCTK_RUNTIME_CONFIG_WALK_OPEN,NULL,level,opt);
    handler(SCTK_RUNTIME_CONFIG_WALK_VALUE,name,"char *",&funcptr->name,SCTK_RUNTIME_CONFIG_WALK_CLOSE,NULL,level,opt);
  } else if (!strncmp(type_name, "enum", 4)) {
    struct enum_type * current_enum, *s1, *s2;
    struct enum_value * iter_enum, * tmp;
    int current_value;
    char * enum_name = NULL;

    HASH_FIND_STR(enums_types, type_name, current_enum);
    if (current_enum) {
      current_value = *(int *) value;
      HASH_ITER(hh, current_enum->values, iter_enum, tmp) {
        if (iter_enum->value == current_value) {
          enum_name = (char *) malloc(sizeof(iter_enum->name));
          strcpy(enum_name, iter_enum->name);
          break;
        }
      }
    }
    else {
      sctk_fatal("Invalid enum type : %s.", type_name);
    }

    handler(SCTK_RUNTIME_CONFIG_WALK_VALUE,name,"char *",&enum_name,SCTK_RUNTIME_CONFIG_WALK_OPEN,NULL,level,opt);
    handler(SCTK_RUNTIME_CONFIG_WALK_VALUE,name,"char *",&enum_name,SCTK_RUNTIME_CONFIG_WALK_CLOSE,NULL,level,opt);
  } else {
    /* get meta data of the entry */
    entry = sctk_runtime_config_get_meta_type(config_meta, type_name);
    assert(entry != NULL);
    /* apply related display method. */
    if (entry->type == SCTK_CONFIG_META_TYPE_STRUCT) {
      sctk_runtime_config_walk_struct(config_meta, handler, name, type_name, value, level,opt);
    }
    else if (entry->type == SCTK_CONFIG_META_TYPE_UNION) {
      sctk_runtime_config_walk_union(config_meta, handler, name , type_name, value,level,opt);
    }
    else {
      sctk_fatal("Invalid type.");
    }
  }
}

/********************************* FUNCTION *********************************/
/**
 * Walk over all child of a composed configuration struct.
 * @param config_meta Define the address of base element of meta description table.
 * @param handler User function to call on each node of the structure.
 * @param name Display name of the root element.
 * @param type_name Type name of the root structure (caution it must be a structure, not union).
 * @param struct_ptr Pointer to the structure value.
 * @param opt Optional value to transmit to the user handler.
 */
void sctk_runtime_config_walk_tree(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                   const char * name, const char * root_struct_name,void * root_struct,void * opt)
{
	sctk_runtime_config_walk_struct(config_meta,handler,name,root_struct_name,root_struct,0,opt);
}

/********************************* FUNCTION *********************************/
/**
 * Test if the given type name is a basic one or not.
 * @param type_name The type name to test (int, double ...)
**/
bool sctk_runtime_config_is_basic_type(const char * type_name)
{
	return (strcmp(type_name,"int")    == 0   || strcmp(type_name,"bool")   == 0
	        ||  strcmp(type_name,"double") == 0   || strcmp(type_name,"char *") == 0
	        ||  strcmp(type_name,"float" ) == 0   || strcmp(type_name,"size_t" ) == 0);
}
