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
#ifndef __SCTK_TOPOLOGY_CPUSET_H_
#define __SCTK_TOPOLOGY_CPUSET_H_

#include <stdio.h>
#include "sctk_config.h"
#include "sctk_spinlock.h"

#if defined(Linux_SYS) && defined(MPC_USE_CPUSET) && !defined(MPC_HAVE_BINDING)
#include <cpuset.h>
#include <sctk_topology_linux_gen_detection.h>
#define MPC_HAVE_BINDING
static char *cpu_set_path = NULL;

static int sctk_cpuset_is_usable = 1;

static void
sctk_create_cpuset (int j)
{
  int res;
  char name[2048];
  cs_cpumask_t cmask;
  cs_memmask_t mmask;

  sprintf (name, "%s/MPC_proc_%02d", cpu_set_path, j);
  sctk_nodebug ("create %s (%d)", name,sctk_cpuinfos[j].cpu_id);
  res = cs_create (name);
  assert (res == 0);

  /*Cpu init */
  res = cs_cpumask_clear (&cmask);
  assert (res == 0);
  res = cs_cpumask_add (&cmask, sctk_cpuinfos[j].cpu_id);
  assert (res == 0);
  sctk_nodebug ("add %d in %s", sctk_cpuinfos[j].cpu_id, name);
  res = cs_set_cpus (name, cmask);
  assert (res == 0);

  /*Mem init */
  res = cs_memmask_clear (&mmask);
  assert (res == 0);
  res = cs_memmask_add (&mmask, sctk_cpuinfos[j].numa_id);
  assert (res == 0);
  res = cs_set_mems (name, mmask);
  assert (res == 0);
  sctk_nodebug ("create %s done", name);
}

int
sctk_bind_to_cpu (int i)
{
  sctk_cpuinfo_t *old_cpu;
  old_cpu = (sctk_cpuinfo_t *) kthread_getspecific (sctk_topology_key);


  if(sctk_cpuset_is_usable){
    if (i >= 0)
      {
	kthread_setspecific (sctk_topology_key, &(sctk_cpuinfos[i]));

	{
	  char name[2048];
	  int res;
	  static sctk_thread_mutex_t mutex = SCTK_THREAD_MUTEX_INITIALIZER;

	  sprintf (name, "%s/MPC_proc_%02d", cpu_set_path, i);

	  sctk_nodebug ("before %s asked %d (%s) %d/%d", cs_get_current (),
			i, name, getpid (), getpid ());
	  sctk_thread_mutex_lock (&mutex);
	  cs_lock_libcpuset ();
	  res = cs_add_task (name, getpid ());

	  if (res != 0)
	    {
	      cs_unlock_libcpuset ();
	      sched_yield ();
	      cs_lock_libcpuset ();
	      res = cs_add_task (name, getpid ());
	    }
	  if (res != 0)
	    {
	      cs_unlock_libcpuset ();
	      sched_yield ();
	      cs_lock_libcpuset ();
	      res = cs_add_task (name, getpid ());
	    }
	  if (res != 0)
	    {
	      cs_unlock_libcpuset ();
	      sched_yield ();
	      cs_lock_libcpuset ();
	      res = cs_add_task (name, getpid ());
	    }


	  assert (res == 0);
	  cs_unlock_libcpuset ();
	  sctk_thread_mutex_unlock (&mutex);
	  sctk_nodebug ("after %s asked %d", cs_get_current (), i);
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
  int res;
  sctk_init_sctk_cpuinfos ();
  res = sctk_update_numa_linux_gen ();

  if(strcmp(sctk_multithreading_mode,"ethread_mxn") != 0){
    sctk_cpuset_is_usable = 0;
  }

  if(sctk_cpuset_is_usable){
    if (res)
      {
	int res;
	int i;
	char name_mpc[2048];
	cs_alloc_hints hints;

	sprintf (name_mpc, "MPC_%d", getpid ());

	sctk_nodebug("Use %d cores",sctk_processor_number_on_node);

	hints.mode = CPUSET_ALLOC_MODE_USER;
	hints.user_cpu_list = sctk_malloc(sctk_processor_number_on_node*sizeof(int));
      
	for(i = 0; i < sctk_processor_number_on_node; i++){
	  hints.user_cpu_list[i] = sctk_cpuinfos[i].cpu_id;
	  sctk_nodebug("%d = %d",i,hints.user_cpu_list[i]);
	}

	assert (cs_init () == 0);
	res = cpuset_simple_create (name_mpc, sctk_processor_number_on_node,
				    CS_AUTOCLEAN, &hints);
	assert (res == 0);
	res = cpuset_assign (name_mpc);
	assert (res == 0);
	sctk_node_number_on_node = cs_nr_nodes ();
	cpu_set_path = cs_get_current ();
	sctk_nodebug ("%d cpus %d nodes in %s",
		      sctk_processor_number_on_node,
		      sctk_node_number_on_node, cpu_set_path);

	for(i = 0; i < sctk_processor_number_on_node; i++){
	  cs_lock_libcpuset ();
	  sctk_create_cpuset (i);
	  cs_unlock_libcpuset ();
	}
      }	
    else
      {
	int i;
	int j;
	int res;
	char name_mpc[2048];
	cs_cpumask_t rootmask;
	int cpu_set = 0;

	sprintf (name_mpc, "MPC_%d", getpid ());

	assert (cs_init () == 0);
	res = cpuset_simple_create (name_mpc, sctk_processor_number_on_node,
				    CS_AUTOCLEAN, NULL);
	assert (res == 0);

	res = cpuset_assign (name_mpc);
	assert (res == 0);
	sctk_node_number_on_node = cs_nr_nodes ();
	cpu_set_path = cs_get_current ();
	sctk_nodebug ("%d cpus %d nodes in %s",
		      sctk_processor_number_on_node,
		      sctk_node_number_on_node, cpu_set_path);

	i = 0;
	while (cpu_set != sctk_processor_number_on_node)
	  {
	    cs_cpumask_t mask;
	    cs_get_node_cpus (i, &mask);
	    res = cs_get_cpus (cpu_set_path, &rootmask);
	    assert (res == 0);
	    sctk_nodebug ("Check node %d %d cpus in mask", i,
			  cs_cpumask_cpus_count (&rootmask));
	    for (j = 0; j < sctk_processor_number_on_node; j++)
	      {
		int first;
		first = cs_cpumask_first (&rootmask);
		if (cs_cpumask_cpu_present (&mask, first))
		  {
		    sctk_cpuinfos[j].numa_id = i;
		    sctk_cpuinfos[j].socket_id = j;
		    sctk_cpuinfos[j].core_id = 0;

		    sctk_cpuinfos[j].cpu_id = first;
		    sctk_cpuinfos[j].i = j;
		    sctk_nodebug ("%d is on %02d", j, i);
		    cpu_set++;
		  }
		cs_cpumask_remove (&rootmask, first);
	      }
	    i++;
	  }

	sctk_restrict_topology ();

	for(i = 0; i < sctk_processor_number_on_node; i++){
	  cs_lock_libcpuset ();
	  sctk_create_cpuset (i);
	  cs_unlock_libcpuset ();
	}
      }
  }
  
}


#endif
#endif
