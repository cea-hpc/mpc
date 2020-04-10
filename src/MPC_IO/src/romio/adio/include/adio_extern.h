/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */

#ifndef ADIO_EXTERN_H_INCLUDED
#define ADIO_EXTERN_H_INCLUDED

#include <romio_priv.h>

/*MPC PATCH*/
#if 0

extern ADIOI_Datarep *ADIOI_Datarep_head;

/* for f2c and c2f conversion */
extern ADIO_File *ADIOI_Ftable;
extern int ADIOI_Ftable_ptr, ADIOI_Ftable_max;
extern ADIO_Request *ADIOI_Reqtable;
extern int ADIOI_Reqtable_ptr, ADIOI_Reqtable_max;
#ifndef HAVE_MPI_INFO
extern MPI_Info *MPIR_Infotable;
extern int MPIR_Infotable_ptr, MPIR_Infotable_max;
#endif
#if defined(ROMIO_XFS) || defined(ROMIO_LUSTRE)
extern int ADIOI_Direct_read, ADIOI_Direct_write;
#endif

extern MPI_Errhandler ADIOI_DFLT_ERR_HANDLER;

extern MPI_Info ADIOI_syshints;

extern MPI_Op ADIO_same_amode;

extern int ADIOI_cb_config_list_keyval;
extern int ADIOI_Flattened_type_keyval;

#else

extern ADIOI_Datarep *mpc_ADIOI_Datarep_head;

/* for f2c and c2f conversion */
extern ADIO_File *mpc_ADIOI_Ftable;
extern int mpc_ADIOI_Ftable_ptr, mpc_ADIOI_Ftable_max;
extern ADIO_Request *mpc_ADIOI_Reqtable;
extern int mpc_ADIOI_Reqtable_ptr, mpc_ADIOI_Reqtable_max;
#ifndef HAVE_MPI_INFO
extern MPI_Info *mpc_MPIR_Infotable;
extern int mpc_MPIR_Infotable_ptr, mpc_MPIR_Infotable_max;
#endif
#if defined(ROMIO_XFS) || defined(ROMIO_LUSTRE)
extern int mpc_ADIOI_Direct_read, mpc_ADIOI_Direct_write;
#endif

extern MPI_Errhandler mpc_ADIOI_DFLT_ERR_HANDLER;

extern MPI_Info mpc_ADIOI_syshints;

extern MPI_Op mpc_ADIO_same_amode;

extern int mpc_ADIOI_cb_config_list_keyval;
extern int mpc_ADIOI_Flattened_type_keyval;
extern int mpc_ADIO_Init_keyval;

#endif


#endif /* ADIO_EXTERN_H_INCLUDED */
