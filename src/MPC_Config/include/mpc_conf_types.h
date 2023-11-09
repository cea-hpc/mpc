#ifndef MPC_CONF_TYPES_H
#define MPC_CONF_TYPES_H

#include <stdio.h>

typedef enum
{
	MPC_CONF_INT,
	MPC_CONF_LONG_INT,
	MPC_CONF_DOUBLE,
	MPC_CONF_STRING,
	MPC_CONF_FUNC,
	MPC_CONF_TYPE,
	MPC_CONF_BOOL,
  MPC_CONF_ENUM,
	MPC_CONF_TYPE_NONE
}mpc_conf_type_t;

#define MPC_CONF_STRING_SIZE 512

/************************
* MPC_CONF ENUM KEY VALUE
************************/

typedef struct {
  char * key;
  int value;
} mpc_conf_enum_keyval_t;

const char *mpc_conf_type_name(mpc_conf_type_t type);

int mpc_conf_type_print_value(mpc_conf_type_t type, char *buf, int len, void *ptr, int do_color, mpc_conf_enum_keyval_t * ekv, int ekv_length);

int mpc_conf_type_set_value(mpc_conf_type_t type, void **dest, void *from);

int mpc_conf_type_set_value_from_string(mpc_conf_type_t type, void **dest, char *from, mpc_conf_enum_keyval_t * ekv, int ekv_length);

mpc_conf_type_t mpc_conf_type_infer_from_string(char *string);

int mpc_conf_type_is_string(mpc_conf_type_t type);

ssize_t mpc_conf_type_size(mpc_conf_type_t type);

#endif /* MPC_CONF_TYPES_H */
