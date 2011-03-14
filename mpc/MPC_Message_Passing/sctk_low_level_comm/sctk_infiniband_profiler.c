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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_infiniband_profiler.h"
#include "sctk_infiniband_config.h"

void sctk_ibv_profiler_init()
{
#ifdef IBV_ENABLE_PROFILE
  int i;

  memset(counters, 0, 20 * sizeof(uint64_t));
  for (i=0; i < NB_PROFILE_ID; ++i)
  {
    locks[i] = SCTK_SPINLOCK_INITIALIZER;
  }
#endif
}

void sctk_ibv_profiler_inc(ibv_profiler_id id)
{
#ifdef IBV_ENABLE_PROFILE
  sctk_spinlock_lock(&locks[id]);
  counters[id].value+=1;
  sctk_spinlock_unlock(&locks[id]);
#endif
}

void sctk_ibv_profiler_dec(ibv_profiler_id id)
{
#ifdef IBV_ENABLE_PROFILE
  sctk_spinlock_lock(&locks[id]);
  counters[id].value-=1;
  sctk_spinlock_unlock(&locks[id]);
#endif
}


void sctk_ibv_profiler_add(ibv_profiler_id id, int c)
{
#ifdef IBV_ENABLE_PROFILE
  sctk_spinlock_lock(&locks[id]);
  counters[id].value+=c;
  sctk_spinlock_unlock(&locks[id]);
#endif
}

void sctk_ibv_profiler_sub(ibv_profiler_id id, int c)
{
#ifdef IBV_ENABLE_PROFILE
  sctk_spinlock_lock(&locks[id]);
  counters[id].value-=c;
  sctk_spinlock_unlock(&locks[id]);
#endif
}

void sctk_ibv_generate_report()
{
#ifdef IBV_ENABLE_PROFILE
  FILE* file;
  char filename[256];
  char line[1024];
  int i;

  sprintf(filename, "profile/mpc_profile_%d_%d", sctk_process_rank, sctk_process_number);

  file = fopen(filename, "w+");
  assume(file);

  for (i=0; i < NB_PROFILE_ID; ++i)
  {
   sprintf(line, "%s %lu\n", counters[i].name, counters[i].value);
   fwrite(line, sizeof(char), strnlen(line, 1024), file);
//    sctk_debug("line: %s", line);
  }
  fclose(file);
#endif
}
