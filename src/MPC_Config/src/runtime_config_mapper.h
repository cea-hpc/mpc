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

#ifndef SCTK_RUNTIME_CONFIG_MAPPER_H
#define SCTK_RUNTIME_CONFIG_MAPPER_H

/********************************* INCLUDES *********************************/
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "runtime_config_sources.h"
#include "mpc_config_struct.h"

/********************************* GLOBALS **********************************/
extern void * sctk_handler;
const char * sctk_cst_string_undefined;

/******************************** STRUCTURE *********************************/
/**
 * Enum to define internal types supported for meta-data structure.
 * @brief Types supported for meta-description entries.
**/
enum sctk_runtime_config_meta_entry_type {
    /** The entry describe a parameter of struct. **/
    SCTK_CONFIG_META_TYPE_PARAM,
    /** Parameter defined as arrays of basic or user types. **/
    SCTK_CONFIG_META_TYPE_ARRAY,
    /** Composed C structure. **/
    SCTK_CONFIG_META_TYPE_STRUCT,
    /** Define a union type. **/
    SCTK_CONFIG_META_TYPE_UNION,
    /** Define an entry in union type. **/
    SCTK_CONFIG_META_TYPE_UNION_ENTRY,
    /** Last entry to close the meta description table. **/
    SCTK_CONFIG_META_TYPE_END
};

/********************************** TYPES ***********************************/
typedef void * sctk_runtime_config_struct_ptr;
/** Declare handler init type function on struct. **/
typedef void(*sctk_runtime_config_struct_init_handler)(sctk_runtime_config_struct_ptr struct_ptr);

/******************************** STRUCTURE *********************************/
/**
 * Structure used to instantiate the meta-description table to map the XML file onto the C struct.
 * @brief Structure for the entries of meta-description table.
**/
struct sctk_runtime_config_entry_meta {
	/** Name of the field. **/
	const char * name;
	/** Type of the field. **/
	enum sctk_runtime_config_meta_entry_type type;
	/** Offset of the field in parent structure (in bytes). **/
	unsigned long offset;
	/** Size of the field type (size of the pointer for arrays).**/
	int size;
	/** Pointer to the name of inner type if not simple one, NULL for basic types. **/
	const char * inner_type;
	/**
	 * Extra pointer information, serve mostly for arrays to store the name of sub-entries.
	 * For struct, store the init handler.
	**/
	void * extra;
};

/********************************* MACROS ***********************************/
/** @TODO remove this by the one from standard, offsetof or similar. **/
#define sctk_runtime_config_get_offset_of_member(type,member) ((unsigned long)&(((type*)NULL)->member))

/********************************* FUNCTION *********************************/
/* mapping functions */
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_get_meta_type( const struct sctk_runtime_config_entry_meta *config_meta, const char * name);
const struct sctk_runtime_config_entry_meta *sctk_runtime_config_get_meta(
    const struct sctk_runtime_config_entry_meta *config_meta, const char *name);
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_meta_get_first_child(const struct sctk_runtime_config_entry_meta * current);
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_meta_get_next_child(const struct sctk_runtime_config_entry_meta * current);
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_get_child_meta(const struct sctk_runtime_config_entry_meta * current,const xmlChar * name);
void * sctk_runtime_config_get_entry(sctk_runtime_config_struct_ptr struct_ptr,const struct sctk_runtime_config_entry_meta * current);
void sctk_runtime_config_apply_init_handler(const struct sctk_runtime_config_entry_meta *config_meta, sctk_runtime_config_struct_ptr struct_ptr,const char * type_name);
void sctk_runtime_config_map_array(const struct sctk_runtime_config_entry_meta *config_meta,
                                   void ** array,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node);
void sctk_runtime_config_map_value(
    const struct sctk_runtime_config_entry_meta *config_meta, void *value,
    const char *type_name, xmlNodePtr node);
bool sctk_runtime_config_map_plain_type(void * value, const char * type_name,xmlNodePtr node);
void sctk_runtime_config_map_struct( const struct sctk_runtime_config_entry_meta *config_meta,
                                     void * struct_ptr,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node);
void sctk_runtime_config_map_union( const struct sctk_runtime_config_entry_meta *config_meta,
                                    void * value,const struct sctk_runtime_config_entry_meta * current,xmlNodePtr node);
void* sctk_runtime_config_get_symbol(char * symbol_name);
/********************************* FUNCTION *********************************/
/* type supports */
int sctk_runtime_config_map_entry_to_int(xmlNodePtr node);
bool sctk_runtime_config_map_entry_to_bool(xmlNodePtr node);
double sctk_runtime_config_map_entry_to_double(xmlNodePtr node);
float sctk_runtime_config_map_entry_to_float(xmlNodePtr node);
size_t sctk_runtime_config_map_entry_parse_size( const char *value );
size_t sctk_runtime_config_map_entry_to_size(xmlNodePtr node);
struct sctk_runtime_config_funcptr sctk_runtime_config_map_entry_to_funcptr(xmlNodePtr node);
int sctk_runtime_config_map_entry_to_enum(xmlNodePtr node, const char * type_name);

/********************************* FUNCTION *********************************/
void sctk_runtime_config_do_cleanup(struct sctk_runtime_config * config);


/* MPIT Support */
void sctk_runtime_config_mpit_bind_variable_set_trampoline( void (*trampoline)(char *, size_t, void *));
void sctk_runtime_config_mpit_bind_variable(char *name, size_t size, void * data_addr);

#endif /*SCTK_RUNTIME_CONFIG_MAPPER_H*/