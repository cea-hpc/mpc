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
#if 0
#ifdef MPC_Allocator
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

static FILE *memtrace;
static int state = 0;
static pthread_t pid;
static int gpid = 0;
static size_t page;

static mpc_inline unsigned long
get_mem_usage ()
{
  char name[2048];
  char a[2048];
  char b[2048];
  char c[2048];
  FILE *fd;
  unsigned long res;

  sprintf (name, "/proc/%d/statm", gpid);
  fd = fopen (name, "r");
  fscanf (fd, "%s %s %s", a, b, c);
  fclose (fd);
  res = (double) atol (a);
  res = res * page;
  return res;
}

typedef struct proc_t
{
  int tid, ppid;
  char state;

  unsigned sctk_long_long utime, stime, cutime, cstime, start_time;

  unsigned long vsize, flags, min_flt, maj_flt, cmin_flt, cmaj_flt;

  long priority, nice, rss, alarm;

  int pgrp, session, tty, tpgid, nlwp;
} proc_t;

static mpc_inline void
get_time_usage (unsigned sctk_long_long * user_time,
		unsigned sctk_long_long * system_time, unsigned long *vsize,
		unsigned long *rsize)
{
  FILE *fd;
  char name[2048];
  char *ptr;
  proc_t P;

  sprintf (name, "/proc/%d/stat", gpid);
  fd = fopen (name, "r");
  fread (name, 1, 2048, fd);
  fclose (fd);
  name[2047] = '\0';

  ptr = name;

  while (*ptr != ')')
    ptr++;

  ptr++;
  ptr++;
  sscanf (ptr, "%c " "%d %d %d %d %d " "%lu %lu %lu %lu %lu " "%Lu %Lu %Lu %Lu "	/* utime stime cutime cstime */
	  "%ld %ld " "%d " "%ld " "%Lu "	/* start_time */
	  "%lu "
	  "%lu ",
	  &P.state,
	  &P.ppid, &P.pgrp, &P.session, &P.tty, &P.tpgid,
	  &P.flags, &P.min_flt, &P.cmin_flt, &P.maj_flt, &P.cmaj_flt,
	  &P.utime, &P.stime, &P.cutime, &P.cstime,
	  &P.priority, &P.nice,
	  &P.nlwp, &P.alarm, &P.start_time, &P.vsize, &P.rss);

/*  fprintf(memtrace,"#%s",ptr);*/

  *user_time = P.utime;
  *system_time = P.stime;
  *vsize = get_mem_usage ();
  *rsize = P.rss * page;
}

static void *
run (void *arg)
{
  unsigned long iter = 0;
  unsigned sctk_long_long usage_user;
  unsigned sctk_long_long usage_sys;
  unsigned long vsize;
  unsigned long rsize;

  get_time_usage (&usage_user, &usage_sys, &vsize, &rsize);
  fprintf (memtrace, "%lu\t%lu\t%lu\t%Lu\t%Lu\n",
	   iter, vsize / 1024, rsize / 1024, usage_user, usage_sys);

  while (state)
    {
      usleep (100);
      iter++;
      get_time_usage (&usage_user, &usage_sys, &vsize, &rsize);
      fprintf (memtrace, "%lu\t%lu\t%lu\t%Lu\t%Lu\n",
	       iter, vsize / 1024, rsize / 1024, usage_user, usage_sys);
    }
  return NULL;
}

static void __attribute__ ((constructor)) init (void)
{
  char name[512];
  char *env_name;
  gpid = getpid ();
  page = getpagesize ();
  fprintf (stderr, "Init mem tracer\n");

  env_name = getenv ("SCTK_ALLOC_NAME");
  if (env_name == NULL)
    {
      sprintf (name, "memory_trace");
      memtrace = fopen (name, "w");
    }
  else
    {
      memtrace = fopen (env_name, "w");
    }
  assert (memtrace != NULL);
  fprintf (memtrace, "#Memtrace\n");
  fprintf (memtrace, "#time size\n");
  state = 1;
  pthread_create (&pid, NULL, run, NULL);
}

static void __attribute__ ((destructor)) dest (void)
{
  state = 0;
  pthread_join (pid, NULL);
  fclose (memtrace);
  fprintf (stderr, "End mem tracer\n");
}
#endif
#endif
