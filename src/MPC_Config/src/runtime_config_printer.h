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

#ifndef SCTK_RUNTIME_CONFIG_PRINTER_H
#define SCTK_RUNTIME_CONFIG_PRINTER_H

/********************************* INCLUDES *********************************/
#include "runtime_config_mapper.h"
#include "runtime_config_walk.h"

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config_display_state {
	bool is_simple_array;
};

/********************************* FUNCTION *********************************/
/* helpers */
void sctk_runtime_config_display_indent(int level);
bool sctk_runtime_config_display_plain_type( const char * type_name,void *value);
void sctk_runtime_config_display_handler(enum sctk_runtime_config_walk_type type,
        const char * name,
        const char * type_name,
        void * value,
        enum sctk_runtime_config_walk_status status,
        const struct sctk_runtime_config_entry_meta * type_meta,
        int level,
        void * opt);

/********************************* FUNCTION *********************************/
/* entry point */
void sctk_runtime_config_display_tree(const struct sctk_runtime_config_entry_meta * config_meta,
                                      const char * root_name,
                                      const char * root_struct_name,
                                      void * root_struct);


#endif /*SCTK_RUNTIME_CONFIG_PRINTER_H*/
