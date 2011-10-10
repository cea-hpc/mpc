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

#include "sctk_pmi.h"

#define MAX_HOST_SIZE 255

/* localhost and disthost of the TCP connection */
static char local_host[HOSTNAME_PORT_SIZE];
static char port[PORT_SIZE];

//ssize_t
//sctk_mpcserver_safe_read (int fd, void *buf, size_t count)
//{
//  char *tmp;
//  ssize_t already_read = 0;
//  ssize_t dcount = 0;
//
//  tmp = buf;
//  while (already_read < count)
//    {
//      tmp += dcount;
//      dcount = read (fd, tmp, count - already_read);
//      if (dcount < 0)
//	{
//	  perror ("safe_read");
//	  abort();
//	}
//      already_read += dcount;
//    }
//  assert (count == already_read);
//  return already_read;
//}
//
//ssize_t
//sctk_mpcserver_safe_write (int fd,  void *buf, size_t count)
//{
//  ssize_t dcount = 0;
//  ssize_t already_written = 0;
//  char *tmp = (char *)buf;
//
//
//  while( already_written < count )
//    {
//     tmp += dcount;
//     dcount = write (fd, buf, count - already_written );
//
//     if ( dcount < 0 )
//       {
//         perror ("safe_write");
//	 abort();
//       }
//
//	already_written += dcount;
//    }
//
//  assert (already_written == count);
//  return already_written;
//}




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
      exit (-1);
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

//int
//sctk_mpcrun_client (char *request, void *in, size_t size_in,
//		    void *out, size_t size_out)
//{
//  int fd;
//  char req[MPC_ACTION_SIZE];
//
//  memset (req, 0, MPC_ACTION_SIZE);
//  assume (strlen (request) + 1 <= MPC_ACTION_SIZE);
//  memcpy (req, request, strlen (request) + 1);
//
//  fd = sctk_tcp_connect_to (sctk_mpcrun_client_port, sctk_mpcrun_client_host);
//  sctk_nodebug ("Connected to host with req %s", req);
//  sctk_mpcserver_safe_write (fd, req, MPC_ACTION_SIZE);
//  sctk_mpcserver_safe_write (fd, &sctk_process_rank, sizeof (int));
//  if (out != NULL)
//    {
//      sctk_mpcserver_safe_write (fd, &size_out, sizeof (unsigned long));
//      sctk_mpcserver_safe_write (fd, out, size_out);
//    }
//  if (in != NULL)
//    {
//      unsigned long size;
//      sctk_mpcserver_safe_read (fd, &size, sizeof (unsigned long));
//      if (size != 0)
//	{
//	  assume (size == size_in);
//	  sctk_mpcserver_safe_read (fd, in, size_in);
//	}
//      else
//	{
//	  sctk_nodebug ("Request fail %s", request);
//	  close (fd);
//	  return 1;
//	}
//    }
//
//  close (fd);
//  return 0;
//}


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
	sctk_pmi_barrier();
}


static int* socket_graph;

void
sctk_mpcrun_send_to_process (void *buf, size_t count, int process){
  sctk_nodebug("Send write message to %d fd %d",process,socket_graph[process]);
  sctk_pmi_send(buf,count,socket_graph[process]);
  sctk_nodebug("Send write message to %d fd %d done",process,socket_graph[process]);
}

void
sctk_mpcrun_read_to_process (void *buf, size_t count, int process){
  sctk_nodebug("Send read message to %d fd %d",process,socket_graph[process]);
  sctk_pmi_recv(buf,count,socket_graph[process]);
  sctk_nodebug("Send read message to %d fd %d done",process,socket_graph[process]);
}


static int sockfd;

  void
sctk_mpcrun_client_create_recv_socket ()
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
sctk_mpcrun_client_get_global_consts()
{
//	sctk_client_hostname_message_t hostname_msg;
//	sctk_client_init_message_t msg;

	sctk_pmi_get_process_number(&sctk_process_number);

	// fill the msg
//	memcpy(hostname_msg.hostname, &local_host, HOSTNAME_SIZE);
//	hostname_msg.process_number = sctk_process_number;

	// FIXME Le assume MPC_SERVER_GET_RANK uniquement à mettre le tableau registered_node à jour
	// pour ce qui concerne le shmfilename... (MPC_Tools/mpcrun_tools/mpcrun_server.c)
	//	assume (sctk_mpcrun_client
	//				(MPC_SERVER_GET_RANK, &msg, sizeof (sctk_client_init_message_t), &hostname_msg,
	//						sizeof(sctk_client_hostname_message_t)) == 0);

	// Get the node rank, the global & local process ranks
	sctk_pmi_get_node_rank(&sctk_node_rank);
	sctk_pmi_get_process_rank(&sctk_process_rank);
	sctk_pmi_get_process_on_node_rank(&sctk_local_process_rank);

	sctk_nodebug("NODE RANK : %d", sctk_node_rank);
	sctk_nodebug("PROCESS RANK (global) : %d", sctk_process_rank);
	sctk_nodebug("PROCESS RANK (local) : %d", sctk_local_process_rank);
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_mpcrun_client_get_local_consts
 *  Description:  Get the number of local processes on a node and the number
 *  of nodes.
 * ==================================================================
 */
void
sctk_mpcrun_client_get_local_consts ()
{
	sctk_pmi_get_processes_on_node_number(&sctk_local_process_number);
	sctk_pmi_get_node_number(&sctk_node_number);
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
sctk_mpcrun_client_get_shmfilename (char* key, char* out, int key_len, int val_len)
{
  sctk_pmi_get_connection_info_str(out,val_len,key);
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
sctk_mpcrun_client_register_shmfilename (char* key, char* in, int key_len, int val_len)
{
  sctk_pmi_put_connection_info_str(in,val_len,key);
  sctk_nodebug("written shm filename : %s", in);

  return;
}

  void
sctk_mpcrun_client_init_host_port()
{
  char *tmp;

  /* fill localhost variable */
  memset(local_host,'\0', HOSTNAME_PORT_SIZE);
  gethostname(local_host, HOSTNAME_SIZE);
  sctk_nodebug("use port %s",port);
  sprintf(local_host+HOSTNAME_SIZE,"%s",port);

  tmp = getenv (MPC_SERVER_PORT);
  assume (tmp);
  if (tmp)
    sctk_mpcrun_client_port = atoi (tmp);
  sctk_nodebug ("PORT %s", tmp);
  tmp = getenv (MPC_SERVER_HOST);
  assume (tmp);
  sctk_nodebug ("HOSTNAME %s", tmp);
  sctk_mpcrun_client_host = tmp;
}

  void
sctk_mpcrun_client_init_connect ()
{
  int rank;
  int i;
  char msg[MAX_HOST_SIZE];
  char * tok;

  rank = sctk_process_rank;
  sctk_nodebug("RANK : %d", rank);

  /* initialize socket graph */
  socket_graph = malloc(sctk_process_number*sizeof(int));
  memset(socket_graph,-1,sctk_process_number*sizeof(int));

  // Initialize host_list if necessary
  if (host_list == NULL)
  {
	  int index;
	  host_list = (sctk_pmi_tcp_t *) malloc(sctk_process_number * sizeof(sctk_pmi_tcp_t));
	  for (index = 0; index < sctk_process_number; index ++)
	  {
		  strcpy(host_list[index].name, "");
		  host_list[index].port_nb = -1;
	  }
  }
  assume(strcmp(host_list[sctk_process_rank].name, "") == 0);

  // Store (hostname, port)
  assume(strlen(local_host) < SCTK_PMI_NAME_SIZE);
  strcpy(host_list[sctk_process_rank].name, local_host);
  host_list[sctk_process_rank].port_nb = atoi(local_host + HOSTNAME_SIZE);

  // Send my (hostname, port) for all others processes
  sprintf(msg, "%s:%d", host_list[sctk_process_rank].name, host_list[sctk_process_rank].port_nb);
  sctk_pmi_put_connection_info(msg, MAX_HOST_SIZE, SCTK_PMI_TAG_MPCRUNCLIENT);

  // Wait for all processes
  sctk_pmi_barrier();

  // Get (hostname, port) of all others processes
  for (i = 0; i < sctk_process_number; i ++)
  {
	  sctk_pmi_get_connection_info(msg, MAX_HOST_SIZE, SCTK_PMI_TAG_MPCRUNCLIENT, i);
	  tok = strtok(msg,":");
	  strcpy(host_list[i].name,tok);
	  tok = strtok(NULL,":");
	  host_list[i].port_nb = atoi(tok);
  }

  sctk_nodebug("SEND %s %s",local_host,local_host+HOSTNAME_SIZE);
  sctk_nodebug("sctk_process_number : %d", sctk_process_rank);
  for(i = 0; i < sctk_process_number; i++)
  {
    if(i != rank)
    {
      if(i < rank)
      {
    	sctk_nodebug("%d socket %s port %d", i, host_list[i].name, host_list[i].port_nb);
        socket_graph[i] = sctk_tcp_connect_to(host_list[i].port_nb, host_list[i].name);
        sctk_pmi_send(&rank, sizeof(int),socket_graph[i]);
        sctk_nodebug("CONNECT %d socket %d",i,socket_graph[i]);
      }
      else
      {
        int fd;
        int remote_rank;
        unsigned int clilen;
        struct sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);
        sctk_nodebug("Wait for connection");
        fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);

        sctk_pmi_recv(&remote_rank, sizeof(int),fd);
        socket_graph[remote_rank] = fd;
        sctk_nodebug("ACCEPT %d socket %d",remote_rank,socket_graph[remote_rank]);
      }
    }
  }
}

static void
generate_random_filename(char* filename, int size)
{
  int i;

  srand(time(0));

  for(i = 0; i < size; i++)
    filename[i] = (rand()%26)+'A';

  filename[i-1] = '\0';
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
  char random[7];

  generate_random_filename(random, 7);

  sprintf(__string, "SHM_%s.%s.%s", local_host, random, local_host+HOSTNAME_SIZE);
}

char* sctk_mpcrun_client_get_hostname()
{
  return local_host;
}
