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
#include <assert.h>
int verbose = 0;
int process_number = 0;

void
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


void
sctk_error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  abort ();
}

ssize_t
safe_read (int fd, void *buf, size_t count)
{
  char *tmp;
  ssize_t allready_readed = 0;
  ssize_t dcount = 0;

  tmp = buf;
  while (allready_readed < count)
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
  assert (count == allready_readed);
  return allready_readed;
}

ssize_t
safe_write (int fd, const void *buf, size_t count)
{
  ssize_t dcount;
  dcount = write (fd, buf, count);
  if (!(dcount == count))
    {
      perror ("safe_write");
    }
  assert (dcount == count);
  return dcount;
}


char hostname[8192];
char port[8192];
int status = 0;

char **remote_argv = NULL;
int remote_argc = 0;

void
add_argument (char *arg)
{
  remote_argv = realloc (remote_argv, (remote_argc + 2) * sizeof (char *));
  remote_argv[remote_argc] = arg;
  remote_argv[remote_argc + 1] = NULL;
  remote_argc++;
  sctk_debug ("%s ", arg);
}

char **host_list;
pid_t *launch_pid;
void
launch (int i)
{

  launch_pid[i] = fork ();
  if (launch_pid[i])
    {
      sctk_debug ("Wait %d\n", launch_pid[i]);
      waitpid (launch_pid[i], &status, WUNTRACED | WCONTINUED);
      sctk_debug ("Wait done %d %d\n", launch_pid[i], status);
    }
  else
    {
      static char error[4096];
      execvp (remote_argv[0], remote_argv);
      sprintf (error, "Fail to execute %s", remote_argv[0]);
      perror (error);
    }
}
void
launch_multiple (int i)
{

  launch_pid[i] = fork ();
  if (launch_pid[i])
    {
      sctk_debug ("Wait %d\n", launch_pid[i]);
    }
  else
    {
      static char error[4096];
      execvp (remote_argv[0], remote_argv);
      sprintf (error, "Fail to execute %s", remote_argv[0]);
      perror (error);
    }
}


static void
sctk_handler (int signo, siginfo_t * info, void *context)
{
  int i;
  sctk_debug ("PROPAGATE SIGNAL %d\n", signo);
  for (i = 0; i < process_number; i++)
    {
      kill (launch_pid[i], signo);
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
sctk_tcp_connect_to (int portno, char *name)
{
  int clientsock_fd;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  int size;

  sctk_debug ("Try connection to %s on port %d", name, portno);

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

  sctk_debug ("Try connection to %s on port %d done", name, portno);
  return clientsock_fd;
}

static int sockfd;
void *thread_socket (void *arg);
void
create_new_thread ()
{
  pthread_t pid;
  pthread_create (&pid, NULL, thread_socket, NULL);
}

static void
sctk_create_recv_socket ()
{
  int portno;
  struct sockaddr_in serv_addr;
  int i;

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
  create_new_thread ();
}
