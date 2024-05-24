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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - POUGET Kevin pougetk@ocre.cea.fr                                 # */
/* ######################################################################## */
#ifndef __SCTK__THREAD_DBG__
#define __SCTK__THREAD_DBG__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct sctk_thread_data_s sctk_thread_data_t;
typedef struct sctk_thread_status_s sctk_thread_status_t;


void sctk_thread_add(sctk_thread_data_t *item, void *tid);
void sctk_thread_remove(sctk_thread_data_t *item);
int sctk_enable_lib_thread_db(void);
int sctk_init_thread_debug(sctk_thread_data_t *item);

struct sctk_ethread_per_thread_s;

void sctk_refresh_thread_debug(struct sctk_ethread_per_thread_s *tid, sctk_thread_status_t s);
void sctk_refresh_thread_debug_migration(struct sctk_ethread_per_thread_s *tid);
int sctk_init_idle_thread_dbg(void *tid, void *start_fact);
int sctk_free_idle_thread_dbg(void *tid);
int sctk_report_creation(void *tid);
int sctk_report_death(void *tid);


#ifdef __cplusplus
}                               /* end of extern "C" */
#endif
#endif
