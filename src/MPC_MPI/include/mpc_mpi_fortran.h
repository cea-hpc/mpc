#ifndef MPC_FORTRAN_H_
#define MPC_FORTRAN_H_

#include <mpc_mpi.h>


/****************************************
* Fortran MPI_Status related constants *
****************************************/

typedef struct {} MPI_F08_status;

#define MPI_F_STATUS_SIZE          8
#define MPI_F_SOURCE               0
#define MPI_F_TAG                  1
#define MPI_F_ERROR                2

#define MPI_F_STATUS_IGNORE        (MPI_Fint *)0
#define MPI_F_STATUSES_IGNORE      (MPI_Fint *)0

#define MPI_F08_STATUS_IGNORE      (MPI_F08_status *)0
#define MPI_F08_STATUSES_IGNORE    (MPI_F08_status *)0


/*************************************
 *  MPI-2 : Fortran handle conversion *
 **************************************/

MPI_Comm MPI_Comm_f2c(MPI_Fint comm);
MPI_Fint MPI_Comm_c2f(MPI_Comm comm);
MPI_Datatype MPI_Type_f2c(MPI_Fint datatype);
MPI_Fint MPI_Type_c2f(MPI_Datatype datatype);
MPI_Group MPI_Group_f2c(MPI_Fint group);
MPI_Fint MPI_Group_c2f(MPI_Group group);
MPI_Request MPI_Request_f2c(MPI_Fint request);
MPI_Fint MPI_Request_c2f(MPI_Request request);
MPI_Win MPI_Win_f2c(MPI_Fint win);
MPI_Fint MPI_Win_c2f(MPI_Win win);
MPI_Op MPI_Op_f2c(MPI_Fint op);
MPI_Fint MPI_Op_c2f(MPI_Op op);
MPI_Info MPI_Info_f2c(MPI_Fint info);
MPI_Fint MPI_Info_c2f(MPI_Info info);
MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler);
MPI_Session MPI_Session_f2c(MPI_Fint session);
MPI_Fint MPI_Session_c2f(MPI_Session session);

/* This is the PMPI Interface */

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm);
MPI_Fint PMPI_Comm_c2f(MPI_Comm comm);
MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype);
MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype);
MPI_Group PMPI_Group_f2c(MPI_Fint group);
MPI_Fint PMPI_Group_c2f(MPI_Group group);
MPI_Request PMPI_Request_f2c(MPI_Fint request);
MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_Win PMPI_Win_f2c(MPI_Fint win);
MPI_Fint PMPI_Win_c2f(MPI_Win win);
MPI_Op PMPI_Op_f2c(MPI_Fint op);
MPI_Fint PMPI_Op_c2f(MPI_Op op);
MPI_Info PMPI_Info_f2c(MPI_Fint info);
MPI_Fint PMPI_Info_c2f(MPI_Info info);
MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler);
MPI_Session PMPI_Session_f2c(MPI_Fint session);
MPI_Fint PMPI_Session_c2f(MPI_Session session);

#endif /* MPC_FORTRAN_H_ */
