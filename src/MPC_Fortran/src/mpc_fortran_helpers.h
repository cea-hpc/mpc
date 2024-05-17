#ifndef MPI_FORTRAN_IFACE_H_
#define MPI_FORTRAN_IFACE_H_

#include <mpc_mpi_fortran.h>

/*******************
 * FORTRAN SUPPORT *
 *******************/

/* Now Define Special Fortran pointers */

void **mpi_predef08_bottom(void);
void **mpi_predef08_inplace(void);
void **mpi_predef08_status_ignore(void);
void **mpi_predef08_statuses_ignore(void);

void mpc_predef08_init_inplace_(void *inplace);
void mpc_predef08_init_bottom_(void *bottom);
void mpc_predef08_init_status_ignore_(void *status_ignore);
void mpc_predef08_init_statuses_ignore_(void *statuses_ignore);

void **mpi_predef_bottom(void);
void **mpi_predef_inplace(void);
void **mpi_predef_status_ignore(void);
void **mpi_predef_statuses_ignore(void);

void mpc_predef_init_inplace_(void *inplace);
void mpc_predef_init_bottom_(void *bottom);
void mpc_predef_init_status_ignore_(void *status_ignore);
void mpc_predef_init_statuses_ignore_(void *statuses_ignore);

/* Handle identifier conversion */

void mpc_fortran_comm_delete(MPI_Fint comm);
void mpc_fortran_datatype_delete(MPI_Fint datatype);
void mpc_fortran_group_delete(MPI_Fint group);
void mpc_fortran_request_delete(MPI_Fint request);
void mpc_fortran_win_delete(MPI_Fint win);
void mpc_fortran_info_delete(MPI_Fint info);
void mpc_fortran_errhandler_delete(MPI_Fint errhandler);
void mpc_fortran_op_delete(MPI_Fint op);


#endif /* MPI_FORTRAN_IFACE_H_ */
