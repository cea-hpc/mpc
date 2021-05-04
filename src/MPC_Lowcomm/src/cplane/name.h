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

int _mpc_lowcomm_monitor_name_publish(char * name, mpc_lowcomm_peer_uid_t peer );
int _mpc_lowcomm_monitor_name_unpublish(char * name );

/***********
 * RESOLVE *
 ***********/

mpc_lowcomm_peer_uid_t _mpc_lowcomm_monitor_name_get_peer(char * name, int * found);

/********
 * LIST *
 ********/

char * _mpc_lowcomm_monitor_name_get_csv_list(void);
void _mpc_lowcomm_monitor_name_free_cvs_list(char *list);

#endif /* _MPC_LOWCOMM_NAME_H_ */