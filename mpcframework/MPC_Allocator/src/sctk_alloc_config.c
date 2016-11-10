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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include <string.h>
#include "sctk_alloc_config.h"
#include "sctk_alloc_debug.h"

#ifdef MPC_Common
#include "sctk_runtime_config_struct_defaults.h"
#endif //MPC_Common

/************************* GLOBALS *************************/
#ifndef MPC_Common
struct sctk_runtime_config_struct_allocator sctk_alloc_global_config;
#endif //MPC_Common

/************************* FUNCTION ************************/
const char *sctk_alloc_bool_to_str(bool value) {
  if (value)
    return "true";
  else
    return "false";
}

/************************* FUNCTION ************************/
#ifndef MPC_Common
void sctk_alloc_print_config(void) {
  printf("=============== MPC ALLOC CONFIG =================\n");
  printf("MPCALLOC_PRINT_CONFIG       : %s\n",
         sctk_alloc_bool_to_str(sctk_alloc_global_config.print_config));
  printf("MPCALLOC_NUMA_STRICT        : %s\n",
         sctk_alloc_bool_to_str(sctk_alloc_global_config.strict));
  printf("MPCALLOC_NUMA_MIGRATION     : %s\n",
         sctk_alloc_bool_to_str(sctk_alloc_global_config.numa_migration));
  printf("MPCALLOC_NUMA_ROUND_ROBIN   : %s\n",
         sctk_alloc_bool_to_str(sctk_alloc_global_config.numa_round_robin));
  printf("MPCALLOC_KEEP_MAX           : %lu MB\n",
         sctk_alloc_global_config.keep_max / 1024 / 1024);
  printf("MPCALLOC_KEEP_MEM           : %lu MB\n",
         sctk_alloc_global_config.keep_mem / 1024 / 1024);
  printf("MPCALLOC_REALLOC_THREASHOLD : %lu MB\n",
         sctk_alloc_global_config.realloc_threashold / 1024 / 1024);
  printf("MPCALLOC_REALLOC_FACTOR     : %u\n",
         sctk_alloc_global_config.realloc_factor);
  printf("MPCALLOC_MM_SOURCES         : %d\n",
         sctk_alloc_global_config.mm_sources);
  printf("==================================================\n");
}
#endif //MPC_Common

/************************* FUNCTION ************************/
/**
 * !!!! CAUTION !!!! This method has no allocator, so it musn't do any allocation.
**/
void sctk_alloc_config_init_static_defaults(
    struct sctk_runtime_config_struct_allocator *config) {
#ifndef MPC_Common
  config->print_config = false;
  config->numa_round_robin = false;
  config->mm_sources = 4;
#endif // MPC_Common

  config->strict = false;
  config->numa_migration = false;
  config->numa = false;
  config->realloc_factor = 2;
  config->realloc_threashold = 50 * 1024 * 1024; // 50MB
  config->keep_max = 8 * 1024 * 1024;            // 8MB
  config->keep_mem = 512 * 1024 * 1024;          // 512MB

#if !defined(MPC_Common) && defined(__MIC__)
  config->numa_round_robin = true;
#endif

#ifdef __MIC__
  config->keep_mem = 64 * 1024 * 1024; // 64MB
#endif

#ifdef HAVE_HWLOC
  /** @todo set true when no mode bugs out of MPC. **/
  config->numa = false;
#endif // HAVE_HWLOC
}

/************************* FUNCTION ************************/
bool sctk_alloc_get_bool_from_env(const char *var_name, bool default_value) {
  // vars
  char *buffer;

  // errors
  assert(var_name != NULL);

  // read env
  buffer = getenv(var_name);

  // convert value of return default
  if (buffer == NULL || *buffer == '\0') {
    return default_value;
  } else if (strcmp(buffer, "true") == 0) {
    return true;
  } else if (strcmp(buffer, "false") == 0) {
    return false;
  } else {
    sctk_fatal(
        "Invalide value of %s environement variable, expect 'true' or 'false'.",
        var_name);
    return default_value;
  }
}

/************************* FUNCTION ************************/
sctk_size_t sctk_alloc_get_size_from_env(const char *var_name,
                                         sctk_size_t default_value,
                                         sctk_size_t factor) {
  // vars
  char *buffer;

  // errors
  assert(var_name != NULL);

  // read env
  buffer = getenv(var_name);

  // convert value of return default
  if (buffer == NULL || *buffer == '\0')
    return default_value;
  else
    return atol(buffer) * factor;
}

/************************* FUNCTION ************************/
/**
 * This method is called to force config initialization just after egg_allocator init steps.
 * It carn perform allocation, so we can use the MPC config system here.
**/
#ifdef MPC_Common
void sctk_alloc_config_init(void)
{
	sctk_runtime_config_init();
}
#else //MPC_Common
void sctk_alloc_config_init(void)
{
	sctk_alloc_global_config.print_config = sctk_alloc_get_bool_from_env("MPCALLOC_PRINT_CONFIG",sctk_alloc_global_config.print_config);
	sctk_alloc_global_config.strict = sctk_alloc_get_bool_from_env("MPCALLOC_NUMA_STRICT",sctk_alloc_global_config.strict);
	sctk_alloc_global_config.numa_round_robin = sctk_alloc_get_bool_from_env("MPCALLOC_NUMA_ROUND_ROBIN",sctk_alloc_global_config.numa_round_robin);
	sctk_alloc_global_config.numa_migration = sctk_alloc_get_bool_from_env("MPCALLOC_NUMA_MIGRATION",sctk_alloc_global_config.numa_migration);
	sctk_alloc_global_config.keep_max = sctk_alloc_get_size_from_env("MPCALLOC_KEEP_MAX",sctk_alloc_global_config.keep_max,1024*1024);
	sctk_alloc_global_config.keep_mem = sctk_alloc_get_size_from_env("MPCALLOC_KEEP_MEM",sctk_alloc_global_config.keep_mem,1024*1024);
	sctk_alloc_global_config.realloc_threashold = sctk_alloc_get_size_from_env("MPCALLOC_REALLOC_THREASHOLD",sctk_alloc_global_config.realloc_threashold,1024*1024);
	sctk_alloc_global_config.realloc_factor = sctk_alloc_get_size_from_env("MPCALLOC_REALLOC_FACTOR",sctk_alloc_global_config.realloc_factor,1);
	sctk_alloc_global_config.mm_sources = sctk_alloc_get_size_from_env("MPCALLOC_MM_SOURCES",sctk_alloc_global_config.mm_sources,1);
	if (sctk_alloc_global_config.print_config)
		sctk_alloc_print_config();
}
#endif //MPC_Common

/************************* FUNCTION ************************/
/**
 * This init method is used before egg allocator, it must not perform any allocation
 * as allocator is not ready when calling this. It must only put default fixed
 * values.
 * Values will be erased after egg initialization with final user values. This is
 * why we must only use parameters which can be replaced on fly or which didn't impact
 * egg_allocator.
**/
#ifdef MPC_Common
void sctk_alloc_config_egg_init(void)
{
	//TODO : Damn, with MPC, we may use sctk_runtime_config_struct_init_allocator() but can't due to
	//usage of strdup in sctk_runtime_config_map_entry_parse_size(), may be we can avoid this
	sctk_alloc_config_init_static_defaults(&__sctk_global_runtime_config__.modules.allocator);
}
#else //MPC_Common
void sctk_alloc_config_egg_init(void)
{
	sctk_alloc_config_init_static_defaults(&sctk_alloc_global_config);
}
#endif //MPC_Common
