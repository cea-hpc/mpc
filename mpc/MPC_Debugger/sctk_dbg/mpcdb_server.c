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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libgen.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

int verbose = 0;
int argc;
char **argv;
int pid;

pthread_attr_t attr;

static void
sctk_debug (const char *fmt, ...)
{
  va_list ap;
  if (verbose)
    {
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fflush (stderr);
    }
}

static void
sctk_error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  abort ();
}

static int
sctk_tcp_connect_to (int portno, char *name)
{
  int clientsock_fd;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  sctk_debug ("Try connection to %s on port %d\n", name, portno);

  clientsock_fd = socket (AF_INET, SOCK_STREAM, 0);
  if (clientsock_fd < 0)
    sctk_error ("ERROR opening socket");
  server = gethostbyname (name);
  if (server == NULL)
    {
      fprintf (stderr, "ERROR, no such host\n");
      exit (0);
    }
  memset ((char *) &serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  memmove ((char *) &serv_addr.sin_addr.s_addr,
	   (char *) server->h_addr, server->h_length);
  serv_addr.sin_port = htons ((unsigned short) portno);
  if (connect
      (clientsock_fd, (struct sockaddr *) (&serv_addr),
       sizeof (serv_addr)) < 0)
    {
      perror ("ERROR connecting");
      abort ();
    }

  sctk_debug ("Try connection to %s on port %d done\n", name, portno);
  return clientsock_fd;
}

int filedes[2];
int filedes_in[2];
int rank;

static int
launch ()
{
  int status;


  pid = fork ();

  if (pid)
    {
      sctk_debug ("Wait %d\n", pid);
      waitpid (pid, &status, WUNTRACED | WCONTINUED);
      if (WIFEXITED (status))
	{
	  status = WEXITSTATUS (status);
	}
      if (WIFSIGNALED (status))
	{
	  status = WTERMSIG (status);
	}
      sctk_debug ("Wait done %d %d\n", pid, status);
    }
  else
    {
      static char error[4096];
      argv[0] = "gdb";

      dup2 (filedes[1], 1);
      dup2 (filedes[1], 2);
      dup2 (filedes_in[0], 0);

      execvp (argv[0], &(argv[0]));
      sprintf (error, "Fail to execute %s", argv[1]);
      perror (error);
    }
  return status;
}

int port;
char *remote;
int fd;

static pthread_mutex_t reader_lock = PTHREAD_MUTEX_INITIALIZER;

static void *
reader (void *arg)
{
  int rd;
  char msg;
  assert (arg == NULL);
  do
    {
      rd = read (filedes[0], &msg, 1);
      if (rd == 1)
	{
	  pthread_mutex_lock (&reader_lock);
	  write (fd, &msg, 1);
	  pthread_mutex_unlock (&reader_lock);
	}
    }
  while (rd == 1);
  return NULL;
}

static ssize_t
safe_read (int fd, void *buf, size_t count)
{
  char *tmp;
  ssize_t allready_readed = 0;
  ssize_t dcount = 0;

  tmp = buf;
  while (allready_readed < (ssize_t)count)
    {
      tmp += dcount;
      dcount = read (fd, tmp, count - allready_readed);
      if (!(dcount >= 0))
	{
	  perror ("safe_read");
	}
      assert (dcount >= 0);
      allready_readed += dcount;
    }
  assert ((ssize_t)count == allready_readed);
  return allready_readed;
}

static ssize_t
safe_write (int fd, const void *buf, size_t count)
{
  ssize_t dcount;
  dcount = write (fd, buf, count);
  if (!(dcount == (ssize_t)count))
    {
      perror ("safe_write");
    }
  assert (dcount == (ssize_t)count);
  return dcount;
}

static char *init_gdb = "\
handle SIGUSR1 pass nostop\n\
define mpc_debug\n\
call sctk_dbg_init()\n\
set scheduler-locking step\n\
end\n\
define mpc_thread_list\n\
set scheduler-locking on\n\
call sctk_thread_list()\n\
set scheduler-locking step\n\
end\n\
define mpc_thread_list_task\n\
set scheduler-locking on\n\
call sctk_thread_list_task($arg0)\n\
set scheduler-locking step\n\
end\n\
define mpc_run\n\
break mpc_user_main__\n\
run\n\
break sctk_thread_print_stack_out\n\
mpc_debug\n\
continue\n\
end\n\
define mpc_thread_cur \n\
set scheduler-locking off\n\
set scheduler-locking on\n\
call sctk_thread_current()\n\
set scheduler-locking step\n\
end\n\
define mpc_thread\n\
break sctk_thread_print_stack_out\n\
set scheduler-locking off\n\
set scheduler-locking on\n\
call sctk_thread_print_stack_in($arg0)\n\
set scheduler-locking step\n\
end\n\
define mpc_task\n\
set scheduler-locking off\n\
set scheduler-locking on\n\
call sctk_thread_print_stack_task($arg0)\n\
set scheduler-locking step\n\
end\n\
define mpc_continue\n\
continue\n\
set scheduler-locking step\n\
end\n\
define mpc_where\n\
where\n\
end\n\
";

static void *
reader_client (void *arg)
{
  char *msg = NULL;
  size_t msg_size = 0;
  char TYPE[8];
  assert (arg == NULL);

  /*Init MPC environnement */
  safe_write (filedes_in[1], init_gdb, strlen (init_gdb));

  do
    {
      size_t size;
      read (fd, &size, sizeof (size_t));
      memset (TYPE, '\0', 8);

      read (fd, TYPE, 8 * sizeof (char));
      if (msg_size < size)
	{
	  msg_size = size + 50;
	  msg = realloc (msg, msg_size);
	  assert (msg_size <= 1024 * 1024);
	}
      memset (msg, '\0', msg_size);

      if (strcmp (TYPE, "DBG") == 0)
	{
	  safe_read (fd, msg, size);
	  sctk_debug ("RECV |%s| |%s|\n", TYPE, msg);
	  safe_write (filedes_in[1], msg, size);
	}
      else if (strcmp (TYPE, "BREAK") == 0)
	{
	  sctk_debug ("RECV interruption %d\n", pid);
	  safe_read (fd, msg, size);
	  kill (pid, SIGINT);
	  sctk_debug ("RECV interruption kill done\n");
	}
      else if (strcmp (TYPE, "FLUSH") == 0)
	{
	  char prompt[256];
	  safe_read (fd, msg, size);
	  sprintf (prompt, "(gdb)");
	  pthread_mutex_lock (&reader_lock);
	  write (fd, prompt, strlen (prompt));
	  pthread_mutex_unlock (&reader_lock);
	}
      else
	{
	  assert (0);
	}
    }
  while (1);
}

static void
sctk_handler (int signo, siginfo_t * info, void *context)
{
  sctk_debug ("PROPAGATE SIGNAL SERVER %d\n", signo);
  kill (pid, signo);
}

static void
set_handler ()
{
  struct sigaction sigparam;
  sigparam.sa_flags = SA_SIGINFO;
  sigparam.sa_sigaction = sctk_handler;
  sigemptyset (&sigparam.sa_mask);
  sigaction (SIGINT, &sigparam, NULL);
}

int
main (int argc_l, char **argv_l)
{
  pthread_t pidt;
  pthread_t pidt2;
  char *dbg_port;
  char *dbg_host;
  int rc;
  if (getenv ("MPC_DEBUG_VERBOSE") != NULL)
    {
      verbose = 1;
    }

  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, 128 * 1024);

  argc = argc_l;
  argv = argv_l;
  dbg_port = getenv ("MPC_DEBUG_PORT");
  dbg_host = getenv ("MPC_DEBUG_HOST");
  sctk_debug ("server on %s %s\n", dbg_port, dbg_host);

  port = atoi (getenv ("MPC_DEBUG_PORT"));
  remote = getenv ("MPC_DEBUG_HOST");

  fd = sctk_tcp_connect_to (port, remote);
  read (fd, &rank, sizeof (int));
  sctk_debug ("New fd %d rank %d\n", fd, rank);

  pipe (filedes);
  pipe (filedes_in);
  pthread_create (&pidt, &attr, reader, NULL);
  pthread_create (&pidt2, &attr, reader_client, NULL);

  set_handler ();
  rc = launch ();

  return rc;
}
