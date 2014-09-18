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

#ifndef MPI_ARRAY_TYPES_H
#define MPI_ARRAY_TYPES_H

/* Content of this file is adapted from ROMIO:
 * Copyright (C) 1997 University of Chicago. 
                               COPYRIGHT

The following is a notice of limited availability of the code and
disclaimer, which must be included in the prologue of the code and in
all source listings of the code.

Copyright (C) 1997 University of Chicago

Permission is hereby granted to use, reproduce, prepare derivative
works, and to redistribute to others. 

The University of Chicago makes no representations as to the suitability, 
operability, accuracy, or correctness of this software for any purpose. 
It is provided "as is" without express or implied warranty. 

This software was authored by:
Rajeev Thakur: (630) 252-1682; thakur@mcs.anl.gov
Mathematics and Computer Science Division
Argonne National Laboratory, Argonne IL 60439, USA


                           GOVERNMENT LICENSE

Portions of this material resulted from work developed under a U.S.
Government Contract and are subject to the following license: the
Government is granted for itself and others acting on its behalf a
paid-up, nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform publicly
and display publicly. 

                              DISCLAIMER

This computer code material was prepared, in part, as an account of
work sponsored by an agency of the United States Government.  Neither
the United States Government, nor the University of Chicago, nor any
of their employees, makes any warranty express or implied, or assumes
any legal liability or responsibility for the accuracy, completeness,
or usefulness of any information, apparatus, product, or process
disclosed, or represents that its use would not infringe privately
owned rights.  

 */

#include "mpc_mpi.h"

/************************************************************************/
/* DARRAY IMPLEMENTATION                                                */
/************************************************************************/

int sctk_Type_create_darray( int size, int rank, int ndims, 
			     int *array_of_gsizes, int *array_of_distribs, 
			     int *array_of_dargs, int *array_of_psizes, 
			     int order, MPI_Datatype oldtype, 
			     MPI_Datatype *newtype );


/************************************************************************/
/* SUBARRAY IMPLEMENTATION                                              */
/************************************************************************/

int sctk_Type_create_subarray( int ndims,
			       int *array_of_sizes, 
			       int *array_of_subsizes,
			       int *array_of_starts,
			       int order,
			       MPI_Datatype oldtype, 
			       MPI_Datatype *newtype );




#endif