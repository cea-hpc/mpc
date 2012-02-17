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

#ifndef SCTK_RUNTIME_CONFIG_PRINTER
#define SCTK_RUNTIME_CONFIG_PRINTER

/********************  HEADERS  *********************/
#include "sctk_runtime_config_mapper.h"

/*******************  FUNCTION  *********************/
const struct sctk_runtime_config_entry_meta * sctk_runtime_config_array_get_inner_type(const struct sctk_runtime_config_entry_meta * current);
void sctk_runtime_config_display_array(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,
                               void ** value,const struct sctk_runtime_config_entry_meta * current,int level);
void sctk_runtime_config_display_entry(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,sctk_runtime_config_struct_ptr struct_ptr,
                               const struct sctk_runtime_config_entry_meta * current,int level);
void sctk_runtime_config_display_struct(const struct sctk_runtime_config_entry_meta * config_meta, struct sctk_runtime_config * config,sctk_runtime_config_struct_ptr struct_ptr,const char * type_name,int level);
void sctk_runtime_config_display_indent(int level);


#endif //SCTK_CONFIG_PRINTER
