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
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_TOPOLOGY_SCHED_AFFINITY_H_
#define __SCTK_TOPOLOGY_SCHED_AFFINITY_H_

#include <stdio.h>
#include <errno.h>
#include "sctk_config.h"

#if defined(Linux_SYS)
#include <sys/types.h>
#include <linux/unistd.h>
#define __USE_GNU
#include <sched.h>
#endif

#if defined(Linux_SYS) && defined(CPU_ZERO) && !defined(MPC_HAVE_BINDING)
#include <sctk_topology_linux_gen_detection.h>
#define MPC_HAVE_BINDING
int
sctk_bind_to_cpu (int i)
{
  sctk_cpuinfo_t *old_cpu;
  old_cpu = (sctk_cpuinfo_t *) kthread_getspecific (sctk_topology_key);

  if (i >= 0)
    {
      kthread_setspecific (sctk_topology_key, &(sctk_cpuinfos[i]));
      {
	int res;
	cpu_set_t cpu;
	CPU_ZERO (&cpu);

	CPU_SET (sctk_cpuinfos[i].cpu_id, &cpu);
	sctk_nodebug ("Bind %d on %d %lu", i, sctk_cpuinfos[i].cpu_id,
		      cpu.__bits[0]);

	res = sched_setaffinity (0, sctk_real_processor_number_on_node, &cpu);
	if (res)
	  {
	    sctk_nodebug ("Affinity res %d %d EFAULT %d EINVAL %d", res,
			  errno, EFAULT, EINVAL);
	    perror ("Fail to setaffinity: ");
	  }
      }
    }

  if (old_cpu == NULL)
    {
      return -1;
    }
  else
    {
      return old_cpu->i;
    }
}

static void
sctk_update_numa ()
{
  sctk_init_sctk_cpuinfos ();

  sctk_update_numa_linux_gen ();
}

#endif

#endif
