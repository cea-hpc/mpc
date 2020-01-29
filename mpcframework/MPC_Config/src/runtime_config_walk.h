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

#ifndef SCTK_RUNTIME_CONFIG_WALK_H
#define SCTK_RUNTIME_CONFIG_WALK_H

/********************************* INCLUDES *********************************/
#include "runtime_config_mapper.h"

/******************************** STRUCTURE *********************************/
/**
 * Open/Close status on the current node of configuration structure.
 */
enum sctk_runtime_config_walk_status {
	SCTK_RUNTIME_CONFIG_WALK_OPEN,
	SCTK_RUNTIME_CONFIG_WALK_CLOSE
};

/******************************** STRUCTURE *********************************/
/**
 * Structure to define the type of node we are walking on while calling the
 * user handler.
 */
enum sctk_runtime_config_walk_type {
    SCTK_RUNTIME_CONFIG_WALK_UNION,
    SCTK_RUNTIME_CONFIG_WALK_STRUCT,
    SCTK_RUNTIME_CONFIG_WALK_ARRAY,
    SCTK_RUNTIME_CONFIG_WALK_VALUE,
};

/********************************** TYPES ***********************************/
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
typedef void (*sckt_runtime_walk_handler)(enum sctk_runtime_config_walk_type type,const char * name,
        const char * type_name,
        void * value,
        enum sctk_runtime_config_walk_status status,
        const struct sctk_runtime_config_entry_meta * type_meta,
        int level,
        void * opt);

/********************************* FUNCTION *********************************/
/* type specific walk functions */
void sctk_runtime_config_walk_union(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                    const char * name,const char * type_name,void * union_ptr,int level,void * opt);
void sctk_runtime_config_walk_struct(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                     const char * name,const char * type_name,void * struct_ptr,int level,void * opt);
void sctk_runtime_config_walk_array(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                    const char * name,const struct sctk_runtime_config_entry_meta * current_meta,void ** value,int level,void * opt);
void sctk_runtime_config_walk_value(const struct sctk_runtime_config_entry_meta * config_meta,sckt_runtime_walk_handler handler,
                                    const char * name, const char * type_name,void * value, int level,void * opt);

/********************************* FUNCTION *********************************/
/* the entry point */
void sctk_runtime_config_walk_tree(const struct sctk_runtime_config_entry_meta * config_meta,
                                   sckt_runtime_walk_handler handler,
                                   const char * name,
                                   const char * root_struct_name,
                                   void * root_struct,
                                   void * opt);

/********************************* FUNCTION *********************************/
/* some helper functions */
bool sctk_runtime_config_is_basic_type(const char * type_name);

#endif /* SCTK_RUNTIME_CONFIG_WALK_H */
