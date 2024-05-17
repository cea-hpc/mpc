#ifndef LOADER_H
#define LOADER_H

#include "mpc_conf.h"

typedef enum
{
    MPC_CONF_MOD_ERROR,
    MPC_CONF_MOD_NONE,
    MPC_CONF_MOD_SYSTEM,
    MPC_CONF_MOD_USER,
    MPC_CONF_MOD_BOTH
}mpc_conf_config_loader_rights_t;


mpc_conf_config_loader_rights_t mpc_conf_config_loader_rights_parse(const char * str);

mpc_conf_config_type_t * mpc_conf_config_loader_paths(char * conf_name,
                                                      char * system_prefix,
                                                      char * user_prefix,
                                                      char * can_create);

mpc_conf_config_type_t * mpc_conf_config_loader_search_path(char * conf_name);

int mpc_conf_config_load(char * conf_name);

#endif /* LOADER_H */
