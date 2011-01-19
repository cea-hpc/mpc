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
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <sys/time.h>
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk.h"
#include "sctk_mpcrun_client.h"
#include "sctk_asm.h"

/* localhost and disthost of the TCP connection */
static char local_host[HOSTNAME_PORT_SIZE];

ssize_t
sctk_mpcserver_safe_read (int fd, void *buf, size_t count)
{
  char *tmp;
  ssize_t allready_readed = 0;
  ssize_t dcount = 0;
  struct timeval tv;
  fd_set rfds;

  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  tmp = buf;
  while (allready_readed < (ssize_t)count)
    {
      tmp += dcount;

      dcount = read (fd, tmp, count - allready_readed);
      if (!(dcount >= 0))
	{
	  perror ("safe_read");
	}
      assume (dcount >= 0);
      allready_readed += dcount;
    }
  assert ((ssize_t)count == allready_readed);
  return allready_readed;
}

ssize_t
sctk_mpcserver_safe_write (int fd, const void *buf, size_t count)
{
  ssize_t dcount;
  sctk_nodebug("COUNT : %d WRITE", count);
  dcount = write (fd, buf, count);
  if (!(dcount == (ssize_t)count))
    {
      perror ("safe_write");
    }
  assume (dcount == (ssize_t)count);
  fsync (fd);
  return dcount;
}

int sctk_use_tcp_o_ib = 0;

int
sctk_tcp_connect_to (int portno, char *name_init)
{
  int clientsock_fd;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char* ip;
  char name[4096];

  sprintf(name,"%s",name_init);

  sctk_nodebug ("Try connection to %s on port %d type %d", name, portno,AF_INET);

  clientsock_fd = socket (AF_INET, SOCK_STREAM, 0);
  if (clientsock_fd < 0)
    sctk_error ("ERROR opening socket");

  if(sctk_use_tcp_o_ib){
    sprintf(name,"%s-ib0",name_init);
  } else {
    sprintf(name,"%s",name_init);
  }
  sctk_nodebug("Connect to %s",name);
  server = gethostbyname (name);
  if (server == NULL)
    {
      fprintf (stderr, "ERROR, no such host\n");
      exit (0);
    }
  ip = (char *) server->h_addr;

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  memmove ((char *) &serv_addr.sin_addr.s_addr,
	   ip, server->h_length);
  serv_addr.sin_port = htons ((unsigned short) portno);
  if (connect
      (clientsock_fd, (struct sockaddr *) (&serv_addr),
       sizeof (serv_addr)) < 0)
    {
      perror ("ERROR connecting");
      abort ();
    }
/*   sctk_print_getsockname(clientsock_fd); */

  sctk_nodebug ("Try connection to %s on port %d done", name, portno);
  return clientsock_fd;
}

int
sctk_mpcrun_client (char *request, void *in, size_t size_in,
		    void *out, size_t size_out)
{
  int fd;
  char req[MPC_ACTION_SIZE];

  memset (req, 0, MPC_ACTION_SIZE);
  assume (strlen (request) + 1 <= MPC_ACTION_SIZE);
  memcpy (req, request, strlen (request) + 1);

  fd = sctk_tcp_connect_to (sctk_mpcrun_client_port, sctk_mpcrun_client_host);
  sctk_nodebug ("Connected to host with req %s", req);
  sctk_mpcserver_safe_write (fd, req, MPC_ACTION_SIZE);
  sctk_mpcserver_safe_write (fd, &sctk_process_rank, sizeof (int));
  if (out != NULL)
    {
      sctk_mpcserver_safe_write (fd, &size_out, sizeof (unsigned long));
      sctk_mpcserver_safe_write (fd, out, size_out);
    }
  if (in != NULL)
    {
      unsigned long size;
      sctk_mpcserver_safe_read (fd, &size, sizeof (unsigned long));
      if (size != 0)
	{
	  assume (size == size_in);
	  sctk_mpcserver_safe_read (fd, in, size_in);
	}
      else
	{
	  sctk_nodebug ("Request fail %s", request);
	  close (fd);
	  return 1;
	}
    }

  close (fd);
  return 0;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_mpcrun_barrier
 *  Description:  Function which performs a TCP barrier on all processes
 *  of the communication.
 * ==================================================================
 */
void
sctk_mpcrun_barrier ()
{
  int total_nb;
  int done = 0;
  total_nb = sctk_process_number;
  sctk_nodebug("number of processes : %d", total_nb);

  do
    {
      assume (sctk_mpcrun_client
	      (MPC_SERVER_BARRIER, &done, sizeof (int), &total_nb,
	       sizeof (int)) == 0);

      /* /!\ cpu_relax added instead of sched yield.
       * Allow to perform a barrier when the thread library isn't run */
      sctk_cpu_relax();
      //    sched_yield ();
      sctk_nodebug ("done %d tot %d", done, total_nb);
    }
  while (done != total_nb);

}


static int* socket_graph;

void
sctk_mpcrun_send_to_process (void *buf, size_t count, int process){
  sctk_nodebug("Send write message to %d fd %d",process,socket_graph[process]);
  sctk_mpcserver_safe_write(socket_graph[process],buf,count);
  sctk_nodebug("Send write message to %d fd %d done",process,socket_graph[process]);
}

void
sctk_mpcrun_read_to_process (void *buf, size_t count, int process){
  sctk_nodebug("Send read message to %d fd %d",process,socket_graph[process]);
  sctk_mpcserver_safe_read(socket_graph[process],buf,count);
  sctk_nodebug("Send read message to %d fd %d done",process,socket_graph[process]);
}


static int sockfd;
static char port[PORT_SIZE];
static void
sctk_create_recv_socket ()
{
  int portno;
  struct sockaddr_in serv_addr;

  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    sctk_error ("ERROR opening socket");
  }

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));

  portno = 1023;
  sctk_nodebug ("Looking for an available port\n");
  do
    {
      portno++;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons ((unsigned short) portno);

    }
  while (bind (sockfd, (struct sockaddr *) &serv_addr,
	       sizeof (serv_addr)) < 0);
  sctk_nodebug ("Looking for an available port found %d\n", portno);
  sprintf (port, "%d", portno);

  listen (sockfd, 10);
}

void
sctk_mpcrun_client_init ()
{
  char *tmp;
  sctk_client_hostname_message_t hostname_msg;
  sctk_client_init_message_t msg;

  tmp = getenv (MPC_SERVER_PORT);
  assume (tmp);
  if (tmp)
    sctk_mpcrun_client_port = atoi (tmp);
  sctk_nodebug ("PORT %s", tmp);
  tmp = getenv (MPC_SERVER_HOST);
  assume (tmp);
  sctk_nodebug ("HOSTNAME %s", tmp);
  sctk_mpcrun_client_host = tmp;
  sctk_process_number = sctk_get_process_nb ();

  /* fill the msg */
  memcpy(hostname_msg.hostname, &local_host, HOSTNAME_PORT_SIZE);
  hostname_msg.process_number = sctk_process_number;

  assume (sctk_mpcrun_client
      (MPC_SERVER_GET_RANK, &msg, sizeof (sctk_client_init_message_t), &hostname_msg,
      sizeof(sctk_client_hostname_message_t)) == 0);

  sctk_process_rank = msg.global_process_rank;
  sctk_node_rank = msg.node_rank;
  sctk_nodebug("NODE RANK : %d", sctk_node_rank);
  sctk_local_process_rank = msg.local_process_rank;

  socket_graph = malloc(sctk_process_number*sizeof(int));
  memset(socket_graph,-1,sctk_process_number*sizeof(int));

}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_mpcrun_client_get_local_size_and_node_number
 *  Description:  Get the number of local processes on a node and the number
 *  of nodes.
 * ==================================================================
 */
void
sctk_mpcrun_client_get_local_size_and_node_number ()
{
  char hostname[HOSTNAME_SIZE];
  int fd;

  sctk_client_local_size_and_node_number_message_t msg;

  memset(hostname,'\0',(HOSTNAME_SIZE)*sizeof(char));
  gethostname(hostname, HOSTNAME_SIZE);
  fd = sctk_tcp_connect_to (sctk_mpcrun_client_port, sctk_mpcrun_client_host);

  sctk_mpcserver_safe_write (fd, MPC_SERVER_GET_LOCAL_SIZE, MPC_ACTION_SIZE);
  sctk_mpcserver_safe_write (fd, &sctk_process_rank, sizeof (int));
  sctk_mpcserver_safe_write (fd, hostname, HOSTNAME_PORT_SIZE);

  sctk_mpcserver_safe_read (fd, &msg, sizeof(sctk_client_local_size_and_node_number_message_t));
  sctk_local_process_number = msg.local_process_number;
  sctk_node_number = msg.node_number;
  sctk_nodebug("local size received : %d", local_size);
  sctk_nodebug("Number of nodes : %d", sctk_node_number);
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_mpcrun_client_get_shm_filename
 *  Description:  Get the SHM filename registered on the TCP server.
 *  This filename is local to each node.
 *
 *  out : pointer of the string where writing the filename
 * ==================================================================
 */
  void
sctk_mpcrun_client_get_shmfilename (char* out)
{
  int fd;

  fd = sctk_tcp_connect_to (sctk_mpcrun_client_port, sctk_mpcrun_client_host);

  sctk_mpcserver_safe_write (fd, MPC_SERVER_GET_SHM_FILENAME, MPC_ACTION_SIZE);
  sctk_mpcserver_safe_write (fd, &sctk_process_rank, sizeof (int));
  sctk_mpcserver_safe_write (fd, local_host, HOSTNAME_PORT_SIZE);

  /*  filename */
  sctk_mpcserver_safe_read (fd, out, SHM_FILENAME_SIZE);

  sctk_nodebug("got %s", out);

  return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_mpcrun_client_register_shmfilename
 *  Description:  Register a shmfilename on the TCP server.
 * ==================================================================
 */
  void
sctk_mpcrun_client_register_shmfilename (char* in)
{
  int fd;

  fd = sctk_tcp_connect_to (sctk_mpcrun_client_port, sctk_mpcrun_client_host);

  sctk_mpcserver_safe_write (fd, MPC_SERVER_REGISTER_SHM_FILENAME, MPC_ACTION_SIZE);
  sctk_mpcserver_safe_write (fd, &sctk_process_rank, sizeof (int));
  sctk_mpcserver_safe_write (fd, local_host, HOSTNAME_PORT_SIZE);
  sctk_mpcserver_safe_write (fd, in, SHM_FILENAME_SIZE);

  sctk_nodebug("written shm filename : %s", in);

  return;
}

  void
sctk_mpcrun_client_init_connect ()
{
  int rank;
  int i;
  char dist_host[HOSTNAME_SIZE+PORT_SIZE+10];

  sctk_create_recv_socket ();

  /* fill localhost variable */
  memset(local_host,'\0', HOSTNAME_PORT_SIZE);
  gethostname(local_host, HOSTNAME_SIZE);
  sctk_nodebug("use port %s",port);
  sprintf(local_host+HOSTNAME_SIZE,"%s",port);

  sctk_mpcrun_client_init ();
  rank = sctk_process_rank;
  sctk_nodebug("RANK : %d", rank);



  assume(sctk_mpcrun_client(MPC_SERVER_REGISTER_HOST,NULL,0,local_host,(HOSTNAME_SIZE+PORT_SIZE+10)*sizeof(char)) == 0);
  sctk_nodebug("SEND %s %s",local_host,local_host+HOSTNAME_SIZE);
  for(i = 0; i < sctk_process_number; i++){
    if(i != rank){
      if(i < rank){
        memset(dist_host,'\0',(HOSTNAME_SIZE+PORT_SIZE+10)*sizeof(char));
        sctk_nodebug("Wait Recv peer");
        while(sctk_mpcrun_client(MPC_SERVER_GET_HOST,dist_host,(HOSTNAME_SIZE+PORT_SIZE+10)*sizeof(char),&i,sizeof(int)) != 0){
          sleep(1);
        }
        sctk_nodebug("%d socket %s port %s",i,dist_host,dist_host+HOSTNAME_SIZE);
        socket_graph[i] = sctk_tcp_connect_to(atoi(dist_host+HOSTNAME_SIZE),dist_host);
        sctk_mpcserver_safe_write (socket_graph[i], &rank, sizeof(int));
        sctk_nodebug("CONNECT %d socket %d",i,socket_graph[i]);
      } else {
        int fd;
        int remote_rank;
        unsigned int clilen;
        struct sockaddr_in cli_addr;
        sctk_nodebug("Wait for connection");
        fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);

        sctk_mpcserver_safe_read (fd, &remote_rank, sizeof(int));
        socket_graph[remote_rank] = fd;
        sctk_nodebug("ACCEPT %d socket %d",remote_rank,socket_graph[remote_rank]);
      }
    }
  }
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_mpcrun_client_forgeshm_filename
 *  Description:  Forge a SHM filename.
 *
 *  string : pointer to the wtring where writing the filename
 * ==================================================================
 */
void sctk_mpcrun_client_forge_shm_filename(char* __string)
{
  char template[] = "XXXXXX";

  mkstemp(template);
  sctk_assert(template[0] != '\0');

  sprintf(__string, "SHM_%s.%s.%s", local_host, template, local_host+HOSTNAME_SIZE);
}
