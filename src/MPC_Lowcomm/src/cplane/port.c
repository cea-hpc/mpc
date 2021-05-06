#include <mpc_lowcomm_monitor.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>

#define MPC_LOWCOMM_MONITOR_PORT_RANGE_START 10000

int __current_port_id = MPC_LOWCOMM_MONITOR_PORT_RANGE_START;
mpc_common_spinlock_t __current_port_id_lock = SCTK_SPINLOCK_INITIALIZER;


int mpc_lowcomm_monitor_open_port(char * id, int id_len)
{
    int ret = -1;
    mpc_common_spinlock_lock(&__current_port_id_lock);
    ret = __current_port_id++;
    mpc_common_spinlock_unlock(&__current_port_id_lock);

    int len = snprintf(id, id_len, "%lu:%d", mpc_lowcomm_monitor_get_uid(), ret);

    if(len >= id_len)
    {
        /* We truncated */
        return -1;
    }

    return 0;
}

int mpc_lowcomm_monitor_close_port(const char * id)
{
    mpc_lowcomm_peer_uid_t uid = 0;
    int port = 0;

    /* Nothing particular to do except some sanity checks */
    if( _mpc_lowcomm_monitor_parse_port(id, &uid, &port) != 0)
    {
        return -1;
    }

    /* Now check the values */
    if( (mpc_lowcomm_monitor_get_uid() != uid) ||
        (port < MPC_LOWCOMM_MONITOR_PORT_RANGE_START) )
    {
        return -1;
    }

    return 0;
}


int mpc_lowcomm_monitor_parse_port(const char * id,  mpc_lowcomm_peer_uid_t *uid, int * port)
{
    int ret = 0;

    char * tmp = strdup(id);
    assume(tmp != NULL);

    char * sep = strchr(tmp, ':');

    if(!sep)
    {
        ret = -1;
        goto PORT_PARSE_END;
    }

    *sep = '\0';
    char * speer = tmp;
    char * sport = sep + 1;

    if( !strlen(speer) || !strlen(sport))
    {
        ret = -1;
        goto PORT_PARSE_END;
    }

    *port = atoi(sport);

    char *endptr = NULL;

    *uid = strtoull(speer, &endptr, 10);

    if ( speer == endptr ) {
        ret = -1;
        goto PORT_PARSE_END;
    }

PORT_PARSE_END:
    free(tmp);
    return ret;
}