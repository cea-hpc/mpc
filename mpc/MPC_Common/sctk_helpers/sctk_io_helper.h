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
/* #   - VALAT Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_IO_HELPER_H
#define SCTK_IO_HELPER_H

/********************************* INCLUDES *********************************/
#include <unistd.h>

/********************************* FUNCTION *********************************/
ssize_t sctk_safe_read(int fd,void * buf,size_t count);
ssize_t sctk_safe_write(int fd,const void * buf,size_t count);
ssize_t sctk_safe_checked_read(int fd,void * buf,size_t count);
ssize_t sctk_safe_checked_write(int fd,const void * buf,size_t count);

#endif /*SCTK_IO_HELPER_H*/
