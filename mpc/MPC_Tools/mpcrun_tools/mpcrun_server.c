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
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                    # */
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libgen.h>
#include <assert.h>
#include <sctk_mpcserver_actions.h>
#include <pthread.h>

int verbose = 0;
char *mpc_args;

int argc;
char **argv;

int pid;

static char hostname[HOSTNAME_SIZE];

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

typedef struct registered_node_s
{
  char  hostname[HOSTNAME_PORT_SIZE];
  char* shm_filename;     /* filename of the SHM module */
  int   local_rank;
} registered_node_t;
int node_number = 0;


static int* process_tcp_node_array = NULL;

/* mapping aray. */
registered_node_t* registered_node = NULL;

static char port[8192];
static int sockfd;

static ssize_t
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

static ssize_t
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

  listen (sockfd, 10);
}

static int process_nb;

static void
sctk_register_rank (int fd)
{
  static int rank = 0;
  unsigned long size;
  int i;
  sctk_client_hostname_message_t  hostname_msg;
  sctk_client_init_message_t msg;
  int node_rank = -1;
  int local_rank = -1;


  /* grep the hostname */
  safe_read (fd, &size, sizeof (unsigned long));
  safe_read (fd, &hostname_msg, size);

  process_nb = hostname_msg.process_number;

  if (process_tcp_node_array == NULL)
  {
    process_tcp_node_array = malloc(process_nb * sizeof(int));
  }

  /*  if the array isn't initialized */
  if (registered_node == NULL)
  {
    registered_node = malloc(process_nb * sizeof(struct registered_node_s));
    for (i = 0; i < process_nb ; ++i)
    {
      memset(registered_node[i].hostname, '\0', 4096);
      registered_node[i].local_rank = 0;
      registered_node[i].shm_filename = NULL;

    }
  }


  sctk_debug("Processues number : %d", process_nb);
  for (i=0; i < process_nb; ++i)
  {
    if ( strncmp(registered_node[i].hostname, hostname_msg.hostname, 4096) == 0)
    {
      sctk_debug("Local rank sent : %d",i);
      local_rank = registered_node[i].local_rank;
      (registered_node[i].local_rank) ++;
      node_rank = i;
      break;
    }
    else if (strlen(registered_node[i].hostname) == 0)
    {
      memcpy(registered_node[i].hostname, hostname_msg.hostname, HOSTNAME_PORT_SIZE);

      /* increment the node number */
      ++node_number;

      sctk_debug("Local rank sent : %d",i);
      local_rank = registered_node[i].local_rank;
      (registered_node[i].local_rank) ++;
      node_rank = i;
      break;
    }
  }
  if (node_rank == -1)
    sctk_error("Impossible to get the rank of the process. Server's crashing...");

  sctk_debug("Node number : %d", node_rank);

  msg.local_process_rank = (unsigned int) local_rank;
  msg.global_process_rank = (unsigned int) rank;
  msg.node_rank = (unsigned int) node_rank;

  size = sizeof(sctk_client_init_message_t);

  safe_write (fd, &size, sizeof (unsigned long));
  safe_write (fd, &msg, size);
  sctk_debug ("Rank %d/%d sended\n", rank, process_nb);
  rank++;
}

unsigned long registered_size;
void **registered = NULL;

  static void
sctk_register (int fd, int rank)
{
  unsigned long size;
  safe_read (fd, &size, sizeof (unsigned long));
  registered_size = size;
  if (registered == NULL)
  {
    int i;
    registered = malloc (process_nb * sizeof (void *));
    for (i = 0; i < process_nb; i++)
    {
      registered[i] = NULL;
    }
  }
  assert (registered[rank] == NULL);
  registered[rank] = malloc (size);
  safe_read (fd, registered[rank], size);
}

  static void
sctk_get (int fd, int rank)
{
  unsigned long size;
  int rank_req;
  safe_read (fd, &size, sizeof (unsigned long));
  safe_read (fd, &rank_req, size);
  sctk_debug ("%d req %d\n", rank, rank_req);
  if (registered == NULL)
  {
    size = 0;
    safe_write (fd, &size, sizeof (unsigned long));
  }
  else
  {
    if (registered[rank_req] == NULL)
    {
      size = 0;
      safe_write (fd, &size, sizeof (unsigned long));
    }
    else
    {
      size = registered_size;
      safe_write (fd, &size, sizeof (unsigned long));
      safe_write (fd, registered[rank_req], size);
    }
  }
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_match_hostname
 *  Description:  Match a hostname and return its index in the array
 * ==================================================================
 */
  static int
sctk_match_hostname(char* host)
{
  int i;

  assert(registered_node != NULL);

  for (i=0; i < process_nb; ++i)
  {
    if (strncmp(registered_node[i].hostname, host, 4096) == 0)
    {
      return i;
    }
    else if (strlen(registered_node[i].hostname) == 0)
    {
      strcpy(registered_node[i].hostname, host);
      return i;
    }
  }
  return -1;
}



  static void
sctk_get_local_size_and_node_number (int fd, int rank)
{
  char hostname[HOSTNAME_PORT_SIZE];
  int index;
  int local_size;
  unsigned long msg_size;
  sctk_client_local_size_and_node_number_message_t msg;

  safe_read (fd, hostname, HOSTNAME_PORT_SIZE);

  sctk_debug("hostname received : %s", hostname);

  index = sctk_match_hostname(hostname);
  local_size = registered_node[index].local_rank;

  sctk_debug("index : %d | local size : %d", index, local_size);

  /* local size of processes in the node and number of nodes */
  msg.local_process_number = local_size;
  msg.node_number = node_number;

  safe_write (fd, &msg, sizeof(sctk_client_local_size_and_node_number_message_t));

  /* send the array processes <-> node numer  */
  msg_size = process_nb * sizeof(int);
  safe_write (fd, &msg_size, sizeof(unsigned long));
}


size_t host_list_size;
char** host_list;
  static void
sctk_register_host (int fd, int rank)
{
  unsigned long size;
  safe_read (fd, &size, sizeof (unsigned long));
  sctk_debug("Msg size %lu",size);
  host_list_size = size;
  if (host_list == NULL)
  {
    int i;
    host_list = malloc (process_nb * sizeof (char *));
    for (i = 0; i < process_nb; i++)
    {
      host_list[i] = NULL;
    }
  }
  assert (host_list[rank] == NULL);
  host_list[rank] = malloc (size);
  safe_read (fd, host_list[rank], size);
  sctk_debug("Register %d |%s| port : %s",rank,host_list[rank], host_list[rank]+HOSTNAME_SIZE);
}

  static void
sctk_get_host (int fd, int rank)
{
  unsigned long size;
  int rank_req;
  safe_read (fd, &size, sizeof (unsigned long));
  safe_read (fd, &rank_req, size);
  sctk_debug ("%d GET HOST req %d\n", rank, rank_req);
  if (host_list[rank_req] == NULL)
  {
    size = 0;
    safe_write (fd, &size, sizeof (unsigned long));
  }
  else
  {
    size = host_list_size;
    safe_write (fd, &size, sizeof (unsigned long));
    safe_write (fd, host_list[rank_req], size);
  }
}


  static void
sctk_register_shm_filename (int fd, int rank)
{
  char shm_name[SHM_FILENAME_SIZE];
  char host[HOSTNAME_PORT_SIZE];
  int index;

  /* grep the hostname */
  safe_read (fd, &host, HOSTNAME_PORT_SIZE);
  index = sctk_match_hostname(host);

  safe_read (fd, &shm_name, SHM_FILENAME_SIZE);

  if (registered_node[index].shm_filename != NULL)
    free(registered_node[index].shm_filename);

  registered_node[index].shm_filename = malloc(SHM_FILENAME_SIZE);
  assert(registered_node[index].shm_filename != NULL);

  memcpy(registered_node[index].shm_filename, &shm_name, SHM_FILENAME_SIZE);

  sctk_debug("SHM filename registered : %s", shm_name);
}

  static void
sctk_get_shm_filename (int fd, int rank)
{
  char host[HOSTNAME_PORT_SIZE];
  int index;

  /* grep the hostname */
  safe_read (fd, &host, HOSTNAME_PORT_SIZE);

  sctk_debug("Host received : %s", host);

  index = sctk_match_hostname(host);
  assert(registered_node[index].shm_filename != NULL);


  safe_write(fd, registered_node[index].shm_filename, SHM_FILENAME_SIZE);

  sctk_debug("SHM filename sent : %s",registered_node[index].shm_filename);
}


/*
 * This function can now perfom more than one barrier !
 */
  static void
sctk_barrier (int fd, int rank)
{
  unsigned long size;
  int total;
  static int *done = NULL;
  static int *done_read = NULL;
  int i;
  int tot_done = 0;
  static volatile int lock = 0;

  safe_read (fd, &size, sizeof (unsigned long));
  safe_read (fd, &total, size);

  if (lock == 0)
  {
    if (done == NULL)
    {
      done = calloc (total, sizeof (int));
      done_read = calloc (total, sizeof (int));
    }
    if (done[rank] == 0)
    {
      done[rank] = 1;
    }
    for (i = 0; i < total; i++)
    {
      if (done[i] != 0)
      {
        tot_done++;
      }
    }

    if (total == tot_done)
    {
      lock = 1;
    }
  }


  if (lock == 1)
  {
    tot_done = 0;

    if (done_read[rank] == 0)
    {
      done_read[rank] = 1;
      for (i = 0; i < total; i++)
      {
        if (done_read[i] != 0)
          tot_done++;
      }


      if (total == tot_done)
      {
        done = NULL;
        done_read = NULL;
        lock = 0;
      }

      tot_done = total;
    }
  }

size = sizeof (int);
safe_write (fd, &size, sizeof (unsigned long));
safe_write (fd, &tot_done, size);
}


  static void *
server (void *arg)
{
  int fd;
  unsigned int clilen;
  struct sockaddr_in cli_addr;
  char req[MPC_ACTION_SIZE];
  sctk_debug ("Launch server\n");
  clilen = sizeof (cli_addr);

  assert (arg == NULL);

  while (1)
  {
    int rank;
    fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (fd < 0)
    {
      perror ("accept");
    }
    assert (fd >= 0);

    //    sctk_debug ("Connection done \n");
    safe_read (fd, req, MPC_ACTION_SIZE);
    safe_read (fd, &rank, sizeof (int));

//    sctk_debug ("Action asked %s\n", req);
    if (strcmp (req, MPC_SERVER_GET_RANK) == 0)
    {
      sctk_register_rank (fd);
    }
    else if (strcmp (req, MPC_SERVER_REGISTER_HOST) == 0)
    {
      sctk_register_host (fd, rank);
    }
    else if (strcmp (req, MPC_SERVER_GET_HOST) == 0)
    {
      sctk_get_host (fd, rank);
    }
    else if (strcmp (req, MPC_SERVER_REGISTER_SHM_FILENAME) == 0)
    {
      sctk_register_shm_filename (fd, rank);
    }
    else if (strcmp (req, MPC_SERVER_GET_SHM_FILENAME) == 0)
    {
      sctk_get_shm_filename (fd, rank);
    }
    else if (strcmp (req, MPC_SERVER_REGISTER) == 0)
    {
      sctk_register (fd, rank);
    }
    else if (strcmp (req, MPC_SERVER_GET) == 0)
    {
      sctk_get (fd, rank);
    }
    else if(strcmp (req, MPC_SERVER_GET_LOCAL_SIZE) == 0)
    {
      sctk_get_local_size_and_node_number(fd, rank);
    }
    else if (strcmp (req, MPC_SERVER_BARRIER) == 0)
    {
      sctk_barrier (fd, rank);
    }
    else
    {
      sctk_error ("Unknown command %s", req);
    }
    close (fd);
  }
}

  static int
launch ()
{
  int status;
  pid = fork ();
  if (pid == -1)
    {
      perror ("fork");
      status = -1;
    }
  else if (pid)
  {
//    sctk_debug ("Wait %d\n", pid);
    waitpid (pid, &status, WUNTRACED | WCONTINUED);
//    sctk_debug ("Wait done %d %d\n", pid, status);
    if (WIFEXITED (status))
    {
      status = WEXITSTATUS (status);
    }
    if (WIFSIGNALED (status))
    {
      status = WTERMSIG (status);
    }
//    sctk_debug ("Wait done %d %d\n", pid, status);
  }
  else
  {
      if (argc <= 1)
        {
          /* don't execute anything, report success */
          status = 0;
        }
      else
        {
          static char error[4096];
          execvp (argv[1], &(argv[1]));
          sprintf (error, "Fail to execute %s", argv[1]);
          perror (error);
        }
  }
  return status;
}

  static void
sctk_handler (int signo, siginfo_t * info, void *context)
{
  sctk_debug ("PROPAGATE SIGNAL %d\n", signo);
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
  int i;
  pthread_t pidt;
#ifndef SCTK_HAVE_SETENV
  static char MPC_SERVER_BUFFER_PORT[4096];
  static char MPC_SERVER_BUFFER_HOST[4096];
#endif

  argc = argc_l;
  argv = argv_l;

  set_handler ();

  gethostname (hostname, HOSTNAME_SIZE);

  for (i = 1; i < argc; i++)
  {
    sctk_debug ("ARG %d = %s\n", i, argv[i]);
  }
  mpc_args = getenv ("MPC_STARTUP_ARGS");
  sctk_debug ("MPC ARGS = %s\n", mpc_args);

  sctk_create_recv_socket ();
#ifdef SCTK_HAVE_SETENV
  setenv ("MPC_SERVER_PORT", port, 1);
  setenv ("MPC_SERVER_HOST", hostname, 1);
  sctk_debug ("SET %s %s\n", hostname, port);
#else
  sprintf (MPC_SERVER_BUFFER_PORT, "%s=%s", "MPC_SERVER_PORT", port);
  putenv (MPC_SERVER_BUFFER_PORT);
  sprintf (MPC_SERVER_BUFFER_HOST, "%s=%s", "MPC_SERVER_HOST", hostname);
  putenv (MPC_SERVER_BUFFER_HOST);
#endif

  pthread_create (&pidt, NULL, server, NULL);

  return launch ();
}
