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
#include <stdio.h>
#include <signal.h>

int verbose = 0;
char *mpc_args;

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

static char port[8192];
static int sockfd;

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

static void
sctk_create_recv_socket ()
{
  int portno;
  struct sockaddr_in serv_addr;

  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    sctk_error ("ERROR opening socket");

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));

  portno = 1023;
  sctk_debug ("Looking for an available port\n");
  do
    {
      portno++;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons ((unsigned short) portno);

    }
  while (bind (sockfd, (struct sockaddr *) &serv_addr,
	       sizeof (serv_addr)) < 0);
  sctk_debug ("Looking for an available port found %d\n", portno);
  sprintf (port, "%d", portno);

  listen (sockfd, 1);
}

#define MAX_SERVER 4096
int volatile file_list[MAX_SERVER];
typedef enum
{ server_mode_all, server_mode_process } server_mode_t;
server_mode_t mode;
int process;
FILE *out;

static void *
writer (void *arg)
{
  char msg = '\0';
  int rd;
  long val;
  int fd;
  size_t pos = 0;
  size_t buffer_size = 0;
  char *message = NULL;
  char prompt[256];
  size_t prompt_size;
  int line_nb = 0;


  val = (long) arg;
  fd = file_list[val];
  assert (fd > 0);
  sprintf (prompt, "(gdb)");
  prompt_size = strlen (prompt);

  sctk_debug ("Use fd %d\n", fd);

  do
    {
      rd = safe_read (fd, &msg, 1);

      if (pos + 5 >= buffer_size)
	{
	  buffer_size += 1024;
	  message = realloc (message, buffer_size);
	  assert (buffer_size <= 1024 * 1024);
	}

      message[pos] = msg;
      pos++;
      message[pos] = '\0';

      if (msg == '\n')
	{
	  line_nb++;
	  message[pos] = '\t';
	  pos++;
	  message[pos] = '\0';
	}

      if (pos >= prompt_size)
	{
	  if ((strcmp (&(message[pos - prompt_size]), prompt) ==
	       0) /* || (line_nb >= 10) */ )
	    {
	      static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	      if (pos > prompt_size)
		{
		  pthread_mutex_lock (&lock);
		  fprintf (out, "PROCESS %ld --->\n\t%s\nPROCESS %ld <---\n",
			   val, message, val);
		  fflush (out);
		  pthread_mutex_unlock (&lock);
		}
	      pos = 0;
	      line_nb = 0;
	    }
	}

    }
  while (rd == 1);
  return NULL;
}

static void *
server (void *arg)
{
  pthread_t pid;
  int fd;
  unsigned int clilen;
  struct sockaddr_in cli_addr;
  long val = 0;
  clilen = sizeof (cli_addr);

  assert (arg == NULL);

  sctk_debug ("Launch server\n");
  while (1)
    {
      fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (fd < 0)
	{
	  perror ("accept");
	}
      assert (fd >= 0);
      file_list[val] = fd;
      safe_write (fd, &val, sizeof (int));
      pthread_create (&pid, &attr, writer, (void *) val);
      val++;
      sctk_debug ("Connection done process %d\n", val - 1);
    }
}

static void
send_to_server (int fd, char *buf, size_t size, char *op)
{
  char TYPE[8];
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock (&lock);
  assert (strlen (op) <= 8);
  memset (TYPE, '\0', 8);
  sprintf (TYPE, op);
  sctk_debug ("SEND to %d, %s, %s\n", fd, TYPE, buf);
  safe_write (fd, &size, sizeof (size_t));
  safe_write (fd, TYPE, 8 * sizeof (char));
  safe_write (fd, buf, size);
  pthread_mutex_unlock (&lock);
}

static void *
reader (void *arg)
{
  char *line = NULL;
  size_t pos = 0;
  size_t buffer_size = 0;
  char *message = NULL;
  int rd;
  char msg;

  assert (arg == NULL);

  do
    {
      size_t size;
      char *op = "DBG";
      pos = 0;
      if (message != NULL)
	memset (message, '\0', buffer_size);


      do
	{
	  rd = safe_read (0, &msg, 1);
	  assert (rd == 1);
	  if (pos + 20 >= buffer_size)
	    {
	      buffer_size += 1024;
	      message = realloc (message, buffer_size);
	      assert (buffer_size <= 1024 * 1024);
	    }
	  message[pos] = msg;
	  pos++;
	  message[pos] = '\0';
	}
      while (msg != '\n');

      line = message;
      size = strlen (line);
      sctk_debug ("READ %s\n", line);

      if (strcmp (line, "halt\n") == 0)
	{
	  op = "BREAK";
	}

      if (strcmp (line, "flush\n") == 0)
	{
	  op = "FLUSH";
	}
      if (strcmp (line, "run\n") == 0)
	{
	  sprintf (line, "mpc_run\n");
	  size = strlen (line);
	}

      if (strcmp (line, "r\n") == 0)
	{
	  sprintf (line, "mpc_run\n");
	  size = strlen (line);
	}

      if (strcmp (line, "thread\n") == 0)
	{
	  sprintf (line, "mpc_thread\n");
	  size = strlen (line);
	}
      if (strcmp (line, "quit\n") == 0)
	{
	  mode = server_mode_all;
	}

      if (strncmp (line, "process ", strlen ("process ")) == 0)
	{
	  char *tmp_msg;
	  pos--;
	  mode = server_mode_process;
	  tmp_msg = &(message[strlen ("process ")]);
	  while (*tmp_msg == ' ')
	    tmp_msg++;
	  if (strcmp (tmp_msg, "all\n") != 0)
	    {
	      process = atoi (tmp_msg);
	      fprintf (out, "Debug process %d only\n", process);
	    }
	  else
	    {
	      mode = server_mode_all;
	      process = -1;
	      fprintf (out, "Debug all processes\n");
	    }
	}
      else if ((mode == server_mode_process) && (process == -1))
	{
	  pos--;
	  message[pos] = '\0';
	  process = atoi (message);
	  fprintf (out, "Debug process %d only\n", process);
	}
      else if (strcmp (line, "server_mode_all\n") == 0)
	{
	  mode = server_mode_all;
	  fprintf (out, "Debug all processes\n");
	}
      else if (strcmp (line, "server_mode_process\n") == 0)
	{
	  mode = server_mode_process;
	  process = -1;
	  fprintf (out, "Debug all processes\n");
	}
      else
	{
	  switch (mode)
	    {
	    case server_mode_all:
	      {
		int i;
		sctk_debug ("WRITE server_mode_all %s\n", line);
		for (i = 0; i < MAX_SERVER; i++)
		  {
		    if (file_list[i] > 0)
		      {
			send_to_server (file_list[i], line, size, op);
		      }
		  }
		break;
	      }
	    case server_mode_process:
	      {
		sctk_debug ("WRITE server_mode_process %s\n", line);
		send_to_server (file_list[process], line, size, op);
		break;
	      }
	    default:
	      assert (0);
	    }
	}
    }
  while (line != 0);

  return NULL;
}

static void
sctk_handler (int signo, siginfo_t * info, void *context)
{
  int i;
  fprintf (out, "Send interruption\n");
  sctk_debug ("PROPAGATE SIGNAL CLIENT %d\n", signo);
  for (i = 0; i < MAX_SERVER; i++)
    {
      if (file_list[i] > 0)
	{
	  send_to_server (file_list[i], "halt", 4, "BREAK");
	}
    }
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

static int
launch ()
{
  int status;
  pid = fork ();
  if (pid)
    {
      set_handler ();
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
      close (0);
      execvp (argv[1], &(argv[1]));
      sprintf (error, "Fail to execute %s", argv[1]);
      perror (error);
    }
  return status;
}

#define MAX_HOST_SIZE 255
static char hostname[MAX_HOST_SIZE];

int
main (int argc_l, char **argv_l)
{
  int i;
  pthread_t pidt;
  pthread_t pidt2;
#ifndef SCTK_HAVE_SETENV
  char MPC_DEBUG_PORT[4096];
  char MPC_DEBUG_HOST[4096];
#endif
  if (getenv ("MPC_DEBUG_VERBOSE") != NULL)
    {
      verbose = 1;
    }
  out = stdout;

  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, 128 * 1024);

  argc = argc_l;
  argv = argv_l;

  gethostname (hostname, MAX_HOST_SIZE);

  for (i = 1; i < argc; i++)
    {
      sctk_debug ("ARG %d = %s\n", i, argv[i]);
    }
  mpc_args = getenv ("MPC_STARTUP_ARGS");
  sctk_debug ("MPC ARGS = %s\n", mpc_args);

  sctk_create_recv_socket ();
#ifdef SCTK_HAVE_SETENV
  setenv ("MPC_DEBUG_PORT", port, 1);
  setenv ("MPC_DEBUG_HOST", hostname, 1);
#else
  sprintf (MPC_DEBUG_PORT, "%s=%s", "MPC_DEBUG_PORT", port);
  putenv (MPC_DEBUG_PORT);
  sprintf (MPC_DEBUG_HOST, "%s=%s", "MPC_DEBUG_HOST", hostname);
  putenv (MPC_DEBUG_HOST);
#endif

  pthread_create (&pidt, &attr, server, NULL);
  pthread_create (&pidt2, &attr, reader, NULL);

  return launch ();
}
