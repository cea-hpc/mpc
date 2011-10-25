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
/* #   - PERACHE Marc     marc.perache@cea.fr                             # */
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                 # */
/* #                                                                      # */
/* ######################################################################## */
#define MPC_USE_SHM

#ifndef __SCTK__SHM_H_
#include "sctk_shm_mem_struct.h"
#include "sctk_hybrid_comm.h"
#define __SCTK__SHM_H_
#ifdef __cplusplus
extern "C" {
#endif

	/* When the init process has finished to filled the
	 * memory structure, it writes the following structure
	 * at the beginning of the mmap */
	struct mmap_infos_s {
    /* base address of the memory struct */
		struct sctk_shm_mem_struct_s *mem_base;
    /* ptr of the shm_malloc */
		void*  malloc_ptr;
    /* ptr returned by the sctk_malloc */
		void* malloc_base;
	};

	/* Initialize SHM module */
	void sctk_net_init_driver_shm ( int *argc, char ***argv );
	/* Preinitilize SHM module */
	void sctk_net_preinit_driver_shm ( sctk_net_driver_pointers_functions_t* pointers );

	/* conversation tables */
	int* sctk_shm_get_global_to_local_process_translation_table();
	int* sctk_shm_get_local_to_global_process_translation_table();

#define xstr(s) str(s)
#define str(s) #s

#define DBG_S(x) if (x) \
		sctk_debug("BEGIN %s", __func__); \
	else \
		sctk_nodebug("");

#define DBG_E(x) if (x) \
		sctk_debug("END %s", __func__); \
	else \
		sctk_nodebug("");

#define STA "Begin"

#ifdef __cplusplus
}
#endif
#endif
