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
#ifndef __SCTK__MPCRUN_CLIENT_H_
#define __SCTK__MPCRUN_CLIENT_H_
#include <stdlib.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "sctk_mpcserver_actions.h"
#ifdef __cplusplus
extern "C"
{
#endif
  extern int sctk_mpcrun_client_port;
  extern char *sctk_mpcrun_client_host;
  extern int sctk_use_tcp_o_ib;

  ssize_t sctk_mpcserver_safe_read (int fd, void *buf, size_t count);
  ssize_t sctk_mpcserver_safe_write (int fd, void *buf, size_t count);
  int sctk_tcp_connect_to (int portno, char *name);
  int sctk_mpcrun_client (char *request, void *in, size_t size_in,
			  void *out, size_t size_out);
  void
  sctk_mpcrun_client_init_connect ();

  void sctk_mpcrun_barrier (void);
  void
  sctk_mpcrun_send_to_process (void *buf, size_t count, int process);
  void
  sctk_mpcrun_read_to_process (void *buf, size_t count, int process);

  void sctk_mpcrun_client_get_shmfilename (char* key, char* out);
  void sctk_mpcrun_client_register_shmfilename (char* key, char* in);

  /* return the hostname and the port of the TCP client */
  char* sctk_mpcrun_client_get_hostname();
  void sctk_mpcrun_client_get_local_size_and_node_number();

  /* forge a string with the SHM filename */
  void sctk_mpcrun_client_forge_shm_filename(char* __string);

  void sctk_mpcrun_client_init_host_port();

  void sctk_mpcrun_client_get_global_consts();
  void sctk_mpcrun_client_get_local_consts ();

  void sctk_mpcrun_client_create_recv_socket ();

  void sctk_mpcrun_client_set_process_number();

#ifdef __cplusplus
}
#endif
#endif
