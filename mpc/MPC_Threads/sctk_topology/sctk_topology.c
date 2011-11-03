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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_config.h"
#include "sctk_topology.h"
#include "sctk_launch.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk.h"
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
#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

static int sctk_processor_number_on_node = 0;
static char sctk_node_name[SCTK_MAX_NODE_NAME];
static sctk_spinlock_t topology_lock = SCTK_SPINLOCK_INITIALIZER;

static hwloc_topology_t topology;
const struct hwloc_topology_support *support;

/* print a cpuset in a human readable way */
static void
print_cpuset(hwloc_bitmap_t cpuset)
{
  char buff[256];
  hwloc_bitmap_list_snprintf(buff, 256, cpuset);
  fprintf(stdout, "cpuset: %s\n", buff);
  fflush(stdout);
}

static void
sctk_update_topology (
    const int processor_number,
    const int index_first_processor
    ) {
  sctk_processor_number_on_node = processor_number ;
  hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
  unsigned int i;
  int err;
  hwloc_bitmap_zero(cpuset);
  for( i=index_first_processor; i < index_first_processor+processor_number; ++i)
  {
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    hwloc_cpuset_t set = hwloc_bitmap_dup(pu->cpuset);
    hwloc_bitmap_singlify(set);
    hwloc_bitmap_or(cpuset, cpuset, set);
  }
  err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
  assume(!err);
  hwloc_bitmap_free(cpuset);
}

  static void
sctk_restrict_topology ()
{
  int rank ;

  if (sctk_enable_smt_capabilities)
  {
    sctk_warning ("SMT capabilities ENABLED");
  }
  else
  {
    /* disable SMT capabilities */
    unsigned int i;
    int err;
    hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(cpuset);
    const int core_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);

    for(i=0;i < core_number; ++i)
    {
      hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
      hwloc_cpuset_t set = hwloc_bitmap_dup(core->cpuset);
      hwloc_bitmap_singlify(set);
      hwloc_bitmap_or(cpuset, cpuset, set);
    }
    /* restrict the topology to physical CPUs */
    err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
    assume(!err);
    hwloc_bitmap_free(cpuset);
  }

  sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

  /* Share nodes */
  sctk_share_node_capabilities = 1;
  if ((sctk_share_node_capabilities == 1) && 
      (sctk_process_number > 1))
  {
    int detected_on_this_host = 0;
    int detected = 0;

#ifdef MPC_Message_Passing
    /* Determine number of processes on this node */
    sctk_pmi_get_processes_on_node_number(&detected_on_this_host);
    sctk_pmi_get_process_on_node_rank(&rank);
    detected = sctk_process_number;
    sctk_nodebug("Nb process on node %d",detected_on_this_host);
#else
    detected = 1;
#endif

    while (detected != sctk_get_process_nb ());
    sctk_nodebug ("%d/%d host detected %d share %s", detected,
        sctk_get_process_nb (), detected_on_this_host,
        sctk_node_name);

    {
      /* Determine processor number per process */
      int processor_number = sctk_processor_number_on_node / detected_on_this_host;
      int remaining_procs = sctk_processor_number_on_node % detected_on_this_host;
      int start = processor_number * rank;
      if(processor_number < 1){
	processor_number = 1;
	remaining_procs = 0;
	start = (processor_number * rank) % sctk_processor_number_on_node;
      }

      if ( remaining_procs > 0 )
      {
        if ( rank < remaining_procs )
        {
          processor_number++;
          start += rank;
        } else {
          start += remaining_procs;
        }
      }

      sctk_update_topology ( processor_number, start ) ;
    }

  } /* share node */

  /* Choose the number of processor used */
  {
    int processor_number;

    processor_number = sctk_get_processor_nb ();

    if (processor_number > 0)
    {
      if (processor_number > sctk_processor_number_on_node)
      {
        sctk_error ("Unable to allocate %d core (%d real cores)",
            processor_number, sctk_processor_number_on_node);
        /* we cannot exit with a sctk_abort() because thread
         * library is not initialized at this moment */
        /* sctk_abort (); */
        exit(1);
      }
      else if ( processor_number < sctk_processor_number_on_node )
      {
        sctk_warning ("Do not use the entire node %d on %d",
            processor_number, sctk_processor_number_on_node);
        sctk_update_topology ( processor_number, 0 ) ;
      }
    }
  }
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

/*! \brief Return the current core_id
*/
static __thread int sctk_get_cpu_val = -1;
static inline  int
sctk_get_cpu_intern ()
{
  hwloc_cpuset_t set = hwloc_bitmap_alloc();

  int ret = hwloc_get_last_cpu_location(topology, set, 0);
  assert(ret!=-1);
  assert(!hwloc_bitmap_iszero(set));

  hwloc_bitmap_free(set);
  return ret;
}

  int
sctk_get_cpu ()
{
  if(sctk_get_cpu_val < 0){
    sctk_spinlock_lock(&topology_lock);
    sctk_get_cpu_val = sctk_get_cpu_intern();
    sctk_spinlock_unlock(&topology_lock);
  }
  return sctk_get_cpu_val;
}

/*! \brief Initialize the topology module
*/
  void
sctk_topology_init ()
{

#ifdef MPC_Message_Passing
  if(sctk_process_number > 1){
    sctk_pmi_init();
  }
#endif

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  support = hwloc_topology_get_support(topology);

  sctk_only_once ();
  sctk_print_version ("Topology", SCTK_LOCAL_VERSION_MAJOR,
      SCTK_LOCAL_VERSION_MINOR);

  sctk_restrict_topology();

#ifndef WINDOWS_SYS
  gethostname (sctk_node_name, SCTK_MAX_NODE_NAME);
#else
  sprintf (sctk_node_name, "localhost");
#endif

  uname (&utsname);

/*   sctk_print_topology (stderr); */
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

/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
 */
  void
sctk_print_topology (FILE * fd)
{
  int i;
  fprintf(fd, "Node %s: %s %s %s\n", sctk_node_name, utsname.sysname,
      utsname.release, utsname.version);
  const int pu_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  for(i=0;i < pu_number; ++i)
  {
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    hwloc_obj_t tmp[3];
    unsigned int node_os_index;
    tmp[0] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);
    tmp[1] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, pu);
    tmp[2] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, pu);

    if(tmp[0] == NULL){
      node_os_index = 0; 
    } else {
      node_os_index = tmp[0]->os_index;
    }

    fprintf(fd, "\tProcessor %4u real (%4u:%4u:%4u)\n", pu->os_index,
	    node_os_index, tmp[1]->logical_index, tmp[2]->logical_index );
  }
 
  fprintf (fd, "\tNUMA: %d\n", sctk_is_numa_node ());
  if (sctk_is_numa_node ())
  {
    fprintf (fd, "\t\t* Node nb %d\n", sctk_get_numa_node_number());
  }
  return;
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

  sctk_spinlock_lock(&topology_lock);

  int ret = sctk_get_cpu_intern();
  sctk_get_cpu_val = ret;

  if (i >= 0)
  {
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    int err = hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);
    if (err)
    {
      fprintf(stderr,"%-40s: %sFAILED (%d, %s)\n", msg, supported?"":"X", errno, errmsg);
    }
    sctk_get_cpu_val = i;
  }
  sctk_spinlock_unlock(&topology_lock);
  return ret;
}

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
  char *
sctk_get_processor_name ()
{
  return utsname.machine;
}

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
 */
  int
sctk_get_first_cpu_in_node (int node)
{
  hwloc_obj_t currentNode = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, node);

  while(currentNode->type != HWLOC_OBJ_CORE) {
    currentNode = currentNode->children[0];
  }

  return currentNode->logical_index ;
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
 * used in ethread
 */
  int
sctk_set_cpu_number (int n)
{
  sctk_update_topology ( n, 0 ) ;
  return n;
}

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
  int
sctk_is_numa_node ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) != 0;
}

/*! \brief Return the number of NUMA nodes
*/
  int
sctk_get_numa_node_number ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) ;
}

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
 */
  int
sctk_get_node_from_cpu (const int vp)
{
  if(sctk_is_numa_node ()){
    const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
    const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);
    return node->logical_index;
  } else {
    return 0;
  }
}

/*! \brief Return the number of NUMA nodes
 * @param level NUMA level
 */
int sctk_get_numa_number (const int level)
{
  if ( level > sctk_get_numa_level_number() ) {
    return 0 ;
  }
  const int socket_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  const int numa_depth = socket_depth - level ;
  return hwloc_get_nbobjs_by_depth(topology,numa_depth);
}

/*! \brief Return the number of sockets
*/
int sctk_get_socket_number ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_SOCKET) ;
}

/*! \brief Return the number of caches
 * @param level Cache level
 */
int sctk_get_cache_number (const int level)
{
  if ( level > sctk_get_cache_level_number() ) {
    return 0 ;
  }
  const int core_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
  const int cache_depth = core_depth - level ;
  return hwloc_get_nbobjs_by_depth(topology,cache_depth);
}

/*! \brief Return the number of cores
*/
int sctk_get_core_number ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE) ;
}

/*! \brief Return the number of cache levels
*/
int sctk_get_cache_level_number ()
{
  const int core_depth   = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
  const int socket_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  return core_depth - socket_depth - 1 ;
}

/*! \brief Return the number of NUMA levels
 * return 0 if machine is not NUMA
*/
int sctk_get_numa_level_number ()
{
  const int machine_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_MACHINE);
  const int socket_depth  = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  return socket_depth - machine_depth - 1 ;
}

/*! \brief Return the NUMA id
 * @param vp VP
 * @param level NUMA level
 */
int sctk_get_numa_id (const int level, const int vp)
{
  const int socket_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  const int numa_depth = socket_depth - level ;
  const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
  const hwloc_obj_t numa = hwloc_get_ancestor_obj_by_depth(topology, numa_depth, pu);
  return numa->logical_index;
}

/*! \brief Return the socket id
 * @param vp VP
 */
int sctk_get_socket_id (const int vp)
{
  const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
  const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, pu);
  return node->logical_index;
}

/*! \brief Return the cache id
 * @param vp VP
 * @param level Cache level
 */
int sctk_get_cache_id (const int level, const int vp)
{
  const int core_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
  const int cache_depth = core_depth - level ;
  const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
  const hwloc_obj_t cache = hwloc_get_ancestor_obj_by_depth(topology, cache_depth, pu);
  return cache->logical_index;
}

/*! \brief Return the core id
 * @param vp VP
 */
int sctk_get_core_id (const int vp)
{
  const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
  const hwloc_obj_t core = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, pu);
  return core->logical_index;
}

/* print the neighborhood*/
  static void
print_neighborhood(int cpuid, int nb_cpus, int* neighborhood, hwloc_obj_t* objs)
{
  int i;
  hwloc_obj_t currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);
  hwloc_obj_type_t type = (sctk_is_numa_node()) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
  int currentCPU_node_id = hwloc_get_ancestor_obj_by_type(topology, type, currentCPU)->logical_index;
  int currentCPU_socket_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, currentCPU)->logical_index;
  int currentCPU_core_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, currentCPU)->logical_index;
  fprintf(stderr, "Neighborhood result starting with CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
      cpuid, currentCPU_node_id, currentCPU_socket_id, currentCPU_core_id);

  for (i = 0; i < nb_cpus; i++)
  {
    hwloc_obj_t current = objs[i];

    fprintf(stderr, "\tNeighbor[%d] -> CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
        i, neighborhood[i],
        hwloc_get_ancestor_obj_by_type(topology, type, current)->logical_index,
        hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, current)->logical_index,
        hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index);
  }
}

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
 */
  void
sctk_get_neighborhood(int cpuid, int nb_cpus, int* neighborhood)
{
  int i;

  hwloc_obj_t *objs;
  hwloc_obj_t currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);
  unsigned nb_cpus_found;

  /* alloc size for retreiving objects. We could also use a static array */
  objs = sctk_malloc(nb_cpus * sizeof(hwloc_obj_t));
  /* the closest CPU is actually... the current CPU :-). Set the closest CPU as the current
   * CPU */
  objs[0] = currentCPU;

  /* get closest objects to the current CPU */
  nb_cpus_found = hwloc_get_closest_objs(topology, currentCPU, &objs[1], (nb_cpus-1));
  /*  +1 because of the current cpu not returned by hwloc_get_closest_objs */
  assume( (nb_cpus_found + 1) == nb_cpus);

  /* fill the neighborhood variable */
  for (i = 0; i < nb_cpus; i++)
  {
    /*
       hwloc_obj_t current = objs[i];
       neighborhood[i] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index;
       */
    neighborhood[i] = objs[i]->logical_index;
  }

  /* uncomment for printing the returned result */
  //print_neighborhood(cpuid, nb_cpus, neighborhood, objs);

  sctk_free(objs);
}
