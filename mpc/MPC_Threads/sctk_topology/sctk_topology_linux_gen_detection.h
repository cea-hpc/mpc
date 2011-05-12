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
#ifndef __SCTK_TOPOLOGY_LINUX_GEN_DETECTION_H_
#define __SCTK_TOPOLOGY_LINUX_GEN_DETECTION_H_

#include <stdio.h>
#include <dirent.h>
#include "sctk_config.h"
#include "sctk_launch.h"

#if defined(Linux_SYS)
#define SMALL_BUFFER_SIZE 40000

typedef struct
{
  int level;
  char type;
  long ident;
} sctk_cache_t;

typedef struct
{
  int core_id;
  int socket_id;
  int cache_nb;
  sctk_cache_t **caches_idents;
} sctk_cpuid_t;

typedef struct topology_node_s
{
  long ident;
  char *type;
  int nb_sons;
  struct topology_node_s **sons;
  struct topology_node_s *father;
  sctk_cpuid_t *cpuid;
} topology_node_t;


static void
sctk_add_cache (sctk_cpuid_t * cpuid, sctk_cache_t * cache)
{
  cpuid->cache_nb++;
  cpuid->caches_idents =
    realloc (cpuid->caches_idents, cpuid->cache_nb * sizeof (sctk_cache_t *));
  cpuid->caches_idents[cpuid->cache_nb - 1] = cache;
}

static topology_node_t *
sctk_create_node (char *type, long ident)
{
  topology_node_t *tmp;

  tmp = malloc (sizeof (topology_node_t));

  tmp->ident = ident;
  tmp->type = type;
  tmp->nb_sons = 0;
  tmp->sons = NULL;
  tmp->cpuid = NULL;
  tmp->father = NULL;

  return tmp;
}

static void
sctk_add_son (topology_node_t * father, topology_node_t * son)
{
  father->nb_sons++;
  father->sons =
    realloc (father->sons, father->nb_sons * sizeof (topology_node_t *));
  father->sons[father->nb_sons - 1] = son;
  son->father = father;
}

static int
sctk_cache_sort (sctk_cache_t ** a, sctk_cache_t ** b)
{
  return (*a)->level > (*b)->level;
}

static void
sctk_cpuid (sctk_cpuid_t * cpuid, int node_id, int cpu_id)
{
  char *sys_dir = NULL;
  FILE *fd = NULL;
  char buffer[4096];
  int i;
  memset (cpuid, 0, sizeof (sctk_cpuid_t));
  sys_dir = malloc (SMALL_BUFFER_SIZE);
  sprintf (sys_dir, "/sys/devices/system/node/node%d/cpu%d/topology/core_id",
	   node_id, cpu_id);
  fd = fopen (sys_dir, "r");
  if (fd)
    {
      assume(fread (buffer, 1, 4096, fd) > 0);
      cpuid->core_id = atoi (buffer);
      fclose (fd);
    }
  sprintf (sys_dir,
	   "/sys/devices/system/node/node%d/cpu%d/topology/physical_package_id",
	   node_id, cpu_id);
  fd = fopen (sys_dir, "r");
  if (fd)
    {
      assume(fread (buffer, 1, 4096, fd) > 0);
      cpuid->socket_id = atoi (buffer);
      fclose (fd);
    }

  for (i = 0;; i++)
    {
      sctk_cache_t *cache;

      sprintf (sys_dir,
	       "/sys/devices/system/node/node%d/cpu%d/cache/index%d/type",
	       node_id, cpu_id, i);
      fd = fopen (sys_dir, "r");
      if (fd == NULL)
	{
	  break;
	}
      memset (buffer, 0, 4096);
      assume(fread (buffer, 1, 4096, fd) > 0);
      sctk_nodebug("level %d %s",cache->level,buffer);
      fclose (fd);
      if (strcmp (buffer, "Instruction\n") != 0)
	{
	  void *tmp;
	  sprintf (sys_dir,
		   "/sys/devices/system/node/node%d/cpu%d/cache/index%d/level",
		   node_id, cpu_id, i);
	  fd = fopen (sys_dir, "r");
	  cache = malloc (sizeof (sctk_cache_t));
	  memset (cache, 0, sizeof (sctk_cache_t));
	  sctk_add_cache (cpuid, cache);
	  assume(fread (buffer, 1, 4096, fd) > 0);
	  cache->level = atoi (buffer);
	  fclose (fd);

	  sprintf (sys_dir,
		   "/sys/devices/system/node/node%d/cpu%d/cache/index%d/shared_cpu_map",
		   node_id, cpu_id, i);
	  fd = fopen (sys_dir, "r");
	  memset (buffer, 0, 4096);
	  while(fread (buffer, 1, 9, fd) == 9);
	  sscanf (buffer, "%x", (unsigned int *) &tmp);
	  cache->ident = (long) tmp;
/*       cache->ident = atol(buffer); */
	  sctk_nodebug("%ld %x %s",cache->ident,cache->ident,buffer);
	  fclose (fd);
	}

    }

  qsort (cpuid->caches_idents, cpuid->cache_nb, sizeof (sctk_cache_t *),
	 (int (*)(const void *, const void *)) sctk_cache_sort);

  free (sys_dir);
}

static int
sctk_compar (topology_node_t ** a, topology_node_t ** b)
{
  if ((*a)->cpuid->socket_id == (*b)->cpuid->socket_id)
    {
      if ((*a)->cpuid->caches_idents != NULL
	  && (*b)->cpuid->caches_idents != NULL)
	{
	  int i;
	  for (i = (*a)->cpuid->cache_nb - 1; i >= 0; i--)
	    {
	      if ((*a)->cpuid->caches_idents[i]->ident >
		  (*b)->cpuid->caches_idents[i]->ident)
		{
		  return 1;
		}
	    }
	}
      else
	{
	  return 0;
	}
    }

  if ((*a)->cpuid->socket_id > (*b)->cpuid->socket_id)
    {
      return 1;
    }
  return 0;
}

static void
sctk_build_cache_hierarchy (topology_node_t * node, int i)
{
  topology_node_t **cpus;
  int cpu_nb;
  topology_node_t *cache;
  long cache_id;
  int k;

  cpus = node->sons;
  cpu_nb = node->nb_sons;
  node->sons = NULL;
  node->nb_sons = 0;

  if (i >= 0)
    {
      k = 0;
      cache_id = cpus[k]->cpuid->caches_idents[i]->ident;
      cache =
	sctk_create_node ("cache level",
			  cpus[k]->cpuid->caches_idents[i]->level);
      sctk_add_son (node, cache);
      sctk_add_son (cache, cpus[k]);
      for (k = 1; k < cpu_nb; k++)
	{
	  if (cache_id != cpus[k]->cpuid->caches_idents[i]->ident)
	    {
	      cache_id = cpus[k]->cpuid->caches_idents[i]->ident;
	      cache =
		sctk_create_node ("cache level",
				  cpus[k]->cpuid->caches_idents[i]->level);
	      sctk_add_son (node, cache);
	    }
	  sctk_add_son (cache, cpus[k]);
	}

      free (cpus);
      i--;
      if (i >= 0)
	{
	  for (k = 0; k < node->nb_sons; k++)
	    {
	      sctk_build_cache_hierarchy (node->sons[k], i);
	    }
	}
    }
  else
    {

    }
}

static topology_node_t **sctk_cpuid_list;

static int
sctk_find_cpus (topology_node_t * node)
{
  char *sys_dir = NULL;
  int i;
  FILE *fd = NULL;
  topology_node_t *cpu;
  topology_node_t *socket;
  int socket_nb = 0;
  int socket_id;
  topology_node_t **cpus;
  int cpu_nb;
  sys_dir = malloc (SMALL_BUFFER_SIZE);

  for (i = 0; i < sctk_processor_number_on_node; i++)
    {
      sprintf (sys_dir, "/sys/devices/system/node/node%ld/cpu%d", node->ident,
	       i);
      fd = fopen (sys_dir, "r");
      if (fd)
	{
	  cpu = sctk_create_node ("cpu_id", i);
	  cpu->cpuid = malloc (sizeof (sctk_cpuid_t));
	  sctk_cpuid (cpu->cpuid, node->ident, i);
	  sctk_add_son (node, cpu);

	  sctk_cpuid_list[i] = cpu;

	  fclose (fd);
	}
    }

  qsort (node->sons, node->nb_sons, sizeof (topology_node_t *),
	 (int (*)(const void *, const void *)) sctk_compar);
  cpus = node->sons;
  cpu_nb = node->nb_sons;

  node->sons = NULL;
  node->nb_sons = 0;

  i = 0;

  if (cpus == NULL)
    {
      return 1;
    }

  socket_id = cpus[i]->cpuid->socket_id;
  socket = sctk_create_node ("socket_id", socket_nb);
  socket_nb++;
  sctk_add_son (node, socket);
  sctk_add_son (socket, cpus[i]);
  for (i = 1; i < cpu_nb; i++)
    {
      if (socket_id != cpus[i]->cpuid->socket_id)
	{
	  sctk_build_cache_hierarchy (socket, cpus[0]->cpuid->cache_nb - 1);
	  socket_id = cpus[i]->cpuid->socket_id;
	  socket = sctk_create_node ("socket_id", socket_nb);
	  sctk_add_son (node, socket);
	  socket_nb++;
	}
      sctk_add_son (socket, cpus[i]);
    }
  sctk_build_cache_hierarchy (socket, cpus[0]->cpuid->cache_nb - 1);

  free (cpus);
  free (sys_dir);
  return 0;
}

static topology_node_t *
sctk_build_topology_tree ()
{
  char *sys_dir = NULL;
  int nodes = 0;
  topology_node_t *system;

  sctk_determine_processor_number ();
  sctk_cpuid_list =
    realloc (sctk_cpuid_list,
	     sctk_processor_number_on_node * sizeof (topology_node_t *));

  system = sctk_create_node ("system", 0);

  struct dirent *entry;
  DIR *dir;

  dir = opendir( "/sys/devices/system/node/" );
  
  if( !dir )
  {
    sctk_error("Failed to open /sys/devices/system/node/ to read topology");
    abort();
  }

  int real_node = 0;

  while((entry = readdir(dir)))
  {
    topology_node_t *node;

    if( !strstr(entry->d_name, "node") || strlen(entry->d_name) < 5 )
      continue;

    real_node = atoi( strstr(entry->d_name, "node") + 4 );

    node = sctk_create_node ("node", real_node);
    sctk_add_son (system, node);
    if (sctk_find_cpus (node))
      {
        return NULL;
      }


    nodes++;
  }

  closedir(dir);

  return system;
}

static void
sctk_view_tree_iter (topology_node_t * system, char *tab)
{
  int i;
  char tmp[4096];

  printf ("%s- %s %ld\n", tab, system->type, system->ident);

  sprintf (tmp, "%s\t|", tab);
  for (i = 0; i < system->nb_sons; i++)
    {
      sctk_view_tree_iter (system->sons[i], tmp);
    }
}

static void
sctk_view_tree (topology_node_t * system)
{
  sctk_view_tree_iter (system, "");
}

static int sctk_current_proc = 0;
static int
sctk_update_numa_topology_iter (topology_node_t * system)
{
  int i;

  if (system->nb_sons == 0)
    {
      sctk_cpuinfos[sctk_current_proc].cpu_id = system->ident;
      if (system->cpuid)
	{
	  sctk_cpuinfos[sctk_current_proc].core_id = system->cpuid->core_id;
	}
      else
	{
	  return 1;
	}
      sctk_current_proc++;
    }

  if (strcmp (system->type, "node") == 0)
    {
      for (i = sctk_current_proc; i < sctk_processor_number_on_node; i++)
	{
	  sctk_cpuinfos[i].numa_id = system->ident;
	}
    }

  if (strcmp (system->type, "socket_id") == 0)
    {
      for (i = sctk_current_proc; i < sctk_processor_number_on_node; i++)
	{
	  sctk_cpuinfos[i].socket_id = system->ident;
	}
    }

  for (i = 0; i < system->nb_sons; i++)
    {
      if (sctk_update_numa_topology_iter (system->sons[i]))
	{
	  return 1;
	}
    }
  return 0;
}
static int
sctk_update_numa_topology (topology_node_t * system)
{
  return sctk_update_numa_topology_iter (system);
}

static int
sctk_update_numa_linux_gen_check ()
{
  static char name[4096];
  FILE *fd = NULL;
  int i; 
  fd = fopen ("/sys/devices/system/node/node0", "r");
  if (fd == NULL)
    {
      return 1;
    }
  fclose (fd);

  for(i = 0; i < sctk_real_processor_number_on_node; i++){
    sprintf(name,"/sys/devices/system/node/node0/cpu%d",i);
    fd = fopen (name, "r");
    if (fd != NULL)
      {
	sctk_nodebug("%s found",name);
	break;
      }
  }

  if (fd == NULL)
    {
      return 1;
    }
  
  fclose (fd);
  sprintf(name,"/sys/devices/system/node/node0/cpu%d/topology/core_id",i);
  fd = fopen (name, "r");
  if (fd == NULL)
    {
      return 1;
    }
  fclose (fd);
  return 0;
}

static int
sctk_update_numa_linux_gen ()
{
  topology_node_t *system;

  if (sctk_update_numa_linux_gen_check ())
    {
      sctk_init_sctk_cpuinfos ();
      sctk_warning ("Unable to initialize topology");
      return 0;
    }
  system = sctk_build_topology_tree ();
  if (system == NULL)
    {
      sctk_init_sctk_cpuinfos ();
      return 0;
    }

  /* sctk_view_tree(system); */
  if (sctk_update_numa_topology (system))
    {
      sctk_init_sctk_cpuinfos ();
    }

  if (sctk_cpuinfos[sctk_processor_number_on_node - 1].numa_id)
    {
      sctk_node_number_on_node =
	sctk_cpuinfos[sctk_processor_number_on_node - 1].numa_id + 1;
    }

  sctk_restrict_topology ();

  return 1;
}

#endif
#endif
