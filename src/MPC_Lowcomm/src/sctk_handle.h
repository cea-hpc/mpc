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
#ifndef MPI_HANDLE_H
#define MPI_HANDLE_H

/************************************************************************/
/* Error Handler                                                        */
/************************************************************************/

typedef void (*sctk_generic_handler)(void *phandle, int *error_code, ...);
typedef int sctk_errhandler_t;

int sctk_errhandler_register(sctk_generic_handler eh, sctk_errhandler_t *errh);
int sctk_errhandler_register_on_slot(sctk_generic_handler eh,
                                     sctk_errhandler_t slot);
sctk_generic_handler sctk_errhandler_resolve(sctk_errhandler_t errh);
int sctk_errhandler_free(sctk_errhandler_t errh);

/************************************************************************/
/* MPI Handles                                                          */
/************************************************************************/

#define SCTK_BOOKED_HANDLES (10000)

typedef int sctk_handle;

struct sctk_handle_context {
  sctk_handle id;
  sctk_errhandler_t handler;
};

typedef enum {
  SCTK_HANDLE_GLOBAL,
  SCTK_HANDLE_COMM,
  SCTK_HANDLE_DATATYPE,
  SCTK_HANDLE_WIN,
  SCTK_HANDLE_FILE,
  SCTK_HANDLE_ERRHANDLER,
  SCTK_HANDLE_OP,
  SCTK_HANDLE_GROUP,
  SCTK_HANDLE_REQUEST,
  SCTK_HANDLE_MESSAGE,
  SCTK_HANDLE_INFO
} sctk_handle_type;

struct sctk_handle_context *sctk_handle_context_new(sctk_handle id);
int sctk_handle_context_release(struct sctk_handle_context *hctx);
sctk_handle sctk_handle_new_from_id(int previous_id, sctk_handle_type type);
sctk_handle sctk_handle_new(sctk_handle_type type);
int sctk_handle_free(sctk_handle id, sctk_handle_type type);

struct sctk_handle_context *sctk_handle_context(sctk_handle id,
                                                sctk_handle_type type);

sctk_errhandler_t sctk_handle_get_errhandler(sctk_handle id,
                                             sctk_handle_type type);
int sctk_handle_set_errhandler(sctk_handle id, sctk_handle_type type,
                               sctk_errhandler_t errh);

#endif /* MPI_HANDLE_H */
