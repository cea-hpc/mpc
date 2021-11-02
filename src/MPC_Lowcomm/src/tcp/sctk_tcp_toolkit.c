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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools                        # */
/* #                                                                      # */
/* ######################################################################## */


#include <mpc_common_debug.h>
#include <sctk_tcp.h>
#include <mpc_launch_pmi.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include "sctk_tcp_toolkit.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <mpc_common_spinlock.h>
#include <sctk_net_tools.h>
#include <errno.h>
#include <sctk_control_messages.h>
#include <mpc_lowcomm_monitor.h>
#include <mpc_common_rank.h>
#include <sctk_alloc.h>
#include <arpa/inet.h>
#include "sctk_rail.h"

#include "lowcomm_thread.h"


/********************************************************************/
/* Connection Setup                                                 */
/********************************************************************/

/**
 * Called when initializing the remote side of a connection.
 * This function emits a connect() call to the requested process.
 * It is generally called for creating the initial topology (above the
 * standard ring) and for the on-demand.
 * \param[in] name_init combination of host:port, the remote process to connect to
 * \param[in] rail the TCP rail
 * \return the newly created FD, -1 otherwise.
 */
static int __connect_to(char *name_init, sctk_rail_info_t *rail)
{
	int             clientsock_fd;
	struct hostent *server;

	char  name[MPC_COMMON_MAX_STRING_SIZE];
	char *portno = NULL;
	int   i;

	sprintf(name, "%s", name_init);

	/* Extract server name and port */

	for(i = 0; i < MPC_COMMON_MAX_STRING_SIZE; i++)
	{
		if(name[i] == ':')
		{
			name[i]      = '\0';
			name_init[i] = '\0';

			/* Make sure we hold the right port-no (from original buffer)
			 * as it might be overwriten if we fallback network */
			portno = name_init + (i + 1);
			mpc_common_nodebug("%s Port no %s", name, portno);
			break;
		}
	}

	char * preferred_network = "";
		preferred_network = "ib";
	/* Start Name Resolution */

	mpc_common_nodebug("Try connection to |%s| on port %s type %d", name, portno, AF_INET);
	struct addrinfo *results;


	/* First use getaddrinfo to extract connection type */
	int ret = mpc_common_getaddrinfo(name, portno, NULL, &results, preferred_network);

	if(ret != 0)
	{
		printf("Error resolving name : %s\n", gai_strerror(ret) );
		return -1;
	}

	struct addrinfo *current = results;

	int connected = 0;

	/* Walk getaddrinfo configurations */
	while(current)
	{
		errno = 0;
		/* First open a socket */
		clientsock_fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);

		if(clientsock_fd < 0)
		{
			perror("socket");
		}
		else
		{
			/* Modify the socket to force the flush of messages. Increase the
			 * performance for short messages. See http://www.ibm.com/developerworks/library/l-hisock/index.html */
			int one = 1;

			setsockopt(clientsock_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one) );
			
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
				mpc_common_debug_abort();
			}

			close(clientsock_fd);
		}

		/* Lets try next connection type */
		current = current->ai_next;
	}

	mpc_common_freeaddrinfo(results);

	/* Did we fail ? */
	if(connected == 0)
	{
		mpc_common_debug_error("Failed to connect to %s:%s\n", name, portno);
		mpc_common_debug_abort();
	}

	/* First step is to write our UID */
	mpc_lowcomm_peer_uid_t myuid = mpc_lowcomm_monitor_get_uid();

	if( mpc_common_io_safe_write(clientsock_fd, &myuid, sizeof(mpc_lowcomm_peer_uid_t)) < 0)
	{
		mpc_common_tracepoint("Failed to write uid");
	}

	return clientsock_fd;
}

/**
 * Create an socket ready to persistently listen on new incoming connection.
 * This function DOES NOT call a blocking accept().
 * \param[in] rail the TCP rail.
 */
static void __create_listening_socket(sctk_rail_info_t *rail)
{
	errno = 0;

	/* Use a portable way of setting a listening port */
	struct addrinfo hints, *results;

	/* Initialize addrinfo struct */
	memset(&hints, 0, sizeof(struct addrinfo) );
	hints.ai_family   = AF_UNSPEC;   /* Any network */
	hints.ai_socktype = SOCK_STREAM; /* Stream TCP socket */
	hints.ai_flags    = AI_PASSIVE;  /* We are going to bind this socket */

	/* We set port "0" in order to get a free port without linear scanning */
	int ret = getaddrinfo(NULL, "0", &hints, &results);

	if(ret != 0)
	{
		printf("Failed to create a bindable socket : %s\n", gai_strerror(ret) );
		mpc_common_debug_abort();
	}

	struct addrinfo *current = results;

	/* Init socket id to -1 */
	rail->network.tcp.sockfd = -1;

	/* Iterate over results picking the first working candidate */
	while(current)
	{
		errno = 0;
		/* First try to open a socket */
		rail->network.tcp.sockfd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);

		if(rail->network.tcp.sockfd < 0)
		{
			perror("socket");
		}
		else
		{
			/* Modify the socket to force the flush of messages. Increase the
			 * performance for short messages. See http://www.ibm.com/developerworks/library/l-hisock/index.html */
			int one = 1;

			setsockopt(rail->network.tcp.sockfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one) );

			errno = 0;

			/* Bind the socket to the port */
			if(bind(rail->network.tcp.sockfd, current->ai_addr, current->ai_addrlen) != -1)
			{
				/* Start listening on that port */
				if(listen(rail->network.tcp.sockfd, 512) != -1)
				{
					/* As we are bound to "0" we want to get the actual port */
					struct sockaddr_in socket_infos;
					socklen_t          infolen = sizeof(struct sockaddr);

					if(getsockname(rail->network.tcp.sockfd, ( struct sockaddr * )&socket_infos, &infolen) == -1)
					{
						perror("getsockname");
						mpc_common_debug_abort();
					}
					else
					{
						if(infolen == sizeof(struct sockaddr_in) )
						{
							rail->network.tcp.portno = ntohs(socket_infos.sin_port);
						}
						else
						{
							mpc_common_debug_error("Could not retrieve port number\n");
							close(rail->network.tcp.sockfd);
							mpc_common_debug_abort();
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
	if(rail->network.tcp.sockfd < 0)
	{
		mpc_common_debug_error("Failed to set a listening socket\n");
		mpc_common_debug_abort();
	}
}

/********************************************************************/
/* Route Initializer                                                */
/********************************************************************/

/**
 * Create a new TCP-specific route.
 * \param[in] dest the remote process id
 * \param[in] fd the associated FD
 * \param[in] rail the TCP rail
 * \param[in] tcp_thread_loop the polling routine
 * \param[in] route_type route type (DYNAMIC,STATIC)
 */
static void __add_route(mpc_lowcomm_peer_uid_t dest_uid, int fd, sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_type_t route_type)
{
	_mpc_lowcomm_endpoint_t *new_route;

	/* Allocate a new route */
	new_route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t) );
	assume(new_route != NULL);
	_mpc_lowcomm_endpoint_init(new_route, dest_uid, rail, route_type);

	/* Make sure the route is flagged connected */
	_mpc_lowcomm_endpoint_set_state(new_route, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	mpc_common_nodebug("Register fd %d", fd);
	new_route->data.tcp.fd = fd;

	mpc_common_spinlock_init(&new_route->data.tcp.lock, 0);

	/* Add the new route */
	if(route_type == _MPC_LOWCOMM_ENDPOINT_STATIC)
	{
		sctk_rail_add_static_route(rail, new_route);
	}
	else
	{
		sctk_rail_add_dynamic_route(rail, new_route);
	}

	/* set the route as connected */
	_mpc_lowcomm_endpoint_set_state(new_route, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	_mpc_lowcomm_kernel_thread_create(NULL, (void * (*)(void *) )rail->network.tcp.tcp_thread_loop, new_route);
}

/********************************************************************/
/* Route Hooks (Dynamic routes)                                     */
/********************************************************************/

static inline char * __gen_rail_target_name(sctk_rail_info_t *rail, char * buff, int bufflen)
{
	snprintf(buff, bufflen, "tcp-%d", rail->rail_number);
	return buff;
}




/**
 * Handler triggered when low_level_comm want to create dynamically a new route.
 * \param[in] rail the TCP rail
 * \param[in] dest_process the process to be connected with.
 */
void tcp_on_demand_connection_handler(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest_process)
{
	_mpc_lowcomm_endpoint_t *rout = sctk_rail_get_any_route_to_process (  rail, dest_process );

	//__sctk_network_connection_from_tcp(mpc_common_get_process_rank(), dest_process, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);
	char my_net_name[128];

	mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest_process,
																	   __gen_rail_target_name(rail, my_net_name, 128),
																	   rail->network.tcp.connection_infos,
																	   &ret);

	if(!resp)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu (timeout)", dest_process);
	}

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	if(content->on_demand.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu", dest_process);
	}

	mpc_lowcomm_monitor_response_free(resp);
}

static int __tcp_on_demand_callback(mpc_lowcomm_peer_uid_t from,
									char *data,
									char * return_data,
									int return_data_len,
									void *prail)
{
	sctk_rail_info_t * rail = prail;

	mpc_common_debug("TCP on-demand from (%u, %u)", mpc_lowcomm_peer_get_set(from), mpc_lowcomm_peer_get_rank(from) );

	/* Here we just need to connect to the target */
	int new_socket = __connect_to(data, rail);

	if(new_socket < 0)
	{
		snprintf(return_data, return_data_len, "Back-connection to %s failed", data);
		return 1;
	}

	__add_route( from, new_socket, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

	return 0;
}


/********************************************************************/
/* TCP Ring Init Function                                           */
/********************************************************************/

void * __accept_loop(void *prail)
{
	sctk_rail_info_t * rail = (sctk_rail_info_t*)prail;
	sctk_tcp_rail_info_t *tcp = (sctk_tcp_rail_info_t*)&rail->network.tcp;

	while(1)
	{
		int new_socket = accept(tcp->sockfd, NULL, 0);

		if(new_socket < 0)
		{
			break;
		}

		/* First thing is to read the corresponding UID */
		mpc_lowcomm_peer_uid_t remote_uid = 0;
		int ret = mpc_common_io_safe_read(new_socket, &remote_uid, sizeof(mpc_lowcomm_peer_uid_t));

		if( (ret < 0) || (remote_uid == 0) )
		{
			/* Something wrong */
			shutdown(new_socket, SHUT_RDWR);
			close(new_socket);
			continue;
		}

		__add_route(remote_uid, new_socket, rail,_MPC_LOWCOMM_ENDPOINT_DYNAMIC);
	}

	return NULL;
}



/**
 * End of TCP rail initialization and create the first topology: the ring.
 * \param[in,out] rail the TCP rail
 * \param[in] interface the interface to try first
 * \param[in] tcp_thread_loop the polling routine function.
 * \param[in] route_init the function registering a new TCP route
 */
void sctk_network_init_tcp_all(sctk_rail_info_t *rail, char *interface,
                               void * (*tcp_thread_loop)(_mpc_lowcomm_endpoint_t *) )
{
	char right_rank_connection_infos[MPC_COMMON_MAX_STRING_SIZE];
	int  right_rank;
	int  right_socket = -1;

	assume(rail->send_message_from_network != NULL);


	/* Register our connection handler */
	char my_net_name[128];
	mpc_lowcomm_monitor_register_on_demand_callback(__gen_rail_target_name(rail, my_net_name, 128),
                                                    __tcp_on_demand_callback,
													(void *)rail);

	/* Fill in common rail info */
	rail->connect_from            = NULL;
	rail->connect_to              = NULL;
	rail->control_message_handler = NULL;

	rail->network.tcp.interface = interface;
	rail->network.tcp.tcp_thread_loop        = tcp_thread_loop;

	/* Start listening TCP socket */
	__create_listening_socket(rail);

	/* Start accept loop */
	_mpc_lowcomm_kernel_thread_create( NULL, __accept_loop, (void*)rail);


    /* As the DNS does not take the load we resolve once
     * per server instead of once per client as we
     * do not want to break anything ! */

    char hostname[128];
    gethostname(hostname, 128);

    char resolved_ip[256];
  
    if( mpc_common_resolve_local_ip_for_iface(resolved_ip, 256, interface) < 0 )
    {
        /* Only use hostname and hope for the best */
        snprintf(resolved_ip, 256, "%s", hostname);
    }

    /* Make sure we do not publish loopback (who knows :-/) */
    if(!strcmp(resolved_ip, "127.0.0.1"))
    {
        snprintf(resolved_ip, 256, "%s", hostname);
    }

    snprintf(rail->network.tcp.connection_infos, MPC_COMMON_MAX_STRING_SIZE, "%s:%d", resolved_ip, rail->network.tcp.portno);
	rail->network.tcp.connection_infos_size = strlen(rail->network.tcp.connection_infos) + 1;

	assume(rail->network.tcp.connection_infos_size < MPC_COMMON_MAX_STRING_SIZE);

}
