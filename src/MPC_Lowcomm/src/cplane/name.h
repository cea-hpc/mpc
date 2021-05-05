#ifndef _MPC_LOWCOMM_NAME_H_
#define _MPC_LOWCOMM_NAME_H_

#include <mpc_lowcomm_monitor.h>

/********************
 * INIT AND RELEASE *
 ********************/

void _mpc_lowcomm_monitor_name_init(void);
void _mpc_lowcomm_monitor_name_release(void);

/***********
 * PUBLISH *
 ***********/

int _mpc_lowcomm_monitor_name_publish(char * name, char * port_name, mpc_lowcomm_peer_uid_t hosting_peer);
int _mpc_lowcomm_monitor_name_unpublish(char * name );

/***********
 * RESOLVE *
 ***********/

char * _mpc_lowcomm_monitor_name_get_port(char * name, mpc_lowcomm_peer_uid_t *hosting_peer);

/********
 * LIST *
 ********/

char * _mpc_lowcomm_monitor_name_get_csv_list(void);
void _mpc_lowcomm_monitor_name_free_cvs_list(char *list);

#endif /* _MPC_LOWCOMM_NAME_H_ */