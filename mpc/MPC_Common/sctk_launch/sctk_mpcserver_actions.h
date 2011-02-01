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
#ifndef __SCTK__MPCRUN_ACTIONS_H_
#define __SCTK__MPCRUN_ACTIONS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#define MPC_SERVER_PORT "MPC_SERVER_PORT"
#define MPC_SERVER_HOST "MPC_SERVER_HOST"

#define MPC_ACTION_SIZE 64
#define MPC_SERVER_GET_RANK       "MPC_SERVER_GET_RANK"
#define MPC_SERVER_GET_LOCAL_RANK "MPC_SERVER_GET_LOCAL_RANK"
#define MPC_SERVER_REGISTER       "MPC_SERVER_REGISTER"
#define MPC_SERVER_GET            "MPC_SERVER_GET"
#define MPC_SERVER_BARRIER        "MPC_SERVER_BARRIER"
#define MPC_SERVER_REGISTER_HOST  "MPC_SERVER_REGISTER_HOST"
#define MPC_SERVER_GET_HOST       "MPC_SERVER_GET_HOST"
#define MPC_SERVER_GET_LOCAL_SIZE "MPC_SERVER_GET_LOCAL_SIZE"
#define MPC_SERVER_REGISTER_SHM_FILENAME  "MPC_SERVER_REGISTER_SHM_FILENAME"
#define MPC_SERVER_GET_SHM_FILENAME       "MPC_SERVER_GET_SHM_FILENAME"
#define MPC_SERVER_SET_PROCESS_NUMBER       "MPC_SERVER_SET_PROCESS_NUMBER"

#define SHM_FILENAME_SIZE 256
#define HOSTNAME_SIZE 4096
#define PORT_SIZE 4096
#define HOSTNAME_PORT_SIZE (HOSTNAME_SIZE+PORT_SIZE+10)*sizeof(char)
#define MAX_PROCESS_TCP_MODULE 10
#define MAX_TCP_SERVERS 10

typedef struct sctk_client_init_message_s
{
  unsigned int local_process_rank;
  unsigned int global_process_rank;
  unsigned int node_rank;
} sctk_client_init_message_t;

typedef struct sctk_client_hostname_message_s
{
  int process_number;
  char hostname[HOSTNAME_PORT_SIZE];
} sctk_client_hostname_message_t;

typedef struct sctk_client_local_size_and_node_number_message_s
{
  int local_process_number;
  int node_number;
} sctk_client_local_size_and_node_number_message_t;


#ifdef __cplusplus
}
#endif
#endif
