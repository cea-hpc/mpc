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


#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_tcp.h>
#include <sctk_pmi.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sctk.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sctk_spinlock.h>
#include <sctk_net_tools.h>



/************ SOCKET MANIPULATION ****************/
static int
sctk_tcp_connect_to (char *name_init,sctk_rail_info_t* rail)
{
  int clientsock_fd;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char* ip;
  char name[MAX_STRING_SIZE];
  int portno;
  int i;
  sprintf(name,"%s",name_init);

  for(i = 0; i < MAX_STRING_SIZE; i++){
    if(name[i] == ':'){
      name[i] = '\0';
      name_init[i] = '\0';
      sctk_nodebug("Port no %s",name + (i+1));
      portno = atoi(name + (i+1));
      break;
    }
  }

  sctk_nodebug ("Try connection to |%s| on port %d type %d", name, portno,AF_INET);

  clientsock_fd = socket (AF_INET, SOCK_STREAM, 0);
  if (clientsock_fd < 0)
    sctk_error ("ERROR opening socket");

  if(rail->network.tcp.sctk_use_tcp_o_ib){
    sprintf(name,"%s-ib0",name_init);
  } else {
    sprintf(name,"%s",name_init);
  }
  sctk_nodebug("Connect to %s",name);

  server = gethostbyname (name);
  if (server == NULL)
    {
      fprintf (stderr, "ERROR, no such host %s\n",name);
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

static void
sctk_client_create_recv_socket (sctk_rail_info_t* rail)
{
  struct sockaddr_in serv_addr;

  rail->network.tcp.sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (rail->network.tcp.sockfd < 0){
    sctk_error ("ERROR opening socket");
  }

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));

  rail->network.tcp.portno = 1023;
  sctk_nodebug ("Looking for an available port\n");
  do
    {
      rail->network.tcp.portno++;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons ((unsigned short) rail->network.tcp.portno);

    }
  while (bind (rail->network.tcp.sockfd, (struct sockaddr *) &serv_addr,
	       sizeof (serv_addr)) < 0);
  sctk_nodebug ("Looking for an available port found %d\n", portno);

  listen (rail->network.tcp.sockfd, 10);
}

/************ ROUTES ****************/
static void sctk_tcp_add_static_route(int dest, int fd,sctk_rail_info_t* rail,
				      void* (*tcp_thread)(sctk_route_table_t*)){
  sctk_route_table_t* tmp;
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;

  tmp = sctk_malloc(sizeof(sctk_route_table_t));
  memset(tmp,0,sizeof(sctk_route_table_t));

  sctk_nodebug("Register fd %d",fd);

  tmp->data.tcp.fd = fd;
  sctk_nodebug("register route to %d on rail %d",dest,rail->rail_number);
  sctk_add_static_route(dest,tmp,rail);

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr,(void*(*)(void*))tcp_thread , tmp);
}

/************ INTER_THEAD_COMM HOOOKS ****************/
static void
sctk_network_connection_to_tcp(int from, int to,sctk_rail_info_t* rail){
  int dest_socket;
  char dest_connection_infos[MAX_STRING_SIZE];

  /*Recv connection informations*/
  sctk_route_messages_recv(from,to,process_specific_message_tag, 0,dest_connection_infos,MAX_STRING_SIZE);

  /*Recv id from the connected process*/
  dest_socket = sctk_tcp_connect_to(dest_connection_infos,rail);

  sctk_tcp_add_static_route(from,dest_socket,rail,(void * (*)(sctk_route_table_t *))(rail->network.tcp.tcp_thread));
}

static void
sctk_network_connection_from_tcp(int from, int to,sctk_rail_info_t* rail){
  int src_socket;
  /*Send connection informations*/
  sctk_route_messages_send(from,to,process_specific_message_tag, 0,rail->network.tcp.connection_infos,MAX_STRING_SIZE);

  src_socket = accept (rail->network.tcp.sockfd, NULL,0);
  if(src_socket < 0){
    perror("Connection error");
    sctk_abort();
  }

  sctk_tcp_add_static_route(to,src_socket,rail,(void * (*)(sctk_route_table_t *))(rail->network.tcp.tcp_thread));
}

/************ INIT ****************/
void sctk_network_init_tcp_all(sctk_rail_info_t* rail,int sctk_use_tcp_o_ib,
			       void* (*tcp_thread)(sctk_route_table_t*),
			       int (*route)(int , sctk_rail_info_t* ),
			       void(*route_init)(sctk_rail_info_t*)){
  char dest_connection_infos[MAX_STRING_SIZE];
  int dest_rank;
  int dest_socket;
  int src_rank;
  int src_socket;

  assume(rail->send_message_from_network != NULL);

  rail->route = route;
  rail->connect_to = sctk_network_connection_to_tcp;
  rail->connect_from = sctk_network_connection_from_tcp;
  rail->network.tcp.sctk_use_tcp_o_ib = sctk_use_tcp_o_ib;
  rail->network.tcp.tcp_thread = tcp_thread;
  sctk_client_create_recv_socket (rail);

  gethostname(rail->network.tcp.connection_infos, MAX_STRING_SIZE-100);
  rail->network.tcp.connection_infos_size = strlen(rail->network.tcp.connection_infos);

  sprintf(rail->network.tcp.connection_infos+rail->network.tcp.connection_infos_size,":%d",rail->network.tcp.portno);
  rail->network.tcp.connection_infos_size = strlen(rail->network.tcp.connection_infos) + 1;
  assume(rail->network.tcp.connection_infos_size < MAX_STRING_SIZE);

  dest_rank = (sctk_process_rank + 1) % sctk_process_number;
  src_rank = (sctk_process_rank + sctk_process_number - 1) % sctk_process_number;

  sctk_debug("Connection Infos (%d): %s",sctk_process_rank,rail->network.tcp.connection_infos);

  assume(sctk_pmi_put_connection_info(rail->network.tcp.connection_infos,MAX_STRING_SIZE,rail->rail_number) == 0);
  sctk_pmi_barrier();

  assume(sctk_pmi_get_connection_info(dest_connection_infos,MAX_STRING_SIZE,rail->rail_number,dest_rank) == 0);

  sctk_debug("DEST Connection Infos(%d) to %d: %s",sctk_process_rank,dest_rank,dest_connection_infos);

  sctk_pmi_barrier();

  if(sctk_process_number > 2){
    if(sctk_process_rank % 2 == 0){
      sctk_nodebug("Connect to %d",dest_rank);
      dest_socket = sctk_tcp_connect_to(dest_connection_infos,rail);
      if(dest_socket < 0){
	perror("Connection error");
	sctk_abort();
      }
    } else {
      sctk_nodebug("Wait connection");
      src_socket = accept (rail->network.tcp.sockfd, NULL,0);
      if(src_socket < 0){
	perror("Connection error");
	sctk_abort();
      }
    }
    if(sctk_process_rank % 2 == 1){
      sctk_nodebug("Connect to %d",dest_rank);
      dest_socket = sctk_tcp_connect_to(dest_connection_infos,rail);
      if(dest_socket < 0){
	perror("Connection error");
	sctk_abort();
      }
    } else {
      sctk_nodebug("Wait connection");
      src_socket = accept (rail->network.tcp.sockfd, NULL,0);
      if(src_socket < 0){
	perror("Connection error");
	sctk_abort();
      }
    }
  } else {
    if(sctk_process_rank % 2 == 0){
      sctk_nodebug("Connect to %d",dest_rank);
      dest_socket = sctk_tcp_connect_to(dest_connection_infos,rail);
      if(dest_socket < 0){
	perror("Connection error");
	sctk_abort();
      }
    } else {
      sctk_nodebug("Wait connection");
      dest_socket = accept (rail->network.tcp.sockfd, NULL,0);
      if(dest_socket < 0){
	perror("Connection error");
	sctk_abort();
      }
    }
  }
  sctk_pmi_barrier();
  sctk_tcp_add_static_route(dest_rank,dest_socket,rail,tcp_thread);
  if(sctk_process_number > 2){
    sctk_tcp_add_static_route(src_rank,src_socket,rail,tcp_thread);
  }
  sctk_pmi_barrier();
}
