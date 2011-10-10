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
#include "sctk_config.h"
#include "sctk_topology.h"
#include "sctk_launch.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_kernel_thread.h"
#include <dirent.h>

#include <stdio.h>
#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#define SCTK_MAX_PROCESSOR_NAME 100
#define SCTK_MAX_NODE_NAME 512
#define SCTK_MAX_CPUINFO_NUMBER 512
#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

static int sctk_processor_number_on_node = 0;
static int sctk_real_processor_number_on_node = 0;
static int sctk_node_number_on_node = 0;
static char sctk_node_name[SCTK_MAX_NODE_NAME];

static hwloc_topology_t topology;
const struct hwloc_topology_support *support;

static void
sctk_restrict_topology ()
{
  /* Disable SMT capabilities */
  if (sctk_enable_smt_capabilities)
    {
	  sctk_warning ("SMT capailities ENABLED");
    }
  else
    {
	  int i, processor_number;
	  hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
	  hwloc_obj_t current = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
	  hwloc_obj_type_t type = (sctk_is_numa_node()) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;

	  hwloc_bitmap_zero(cpuset);
	  processor_number = sctk_processor_number_on_node;

	  for (i = 1; i < sctk_processor_number_on_node;)
	  {
		  int current_node_id = hwloc_get_ancestor_obj_by_type(topology, type, current)->logical_index;
		  int current_socket_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, current)->logical_index;
		  int current_core_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index;

		  hwloc_obj_t cpu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
		  int cpu_node_id = hwloc_get_ancestor_obj_by_type(topology, type, cpu)->logical_index;
		  int cpu_socket_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, cpu)->logical_index;
		  int cpu_core_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, cpu)->logical_index;

		  if ((current_node_id == cpu_node_id) && (current_socket_id == cpu_socket_id) && (current_core_id == cpu_core_id))
		  {
			  int j;

			  hwloc_bitmap_clr(cpuset, i);
			  int err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
			  sctk_assert(!err);
		  }
		  else {
			  current = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
			  i ++;
		  }

		  if (i == processor_number)
		  {
			  break;
		  }
	  }

	  sctk_processor_number_on_node = processor_number;
    }

  /* Share nodes */
  if (sctk_share_node_capabilities == 1
      && strcmp (sctk_store_dir, "/dev/null") != 0)
    {
      static char name[4096];
      static char pattern_name[4096];
      static char this_name[4096];
      FILE *file;
      int detected = 0;
      int detected_on_this_host = 0;
      int processor_number;
      int rank;
      int start;
      int i;

      /* Determine number of processes on this node */
      sprintf (name, "%s/use_%s_%d", sctk_store_dir, sctk_node_name,
	       getpid ());
      sprintf (this_name, "use_%s_%d", sctk_node_name, getpid ());
      sprintf (pattern_name, "use_%s_", sctk_node_name);
      sctk_nodebug ("file %s", name);
      file = fopen (name, "w");
      assume (file != NULL);
      fprintf (file, "use\n");
      fclose (file);

      do
	{
	  DIR *d;
	  struct dirent *direntry;

	  detected = 0;
	  detected_on_this_host = 0;

	  d = opendir (sctk_store_dir);
	  do
	    {
	      direntry = readdir (d);
	      if (direntry)
		{
		  if (memcmp (direntry->d_name, "use_", 4) == 0)
		    {
		      detected++;
		      sctk_nodebug ("Compare %s and %s", direntry->d_name,
				    pattern_name);
		      if (memcmp
			  (direntry->d_name, pattern_name,
			   strlen (pattern_name)) == 0)
			{
			  detected_on_this_host++;
			  if (memcmp
			      (direntry->d_name, this_name,
			       strlen (this_name)) == 0)
			    {
			      rank = detected_on_this_host - 1;
			    }
			}
		    }
		}
	    }
	  while (direntry != NULL);
	  closedir (d);
	  usleep (50);
	}
      while (detected != sctk_get_process_nb ());
      sctk_nodebug ("%d/%d host detected %d share %s", detected,
		    sctk_get_process_nb (), detected_on_this_host,
		    sctk_node_name);

      /* Determine processor number per process */
      processor_number =
	sctk_processor_number_on_node / detected_on_this_host;
      start = processor_number * rank;
      if (sctk_processor_number_on_node % detected_on_this_host)
	{
	  if (sctk_processor_number_on_node % detected_on_this_host > rank)
	    {
	      processor_number++;
	    }
	  if (sctk_processor_number_on_node % detected_on_this_host > rank)
	    {
	      start += rank;
	    }
	  else
	    {
	      start += sctk_processor_number_on_node % detected_on_this_host;
	    }
	}
      sctk_processor_number_on_node = processor_number;
      sctk_nodebug ("Rank %d use %d procs start %d", rank,
		    sctk_processor_number_on_node, start);
    }

  /* Choose the number of processor used */
  {
    int processor_number;

    processor_number = sctk_get_processor_nb ();

    if (processor_number > sctk_processor_number_on_node)
      {
	sctk_error ("Unable to allocate %d core (%d real cores)",
		    processor_number, sctk_processor_number_on_node);
	sctk_abort ();
      }

    if (processor_number > 0)
      {
	if (sctk_processor_number_on_node > processor_number)
	  {
	    sctk_warning ("Do not use the entire node %d on %d",
			  processor_number, sctk_processor_number_on_node);
	  }

	sctk_processor_number_on_node = processor_number;
      }
  }
   /* sctk_print_topology (stderr); */
}

#include <sched.h>
#if defined(HP_UX_SYS)
#include <sys/param.h>
#include <sys/pstat.h>
#endif
#ifdef HAVE_UTSNAME
static struct utsname utsname;
#else
struct utsname
{
  char sysname[];
  char nodename[];
  char release[];
  char version[];
  char machine[];
};

static struct utsname utsname;

static int
uname (struct utsname *buf)
{
  buf->sysname = "";
  buf->nodename = "";
  buf->release = "";
  buf->version = "";
  buf->machine = "";
  return 0;
}

#endif

static void
sctk_determine_processor_number ()
{
	sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
	if (SCTK_MAX_CPUINFO_NUMBER <= sctk_processor_number_on_node)
	{
		fprintf (stderr, "Too many processors\n");
		sctk_abort ();
	}
	sctk_real_processor_number_on_node = sctk_processor_number_on_node;
}

/*! \brief Return the current core_id
*/
int
sctk_get_cpu ()
{
	hwloc_obj_t root = hwloc_get_root_obj(topology);
	hwloc_cpuset_t set = hwloc_bitmap_alloc();

	int ret = hwloc_get_last_cpu_location(topology, set, 0);
	assert(!ret);
	assert(!hwloc_bitmap_iszero(set));
	assert(hwloc_bitmap_isincluded(set, root->cpuset));

	hwloc_bitmap_free(set);

	return ret;
}


static void
sctk_update_memory (void)
{
}

static void
sctk_update_numa (void)
{
	sctk_restrict_topology();
}

/*! \brief Initialize the topology module
*/
void
sctk_topology_init ()
{
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);

	support = hwloc_topology_get_support(topology);

  sctk_only_once ();
  sctk_print_version ("Topology", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);

  sctk_determine_processor_number ();

#ifndef WINDOWS_SYS
  gethostname (sctk_node_name, SCTK_MAX_NODE_NAME);
#else
  sprintf (sctk_node_name, "localhost");
#endif

  uname (&utsname);

  sctk_update_memory ();
  sctk_update_numa ();

   /* sctk_print_topology (stderr); */
}

/*! \brief Destroy the topology module
*/
void sctk_topology_destroy (void)
{
	hwloc_topology_destroy(topology);
}


/*! \brief Return the hostname
*/
char *
sctk_get_node_name ()
{
  return sctk_node_name;
}

/*! \brief Return the first child of \p obj of type \p type
 * @param obj Parent object
 * @param type Child type
*/
static hwloc_obj_t
sctk_get_first_child_by_type(hwloc_obj_t obj, hwloc_obj_type_t type)
{
	hwloc_obj_t child = obj;
	do
	{
		child = child->first_child;
	}
	while(child->type != type);
	return child;
}

static void
sctk_print_children(FILE * fd, hwloc_topology_t topology, hwloc_obj_t obj)
{
    char string[128];
    unsigned i;

    if (obj->type == HWLOC_OBJ_MACHINE)
    {
    	fprintf(fd, "Node %s(%d): %s %s %s\n", sctk_node_name,obj->logical_index, utsname.sysname,
    			   utsname.release, utsname.version);

    	hwloc_obj_t first_child_pu;
    	for (first_child_pu = sctk_get_first_child_by_type(obj, HWLOC_OBJ_PU); first_child_pu;
    			first_child_pu = first_child_pu->next_cousin)
    	{
    		hwloc_obj_t tmp;
			fprintf(fd, "\tProcessor %4u real %4u (%4u:%4u:%4u)\n", first_child_pu->logical_index, first_child_pu->sibling_rank,
				( (tmp = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, first_child_pu)) ? tmp->logical_index : 0),
				( (tmp = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, first_child_pu)) ? tmp->logical_index : 0),
				( (tmp = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, first_child_pu)) ? tmp->logical_index : 0));
    	}

    	fprintf (fd, "\tNUMA: %d\n", sctk_is_numa_node ());
		if (sctk_is_numa_node ())
		{
			fprintf (fd, "\t\t* Node nb %d\n", obj->arity);
		}

		return;
    }
    else
    {
    	for (i = 0; i < obj->arity; i ++)
    	{
    		sctk_print_children(fd, topology, obj->children[i]);
    	}
    }
}

/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
*/
void
sctk_print_topology (FILE * fd)
{

	sctk_print_children(fd, topology, hwloc_get_root_obj(topology));
}

/*! \brief Bind the current thread
 * @ param i The cpu_id to bind
*/
int
sctk_bind_to_cpu (int i)
{
	char * msg = "Bind to cpu";
	int supported = support->cpubind->set_thisthread_cpubind;
	const char *errmsg = strerror(errno);

	int ret = sctk_get_cpu();

	if (i >= 0)
	{
		hwloc_obj_t cpu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
		int err = hwloc_set_cpubind(topology, cpu->cpuset, HWLOC_CPUBIND_THREAD);
		if (err)
		{
			printf("%-40s: %sFAILED (%d, %s)\n", msg, supported?"":"X", errno, errmsg);
		}
	}

	return ret;
}

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
char *
sctk_get_processor_name ()
{
  return utsname.machine;
}

/*! \brief Return the NUMA node according to the code_id number
 * @param cpu core_id
*/
int
sctk_get_node_from_cpu (int cpu)
{
	hwloc_obj_t currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpu);
	hwloc_obj_t parentNode = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, currentCPU);
	if (parentNode)
	{
		return parentNode->logical_index;
	}
	return 0;
}

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
*/
int
sctk_get_first_cpu_in_node (int node)
{
	hwloc_obj_t firstChildNode, currentNode = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, node);
	hwloc_obj_type_t typeChildren = HWLOC_OBJ_NODE;

	while ((typeChildren != HWLOC_OBJ_PU) && (firstChildNode = currentNode->children[0]))
	{
		typeChildren = firstChildNode->type;
	}
	return ((typeChildren == HWLOC_OBJ_PU) ? firstChildNode->logical_index : 0);
}

/*! \brief Return the total number of core for the process
*/
int
sctk_get_cpu_number ()
{
	return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
}

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
*/
int
sctk_set_cpu_number (int n)
{
  sctk_processor_number_on_node = n;
  return n;
}

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
int
sctk_is_numa_node ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) != 0;
}

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void
sctk_get_neighborhood(int cpuid, int nb_cpus, int* neighborhood)
{
	int n, s, c, p;
	int neighbor_id = 0;
	int i;

	hwloc_obj_t currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);
	hwloc_obj_type_t type = (sctk_is_numa_node()) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
	int currentCPU_node_id = hwloc_get_ancestor_obj_by_type(topology, type, currentCPU)->logical_index;
	int currentCPU_socket_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, currentCPU)->logical_index;
	int currentCPU_core_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, currentCPU)->logical_index;

	/* Compute the number of nodes on this architecture */
	int max_node = hwloc_get_nbobjs_by_type(topology, type);
	sctk_assert(max_node > 0);
	sctk_nodebug( "sctk_get_neighborhood: Max node = %d\n", max_node );

	for (n = 0; n < max_node; n ++)
	{
		int current_node_id = (currentCPU_node_id + n) % max_node;
		hwloc_obj_t current_node_obj = hwloc_get_obj_by_type(topology, type, current_node_id);

		/* Compute the number of sockets for this node */
		int max_socket = (current_node_obj->first_child->type == HWLOC_OBJ_SOCKET) ? current_node_obj->arity : 0;
		sctk_assert(max_socket > 0);
		sctk_nodebug( "sctk_get_neighborhood: Found %d sockets for node %d", max_socket, current_node_id ) ;

		for (s = 0; s < max_socket; s ++)
		{
			int current_socket_id = s;
			/* For the first time, start from the target CPU */
			if (n == 0)
			{
				current_socket_id = (currentCPU_socket_id + s) % max_socket;
			}
			hwloc_obj_t current_socket_obj = current_node_obj->children[current_socket_id];

			/* Compute the number of cores for this socket */
			hwloc_obj_t core = sctk_get_first_child_by_type(current_socket_obj, HWLOC_OBJ_CORE), core_tmp, * core_tab;
			int max_core = 0;
			if (core) {
				core_tab = (hwloc_obj_t *) malloc ((max_core + 1) * sizeof(hwloc_obj_t));
				core_tab[max_core] = core;
				max_core ++;
			}
			for (core_tmp = core->next_cousin; core_tmp && hwloc_obj_is_in_subtree(topology, core_tmp, current_socket_obj); core_tmp = core_tmp->next_cousin, max_core ++)
			{
				core_tab = (hwloc_obj_t *) realloc (core_tab, (max_core + 1) * sizeof(hwloc_obj_t));
				core_tab[max_core] = core_tmp;
			}
			sctk_assert(max_core > 0);

			sctk_nodebug( "sctk_get_neighborhood: Found %d cores for socket %d on node %d",
				max_core, current_socket_id, current_node_id ) ;

			for (c = 0; c < max_core; c ++)
			{
				int current_core_id = c;
				/* For the first time, start from the target CPU */
				if (n == 0 && s == 0)
				{
					current_core_id = (currentCPU_core_id + c) % max_core;
				}
				hwloc_obj_t current_core_obj = core_tab[c];

				/* Compute the number of PU for this core */
				int max_pu = (current_core_obj->first_child->type == HWLOC_OBJ_PU) ? current_core_obj->arity : 0;
				sctk_assert (max_pu > 0);
				sctk_nodebug( "sctk_get_neighborhood: Found %d pu for core %d on socket %d on node %d",
					max_pu, current_core_id, current_socket_id, current_node_id ) ;

				/* Store the CPU which has the correct coordinates */
				for (p = 0; p < current_core_obj->arity; p ++)
				{
					neighborhood[neighbor_id] = (int) current_core_obj->children[p]->logical_index;
					neighbor_id ++;
				}
			}

			free(core_tab);
		}
	}

	sctk_assert (neighbor_id == nb_cpus);

#if 0
	/* Print the final result */
	fprintf(stderr, "Neighborhood result starting with CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
			cpuid, currentCPU_node_id, currentCPU_socket_id, currentCPU_core_id);

	for (i = 0; i < nb_cpus; i ++)
	{
		hwloc_obj_t current = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, neighborhood[i]);
		fprintf(stderr, "\tNeighbor[%d] -> CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
				i, neighborhood[i],
				hwloc_get_ancestor_obj_by_type(topology, type, current)->logical_index,
				hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, current)->logical_index,
				hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index);
	}
#endif
}
