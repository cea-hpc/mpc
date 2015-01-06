/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr					  # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef sctk_aio_H
#define sctk_aio_H

#include <aio.h>


int sctk_aio_read( struct aiocb * cb );
int sctk_aio_write( struct aiocb * cb );
int sctk_aio_fsync( int op, struct aiocb * cb );
int sctk_aio_error( struct aiocb * cb );
ssize_t sctk_aio_return( struct aiocb * cb );
int sctk_aio_cancel( int fd, struct aiocb * cb );
int sctk_aio_suspend( const struct aiocb * const aiocb_list[], int nitems, const struct timespec * timeout );
int sctk_aio_lio_listio( int mode , struct aiocb * const aiocb_list[], int nitems, struct sigevent *sevp );

int sctk_aio_threads_release();

#endif /* sctk_aio_H */
