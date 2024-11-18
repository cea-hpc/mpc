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
#ifndef MPC_MPI_MPIT_H
#define MPC_MPI_MPIT_H

#include <mpc_mpi.h>
#include <mpc_conf.h>


#define MPI_T_PVAR_SESSION_LIVE (1)

#define MPI_T_NB_SAFETY_LEVEL 4

typedef enum{
    MPC_MPI_T_SEND,
    MPC_MPI_T_RECV
}_mpc_mpi_mpit_event_type_t;

typedef struct _mpc_mpi_mpit_event_s{
    char * name; /* name of the event */
    char * doc; /* documentation of the event */
    int event_index; 
    int id; 
    int source_index; /* index of source from which the event is triggered */
    int len_name; /* len of name */
    char * descr; /* description of the event */
    int len_descr; /* length of the descr */
    MPI_Datatype *array_of_datatypes;
    MPI_Aint *array_of_displacements;
    int num_elements_datatype; /* number of elements in array_of_datatypes */
    MPI_Info *info;
    int size_data; /* size of data buffer for each instance */
}_mpc_mpi_mpit_event_t;

typedef enum{
    MPC_MPI_T_UNDEFINED,
    MPC_MPI_T_PVAR,
    MPC_MPI_T_CVAR
}_mpc_mpi_mpit_var_type_t;


typedef struct _mpc_mpi_mpit_var_s
{
    int id;
    _mpc_mpi_mpit_var_type_t type;
    mpc_conf_config_type_elem_t * elem_node;
}_mpc_mpi_mpit_var_t;

typedef struct _mpc_mpi_mpit_event_source_s
{
    char * name; /* name of the event */
    char * doc; /* documentation of the event */
    int source_index; /* index of source */
    int len_name; /* len of name */
    char * descr; /* description of the event */
    int len_descr; /* length of the descr */
    MPI_T_source_order ordering;
    MPI_Count ticks_per_second; 
    MPI_Count timestamp; 
    MPI_Count max_ticks;
    MPI_Info *info;
}_mpc_mpi_mpit_event_source_t;

typedef struct _mpc_mpi_mpit_callback_s
{
    int id;
    MPI_T_event_cb_function *cb;
    MPI_T_event_instance event_instance;
    MPI_T_event_registration event_registration;
    MPI_T_cb_safety cb_safety; 
    MPI_Count timestamp; 
    void *user_data;
}_mpc_mpi_mpit_callback_t;

typedef struct _mpc_mpi_mpit_dropped_callback_s
{
    MPI_T_event_dropped_cb_function *dropped_cb;
    MPI_T_event_registration event_registration;
    MPI_Count timestamp; 
}_mpc_mpi_mpit_dropped_callback_t;

typedef struct _mpc_mpi_mpit_instance_s
{
    int id;
    MPI_T_event_registration *event_registration;
}_mpc_mpi_mpit_event_instance_t;

typedef struct _mpc_mpi_mpit_event_registration_s
{
    int id;
    int event_index;
    _mpc_mpi_mpit_callback_t ** callbacks;
    int callback_count;
    int callback_size;
    _mpc_mpi_mpit_dropped_callback_t * dropped_callback;
    int dropped_count;
    MPI_T_event_registration * event_registration;
    MPI_Info info;
    void * obj_handle;
    int source_id;
}_mpc_mpi_mpit_event_registration_t;


#define MPIT_CAT_NAME_LEN 2048

typedef struct _mpc_mpi_mpit_category_s
{
    int id;
    char name[MPIT_CAT_NAME_LEN];

    /* Note the elem and the conf type are bound */
    mpc_conf_config_type_elem_t * elem;
    mpc_conf_config_type_t * conf_node;

    int children_count;
    struct _mpc_mpi_mpit_category_s ** children;

    _mpc_mpi_mpit_var_type_t type;
    int var_count;
    _mpc_mpi_mpit_var_t * vars;
}_mpc_mpi_mpit_cat_t;

/***************************
 * THE MPI_T STORAGE PLACE *
 ***************************/

struct _mpi_t_state
{
    _mpc_mpi_mpit_cat_t **categories;
    int categories_count;
    int categories_size;

    _mpc_mpi_mpit_var_t **cvars;
    int cvar_count;
    int cvar_size;

    _mpc_mpi_mpit_var_t **pvars;
    int pvar_count;
    int pvar_size;

    _mpc_mpi_mpit_event_source_t **event_sources;
    int event_source_count;
    int event_source_size;

    _mpc_mpi_mpit_event_t **events;
    int event_count;
    int event_size;

    _mpc_mpi_mpit_callback_t **callbacks;
    int callback_count;
    int callback_size;

    _mpc_mpi_mpit_event_registration_t **event_registrations;
    int event_registration_count;
    int event_registration_size;

    MPI_T_event_instance **event_instances;
    int event_instance_count;
    int event_instance_size;

    int init_count;
};

void _mpi_t_state_init(void);
void _mpi_t_state_release(void);
void _mpit_state_add_var(_mpc_mpi_mpit_var_t *var);
void _mpit_state_add_cat(_mpc_mpi_mpit_cat_t *cat);
_mpc_mpi_mpit_cat_t * _mpit_state_get_cat_by_conf_type(mpc_conf_config_type_t * conf_node);

void mpc_mpi_mpit_alloc_data(void ** data, MPI_Datatype **array_of_datatypes, MPI_Aint **array_of_displacements, int event_type);
int mpc_mpi_mpit_looking_for_event_to_trigger(int event_index);
void mpc_mpi_mpit_trigger_event(int event_index);

#endif /* MPC_MPI_MPIT_H */
