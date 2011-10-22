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
#include <sctk_simple_tcp.h>
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
#include <sctk_route.h>
#include <sctk_net_tools.h>


#define MAX_STRING_SIZE 2048

static int sctk_use_tcp_o_ib = 0;
static int sockfd;
static int portno;

/************ SOCKET MANIPULATION ****************/
static int
sctk_tcp_connect_to (char *name_init)
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

  if(sctk_use_tcp_o_ib){
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
sctk_client_create_recv_socket ()
{
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

  listen (sockfd, 10);
}

/************ ROUTES ****************/
static void* sctk_simple_tcp_thread(sctk_route_table_t* tmp){
  int fd;
  fd = tmp->data.simple_tcp.fd;
  while(1){
    sctk_thread_ptp_message_t * msg;
    void* body;
    size_t size;

    sctk_safe_read(fd,&size,sizeof(size_t));

    size = size - sizeof(sctk_thread_ptp_message_body_t) + 
      sizeof(sctk_thread_ptp_message_t);
    msg = sctk_malloc(size);
    body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

    /* Recv header*/
    sctk_nodebug("Read %d",sizeof(sctk_thread_ptp_message_body_t));
    sctk_safe_read(fd,msg,sizeof(sctk_thread_ptp_message_body_t));

    msg->body.completion_flag = NULL;
    msg->tail.message_type = sctk_message_network;
    
    /* Recv body*/
    size = size - sizeof(sctk_thread_ptp_message_t);
    sctk_safe_read(fd,body,size);

    sctk_reinit_header(msg,sctk_free,sctk_net_message_copy);

    sctk_nodebug("MSG RECV|%s|", (char*)body);    

    sctk_send_message(msg);
  }
}

/* static void sctk_simple_tcp_write(size_t size, void* msg, int fd){ */
/*   size_t res; */
  
/*   sctk_nodebug("Use fd %d",fd); */

/*   res = write(fd,&size,sizeof(size_t)); */
/*   if(res != sizeof(size_t)){ */
/*     perror("Write error"); */
/*     sctk_abort(); */
/*   } */
/*   res = write(fd,msg,size); */
/*   if(res != size){ */
/*     perror("Write error"); */
/*     sctk_abort(); */
/*   } */
/* } */

static void sctk_simple_tcp_add_route(int dest, int fd){
  sctk_route_table_t* tmp;
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;

  tmp = sctk_malloc(sizeof(sctk_route_table_t));
  
  sctk_nodebug("Register fd %d",fd);

  tmp->data.simple_tcp.fd = fd;

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr,(void*(*)(void*))sctk_simple_tcp_thread , tmp);

  sctk_add_route(dest,tmp);
}

/************ INTER_THEAD_COMM HOOOKS ****************/

static void 
sctk_network_send_message_simple_tcp (sctk_thread_ptp_message_t * msg){
  sctk_route_table_t* tmp;
  size_t size;
  int fd;

  tmp = sctk_get_route(msg->body.header.glob_destination);

  sctk_spinlock_lock(&(tmp->data.simple_tcp.lock));

  fd = tmp->data.simple_tcp.fd;

  size = msg->body.header.msg_size + sizeof(sctk_thread_ptp_message_body_t);

  sctk_safe_write(fd,&size,sizeof(size_t));

  sctk_safe_write(fd,msg,sizeof(sctk_thread_ptp_message_body_t));

  sctk_net_write_in_fd(msg,fd);
  sctk_spinlock_unlock(&(tmp->data.simple_tcp.lock));

  sctk_complete_and_free_message(msg);
}

static void 
sctk_network_notify_recv_message_simple_tcp (sctk_thread_ptp_message_t * msg){
/*   sctk_route_table_t* tmp; */

/*   tmp = sctk_get_route(msg->body.header.glob_source); */
/*   not_implemented(); */
}

static void 
sctk_network_notify_matching_message_simple_tcp (sctk_thread_ptp_message_t * msg){
/*   sctk_route_table_t* tmp; */

/*   tmp = sctk_get_route(msg->body.header.glob_source); */
/*   not_implemented(); */
}

static void 
sctk_network_notify_perform_message_simple_tcp (int remote){
/*   sctk_route_table_t* tmp; */

/*   tmp = sctk_get_route(remote); */
/*   not_implemented(); */
}

static void 
sctk_network_notify_idle_message_simple_tcp (){
/*   not_implemented(); */
}

/************ INIT ****************/
void sctk_network_init_simple_tcp(char* name){
 char connection_infos[MAX_STRING_SIZE];
 char dest_connection_infos[MAX_STRING_SIZE];
 size_t connection_infos_size;
 int dest_rank;
 int dest_socket;
 int src_rank;
 int src_socket;

  sctk_client_create_recv_socket ();

  sctk_network_send_message_set(sctk_network_send_message_simple_tcp);
  sctk_network_notify_recv_message_set(sctk_network_notify_recv_message_simple_tcp);
  sctk_network_notify_matching_message_set(sctk_network_notify_matching_message_simple_tcp);
  sctk_network_notify_perform_message_set(sctk_network_notify_perform_message_simple_tcp);
  sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_simple_tcp);

  gethostname(connection_infos, MAX_STRING_SIZE-100);
  connection_infos_size = strlen(connection_infos);
  
  sprintf(connection_infos+connection_infos_size,":%d",portno);
  connection_infos_size = strlen(connection_infos) + 1;
  assume(connection_infos_size < MAX_STRING_SIZE);

  dest_rank = (sctk_process_rank + 1) % sctk_process_number;
  src_rank = (sctk_process_rank + sctk_process_number - 1) % sctk_process_number;

  sctk_nodebug("Connection Infos (%d): %s",sctk_process_rank,connection_infos);

  assume(sctk_pmi_put_connection_info(connection_infos,MAX_STRING_SIZE,0) == 0);
  sctk_pmi_barrier();

  assume(sctk_pmi_get_connection_info(dest_connection_infos,MAX_STRING_SIZE,0,dest_rank) == 0);

  sctk_nodebug("DEST Connection Infos(%d) to %d: %s",sctk_process_rank,dest_rank,dest_connection_infos);

  sctk_pmi_barrier();

  if(sctk_process_rank % 2 == 0){
    sctk_nodebug("Connect to %d",dest_rank);
    dest_socket = sctk_tcp_connect_to(dest_connection_infos);
    if(dest_socket < 0){
      perror("Connection error");
      sctk_abort();
    }
  } else {
    sctk_nodebug("Wait connection");
    src_socket = accept (sockfd, NULL,0);  
    if(src_socket < 0){
      perror("Connection error");
      sctk_abort();
    }
  }
  if(sctk_process_rank % 2 == 1){
    sctk_nodebug("Connect to %d",dest_rank);
    dest_socket = sctk_tcp_connect_to(dest_connection_infos);
    if(dest_socket < 0){
      perror("Connection error");
      sctk_abort();
    }
  } else {
    sctk_nodebug("Wait connection");
    src_socket = accept (sockfd, NULL,0);
    if(src_socket < 0){
      perror("Connection error");
      sctk_abort();
    } 
  }
  sctk_pmi_barrier(); 
  sctk_simple_tcp_add_route(dest_rank,dest_socket);
  sctk_simple_tcp_add_route(src_rank,src_socket);
  sctk_pmi_barrier();   

  sctk_network_mode = "TCP simple(ring)";

}
