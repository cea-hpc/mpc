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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "mpcrun_env_var.h"

int my_rank = -1;
int fd_server = 0;

void *
thread_listener (void *arg)
{
  size_t size;
  safe_read (fd_server, &size, sizeof (size_t));

  return NULL;
}

int
main (int argc, char **argv)
{
  int i;
  int env_number;
  pthread_t pid;
  char str_rank[4096];

  process_number = 1;
  launch_pid = malloc (process_number * sizeof (pid_t));

  argc--;
  sprintf (hostname, "%s", argv[argc]);
  argv[argc] = NULL;
  argc--;
  sprintf (port, "%s", argv[argc]);
  argv[argc] = NULL;
  argc--;
  if (chdir (argv[argc]))
    {
      static char error[4096];
      sprintf (error, "Fail to chdir %s", argv[argc]);
      perror (error);
    }
  argv[argc] = NULL;

  sctk_debug ("\nLaunch:");
  for (i = 1; i < argc; i++)
    {
      add_argument (argv[i]);
    }
  sctk_debug ("\n");

  fd_server = sctk_tcp_connect_to (atoi (port), hostname);
  sctk_create_recv_socket ();

  safe_read (fd_server, &verbose, sizeof (int));
  sctk_debug ("Verbose %d\n", verbose);
  safe_read (fd_server, &my_rank, sizeof (int));
  sctk_debug ("Rank %d\n", my_rank);
  safe_read (fd_server, &process_number, sizeof (int));
  sctk_debug ("process_number %d\n", process_number);

  safe_read (fd_server, &env_number, sizeof (int));
  for (i = 0; i < env_number; i++)
    {
      size_t size;
      char *name;
      char *val;
      safe_read (fd_server, &size, sizeof (size_t));
      sctk_debug ("Recv %lu\n", size);
      name = malloc (size);
      safe_read (fd_server, name, size);
      sctk_debug ("Recv %s\n", name);

      safe_read (fd_server, &size, sizeof (size_t));
      sctk_debug ("Recv %lu\n", size);
      if (size > 0)
	{
	  val = malloc (size);
	  safe_read (fd_server, val, size);
	}
      else
	{
	  val = NULL;
	}

      sctk_debug ("%s='%s'\n", name, val);
      if (val != NULL)
	{
	  setenv (name, val, 1);
	}
      else
	{
	  unsetenv (name);
	}
    }

  sprintf (str_rank, "%d", my_rank);
  setenv ("MPC_PROCESS_RANK", str_rank, 1);
  host_list = malloc (process_number * sizeof (char *));
  for (i = 0; i < process_number; i++)
    {
      size_t size;
      safe_read (fd_server, &size, sizeof (size_t));
      host_list[i] = malloc (size);
      safe_read (fd_server, host_list[i], size);
    }

  set_handler ();
  pthread_create (&pid, NULL, thread_listener, NULL);
  launch (0);
  return status;
}
