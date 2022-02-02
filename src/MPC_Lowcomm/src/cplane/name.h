/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.fr                       # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef _MPC_LOWCOMM_NAME_H_
#define _MPC_LOWCOMM_NAME_H_

#include <mpc_lowcomm_monitor.h>

/********************
 * INIT AND RELEASE *
 ********************/

/**
 * @brief Initialize the name resolution storage
 *
 */
void _mpc_lowcomm_monitor_name_init(void);

/**
 * @brief Release the name resolution storage
 *
 */
void _mpc_lowcomm_monitor_name_release(void);

/***********
 * PUBLISH *
 ***********/

/**
 * @brief Store name resoltion information
 *
 * @param name the service name
 * @param port_name the port information
 * @param hosting_peer who is providing these information
 * @return int 0 if all OK
 */
int _mpc_lowcomm_monitor_name_publish(char * name,
                                      char * port_name,
                                      mpc_lowcomm_peer_uid_t hosting_peer);

/**
 * @brief Remove name resoltion information
 *
 * @param name the service name
 * @return int 0 if all OK
 */
int _mpc_lowcomm_monitor_name_unpublish(char * name );

/***********
 * RESOLVE *
 ***********/

/**
 * @brief Extract information from a stored service name
 *
 * @param name the service name to retrieve
 * @param hosting_peer (out) the hosting peer
 * @return char* NULL if not found, port information otherwise
 */
char * _mpc_lowcomm_monitor_name_get_port(char * name, mpc_lowcomm_peer_uid_t *hosting_peer);

/********
 * LIST *
 ********/

/**
 * @brief Generate a comma separated list with all locally known services
 *
 * @return char* the service list ("" if none) to be freed with @ref _mpc_lowcomm_monitor_name_free_cvs_list
 */
char * _mpc_lowcomm_monitor_name_get_csv_list(void);

/**
 * @brief Free a service list as given by @ref _mpc_lowcomm_monitor_name_get_csv_list
 *
 * @param list the list to be freed
 */
void _mpc_lowcomm_monitor_name_free_cvs_list(char *list);

#endif /* _MPC_LOWCOMM_NAME_H_ */