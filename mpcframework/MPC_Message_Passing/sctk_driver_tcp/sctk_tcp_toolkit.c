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
#include <netinet/tcp.h>
#include <netdb.h>
#include <sctk_spinlock.h>
#include <sctk_net_tools.h>
#include <errno.h>

/************ SOCKET MANIPULATION ****************/

static int sctk_tcp_connect_to (char *name_init, sctk_rail_info_t* rail)
{
	int clientsock_fd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char* ip;
	char name[MAX_STRING_SIZE];
	char * portno = NULL;
	int i;

	sprintf(name,"%s",name_init);

	/* Extract server name and port */
	
	for(i = 0; i < MAX_STRING_SIZE; i++)
	{
		if(name[i] == ':')
		{
			name[i] = '\0';
			name_init[i] = '\0';
			sctk_nodebug("Port no %s",name + (i+1));
			portno = name + (i+1);
			
			break;
		}
	}

	/* Rely on IP over IB if possible */
	
	if(rail->network.tcp.sctk_use_tcp_o_ib)
	{
		sprintf(name,"%s-ib0",name_init);
	}
	else
	{
		/* Try the hostname */
		sprintf(name,"%s",name_init);

		/* Make suer it resolves */
		server = gethostbyname(name);
		
		if (server == NULL)
		{
			/* If host fails to resolve fallback to IP over IB */
			sprintf(name,"%s-ib0",name_init);
		}
	}
	
	/* Start Name Resolution */
	
	sctk_nodebug ("Try connection to |%s| on port %s type %d", name, portno,AF_INET);
	struct addrinfo *results;

	/* First use getaddrinfo to extract connection type */
	int ret = getaddrinfo( name, portno, NULL, &results);

	if( ret != 0 )
	{
		printf("Error resolving name : %s\n", gai_strerror(ret) );
		return -1;
	}

	struct addrinfo *current = results;

	int connected = 0;

	/* Walk getaddrinfo configurations */
	while( current )
	{
		errno = 0;
		/* First open a socket */
		clientsock_fd = socket( current->ai_family, current->ai_socktype, current->ai_protocol );

		if( clientsock_fd < 0 )
		{
			perror( "socket" );
		}
		else
		{
			/* Modify the socket to force the flush of messages. Increase the
			* performance for short messages. See http://www.ibm.com/developerworks/library/l-hisock/index.html */
			int one = 1;
			if (setsockopt(clientsock_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1 )
			{
				sctk_error("Cannot modify the socket options!");
			}
			
			/* Try to connect */
			errno = 0;
			if(connect(clientsock_fd, current->ai_addr, current->ai_addrlen) != -1)
			{
				connected = 1;
				/* We are connected all ok */
				break;
			}
			else
			{
				perror("connect");
			}

			close( clientsock_fd );
		}

		/* Lets try next connection type */
		current = current->ai_next;
	}

	freeaddrinfo(results);

	/* Did we fail ? */
	if( connected == 0 )
	{
		sctk_error("Failed to connect to %s:%s\n", name, portno);
		sctk_abort();
	}
	
	return clientsock_fd;
}

static void sctk_client_create_recv_socket (sctk_rail_info_t* rail)
{
	errno = 0;

	/* Use a portable way of setting a listening port */
	struct addrinfo hints, *results;

	/* Initialize addrinfo struct */
	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC; /* Any network */
	hints.ai_socktype = SOCK_STREAM; /* Stream TCP socket */
	hints.ai_flags = AI_PASSIVE; /* We are going to bind this socket */

	/* We set port "0" in order to get a free port without linear scanning */
	int ret = getaddrinfo( NULL, "0", &hints, &results);

	if( ret != 0 )
	{
		printf("Failed to create a bindable socket : %s\n", gai_strerror(ret) );
		sctk_abort();
	}

	struct addrinfo *current = results;

	/* Init socket id to -1 */
	rail->network.tcp.sockfd = -1;
	
	/* Iterate over results picking the first working candidate */
	while( current )
	{
		errno = 0;
		/* First try to open a socket */
		rail->network.tcp.sockfd = socket( current->ai_family, current->ai_socktype, current->ai_protocol );

		if( rail->network.tcp.sockfd < 0 )
		{
			perror( "socket" );
		}
		else
		{
			/* Modify the socket to force the flush of messages. Increase the
			* performance for short messages. See http://www.ibm.com/developerworks/library/l-hisock/index.html */
			int one = 1;
			if (setsockopt(rail->network.tcp.sockfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1 )
			{
				sctk_error("Cannot modify the socket options!");
			}

			errno = 0;
			
			/* Bind the socket to the port */
			if (bind(rail->network.tcp.sockfd, current->ai_addr, current->ai_addrlen) != -1)
			{
					/* Start listening on that port */
					if (listen(rail->network.tcp.sockfd,  512) != -1)
					{
							/* As we are bound to "0" we want to get the actual port */
							struct sockaddr_in socket_infos;
							socklen_t infolen = sizeof(struct sockaddr);
							
							if(getsockname(rail->network.tcp.sockfd, (struct sockaddr*)&socket_infos, &infolen) == -1 )
							{
								perror( "getsockname" );
								sctk_abort();
							}
							else
							{
								if( infolen == sizeof(struct sockaddr_in) )
								{
									rail->network.tcp.portno = ntohs(socket_infos.sin_port);
								}
								else
								{
									sctk_error("Could not retrieve port number\n");
									close(rail->network.tcp.sockfd);
									sctk_abort();
								}
							}   

							break;
			
					}
					else
					{
						perror("listen");
						close(rail->network.tcp.sockfd);
						
					}
			}
			else
			{
				perror("bind");
				close(rail->network.tcp.sockfd);
			}
		}
		
		current = current->ai_next;
	}

	freeaddrinfo(results);

	/* Did we fail ? */
	if( rail->network.tcp.sockfd < 0 )
	{
		sctk_error("Failed to set a listening socket\n");
		sctk_abort();
	}
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

  /* Modify the socket to force the flush of messages. Increase the
   * performance for short messages. See http://www.ibm.com/developerworks/library/l-hisock/index.html */
  int one = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1 ){
    sctk_error("Cannot modify the socket options!");
    sctk_abort();
  }

  tmp->data.tcp.fd = fd;
  sctk_nodebug("register route to %d on rail %d",dest,rail->rail_number);
  sctk_add_static_route(dest,tmp,rail);
  /* set the route as connected */
  sctk_route_set_state(tmp, state_connected);

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
  int dest_socket = -1;
  int src_rank;
  int src_socket = -1;

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
