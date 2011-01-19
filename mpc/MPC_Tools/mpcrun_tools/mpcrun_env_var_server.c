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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "mpcrun_env_var.h"
#include <libgen.h>



char *mpc_rc_file_name = NULL;
char *variables_to_propagate = NULL;
char *mpc_variables_to_propagate =
  "MPC_STARTUP_ARGS MPC_VERBOSE PWD SCTK_LINUX_DISABLE_ADDR_RADOMIZE MPC_DISABLE_BANNER";

typedef struct
{
  char *name;
  char *value;
} mpc_env_propagate_t;

mpc_env_propagate_t *tab = NULL;
size_t max_size = 0;


void
propagate (char *list)
{
  size_t size;
  size_t i;
  size_t nb_item = 0;
  size_t val = max_size;
  if (list != NULL)
    {
      size = strlen (list);
      nb_item++;
      for (i = 0; i < size; i++)
	{
	  if (list[i] == ' ')
	    {
	      nb_item++;
	    }
	}
      max_size += nb_item;
      tab = realloc (tab, max_size * sizeof (mpc_env_propagate_t));
      for (i = max_size - nb_item; i < max_size; i++)
	{
	  tab[i].name = NULL;
	  tab[i].value = NULL;
	}

      tab[val].name = list;
      for (i = 0; i < size; i++)
	{
	  if (list[i] == ' ')
	    {
	      val++;
	      tab[val].name = &(list[i + 1]);
	      list[i] = '\0';
	    }
	}
    }
}

char *
find_env (char *name)
{
  size_t i;

  for (i = 0; i < max_size; i++)
    {
      if (strcmp (name, tab[i].name) == 0)
	{
	  return tab[i].value;
	}
    }
  return NULL;
}

static volatile int rank = 0;
static pthread_mutex_t rank_lock = PTHREAD_MUTEX_INITIALIZER;

void *
thread_socket (void *arg)
{
  int my_rank;
  int fd;
  int i;
  unsigned int clilen;
  struct sockaddr_in cli_addr;
  clilen = sizeof (cli_addr);
  fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (fd < 0)
    {
      perror ("accept");
    }
  assert (fd >= 0);

  pthread_mutex_lock (&rank_lock);
  my_rank = rank;
  rank++;
  pthread_mutex_unlock (&rank_lock);

  sctk_debug ("Connected %d\n", fd);
  create_new_thread ();

  safe_write (fd, &verbose, sizeof (int));
  safe_write (fd, &my_rank, sizeof (int));
  safe_write (fd, &process_number, sizeof (int));

  sctk_debug ("Send %d\n", verbose);

  safe_write (fd, &max_size, sizeof (int));
  for (i = 0; i < max_size; i++)
    {
      size_t size;
      size = strlen (tab[i].name) + 1;
      safe_write (fd, &size, sizeof (size_t));
      sctk_debug ("Send %lu\n", size);
      safe_write (fd, tab[i].name, size);

      if (tab[i].value != NULL)
	{
	  size = strlen (tab[i].value) + 1;
	  safe_write (fd, &size, sizeof (size_t));
	  sctk_debug ("Send %lu\n", size);
	  safe_write (fd, tab[i].value, size);
	}
      else
	{
	  size = 0;
	  safe_write (fd, &size, sizeof (size_t));
	  sctk_debug ("Send %lu\n", size);
	}
    }
  for (i = 0; i < process_number; i++)
    {
      size_t size;
      size = strlen (host_list[i]) + 1;
      safe_write (fd, &size, sizeof (size_t));
      safe_write (fd, host_list[i], size);
    }
  return NULL;
}

int
main (int argc, char **argv)
{
  size_t i;
  char *tmp;
  char remote_host[8092];
  char client[8092];
  char *dir_name;
  if (getenv ("MPC_VERBOSE"))
    {
      verbose = 1;
    }

  process_number = atoi (argv[1]);
  if (process_number == 0)
    {
      sctk_error ("Unspecified number of processes\n");
      abort ();
    }

  launch_pid = malloc (process_number * sizeof (pid_t));
  host_list = malloc (process_number * sizeof (char *));

  tmp = mpc_variables_to_propagate;
  mpc_variables_to_propagate = malloc (strlen (tmp) + 1);
  memcpy (mpc_variables_to_propagate, tmp, strlen (tmp) + 1);

  mpc_rc_file_name = getenv ("MPC_USE_MPCRC_FILE");
  sctk_debug ("Use %s mpc_rc file %d\n", mpc_rc_file_name, argc);

  variables_to_propagate = getenv ("MPC_ENVIRONNEMENT_PROPAGATE");

  propagate (variables_to_propagate);
  propagate (mpc_variables_to_propagate);

  for (i = 0; i < max_size; i++)
    {
      tab[i].value = getenv (tab[i].name);
      sctk_debug ("%s='%s'\n", tab[i].name, tab[i].value);
    }

  gethostname (hostname, 8192);
  sprintf (port, "0");
  sctk_debug ("\nLaunch environnement server on %s and port %s\n", hostname,
	      port);

  sctk_create_recv_socket ();
  set_handler ();

  sprintf (remote_host, "remote_host");
  sctk_debug ("\nLaunch:");
  add_argument ("ssh");
  add_argument ("-X");
  add_argument (remote_host);
  dir_name = dirname (argv[0]);
  if (dir_name[0] == '/')
    {
      sprintf (client, "%s/mpcrun_env_var_client", dir_name);
    }
  else
    {
      sprintf (client, "%s/mpcrun_env_var_client", find_env ("PWD"));
    }
  add_argument (client);
  for (i = process_number + 2; i < argc; i++)
    {
      add_argument (argv[i]);
    }
  add_argument (find_env ("PWD"));
  add_argument (port);
  add_argument (hostname);
  sctk_debug ("\n");

  for (i = 0; i < process_number; i++)
    {
      host_list[i] = argv[i + 2];
    }

  for (i = 0; i < process_number; i++)
    {
      sprintf (remote_host, argv[i + 2]);
      launch_multiple (i);
    }
  for (i = 0; i < process_number; i++)
    {
      int loc_status;
      waitpid (launch_pid[i], &loc_status, WUNTRACED | WCONTINUED);
      if (loc_status)
	{
	  status = loc_status;
	  if (WIFEXITED (status))
	    {
	      status = WEXITSTATUS (status);
	    }
	  if (WIFSIGNALED (status))
	    {
	      status = WTERMSIG (status);
	    }
	}
      sctk_debug ("Wait done %d %d\n", launch_pid[i], status);
    }

  return status;
}
