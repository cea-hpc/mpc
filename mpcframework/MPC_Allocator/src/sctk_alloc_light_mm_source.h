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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_MM_SOURCE_LIGHT_H
#define SCTK_ALLOC_MM_SOURCE_LIGHT_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#ifdef HAVE_HWLOC
#include <hwloc.h>
#endif //HAVE_HWLOC
#include <opa_primitives.h>
#include "sctk_alloc_mmsrc.h"
#include "sctk_alloc_lock.h"

/************************** CONSTS *************************/
#define SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE -1

/*************************** ENUM **************************/
/**
 * Flags to enable some function of light memory sources.
**/
enum sctk_alloc_mm_source_light_flags
{
	/** Default state. **/
	SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT      = 0x0,
	/** Enable NUMA strict which force memory bindings. **/
	SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT  = 0x1,
};

/************************** STRUCT *************************/
/**
 * Structure a a free macro bloc managed by the light memory source
 * for future resuse.
**/
struct sctk_alloc_mm_source_light_free_macro_bloc
{
	/** Next free bloc to build the chained list. **/
	struct sctk_alloc_mm_source_light_free_macro_bloc * next;
	/** Size of the current macro bloc (accounting the header). **/
	sctk_size_t size;
};

/************************** STRUCT *************************/
/**
 * Structure of the memory source.
**/
struct sctk_alloc_mm_source_light
{
	/** Inheritance of the basic memory source commonb structure. **/
	struct sctk_alloc_mm_source source;
	/** List of free bloc to reuse. **/
	struct sctk_alloc_mm_source_light_free_macro_bloc * cache;
	/** Flags to setup some optional behaviors. **/
	enum sctk_alloc_mm_source_light_flags mode;
	/** Spinlock to protect the access to cache and counter. **/
	sctk_alloc_spinlock_t spinlock;
	/** The NUMA node of the memory source, -1 if none. **/
	int numa_node;
	/** Enable strict NUMA binding. **/
	bool strict_numa_bind;
	/** More for debug, to know how-many blocks are in use. **/
	OPA_int_t counter;
	/** Current amount of memory stored in mm source cache. **/
	size_t size;
	#ifdef HAVE_HWLOC
	/** Define the nodset related to the NUMA binding for hwloc.**/
	hwloc_nodeset_t nodeset;
	#endif
};

/************************* FUNCTION ************************/
//main functions to manipulate the struct
void sctk_alloc_mm_source_light_init(struct sctk_alloc_mm_source_light * source,int numa_node,enum sctk_alloc_mm_source_light_flags mode);
void sctk_alloc_mm_source_light_reg_in_cache(
    struct sctk_alloc_mm_source_light *light_source,
    struct sctk_alloc_mm_source_light_free_macro_bloc *free_bloc);
struct sctk_alloc_mm_source_light_free_macro_bloc *
sctk_alloc_mm_source_light_setup_free_macro_bloc(void *ptr, sctk_size_t size);
struct sctk_alloc_mm_source_light_free_macro_bloc *
sctk_alloc_mm_source_light_to_free_macro_bloc(
    struct sctk_alloc_macro_bloc *macro_bloc);
struct sctk_alloc_macro_bloc *sctk_alloc_mm_source_light_to_macro_bloc(
    struct sctk_alloc_mm_source_light_free_macro_bloc *free_bloc);
struct sctk_alloc_macro_bloc *sctk_alloc_mm_source_light_find_in_cache(
    struct sctk_alloc_mm_source_light *light_source, sctk_size_t size);
extern void
sctk_alloc_mm_source_light_cleanup(struct sctk_alloc_mm_source *source);
extern struct sctk_alloc_macro_bloc *
sctk_alloc_mm_source_light_request_memory(struct sctk_alloc_mm_source *source,
                                          sctk_ssize_t size);
extern void
sctk_alloc_mm_source_light_free_memory(struct sctk_alloc_mm_source *source,
                                       struct sctk_alloc_macro_bloc *bloc);
void sctk_alloc_mm_source_light_insert_segment(
    struct sctk_alloc_mm_source_light *light_source, void *base,
    sctk_size_t size);
struct sctk_alloc_macro_bloc *sctk_alloc_mm_source_light_mmap_new_segment(
    struct sctk_alloc_mm_source_light *light_source, sctk_size_t size);
extern struct sctk_alloc_macro_bloc *
sctk_alloc_mm_source_light_remap(struct sctk_alloc_macro_bloc *macro_bloc,
                                 sctk_size_t size);
struct sctk_alloc_mm_source_light * sctk_alloc_get_mm_source_light(struct sctk_alloc_mm_source * source);
bool sctk_alloc_mm_source_light_keep(
    struct sctk_alloc_mm_source_light *light_source, sctk_size_t size,
    bool for_register);
void sctk_alloc_mm_source_light_migrate(struct sctk_alloc_mm_source_light * light_source,int target_numa_node);

/************************* FUNCTION ************************/
//helpers
void sctk_alloc_force_segment_binding(
    struct sctk_alloc_mm_source_light *light_source, void *base,
    sctk_size_t size);
#ifdef HAVE_HWLOC
hwloc_nodeset_t sctk_alloc_mm_source_light_init_nodeset(int numa_node);
#endif

#ifdef __cplusplus
};
#endif

#endif //SCTK_ALLOC_MM_SOURCE_LIGHT_H
