/* ############################# MPC License ############################## */
/* # Wed Jan 13 09:39:06 CET 2021                                         # */
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

#include "mpit.h"
#include "mpc_common_spinlock.h"
#include "mpc_mpi.h"
#include "mpc_mpi_internal.h"
#include <sctk_alloc.h>

/*********************
 * VARIABLES STORAGE *
 *********************/

_mpc_mpi_mpit_var_t * _mpc_mpi_mpit_var_init(_mpc_mpi_mpit_var_type_t type, mpc_conf_config_type_elem_t * node)
{
    _mpc_mpi_mpit_var_t * ret = sctk_malloc(sizeof(_mpc_mpi_mpit_var_t));
    assume(ret != NULL);

    ret->id = -1;
    ret->type = type;
    ret->elem_node = node;

    return ret;
}

/***********************
 * CATEGORIES HANDLING *
 ***********************/

static inline _mpc_mpi_mpit_cat_t * _mpc_mpi_mpit_cat_init(mpc_conf_config_type_elem_t * node,
                                                           _mpc_mpi_mpit_var_type_t type)
{
    _mpc_mpi_mpit_cat_t * ret = sctk_malloc(sizeof(_mpc_mpi_mpit_cat_t));
    assume(ret != NULL);

    ret->id = -1;


    ret->elem = node;
    mpc_conf_config_type_t *conf_type = mpc_conf_config_type_elem_get_inner(node);
    ret->conf_node = conf_type;

    ret->children_count = 0;
    ret->children = NULL;

    ret->type = type;
    ret->var_count = 0;
    ret->vars = NULL;

    mpc_conf_config_type_elem_get_path_to(node, ret->name, MPIT_CAT_NAME_LEN, ".");

    return ret;
}

static inline _mpc_mpi_mpit_cat_t * __cat_recursive_scan_conf(mpc_conf_config_type_elem_t * elem, _mpc_mpi_mpit_var_type_t type)
{
    if(!elem)
    {
        return NULL;
    }

    mpc_conf_config_type_t *conf_type = mpc_conf_config_type_elem_get_inner(elem);

    unsigned int i = 0;
    unsigned int len = mpc_conf_config_type_count(conf_type);

    /* Count each element */
    int num_var = 0;
    int num_cat = 0;

    for(i = 0 ; i < len; i++)
    {
        mpc_conf_config_type_elem_t * elem = mpc_conf_config_type_nth(conf_type, i);

        if(elem->type == MPC_CONF_TYPE)
        {
            num_cat++;
        }
        else
        {
            num_var++;
        }
    }

    int var_added = 0;
    int cat_added = 0;

    /* Now process */

    _mpc_mpi_mpit_cat_t * new_cat = _mpc_mpi_mpit_cat_init(elem, type);
    assume(new_cat != NULL);

    new_cat->children = sctk_malloc(sizeof(_mpc_mpi_mpit_cat_t *) * num_cat);
    assume(new_cat->children != NULL);
    new_cat->children_count = num_cat;

    new_cat->vars = sctk_malloc(sizeof(_mpc_mpi_mpit_var_t *) * num_cat);
    assume(new_cat->vars != NULL);

    for(i = 0 ; i < len; i++)
    {
        mpc_conf_config_type_elem_t * elem = mpc_conf_config_type_nth(conf_type, i);

        if(elem->type == MPC_CONF_TYPE)
        {
            _mpc_mpi_mpit_cat_t * child_cat = __cat_recursive_scan_conf(elem, type);
            new_cat->children[cat_added] = child_cat;
            cat_added++;
        }
        else
        {
            _mpc_mpi_mpit_var_t * new_var = _mpc_mpi_mpit_var_init(type, elem);

            /* Some variables could not be supported by MPI_T (bad type for example) */
            if(new_var)
            {
                _mpit_state_add_var(new_var);
                var_added++;
            }

        }
    }

    /* Make sure we are coherent */
    assume(cat_added == num_cat);

    /* We could have skipped some vars */
    num_var = var_added;
    new_cat->var_count = num_var;

    _mpit_state_add_cat(new_cat);

    return new_cat;
}

/*************************
 * MPIT INIT AND RELEASE *
 *************************/

struct _mpi_t_state __mpit = { 0 };
static mpc_common_spinlock_t __mpit_init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;


static inline void __expandable_array_init(volatile void ***parray, int * psize)
{
    *parray = sctk_malloc(sizeof(void *) * 2);
    assume(*parray != NULL);
    *psize = 2;
}

static inline int __expandable_array_add(volatile void ***parray, int *psize, int *pcount, void * ptr)
{
    volatile void ** array = *parray;

    int cur_off = *pcount;

    array[cur_off] = ptr;
    *pcount = cur_off + 1;

    /* Do we need more space ? */
    if(*psize <= *pcount )
    {
        *psize = *psize * 2;

        *parray = sctk_realloc(array, *psize * sizeof(void *));
        assume(*parray != NULL);
    }


    return cur_off;
}



static inline void __expandable_array_free(volatile void ***parray, int *pcount)
{
    int i = 0;
    volatile void ** array = *parray;

    for( i = 0 ; i < *pcount; i++)
    {
        volatile void *elem = array[i];

        if(elem)
        {
            sctk_free((void *)elem);
            array[i] = NULL;
        }
    }

    sctk_free(array);

    *pcount = 0;
}

void _mpit_state_add_var(_mpc_mpi_mpit_var_t *var)
{
    mpc_common_debug("ADD VAR %s", var->elem_node->name);
    volatile void ***parray = NULL;
    int *pcount = NULL;
    int *psize = NULL;

    switch (var->type)
    {
    case MPC_MPI_T_PVAR:
        parray = (volatile void ***)&__mpit.pvars;
        pcount = &__mpit.pvar_count;
        psize = &__mpit.pvar_size;
        break;
    case MPC_MPI_T_CVAR:
        parray = (volatile void ***)&__mpit.cvars;
        pcount = &__mpit.cvar_count;
        psize = &__mpit.cvar_size;
        break;
    default:
        not_reachable();
        break;
    }

    int new_id = __expandable_array_add(parray,psize, pcount, (void *)var);
    /* Store ID inside the variable */
    var->id = new_id;
}

void _mpit_state_add_cat(_mpc_mpi_mpit_cat_t *cat)
{
    mpc_common_debug("ADD CAT %s", cat->name);

    int new_id = __expandable_array_add((volatile void ***)&__mpit.categories,
                                        &__mpit.categories_size,
                                        &__mpit.categories_count,
                                        (void *)cat);
    /* Store ID inside the variable */
    cat->id = new_id;
}

void _mpi_t_state_init(void)
{
    int did_init = 0;

    mpc_common_spinlock_lock(&__mpit_init_lock);

    did_init = __mpit.init_count;

    __mpit.init_count++;

    mpc_common_spinlock_unlock(&__mpit_init_lock);

    if(did_init)
    {
        /* Only the first process does the init for the process */
        return;
    }

    mpc_common_debug("MPIT INIT");

    /* Storage base */
    __expandable_array_init((volatile void ***)&__mpit.categories, &__mpit.categories_size);
    __mpit.categories_count = 0;

    __expandable_array_init((volatile void ***)&__mpit.cvars, &__mpit.cvar_size);
    __mpit.cvar_count = 0;

    __expandable_array_init((volatile void ***)&__mpit.pvars, &__mpit.pvar_size);
    __mpit.pvar_count = 0;

    mpc_common_debug("MPIT INIT DONE");


    /* Now register each main component of the config */
    mpc_conf_config_type_elem_t *conf_conf = mpc_conf_root_elem("conf");

    if(conf_conf)
    {
        __cat_recursive_scan_conf(conf_conf, MPC_MPI_T_CVAR);
    }

    mpc_conf_config_type_elem_t *mpc_conf = mpc_conf_root_elem("mpcframework");

    if(mpc_conf)
    {
        __cat_recursive_scan_conf(mpc_conf, MPC_MPI_T_CVAR);
    }

    mpc_conf_config_type_elem_t *prof_conf = mpc_conf_root_elem("profiling");

    if(prof_conf)
    {
        __cat_recursive_scan_conf(prof_conf, MPC_MPI_T_PVAR);
    }
}


void _mpi_t_state_release(void)
{
    int did_init = 0;

    mpc_common_spinlock_lock(&__mpit_init_lock);

    did_init = __mpit.init_count;

    __mpit.init_count--;

    mpc_common_spinlock_unlock(&__mpit_init_lock);

    if(did_init != 1)
    {
        /* Only the last process will free */
        return;
    }


    __expandable_array_free((volatile void***)&__mpit.categories, &__mpit.categories_count);
    __expandable_array_free((volatile void***)&__mpit.cvars, &__mpit.cvar_count);
    __expandable_array_free((volatile void***)&__mpit.pvars, &__mpit.pvar_count);
}



#pragma weak MPI_T_init_thread = PMPI_T_init_thread

int PMPI_T_init_thread(int required, int *provided)
{
    *provided = required;

    _mpi_t_state_init();

    return MPI_SUCCESS;
}

#pragma weak MPI_T_finalize = PMPI_T_finalize

int PMPI_T_finalize()
{
    _mpi_t_state_release();
    return MPI_SUCCESS;
}

/**************
 * MPIT ENUMS *
 **************/

#pragma weak MPI_T_enum_get_info = PMPI_T_enum_get_info

int PMPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len)
{
    mpc_conf_config_type_t * etype = (mpc_conf_config_type_t *)enumtype;

    if(!etype || !num || !name || !name_len)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID, "One of the arguments was NULL");
    }

    *num = mpc_conf_config_type_count(etype);
    (void)snprintf(name, *name_len, "%s", etype->name);
    *name_len = (int)strlen(name);

    return MPI_SUCCESS;
}

#pragma weak MPI_T_enum_get_item = PMPI_T_enum_get_item

int PMPI_T_enum_get_item(MPI_T_enum enumtype, int index, int *value, char *name, int *name_len)
{
    mpc_conf_config_type_t * etype = (mpc_conf_config_type_t *)enumtype;

    if(!etype || !name || !name_len)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID, "One of the arguments was NULL");
    }

    int len = mpc_conf_config_type_count(etype);

    if(len <= index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_ITEM, "MPI_T_Enum index out of range");
    }

    mpc_conf_config_type_elem_t * elem = mpc_conf_config_type_nth(etype, index);

    if(elem)
    {
        /* We only expect integers */
        if(elem->type != MPC_CONF_INT)
        {
            MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "MPI_T_Enum failed to retrieve item (bad internal type)");
        }

        *value = mpc_conf_type_elem_get_as_int(elem);
        (void)snprintf(name, *name_len, "%s", elem->name);
        *name_len = (int)strlen(name);

    }
    else
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "MPI_T_Enum failed to retrieve item");
    }

    return MPI_SUCCESS;
}

/*******************
 * MPIT CATEGORIES *
 *******************/
#pragma weak MPI_T_category_changed = PMPI_T_category_changed

int PMPI_T_category_changed(int *stamp)
{
    /* We do not plan to support dyn load yet */
    *stamp = 1337;
    return MPI_SUCCESS;
}

#pragma weak MPI_T_category_get_categories = PMPI_T_category_get_categories

int PMPI_T_category_get_categories(int cat_index, int len, int indices[])
{
    if(__mpit.categories_count <= cat_index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad index");
    }

    _mpc_mpi_mpit_cat_t * cat = __mpit.categories[cat_index];

    int i = 0;

    for(i = 0 ; (i < len) && (i < cat->children_count); i++ )
    {
        indices[i] = cat->children[i]->id;
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_category_get_pvars = PMPI_T_category_get_pvars

int PMPI_T_category_get_pvars(int cat_index, int len, int indices[])
{
    if(__mpit.categories_count <= cat_index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad index");
    }

    _mpc_mpi_mpit_cat_t * cat = __mpit.categories[cat_index];

    if(cat->type != MPC_MPI_T_PVAR)
    {
        return MPI_SUCCESS;
    }

    int i = 0;

    for( i = 0 ; (i < len) && (i < cat->var_count); i++)
    {
        indices[i] = cat->vars[i].id;
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_category_get_cvars = PMPI_T_category_get_cvars

int PMPI_T_category_get_cvars(int cat_index, int len, int indices[])
{
    if(__mpit.categories_count <= cat_index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad index");
    }

    _mpc_mpi_mpit_cat_t * cat = __mpit.categories[cat_index];

    if(cat->type != MPC_MPI_T_CVAR)
    {
        return MPI_SUCCESS;
    }

    int i = 0;

    for( i = 0 ; (i < len) && (i < cat->var_count); i++)
    {
        indices[i] = cat->vars[i].id;
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_category_get_index = PMPI_T_category_get_index

int PMPI_T_category_get_index(const char *name, int *cat_index)
{
    int i = 0;

    for( i = 0 ; i < __mpit.categories_count; i++)
    {
        _mpc_mpi_mpit_cat_t * cat = __mpit.categories[i];

        if(!strcmp(cat->conf_node->name, name))
        {
            *cat_index = i;
            return MPI_SUCCESS;
        }
    }

    MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_NAME, "Could not find given category name");
}

#pragma weak MPI_T_category_get_info = PMPI_T_category_get_info

int PMPI_T_category_get_info(int cat_index,
                             char *name, int *name_len,
                             char *desc, int *desc_len,
                             int *num_cvars,
                             int *num_pvars,
                             int *num_categories)
{
    if(__mpit.categories_count <= cat_index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad index");
    }

    _mpc_mpi_mpit_cat_t * cat = __mpit.categories[cat_index];

    (void)snprintf(name, *name_len, "%s", cat->name);
    *name_len = (int)strlen(cat->name);

    assume(cat->elem != NULL);
    (void)snprintf(desc, *desc_len, "%s", cat->elem->doc);
    *desc_len = (int)strlen(cat->elem->doc);

    /* Now fill counters */
    *num_categories = cat->children_count;

    *num_pvars = 0;
    *num_cvars = 0;

    switch (cat->type)
    {
    case MPC_MPI_T_PVAR:
        *num_pvars = cat->var_count;
        break;
    case MPC_MPI_T_CVAR:
        *num_cvars = cat->var_count;
        break;
    default:
        not_reachable();
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_category_get_num = PMPI_T_category_get_num

int PMPI_T_category_get_num(int *num_cat)
{
    *num_cat = __mpit.categories_count;
    return MPI_SUCCESS;
}

/*********************
 * CONTROL VARIABLES *
 *********************/

#pragma weak MPI_T_cvar_get_index = PMPI_T_cvar_get_index

int PMPI_T_cvar_get_index(const char *name, int *cvar_index)
{
    int i = 0;

    for( i = 0 ; i < __mpit.cvar_count; i++)
    {
        _mpc_mpi_mpit_var_t * var = __mpit.cvars[i];

        if(!strcmp(var->elem_node->name, name))
        {
            *cvar_index = i;
            return MPI_SUCCESS;
        }
    }

    MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_NAME, "Could not find given cvar name");
}

static inline MPI_Datatype __conf_type_to_mpi(mpc_conf_type_t type)
{
    switch (type)
    {
	case MPC_CONF_LONG_INT:
    {
        if(sizeof(long int) == 8)
        {
            return MPI_INT64_T;
        }

        if(sizeof(long int) == 4)
        {
            return MPI_INT32_T;
        }

        not_implemented();
    }
	case MPC_CONF_DOUBLE:
        return MPI_DOUBLE;
	case MPC_CONF_STRING:
	case MPC_CONF_FUNC:
        return MPI_CHAR;
	case MPC_CONF_TYPE:
        not_reachable();
	case MPC_CONF_INT:
	case MPC_CONF_BOOL:
        return MPI_INT;
	case MPC_CONF_TYPE_NONE:
        not_reachable();
    default:
        not_implemented();
    }

    not_reachable();
    return MPI_DATATYPE_NULL;
}



#pragma weak MPI_T_cvar_get_info = PMPI_T_cvar_get_info

int PMPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *scope)
{
    if(__mpit.cvar_count <= cvar_index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad cvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.cvars[cvar_index];

    (void)snprintf(name, *name_len, "%s", var->elem_node->name);
    *name_len = (int)strlen(var->elem_node->name);

    /* Not supported yet */
    *verbosity = MPI_T_VERBOSITY_USER_ALL;

    *datatype = __conf_type_to_mpi(var->elem_node->type);

    /* Not supported yet (needs choices inside the config) */
    *enumtype = NULL;


    (void)snprintf(desc, *desc_len, "%s", var->elem_node->doc);
    *desc_len = (int)strlen(var->elem_node->doc);

    /* Not supported */
    *bind = MPI_T_BIND_NO_OBJECT;

    if(var->elem_node->is_locked)
    {
        *scope = MPI_T_SCOPE_CONSTANT;
    }
    else
    {
        /* Not support yet therefore we are conservative */
        *scope = MPI_T_SCOPE_ALL;
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_cvar_get_num = PMPI_T_cvar_get_num

int PMPI_T_cvar_get_num(int *num_cvar)
{
    *num_cvar = __mpit.cvar_count;
    return MPI_SUCCESS;
}

#pragma weak MPI_T_cvar_handle_alloc = PMPI_T_cvar_handle_alloc

int PMPI_T_cvar_handle_alloc(int cvar_index,
                             void *obj_handle,
                             MPI_T_cvar_handle *handle,
                             int *count)
{
    if(__mpit.cvar_count <= cvar_index)
    {
        *handle = MPI_T_CVAR_HANDLE_NULL;
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad cvar index");
    }

    /* obj_handle is ignored as being not implemented */
    if(obj_handle)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_HANDLE, "Binding to handles is not supported");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.cvars[cvar_index];


    *handle = cvar_index;

    if(var->elem_node->type == MPC_CONF_STRING)
    {
        *count = MPC_CONF_STRING_SIZE;
    }
    else
    {
        *count = 1;
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_cvar_handle_free = PMPI_T_cvar_handle_free

int PMPI_T_cvar_handle_free(MPI_T_cvar_handle *handle)
{
    *handle = MPI_T_CVAR_HANDLE_NULL;
    return MPI_SUCCESS;
}

#pragma weak MPI_T_cvar_read = PMPI_T_cvar_read

static inline int __read_var(_mpc_mpi_mpit_var_t * var, void * buff)
{
    size_t type_size = mpc_conf_type_size(var->elem_node->type);
    memcpy(buff, var->elem_node->addr, type_size);

    return MPI_SUCCESS;
}


int PMPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
{
    if(__mpit.cvar_count <= handle)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad cvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.cvars[handle];

    return __read_var(var, buf);
}

#pragma weak MPI_T_cvar_write = PMPI_T_cvar_write

int PMPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
{
    if(__mpit.cvar_count <= handle)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad cvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.cvars[handle];

    int rc = mpc_conf_config_type_elem_set(var->elem_node, var->elem_node->type, (void*)buf);

    if(rc != 0)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_CVAR_SET_NEVER, "Could not set this CVAR");
    }

    return MPI_SUCCESS;
}

/*************************
 * PERFORMANCE VARIABLES *
 *************************/

#pragma weak MPI_T_pvar_get_index = PMPI_T_pvar_get_index

int PMPI_T_pvar_get_index(const char *name, int var_class __UNUSED__, int *pvar_index)
{
    int i = 0;

    for( i = 0 ; i < __mpit.pvar_count; i++)
    {
        _mpc_mpi_mpit_var_t * var = __mpit.pvars[i];

        if(!strcmp(var->elem_node->name, name))
        {
            *pvar_index = i;
            return MPI_SUCCESS;
        }
    }

    MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_NAME, "Could not find given pvar name");
}

#pragma weak MPI_T_pvar_get_info = PMPI_T_pvar_get_info

int PMPI_T_pvar_get_info(int pvar_index,
                         char *name, int *name_len,
                         int *verbosity,
                         int *var_class,
                         MPI_Datatype *datatype,
                         MPI_T_enum *enumtype,
                         char *desc, int *desc_len,
                         int *bind,
                         int *readonly,
                         int *continuous,
                         int *atomic)
{
    if(__mpit.pvar_count <= pvar_index)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad cvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.pvars[pvar_index];

    (void)snprintf(name, *name_len, "%s", var->elem_node->name);
    *name_len = (int)strlen(var->elem_node->name);

    /* Not supported yet */
    *verbosity = MPI_T_VERBOSITY_USER_ALL;

    /* Not supported yet */
    *var_class = MPI_T_PVAR_CLASS_STATE;

    *datatype = __conf_type_to_mpi(var->elem_node->type);

    /* Not supported yet (needs choices inside the config) */
    *enumtype = NULL;

    (void)snprintf(desc, *desc_len, "%s", var->elem_node->doc);
    *desc_len = (int)strlen(var->elem_node->doc);

    /* Not supported */
    *bind = MPI_T_BIND_NO_OBJECT;

    *readonly = var->elem_node->is_locked;

    /* Not supported otherwise all vars are continuous */
    *continuous = 1;

    /* Not supported otherwise */
    *atomic = 0;


    return MPI_SUCCESS;
}

#pragma weak MPI_T_pvar_get_num = PMPI_T_pvar_get_num

int PMPI_T_pvar_get_num(int *num_pvar)
{
    *num_pvar = __mpit.pvar_count;
    return MPI_SUCCESS;
}

#pragma weak MPI_T_pvar_handle_alloc = PMPI_T_pvar_handle_alloc

int PMPI_T_pvar_handle_alloc(MPI_T_pvar_session session,
                             int pvar_index,
                             void *obj_handle,
                             MPI_T_pvar_handle *handle,
                             int *count)
{
    if(session != 1)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_SESSION, "PVAR sessions not initialized");
    }

    /* obj_handle is ignored as being not implemented */
    if(obj_handle)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_HANDLE, "Binding to handles is not supported");
    }

    if(__mpit.pvar_count <= pvar_index)
    {
        *handle = MPI_T_CVAR_HANDLE_NULL;
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Bad cvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.pvars[pvar_index];


    *handle = pvar_index;

    if(var->elem_node->type == MPC_CONF_STRING)
    {
        *count = MPC_CONF_STRING_SIZE;
    }
    else
    {
        *count = 1;
    }

    return MPI_SUCCESS;
}

#pragma weak MPI_T_pvar_handle_free = PMPI_T_pvar_handle_free

int PMPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle)
{
    if(session != MPI_T_PVAR_SESSION_LIVE)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_SESSION, "PVAR sessions not initialized");
    }

    if(*handle == MPI_T_PVAR_HANDLE_NULL)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_HANDLE, "Invalid handle");
    }

    *handle = MPI_T_PVAR_HANDLE_NULL;

    return MPI_SUCCESS;
}

#pragma weak MPI_T_pvar_read = PMPI_T_pvar_read

int PMPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
{
    if(session != MPI_T_PVAR_SESSION_LIVE)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_SESSION, "PVAR sessions not initialized");
    }

    if(handle == MPI_T_PVAR_HANDLE_NULL)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_HANDLE, "Invalid handle");
    }

    if(__mpit.pvar_count <= handle)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Invalid pvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.pvars[handle];

    return __read_var(var, buf);
}

#pragma weak MPI_T_pvar_readreset = PMPI_T_pvar_readreset

int PMPI_T_pvar_readreset(MPI_T_pvar_session session,
                          MPI_T_pvar_handle handle,
                          void *buf)
{
    int rc = PMPI_T_pvar_read(session, handle, buf);
    MPI_HANDLE_ERROR(rc, MPI_COMM_SELF, "Could not read variable");
    rc = MPI_T_pvar_reset(session, handle);
    MPI_HANDLE_ERROR(rc, MPI_COMM_SELF, "Could not reset variable");
    return rc;
}

#pragma weak MPI_T_pvar_reset = PMPI_T_pvar_reset

int PMPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    /* Max element size is the string */
    char buff[MPC_CONF_STRING_SIZE];
    memset(buff, 0, MPC_CONF_STRING_SIZE * sizeof(char));
    return PMPI_T_pvar_write(session, handle, buff);
}

#pragma weak MPI_T_pvar_session_create = PMPI_T_pvar_session_create

int PMPI_T_pvar_session_create(MPI_T_pvar_session *session)
{
    *session = MPI_T_PVAR_SESSION_LIVE;
    return MPI_SUCCESS;
}

#pragma weak MPI_T_pvar_session_free = PMPI_T_pvar_session_free

int PMPI_T_pvar_session_free(MPI_T_pvar_session *session)
{
    if(*session != MPI_T_PVAR_SESSION_LIVE)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_SESSION, "PVAR sessions not initialized");
    }

    *session = MPI_T_PVAR_SESSION_NULL;
    return MPI_SUCCESS;
}


#pragma weak MPI_T_pvar_start = PMPI_T_pvar_start

int PMPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    if(session != MPI_T_PVAR_SESSION_LIVE)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_SESSION, "PVAR sessions not initialized");
    }

    if(handle == MPI_T_PVAR_HANDLE_NULL)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_HANDLE, "Invalid handle");
    }

    /* For now we only have continuous variables */
    return MPI_SUCCESS;
}

#pragma weak MPI_T_pvar_stop = PMPI_T_pvar_stop

int PMPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    /* Both no op */
    return PMPI_T_pvar_start(session, handle);
}

#pragma weak MPI_T_pvar_write = PMPI_T_pvar_write

int PMPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf)
{
    if(session != MPI_T_PVAR_SESSION_LIVE)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_SESSION, "PVAR sessions not initialized");
    }

    if(handle == MPI_T_PVAR_HANDLE_NULL)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_HANDLE, "Invalid handle");
    }

    if(__mpit.pvar_count <= handle)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_INVALID_INDEX, "Invalid pvar index");
    }

    _mpc_mpi_mpit_var_t * var = __mpit.pvars[handle];

    int rc = mpc_conf_config_type_elem_set(var->elem_node, var->elem_node->type, (void*)buf);

    if(rc != 0)
    {
        MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_T_ERR_PVAR_NO_WRITE, "Could not set this CVAR");
    }

    return MPI_SUCCESS;
}
