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
/* #   - CAPRA Antoine capra@paratools.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdlib.h>
#include "sctk_shm_mapper.h"

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

char *sctk_mpi_handler_gen_filename(void *option, void *option1);

/*! \brief Send filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

bool sctk_mpi_handler_send_filename(char *filename, void *option,
                                    void *option1);

/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */
char *sctk_mpi_handler_recv_filename(void *option, void *option1);

/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */
void sctk_shm_mpi_handler_free(struct sctk_alloc_mapper_handler_s *mpi_handler);

/*! \brief Generate a handler struct for MPI
 * @param comm the communicator to be used (stored in the handler desc)
 */
struct sctk_alloc_mapper_handler_s *
sctk_shm_mpi_handler_init(mpc_lowcomm_communicator_t comm);
