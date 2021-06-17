/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "session.h"

int PMPI_Session_create_errhandler(MPI_Session_errhandler_function *session_errhandler_fn,
                                   MPI_Errhandler *errhandler)
{

}

int PMPI_Session_set_errhandler(MPI_Session session, MPI_Errhandler errhandler)
{

}

int PMPI_Session_get_errhandler(MPI_Session session, MPI_Errhandler *errhandler)
{

}

int PMPI_Session_call_errhandler(MPI_Session session, int error_code)
{

}

int PMPI_Session_init(MPI_Info info, MPI_Errhandler errhandler, MPI_Session *session)
{
    int res = MPI_SUCCESS;

    if(res != MPI_SUCCESS)
    {
        /* Call errhandler */
    }

    return res;
}

/* MPI_Session_finalize */

int PMPI_Session_finalize(MPI_Session *session)
{

}

/* MPI_Session_get_info */

int PMPI_Session_get_info(MPI_Session session, MPI_Info *info_used)
{

}

/* MPI_Session_get_num_psets */

int PMPI_Session_get_num_psets(MPI_Session session, MPI_Info info, int *npset_names)
{

}

/* MPI_Session_get_nth_pset */

int PMPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n, int *pset_len, char *pset_name)
{

}

/* MPI_Session_get_pset_info */

int PMPI_Session_get_pset_info(MPI_Session session, const char * pset_name, MPI_Info *info)
{

}


int PMPI_Group_from_session_pset(MPI_Session session, const char *pset_name, MPI_Group *newgroup)
{
    
}
