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

    int init_count;
};

void _mpi_t_state_init(void);
void _mpi_t_state_release(void);
void _mpit_state_add_var(_mpc_mpi_mpit_var_t *var);
void _mpit_state_add_cat(_mpc_mpi_mpit_cat_t *cat);
_mpc_mpi_mpit_cat_t * _mpit_state_get_cat_by_conf_type(mpc_conf_config_type_t * conf_node);

#endif /* MPC_MPI_MPIT_H */
