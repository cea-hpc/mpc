#ifndef MPC_CONF_H
#define MPC_CONF_H

#include <stdio.h>

#include "mpc_conf_types.h"

/***************
 * OUTPUT KIND *
 ***************/

typedef enum{
	MPC_CONF_FORMAT_NONE,
	MPC_CONF_FORMAT_CONF,
	MPC_CONF_FORMAT_INI,
	MPC_CONF_FORMAT_XML
}mpc_conf_output_type_t;

mpc_conf_output_type_t mpc_conf_output_type_infer_from_file(char *path);

/*****************
* MPC_CONF ELEM *
*****************/

typedef struct mpc_conf_config_type_elem_s
{
	struct mpc_conf_config_type_elem_s * parent;
	char *                    name;
	void *                    addr;
	mpc_conf_type_t           type;
	char *                    doc;
  mpc_conf_enum_keyval_t *  ekv;
  int                       ekv_length;
	short                     addr_is_to_free;
	short                     is_locked;
}mpc_conf_config_type_elem_t;

mpc_conf_config_type_elem_t *mpc_conf_config_type_elem_init(char *name, void *addr, mpc_conf_type_t type, char *doc, mpc_conf_enum_keyval_t * ekv, int ekv_length);

void mpc_conf_config_type_elem_release(mpc_conf_config_type_elem_t **elem);

int mpc_conf_config_type_elem_set_from_elem(mpc_conf_config_type_elem_t *elem, mpc_conf_config_type_elem_t *src);

int mpc_conf_config_type_elem_set(mpc_conf_config_type_elem_t *elem, mpc_conf_type_t type, void *ptr);

int mpc_conf_config_type_elem_set_doc(mpc_conf_config_type_elem_t *elem, char * doc);

int mpc_conf_config_type_elem_set_from_string(mpc_conf_config_type_elem_t *elem, mpc_conf_type_t type, char *string);

void mpc_conf_config_type_elem_set_to_free(mpc_conf_config_type_elem_t *elem, int to_free);

void mpc_conf_config_type_elem_set_locked(mpc_conf_config_type_elem_t *elem, int locked);

int mpc_conf_config_type_elem_print_fd(mpc_conf_config_type_elem_t *elem, FILE * fd, mpc_conf_output_type_t output_type);

int mpc_conf_config_type_elem_print(mpc_conf_config_type_elem_t *elem, mpc_conf_output_type_t output_type);

int mpc_conf_type_elem_get_as_int(mpc_conf_config_type_elem_t * elem);

char * mpc_conf_type_elem_get_as_string(mpc_conf_config_type_elem_t * elem);

int mpc_conf_config_type_elem_get_path_to(mpc_conf_config_type_elem_t *elem, char * path, int len, char * separator);

/**********************
* CONFIGURATION TYPE *
**********************/

#define MPC_CONF_CONFIG_TYPE_MAX_ELEM    128

typedef struct mpc_conf_config_type_s
{
	char *                       name;
	mpc_conf_config_type_elem_t *elems[MPC_CONF_CONFIG_TYPE_MAX_ELEM];
	unsigned int                 elem_count;
	int                          is_array;
}mpc_conf_config_type_t;

#define PARAM(name, ptr, type, doc, ...)    name, ptr, (int)type, doc __VA_OPT__(, __VA_ARGS__)
#define ENUM_KEYVAL(...) (sizeof((mpc_conf_enum_keyval_t[])__VA_ARGS__) / sizeof(mpc_conf_enum_keyval_t)), ((mpc_conf_enum_keyval_t[])__VA_ARGS__)

mpc_conf_config_type_t *mpc_conf_config_type_init(char *name, ...);

void mpc_conf_config_type_clear(mpc_conf_config_type_t *type);

void mpc_conf_config_type_release(mpc_conf_config_type_t **ptype);

mpc_conf_config_type_elem_t *mpc_conf_config_type_get(mpc_conf_config_type_t *type, char *name);

mpc_conf_config_type_elem_t *mpc_conf_config_type_append(mpc_conf_config_type_t *type,
														 char *ename,
														 void *eptr,
														 mpc_conf_type_t etype,
														 char *edoc,
                             mpc_conf_enum_keyval_t * eekv,
                             int eekv_length);

mpc_conf_config_type_t * mpc_conf_config_type_elem_update(mpc_conf_config_type_t * ref, mpc_conf_config_type_t * updater, int force_content);

int mpc_config_type_match_order(mpc_conf_config_type_t *type, mpc_conf_config_type_t *ref);

int mpc_config_type_pop_elem(mpc_conf_config_type_t *type, mpc_conf_config_type_elem_t * elem);

int mpc_config_type_append_elem(mpc_conf_config_type_t *type, mpc_conf_config_type_elem_t * elem);

int mpc_conf_config_type_print_fd(mpc_conf_config_type_t *type, FILE * fd, mpc_conf_output_type_t output_type);

int mpc_conf_config_type_print(mpc_conf_config_type_t *type, mpc_conf_output_type_t output_type);

unsigned int mpc_conf_config_type_count(mpc_conf_config_type_t *type);

mpc_conf_config_type_elem_t *mpc_conf_config_type_nth(mpc_conf_config_type_t *type, unsigned int id);

mpc_conf_config_type_t *  mpc_conf_type_elem_get_root(mpc_conf_config_type_elem_t * elem);

mpc_conf_config_type_t *mpc_conf_config_type_elem_get_inner(mpc_conf_config_type_elem_t *elem);


/*******************
 * MPC_CONF CONFIG *
 *******************/

typedef struct mpc_conf_self_config_s
{
	int init_done;
	int color_enabled;
	int indent_count;
	int verbose;
	int is_valid;
}mpc_conf_self_config_t;

mpc_conf_self_config_t * mpc_conf_self_config_get(void);

/**********************
* ROOT CONFIGURATION *
**********************/

int mpc_conf_root_config_append(char *conf_name, mpc_conf_config_type_t *conf, char *doc);

mpc_conf_config_type_elem_t * mpc_conf_root_elem(char * conf_name);

mpc_conf_config_type_t *mpc_conf_root_config(char *conf_name);

int mpc_conf_root_config_init(char *conf_name);

int mpc_conf_root_config_release(char *conf_name);

int mpc_conf_root_config_release_all(void);

mpc_conf_config_type_elem_t *mpc_conf_root_config_get_sep(char *path, char *separator);

mpc_conf_config_type_elem_t *mpc_conf_root_config_get(char *path);

mpc_conf_config_type_elem_t *mpc_conf_root_config_set_sep(char *path,
                                                          char *separator,
                                                          mpc_conf_type_t etype,
                                                          void *eptr,
                                                          int allow_create);

mpc_conf_config_type_elem_t *mpc_conf_root_config_set(char *path,
                                                      mpc_conf_type_t etype,
                                                      void *eptr,
                                                      int allow_create);

int mpc_conf_root_config_search_path(char *conf_name, char * system, char * user, char * rights);

int mpc_conf_root_config_delete_sep(char *path, char *separator);

int mpc_conf_root_config_set_lock(char * path, int is_locked);

int mpc_conf_root_config_delete(char *path);

int mpc_conf_root_config_print_fd(FILE *fd, mpc_conf_output_type_t output_type);

int mpc_conf_root_config_print(mpc_conf_output_type_t output_type);

int mpc_conf_root_config_load_env(char *conf_name);

int mpc_conf_root_config_load_env_all(void);

int mpc_conf_root_config_load_files(char *conf_name);

int mpc_conf_root_config_load_files_all(void);

int mpc_conf_root_config_save(char *output_file, mpc_conf_output_type_t output_type);

/***********
 * HELPERS *
 ***********/

int mpc_conf_resolvefunc(char * func_name, void (**func_ptr)());

#define __mpc_conf_stringify(a) #a
#define mpc_conf_stringify(a) __mpc_conf_stringify(a)

char * mpc_conf_user_prefix(char *conf_name, char *buff, int size);

#endif
