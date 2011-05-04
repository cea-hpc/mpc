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


#ifdef SCTK_KERNEL_THREAD_USE_TLS
static __thread void *sctk_topology_key;
#else
static kthread_key_t sctk_topology_key;
#endif

#include <stdio.h>
#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#define SCTK_MAX_PROCESSOR_NAME 100
#define SCTK_MAX_NODE_NAME 512
#define SCTK_MAX_CPUINFO_NUMBER 512
#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1
typedef struct sctk_cpuinfo_s
{
  int numa_id;			/*NUMA node associated with this CPU */
  int socket_id;
  int core_id;

  int cpu_id;			/*Real id of the CPU */

  int i;
} sctk_cpuinfo_t;

static int sctk_processor_number_on_node = 0;
static int sctk_real_processor_number_on_node = 0;
static int sctk_node_number_on_node = 0;
static sctk_cpuinfo_t sctk_cpuinfos[SCTK_MAX_CPUINFO_NUMBER];
static char sctk_node_name[SCTK_MAX_NODE_NAME];

static void
sctk_init_sctk_cpuinfos ()
{
  int i;
  for (i = 0; i < SCTK_MAX_CPUINFO_NUMBER; i++)
    {
      sctk_cpuinfos[i].numa_id = -1;
      sctk_cpuinfos[i].socket_id = -1;
      sctk_cpuinfos[i].core_id = -1;

      sctk_cpuinfos[i].cpu_id = i;
      sctk_cpuinfos[i].i = i;
    }
  sctk_node_number_on_node = 0;
}

static void sctk_determine_processor_number ();

static void
sctk_restrict_topology ()
{
  /* Disable SMT capabilities */
  if (sctk_enable_smt_capabilities)
    {
	sctk_warning ("SMT capabilities ENABLED");

    }
  else
    {
      int i;
      sctk_cpuinfo_t current;
      int processor_number;

      processor_number = sctk_processor_number_on_node;
      memcpy (&current, &(sctk_cpuinfos[0]), sizeof (sctk_cpuinfo_t));
      for (i = 1; i < sctk_processor_number_on_node;)
	{
	  if ((current.numa_id == sctk_cpuinfos[i].numa_id) &&
	      (current.socket_id == sctk_cpuinfos[i].socket_id) &&
	      (current.core_id == sctk_cpuinfos[i].core_id))
	    {
	      int j;
	      sctk_nodebug ("Remove %d", sctk_cpuinfos[i].cpu_id);
	      processor_number--;

	      for (j = i + 1; j < sctk_processor_number_on_node; j++)
		{
		  memcpy (&(sctk_cpuinfos[j - 1]), &(sctk_cpuinfos[j]),
			  sizeof (sctk_cpuinfo_t));
		}

	    }
	  else
	    {
	      memcpy (&current, &(sctk_cpuinfos[i]), sizeof (sctk_cpuinfo_t));
	      i++;
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
      sctk_cpuinfo_t current;

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

      /* Set processor start number */
      for (i = 0; i < sctk_processor_number_on_node; i++)
	{
	  memcpy (&current, &(sctk_cpuinfos[i + start]),
		  sizeof (sctk_cpuinfo_t));
	  memcpy (&(sctk_cpuinfos[i]), &current, sizeof (sctk_cpuinfo_t));
	}
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
/*   sctk_print_topology (stderr); */
}

#include <sctk_topology_cpuset.h>
#include <sctk_topology_libnuma.h>
#include <sctk_topology_sched_affinity.h>
#include <sctk_topology_none.h>


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
  long sctk_processor_number_on_node_tmp;
  /*   Détermination du nombre de processeurs disponibles */
#ifdef SunOS_SYS
  sctk_processor_number_on_node_tmp = sysconf (_SC_NPROCESSORS_CONF);
#elif defined(Linux_SYS)
  sctk_processor_number_on_node_tmp = sysconf (_SC_NPROCESSORS_CONF);
#elif defined(IRIX64_SYS)
  sctk_processor_number_on_node_tmp = sysconf (_SC_NPROC_CONF);
#elif defined(OSF1_SYS)
  sctk_processor_number_on_node_tmp = sysconf (_SC_NPROCESSORS_CONF);
#elif defined(AIX_SYS)
  sctk_processor_number_on_node_tmp = sysconf (_SC_NPROCESSORS_CONF);
#elif defined(HP_UX_SYS)
  do
    {
      struct pst_dynamic psd;
      pstat_getdynamic (&psd, sizeof (struct pst_dynamic), (size_t) 1, 0);
      sctk_processor_number_on_node_tmp = psd.psd_proc_cnt;
    }
  while (0);
#elif defined(WINDOWS_SYS)
  sctk_processor_number_on_node_tmp = pthread_num_processors_np ();
#else
#error unknown system
  sctk_processor_number_on_node = 1;
  sctk_abort ();
#endif
  sctk_processor_number_on_node = (int) sctk_processor_number_on_node_tmp;
  if (SCTK_MAX_CPUINFO_NUMBER <= sctk_processor_number_on_node)
    {
      fprintf (stderr, "Too many processors\n");
      sctk_abort ();
    }
  sctk_real_processor_number_on_node = sctk_processor_number_on_node;
}

int
sctk_get_cpu ()
{
  sctk_cpuinfo_t *old_cpu;
  old_cpu = (sctk_cpuinfo_t *) kthread_getspecific (sctk_topology_key);

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
sctk_update_memory (void)
{
}


void
sctk_topology_init ()
{
  sctk_only_once ();
  sctk_print_version ("Topology", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);

  kthread_key_create (&sctk_topology_key, NULL);

  sctk_determine_processor_number ();
  memset (sctk_cpuinfos, 0,
	  sctk_processor_number_on_node * sizeof (sctk_cpuinfo_t));

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

char *
sctk_get_node_name ()
{
  return sctk_node_name;
}

void
sctk_print_topology (FILE * fd)
{
  int i;
  fprintf (fd, "Node %s: %s %s %s\n", sctk_node_name, utsname.sysname,
	   utsname.release, utsname.version);
  for (i = 0; i < sctk_processor_number_on_node; i++)
    {
      fprintf (fd, "\tProcessor %4d real %4d (%4d:%4d:%4d)\n", i,
	       sctk_cpuinfos[i].cpu_id,
	       sctk_cpuinfos[i].numa_id,
	       sctk_cpuinfos[i].socket_id, sctk_cpuinfos[i].core_id);
    }
  fprintf (fd, "\tNUMA: %d\n", sctk_is_numa_node ());
  if (sctk_is_numa_node ())
    {
      fprintf (fd, "\t\t* Node nb %d\n", sctk_node_number_on_node);
    }
}

char *
sctk_get_processor_name ()
{
  return utsname.machine;
}

int
sctk_get_node_from_cpu (int cpu)
{
  return sctk_cpuinfos[cpu].numa_id;
}

int
sctk_get_first_cpu_in_node (int node)
{
  int i;
  for (i = 0; i < sctk_processor_number_on_node; i++)
    {
      if (sctk_cpuinfos[i].numa_id == node)
	return i;
    }
  return 0;
}

int
sctk_get_cpu_number ()
{
  return sctk_processor_number_on_node;
}

int
sctk_set_cpu_number (int n)
{
  sctk_processor_number_on_node = n;
  return n;
}

int
sctk_is_numa_node ()
{
  return sctk_node_number_on_node != 0;
}

void sctk_get_neighborhood(int cpuid, int nb_cpus, int* neighborhood){

  int max_numa = 0;
  int max_socket = 0;
  int max_core = 0;
  int neighbor_index = 0;
  sctk_cpuinfo_t * cpuinfo ;
  int n, s, c, i ;

  /* Grab the info on the CPU 'cpuid' */
  cpuinfo = &sctk_cpuinfos[ cpuid ] ;

  /* Compute the number of NUMA nodes on this architecture */
  max_numa = sctk_cpuinfos->numa_id ;
  for ( i = 0 ; i < nb_cpus ; i++ ) {
    if ( sctk_cpuinfos[i].numa_id + 1 > max_numa ) {
      max_numa = sctk_cpuinfos[i].numa_id + 1 ;
    }
  }

  sctk_nodebug( "sctk_get_neighborhood: Max NUMA = %d", max_numa ) ;

  for ( n = 0 ; n < max_numa ; n++ ) {
    int current_numa_id = ( cpuinfo->numa_id + n ) % max_numa ;

    /* Compute the number of sockets for this NUMA node */
    max_socket = -1 ;
    for ( i = 0 ; i < nb_cpus ; i++ ) {
      if ( sctk_cpuinfos[i].numa_id == current_numa_id &&
	  sctk_cpuinfos[i].socket_id + 1 > max_socket ) {
	max_socket = sctk_cpuinfos[i].socket_id + 1 ;
      }
    }

    sctk_assert( max_socket > 0 ) ;

    for ( s = 0 ; s < max_socket ; s++ ) {
      int current_socket_id = s ;

      /* For the first time, start from the target cpu */
      if ( n == 0 ) {
	current_socket_id = ( cpuinfo->socket_id + s ) % max_socket ;
      }

      /* Compute the number of cores for this socket */
      max_core = -1 ;
      for ( i = 0 ; i < nb_cpus ; i++ ) {
	if ( sctk_cpuinfos[i].numa_id == current_numa_id &&
	     sctk_cpuinfos[i].socket_id == current_socket_id &&
	    sctk_cpuinfos[i].core_id + 1 > max_core ) {
	  max_core = sctk_cpuinfos[i].core_id + 1 ;
	}
      }

      sctk_assert( max_core > 0 ) ;

      sctk_nodebug( "sctk_get_neighborhood: Found %d cores for node %d %d",
	  max_core, current_numa_id, current_socket_id ) ;


      for ( c = 0 ; c < max_core ; c++ ) {
	int current_core_id =c ;

	/* For the first time, start from the target cpu */
	if ( n == 0 && s == 0 ) {
	  current_core_id = ( cpuinfo->core_id + c ) % max_core ;
	}

	/* Find the CPU which has the correct coordinates */
	for ( i = 0 ; i < nb_cpus ; i++ ) {

	  sctk_nodebug( "sctk_get_neighborhood: Checking %d %d %d with current %d %d %d",
	      sctk_cpuinfos[i].numa_id, sctk_cpuinfos[i].socket_id, sctk_cpuinfos[i].core_id,
	      current_numa_id, current_socket_id, current_core_id ) ;

	  if ( sctk_cpuinfos[i].numa_id == current_numa_id &&
	      sctk_cpuinfos[i].socket_id == current_socket_id &&
	      sctk_cpuinfos[i].core_id == current_core_id ) {

#warning "There is a problem here"
          if( neighbor_index < nb_cpus ) ;
	        neighborhood[ neighbor_index ] = i ;

          neighbor_index++ ;
	  }
	}


      }
    }

  }

  sctk_assert( neighbor_index == nb_cpus ) ;

#if 0
  /* Print the final result */
  fprintf( stderr, "Neighborhood result starting with CPU %d: (NUMA:%d, Socket:%d, Core:%d)\n"
      , cpuid, cpuinfo->numa_id, cpuinfo->socket_id, cpuinfo->core_id ) ;

  for ( i = 0 ; i < nb_cpus ; i++ ) {
    fprintf( stderr, "\tNeighbor[%d] -> CPU %d: (NUMA:%d, Socket:%d, Core:%d)\n"
	, i, neighborhood[i],
	sctk_cpuinfos[ neighborhood[i] ].numa_id,
	sctk_cpuinfos[ neighborhood[i] ].socket_id,
	sctk_cpuinfos[ neighborhood[i] ].core_id ) ;
  }
#endif

}
