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
#ifndef __SCTK_LOW_LEVEL_COMM_H_
#define __SCTK_LOW_LEVEL_COMM_H_

#include <sctk_inter_thread_comm.h>


struct sctk_rail_info_s;

/** \brief Init Network dependent configuration inside a rail
 *
 *   This function is called from sctk_rail.c as rails
 *   can be hierarchical, requiring the ability to call from there.
 *
 *  \param rail Rail structure to be initialized
 *  \param driver_type the type of driver to load 
 *         (follows enum sctk_runtime_config_struct_net_driver_type from the config)
 *          with the extra SCTK_RAIL_TOPOLOGICAL value (9999) to identify topo rails
 * 
 */
void sctk_rail_init_driver( struct sctk_rail_info_s * rail, int driver_type );

/** \brief Init MPC network configuration
 *
 *   This function also loads the default configuration from the command
 *   line including rails and network backends
 *
 *  \param name Name of the configuration from the command line (can be NULL)
 */
void sctk_net_init_driver ( char *name );
void sctk_net_init_task_level(int taskid, int vp);
void sctk_net_finalize_task_level(int taskid, int vp);
/** \brief Get a pointer to a given rail
*   \param name Name of the requested rail
*   \return The rail or NULL
*/
struct sctk_runtime_config_struct_net_rail *sctk_get_rail_config_by_name ( char *name );

/** \brief Get a pointer to a driver config
*   \param name Name of the requested driver config
*   \return The driver config or NULL
*/
struct sctk_runtime_config_struct_net_driver_config *sctk_get_driver_config_by_name ( char *name );


void sctk_net_init_pmi();
int sctk_is_net_migration_available();

void sctk_network_send_message ( sctk_thread_ptp_message_t *msg );
void sctk_network_send_message_set ( void ( *sctk_network_send_message_val ) ( sctk_thread_ptp_message_t * ) );

void sctk_network_notify_recv_message ( sctk_thread_ptp_message_t *msg );
void sctk_network_notify_recv_message_set ( void ( *sctk_network_notify_recv_message_val ) ( sctk_thread_ptp_message_t * ) );

void sctk_network_notify_matching_message ( sctk_thread_ptp_message_t *msg );
void sctk_network_notify_matching_message_set ( void ( *sctk_network_notify_matching_message_val ) ( sctk_thread_ptp_message_t * ) );

void sctk_network_notify_perform_message ( int remote_process, int remote_task_id, int polling_task_id, int blocking );
void sctk_network_notify_perform_message_set ( void ( *sctk_network_notify_perform_message_val ) ( int, int, int, int ) );

void sctk_network_notify_idle_message ();
void sctk_network_notify_idle_message_set ( void ( *sctk_network_notify_idle_message_val ) () );

void sctk_network_notify_any_source_message ( int polling_task_id, int blocking );
void sctk_network_notify_any_source_message_set ( void ( *sctk_network_notify_perform_message_val ) ( int polling_task_id, int blocking ) );

void sctk_network_notify_new_communicator ( int comm_idx, size_t comm_size);
void sctk_network_notify_new_communicator_set ( void ( *sctk_network_notify_new_comm_val ) ( int comm_idx, size_t comm_size ) );

void sctk_network_notify_probe_message_set (void ( *sctk_network_notify_probe_message_val) () );

size_t sctk_net_memory_allocation_hook ( size_t size_origin );
void sctk_net_memory_free_hook ( void * ptr , size_t size );

int sctk_net_is_mode_hybrid ();
int sctk_net_set_mode_hybrid ();

#endif
