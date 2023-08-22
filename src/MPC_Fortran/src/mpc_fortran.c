#include <mpc_mpi.h>
#include <string.h>
#include <mpc_config.h>
#include <sctk_alloc.h>
#include <mpc_keywords.h>
#include <signal.h>
#include "mpc_fortran_helpers.h"

#ifdef MPC_MPIIO_ENABLED
#include <mpio.h>
#endif

static inline char *char_fortran_to_c(char *buf, int size, char **free_ptr)
{
	char *   tmp;
	long int i;

	tmp = sctk_malloc(size + 1);

	*free_ptr = tmp;

	for(i = 0; i < size; i++)
	{
		tmp[i] = buf[i];
	}
	tmp[i] = '\0';

	/* Trim */

	while(*tmp == ' ')
	{
		tmp++;
	}

	size_t len = strlen(tmp);

	char *begin = tmp;

	while( (tmp[len - 1] == ' ') && (&tmp[len] != begin) )
	{
		tmp[len - 1] = '\0';
		len--;
	}

	return tmp;
}

static inline void char_c_to_fortran(char *buf, size_t size)
{
	size_t i;

	for(i = strlen(buf); i < size; i++)
	{
		buf[i] = ' ';
	}
}

#if defined(USE_CHAR_MIXED)
	#define CHAR_END(thename)
	#define CHAR_MIXED(thename)    long int thename,
#else
	#define CHAR_END(thename)      , long int thename
	#define CHAR_MIXED(thename)
#endif


static inline int buffer_is_bottom(void *buffer)
{
	return (buffer == *mpi_predef_bottom() ) ||
	       (buffer == *mpi_predef08_bottom() );
}

static inline int buffer_is_mpiinplace(void *buffer)
{
	return (buffer == *mpi_predef_inplace() ) ||
	       (buffer == *mpi_predef08_inplace() );
}

#pragma weak mpi_abort_ = pmpi_abort_
void pmpi_abort_(MPI_Fint *comm, int *errorcode, int *ierror)
{
/* MPI_Abort */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Abort(c_comm, *errorcode);
}

#pragma weak mpi_abort__ = pmpi_abort__
void pmpi_abort__(MPI_Fint *comm, int *errorcode, int *ierror)
{
/* MPI_Abort */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Abort(c_comm, *errorcode);
}

#pragma weak mpi_accumulate_ = pmpi_accumulate_
void pmpi_accumulate_(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, int *ierror)
{
/* MPI_Accumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Accumulate(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win);
}

#pragma weak mpi_accumulate__ = pmpi_accumulate__
void pmpi_accumulate__(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, int *ierror)
{
/* MPI_Accumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Accumulate(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win);
}

#pragma weak mpi_add_error_class_ = pmpi_add_error_class_
void pmpi_add_error_class_(int *errorclass, int *ierror)
{
/* MPI_Add_error_class */

	*ierror = MPI_Add_error_class(errorclass);
}

#pragma weak mpi_add_error_class__ = pmpi_add_error_class__
void pmpi_add_error_class__(int *errorclass, int *ierror)
{
/* MPI_Add_error_class */

	*ierror = MPI_Add_error_class(errorclass);
}

#pragma weak mpi_add_error_code_ = pmpi_add_error_code_
void pmpi_add_error_code_(int *errorclass, int *errorcode, int *ierror)
{
/* MPI_Add_error_code */

	*ierror = MPI_Add_error_code(*errorclass, errorcode);
}

#pragma weak mpi_add_error_code__ = pmpi_add_error_code__
void pmpi_add_error_code__(int *errorclass, int *errorcode, int *ierror)
{
/* MPI_Add_error_code */

	*ierror = MPI_Add_error_code(*errorclass, errorcode);
}

#pragma weak mpi_add_error_string_ = pmpi_add_error_string_
void pmpi_add_error_string_(int *errorcode, const char *string CHAR_MIXED(size_string), int *ierror CHAR_END(size_string) )
{
/* MPI_Add_error_string */
	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)string, size_string, &ptr_string);

	*ierror = MPI_Add_error_string(*errorcode, tmp_string);
	sctk_free(ptr_string);
}

#pragma weak mpi_add_error_string__ = pmpi_add_error_string__
void pmpi_add_error_string__(int *errorcode, const char *string CHAR_MIXED(size_string), int *ierror CHAR_END(size_string) )
{
/* MPI_Add_error_string */
	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)string, size_string, &ptr_string);

	*ierror = MPI_Add_error_string(*errorcode, tmp_string);
	sctk_free(ptr_string);
}

#pragma weak mpi_aint_add_ = pmpi_aint_add_
MPI_Aint pmpi_aint_add_(MPI_Aint *base, MPI_Aint *disp)
{
/* MPI_Aint_add */

	return MPI_Aint_add(*base, *disp);
}

#pragma weak mpi_aint_add__ = pmpi_aint_add__
MPI_Aint pmpi_aint_add__(MPI_Aint *base, MPI_Aint *disp)
{
/* MPI_Aint_add */

	return MPI_Aint_add(*base, *disp);
}

#pragma weak mpi_aint_diff_ = pmpi_aint_diff_
MPI_Aint pmpi_aint_diff_(MPI_Aint *addr1, MPI_Aint *addr2)
{
/* MPI_Aint_diff */

	return MPI_Aint_diff(*addr1, *addr2);
}

#pragma weak mpi_aint_diff__ = pmpi_aint_diff__
MPI_Aint pmpi_aint_diff__(MPI_Aint *addr1, MPI_Aint *addr2)
{
/* MPI_Aint_diff */

	return MPI_Aint_diff(*addr1, *addr2);
}

#pragma weak mpi_allgather_ = pmpi_allgather_
void pmpi_allgather_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Allgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_allgather__ = pmpi_allgather__
void pmpi_allgather__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Allgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_allgather_init_ = pmpi_allgather_init_
void pmpi_allgather_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Allgather_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Allgather_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_allgather_init__ = pmpi_allgather_init__
void pmpi_allgather_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Allgather_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Allgather_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_allgatherv_ = pmpi_allgatherv_
void pmpi_allgatherv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Allgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm);
}

#pragma weak mpi_allgatherv__ = pmpi_allgatherv__
void pmpi_allgatherv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Allgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm);
}

#pragma weak mpi_allgatherv_init_ = pmpi_allgatherv_init_
void pmpi_allgatherv_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Allgatherv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Allgatherv_init(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_allgatherv_init__ = pmpi_allgatherv_init__
void pmpi_allgatherv_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Allgatherv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Allgatherv_init(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_alloc_mem_ = pmpi_alloc_mem_
void pmpi_alloc_mem_(MPI_Aint *size, MPI_Fint *info, void *baseptr, int *ierror)
{
/* MPI_Alloc_mem */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Alloc_mem(*size, c_info, baseptr);
}

#pragma weak mpi_alloc_mem__ = pmpi_alloc_mem__
void pmpi_alloc_mem__(MPI_Aint *size, MPI_Fint *info, void *baseptr, int *ierror)
{
/* MPI_Alloc_mem */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Alloc_mem(*size, c_info, baseptr);
}

#pragma weak mpi_allreduce_ = pmpi_allreduce_
void pmpi_allreduce_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Allreduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Allreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

#pragma weak mpi_allreduce__ = pmpi_allreduce__
void pmpi_allreduce__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Allreduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Allreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

#pragma weak mpi_allreduce_init_ = pmpi_allreduce_init_
void pmpi_allreduce_init_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Allreduce_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Allreduce_init(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_allreduce_init__ = pmpi_allreduce_init__
void pmpi_allreduce_init__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Allreduce_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Allreduce_init(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_alltoall_ = pmpi_alltoall_
void pmpi_alltoall_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Alltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_alltoall__ = pmpi_alltoall__
void pmpi_alltoall__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Alltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_alltoall_init_ = pmpi_alltoall_init_
void pmpi_alltoall_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Alltoall_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Alltoall_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_alltoall_init__ = pmpi_alltoall_init__
void pmpi_alltoall_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Alltoall_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Alltoall_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_alltoallv_ = pmpi_alltoallv_
void pmpi_alltoallv_(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Alltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm);
}

#pragma weak mpi_alltoallv__ = pmpi_alltoallv__
void pmpi_alltoallv__(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Alltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm);
}

#pragma weak mpi_alltoallv_init_ = pmpi_alltoallv_init_
void pmpi_alltoallv_init_(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Alltoallv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Alltoallv_init(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_alltoallv_init__ = pmpi_alltoallv_init__
void pmpi_alltoallv_init__(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Alltoallv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Alltoallv_init(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_alltoallw_ = pmpi_alltoallw_
void pmpi_alltoallw_(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, int *ierror)
{
/* MPI_Alltoallw */
	int alltoallwlen = 0;

	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}


	*ierror = MPI_Alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_alltoallw__ = pmpi_alltoallw__
void pmpi_alltoallw__(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, int *ierror)
{
/* MPI_Alltoallw */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	int alltoallwlen = 0;
	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	*ierror = MPI_Alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_alltoallw_init_ = pmpi_alltoallw_init_
void pmpi_alltoallw_init_(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Alltoallw_init */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Request c_request;

	*ierror  = MPI_Alltoallw_init(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_alltoallw_init__ = pmpi_alltoallw_init__
void pmpi_alltoallw_init__(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Alltoallw_init */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Request c_request;

	*ierror  = MPI_Alltoallw_init(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_attr_delete_ = pmpi_attr_delete_
void pmpi_attr_delete_(MPI_Fint *comm, int *keyval, int *ierror)
{
/* MPI_Attr_delete */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Attr_delete(c_comm, *keyval);
}

#pragma weak mpi_attr_delete__ = pmpi_attr_delete__
void pmpi_attr_delete__(MPI_Fint *comm, int *keyval, int *ierror)
{
/* MPI_Attr_delete */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Attr_delete(c_comm, *keyval);
}

#pragma weak mpi_attr_get_ = pmpi_attr_get_
void pmpi_attr_get_(MPI_Fint *comm, int *keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Attr_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	void * attr = NULL;

	*ierror = MPI_Attr_get(c_comm, *keyval, &attr, flag);

	if(*flag && (*ierror == MPI_SUCCESS))
	{
		*((int*)attribute_val) = ((int)((long)attr));
	}
}

#pragma weak mpi_attr_get__ = pmpi_attr_get__
void pmpi_attr_get__(MPI_Fint *comm, int *keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Attr_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	void * attr = NULL;

	*ierror = MPI_Attr_get(c_comm, *keyval, &attr, flag);

	if(*flag && (*ierror == MPI_SUCCESS))
	{
		*((int*)attribute_val) = ((int)((long)attr));
	}
}

#pragma weak mpi_attr_put_ = pmpi_attr_put_
void pmpi_attr_put_(MPI_Fint *comm, int *keyval, void *attribute_val, int *ierror)
{
/* MPI_Attr_put */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Attr_put(c_comm, *keyval, attribute_val);
}

#pragma weak mpi_attr_put__ = pmpi_attr_put__
void pmpi_attr_put__(MPI_Fint *comm, int *keyval, void *attribute_val, int *ierror)
{
/* MPI_Attr_put */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Attr_put(c_comm, *keyval, attribute_val);
}

#pragma weak mpi_barrier_ = pmpi_barrier_
void pmpi_barrier_(MPI_Fint *comm, int *ierror)
{
/* MPI_Barrier */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Barrier(c_comm);
}

#pragma weak mpi_barrier__ = pmpi_barrier__
void pmpi_barrier__(MPI_Fint *comm, int *ierror)
{
/* MPI_Barrier */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Barrier(c_comm);
}

#pragma weak mpi_barrier_init_ = pmpi_barrier_init_
void pmpi_barrier_init_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Barrier_init */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Request c_request;

	*ierror  = MPI_Barrier_init(c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_barrier_init__ = pmpi_barrier_init__
void pmpi_barrier_init__(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Barrier_init */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Request c_request;

	*ierror  = MPI_Barrier_init(c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_bcast_ = pmpi_bcast_
void pmpi_bcast_(void *buffer, int *count, MPI_Fint *datatype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Bcast */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Bcast(buffer, *count, c_datatype, *root, c_comm);
}

#pragma weak mpi_bcast__ = pmpi_bcast__
void pmpi_bcast__(void *buffer, int *count, MPI_Fint *datatype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Bcast */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Bcast(buffer, *count, c_datatype, *root, c_comm);
}

#pragma weak mpi_bcast_init_ = pmpi_bcast_init_
void pmpi_bcast_init_(void *buffer, int *count, MPI_Fint *datatype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Bcast_init */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Bcast_init(buffer, *count, c_datatype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_bcast_init__ = pmpi_bcast_init__
void pmpi_bcast_init__(void *buffer, int *count, MPI_Fint *datatype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Bcast_init */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Bcast_init(buffer, *count, c_datatype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_bsend_ = pmpi_bsend_
void pmpi_bsend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Bsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Bsend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_bsend__ = pmpi_bsend__
void pmpi_bsend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Bsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Bsend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_bsend_init_ = pmpi_bsend_init_
void pmpi_bsend_init_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Bsend_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Bsend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_bsend_init__ = pmpi_bsend_init__
void pmpi_bsend_init__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Bsend_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Bsend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_buffer_attach_ = pmpi_buffer_attach_
void pmpi_buffer_attach_(void *buffer, int *size, int *ierror)
{
/* MPI_Buffer_attach */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}

	*ierror = MPI_Buffer_attach(buffer, *size);
}

#pragma weak mpi_buffer_attach__ = pmpi_buffer_attach__
void pmpi_buffer_attach__(void *buffer, int *size, int *ierror)
{
/* MPI_Buffer_attach */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}

	*ierror = MPI_Buffer_attach(buffer, *size);
}

#pragma weak mpi_buffer_detach_ = pmpi_buffer_detach_
void pmpi_buffer_detach_(void *buffer_addr, int *size, int *ierror)
{
/* MPI_Buffer_detach */

	*ierror = MPI_Buffer_detach(buffer_addr, size);
}

#pragma weak mpi_buffer_detach__ = pmpi_buffer_detach__
void pmpi_buffer_detach__(void *buffer_addr, int *size, int *ierror)
{
/* MPI_Buffer_detach */

	*ierror = MPI_Buffer_detach(buffer_addr, size);
}

#pragma weak mpi_cancel_ = pmpi_cancel_
void pmpi_cancel_(MPI_Fint *request, int *ierror)
{
/* MPI_Cancel */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror = MPI_Cancel(&c_request);
}

#pragma weak mpi_cancel__ = pmpi_cancel__
void pmpi_cancel__(MPI_Fint *request, int *ierror)
{
/* MPI_Cancel */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror = MPI_Cancel(&c_request);
}

#pragma weak mpi_cart_coords_ = pmpi_cart_coords_
void pmpi_cart_coords_(MPI_Fint *comm, int *rank, int *maxdims, int coords[], int *ierror)
{
/* MPI_Cart_coords */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_coords(c_comm, *rank, *maxdims, coords);
}

#pragma weak mpi_cart_coords__ = pmpi_cart_coords__
void pmpi_cart_coords__(MPI_Fint *comm, int *rank, int *maxdims, int coords[], int *ierror)
{
/* MPI_Cart_coords */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_coords(c_comm, *rank, *maxdims, coords);
}

#pragma weak mpi_cart_create_ = pmpi_cart_create_
void pmpi_cart_create_(MPI_Fint *comm_old, int *ndims, const int dims[], const int periods[], int *reorder, MPI_Fint *comm_cart, int *ierror)
{
/* MPI_Cart_create */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Comm c_comm_cart;

	*ierror    = MPI_Cart_create(c_comm_old, *ndims, dims, periods, *reorder, &c_comm_cart);
	*comm_cart = PMPI_Comm_c2f(c_comm_cart);
}

#pragma weak mpi_cart_create__ = pmpi_cart_create__
void pmpi_cart_create__(MPI_Fint *comm_old, int *ndims, const int dims[], const int periods[], int *reorder, MPI_Fint *comm_cart, int *ierror)
{
/* MPI_Cart_create */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Comm c_comm_cart;

	*ierror    = MPI_Cart_create(c_comm_old, *ndims, dims, periods, *reorder, &c_comm_cart);
	*comm_cart = PMPI_Comm_c2f(c_comm_cart);
}

#pragma weak mpi_cart_get_ = pmpi_cart_get_
void pmpi_cart_get_(MPI_Fint *comm, int *maxdims, int dims[], int periods[], int coords[], int *ierror)
{
/* MPI_Cart_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_get(c_comm, *maxdims, dims, periods, coords);
}

#pragma weak mpi_cart_get__ = pmpi_cart_get__
void pmpi_cart_get__(MPI_Fint *comm, int *maxdims, int dims[], int periods[], int coords[], int *ierror)
{
/* MPI_Cart_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_get(c_comm, *maxdims, dims, periods, coords);
}

#pragma weak mpi_cart_map_ = pmpi_cart_map_
void pmpi_cart_map_(MPI_Fint *comm, int *ndims, const int dims[], const int periods[], int *newrank, int *ierror)
{
/* MPI_Cart_map */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_map(c_comm, *ndims, dims, periods, newrank);
}

#pragma weak mpi_cart_map__ = pmpi_cart_map__
void pmpi_cart_map__(MPI_Fint *comm, int *ndims, const int dims[], const int periods[], int *newrank, int *ierror)
{
/* MPI_Cart_map */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_map(c_comm, *ndims, dims, periods, newrank);
}

#pragma weak mpi_cart_rank_ = pmpi_cart_rank_
void pmpi_cart_rank_(MPI_Fint *comm, const int coords[], int *rank, int *ierror)
{
/* MPI_Cart_rank */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_rank(c_comm, coords, rank);
}

#pragma weak mpi_cart_rank__ = pmpi_cart_rank__
void pmpi_cart_rank__(MPI_Fint *comm, const int coords[], int *rank, int *ierror)
{
/* MPI_Cart_rank */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_rank(c_comm, coords, rank);
}

#pragma weak mpi_cart_shift_ = pmpi_cart_shift_
void pmpi_cart_shift_(MPI_Fint *comm, int *direction, int *disp, int *rank_source, int *rank_dest, int *ierror)
{
/* MPI_Cart_shift */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_shift(c_comm, *direction, *disp, rank_source, rank_dest);
}

#pragma weak mpi_cart_shift__ = pmpi_cart_shift__
void pmpi_cart_shift__(MPI_Fint *comm, int *direction, int *disp, int *rank_source, int *rank_dest, int *ierror)
{
/* MPI_Cart_shift */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cart_shift(c_comm, *direction, *disp, rank_source, rank_dest);
}

#pragma weak mpi_cart_sub_ = pmpi_cart_sub_
void pmpi_cart_sub_(MPI_Fint *comm, const int remain_dims[], MPI_Fint *newcomm, int *ierror)
{
/* MPI_Cart_sub */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Cart_sub(c_comm, remain_dims, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_cart_sub__ = pmpi_cart_sub__
void pmpi_cart_sub__(MPI_Fint *comm, const int remain_dims[], MPI_Fint *newcomm, int *ierror)
{
/* MPI_Cart_sub */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Cart_sub(c_comm, remain_dims, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_cartdim_get_ = pmpi_cartdim_get_
void pmpi_cartdim_get_(MPI_Fint *comm, int *ndims, int *ierror)
{
/* MPI_Cartdim_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cartdim_get(c_comm, ndims);
}

#pragma weak mpi_cartdim_get__ = pmpi_cartdim_get__
void pmpi_cartdim_get__(MPI_Fint *comm, int *ndims, int *ierror)
{
/* MPI_Cartdim_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Cartdim_get(c_comm, ndims);
}

#pragma weak mpi_close_port_ = pmpi_close_port_
void pmpi_close_port_(const char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_port_name) )
{
/* MPI_Close_port */
	char *tmp_port_name = NULL, *ptr_port_name = NULL;

	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);

	*ierror = MPI_Close_port(tmp_port_name);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_close_port__ = pmpi_close_port__
void pmpi_close_port__(const char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_port_name) )
{
/* MPI_Close_port */
	char *tmp_port_name = NULL, *ptr_port_name = NULL;

	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);

	*ierror = MPI_Close_port(tmp_port_name);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_comm_accept_ = pmpi_comm_accept_
void pmpi_comm_accept_(const char *port_name CHAR_MIXED(size_port_name), MPI_Fint *info, int *root, MPI_Fint *comm, MPI_Fint *newcomm, int *ierror CHAR_END(size_port_name) )
{
/* MPI_Comm_accept */
	char *tmp_port_name = NULL, *ptr_port_name = NULL;

	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_accept(tmp_port_name, c_info, *root, c_comm, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_comm_accept__ = pmpi_comm_accept__
void pmpi_comm_accept__(const char *port_name CHAR_MIXED(size_port_name), MPI_Fint *info, int *root, MPI_Fint *comm, MPI_Fint *newcomm, int *ierror CHAR_END(size_port_name) )
{
/* MPI_Comm_accept */
	char *tmp_port_name = NULL, *ptr_port_name = NULL;

	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_accept(tmp_port_name, c_info, *root, c_comm, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_comm_call_errhandler_ = pmpi_comm_call_errhandler_
void pmpi_comm_call_errhandler_(MPI_Fint *comm, int *errorcode, int *ierror)
{
/* MPI_Comm_call_errhandler */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_call_errhandler(c_comm, *errorcode);
}

#pragma weak mpi_comm_call_errhandler__ = pmpi_comm_call_errhandler__
void pmpi_comm_call_errhandler__(MPI_Fint *comm, int *errorcode, int *ierror)
{
/* MPI_Comm_call_errhandler */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_call_errhandler(c_comm, *errorcode);
}

#pragma weak mpi_comm_compare_ = pmpi_comm_compare_
void pmpi_comm_compare_(MPI_Fint *comm1, MPI_Fint *comm2, int *result, int *ierror)
{
/* MPI_Comm_compare */
	MPI_Comm c_comm1 = PMPI_Comm_f2c(*comm1);
	MPI_Comm c_comm2 = PMPI_Comm_f2c(*comm2);

	*ierror = MPI_Comm_compare(c_comm1, c_comm2, result);
}

#pragma weak mpi_comm_compare__ = pmpi_comm_compare__
void pmpi_comm_compare__(MPI_Fint *comm1, MPI_Fint *comm2, int *result, int *ierror)
{
/* MPI_Comm_compare */
	MPI_Comm c_comm1 = PMPI_Comm_f2c(*comm1);
	MPI_Comm c_comm2 = PMPI_Comm_f2c(*comm2);

	*ierror = MPI_Comm_compare(c_comm1, c_comm2, result);
}

#pragma weak mpi_comm_connect_ = pmpi_comm_connect_
void pmpi_comm_connect_(const char *port_name CHAR_MIXED(size_port_name), MPI_Fint *info, int *root, MPI_Fint *comm, MPI_Fint *newcomm, int *ierror CHAR_END(size_port_name) )
{
/* MPI_Comm_connect */
	char *tmp_port_name = NULL, *ptr_port_name = NULL;

	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_connect(tmp_port_name, c_info, *root, c_comm, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_comm_connect__ = pmpi_comm_connect__
void pmpi_comm_connect__(const char *port_name CHAR_MIXED(size_port_name), MPI_Fint *info, int *root, MPI_Fint *comm, MPI_Fint *newcomm, int *ierror CHAR_END(size_port_name) )
{
/* MPI_Comm_connect */
	char *tmp_port_name = NULL, *ptr_port_name = NULL;

	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_connect(tmp_port_name, c_info, *root, c_comm, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_comm_create_ = pmpi_comm_create_
void pmpi_comm_create_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_create */
	MPI_Comm  c_comm  = PMPI_Comm_f2c(*comm);
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Comm  c_newcomm;

	*ierror  = MPI_Comm_create(c_comm, c_group, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_create__ = pmpi_comm_create__
void pmpi_comm_create__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_create */
	MPI_Comm  c_comm  = PMPI_Comm_f2c(*comm);
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Comm  c_newcomm;

	*ierror  = MPI_Comm_create(c_comm, c_group, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_errhandler_get_ = pmpi_errhandler_get_
void pmpi_errhandler_get_(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_create_errhandler */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Errhandler_get(c_comm, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_errhandler_get__ = pmpi_errhandler_get__
void pmpi_errhandler_get__(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
	pmpi_errhandler_get_(comm, errhandler, ierror);
}

#pragma weak mpi_comm_create_errhandler_ = pmpi_comm_create_errhandler_
void pmpi_comm_create_errhandler_(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_create_errhandler */
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Comm_create_errhandler(comm_errhandler_fn, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_comm_create_errhandler__ = pmpi_comm_create_errhandler__
void pmpi_comm_create_errhandler__(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_create_errhandler */
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Comm_create_errhandler(comm_errhandler_fn, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_comm_create_group_ = pmpi_comm_create_group_
void pmpi_comm_create_group_(MPI_Fint *comm, MPI_Fint *group, int *tag, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_create_group */
	MPI_Comm  c_comm  = PMPI_Comm_f2c(*comm);
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Comm  c_newcomm;

	*ierror  = MPI_Comm_create_group(c_comm, c_group, *tag, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_create_group__ = pmpi_comm_create_group__
void pmpi_comm_create_group__(MPI_Fint *comm, MPI_Fint *group, int *tag, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_create_group */
	MPI_Comm  c_comm  = PMPI_Comm_f2c(*comm);
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Comm  c_newcomm;

	*ierror  = MPI_Comm_create_group(c_comm, c_group, *tag, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_create_keyval_ = pmpi_comm_create_keyval_
void pmpi_comm_create_keyval_(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state, int *ierror)
{
/* MPI_Comm_create_keyval */

    *comm_keyval = -1; /**< Denotes fortran functions */
	*ierror = MPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
}

#pragma weak mpi_comm_create_keyval__ = pmpi_comm_create_keyval__
void pmpi_comm_create_keyval__(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state, int *ierror)
{
/* MPI_Comm_create_keyval */

    *comm_keyval = -1; /**< Denotes fortran functions */
	*ierror = MPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
}

#pragma weak mpi_comm_delete_attr_ = pmpi_comm_delete_attr_
void pmpi_comm_delete_attr_(MPI_Fint *comm, int *comm_keyval, int *ierror)
{
/* MPI_Comm_delete_attr */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_delete_attr(c_comm, *comm_keyval);
}

#pragma weak mpi_comm_delete_attr__ = pmpi_comm_delete_attr__
void pmpi_comm_delete_attr__(MPI_Fint *comm, int *comm_keyval, int *ierror)
{
/* MPI_Comm_delete_attr */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_delete_attr(c_comm, *comm_keyval);
}

#pragma weak mpi_comm_disconnect_ = pmpi_comm_disconnect_
void pmpi_comm_disconnect_(MPI_Fint *comm, int *ierror)
{
/* MPI_Comm_disconnect */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_disconnect(&c_comm);
	*comm   = PMPI_Comm_c2f(c_comm);
}

#pragma weak mpi_comm_disconnect__ = pmpi_comm_disconnect__
void pmpi_comm_disconnect__(MPI_Fint *comm, int *ierror)
{
/* MPI_Comm_disconnect */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_disconnect(&c_comm);
	*comm   = PMPI_Comm_c2f(c_comm);
}

#pragma weak mpi_comm_dup_ = pmpi_comm_dup_
void pmpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_dup */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_dup(c_comm, &c_newcomm);
    if( *ierror == MPI_SUCCESS ) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
}

#pragma weak mpi_comm_dup__ = pmpi_comm_dup__
void pmpi_comm_dup__(MPI_Fint *comm, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_dup */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_dup(c_comm, &c_newcomm);
    if( *ierror == MPI_SUCCESS ) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
}

#pragma weak mpi_comm_dup_with_info_ = pmpi_comm_dup_with_info_
void pmpi_comm_dup_with_info_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_dup_with_info */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_dup_with_info(c_comm, c_info, &c_newcomm);
    if( *ierror == MPI_SUCCESS ) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
}

#pragma weak mpi_comm_dup_with_info__ = pmpi_comm_dup_with_info__
void pmpi_comm_dup_with_info__(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_dup_with_info */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_dup_with_info(c_comm, c_info, &c_newcomm);
    if( *ierror == MPI_SUCCESS ) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
}

#pragma weak mpi_comm_free_ = pmpi_comm_free_
void pmpi_comm_free_(MPI_Fint *comm, int *ierror)
{
/* MPI_Comm_free */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_free(&c_comm);
	*comm   = PMPI_Comm_c2f(c_comm);
}

#pragma weak mpi_comm_free__ = pmpi_comm_free__
void pmpi_comm_free__(MPI_Fint *comm, int *ierror)
{
/* MPI_Comm_free */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_free(&c_comm);
	*comm   = PMPI_Comm_c2f(c_comm);
}

#pragma weak mpi_comm_free_keyval_ = pmpi_comm_free_keyval_
void pmpi_comm_free_keyval_(int *comm_keyval, int *ierror)
{
/* MPI_Comm_free_keyval */

	*ierror = MPI_Comm_free_keyval(comm_keyval);
}

#pragma weak mpi_comm_free_keyval__ = pmpi_comm_free_keyval__
void pmpi_comm_free_keyval__(int *comm_keyval, int *ierror)
{
/* MPI_Comm_free_keyval */

	*ierror = MPI_Comm_free_keyval(comm_keyval);
}

#pragma weak mpi_comm_get_attr_ = pmpi_comm_get_attr_
void pmpi_comm_get_attr_(MPI_Fint *comm, int *comm_keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Comm_get_attr */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_get_attr(c_comm, *comm_keyval, attribute_val, flag);
}

#pragma weak mpi_comm_get_attr__ = pmpi_comm_get_attr__
void pmpi_comm_get_attr__(MPI_Fint *comm, int *comm_keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Comm_get_attr */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_get_attr(c_comm, *comm_keyval, attribute_val, flag);
}

#pragma weak mpi_comm_get_errhandler_ = pmpi_comm_get_errhandler_
void pmpi_comm_get_errhandler_(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_get_errhandler */
	MPI_Comm       c_comm = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Comm_get_errhandler(c_comm, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_comm_get_errhandler__ = pmpi_comm_get_errhandler__
void pmpi_comm_get_errhandler__(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_get_errhandler */
	MPI_Comm       c_comm = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Comm_get_errhandler(c_comm, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_comm_get_info_ = pmpi_comm_get_info_
void pmpi_comm_get_info_(MPI_Fint *comm, MPI_Fint *info_used, int *ierror)
{
/* MPI_Comm_get_info */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info_used;

	*ierror    = MPI_Comm_get_info(c_comm, &c_info_used);
	*info_used = PMPI_Info_c2f(c_info_used);
}

#pragma weak mpi_comm_get_info__ = pmpi_comm_get_info__
void pmpi_comm_get_info__(MPI_Fint *comm, MPI_Fint *info_used, int *ierror)
{
/* MPI_Comm_get_info */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info_used;

	*ierror    = MPI_Comm_get_info(c_comm, &c_info_used);
	*info_used = PMPI_Info_c2f(c_info_used);
}

#pragma weak mpi_comm_get_name_ = pmpi_comm_get_name_
void pmpi_comm_get_name_(MPI_Fint *comm, char *comm_name CHAR_MIXED(size_comm_name), int *resultlen, int *ierror CHAR_END(size_comm_name) )
{
/* MPI_Comm_get_name */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_get_name(c_comm, comm_name, resultlen);
	char_c_to_fortran(comm_name, size_comm_name);
}

#pragma weak mpi_comm_get_name__ = pmpi_comm_get_name__
void pmpi_comm_get_name__(MPI_Fint *comm, char *comm_name CHAR_MIXED(size_comm_name), int *resultlen, int *ierror CHAR_END(size_comm_name) )
{
/* MPI_Comm_get_name */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_get_name(c_comm, comm_name, resultlen);
	char_c_to_fortran(comm_name, size_comm_name);
}

#pragma weak mpi_comm_get_parent_ = pmpi_comm_get_parent_
void pmpi_comm_get_parent_(MPI_Fint *parent, int *ierror)
{
/* MPI_Comm_get_parent */
	MPI_Comm c_parent;

	*ierror = MPI_Comm_get_parent(&c_parent);
	*parent = PMPI_Comm_c2f(c_parent);
}

#pragma weak mpi_comm_get_parent__ = pmpi_comm_get_parent__
void pmpi_comm_get_parent__(MPI_Fint *parent, int *ierror)
{
/* MPI_Comm_get_parent */
	MPI_Comm c_parent;

	*ierror = MPI_Comm_get_parent(&c_parent);
	*parent = PMPI_Comm_c2f(c_parent);
}

#pragma weak mpi_comm_group_ = pmpi_comm_group_
void pmpi_comm_group_(MPI_Fint *comm, MPI_Fint *group, int *ierror)
{
/* MPI_Comm_group */
	MPI_Comm  c_comm = PMPI_Comm_f2c(*comm);
	MPI_Group c_group;

	*ierror = MPI_Comm_group(c_comm, &c_group);
    if(*ierror == MPI_SUCCESS) {
        *group  = PMPI_Group_c2f(c_group);
    }
}

#pragma weak mpi_comm_group__ = pmpi_comm_group__
void pmpi_comm_group__(MPI_Fint *comm, MPI_Fint *group, int *ierror)
{
/* MPI_Comm_group */
	MPI_Comm  c_comm = PMPI_Comm_f2c(*comm);
	MPI_Group c_group;

	*ierror = MPI_Comm_group(c_comm, &c_group);
    if(*ierror == MPI_SUCCESS) {
        *group  = PMPI_Group_c2f(c_group);
    }
}

#pragma weak mpi_comm_idup_ = pmpi_comm_idup_
void pmpi_comm_idup_(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, int *ierror)
{
/* MPI_Comm_idup */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm    c_newcomm;
	MPI_Request c_request;

    *ierror  = MPI_Comm_idup(c_comm, &c_newcomm, &c_request);
    if(*ierror == MPI_SUCCESS) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
        *request = PMPI_Request_c2f(c_request);
    }
}

#pragma weak mpi_comm_idup__ = pmpi_comm_idup__
void pmpi_comm_idup__(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, int *ierror)
{
/* MPI_Comm_idup */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm    c_newcomm;
	MPI_Request c_request;

	*ierror  = MPI_Comm_idup(c_comm, &c_newcomm, &c_request);
    if(*ierror == MPI_SUCCESS) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
        *request = PMPI_Request_c2f(c_request);
    }
}

#pragma weak mpi_comm_idup_with_info_ = pmpi_comm_idup_with_info_
void pmpi_comm_idup_with_info_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *request, int *ierror)
{
/* MPI_Comm_idup_with_info */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Comm    c_newcomm;
	MPI_Request c_request;

	*ierror  = MPI_Comm_idup_with_info(c_comm, c_info, &c_newcomm, &c_request);
    if(*ierror == MPI_SUCCESS) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
        *request = PMPI_Request_c2f(c_request);
    }
}

#pragma weak mpi_comm_idup_with_info__ = pmpi_comm_idup_with_info__
void pmpi_comm_idup_with_info__(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *request, int *ierror)
{
/* MPI_Comm_idup_with_info */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Comm    c_newcomm;
	MPI_Request c_request;

	*ierror  = MPI_Comm_idup_with_info(c_comm, c_info, &c_newcomm, &c_request);
    if(*ierror == MPI_SUCCESS) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
        *request = PMPI_Request_c2f(c_request);
    }
}

#pragma weak mpi_comm_join_ = pmpi_comm_join_
void pmpi_comm_join_(int *fd, MPI_Fint *intercomm, int *ierror)
{
/* MPI_Comm_join */
	MPI_Comm c_intercomm;

    *ierror    = MPI_Comm_join(*fd, &c_intercomm);
    if( *ierror == MPI_SUCCESS) {
        *intercomm = PMPI_Comm_c2f(c_intercomm);
    }
}

#pragma weak mpi_comm_join__ = pmpi_comm_join__
void pmpi_comm_join__(int *fd, MPI_Fint *intercomm, int *ierror)
{
/* MPI_Comm_join */
	MPI_Comm c_intercomm;

	*ierror    = MPI_Comm_join(*fd, &c_intercomm);
    if( *ierror == MPI_SUCCESS) {
        *intercomm = PMPI_Comm_c2f(c_intercomm);
    }
}

#pragma weak mpi_comm_rank_ = pmpi_comm_rank_
void pmpi_comm_rank_(MPI_Fint *comm, int *rank, int *ierror)
{
/* MPI_Comm_rank */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_rank(c_comm, rank);
}

#pragma weak mpi_comm_rank__ = pmpi_comm_rank__
void pmpi_comm_rank__(MPI_Fint *comm, int *rank, int *ierror)
{
/* MPI_Comm_rank */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_rank(c_comm, rank);
}

#pragma weak mpi_comm_remote_group_ = pmpi_comm_remote_group_
void pmpi_comm_remote_group_(MPI_Fint *comm, MPI_Fint *group, int *ierror)
{
/* MPI_Comm_remote_group */
	MPI_Comm  c_comm = PMPI_Comm_f2c(*comm);
	MPI_Group c_group;

	*ierror = MPI_Comm_remote_group(c_comm, &c_group);
    if( *ierror == MPI_SUCCESS) {
        *group  = PMPI_Group_c2f(c_group);
    }
}

#pragma weak mpi_comm_remote_group__ = pmpi_comm_remote_group__
void pmpi_comm_remote_group__(MPI_Fint *comm, MPI_Fint *group, int *ierror)
{
/* MPI_Comm_remote_group */
	MPI_Comm  c_comm = PMPI_Comm_f2c(*comm);
	MPI_Group c_group;

	*ierror = MPI_Comm_remote_group(c_comm, &c_group);
    if( *ierror == MPI_SUCCESS) {
        *group  = PMPI_Group_c2f(c_group);
    }
}

#pragma weak mpi_comm_remote_size_ = pmpi_comm_remote_size_
void pmpi_comm_remote_size_(MPI_Fint *comm, int *size, int *ierror)
{
/* MPI_Comm_remote_size */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_remote_size(c_comm, size);
}

#pragma weak mpi_comm_remote_size__ = pmpi_comm_remote_size__
void pmpi_comm_remote_size__(MPI_Fint *comm, int *size, int *ierror)
{
/* MPI_Comm_remote_size */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_remote_size(c_comm, size);
}

#pragma weak mpi_comm_set_attr_ = pmpi_comm_set_attr_
void pmpi_comm_set_attr_(MPI_Fint *comm, int *comm_keyval, void *attribute_val, int *ierror)
{
/* MPI_Comm_set_attr */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_set_attr(c_comm, *comm_keyval, attribute_val);
}

#pragma weak mpi_comm_set_attr__ = pmpi_comm_set_attr__
void pmpi_comm_set_attr__(MPI_Fint *comm, int *comm_keyval, void *attribute_val, int *ierror)
{
/* MPI_Comm_set_attr */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_set_attr(c_comm, *comm_keyval, attribute_val);
}

#pragma weak mpi_errhandler_set_ = pmpi_errhandler_set_
void pmpi_errhandler_set_(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Errhandler_set */
	MPI_Comm       c_comm       = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror = MPI_Errhandler_set(c_comm, c_errhandler);
}

#pragma weak mpi_errhandler_set__ = pmpi_errhandler_set__
void pmpi_errhandler_set__(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Errhandler_set */
	MPI_Comm       c_comm       = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror = MPI_Errhandler_set(c_comm, c_errhandler);
}

#pragma weak mpi_comm_set_errhandler_ = pmpi_comm_set_errhandler_
void pmpi_comm_set_errhandler_(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_set_errhandler */
	MPI_Comm       c_comm       = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror = MPI_Comm_set_errhandler(c_comm, c_errhandler);
}

#pragma weak mpi_comm_set_errhandler__ = pmpi_comm_set_errhandler__
void pmpi_comm_set_errhandler__(MPI_Fint *comm, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Comm_set_errhandler */
	MPI_Comm       c_comm       = PMPI_Comm_f2c(*comm);
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror = MPI_Comm_set_errhandler(c_comm, c_errhandler);
}

#pragma weak mpi_comm_set_info_ = pmpi_comm_set_info_
void pmpi_comm_set_info_(MPI_Fint *comm, MPI_Fint *info, int *ierror)
{
/* MPI_Comm_set_info */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Comm_set_info(c_comm, c_info);
}

#pragma weak mpi_comm_set_info__ = pmpi_comm_set_info__
void pmpi_comm_set_info__(MPI_Fint *comm, MPI_Fint *info, int *ierror)
{
/* MPI_Comm_set_info */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Comm_set_info(c_comm, c_info);
}

#pragma weak mpi_comm_set_name_ = pmpi_comm_set_name_
void pmpi_comm_set_name_(MPI_Fint *comm, const char *comm_name CHAR_MIXED(size_comm_name), int *ierror CHAR_END(size_comm_name) )
{
/* MPI_Comm_set_name */
	MPI_Comm c_comm        = PMPI_Comm_f2c(*comm);
	char *   tmp_comm_name = NULL, *ptr_comm_name = NULL;

	tmp_comm_name = char_fortran_to_c( (char *)comm_name, size_comm_name, &ptr_comm_name);

	*ierror = MPI_Comm_set_name(c_comm, tmp_comm_name);
	sctk_free(ptr_comm_name);
}

#pragma weak mpi_comm_set_name__ = pmpi_comm_set_name__
void pmpi_comm_set_name__(MPI_Fint *comm, const char *comm_name CHAR_MIXED(size_comm_name), int *ierror CHAR_END(size_comm_name) )
{
/* MPI_Comm_set_name */
	MPI_Comm c_comm        = PMPI_Comm_f2c(*comm);
	char *   tmp_comm_name = NULL, *ptr_comm_name = NULL;

	tmp_comm_name = char_fortran_to_c( (char *)comm_name, size_comm_name, &ptr_comm_name);

	*ierror = MPI_Comm_set_name(c_comm, tmp_comm_name);
	sctk_free(ptr_comm_name);
}

#pragma weak mpi_comm_size_ = pmpi_comm_size_
void pmpi_comm_size_(MPI_Fint *comm, int *size, int *ierror)
{
/* MPI_Comm_size */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_size(c_comm, size);
}

#pragma weak mpi_comm_size__ = pmpi_comm_size__
void pmpi_comm_size__(MPI_Fint *comm, int *size, int *ierror)
{
/* MPI_Comm_size */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_size(c_comm, size);
}

#pragma weak mpi_comm_spawn_ = pmpi_comm_spawn_
void pmpi_comm_spawn_(const char *command CHAR_MIXED(size_command), char *argv[], int *maxprocs, MPI_Fint *info, int *root, MPI_Fint *comm, MPI_Fint *intercomm, int array_of_errcodes[], int *ierror CHAR_END(size_command) )
{
/* MPI_Comm_spawn */
	char *tmp_command = NULL, *ptr_command = NULL;

	tmp_command = char_fortran_to_c( (char *)command, size_command, &ptr_command);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_intercomm;

	*ierror    = MPI_Comm_spawn(tmp_command, argv, *maxprocs, c_info, *root, c_comm, &c_intercomm, array_of_errcodes);
	*intercomm = PMPI_Comm_c2f(c_intercomm);
	sctk_free(ptr_command);
}

#pragma weak mpi_comm_spawn__ = pmpi_comm_spawn__
void pmpi_comm_spawn__(const char *command CHAR_MIXED(size_command), char *argv[], int *maxprocs, MPI_Fint *info, int *root, MPI_Fint *comm, MPI_Fint *intercomm, int array_of_errcodes[], int *ierror CHAR_END(size_command) )
{
/* MPI_Comm_spawn */
	char *tmp_command = NULL, *ptr_command = NULL;

	tmp_command = char_fortran_to_c( (char *)command, size_command, &ptr_command);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_intercomm;

	*ierror    = MPI_Comm_spawn(tmp_command, argv, *maxprocs, c_info, *root, c_comm, &c_intercomm, array_of_errcodes);
	*intercomm = PMPI_Comm_c2f(c_intercomm);
	sctk_free(ptr_command);
}

#pragma weak mpi_comm_spawn_multiple_ = pmpi_comm_spawn_multiple_
void pmpi_comm_spawn_multiple_(int *count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Fint array_of_info[], int *root, MPI_Fint *comm, MPI_Fint *intercomm, int array_of_errcodes[], int *ierror)
{
/* MPI_Comm_spawn_multiple */

	int       incnt_array_of_info = 0;
	MPI_Info *c_array_of_info     = NULL;

	c_array_of_info = (MPI_Info *)sctk_malloc(sizeof(MPI_Info) * *count);

	for(incnt_array_of_info = 0; incnt_array_of_info < *count; incnt_array_of_info++)
	{
		c_array_of_info[incnt_array_of_info] = PMPI_Info_f2c(array_of_info[incnt_array_of_info]);
	}
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_intercomm;

	*ierror    = MPI_Comm_spawn_multiple(*count, array_of_commands, array_of_argv, array_of_maxprocs, c_array_of_info, *root, c_comm, &c_intercomm, array_of_errcodes);
	*intercomm = PMPI_Comm_c2f(c_intercomm);
	sctk_free(c_array_of_info);
}

#pragma weak mpi_comm_spawn_multiple__ = pmpi_comm_spawn_multiple__
void pmpi_comm_spawn_multiple__(int *count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Fint array_of_info[], int *root, MPI_Fint *comm, MPI_Fint *intercomm, int array_of_errcodes[], int *ierror)
{
/* MPI_Comm_spawn_multiple */

	int       incnt_array_of_info = 0;
	MPI_Info *c_array_of_info     = NULL;

	c_array_of_info = (MPI_Info *)sctk_malloc(sizeof(MPI_Info) * *count);

	for(incnt_array_of_info = 0; incnt_array_of_info < *count; incnt_array_of_info++)
	{
		c_array_of_info[incnt_array_of_info] = PMPI_Info_f2c(array_of_info[incnt_array_of_info]);
	}
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_intercomm;

	*ierror    = MPI_Comm_spawn_multiple(*count, array_of_commands, array_of_argv, array_of_maxprocs, c_array_of_info, *root, c_comm, &c_intercomm, array_of_errcodes);
	*intercomm = PMPI_Comm_c2f(c_intercomm);
	sctk_free(c_array_of_info);
}

#pragma weak mpi_comm_split_ = pmpi_comm_split_
void pmpi_comm_split_(MPI_Fint *comm, int *color, int *key, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_split */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_split(c_comm, *color, *key, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_split__ = pmpi_comm_split__
void pmpi_comm_split__(MPI_Fint *comm, int *color, int *key, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_split */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_split(c_comm, *color, *key, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_split_type_ = pmpi_comm_split_type_
void pmpi_comm_split_type_(MPI_Fint *comm, int *split_type, int *key, MPI_Fint *info, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_split_type */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_split_type(c_comm, *split_type, *key, c_info, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_split_type__ = pmpi_comm_split_type__
void pmpi_comm_split_type__(MPI_Fint *comm, int *split_type, int *key, MPI_Fint *info, MPI_Fint *newcomm, int *ierror)
{
/* MPI_Comm_split_type */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_newcomm;

	*ierror  = MPI_Comm_split_type(c_comm, *split_type, *key, c_info, &c_newcomm);
    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_test_inter_ = pmpi_comm_test_inter_
void pmpi_comm_test_inter_(MPI_Fint *comm, int *flag, int *ierror)
{
/* MPI_Comm_test_inter */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_test_inter(c_comm, flag);
}

#pragma weak mpi_comm_test_inter__ = pmpi_comm_test_inter__
void pmpi_comm_test_inter__(MPI_Fint *comm, int *flag, int *ierror)
{
/* MPI_Comm_test_inter */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Comm_test_inter(c_comm, flag);
}

#pragma weak mpi_compare_and_swap_ = pmpi_compare_and_swap_
void pmpi_compare_and_swap_(const void *origin_addr, const void *compare_addr, void *result_addr, MPI_Fint *datatype, int *target_rank, MPI_Aint *target_disp, MPI_Fint *win, int *ierror)
{
/* MPI_Compare_and_swap */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	if(buffer_is_bottom( (void *)compare_addr) )
	{
		compare_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Win      c_win      = PMPI_Win_f2c(*win);

	*ierror = MPI_Compare_and_swap(origin_addr, compare_addr, result_addr, c_datatype, *target_rank, *target_disp, c_win);
}

#pragma weak mpi_compare_and_swap__ = pmpi_compare_and_swap__
void pmpi_compare_and_swap__(const void *origin_addr, const void *compare_addr, void *result_addr, MPI_Fint *datatype, int *target_rank, MPI_Aint *target_disp, MPI_Fint *win, int *ierror)
{
/* MPI_Compare_and_swap */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	if(buffer_is_bottom( (void *)compare_addr) )
	{
		compare_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Win      c_win      = PMPI_Win_f2c(*win);

	*ierror = MPI_Compare_and_swap(origin_addr, compare_addr, result_addr, c_datatype, *target_rank, *target_disp, c_win);
}

#pragma weak mpi_dims_create_ = pmpi_dims_create_
void pmpi_dims_create_(int *nnodes, int *ndims, int dims[], int *ierror)
{
/* MPI_Dims_create */

	*ierror = MPI_Dims_create(*nnodes, *ndims, dims);
}

#pragma weak mpi_dims_create__ = pmpi_dims_create__
void pmpi_dims_create__(int *nnodes, int *ndims, int dims[], int *ierror)
{
/* MPI_Dims_create */

	*ierror = MPI_Dims_create(*nnodes, *ndims, dims);
}

#pragma weak mpi_dist_graph_create_ = pmpi_dist_graph_create_
void pmpi_dist_graph_create_(MPI_Fint *comm_old, int *n, const int sources[], const int degrees[], const int destinations[], const int weights[], MPI_Fint *info, int *reorder, MPI_Fint *comm_dist_graph, int *ierror)
{
/* MPI_Dist_graph_create */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Info c_info     = PMPI_Info_f2c(*info);
	MPI_Comm c_comm_dist_graph;

	*ierror          = MPI_Dist_graph_create(c_comm_old, *n, sources, degrees, destinations, weights, c_info, *reorder, &c_comm_dist_graph);
	*comm_dist_graph = PMPI_Comm_c2f(c_comm_dist_graph);
}

#pragma weak mpi_dist_graph_create__ = pmpi_dist_graph_create__
void pmpi_dist_graph_create__(MPI_Fint *comm_old, int *n, const int sources[], const int degrees[], const int destinations[], const int weights[], MPI_Fint *info, int *reorder, MPI_Fint *comm_dist_graph, int *ierror)
{
/* MPI_Dist_graph_create */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Info c_info     = PMPI_Info_f2c(*info);
	MPI_Comm c_comm_dist_graph;

	*ierror          = MPI_Dist_graph_create(c_comm_old, *n, sources, degrees, destinations, weights, c_info, *reorder, &c_comm_dist_graph);
	*comm_dist_graph = PMPI_Comm_c2f(c_comm_dist_graph);
}

#pragma weak mpi_dist_graph_create_adjacent_ = pmpi_dist_graph_create_adjacent_
void pmpi_dist_graph_create_adjacent_(MPI_Fint *comm_old, int *indegree, const int sources[], const int sourceweights[], int *outdegree, const int destinations[], const int destweights[], MPI_Fint *info, int *reorder, MPI_Fint *comm_dist_graph, int *ierror)
{
/* MPI_Dist_graph_create_adjacent */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Info c_info     = PMPI_Info_f2c(*info);
	MPI_Comm c_comm_dist_graph;

	*ierror          = MPI_Dist_graph_create_adjacent(c_comm_old, *indegree, sources, sourceweights, *outdegree, destinations, destweights, c_info, *reorder, &c_comm_dist_graph);
	*comm_dist_graph = PMPI_Comm_c2f(c_comm_dist_graph);
}

#pragma weak mpi_dist_graph_create_adjacent__ = pmpi_dist_graph_create_adjacent__
void pmpi_dist_graph_create_adjacent__(MPI_Fint *comm_old, int *indegree, const int sources[], const int sourceweights[], int *outdegree, const int destinations[], const int destweights[], MPI_Fint *info, int *reorder, MPI_Fint *comm_dist_graph, int *ierror)
{
/* MPI_Dist_graph_create_adjacent */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Info c_info     = PMPI_Info_f2c(*info);
	MPI_Comm c_comm_dist_graph;

	*ierror          = MPI_Dist_graph_create_adjacent(c_comm_old, *indegree, sources, sourceweights, *outdegree, destinations, destweights, c_info, *reorder, &c_comm_dist_graph);
	*comm_dist_graph = PMPI_Comm_c2f(c_comm_dist_graph);
}

#pragma weak mpi_dist_graph_neighbors_ = pmpi_dist_graph_neighbors_
void pmpi_dist_graph_neighbors_(MPI_Fint *comm, int *maxindegree, int sources[], int sourceweights[], int *maxoutdegree, int destinations[], int destweights[], int *ierror)
{
/* MPI_Dist_graph_neighbors */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Dist_graph_neighbors(c_comm, *maxindegree, sources, sourceweights, *maxoutdegree, destinations, destweights);
}

#pragma weak mpi_dist_graph_neighbors__ = pmpi_dist_graph_neighbors__
void pmpi_dist_graph_neighbors__(MPI_Fint *comm, int *maxindegree, int sources[], int sourceweights[], int *maxoutdegree, int destinations[], int destweights[], int *ierror)
{
/* MPI_Dist_graph_neighbors */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Dist_graph_neighbors(c_comm, *maxindegree, sources, sourceweights, *maxoutdegree, destinations, destweights);
}

#pragma weak mpi_dist_graph_neighbors_count_ = pmpi_dist_graph_neighbors_count_
void pmpi_dist_graph_neighbors_count_(MPI_Fint *comm, int *indegree, int *outdegree, int *weighted, int *ierror)
{
/* MPI_Dist_graph_neighbors_count */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Dist_graph_neighbors_count(c_comm, indegree, outdegree, weighted);
}

#pragma weak mpi_dist_graph_neighbors_count__ = pmpi_dist_graph_neighbors_count__
void pmpi_dist_graph_neighbors_count__(MPI_Fint *comm, int *indegree, int *outdegree, int *weighted, int *ierror)
{
/* MPI_Dist_graph_neighbors_count */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Dist_graph_neighbors_count(c_comm, indegree, outdegree, weighted);
}

#pragma weak mpi_errhandler_free_ = pmpi_errhandler_free_
void pmpi_errhandler_free_(MPI_Fint *errhandler, int *ierror)
{
/* MPI_Errhandler_free */
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror     = MPI_Errhandler_free(&c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_errhandler_free__ = pmpi_errhandler_free__
void pmpi_errhandler_free__(MPI_Fint *errhandler, int *ierror)
{
/* MPI_Errhandler_free */
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror     = MPI_Errhandler_free(&c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_error_class_ = pmpi_error_class_
void pmpi_error_class_(int *errorcode, int *errorclass, int *ierror)
{
/* MPI_Error_class */

	*ierror = MPI_Error_class(*errorcode, errorclass);
}

#pragma weak mpi_error_class__ = pmpi_error_class__
void pmpi_error_class__(int *errorcode, int *errorclass, int *ierror)
{
/* MPI_Error_class */

	*ierror = MPI_Error_class(*errorcode, errorclass);
}

#pragma weak mpi_error_string_ = pmpi_error_string_
void pmpi_error_string_(int *errorcode, char *string CHAR_MIXED(size_string), int *resultlen, int *ierror CHAR_END(size_string) )
{
/* MPI_Error_string */

	*ierror = MPI_Error_string(*errorcode, string, resultlen);
	char_c_to_fortran(string, size_string);
}

#pragma weak mpi_error_string__ = pmpi_error_string__
void pmpi_error_string__(int *errorcode, char *string CHAR_MIXED(size_string), int *resultlen, int *ierror CHAR_END(size_string) )
{
/* MPI_Error_string */

	*ierror = MPI_Error_string(*errorcode, string, resultlen);
	char_c_to_fortran(string, size_string);
}

#pragma weak mpi_exscan_ = pmpi_exscan_
void pmpi_exscan_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Exscan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Exscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

#pragma weak mpi_exscan__ = pmpi_exscan__
void pmpi_exscan__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Exscan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Exscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

#pragma weak mpi_exscan_init_ = pmpi_exscan_init_
void pmpi_exscan_init_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Exscan_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Exscan_init(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_exscan_init__ = pmpi_exscan_init__
void pmpi_exscan_init__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Exscan_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Exscan_init(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_f_sync_reg_ = pmpi_f_sync_reg_
void pmpi_f_sync_reg_(__UNUSED__ void *buf)
{
}

#pragma weak mpi_f_sync_reg__ = pmpi_f_sync_reg__
void pmpi_f_sync_reg__(__UNUSED__ void *buf)
{
}

#pragma weak mpi_fetch_and_op_ = pmpi_fetch_and_op_
void pmpi_fetch_and_op_(const void *origin_addr, void *result_addr, MPI_Fint *datatype, int *target_rank, MPI_Aint *target_disp, MPI_Fint *op, MPI_Fint *win, int *ierror)
{
/* MPI_Fetch_and_op */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Win      c_win      = PMPI_Win_f2c(*win);

	*ierror = MPI_Fetch_and_op(origin_addr, result_addr, c_datatype, *target_rank, *target_disp, c_op, c_win);
}

#pragma weak mpi_fetch_and_op__ = pmpi_fetch_and_op__
void pmpi_fetch_and_op__(const void *origin_addr, void *result_addr, MPI_Fint *datatype, int *target_rank, MPI_Aint *target_disp, MPI_Fint *op, MPI_Fint *win, int *ierror)
{
/* MPI_Fetch_and_op */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Win      c_win      = PMPI_Win_f2c(*win);

	*ierror = MPI_Fetch_and_op(origin_addr, result_addr, c_datatype, *target_rank, *target_disp, c_op, c_win);
}

#pragma weak mpi_finalize_ = pmpi_finalize_
void pmpi_finalize_(int *ierror)
{
/* MPI_Finalize */

	*ierror = MPI_Finalize();
}

#pragma weak mpi_finalize__ = pmpi_finalize__
void pmpi_finalize__(int *ierror)
{
/* MPI_Finalize */

	*ierror = MPI_Finalize();
}

#pragma weak mpi_finalized_ = pmpi_finalized_
void pmpi_finalized_(int *flag, int *ierror)
{
/* MPI_Finalized */

	*ierror = MPI_Finalized(flag);
}

#pragma weak mpi_finalized__ = pmpi_finalized__
void pmpi_finalized__(int *flag, int *ierror)
{
/* MPI_Finalized */

	*ierror = MPI_Finalized(flag);
}

#pragma weak mpi_free_mem_ = pmpi_free_mem_
void pmpi_free_mem_(void *base, int *ierror)
{
/* MPI_Free_mem */
	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}

	*ierror = MPI_Free_mem(base);
}

#pragma weak mpi_free_mem__ = pmpi_free_mem__
void pmpi_free_mem__(void *base, int *ierror)
{
/* MPI_Free_mem */
	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}

	*ierror = MPI_Free_mem(base);
}

#pragma weak mpi_gather_ = pmpi_gather_
void pmpi_gather_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Gather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Gather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

#pragma weak mpi_gather__ = pmpi_gather__
void pmpi_gather__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Gather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Gather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

#pragma weak mpi_gather_init_ = pmpi_gather_init_
void pmpi_gather_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Gather_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Gather_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_gather_init__ = pmpi_gather_init__
void pmpi_gather_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Gather_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Gather_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_gatherv_ = pmpi_gatherv_
void pmpi_gatherv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Gatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Gatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm);
}

#pragma weak mpi_gatherv__ = pmpi_gatherv__
void pmpi_gatherv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Gatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Gatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm);
}

#pragma weak mpi_gatherv_init_ = pmpi_gatherv_init_
void pmpi_gatherv_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Gatherv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Gatherv_init(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_gatherv_init__ = pmpi_gatherv_init__
void pmpi_gatherv_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Gatherv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Gatherv_init(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_get_ = pmpi_get_
void pmpi_get_(void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, int *ierror)
{
/* MPI_Get */
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);

	*ierror = MPI_Get(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win);
}

#pragma weak mpi_get__ = pmpi_get__
void pmpi_get__(void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, int *ierror)
{
/* MPI_Get */
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);

	*ierror = MPI_Get(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win);
}

#pragma weak mpi_get_accumulate_ = pmpi_get_accumulate_
void pmpi_get_accumulate_(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, void *result_addr, int *result_count, MPI_Fint *result_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, int *ierror)
{
/* MPI_Get_accumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_result_datatype = PMPI_Type_f2c(*result_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Get_accumulate(origin_addr, *origin_count, c_origin_datatype, result_addr, *result_count, c_result_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win);
}

#pragma weak mpi_get_accumulate__ = pmpi_get_accumulate__
void pmpi_get_accumulate__(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, void *result_addr, int *result_count, MPI_Fint *result_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, int *ierror)
{
/* MPI_Get_accumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_result_datatype = PMPI_Type_f2c(*result_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Get_accumulate(origin_addr, *origin_count, c_origin_datatype, result_addr, *result_count, c_result_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win);
}

#pragma weak mpi_get_address_ = pmpi_get_address_
void pmpi_get_address_(const void *location, MPI_Aint *address, int *ierror)
{
/* MPI_Get_address */
	if(buffer_is_bottom( (void *)location) )
	{
		location = MPI_BOTTOM;
	}

	*ierror = MPI_Get_address(location, address);
}

#pragma weak mpi_get_address__ = pmpi_get_address__
void pmpi_get_address__(const void *location, MPI_Aint *address, int *ierror)
{
/* MPI_Get_address */
	if(buffer_is_bottom( (void *)location) )
	{
		location = MPI_BOTTOM;
	}

	*ierror = MPI_Get_address(location, address);
}

#pragma weak mpi_get_count_ = pmpi_get_count_
void pmpi_get_count_(const MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Get_count */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Get_count(status, c_datatype, count);
}

#pragma weak mpi_get_count__ = pmpi_get_count__
void pmpi_get_count__(const MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Get_count */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Get_count(status, c_datatype, count);
}

#pragma weak mpi_get_library_version_ = pmpi_get_library_version_
void pmpi_get_library_version_(char *version CHAR_MIXED(size_version), int *resultlen, int *ierror CHAR_END(size_version) )
{
/* MPI_Get_library_version */

	*ierror = MPI_Get_library_version(version, resultlen);
	char_c_to_fortran(version, size_version);
}

#pragma weak mpi_get_library_version__ = pmpi_get_library_version__
void pmpi_get_library_version__(char *version CHAR_MIXED(size_version), int *resultlen, int *ierror CHAR_END(size_version) )
{
/* MPI_Get_library_version */

	*ierror = MPI_Get_library_version(version, resultlen);
	char_c_to_fortran(version, size_version);
}

#pragma weak mpi_get_processor_name_ = pmpi_get_processor_name_
void pmpi_get_processor_name_(char *name CHAR_MIXED(size_name), int *resultlen, int *ierror CHAR_END(size_name) )
{
/* MPI_Get_processor_name */

	*ierror = MPI_Get_processor_name(name, resultlen);
	char_c_to_fortran(name, size_name);
}

#pragma weak mpi_get_processor_name__ = pmpi_get_processor_name__
void pmpi_get_processor_name__(char *name CHAR_MIXED(size_name), int *resultlen, int *ierror CHAR_END(size_name) )
{
/* MPI_Get_processor_name */

	*ierror = MPI_Get_processor_name(name, resultlen);
	char_c_to_fortran(name, size_name);
}

#pragma weak mpi_get_version_ = pmpi_get_version_
void pmpi_get_version_(int *version, int *subversion, int *ierror)
{
/* MPI_Get_version */

	*ierror = MPI_Get_version(version, subversion);
}

#pragma weak mpi_get_version__ = pmpi_get_version__
void pmpi_get_version__(int *version, int *subversion, int *ierror)
{
/* MPI_Get_version */

	*ierror = MPI_Get_version(version, subversion);
}

#pragma weak mpi_graph_create_ = pmpi_graph_create_
void pmpi_graph_create_(MPI_Fint *comm_old, int *nnodes, const int index[], const int edges[], int *reorder, MPI_Fint *comm_graph, int *ierror)
{
/* MPI_Graph_create */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Comm c_comm_graph;

	*ierror     = MPI_Graph_create(c_comm_old, *nnodes, index, edges, *reorder, &c_comm_graph);
	*comm_graph = PMPI_Comm_c2f(c_comm_graph);
}

#pragma weak mpi_graph_create__ = pmpi_graph_create__
void pmpi_graph_create__(MPI_Fint *comm_old, int *nnodes, const int index[], const int edges[], int *reorder, MPI_Fint *comm_graph, int *ierror)
{
/* MPI_Graph_create */
	MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
	MPI_Comm c_comm_graph;

	*ierror     = MPI_Graph_create(c_comm_old, *nnodes, index, edges, *reorder, &c_comm_graph);
	*comm_graph = PMPI_Comm_c2f(c_comm_graph);
}

#pragma weak mpi_graph_get_ = pmpi_graph_get_
void pmpi_graph_get_(MPI_Fint *comm, int *maxindex, int *maxedges, int index[], int edges[], int *ierror)
{
/* MPI_Graph_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_get(c_comm, *maxindex, *maxedges, index, edges);
}

#pragma weak mpi_graph_get__ = pmpi_graph_get__
void pmpi_graph_get__(MPI_Fint *comm, int *maxindex, int *maxedges, int index[], int edges[], int *ierror)
{
/* MPI_Graph_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_get(c_comm, *maxindex, *maxedges, index, edges);
}

#pragma weak mpi_graph_map_ = pmpi_graph_map_
void pmpi_graph_map_(MPI_Fint *comm, int *nnodes, const int index[], const int edges[], int *newrank, int *ierror)
{
/* MPI_Graph_map */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_map(c_comm, *nnodes, index, edges, newrank);
}

#pragma weak mpi_graph_map__ = pmpi_graph_map__
void pmpi_graph_map__(MPI_Fint *comm, int *nnodes, const int index[], const int edges[], int *newrank, int *ierror)
{
/* MPI_Graph_map */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_map(c_comm, *nnodes, index, edges, newrank);
}

#pragma weak mpi_graph_neighbors_ = pmpi_graph_neighbors_
void pmpi_graph_neighbors_(MPI_Fint *comm, int *rank, int *maxneighbors, int neighbors[], int *ierror)
{
/* MPI_Graph_neighbors */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_neighbors(c_comm, *rank, *maxneighbors, neighbors);
}

#pragma weak mpi_graph_neighbors__ = pmpi_graph_neighbors__
void pmpi_graph_neighbors__(MPI_Fint *comm, int *rank, int *maxneighbors, int neighbors[], int *ierror)
{
/* MPI_Graph_neighbors */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_neighbors(c_comm, *rank, *maxneighbors, neighbors);
}

#pragma weak mpi_graph_neighbors_count_ = pmpi_graph_neighbors_count_
void pmpi_graph_neighbors_count_(MPI_Fint *comm, int *rank, int *nneighbors, int *ierror)
{
/* MPI_Graph_neighbors_count */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_neighbors_count(c_comm, *rank, nneighbors);
}

#pragma weak mpi_graph_neighbors_count__ = pmpi_graph_neighbors_count__
void pmpi_graph_neighbors_count__(MPI_Fint *comm, int *rank, int *nneighbors, int *ierror)
{
/* MPI_Graph_neighbors_count */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graph_neighbors_count(c_comm, *rank, nneighbors);
}

#pragma weak mpi_graphdims_get_ = pmpi_graphdims_get_
void pmpi_graphdims_get_(MPI_Fint *comm, int *nnodes, int *nedges, int *ierror)
{
/* MPI_Graphdims_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graphdims_get(c_comm, nnodes, nedges);
}

#pragma weak mpi_graphdims_get__ = pmpi_graphdims_get__
void pmpi_graphdims_get__(MPI_Fint *comm, int *nnodes, int *nedges, int *ierror)
{
/* MPI_Graphdims_get */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Graphdims_get(c_comm, nnodes, nedges);
}

#pragma weak mpi_grequest_complete_ = pmpi_grequest_complete_
void pmpi_grequest_complete_(MPI_Fint *request, int *ierror)
{
/* MPI_Grequest_complete */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror = MPI_Grequest_complete(c_request);
}

#pragma weak mpi_grequest_complete__ = pmpi_grequest_complete__
void pmpi_grequest_complete__(MPI_Fint *request, int *ierror)
{
/* MPI_Grequest_complete */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror = MPI_Grequest_complete(c_request);
}

#pragma weak mpi_grequest_start_ = pmpi_grequest_start_
void pmpi_grequest_start_(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Fint *request, int *ierror)
{
/* MPI_Grequest_start */
	MPI_Request c_request;

	*ierror  = MPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_grequest_start__ = pmpi_grequest_start__
void pmpi_grequest_start__(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Fint *request, int *ierror)
{
/* MPI_Grequest_start */
	MPI_Request c_request;

	*ierror  = MPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_group_compare_ = pmpi_group_compare_
void pmpi_group_compare_(MPI_Fint *group1, MPI_Fint *group2, int *result, int *ierror)
{
/* MPI_Group_compare */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);

	*ierror = MPI_Group_compare(c_group1, c_group2, result);
}

#pragma weak mpi_group_compare__ = pmpi_group_compare__
void pmpi_group_compare__(MPI_Fint *group1, MPI_Fint *group2, int *result, int *ierror)
{
/* MPI_Group_compare */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);

	*ierror = MPI_Group_compare(c_group1, c_group2, result);
}

#pragma weak mpi_group_difference_ = pmpi_group_difference_
void pmpi_group_difference_(MPI_Fint *group1, MPI_Fint *group2, MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_difference */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_difference(c_group1, c_group2, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_difference__ = pmpi_group_difference__
void pmpi_group_difference__(MPI_Fint *group1, MPI_Fint *group2, MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_difference */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_difference(c_group1, c_group2, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_excl_ = pmpi_group_excl_
void pmpi_group_excl_(MPI_Fint *group, int *n, const int ranks[], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_excl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_excl(c_group, *n, ranks, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_excl__ = pmpi_group_excl__
void pmpi_group_excl__(MPI_Fint *group, int *n, const int ranks[], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_excl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_excl(c_group, *n, ranks, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_free_ = pmpi_group_free_
void pmpi_group_free_(MPI_Fint *group, int *ierror)
{
/* MPI_Group_free */
	MPI_Group c_group = PMPI_Group_f2c(*group);

	*ierror = MPI_Group_free(&c_group);
	*group  = PMPI_Group_c2f(c_group);
}

#pragma weak mpi_group_free__ = pmpi_group_free__
void pmpi_group_free__(MPI_Fint *group, int *ierror)
{
/* MPI_Group_free */
	MPI_Group c_group = PMPI_Group_f2c(*group);

	*ierror = MPI_Group_free(&c_group);
	*group  = PMPI_Group_c2f(c_group);
}

#pragma weak mpi_group_incl_ = pmpi_group_incl_
void pmpi_group_incl_(MPI_Fint *group, int *n, const int ranks[], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_incl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_incl(c_group, *n, ranks, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_incl__ = pmpi_group_incl__
void pmpi_group_incl__(MPI_Fint *group, int *n, const int ranks[], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_incl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_incl(c_group, *n, ranks, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_intersection_ = pmpi_group_intersection_
void pmpi_group_intersection_(MPI_Fint *group1, MPI_Fint *group2, MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_intersection */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_intersection(c_group1, c_group2, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_intersection__ = pmpi_group_intersection__
void pmpi_group_intersection__(MPI_Fint *group1, MPI_Fint *group2, MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_intersection */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_intersection(c_group1, c_group2, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_range_excl_ = pmpi_group_range_excl_
void pmpi_group_range_excl_(MPI_Fint *group, int *n, int ranges[][3], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_range_excl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_range_excl(c_group, *n, ranges, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_range_excl__ = pmpi_group_range_excl__
void pmpi_group_range_excl__(MPI_Fint *group, int *n, int ranges[][3], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_range_excl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_range_excl(c_group, *n, ranges, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_range_incl_ = pmpi_group_range_incl_
void pmpi_group_range_incl_(MPI_Fint *group, int *n, int ranges[][3], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_range_incl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_range_incl(c_group, *n, ranges, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_range_incl__ = pmpi_group_range_incl__
void pmpi_group_range_incl__(MPI_Fint *group, int *n, int ranges[][3], MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_range_incl */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_range_incl(c_group, *n, ranges, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_rank_ = pmpi_group_rank_
void pmpi_group_rank_(MPI_Fint *group, int *rank, int *ierror)
{
/* MPI_Group_rank */
	MPI_Group c_group = PMPI_Group_f2c(*group);

	*ierror = MPI_Group_rank(c_group, rank);
}

#pragma weak mpi_group_rank__ = pmpi_group_rank__
void pmpi_group_rank__(MPI_Fint *group, int *rank, int *ierror)
{
/* MPI_Group_rank */
	MPI_Group c_group = PMPI_Group_f2c(*group);

	*ierror = MPI_Group_rank(c_group, rank);
}

#pragma weak mpi_group_size_ = pmpi_group_size_
void pmpi_group_size_(MPI_Fint *group, int *size, int *ierror)
{
/* MPI_Group_size */
	MPI_Group c_group = PMPI_Group_f2c(*group);

	*ierror = MPI_Group_size(c_group, size);
}

#pragma weak mpi_group_size__ = pmpi_group_size__
void pmpi_group_size__(MPI_Fint *group, int *size, int *ierror)
{
/* MPI_Group_size */
	MPI_Group c_group = PMPI_Group_f2c(*group);

	*ierror = MPI_Group_size(c_group, size);
}

#pragma weak mpi_group_translate_ranks_ = pmpi_group_translate_ranks_
void pmpi_group_translate_ranks_(MPI_Fint *group1, int *n, const int ranks1[], MPI_Fint *group2, int ranks2[], int *ierror)
{
/* MPI_Group_translate_ranks */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);

	*ierror = MPI_Group_translate_ranks(c_group1, *n, ranks1, c_group2, ranks2);
}

#pragma weak mpi_group_translate_ranks__ = pmpi_group_translate_ranks__
void pmpi_group_translate_ranks__(MPI_Fint *group1, int *n, const int ranks1[], MPI_Fint *group2, int ranks2[], int *ierror)
{
/* MPI_Group_translate_ranks */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);

	*ierror = MPI_Group_translate_ranks(c_group1, *n, ranks1, c_group2, ranks2);
}

#pragma weak mpi_group_union_ = pmpi_group_union_
void pmpi_group_union_(MPI_Fint *group1, MPI_Fint *group2, MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_union */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_union(c_group1, c_group2, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_group_union__ = pmpi_group_union__
void pmpi_group_union__(MPI_Fint *group1, MPI_Fint *group2, MPI_Fint *newgroup, int *ierror)
{
/* MPI_Group_union */
	MPI_Group c_group1 = PMPI_Group_f2c(*group1);
	MPI_Group c_group2 = PMPI_Group_f2c(*group2);
	MPI_Group c_newgroup;

	*ierror   = MPI_Group_union(c_group1, c_group2, &c_newgroup);
    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_newgroup);
}

#pragma weak mpi_iallgather_ = pmpi_iallgather_
void pmpi_iallgather_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iallgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iallgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iallgather__ = pmpi_iallgather__
void pmpi_iallgather__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iallgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iallgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iallgatherv_ = pmpi_iallgatherv_
void pmpi_iallgatherv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iallgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iallgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iallgatherv__ = pmpi_iallgatherv__
void pmpi_iallgatherv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iallgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iallgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iallreduce_ = pmpi_iallreduce_
void pmpi_iallreduce_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iallreduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iallreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iallreduce__ = pmpi_iallreduce__
void pmpi_iallreduce__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iallreduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iallreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ialltoall_ = pmpi_ialltoall_
void pmpi_ialltoall_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ialltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ialltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ialltoall__ = pmpi_ialltoall__
void pmpi_ialltoall__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ialltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ialltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ialltoallv_ = pmpi_ialltoallv_
void pmpi_ialltoallv_(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ialltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ialltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ialltoallv__ = pmpi_ialltoallv__
void pmpi_ialltoallv__(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ialltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ialltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ialltoallw_ = pmpi_ialltoallw_
void pmpi_ialltoallw_(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ialltoallw */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Request c_request;

	*ierror  = MPI_Ialltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_ialltoallw__ = pmpi_ialltoallw__
void pmpi_ialltoallw__(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ialltoallw */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Request c_request;

	*ierror  = MPI_Ialltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_ibarrier_ = pmpi_ibarrier_
void pmpi_ibarrier_(MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ibarrier */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Request c_request;

	*ierror  = MPI_Ibarrier(c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ibarrier__ = pmpi_ibarrier__
void pmpi_ibarrier__(MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ibarrier */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	MPI_Request c_request;

	*ierror  = MPI_Ibarrier(c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ibcast_ = pmpi_ibcast_
void pmpi_ibcast_(void *buffer, int *count, MPI_Fint *datatype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ibcast */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ibcast(buffer, *count, c_datatype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ibcast__ = pmpi_ibcast__
void pmpi_ibcast__(void *buffer, int *count, MPI_Fint *datatype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ibcast */
	if(buffer_is_bottom( (void *)buffer) )
	{
		buffer = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ibcast(buffer, *count, c_datatype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ibsend_ = pmpi_ibsend_
void pmpi_ibsend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ibsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ibsend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ibsend__ = pmpi_ibsend__
void pmpi_ibsend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ibsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ibsend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iexscan_ = pmpi_iexscan_
void pmpi_iexscan_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iexscan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iexscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iexscan__ = pmpi_iexscan__
void pmpi_iexscan__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iexscan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iexscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_igather_ = pmpi_igather_
void pmpi_igather_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Igather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Igather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_igather__ = pmpi_igather__
void pmpi_igather__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Igather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Igather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_igatherv_ = pmpi_igatherv_
void pmpi_igatherv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Igatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Igatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_igatherv__ = pmpi_igatherv__
void pmpi_igatherv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Igatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Igatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_improbe_ = pmpi_improbe_
void pmpi_improbe_(int *source, int *tag, MPI_Fint *comm, int *flag, MPI_Message *message, MPI_Status *status, int *ierror)
{
/* MPI_Improbe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Improbe(*source, *tag, c_comm, flag, message, status);
}

#pragma weak mpi_improbe__ = pmpi_improbe__
void pmpi_improbe__(int *source, int *tag, MPI_Fint *comm, int *flag, MPI_Message *message, MPI_Status *status, int *ierror)
{
/* MPI_Improbe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Improbe(*source, *tag, c_comm, flag, message, status);
}

#pragma weak mpi_imrecv_ = pmpi_imrecv_
void pmpi_imrecv_(void *buf, int *count, MPI_Fint *datatype, MPI_Message *message, MPI_Fint *request, int *ierror)
{
/* MPI_Imrecv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Request  c_request;

	*ierror  = MPI_Imrecv(buf, *count, c_datatype, message, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_imrecv__ = pmpi_imrecv__
void pmpi_imrecv__(void *buf, int *count, MPI_Fint *datatype, MPI_Message *message, MPI_Fint *request, int *ierror)
{
/* MPI_Imrecv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Request  c_request;

	*ierror  = MPI_Imrecv(buf, *count, c_datatype, message, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_allgather_ = pmpi_ineighbor_allgather_
void pmpi_ineighbor_allgather_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_allgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_allgather__ = pmpi_ineighbor_allgather__
void pmpi_ineighbor_allgather__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_allgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_allgatherv_ = pmpi_ineighbor_allgatherv_
void pmpi_ineighbor_allgatherv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_allgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_allgatherv__ = pmpi_ineighbor_allgatherv__
void pmpi_ineighbor_allgatherv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_allgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_alltoall_ = pmpi_ineighbor_alltoall_
void pmpi_ineighbor_alltoall_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_alltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_alltoall__ = pmpi_ineighbor_alltoall__
void pmpi_ineighbor_alltoall__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_alltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_alltoallv_ = pmpi_ineighbor_alltoallv_
void pmpi_ineighbor_alltoallv_(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_alltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_alltoallv__ = pmpi_ineighbor_alltoallv__
void pmpi_ineighbor_alltoallv__(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_alltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ineighbor_alltoallw_ = pmpi_ineighbor_alltoallw_
void pmpi_ineighbor_alltoallw_(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_alltoallw */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Request c_request;

	*ierror  = MPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_ineighbor_alltoallw__ = pmpi_ineighbor_alltoallw__
void pmpi_ineighbor_alltoallw__(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ineighbor_alltoallw */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Request c_request;

	*ierror  = MPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_info_create_ = pmpi_info_create_
void pmpi_info_create_(MPI_Fint *info, int *ierror)
{
/* MPI_Info_create */
	MPI_Info c_info;

	*ierror = MPI_Info_create(&c_info);
	*info   = PMPI_Info_c2f(c_info);
}

#pragma weak mpi_info_create__ = pmpi_info_create__
void pmpi_info_create__(MPI_Fint *info, int *ierror)
{
/* MPI_Info_create */
	MPI_Info c_info;

	*ierror = MPI_Info_create(&c_info);
	*info   = PMPI_Info_c2f(c_info);
}

#pragma weak mpi_info_delete_ = pmpi_info_delete_
void pmpi_info_delete_(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *ierror CHAR_END(size_key) )
{
/* MPI_Info_delete */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_delete(c_info, tmp_key);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_delete__ = pmpi_info_delete__
void pmpi_info_delete__(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *ierror CHAR_END(size_key) )
{
/* MPI_Info_delete */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_delete(c_info, tmp_key);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_dup_ = pmpi_info_dup_
void pmpi_info_dup_(MPI_Fint *info, MPI_Fint *newinfo, int *ierror)
{
/* MPI_Info_dup */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Info c_newinfo;

	*ierror  = MPI_Info_dup(c_info, &c_newinfo);
	*newinfo = PMPI_Info_c2f(c_newinfo);
}

#pragma weak mpi_info_dup__ = pmpi_info_dup__
void pmpi_info_dup__(MPI_Fint *info, MPI_Fint *newinfo, int *ierror)
{
/* MPI_Info_dup */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Info c_newinfo;

	*ierror  = MPI_Info_dup(c_info, &c_newinfo);
	*newinfo = PMPI_Info_c2f(c_newinfo);
}

#pragma weak mpi_info_free_ = pmpi_info_free_
void pmpi_info_free_(MPI_Fint *info, int *ierror)
{
/* MPI_Info_free */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Info_free(&c_info);
	*info   = PMPI_Info_c2f(c_info);
}

#pragma weak mpi_info_free__ = pmpi_info_free__
void pmpi_info_free__(MPI_Fint *info, int *ierror)
{
/* MPI_Info_free */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Info_free(&c_info);
	*info   = PMPI_Info_c2f(c_info);
}

#pragma weak mpi_info_get_ = pmpi_info_get_
void pmpi_info_get_(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *valuelen, char *value CHAR_MIXED(size_value), int *flag, int *ierror CHAR_END(size_key)CHAR_END(size_value) )
{
/* MPI_Info_get */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_get(c_info, tmp_key, *valuelen, value, flag);
	char_c_to_fortran(value, size_value);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_get__ = pmpi_info_get__
void pmpi_info_get__(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *valuelen, char *value CHAR_MIXED(size_value), int *flag, int *ierror CHAR_END(size_key)CHAR_END(size_value) )
{
/* MPI_Info_get */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_get(c_info, tmp_key, *valuelen, value, flag);
	char_c_to_fortran(value, size_value);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_get_nkeys_ = pmpi_info_get_nkeys_
void pmpi_info_get_nkeys_(MPI_Fint *info, int *nkeys, int *ierror)
{
/* MPI_Info_get_nkeys */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Info_get_nkeys(c_info, nkeys);
}

#pragma weak mpi_info_get_nkeys__ = pmpi_info_get_nkeys__
void pmpi_info_get_nkeys__(MPI_Fint *info, int *nkeys, int *ierror)
{
/* MPI_Info_get_nkeys */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Info_get_nkeys(c_info, nkeys);
}

#pragma weak mpi_info_get_nthkey_ = pmpi_info_get_nthkey_
void pmpi_info_get_nthkey_(MPI_Fint *info, int *n, char *key CHAR_MIXED(size_key), int *ierror CHAR_END(size_key) )
{
/* MPI_Info_get_nthkey */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Info_get_nthkey(c_info, *n, key);
	char_c_to_fortran(key, size_key);
}

#pragma weak mpi_info_get_nthkey__ = pmpi_info_get_nthkey__
void pmpi_info_get_nthkey__(MPI_Fint *info, int *n, char *key CHAR_MIXED(size_key), int *ierror CHAR_END(size_key) )
{
/* MPI_Info_get_nthkey */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Info_get_nthkey(c_info, *n, key);
	char_c_to_fortran(key, size_key);
}

#pragma weak mpi_info_get_string_ = pmpi_info_get_string_
void pmpi_info_get_string_(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *buflen, char *value CHAR_MIXED(size_value), int *flag, int *ierror CHAR_END(size_key)CHAR_END(size_value) )
{
/* MPI_Info_get_string */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_get_string(c_info, tmp_key, buflen, value, flag);
	char_c_to_fortran(value, size_value);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_get_string__ = pmpi_info_get_string__
void pmpi_info_get_string__(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *buflen, char *value CHAR_MIXED(size_value), int *flag, int *ierror CHAR_END(size_key)CHAR_END(size_value) )
{
/* MPI_Info_get_string */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_get_string(c_info, tmp_key, buflen, value, flag);
	char_c_to_fortran(value, size_value);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_get_valuelen_ = pmpi_info_get_valuelen_
void pmpi_info_get_valuelen_(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *valuelen, int *flag, int *ierror CHAR_END(size_key) )
{
/* MPI_Info_get_valuelen */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_get_valuelen(c_info, tmp_key, valuelen, flag);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_get_valuelen__ = pmpi_info_get_valuelen__
void pmpi_info_get_valuelen__(MPI_Fint *info, const char *key CHAR_MIXED(size_key), int *valuelen, int *flag, int *ierror CHAR_END(size_key) )
{
/* MPI_Info_get_valuelen */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);

	*ierror = MPI_Info_get_valuelen(c_info, tmp_key, valuelen, flag);
	sctk_free(ptr_key);
}

#pragma weak mpi_info_set_ = pmpi_info_set_
void pmpi_info_set_(MPI_Fint *info, const char *key CHAR_MIXED(size_key), const char *value CHAR_MIXED(size_value), int *ierror CHAR_END(size_key)CHAR_END(size_value) )
{
/* MPI_Info_set */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);
	char *tmp_value = NULL, *ptr_value = NULL;
	tmp_value = char_fortran_to_c( (char *)value, size_value, &ptr_value);

	*ierror = MPI_Info_set(c_info, tmp_key, tmp_value);
	sctk_free(ptr_key);
	sctk_free(ptr_value);
}

#pragma weak mpi_info_set__ = pmpi_info_set__
void pmpi_info_set__(MPI_Fint *info, const char *key CHAR_MIXED(size_key), const char *value CHAR_MIXED(size_value), int *ierror CHAR_END(size_key)CHAR_END(size_value) )
{
/* MPI_Info_set */
	MPI_Info c_info  = PMPI_Info_f2c(*info);
	char *   tmp_key = NULL, *ptr_key = NULL;

	tmp_key = char_fortran_to_c( (char *)key, size_key, &ptr_key);
	char *tmp_value = NULL, *ptr_value = NULL;
	tmp_value = char_fortran_to_c( (char *)value, size_value, &ptr_value);

	*ierror = MPI_Info_set(c_info, tmp_key, tmp_value);
	sctk_free(ptr_key);
	sctk_free(ptr_value);
}

#pragma weak mpi_init_ = pmpi_init_
void pmpi_init_(int *ierror)
{
	int *   argc = NULL;
	char ***argv = NULL;

/* MPI_Init */

	*ierror = MPI_Init(argc, argv);
}

#pragma weak mpi_init__ = pmpi_init__
void pmpi_init__(int *ierror)
{
	int *   argc = NULL;
	char ***argv = NULL;

/* MPI_Init */

	*ierror = MPI_Init(argc, argv);
}

#pragma weak mpi_init_thread_ = pmpi_init_thread_
void pmpi_init_thread_(int *required, int *provided, int *ierror)
{
	int *   argc = NULL;
	char ***argv = NULL;

/* MPI_Init_thread */

	*ierror = MPI_Init_thread(argc, argv, *required, provided);
}

#pragma weak mpi_init_thread__ = pmpi_init_thread__
void pmpi_init_thread__(int *required, int *provided, int *ierror)
{
	int *   argc = NULL;
	char ***argv = NULL;

/* MPI_Init_thread */

	*ierror = MPI_Init_thread(argc, argv, *required, provided);
}

#pragma weak mpi_initialized_ = pmpi_initialized_
void pmpi_initialized_(int *flag, int *ierror)
{
/* MPI_Initialized */

	*ierror = MPI_Initialized(flag);
}

#pragma weak mpi_initialized__ = pmpi_initialized__
void pmpi_initialized__(int *flag, int *ierror)
{
/* MPI_Initialized */

	*ierror = MPI_Initialized(flag);
}

#pragma weak mpi_intercomm_create_ = pmpi_intercomm_create_
void pmpi_intercomm_create_(MPI_Fint *local_comm, int *local_leader, MPI_Fint *peer_comm, int *remote_leader, int *tag, MPI_Fint *newintercomm, int *ierror)
{
/* MPI_Intercomm_create */
	MPI_Comm c_local_comm = PMPI_Comm_f2c(*local_comm);
	MPI_Comm c_peer_comm  = PMPI_Comm_f2c(*peer_comm);
	MPI_Comm c_newintercomm;

	*ierror       = MPI_Intercomm_create(c_local_comm, *local_leader, c_peer_comm, *remote_leader, *tag, &c_newintercomm);
    if( *ierror == MPI_SUCCESS) {
        *newintercomm = PMPI_Comm_c2f(c_newintercomm);
    }
}

#pragma weak mpi_intercomm_create__ = pmpi_intercomm_create__
void pmpi_intercomm_create__(MPI_Fint *local_comm, int *local_leader, MPI_Fint *peer_comm, int *remote_leader, int *tag, MPI_Fint *newintercomm, int *ierror)
{
/* MPI_Intercomm_create */
	MPI_Comm c_local_comm = PMPI_Comm_f2c(*local_comm);
	MPI_Comm c_peer_comm  = PMPI_Comm_f2c(*peer_comm);
	MPI_Comm c_newintercomm;

	*ierror       = MPI_Intercomm_create(c_local_comm, *local_leader, c_peer_comm, *remote_leader, *tag, &c_newintercomm);
    if( *ierror == MPI_SUCCESS) {
        *newintercomm = PMPI_Comm_c2f(c_newintercomm);
    }
}

#pragma weak mpi_intercomm_merge_ = pmpi_intercomm_merge_
void pmpi_intercomm_merge_(MPI_Fint *intercomm, int *high, MPI_Fint *newintracomm, int *ierror)
{
/* MPI_Intercomm_merge */
	MPI_Comm c_intercomm = PMPI_Comm_f2c(*intercomm);
	MPI_Comm c_newintracomm;

    *ierror       = MPI_Intercomm_merge(c_intercomm, *high, &c_newintracomm);
    if( *ierror == MPI_SUCCESS) {
        *newintracomm = PMPI_Comm_c2f(c_newintracomm);
    }
}

#pragma weak mpi_intercomm_merge__ = pmpi_intercomm_merge__
void pmpi_intercomm_merge__(MPI_Fint *intercomm, int *high, MPI_Fint *newintracomm, int *ierror)
{
/* MPI_Intercomm_merge */
	MPI_Comm c_intercomm = PMPI_Comm_f2c(*intercomm);
	MPI_Comm c_newintracomm;

	*ierror       = MPI_Intercomm_merge(c_intercomm, *high, &c_newintracomm);
    if( *ierror == MPI_SUCCESS) {
        *newintracomm = PMPI_Comm_c2f(c_newintracomm);
    }
}

#pragma weak mpi_iprobe_ = pmpi_iprobe_
void pmpi_iprobe_(int *source, int *tag, MPI_Fint *comm, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Iprobe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Iprobe(*source, *tag, c_comm, flag, status);
}

#pragma weak mpi_iprobe__ = pmpi_iprobe__
void pmpi_iprobe__(int *source, int *tag, MPI_Fint *comm, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Iprobe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Iprobe(*source, *tag, c_comm, flag, status);
}

#pragma weak mpi_irecv_ = pmpi_irecv_
void pmpi_irecv_(void *buf, int *count, MPI_Fint *datatype, int *source, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Irecv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Irecv(buf, *count, c_datatype, *source, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_irecv__ = pmpi_irecv__
void pmpi_irecv__(void *buf, int *count, MPI_Fint *datatype, int *source, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Irecv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Irecv(buf, *count, c_datatype, *source, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ireduce_ = pmpi_ireduce_
void pmpi_ireduce_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ireduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ireduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ireduce__ = pmpi_ireduce__
void pmpi_ireduce__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ireduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ireduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ireduce_scatter_ = pmpi_ireduce_scatter_
void pmpi_ireduce_scatter_(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ireduce_scatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ireduce_scatter__ = pmpi_ireduce_scatter__
void pmpi_ireduce_scatter__(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ireduce_scatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ireduce_scatter_block_ = pmpi_ireduce_scatter_block_
void pmpi_ireduce_scatter_block_(const void *sendbuf, void *recvbuf, int *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ireduce_scatter_block */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ireduce_scatter_block(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ireduce_scatter_block__ = pmpi_ireduce_scatter_block__
void pmpi_ireduce_scatter_block__(const void *sendbuf, void *recvbuf, int *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ireduce_scatter_block */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ireduce_scatter_block(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_irsend_ = pmpi_irsend_
void pmpi_irsend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Irsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Irsend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_irsend__ = pmpi_irsend__
void pmpi_irsend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Irsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Irsend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_is_thread_main_ = pmpi_is_thread_main_
void pmpi_is_thread_main_(int *flag, int *ierror)
{
/* MPI_Is_thread_main */

	*ierror = MPI_Is_thread_main(flag);
}

#pragma weak mpi_is_thread_main__ = pmpi_is_thread_main__
void pmpi_is_thread_main__(int *flag, int *ierror)
{
/* MPI_Is_thread_main */

	*ierror = MPI_Is_thread_main(flag);
}

#pragma weak mpi_iscan_ = pmpi_iscan_
void pmpi_iscan_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iscan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iscan__ = pmpi_iscan__
void pmpi_iscan__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iscan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iscatter_ = pmpi_iscatter_
void pmpi_iscatter_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iscatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iscatter(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iscatter__ = pmpi_iscatter__
void pmpi_iscatter__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iscatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iscatter(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iscatterv_ = pmpi_iscatterv_
void pmpi_iscatterv_(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iscatterv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iscatterv(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_iscatterv__ = pmpi_iscatterv__
void pmpi_iscatterv__(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Iscatterv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Iscatterv(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_isend_ = pmpi_isend_
void pmpi_isend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Isend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Isend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_isend__ = pmpi_isend__
void pmpi_isend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Isend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Isend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_issend_ = pmpi_issend_
void pmpi_issend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Issend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Issend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_issend__ = pmpi_issend__
void pmpi_issend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Issend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Issend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_keyval_create_ = pmpi_keyval_create_
void pmpi_keyval_create_(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state, int *ierror)
{
/* MPI_Keyval_create */

    *keyval = -1; /**< Denotes fortran functions */
	*ierror = MPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

#pragma weak mpi_keyval_create__ = pmpi_keyval_create__
void pmpi_keyval_create__(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state, int *ierror)
{
/* MPI_Keyval_create */

    *keyval = -1; /**< Denotes fortran functions */
	*ierror = MPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

#pragma weak mpi_keyval_free_ = pmpi_keyval_free_
void pmpi_keyval_free_(int *keyval, int *ierror)
{
/* MPI_Keyval_free */

	*ierror = MPI_Keyval_free(keyval);
}

#pragma weak mpi_keyval_free__ = pmpi_keyval_free__
void pmpi_keyval_free__(int *keyval, int *ierror)
{
/* MPI_Keyval_free */

	*ierror = MPI_Keyval_free(keyval);
}

#pragma weak mpi_lookup_name_ = pmpi_lookup_name_
void pmpi_lookup_name_(const char *service_name CHAR_MIXED(size_service_name), MPI_Fint *info, char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_service_name)CHAR_END(size_port_name) )
{
/* MPI_Lookup_name */
	char *tmp_service_name = NULL, *ptr_service_name = NULL;

	tmp_service_name = char_fortran_to_c( (char *)service_name, size_service_name, &ptr_service_name);
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Lookup_name(tmp_service_name, c_info, port_name);
	char_c_to_fortran(port_name, size_port_name);
	sctk_free(ptr_service_name);
}

#pragma weak mpi_lookup_name__ = pmpi_lookup_name__
void pmpi_lookup_name__(const char *service_name CHAR_MIXED(size_service_name), MPI_Fint *info, char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_service_name)CHAR_END(size_port_name) )
{
/* MPI_Lookup_name */
	char *tmp_service_name = NULL, *ptr_service_name = NULL;

	tmp_service_name = char_fortran_to_c( (char *)service_name, size_service_name, &ptr_service_name);
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Lookup_name(tmp_service_name, c_info, port_name);
	char_c_to_fortran(port_name, size_port_name);
	sctk_free(ptr_service_name);
}

#pragma weak mpi_mprobe_ = pmpi_mprobe_
void pmpi_mprobe_(int *source, int *tag, MPI_Fint *comm, MPI_Message *message, MPI_Status *status, int *ierror)
{
/* MPI_Mprobe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Mprobe(*source, *tag, c_comm, message, status);
}

#pragma weak mpi_mprobe__ = pmpi_mprobe__
void pmpi_mprobe__(int *source, int *tag, MPI_Fint *comm, MPI_Message *message, MPI_Status *status, int *ierror)
{
/* MPI_Mprobe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Mprobe(*source, *tag, c_comm, message, status);
}

#pragma weak mpi_mrecv_ = pmpi_mrecv_
void pmpi_mrecv_(void *buf, int *count, MPI_Fint *datatype, MPI_Message *message, MPI_Status *status, int *ierror)
{
/* MPI_Mrecv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Mrecv(buf, *count, c_datatype, message, status);
}

#pragma weak mpi_mrecv__ = pmpi_mrecv__
void pmpi_mrecv__(void *buf, int *count, MPI_Fint *datatype, MPI_Message *message, MPI_Status *status, int *ierror)
{
/* MPI_Mrecv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Mrecv(buf, *count, c_datatype, message, status);
}

#pragma weak mpi_neighbor_allgather_ = pmpi_neighbor_allgather_
void pmpi_neighbor_allgather_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_allgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_allgather__ = pmpi_neighbor_allgather__
void pmpi_neighbor_allgather__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_allgather */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_allgather_init_ = pmpi_neighbor_allgather_init_
void pmpi_neighbor_allgather_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_allgather_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_allgather_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_allgather_init__ = pmpi_neighbor_allgather_init__
void pmpi_neighbor_allgather_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_allgather_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_allgather_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_allgatherv_ = pmpi_neighbor_allgatherv_
void pmpi_neighbor_allgatherv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_allgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_allgatherv__ = pmpi_neighbor_allgatherv__
void pmpi_neighbor_allgatherv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_allgatherv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_allgatherv_init_ = pmpi_neighbor_allgatherv_init_
void pmpi_neighbor_allgatherv_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_allgatherv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_allgatherv_init(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_allgatherv_init__ = pmpi_neighbor_allgatherv_init__
void pmpi_neighbor_allgatherv_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_allgatherv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_allgatherv_init(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_alltoall_ = pmpi_neighbor_alltoall_
void pmpi_neighbor_alltoall_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_alltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_alltoall__ = pmpi_neighbor_alltoall__
void pmpi_neighbor_alltoall__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_alltoall */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_alltoall_init_ = pmpi_neighbor_alltoall_init_
void pmpi_neighbor_alltoall_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_alltoall_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_alltoall_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_alltoall_init__ = pmpi_neighbor_alltoall_init__
void pmpi_neighbor_alltoall_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_alltoall_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_alltoall_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_alltoallv_ = pmpi_neighbor_alltoallv_
void pmpi_neighbor_alltoallv_(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_alltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_alltoallv__ = pmpi_neighbor_alltoallv__
void pmpi_neighbor_alltoallv__(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_alltoallv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm);
}

#pragma weak mpi_neighbor_alltoallv_init_ = pmpi_neighbor_alltoallv_init_
void pmpi_neighbor_alltoallv_init_(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_alltoallv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_alltoallv_init(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_alltoallv_init__ = pmpi_neighbor_alltoallv_init__
void pmpi_neighbor_alltoallv_init__(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Fint *sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_alltoallv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Neighbor_alltoallv_init(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_neighbor_alltoallw_ = pmpi_neighbor_alltoallw_
void pmpi_neighbor_alltoallw_(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_alltoallw */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}


	*ierror = MPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_neighbor_alltoallw__ = pmpi_neighbor_alltoallw__
void pmpi_neighbor_alltoallw__(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, int *ierror)
{
/* MPI_Neighbor_alltoallw */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}


	*ierror = MPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_neighbor_alltoallw_init_ = pmpi_neighbor_alltoallw_init_
void pmpi_neighbor_alltoallw_init_(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_alltoallw_init */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Request c_request;

	*ierror  = MPI_Neighbor_alltoallw_init(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_neighbor_alltoallw_init__ = pmpi_neighbor_alltoallw_init__
void pmpi_neighbor_alltoallw_init__(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Fint sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Fint recvtypes[], MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Neighbor_alltoallw_init */
	MPI_Comm    c_comm = PMPI_Comm_f2c(*comm);
	int alltoallwlen = 0;

	PMPI_Comm_size(c_comm, &alltoallwlen);
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}

	int           incnt_sendtypes = 0;
	MPI_Datatype *c_sendtypes     = NULL;

	c_sendtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_sendtypes = 0; incnt_sendtypes < alltoallwlen; incnt_sendtypes++)
	{
		c_sendtypes[incnt_sendtypes] = PMPI_Type_f2c(sendtypes[incnt_sendtypes]);
	}

	int           incnt_recvtypes = 0;
	MPI_Datatype *c_recvtypes     = NULL;

	c_recvtypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * alltoallwlen);

	for(incnt_recvtypes = 0; incnt_recvtypes < alltoallwlen; incnt_recvtypes++)
	{
		c_recvtypes[incnt_recvtypes] = PMPI_Type_f2c(recvtypes[incnt_recvtypes]);
	}

	MPI_Info    c_info = PMPI_Info_f2c(*info);
	MPI_Request c_request;

	*ierror  = MPI_Neighbor_alltoallw_init(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
	sctk_free(c_sendtypes);
	sctk_free(c_recvtypes);
}

#pragma weak mpi_op_commutative_ = pmpi_op_commutative_
void pmpi_op_commutative_(MPI_Fint *op, int *commute, int *ierror)
{
/* MPI_Op_commutative */
	MPI_Op c_op = PMPI_Op_f2c(*op);

	*ierror = MPI_Op_commutative(c_op, commute);
}

#pragma weak mpi_op_commutative__ = pmpi_op_commutative__
void pmpi_op_commutative__(MPI_Fint *op, int *commute, int *ierror)
{
/* MPI_Op_commutative */
	MPI_Op c_op = PMPI_Op_f2c(*op);

	*ierror = MPI_Op_commutative(c_op, commute);
}

#pragma weak mpi_op_create_ = pmpi_op_create_
void pmpi_op_create_(MPI_User_function *user_fn, int *commute, MPI_Fint *op, int *ierror)
{
/* MPI_Op_create */
	MPI_Op c_op;

	*ierror = MPI_Op_create(user_fn, *commute, &c_op);
	*op     = PMPI_Op_c2f(c_op);
}

#pragma weak mpi_op_create__ = pmpi_op_create__
void pmpi_op_create__(MPI_User_function *user_fn, int *commute, MPI_Fint *op, int *ierror)
{
/* MPI_Op_create */
	MPI_Op c_op;

	*ierror = MPI_Op_create(user_fn, *commute, &c_op);
	*op     = PMPI_Op_c2f(c_op);
}

#pragma weak mpi_op_free_ = pmpi_op_free_
void pmpi_op_free_(MPI_Fint *op, int *ierror)
{
/* MPI_Op_free */
	MPI_Op c_op = PMPI_Op_f2c(*op);

	*ierror = MPI_Op_free(&c_op);
	*op     = PMPI_Op_c2f(c_op);
}

#pragma weak mpi_op_free__ = pmpi_op_free__
void pmpi_op_free__(MPI_Fint *op, int *ierror)
{
/* MPI_Op_free */
	MPI_Op c_op = PMPI_Op_f2c(*op);

	*ierror = MPI_Op_free(&c_op);
	*op     = PMPI_Op_c2f(c_op);
}

#pragma weak mpi_open_port_ = pmpi_open_port_
void pmpi_open_port_(MPI_Fint *info, char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_port_name) )
{
/* MPI_Open_port */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Open_port(c_info, port_name);
	char_c_to_fortran(port_name, size_port_name);
}

#pragma weak mpi_open_port__ = pmpi_open_port__
void pmpi_open_port__(MPI_Fint *info, char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_port_name) )
{
/* MPI_Open_port */
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Open_port(c_info, port_name);
	char_c_to_fortran(port_name, size_port_name);
}

#pragma weak mpi_pack_ = pmpi_pack_
void pmpi_pack_(const void *inbuf, int *incount, MPI_Fint *datatype, void *outbuf, int *outsize, int *position, MPI_Fint *comm, int *ierror)
{
/* MPI_Pack */
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Pack(inbuf, *incount, c_datatype, outbuf, *outsize, position, c_comm);
}

#pragma weak mpi_pack__ = pmpi_pack__
void pmpi_pack__(const void *inbuf, int *incount, MPI_Fint *datatype, void *outbuf, int *outsize, int *position, MPI_Fint *comm, int *ierror)
{
/* MPI_Pack */
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Pack(inbuf, *incount, c_datatype, outbuf, *outsize, position, c_comm);
}

#pragma weak mpi_pack_external_ = pmpi_pack_external_
void pmpi_pack_external_(const char datarep[] CHAR_MIXED(size_datarep), const void *inbuf, int *incount, MPI_Fint *datatype, void *outbuf, MPI_Aint *outsize, MPI_Aint *position, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Pack_external */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Pack_external(tmp_datarep, inbuf, *incount, c_datatype, outbuf, *outsize, position);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_pack_external__ = pmpi_pack_external__
void pmpi_pack_external__(const char datarep[] CHAR_MIXED(size_datarep), const void *inbuf, int *incount, MPI_Fint *datatype, void *outbuf, MPI_Aint *outsize, MPI_Aint *position, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Pack_external */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Pack_external(tmp_datarep, inbuf, *incount, c_datatype, outbuf, *outsize, position);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_pack_external_size_ = pmpi_pack_external_size_
void pmpi_pack_external_size_(const char datarep[] CHAR_MIXED(size_datarep), int *incount, MPI_Fint *datatype, MPI_Aint *size, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Pack_external_size */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Pack_external_size(tmp_datarep, *incount, c_datatype, size);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_pack_external_size__ = pmpi_pack_external_size__
void pmpi_pack_external_size__(const char datarep[] CHAR_MIXED(size_datarep), int *incount, MPI_Fint *datatype, MPI_Aint *size, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Pack_external_size */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Pack_external_size(tmp_datarep, *incount, c_datatype, size);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_pack_size_ = pmpi_pack_size_
void pmpi_pack_size_(int *incount, MPI_Fint *datatype, MPI_Fint *comm, int *size, int *ierror)
{
/* MPI_Pack_size */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Pack_size(*incount, c_datatype, c_comm, size);
}

#pragma weak mpi_pack_size__ = pmpi_pack_size__
void pmpi_pack_size__(int *incount, MPI_Fint *datatype, MPI_Fint *comm, int *size, int *ierror)
{
/* MPI_Pack_size */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Pack_size(*incount, c_datatype, c_comm, size);
}

#pragma weak mpi_probe_ = pmpi_probe_
void pmpi_probe_(int *source, int *tag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Probe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Probe(*source, *tag, c_comm, status);
}

#pragma weak mpi_probe__ = pmpi_probe__
void pmpi_probe__(int *source, int *tag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Probe */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Probe(*source, *tag, c_comm, status);
}

#pragma weak mpi_publish_name_ = pmpi_publish_name_
void pmpi_publish_name_(const char *service_name CHAR_MIXED(size_service_name), MPI_Fint *info, const char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_service_name)CHAR_END(size_port_name) )
{
/* MPI_Publish_name */
	char *tmp_service_name = NULL, *ptr_service_name = NULL;

	tmp_service_name = char_fortran_to_c( (char *)service_name, size_service_name, &ptr_service_name);
	MPI_Info c_info        = PMPI_Info_f2c(*info);
	char *   tmp_port_name = NULL, *ptr_port_name = NULL;
	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);

	*ierror = MPI_Publish_name(tmp_service_name, c_info, tmp_port_name);
	sctk_free(ptr_service_name);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_publish_name__ = pmpi_publish_name__
void pmpi_publish_name__(const char *service_name CHAR_MIXED(size_service_name), MPI_Fint *info, const char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_service_name)CHAR_END(size_port_name) )
{
/* MPI_Publish_name */
	char *tmp_service_name = NULL, *ptr_service_name = NULL;

	tmp_service_name = char_fortran_to_c( (char *)service_name, size_service_name, &ptr_service_name);
	MPI_Info c_info        = PMPI_Info_f2c(*info);
	char *   tmp_port_name = NULL, *ptr_port_name = NULL;
	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);

	*ierror = MPI_Publish_name(tmp_service_name, c_info, tmp_port_name);
	sctk_free(ptr_service_name);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_put_ = pmpi_put_
void pmpi_put_(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, int *ierror)
{
/* MPI_Put */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);

	*ierror = MPI_Put(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win);
}

#pragma weak mpi_put__ = pmpi_put__
void pmpi_put__(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, int *ierror)
{
/* MPI_Put */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);

	*ierror = MPI_Put(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win);
}

#pragma weak mpi_query_thread_ = pmpi_query_thread_
void pmpi_query_thread_(int *provided, int *ierror)
{
/* MPI_Query_thread */

	*ierror = MPI_Query_thread(provided);
}

#pragma weak mpi_query_thread__ = pmpi_query_thread__
void pmpi_query_thread__(int *provided, int *ierror)
{
/* MPI_Query_thread */

	*ierror = MPI_Query_thread(provided);
}

#pragma weak mpi_raccumulate_ = pmpi_raccumulate_
void pmpi_raccumulate_(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Raccumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Raccumulate(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_raccumulate__ = pmpi_raccumulate__
void pmpi_raccumulate__(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Raccumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Raccumulate(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_recv_ = pmpi_recv_
void pmpi_recv_(void *buf, int *count, MPI_Fint *datatype, int *source, int *tag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Recv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Recv(buf, *count, c_datatype, *source, *tag, c_comm, status);
}

#pragma weak mpi_recv__ = pmpi_recv__
void pmpi_recv__(void *buf, int *count, MPI_Fint *datatype, int *source, int *tag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Recv */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Recv(buf, *count, c_datatype, *source, *tag, c_comm, status);
}

#pragma weak mpi_recv_init_ = pmpi_recv_init_
void pmpi_recv_init_(void *buf, int *count, MPI_Fint *datatype, int *source, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Recv_init */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Recv_init(buf, *count, c_datatype, *source, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_recv_init__ = pmpi_recv_init__
void pmpi_recv_init__(void *buf, int *count, MPI_Fint *datatype, int *source, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Recv_init */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Recv_init(buf, *count, c_datatype, *source, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_reduce_ = pmpi_reduce_
void pmpi_reduce_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Reduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Reduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm);
}

#pragma weak mpi_reduce__ = pmpi_reduce__
void pmpi_reduce__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Reduce */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Reduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm);
}

#pragma weak mpi_reduce_init_ = pmpi_reduce_init_
void pmpi_reduce_init_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Reduce_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Reduce_init(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_reduce_init__ = pmpi_reduce_init__
void pmpi_reduce_init__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Reduce_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Reduce_init(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_reduce_local_ = pmpi_reduce_local_
void pmpi_reduce_local_(const void *inbuf, void *inoutbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *ierror)
{
/* MPI_Reduce_local */
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)inbuf) )
	{
		inbuf = MPI_IN_PLACE;
	}
	if(buffer_is_bottom( (void *)inoutbuf) )
	{
		inoutbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)inoutbuf) )
	{
		inoutbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);

	*ierror = MPI_Reduce_local(inbuf, inoutbuf, *count, c_datatype, c_op);
}

#pragma weak mpi_reduce_local__ = pmpi_reduce_local__
void pmpi_reduce_local__(const void *inbuf, void *inoutbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, int *ierror)
{
/* MPI_Reduce_local */
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)inbuf) )
	{
		inbuf = MPI_IN_PLACE;
	}
	if(buffer_is_bottom( (void *)inoutbuf) )
	{
		inoutbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)inoutbuf) )
	{
		inoutbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);

	*ierror = MPI_Reduce_local(inbuf, inoutbuf, *count, c_datatype, c_op);
}

#pragma weak mpi_reduce_scatter_ = pmpi_reduce_scatter_
void pmpi_reduce_scatter_(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Reduce_scatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm);
}

#pragma weak mpi_reduce_scatter__ = pmpi_reduce_scatter__
void pmpi_reduce_scatter__(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Reduce_scatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm);
}

#pragma weak mpi_reduce_scatter_block_ = pmpi_reduce_scatter_block_
void pmpi_reduce_scatter_block_(const void *sendbuf, void *recvbuf, int *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Reduce_scatter_block */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Reduce_scatter_block(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm);
}

#pragma weak mpi_reduce_scatter_block__ = pmpi_reduce_scatter_block__
void pmpi_reduce_scatter_block__(const void *sendbuf, void *recvbuf, int *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Reduce_scatter_block */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Reduce_scatter_block(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm);
}

#pragma weak mpi_reduce_scatter_block_init_ = pmpi_reduce_scatter_block_init_
void pmpi_reduce_scatter_block_init_(const void *sendbuf, void *recvbuf, int *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Reduce_scatter_block_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Reduce_scatter_block_init(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_reduce_scatter_block_init__ = pmpi_reduce_scatter_block_init__
void pmpi_reduce_scatter_block_init__(const void *sendbuf, void *recvbuf, int *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Reduce_scatter_block_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Reduce_scatter_block_init(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_reduce_scatter_init_ = pmpi_reduce_scatter_init_
void pmpi_reduce_scatter_init_(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Reduce_scatter_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Reduce_scatter_init(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_reduce_scatter_init__ = pmpi_reduce_scatter_init__
void pmpi_reduce_scatter_init__(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Reduce_scatter_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Reduce_scatter_init(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_register_datarep_ = pmpi_register_datarep_
void pmpi_register_datarep_(const char *datarep CHAR_MIXED(size_datarep), MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Register_datarep */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);

	*ierror = MPI_Register_datarep(tmp_datarep, read_conversion_fn, write_conversion_fn, dtype_file_extent_fn, extra_state);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_register_datarep__ = pmpi_register_datarep__
void pmpi_register_datarep__(const char *datarep CHAR_MIXED(size_datarep), MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Register_datarep */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);

	*ierror = MPI_Register_datarep(tmp_datarep, read_conversion_fn, write_conversion_fn, dtype_file_extent_fn, extra_state);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_request_free_ = pmpi_request_free_
void pmpi_request_free_(MPI_Fint *request, int *ierror)
{
/* MPI_Request_free */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Request_free(&c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_request_free__ = pmpi_request_free__
void pmpi_request_free__(MPI_Fint *request, int *ierror)
{
/* MPI_Request_free */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Request_free(&c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_request_get_status_ = pmpi_request_get_status_
void pmpi_request_get_status_(MPI_Fint *request, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Request_get_status */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror = MPI_Request_get_status(c_request, flag, status);
}

#pragma weak mpi_request_get_status__ = pmpi_request_get_status__
void pmpi_request_get_status__(MPI_Fint *request, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Request_get_status */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror = MPI_Request_get_status(c_request, flag, status);
}

#pragma weak mpi_rget_ = pmpi_rget_
void pmpi_rget_(void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Rget */
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Rget(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rget__ = pmpi_rget__
void pmpi_rget__(void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Rget */
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Rget(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rget_accumulate_ = pmpi_rget_accumulate_
void pmpi_rget_accumulate_(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, void *result_addr, int *result_count, MPI_Fint *result_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Rget_accumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_result_datatype = PMPI_Type_f2c(*result_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Rget_accumulate(origin_addr, *origin_count, c_origin_datatype, result_addr, *result_count, c_result_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rget_accumulate__ = pmpi_rget_accumulate__
void pmpi_rget_accumulate__(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, void *result_addr, int *result_count, MPI_Fint *result_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *op, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Rget_accumulate */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_result_datatype = PMPI_Type_f2c(*result_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Op       c_op  = PMPI_Op_f2c(*op);
	MPI_Win      c_win = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Rget_accumulate(origin_addr, *origin_count, c_origin_datatype, result_addr, *result_count, c_result_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_op, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rput_ = pmpi_rput_
void pmpi_rput_(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Rput */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Rput(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rput__ = pmpi_rput__
void pmpi_rput__(const void *origin_addr, int *origin_count, MPI_Fint *origin_datatype, int *target_rank, MPI_Aint *target_disp, int *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *request, int *ierror)
{
/* MPI_Rput */
	if(buffer_is_bottom( (void *)origin_addr) )
	{
		origin_addr = MPI_BOTTOM;
	}
	MPI_Datatype c_origin_datatype = PMPI_Type_f2c(*origin_datatype);
	MPI_Datatype c_target_datatype = PMPI_Type_f2c(*target_datatype);
	MPI_Win      c_win             = PMPI_Win_f2c(*win);
	MPI_Request  c_request;

	*ierror  = MPI_Rput(origin_addr, *origin_count, c_origin_datatype, *target_rank, *target_disp, *target_count, c_target_datatype, c_win, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rsend_ = pmpi_rsend_
void pmpi_rsend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Rsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Rsend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_rsend__ = pmpi_rsend__
void pmpi_rsend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Rsend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Rsend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_rsend_init_ = pmpi_rsend_init_
void pmpi_rsend_init_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Rsend_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Rsend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_rsend_init__ = pmpi_rsend_init__
void pmpi_rsend_init__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Rsend_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Rsend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_scan_ = pmpi_scan_
void pmpi_scan_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Scan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Scan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

#pragma weak mpi_scan__ = pmpi_scan__
void pmpi_scan__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, int *ierror)
{
/* MPI_Scan */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Scan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

#pragma weak mpi_scan_init_ = pmpi_scan_init_
void pmpi_scan_init_(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Scan_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Scan_init(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_scan_init__ = pmpi_scan_init__
void pmpi_scan_init__(const void *sendbuf, void *recvbuf, int *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Scan_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Op       c_op       = PMPI_Op_f2c(*op);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Scan_init(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_scatter_ = pmpi_scatter_
void pmpi_scatter_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Scatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Scatter(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

#pragma weak mpi_scatter__ = pmpi_scatter__
void pmpi_scatter__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Scatter */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Scatter(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

#pragma weak mpi_scatter_init_ = pmpi_scatter_init_
void pmpi_scatter_init_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Scatter_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Scatter_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_scatter_init__ = pmpi_scatter_init__
void pmpi_scatter_init__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Scatter_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Scatter_init(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_scatterv_ = pmpi_scatterv_
void pmpi_scatterv_(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Scatterv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Scatterv(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

#pragma weak mpi_scatterv__ = pmpi_scatterv__
void pmpi_scatterv__(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, int *ierror)
{
/* MPI_Scatterv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Scatterv(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

#pragma weak mpi_scatterv_init_ = pmpi_scatterv_init_
void pmpi_scatterv_init_(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Scatterv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Scatterv_init(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_scatterv_init__ = pmpi_scatterv_init__
void pmpi_scatterv_init__(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Fint *sendtype, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *root, MPI_Fint *comm, MPI_Fint *info, MPI_Fint *request, int *ierror)
{
/* MPI_Scatterv_init */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	if(buffer_is_mpiinplace( (void *)sendbuf) )
	{
		sendbuf = MPI_IN_PLACE;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Info     c_info     = PMPI_Info_f2c(*info);
	MPI_Request  c_request;

	*ierror  = MPI_Scatterv_init(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, c_info, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_send_ = pmpi_send_
void pmpi_send_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Send */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Send(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_send__ = pmpi_send__
void pmpi_send__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Send */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Send(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_send_init_ = pmpi_send_init_
void pmpi_send_init_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Send_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Send_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_send_init__ = pmpi_send_init__
void pmpi_send_init__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Send_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Send_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_sendrecv_ = pmpi_sendrecv_
void pmpi_sendrecv_(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, int *dest, int *sendtag, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *source, int *recvtag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Sendrecv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Sendrecv(sendbuf, *sendcount, c_sendtype, *dest, *sendtag, recvbuf, *recvcount, c_recvtype, *source, *recvtag, c_comm, status);
}

#pragma weak mpi_sendrecv__ = pmpi_sendrecv__
void pmpi_sendrecv__(const void *sendbuf, int *sendcount, MPI_Fint *sendtype, int *dest, int *sendtag, void *recvbuf, int *recvcount, MPI_Fint *recvtype, int *source, int *recvtag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Sendrecv */
	if(buffer_is_bottom( (void *)sendbuf) )
	{
		sendbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
	MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Sendrecv(sendbuf, *sendcount, c_sendtype, *dest, *sendtag, recvbuf, *recvcount, c_recvtype, *source, *recvtag, c_comm, status);
}

#pragma weak mpi_sendrecv_replace_ = pmpi_sendrecv_replace_
void pmpi_sendrecv_replace_(void *buf, int *count, MPI_Fint *datatype, int *dest, int *sendtag, int *source, int *recvtag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Sendrecv_replace */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Sendrecv_replace(buf, *count, c_datatype, *dest, *sendtag, *source, *recvtag, c_comm, status);
}

#pragma weak mpi_sendrecv_replace__ = pmpi_sendrecv_replace__
void pmpi_sendrecv_replace__(void *buf, int *count, MPI_Fint *datatype, int *dest, int *sendtag, int *source, int *recvtag, MPI_Fint *comm, MPI_Status *status, int *ierror)
{
/* MPI_Sendrecv_replace */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Sendrecv_replace(buf, *count, c_datatype, *dest, *sendtag, *source, *recvtag, c_comm, status);
}

#pragma weak mpi_ssend_ = pmpi_ssend_
void pmpi_ssend_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Ssend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Ssend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_ssend__ = pmpi_ssend__
void pmpi_ssend__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, int *ierror)
{
/* MPI_Ssend */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Ssend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

#pragma weak mpi_ssend_init_ = pmpi_ssend_init_
void pmpi_ssend_init_(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ssend_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ssend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_ssend_init__ = pmpi_ssend_init__
void pmpi_ssend_init__(const void *buf, int *count, MPI_Fint *datatype, int *dest, int *tag, MPI_Fint *comm, MPI_Fint *request, int *ierror)
{
/* MPI_Ssend_init */
	if(buffer_is_bottom( (void *)buf) )
	{
		buf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);
	MPI_Request  c_request;

	*ierror  = MPI_Ssend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_start_ = pmpi_start_
void pmpi_start_(MPI_Fint *request, int *ierror)
{
/* MPI_Start */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Start(&c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_start__ = pmpi_start__
void pmpi_start__(MPI_Fint *request, int *ierror)
{
/* MPI_Start */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Start(&c_request);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_startall_ = pmpi_startall_
void pmpi_startall_(int *count, MPI_Fint array_of_requests[], int *ierror)
{
/* MPI_Startall */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Startall(*count, c_array_of_requests);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_startall__ = pmpi_startall__
void pmpi_startall__(int *count, MPI_Fint array_of_requests[], int *ierror)
{
/* MPI_Startall */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Startall(*count, c_array_of_requests);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_status_set_cancelled_ = pmpi_status_set_cancelled_
void pmpi_status_set_cancelled_(MPI_Status *status, int *flag, int *ierror)
{
/* MPI_Status_set_cancelled */

	*ierror = MPI_Status_set_cancelled(status, *flag);
}

#pragma weak mpi_status_set_cancelled__ = pmpi_status_set_cancelled__
void pmpi_status_set_cancelled__(MPI_Status *status, int *flag, int *ierror)
{
/* MPI_Status_set_cancelled */

	*ierror = MPI_Status_set_cancelled(status, *flag);
}

#pragma weak mpi_status_set_elements_ = pmpi_status_set_elements_
void pmpi_status_set_elements_(MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Status_set_elements */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Status_set_elements(status, c_datatype, *count);
}

#pragma weak mpi_status_set_elements__ = pmpi_status_set_elements__
void pmpi_status_set_elements__(MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Status_set_elements */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Status_set_elements(status, c_datatype, *count);
}

#pragma weak mpi_status_set_elements_x_ = pmpi_status_set_elements_x_
void pmpi_status_set_elements_x_(MPI_Status *status, MPI_Fint *datatype, MPI_Count *count, int *ierror)
{
/* MPI_Status_set_elements_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Status_set_elements_x(status, c_datatype, *count);
}

#pragma weak mpi_status_set_elements_x__ = pmpi_status_set_elements_x__
void pmpi_status_set_elements_x__(MPI_Status *status, MPI_Fint *datatype, MPI_Count *count, int *ierror)
{
/* MPI_Status_set_elements_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Status_set_elements_x(status, c_datatype, *count);
}

#pragma weak mpi_test_ = pmpi_test_
void pmpi_test_(MPI_Fint *request, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Test */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Test(&c_request, flag, status);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_test__ = pmpi_test__
void pmpi_test__(MPI_Fint *request, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Test */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Test(&c_request, flag, status);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_test_cancelled_ = pmpi_test_cancelled_
void pmpi_test_cancelled_(const MPI_Status *status, int *flag, int *ierror)
{
/* MPI_Test_cancelled */

	*ierror = MPI_Test_cancelled(status, flag);
}

#pragma weak mpi_test_cancelled__ = pmpi_test_cancelled__
void pmpi_test_cancelled__(const MPI_Status *status, int *flag, int *ierror)
{
/* MPI_Test_cancelled */

	*ierror = MPI_Test_cancelled(status, flag);
}

#pragma weak mpi_testall_ = pmpi_testall_
void pmpi_testall_(int *count, MPI_Fint array_of_requests[], int *flag, MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Testall */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Testall(*count, c_array_of_requests, flag, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_testall__ = pmpi_testall__
void pmpi_testall__(int *count, MPI_Fint array_of_requests[], int *flag, MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Testall */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Testall(*count, c_array_of_requests, flag, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_testany_ = pmpi_testany_
void pmpi_testany_(int *count, MPI_Fint array_of_requests[], int *index, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Testany */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Testany(*count, c_array_of_requests, index, flag, status);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_testany__ = pmpi_testany__
void pmpi_testany__(int *count, MPI_Fint array_of_requests[], int *index, int *flag, MPI_Status *status, int *ierror)
{
/* MPI_Testany */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Testany(*count, c_array_of_requests, index, flag, status);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_testsome_ = pmpi_testsome_
void pmpi_testsome_(int *incount, MPI_Fint array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Testsome */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *incount);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *incount; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Testsome(*incount, c_array_of_requests, outcount, array_of_indices, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *incount; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_testsome__ = pmpi_testsome__
void pmpi_testsome__(int *incount, MPI_Fint array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Testsome */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *incount);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *incount; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Testsome(*incount, c_array_of_requests, outcount, array_of_indices, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *incount; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_topo_test_ = pmpi_topo_test_
void pmpi_topo_test_(MPI_Fint *comm, int *status, int *ierror)
{
/* MPI_Topo_test */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Topo_test(c_comm, status);
}

#pragma weak mpi_topo_test__ = pmpi_topo_test__
void pmpi_topo_test__(MPI_Fint *comm, int *status, int *ierror)
{
/* MPI_Topo_test */
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Topo_test(c_comm, status);
}

#pragma weak mpi_type_commit_ = pmpi_type_commit_
void pmpi_type_commit_(MPI_Fint *datatype, int *ierror)
{
/* MPI_Type_commit */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror   = MPI_Type_commit(&c_datatype);
	*datatype = PMPI_Type_c2f(c_datatype);
}

#pragma weak mpi_type_commit__ = pmpi_type_commit__
void pmpi_type_commit__(MPI_Fint *datatype, int *ierror)
{
/* MPI_Type_commit */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror   = MPI_Type_commit(&c_datatype);
	*datatype = PMPI_Type_c2f(c_datatype);
}

#pragma weak mpi_type_contiguous_ = pmpi_type_contiguous_
void pmpi_type_contiguous_(int *count, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
    /* MPI_Type_contiguous */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_contiguous(*count, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_contiguous__ = pmpi_type_contiguous__
void pmpi_type_contiguous__(int *count, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_contiguous */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_contiguous(*count, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_darray_ = pmpi_type_create_darray_
void pmpi_type_create_darray_(int *size, int *rank, int *ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int *order, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_darray */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_darray(*size, *rank, *ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, *order, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_darray__ = pmpi_type_create_darray__
void pmpi_type_create_darray__(int *size, int *rank, int *ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int *order, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_darray */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_darray(*size, *rank, *ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, *order, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_f90_complex_ = pmpi_type_create_f90_complex_
void pmpi_type_create_f90_complex_(int *p, int *r, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_f90_complex */
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_f90_complex(*p, *r, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_f90_complex__ = pmpi_type_create_f90_complex__
void pmpi_type_create_f90_complex__(int *p, int *r, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_f90_complex */
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_f90_complex(*p, *r, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_f90_integer_ = pmpi_type_create_f90_integer_
void pmpi_type_create_f90_integer_(int *r, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_f90_integer */
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_f90_integer(*r, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_f90_integer__ = pmpi_type_create_f90_integer__
void pmpi_type_create_f90_integer__(int *r, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_f90_integer */
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_f90_integer(*r, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_f90_real_ = pmpi_type_create_f90_real_
void pmpi_type_create_f90_real_(int *p, int *r, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_f90_real */
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_f90_real(*p, *r, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_f90_real__ = pmpi_type_create_f90_real__
void pmpi_type_create_f90_real__(int *p, int *r, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_f90_real */
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_f90_real(*p, *r, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_hindexed_ = pmpi_type_create_hindexed_
void pmpi_type_create_hindexed_(int *count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hindexed */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_hindexed(*count, array_of_blocklengths, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_hindexed__ = pmpi_type_create_hindexed__
void pmpi_type_create_hindexed__(int *count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hindexed */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_hindexed(*count, array_of_blocklengths, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}


#pragma weak mpi_type_hindexed_ = pmpi_type_hindexed_
void pmpi_type_hindexed_(int *count,
                       const int array_of_blocklengths[],
                       const MPI_Aint array_of_displacements[],
                       MPI_Fint * oldtype, MPI_Fint * newtype, int *ierror)
{
/* MPI_Type_hindexed */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_hindexed(*count, array_of_blocklengths, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_hindexed__ = pmpi_type_hindexed__
void pmpi_type_hindexed__(int *count,
                        const int array_of_blocklengths[],
                        const MPI_Aint array_of_displacements[],
                        MPI_Fint * oldtype, MPI_Fint * newtype, int *ierror)
{
/* MPI_Type_hindexed */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_hindexed(*count, array_of_blocklengths, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}


#pragma weak mpi_type_create_hindexed_block_ = pmpi_type_create_hindexed_block_
void pmpi_type_create_hindexed_block_(int *count, int *blocklength, const MPI_Aint array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hindexed_block */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_hindexed_block(*count, *blocklength, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_hindexed_block__ = pmpi_type_create_hindexed_block__
void pmpi_type_create_hindexed_block__(int *count, int *blocklength, const MPI_Aint array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hindexed_block */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_hindexed_block(*count, *blocklength, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_hvector_ = pmpi_type_hvector_
void pmpi_type_hvector_(int *count, int *blocklength, MPI_Aint *stride, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hvector */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_hvector(*count, *blocklength, *stride, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_hvector__ = pmpi_type_hvector__
void pmpi_type_hvector__(int *count, int *blocklength, MPI_Aint *stride, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hvector */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_hvector(*count, *blocklength, *stride, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_hvector_ = pmpi_type_create_hvector_
void pmpi_type_create_hvector_(int *count, int *blocklength, MPI_Aint *stride, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hvector */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_hvector(*count, *blocklength, *stride, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_hvector__ = pmpi_type_create_hvector__
void pmpi_type_create_hvector__(int *count, int *blocklength, MPI_Aint *stride, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_hvector */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_hvector(*count, *blocklength, *stride, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_indexed_block_ = pmpi_type_create_indexed_block_
void pmpi_type_create_indexed_block_(int *count, int *blocklength, const int array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_indexed_block */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_indexed_block(*count, *blocklength, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_indexed_block__ = pmpi_type_create_indexed_block__
void pmpi_type_create_indexed_block__(int *count, int *blocklength, const int array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_indexed_block */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_indexed_block(*count, *blocklength, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_keyval_ = pmpi_type_create_keyval_
void pmpi_type_create_keyval_(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state, int *ierror)
{
/* MPI_Type_create_keyval */

	*ierror = MPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state);
}

#pragma weak mpi_type_create_keyval__ = pmpi_type_create_keyval__
void pmpi_type_create_keyval__(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state, int *ierror)
{
/* MPI_Type_create_keyval */

	*ierror = MPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state);
}

#pragma weak mpi_type_create_resized_ = pmpi_type_create_resized_
void pmpi_type_create_resized_(MPI_Fint *oldtype, MPI_Aint *lb, MPI_Aint *extent, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_resized */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_resized(c_oldtype, *lb, *extent, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_resized__ = pmpi_type_create_resized__
void pmpi_type_create_resized__(MPI_Fint *oldtype, MPI_Aint *lb, MPI_Aint *extent, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_resized */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_resized(c_oldtype, *lb, *extent, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_struct_ = pmpi_type_create_struct_
void pmpi_type_create_struct_(int *count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Fint array_of_types[], MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_struct */

	int           incnt_array_of_types = 0;
	MPI_Datatype *c_array_of_types     = NULL;

	c_array_of_types = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * *count);

	for(incnt_array_of_types = 0; incnt_array_of_types < *count; incnt_array_of_types++)
	{
		c_array_of_types[incnt_array_of_types] = PMPI_Type_f2c(array_of_types[incnt_array_of_types]);
	}
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_struct(*count, array_of_blocklengths, array_of_displacements, c_array_of_types, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
	sctk_free(c_array_of_types);
}

#pragma weak mpi_type_create_struct__ = pmpi_type_create_struct__
void pmpi_type_create_struct__(int *count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Fint array_of_types[], MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_struct */

	int           incnt_array_of_types = 0;
	MPI_Datatype *c_array_of_types     = NULL;

	c_array_of_types = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * *count);

	for(incnt_array_of_types = 0; incnt_array_of_types < *count; incnt_array_of_types++)
	{
		c_array_of_types[incnt_array_of_types] = PMPI_Type_f2c(array_of_types[incnt_array_of_types]);
	}
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_struct(*count, array_of_blocklengths, array_of_displacements, c_array_of_types, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
	sctk_free(c_array_of_types);
}

#pragma weak mpi_type_struct_ = pmpi_type_struct_
void pmpi_type_struct_(int *count,
							 const int array_of_blocklengths[],
							 const MPI_Aint array_of_displacements[],
							 const MPI_Fint array_of_types[],
							 MPI_Fint *newtype,
							 int *ierror)
{
	pmpi_type_create_struct_(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype, ierror);
}

#pragma weak mpi_type_struct__ = pmpi_type_struct__
void pmpi_type_struct__(int *count,
							 const int array_of_blocklengths[],
							 const MPI_Aint array_of_displacements[],
							 const MPI_Fint array_of_types[],
							 MPI_Fint *newtype,
							 int *ierror)
{
	pmpi_type_struct_(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype, ierror);
}

#pragma weak mpi_type_create_subarray_ = pmpi_type_create_subarray_
void pmpi_type_create_subarray_(int *ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int *order, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_subarray */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_subarray(*ndims, array_of_sizes, array_of_subsizes, array_of_starts, *order, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_create_subarray__ = pmpi_type_create_subarray__
void pmpi_type_create_subarray__(int *ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int *order, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_create_subarray */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_create_subarray(*ndims, array_of_sizes, array_of_subsizes, array_of_starts, *order, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_delete_attr_ = pmpi_type_delete_attr_
void pmpi_type_delete_attr_(MPI_Fint *datatype, int *type_keyval, int *ierror)
{
/* MPI_Type_delete_attr */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_delete_attr(c_datatype, *type_keyval);
}

#pragma weak mpi_type_delete_attr__ = pmpi_type_delete_attr__
void pmpi_type_delete_attr__(MPI_Fint *datatype, int *type_keyval, int *ierror)
{
/* MPI_Type_delete_attr */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_delete_attr(c_datatype, *type_keyval);
}

#pragma weak mpi_type_dup_ = pmpi_type_dup_
void pmpi_type_dup_(MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_dup */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_dup(c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_dup__ = pmpi_type_dup__
void pmpi_type_dup__(MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_dup */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_dup(c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_free_ = pmpi_type_free_
void pmpi_type_free_(MPI_Fint *datatype, int *ierror)
{
/* MPI_Type_free */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror   = MPI_Type_free(&c_datatype);
	*datatype = PMPI_Type_c2f(c_datatype);
}

#pragma weak mpi_type_free__ = pmpi_type_free__
void pmpi_type_free__(MPI_Fint *datatype, int *ierror)
{
/* MPI_Type_free */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror   = MPI_Type_free(&c_datatype);
	*datatype = PMPI_Type_c2f(c_datatype);
}

#pragma weak mpi_type_free_keyval_ = pmpi_type_free_keyval_
void pmpi_type_free_keyval_(int *type_keyval, int *ierror)
{
/* MPI_Type_free_keyval */

	*ierror = MPI_Type_free_keyval(type_keyval);
}

#pragma weak mpi_type_free_keyval__ = pmpi_type_free_keyval__
void pmpi_type_free_keyval__(int *type_keyval, int *ierror)
{
/* MPI_Type_free_keyval */

	*ierror = MPI_Type_free_keyval(type_keyval);
}

#pragma weak mpi_type_get_attr_ = pmpi_type_get_attr_
void pmpi_type_get_attr_(MPI_Fint *datatype, int *type_keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Type_get_attr */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_attr(c_datatype, *type_keyval, attribute_val, flag);
}

#pragma weak mpi_type_get_attr__ = pmpi_type_get_attr__
void pmpi_type_get_attr__(MPI_Fint *datatype, int *type_keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Type_get_attr */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_attr(c_datatype, *type_keyval, attribute_val, flag);
}

#pragma weak mpi_type_get_contents_ = pmpi_type_get_contents_
void pmpi_type_get_contents_(MPI_Fint *datatype, int *max_integers, int *max_addresses, int *max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Fint array_of_datatypes[], int *ierror)
{
/* MPI_Type_get_contents */
	MPI_Datatype  c_datatype           = PMPI_Type_f2c(*datatype);
	MPI_Datatype *c_array_of_datatypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * *max_datatypes);

	*ierror = MPI_Type_get_contents(c_datatype, *max_integers, *max_addresses, *max_datatypes, array_of_integers, array_of_addresses, c_array_of_datatypes);

	int outcnt_array_of_datatypes = 0;

	for(outcnt_array_of_datatypes = 0; outcnt_array_of_datatypes < *max_datatypes; outcnt_array_of_datatypes++)
	{
		array_of_datatypes[outcnt_array_of_datatypes] = PMPI_Type_c2f(c_array_of_datatypes[outcnt_array_of_datatypes]);
	}

	sctk_free(c_array_of_datatypes);
}

#pragma weak mpi_type_get_contents__ = pmpi_type_get_contents__
void pmpi_type_get_contents__(MPI_Fint *datatype, int *max_integers, int *max_addresses, int *max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Fint array_of_datatypes[], int *ierror)
{
/* MPI_Type_get_contents */
	MPI_Datatype  c_datatype           = PMPI_Type_f2c(*datatype);
	MPI_Datatype *c_array_of_datatypes = (MPI_Datatype *)sctk_malloc(sizeof(MPI_Datatype) * *max_datatypes);

	*ierror = MPI_Type_get_contents(c_datatype, *max_integers, *max_addresses, *max_datatypes, array_of_integers, array_of_addresses, c_array_of_datatypes);

	int outcnt_array_of_datatypes = 0;

	for(outcnt_array_of_datatypes = 0; outcnt_array_of_datatypes < *max_datatypes; outcnt_array_of_datatypes++)
	{
		array_of_datatypes[outcnt_array_of_datatypes] = PMPI_Type_c2f(c_array_of_datatypes[outcnt_array_of_datatypes]);
	}

	sctk_free(c_array_of_datatypes);
}

#pragma weak mpi_get_elements_ = pmpi_get_elements_
void pmpi_get_elements_(MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Type_get_elements */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_elements(status, c_datatype, count);
}

#pragma weak mpi_get_elements__ = pmpi_get_elements__
void pmpi_get_elements__(MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Type_get_elements */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_elements(status, c_datatype, count);
}

#pragma weak mpi_get_elements_x_ = pmpi_get_elements_x_
void pmpi_get_elements_x_(MPI_Status *status, MPI_Fint *datatype, MPI_Count *count, int *ierror)
{
/* MPI_Type_get_elements_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Get_elements_x(status, c_datatype, count);
}

#pragma weak mpi_get_elements_x__ = pmpi_get_elements_x__
void pmpi_get_elements_x__(MPI_Status *status, MPI_Fint *datatype, MPI_Count *count, int *ierror)
{
/* MPI_Type_get_elements_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Get_elements_x(status, c_datatype, count);
}

#pragma weak mpi_type_get_elements_ = pmpi_type_get_elements_
void pmpi_type_get_elements_(MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Type_get_elements */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_elements(status, c_datatype, count);
}

#pragma weak mpi_type_get_elements__ = pmpi_type_get_elements__
void pmpi_type_get_elements__(MPI_Status *status, MPI_Fint *datatype, int *count, int *ierror)
{
/* MPI_Type_get_elements */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_elements(status, c_datatype, count);
}

#pragma weak mpi_type_get_elements_x_ = pmpi_type_get_elements_x_
void pmpi_type_get_elements_x_(MPI_Status *status, MPI_Fint *datatype, MPI_Count *count, int *ierror)
{
/* MPI_Type_get_elements_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_elements_x(status, c_datatype, count);
}

#pragma weak mpi_type_get_elements_x__ = pmpi_type_get_elements_x__
void pmpi_type_get_elements_x__(MPI_Status *status, MPI_Fint *datatype, MPI_Count *count, int *ierror)
{
/* MPI_Type_get_elements_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_elements_x(status, c_datatype, count);
}

#pragma weak mpi_type_get_envelope_ = pmpi_type_get_envelope_
void pmpi_type_get_envelope_(MPI_Fint *datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner, int *ierror)
{
/* MPI_Type_get_envelope */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_envelope(c_datatype, num_integers, num_addresses, num_datatypes, combiner);
}

#pragma weak mpi_type_get_envelope__ = pmpi_type_get_envelope__
void pmpi_type_get_envelope__(MPI_Fint *datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner, int *ierror)
{
/* MPI_Type_get_envelope */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_envelope(c_datatype, num_integers, num_addresses, num_datatypes, combiner);
}

#pragma weak mpi_type_extent_ = pmpi_type_extent_
void pmpi_type_extent_(MPI_Fint *datatype, MPI_Aint *extent, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_extent(c_datatype, extent);
}

#pragma weak mpi_type_extent__ = pmpi_type_extent__
void pmpi_type_extent__(MPI_Fint *datatype, MPI_Aint *extent, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_extent(c_datatype, extent);
}

#pragma weak mpi_type_lb_ = pmpi_type_lb_
void pmpi_type_lb_(MPI_Fint *datatype, MPI_Aint *lb, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_lb(c_datatype, lb);
}

#pragma weak mpi_type_lb__ = pmpi_type_lb__
void pmpi_type_lb__(MPI_Fint *datatype, MPI_Aint *lb, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_lb(c_datatype, lb);
}

#pragma weak mpi_type_ub_ = pmpi_type_ub_
void pmpi_type_ub_(MPI_Fint *datatype, MPI_Aint *ub, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_ub(c_datatype, ub);
}

#pragma weak mpi_type_ub__ = pmpi_type_ub__
void pmpi_type_ub__(MPI_Fint *datatype, MPI_Aint *ub, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_ub(c_datatype, ub);
}

#pragma weak mpi_type_get_extent_ = pmpi_type_get_extent_
void pmpi_type_get_extent_(MPI_Fint *datatype, MPI_Aint *lb, MPI_Aint *extent, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_extent(c_datatype, lb, extent);
}

#pragma weak mpi_type_get_extent__ = pmpi_type_get_extent__
void pmpi_type_get_extent__(MPI_Fint *datatype, MPI_Aint *lb, MPI_Aint *extent, int *ierror)
{
/* MPI_Type_get_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_extent(c_datatype, lb, extent);
}

#pragma weak mpi_type_get_extent_x_ = pmpi_type_get_extent_x_
void pmpi_type_get_extent_x_(MPI_Fint *datatype, MPI_Count *lb, MPI_Count *extent, int *ierror)
{
/* MPI_Type_get_extent_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_extent_x(c_datatype, lb, extent);
}

#pragma weak mpi_type_get_extent_x__ = pmpi_type_get_extent_x__
void pmpi_type_get_extent_x__(MPI_Fint *datatype, MPI_Count *lb, MPI_Count *extent, int *ierror)
{
/* MPI_Type_get_extent_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_extent_x(c_datatype, lb, extent);
}

#pragma weak mpi_type_get_name_ = pmpi_type_get_name_
void pmpi_type_get_name_(MPI_Fint *datatype, char *type_name CHAR_MIXED(size_type_name), int *resultlen, int *ierror CHAR_END(size_type_name) )
{
/* MPI_Type_get_name */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_name(c_datatype, type_name, resultlen);
	char_c_to_fortran(type_name, size_type_name);
}

#pragma weak mpi_type_get_name__ = pmpi_type_get_name__
void pmpi_type_get_name__(MPI_Fint *datatype, char *type_name CHAR_MIXED(size_type_name), int *resultlen, int *ierror CHAR_END(size_type_name) )
{
/* MPI_Type_get_name */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_name(c_datatype, type_name, resultlen);
	char_c_to_fortran(type_name, size_type_name);
}

#pragma weak mpi_type_get_true_extent_ = pmpi_type_get_true_extent_
void pmpi_type_get_true_extent_(MPI_Fint *datatype, MPI_Aint *true_lb, MPI_Aint *true_extent, int *ierror)
{
/* MPI_Type_get_true_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_true_extent(c_datatype, true_lb, true_extent);
}

#pragma weak mpi_type_get_true_extent__ = pmpi_type_get_true_extent__
void pmpi_type_get_true_extent__(MPI_Fint *datatype, MPI_Aint *true_lb, MPI_Aint *true_extent, int *ierror)
{
/* MPI_Type_get_true_extent */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_true_extent(c_datatype, true_lb, true_extent);
}

#pragma weak mpi_type_get_true_extent_x_ = pmpi_type_get_true_extent_x_
void pmpi_type_get_true_extent_x_(MPI_Fint *datatype, MPI_Count *true_lb, MPI_Count *true_extent, int *ierror)
{
/* MPI_Type_get_true_extent_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_true_extent_x(c_datatype, true_lb, true_extent);
}

#pragma weak mpi_type_get_true_extent_x__ = pmpi_type_get_true_extent_x__
void pmpi_type_get_true_extent_x__(MPI_Fint *datatype, MPI_Count *true_lb, MPI_Count *true_extent, int *ierror)
{
/* MPI_Type_get_true_extent_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_get_true_extent_x(c_datatype, true_lb, true_extent);
}

#pragma weak mpi_type_indexed_ = pmpi_type_indexed_
void pmpi_type_indexed_(int *count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_indexed */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_indexed(*count, array_of_blocklengths, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_indexed__ = pmpi_type_indexed__
void pmpi_type_indexed__(int *count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_indexed */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_indexed(*count, array_of_blocklengths, array_of_displacements, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_match_size_ = pmpi_type_match_size_
void pmpi_type_match_size_(int *typeclass, int *size, MPI_Fint *datatype, int *ierror)
{
/* MPI_Type_match_size */
	MPI_Datatype c_datatype;

	*ierror   = MPI_Type_match_size(*typeclass, *size, &c_datatype);
	*datatype = PMPI_Type_c2f(c_datatype);
}

#pragma weak mpi_type_match_size__ = pmpi_type_match_size__
void pmpi_type_match_size__(int *typeclass, int *size, MPI_Fint *datatype, int *ierror)
{
/* MPI_Type_match_size */
	MPI_Datatype c_datatype;

	*ierror   = MPI_Type_match_size(*typeclass, *size, &c_datatype);
	*datatype = PMPI_Type_c2f(c_datatype);
}

#pragma weak mpi_type_set_attr_ = pmpi_type_set_attr_
void pmpi_type_set_attr_(MPI_Fint *datatype, int *type_keyval, void *attribute_val, int *ierror)
{
/* MPI_Type_set_attr */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_set_attr(c_datatype, *type_keyval, attribute_val);
}

#pragma weak mpi_type_set_attr__ = pmpi_type_set_attr__
void pmpi_type_set_attr__(MPI_Fint *datatype, int *type_keyval, void *attribute_val, int *ierror)
{
/* MPI_Type_set_attr */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_set_attr(c_datatype, *type_keyval, attribute_val);
}

#pragma weak mpi_type_set_name_ = pmpi_type_set_name_
void pmpi_type_set_name_(MPI_Fint *datatype, const char *type_name CHAR_MIXED(size_type_name), int *ierror CHAR_END(size_type_name) )
{
/* MPI_Type_set_name */
	MPI_Datatype c_datatype    = PMPI_Type_f2c(*datatype);
	char *       tmp_type_name = NULL, *ptr_type_name = NULL;

	tmp_type_name = char_fortran_to_c( (char *)type_name, size_type_name, &ptr_type_name);

	*ierror = MPI_Type_set_name(c_datatype, tmp_type_name);
	sctk_free(ptr_type_name);
}

#pragma weak mpi_type_set_name__ = pmpi_type_set_name__
void pmpi_type_set_name__(MPI_Fint *datatype, const char *type_name CHAR_MIXED(size_type_name), int *ierror CHAR_END(size_type_name) )
{
/* MPI_Type_set_name */
	MPI_Datatype c_datatype    = PMPI_Type_f2c(*datatype);
	char *       tmp_type_name = NULL, *ptr_type_name = NULL;

	tmp_type_name = char_fortran_to_c( (char *)type_name, size_type_name, &ptr_type_name);

	*ierror = MPI_Type_set_name(c_datatype, tmp_type_name);
	sctk_free(ptr_type_name);
}

#pragma weak mpi_type_size_ = pmpi_type_size_
void pmpi_type_size_(MPI_Fint *datatype, int *size, int *ierror)
{
/* MPI_Type_size */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_size(c_datatype, size);
}

#pragma weak mpi_type_size__ = pmpi_type_size__
void pmpi_type_size__(MPI_Fint *datatype, int *size, int *ierror)
{
/* MPI_Type_size */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_size(c_datatype, size);
}

#pragma weak mpi_type_size_x_ = pmpi_type_size_x_
void pmpi_type_size_x_(MPI_Fint *datatype, MPI_Count *size, int *ierror)
{
/* MPI_Type_size_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_size_x(c_datatype, size);
}

#pragma weak mpi_type_size_x__ = pmpi_type_size_x__
void pmpi_type_size_x__(MPI_Fint *datatype, MPI_Count *size, int *ierror)
{
/* MPI_Type_size_x */
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Type_size_x(c_datatype, size);
}

#pragma weak mpi_type_vector_ = pmpi_type_vector_
void pmpi_type_vector_(int *count, int *blocklength, int *stride, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_vector */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_vector(*count, *blocklength, *stride, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_type_vector__ = pmpi_type_vector__
void pmpi_type_vector__(int *count, int *blocklength, int *stride, MPI_Fint *oldtype, MPI_Fint *newtype, int *ierror)
{
/* MPI_Type_vector */
	MPI_Datatype c_oldtype = PMPI_Type_f2c(*oldtype);
	MPI_Datatype c_newtype;

	*ierror  = MPI_Type_vector(*count, *blocklength, *stride, c_oldtype, &c_newtype);
	*newtype = PMPI_Type_c2f(c_newtype);
}

#pragma weak mpi_unpack_ = pmpi_unpack_
void pmpi_unpack_(const void *inbuf, int *insize, int *position, void *outbuf, int *outcount, MPI_Fint *datatype, MPI_Fint *comm, int *ierror)
{
/* MPI_Unpack */
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, c_datatype, c_comm);
}

#pragma weak mpi_unpack__ = pmpi_unpack__
void pmpi_unpack__(const void *inbuf, int *insize, int *position, void *outbuf, int *outcount, MPI_Fint *datatype, MPI_Fint *comm, int *ierror)
{
/* MPI_Unpack */
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
	MPI_Comm     c_comm     = PMPI_Comm_f2c(*comm);

	*ierror = MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, c_datatype, c_comm);
}

#pragma weak mpi_unpack_external_ = pmpi_unpack_external_
void pmpi_unpack_external_(const char datarep[] CHAR_MIXED(size_datarep), const void *inbuf, MPI_Aint *insize, MPI_Aint *position, void *outbuf, int *outcount, MPI_Fint *datatype, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Unpack_external */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Unpack_external(tmp_datarep, inbuf, *insize, position, outbuf, *outcount, c_datatype);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_unpack_external__ = pmpi_unpack_external__
void pmpi_unpack_external__(const char datarep[] CHAR_MIXED(size_datarep), const void *inbuf, MPI_Aint *insize, MPI_Aint *position, void *outbuf, int *outcount, MPI_Fint *datatype, int *ierror CHAR_END(size_datarep) )
{
/* MPI_Unpack_external */
	char *tmp_datarep = NULL, *ptr_datarep = NULL;

	tmp_datarep = char_fortran_to_c( (char *)datarep, size_datarep, &ptr_datarep);
	if(buffer_is_bottom( (void *)inbuf) )
	{
		inbuf = MPI_BOTTOM;
	}
	MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);

	*ierror = MPI_Unpack_external(tmp_datarep, inbuf, *insize, position, outbuf, *outcount, c_datatype);
	sctk_free(ptr_datarep);
}

#pragma weak mpi_unpublish_name_ = pmpi_unpublish_name_
void pmpi_unpublish_name_(const char *service_name CHAR_MIXED(size_service_name), MPI_Fint *info, const char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_service_name)CHAR_END(size_port_name) )
{
/* MPI_Unpublish_name */
	char *tmp_service_name = NULL, *ptr_service_name = NULL;

	tmp_service_name = char_fortran_to_c( (char *)service_name, size_service_name, &ptr_service_name);
	MPI_Info c_info        = PMPI_Info_f2c(*info);
	char *   tmp_port_name = NULL, *ptr_port_name = NULL;
	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);

	*ierror = MPI_Unpublish_name(tmp_service_name, c_info, tmp_port_name);
	sctk_free(ptr_service_name);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_unpublish_name__ = pmpi_unpublish_name__
void pmpi_unpublish_name__(const char *service_name CHAR_MIXED(size_service_name), MPI_Fint *info, const char *port_name CHAR_MIXED(size_port_name), int *ierror CHAR_END(size_service_name)CHAR_END(size_port_name) )
{
/* MPI_Unpublish_name */
	char *tmp_service_name = NULL, *ptr_service_name = NULL;

	tmp_service_name = char_fortran_to_c( (char *)service_name, size_service_name, &ptr_service_name);
	MPI_Info c_info        = PMPI_Info_f2c(*info);
	char *   tmp_port_name = NULL, *ptr_port_name = NULL;
	tmp_port_name = char_fortran_to_c( (char *)port_name, size_port_name, &ptr_port_name);

	*ierror = MPI_Unpublish_name(tmp_service_name, c_info, tmp_port_name);
	sctk_free(ptr_service_name);
	sctk_free(ptr_port_name);
}

#pragma weak mpi_wait_ = pmpi_wait_
void pmpi_wait_(MPI_Fint *request, MPI_Status *status, int *ierror)
{
/* MPI_Wait */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Wait(&c_request, status);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_wait__ = pmpi_wait__
void pmpi_wait__(MPI_Fint *request, MPI_Status *status, int *ierror)
{
/* MPI_Wait */
	MPI_Request c_request = PMPI_Request_f2c(*request);

	*ierror  = MPI_Wait(&c_request, status);
	*request = PMPI_Request_c2f(c_request);
}

#pragma weak mpi_waitall_ = pmpi_waitall_
void pmpi_waitall_(int *count, MPI_Fint array_of_requests[], MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Waitall */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Waitall(*count, c_array_of_requests, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_waitall__ = pmpi_waitall__
void pmpi_waitall__(int *count, MPI_Fint array_of_requests[], MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Waitall */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Waitall(*count, c_array_of_requests, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_waitany_ = pmpi_waitany_
void pmpi_waitany_(int *count, MPI_Fint array_of_requests[], int *index, MPI_Status *status, int *ierror)
{
/* MPI_Waitany */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Waitany(*count, c_array_of_requests, index, status);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_waitany__ = pmpi_waitany__
void pmpi_waitany__(int *count, MPI_Fint array_of_requests[], int *index, MPI_Status *status, int *ierror)
{
/* MPI_Waitany */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *count);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *count; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Waitany(*count, c_array_of_requests, index, status);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *count; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_waitsome_ = pmpi_waitsome_
void pmpi_waitsome_(int *incount, MPI_Fint array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Waitsome */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *incount);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *incount; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Waitsome(*incount, c_array_of_requests, outcount, array_of_indices, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *incount; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_waitsome__ = pmpi_waitsome__
void pmpi_waitsome__(int *incount, MPI_Fint array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[], int *ierror)
{
/* MPI_Waitsome */

	int          incnt_array_of_requests = 0;
	MPI_Request *c_array_of_requests     = NULL;

	c_array_of_requests = (MPI_Request *)sctk_malloc(sizeof(MPI_Request) * *incount);

	for(incnt_array_of_requests = 0; incnt_array_of_requests < *incount; incnt_array_of_requests++)
	{
		c_array_of_requests[incnt_array_of_requests] = PMPI_Request_f2c(array_of_requests[incnt_array_of_requests]);
	}

	*ierror = MPI_Waitsome(*incount, c_array_of_requests, outcount, array_of_indices, array_of_statuses);

	int outcnt_array_of_requests = 0;

	for(outcnt_array_of_requests = 0; outcnt_array_of_requests < *incount; outcnt_array_of_requests++)
	{
		array_of_requests[outcnt_array_of_requests] = PMPI_Request_c2f(c_array_of_requests[outcnt_array_of_requests]);
	}

	sctk_free(c_array_of_requests);
}

#pragma weak mpi_win_allocate_ = pmpi_win_allocate_
void pmpi_win_allocate_(MPI_Aint *size, int *disp_unit, MPI_Fint *info, MPI_Fint *comm, void *baseptr, MPI_Fint *win, int *ierror)
{
/* MPI_Win_allocate */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_allocate(*size, *disp_unit, c_info, c_comm, baseptr, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_allocate__ = pmpi_win_allocate__
void pmpi_win_allocate__(MPI_Aint *size, int *disp_unit, MPI_Fint *info, MPI_Fint *comm, void *baseptr, MPI_Fint *win, int *ierror)
{
/* MPI_Win_allocate */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_allocate(*size, *disp_unit, c_info, c_comm, baseptr, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_allocate_shared_ = pmpi_win_allocate_shared_
void pmpi_win_allocate_shared_(MPI_Aint *size, int *disp_unit, MPI_Fint *info, MPI_Fint *comm, void *baseptr, MPI_Fint *win, int *ierror)
{
/* MPI_Win_allocate_shared */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_allocate_shared(*size, *disp_unit, c_info, c_comm, baseptr, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_allocate_shared__ = pmpi_win_allocate_shared__
void pmpi_win_allocate_shared__(MPI_Aint *size, int *disp_unit, MPI_Fint *info, MPI_Fint *comm, void *baseptr, MPI_Fint *win, int *ierror)
{
/* MPI_Win_allocate_shared */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_allocate_shared(*size, *disp_unit, c_info, c_comm, baseptr, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_attach_ = pmpi_win_attach_
void pmpi_win_attach_(MPI_Fint *win, void *base, MPI_Aint *size, int *ierror)
{
/* MPI_Win_attach */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}

	*ierror = MPI_Win_attach(c_win, base, *size);
}

#pragma weak mpi_win_attach__ = pmpi_win_attach__
void pmpi_win_attach__(MPI_Fint *win, void *base, MPI_Aint *size, int *ierror)
{
/* MPI_Win_attach */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}

	*ierror = MPI_Win_attach(c_win, base, *size);
}

#pragma weak mpi_win_call_errhandler_ = pmpi_win_call_errhandler_
void pmpi_win_call_errhandler_(MPI_Fint *win, int *errorcode, int *ierror)
{
/* MPI_Win_call_errhandler */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_call_errhandler(c_win, *errorcode);
}

#pragma weak mpi_win_call_errhandler__ = pmpi_win_call_errhandler__
void pmpi_win_call_errhandler__(MPI_Fint *win, int *errorcode, int *ierror)
{
/* MPI_Win_call_errhandler */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_call_errhandler(c_win, *errorcode);
}

#pragma weak mpi_win_complete_ = pmpi_win_complete_
void pmpi_win_complete_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_complete */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_complete(c_win);
}

#pragma weak mpi_win_complete__ = pmpi_win_complete__
void pmpi_win_complete__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_complete */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_complete(c_win);
}

#pragma weak mpi_win_create_ = pmpi_win_create_
void pmpi_win_create_(void *base, MPI_Aint *size, int *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, int *ierror)
{
/* MPI_Win_create */
	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_create(base, *size, *disp_unit, c_info, c_comm, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_create__ = pmpi_win_create__
void pmpi_win_create__(void *base, MPI_Aint *size, int *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, int *ierror)
{
/* MPI_Win_create */
	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_create(base, *size, *disp_unit, c_info, c_comm, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_create_dynamic_ = pmpi_win_create_dynamic_
void pmpi_win_create_dynamic_(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, int *ierror)
{
/* MPI_Win_create_dynamic */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_create_dynamic(c_info, c_comm, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_create_dynamic__ = pmpi_win_create_dynamic__
void pmpi_win_create_dynamic__(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, int *ierror)
{
/* MPI_Win_create_dynamic */
	MPI_Info c_info = PMPI_Info_f2c(*info);
	MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
	MPI_Win  c_win;

	*ierror = MPI_Win_create_dynamic(c_info, c_comm, &c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_create_errhandler_ = pmpi_win_create_errhandler_
void pmpi_win_create_errhandler_(MPI_Win_errhandler_function *win_errhandler_fn, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Win_create_errhandler */
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Win_create_errhandler(win_errhandler_fn, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_win_create_errhandler__ = pmpi_win_create_errhandler__
void pmpi_win_create_errhandler__(MPI_Win_errhandler_function *win_errhandler_fn, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Win_create_errhandler */
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Win_create_errhandler(win_errhandler_fn, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_win_create_keyval_ = pmpi_win_create_keyval_
void pmpi_win_create_keyval_(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state, int *ierror)
{
/* MPI_Win_create_keyval */

	*ierror = MPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
}

#pragma weak mpi_win_create_keyval__ = pmpi_win_create_keyval__
void pmpi_win_create_keyval__(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state, int *ierror)
{
/* MPI_Win_create_keyval */

	*ierror = MPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
}

#pragma weak mpi_win_delete_attr_ = pmpi_win_delete_attr_
void pmpi_win_delete_attr_(MPI_Fint *win, int *win_keyval, int *ierror)
{
/* MPI_Win_delete_attr */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_delete_attr(c_win, *win_keyval);
}

#pragma weak mpi_win_delete_attr__ = pmpi_win_delete_attr__
void pmpi_win_delete_attr__(MPI_Fint *win, int *win_keyval, int *ierror)
{
/* MPI_Win_delete_attr */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_delete_attr(c_win, *win_keyval);
}

#pragma weak mpi_win_detach_ = pmpi_win_detach_
void pmpi_win_detach_(MPI_Fint *win, const void *base, int *ierror)
{
/* MPI_Win_detach */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}

	*ierror = MPI_Win_detach(c_win, base);
}

#pragma weak mpi_win_detach__ = pmpi_win_detach__
void pmpi_win_detach__(MPI_Fint *win, const void *base, int *ierror)
{
/* MPI_Win_detach */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	if(buffer_is_bottom( (void *)base) )
	{
		base = MPI_BOTTOM;
	}

	*ierror = MPI_Win_detach(c_win, base);
}

#pragma weak mpi_win_fence_ = pmpi_win_fence_
void pmpi_win_fence_(int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_fence */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_fence(*assert, c_win);
}

#pragma weak mpi_win_fence__ = pmpi_win_fence__
void pmpi_win_fence__(int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_fence */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_fence(*assert, c_win);
}

#pragma weak mpi_win_flush_ = pmpi_win_flush_
void pmpi_win_flush_(int *rank, MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush(*rank, c_win);
}

#pragma weak mpi_win_flush__ = pmpi_win_flush__
void pmpi_win_flush__(int *rank, MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush(*rank, c_win);
}

#pragma weak mpi_win_flush_all_ = pmpi_win_flush_all_
void pmpi_win_flush_all_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush_all(c_win);
}

#pragma weak mpi_win_flush_all__ = pmpi_win_flush_all__
void pmpi_win_flush_all__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush_all(c_win);
}

#pragma weak mpi_win_flush_local_ = pmpi_win_flush_local_
void pmpi_win_flush_local_(int *rank, MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush_local */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush_local(*rank, c_win);
}

#pragma weak mpi_win_flush_local__ = pmpi_win_flush_local__
void pmpi_win_flush_local__(int *rank, MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush_local */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush_local(*rank, c_win);
}

#pragma weak mpi_win_flush_local_all_ = pmpi_win_flush_local_all_
void pmpi_win_flush_local_all_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush_local_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush_local_all(c_win);
}

#pragma weak mpi_win_flush_local_all__ = pmpi_win_flush_local_all__
void pmpi_win_flush_local_all__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_flush_local_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_flush_local_all(c_win);
}

#pragma weak mpi_win_free_ = pmpi_win_free_
void pmpi_win_free_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_free */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_free(&c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_free__ = pmpi_win_free__
void pmpi_win_free__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_free */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_free(&c_win);
	*win    = PMPI_Win_c2f(c_win);
}

#pragma weak mpi_win_free_keyval_ = pmpi_win_free_keyval_
void pmpi_win_free_keyval_(int *win_keyval, int *ierror)
{
/* MPI_Win_free_keyval */

	*ierror = MPI_Win_free_keyval(win_keyval);
}

#pragma weak mpi_win_free_keyval__ = pmpi_win_free_keyval__
void pmpi_win_free_keyval__(int *win_keyval, int *ierror)
{
/* MPI_Win_free_keyval */

	*ierror = MPI_Win_free_keyval(win_keyval);
}

#pragma weak mpi_win_get_attr_ = pmpi_win_get_attr_
void pmpi_win_get_attr_(MPI_Fint *win, int *win_keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Win_get_attr */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_get_attr(c_win, *win_keyval, attribute_val, flag);
}

#pragma weak mpi_win_get_attr__ = pmpi_win_get_attr__
void pmpi_win_get_attr__(MPI_Fint *win, int *win_keyval, void *attribute_val, int *flag, int *ierror)
{
/* MPI_Win_get_attr */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_get_attr(c_win, *win_keyval, attribute_val, flag);
}

#pragma weak mpi_win_get_errhandler_ = pmpi_win_get_errhandler_
void pmpi_win_get_errhandler_(MPI_Fint *win, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Win_get_errhandler */
	MPI_Win        c_win = PMPI_Win_f2c(*win);
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Win_get_errhandler(c_win, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_win_get_errhandler__ = pmpi_win_get_errhandler__
void pmpi_win_get_errhandler__(MPI_Fint *win, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Win_get_errhandler */
	MPI_Win        c_win = PMPI_Win_f2c(*win);
	MPI_Errhandler c_errhandler;

	*ierror     = MPI_Win_get_errhandler(c_win, &c_errhandler);
	*errhandler = PMPI_Errhandler_c2f(c_errhandler);
}

#pragma weak mpi_win_get_group_ = pmpi_win_get_group_
void pmpi_win_get_group_(MPI_Fint *win, MPI_Fint *group, int *ierror)
{
/* MPI_Win_get_group */
	MPI_Win   c_win = PMPI_Win_f2c(*win);
	MPI_Group c_group;

	*ierror = MPI_Win_get_group(c_win, &c_group);
	*group  = PMPI_Group_c2f(c_group);
}

#pragma weak mpi_win_get_group__ = pmpi_win_get_group__
void pmpi_win_get_group__(MPI_Fint *win, MPI_Fint *group, int *ierror)
{
/* MPI_Win_get_group */
	MPI_Win   c_win = PMPI_Win_f2c(*win);
	MPI_Group c_group;

	*ierror = MPI_Win_get_group(c_win, &c_group);
	*group  = PMPI_Group_c2f(c_group);
}

#pragma weak mpi_win_get_info_ = pmpi_win_get_info_
void pmpi_win_get_info_(MPI_Fint *win, MPI_Fint *info_used, int *ierror)
{
/* MPI_Win_get_info */
	MPI_Win  c_win = PMPI_Win_f2c(*win);
	MPI_Info c_info_used;

	*ierror    = MPI_Win_get_info(c_win, &c_info_used);
	*info_used = PMPI_Info_c2f(c_info_used);
}

#pragma weak mpi_win_get_info__ = pmpi_win_get_info__
void pmpi_win_get_info__(MPI_Fint *win, MPI_Fint *info_used, int *ierror)
{
/* MPI_Win_get_info */
	MPI_Win  c_win = PMPI_Win_f2c(*win);
	MPI_Info c_info_used;

	*ierror    = MPI_Win_get_info(c_win, &c_info_used);
	*info_used = PMPI_Info_c2f(c_info_used);
}

#pragma weak mpi_win_get_name_ = pmpi_win_get_name_
void pmpi_win_get_name_(MPI_Fint *win, char *win_name CHAR_MIXED(size_win_name), int *resultlen, int *ierror CHAR_END(size_win_name) )
{
/* MPI_Win_get_name */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_get_name(c_win, win_name, resultlen);
	char_c_to_fortran(win_name, size_win_name);
}

#pragma weak mpi_win_get_name__ = pmpi_win_get_name__
void pmpi_win_get_name__(MPI_Fint *win, char *win_name CHAR_MIXED(size_win_name), int *resultlen, int *ierror CHAR_END(size_win_name) )
{
/* MPI_Win_get_name */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_get_name(c_win, win_name, resultlen);
	char_c_to_fortran(win_name, size_win_name);
}

#pragma weak mpi_win_lock_ = pmpi_win_lock_
void pmpi_win_lock_(int *lock_type, int *rank, int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_lock */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_lock(*lock_type, *rank, *assert, c_win);
}

#pragma weak mpi_win_lock__ = pmpi_win_lock__
void pmpi_win_lock__(int *lock_type, int *rank, int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_lock */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_lock(*lock_type, *rank, *assert, c_win);
}

#pragma weak mpi_win_lock_all_ = pmpi_win_lock_all_
void pmpi_win_lock_all_(int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_lock_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_lock_all(*assert, c_win);
}

#pragma weak mpi_win_lock_all__ = pmpi_win_lock_all__
void pmpi_win_lock_all__(int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_lock_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_lock_all(*assert, c_win);
}

#pragma weak mpi_win_post_ = pmpi_win_post_
void pmpi_win_post_(MPI_Fint *group, int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_post */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Win   c_win   = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_post(c_group, *assert, c_win);
}

#pragma weak mpi_win_post__ = pmpi_win_post__
void pmpi_win_post__(MPI_Fint *group, int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_post */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Win   c_win   = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_post(c_group, *assert, c_win);
}

#pragma weak mpi_win_set_attr_ = pmpi_win_set_attr_
void pmpi_win_set_attr_(MPI_Fint *win, int *win_keyval, void *attribute_val, int *ierror)
{
/* MPI_Win_set_attr */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_set_attr(c_win, *win_keyval, attribute_val);
}

#pragma weak mpi_win_set_attr__ = pmpi_win_set_attr__
void pmpi_win_set_attr__(MPI_Fint *win, int *win_keyval, void *attribute_val, int *ierror)
{
/* MPI_Win_set_attr */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_set_attr(c_win, *win_keyval, attribute_val);
}

#pragma weak mpi_win_set_errhandler_ = pmpi_win_set_errhandler_
void pmpi_win_set_errhandler_(MPI_Fint *win, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Win_set_errhandler */
	MPI_Win        c_win        = PMPI_Win_f2c(*win);
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror = MPI_Win_set_errhandler(c_win, c_errhandler);
}

#pragma weak mpi_win_set_errhandler__ = pmpi_win_set_errhandler__
void pmpi_win_set_errhandler__(MPI_Fint *win, MPI_Fint *errhandler, int *ierror)
{
/* MPI_Win_set_errhandler */
	MPI_Win        c_win        = PMPI_Win_f2c(*win);
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c(*errhandler);

	*ierror = MPI_Win_set_errhandler(c_win, c_errhandler);
}

#pragma weak mpi_win_set_info_ = pmpi_win_set_info_
void pmpi_win_set_info_(MPI_Fint *win, MPI_Fint *info, int *ierror)
{
/* MPI_Win_set_info */
	MPI_Win  c_win  = PMPI_Win_f2c(*win);
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Win_set_info(c_win, c_info);
}

#pragma weak mpi_win_set_info__ = pmpi_win_set_info__
void pmpi_win_set_info__(MPI_Fint *win, MPI_Fint *info, int *ierror)
{
/* MPI_Win_set_info */
	MPI_Win  c_win  = PMPI_Win_f2c(*win);
	MPI_Info c_info = PMPI_Info_f2c(*info);

	*ierror = MPI_Win_set_info(c_win, c_info);
}

#pragma weak mpi_win_set_name_ = pmpi_win_set_name_
void pmpi_win_set_name_(MPI_Fint *win, const char *win_name CHAR_MIXED(size_win_name), int *ierror CHAR_END(size_win_name) )
{
/* MPI_Win_set_name */
	MPI_Win c_win        = PMPI_Win_f2c(*win);
	char *  tmp_win_name = NULL, *ptr_win_name = NULL;

	tmp_win_name = char_fortran_to_c( (char *)win_name, size_win_name, &ptr_win_name);

	*ierror = MPI_Win_set_name(c_win, tmp_win_name);
	sctk_free(ptr_win_name);
}

#pragma weak mpi_win_set_name__ = pmpi_win_set_name__
void pmpi_win_set_name__(MPI_Fint *win, const char *win_name CHAR_MIXED(size_win_name), int *ierror CHAR_END(size_win_name) )
{
/* MPI_Win_set_name */
	MPI_Win c_win        = PMPI_Win_f2c(*win);
	char *  tmp_win_name = NULL, *ptr_win_name = NULL;

	tmp_win_name = char_fortran_to_c( (char *)win_name, size_win_name, &ptr_win_name);

	*ierror = MPI_Win_set_name(c_win, tmp_win_name);
	sctk_free(ptr_win_name);
}

#pragma weak mpi_win_shared_query_ = pmpi_win_shared_query_
void pmpi_win_shared_query_(MPI_Fint *win, int *rank, MPI_Aint *size, int *disp_unit, void *baseptr, int *ierror)
{
/* MPI_Win_shared_query */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_shared_query(c_win, *rank, size, disp_unit, baseptr);
}

#pragma weak mpi_win_shared_query__ = pmpi_win_shared_query__
void pmpi_win_shared_query__(MPI_Fint *win, int *rank, MPI_Aint *size, int *disp_unit, void *baseptr, int *ierror)
{
/* MPI_Win_shared_query */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_shared_query(c_win, *rank, size, disp_unit, baseptr);
}

#pragma weak mpi_win_start_ = pmpi_win_start_
void pmpi_win_start_(MPI_Fint *group, int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_start */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Win   c_win   = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_start(c_group, *assert, c_win);
}

#pragma weak mpi_win_start__ = pmpi_win_start__
void pmpi_win_start__(MPI_Fint *group, int *assert, MPI_Fint *win, int *ierror)
{
/* MPI_Win_start */
	MPI_Group c_group = PMPI_Group_f2c(*group);
	MPI_Win   c_win   = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_start(c_group, *assert, c_win);
}

#pragma weak mpi_win_sync_ = pmpi_win_sync_
void pmpi_win_sync_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_sync */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_sync(c_win);
}

#pragma weak mpi_win_sync__ = pmpi_win_sync__
void pmpi_win_sync__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_sync */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_sync(c_win);
}

#pragma weak mpi_win_test_ = pmpi_win_test_
void pmpi_win_test_(MPI_Fint *win, int *flag, int *ierror)
{
/* MPI_Win_test */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_test(c_win, flag);
}

#pragma weak mpi_win_test__ = pmpi_win_test__
void pmpi_win_test__(MPI_Fint *win, int *flag, int *ierror)
{
/* MPI_Win_test */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_test(c_win, flag);
}

#pragma weak mpi_win_unlock_ = pmpi_win_unlock_
void pmpi_win_unlock_(int *rank, MPI_Fint *win, int *ierror)
{
/* MPI_Win_unlock */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_unlock(*rank, c_win);
}

#pragma weak mpi_win_unlock__ = pmpi_win_unlock__
void pmpi_win_unlock__(int *rank, MPI_Fint *win, int *ierror)
{
/* MPI_Win_unlock */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_unlock(*rank, c_win);
}

#pragma weak mpi_win_unlock_all_ = pmpi_win_unlock_all_
void pmpi_win_unlock_all_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_unlock_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_unlock_all(c_win);
}

#pragma weak mpi_win_unlock_all__ = pmpi_win_unlock_all__
void pmpi_win_unlock_all__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_unlock_all */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_unlock_all(c_win);
}

#pragma weak mpi_win_wait_ = pmpi_win_wait_
void pmpi_win_wait_(MPI_Fint *win, int *ierror)
{
/* MPI_Win_wait */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_wait(c_win);
}

#pragma weak mpi_win_wait__ = pmpi_win_wait__
void pmpi_win_wait__(MPI_Fint *win, int *ierror)
{
/* MPI_Win_wait */
	MPI_Win c_win = PMPI_Win_f2c(*win);

	*ierror = MPI_Win_wait(c_win);
}

#pragma weak mpi_wtick_ = pmpi_wtick_
double pmpi_wtick_()
{
/* MPI_Wtick */

	return MPI_Wtick();
}

#pragma weak mpi_wtick__ = pmpi_wtick__
double pmpi_wtick__()
{
/* MPI_Wtick */

	return MPI_Wtick();
}

#pragma weak mpi_wtime_ = pmpi_wtime_
double pmpi_wtime_()
{
/* MPI_Wtime */

	return MPI_Wtime();
}

#pragma weak mpi_wtime__ = pmpi_wtime__
double pmpi_wtime__()
{
/* MPI_Wtime */

	return MPI_Wtime();
}

#pragma weak mpi_session_create_errhandler_ = pmpi_session_create_errhandler_
void pmpi_session_create_errhandler_(MPI_Session_errhandler_function *session_errhandler_fn,
                                   MPI_Fint *errhandler, int *ierror)
{
	MPI_Errhandler c_errhandler;

	*ierror = PMPI_Session_create_errhandler( session_errhandler_fn, &c_errhandler );
	*errhandler = PMPI_Errhandler_c2f( c_errhandler );
}

#pragma weak mpi_session_create_errhandler__ = pmpi_session_create_errhandler__
void pmpi_session_create_errhandler__(MPI_Session_errhandler_function *session_errhandler_fn,
                                   MPI_Fint *errhandler, int *ierror)
{
	pmpi_session_create_errhandler_( session_errhandler_fn, errhandler, ierror );
}

#pragma weak mpi_session_set_errhandler_ = pmpi_session_set_errhandler_
void pmpi_session_set_errhandler_(MPI_Fint * session, MPI_Fint * errhandler, int *ierror)
{
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c( *errhandler );
	MPI_Session c_session = PMPI_Session_f2c( *session );
	*ierror = PMPI_Session_set_errhandler( c_session, c_errhandler );
}

#pragma weak mpi_session_set_errhandler__ = pmpi_session_set_errhandler__
void pmpi_session_set_errhandler__(MPI_Fint * session, MPI_Fint * errhandler, int *ierror)
{
	pmpi_session_set_errhandler_( session, errhandler, ierror );
}

#pragma weak mpi_session_get_errhandler_ = pmpi_session_get_errhandler_
void pmpi_session_get_errhandler_(MPI_Fint * session, MPI_Fint *errhandler, int * ierror)
{
	MPI_Session c_session = PMPI_Session_f2c( *session );

	MPI_Errhandler c_errhandler;

	*ierror = PMPI_Session_get_errhandler( c_session, &c_errhandler );

	*errhandler = PMPI_Errhandler_c2f( c_errhandler );
}

#pragma weak mpi_session_get_errhandler__ = pmpi_session_get_errhandler__
void pmpi_session_get_errhandler__(MPI_Fint * session, MPI_Fint *errhandler, int * ierror)
{
	pmpi_session_get_errhandler_( session, errhandler, ierror );
}

#pragma weak mpi_session_call_errhandler_ = pmpi_session_call_errhandler_
void pmpi_session_call_errhandler_(MPI_Fint *  session, MPI_Fint *  error_code, int *ierror)
{
	int c_error_code = *error_code;
	MPI_Session c_session = PMPI_Session_f2c( *session );

	*ierror = PMPI_Session_call_errhandler( c_session, c_error_code );
}

#pragma weak mpi_session_call_errhandler__ = pmpi_session_call_errhandler__
void pmpi_session_call_errhandler__(MPI_Fint *  session, MPI_Fint *  error_code, int *ierror)
{
	pmpi_session_call_errhandler_( session, error_code, ierror );
}

#pragma weak mpi_session_init_ = pmpi_session_init_
void pmpi_session_init_(MPI_Fint *  info, MPI_Fint *  errhandler, MPI_Fint * session, int *ierror)
{
	MPI_Info c_info = PMPI_Info_f2c( *info );
	MPI_Errhandler c_errh = PMPI_Errhandler_f2c(*errhandler);

	MPI_Session c_session;
	*ierror = PMPI_Session_init( c_info, c_errh , &c_session);

	*session = PMPI_Session_c2f( c_session );
}

#pragma weak mpi_session_init__ = pmpi_session_init__
void pmpi_session_init__(MPI_Fint *  info, MPI_Fint *  errhandler, MPI_Fint * session, int *ierror)
{
	pmpi_session_init_( info, errhandler, session, ierror );
}

#pragma weak mpi_session_finalize_ = pmpi_session_finalize_
void pmpi_session_finalize_(MPI_Fint *session, int *ierror)
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	*ierror = PMPI_Session_finalize( &c_session );
	*session = PMPI_Session_c2f( c_session );
}

#pragma weak mpi_session_finalize__ = pmpi_session_finalize__
void pmpi_session_finalize__(MPI_Fint *session, int *ierror)
{
	pmpi_session_finalize_( session, ierror );
}

#pragma weak mpi_session_get_info_ = pmpi_session_get_info_
void pmpi_session_get_info_(MPI_Fint * session, MPI_Fint * info_used, int *ierror)
{
	MPI_Info c_info;
	MPI_Session c_session = PMPI_Session_f2c( *session );

	*ierror = PMPI_Session_get_info(c_session, &c_info);

	*info_used = PMPI_Info_c2f( c_info );
}

#pragma weak mpi_session_get_num_psets_ = pmpi_session_get_num_psets_
void pmpi_session_get_num_psets_(MPI_Fint * session, MPI_Fint *  info, MPI_Fint *npset_names, int *ierror)
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Info c_info = PMPI_Info_f2c( *info);

	int num;

	*ierror = PMPI_Session_get_num_psets( c_session, c_info, &num );

	*npset_names = num;
}

#pragma weak mpi_session_get_num_psets__ = pmpi_session_get_num_psets__
void pmpi_session_get_num_psets__(MPI_Fint * session, MPI_Fint *  info, MPI_Fint *npset_names, int *ierror)
{
	pmpi_session_get_num_psets_( session, info, npset_names, ierror );
}

#pragma weak mpi_session_get_nth_pset_ = pmpi_session_get_nth_pset_
void pmpi_session_get_nth_pset_(MPI_Fint * session, MPI_Fint * info, MPI_Fint * n, MPI_Fint *pset_len, char *pset_name CHAR_MIXED(size_string), int *ierror CHAR_END(size_string) )
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Info c_info = PMPI_Info_f2c( *info );

	int len = *pset_len;
	int c_n = *n;


	int ret = PMPI_Session_get_nth_pset( c_session, c_info, c_n, &len, pset_name );

	kill(0, 2);

	if(ierror)
	{
		*ierror = ret;
	}

	if(*pset_len && pset_name)
	{
		char_c_to_fortran(pset_name, size_string);
	}

	*pset_len = len;
}

#pragma weak mpi_session_get_nth_pset__ = pmpi_session_get_nth_pset__
void pmpi_session_get_nth_pset__(MPI_Fint * session, MPI_Fint * info, MPI_Fint * n, MPI_Fint *pset_len, char *pset_name CHAR_MIXED(size_string), int *ierror CHAR_END(size_string) )
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Info c_info = PMPI_Info_f2c( *info );

	int len = *pset_len;
	int c_n = *n;

	int ret = PMPI_Session_get_nth_pset( c_session, c_info, c_n, &len, pset_name );

	if(ierror)
	{
		*ierror = ret;
	}

	if(*pset_len && pset_name)
	{
		char_c_to_fortran(pset_name, size_string);
	}

	*pset_len = len;
}

#pragma weak mpi_group_from_session_pset_ = pmpi_group_from_session_pset_
void pmpi_group_from_session_pset_(MPI_Fint * session, const char *pset_name  CHAR_MIXED(size_string), MPI_Fint * newgroup, int *ierror CHAR_END(size_string))
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Group c_group;

	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)pset_name, size_string, &ptr_string);

	*ierror = PMPI_Group_from_session_pset( c_session, tmp_string, &c_group );

	sctk_free( ptr_string );

    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_group);
}

#pragma weak mpi_group_from_session_pset__ = pmpi_group_from_session_pset__
void pmpi_group_from_session_pset__(MPI_Fint * session, const char *pset_name  CHAR_MIXED(size_string), MPI_Fint * newgroup, int *ierror CHAR_END(size_string))
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Group c_group;

	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)pset_name, size_string, &ptr_string);

	*ierror = PMPI_Group_from_session_pset( c_session, tmp_string, &c_group );

	sctk_free( ptr_string );

    if( *ierror == MPI_SUCCESS)
        *newgroup = PMPI_Group_c2f(c_group);
}

#pragma weak mpi_comm_create_from_group_ = pmpi_comm_create_from_group_
void pmpi_comm_create_from_group_(MPI_Fint * group, const char * stringtag CHAR_MIXED(size_string), MPI_Fint * info, MPI_Fint * errhandler, MPI_Fint *newcomm, int *ierror CHAR_END(size_string) )
{
	MPI_Group c_group = PMPI_Group_f2c( *group );
	MPI_Info c_info = PMPI_Info_f2c( *info );
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c( *errhandler );


	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)stringtag, size_string, &ptr_string);

	MPI_Comm c_newcomm;

	*ierror = PMPI_Comm_create_from_group(c_group, tmp_string, c_info, c_errhandler, &c_newcomm);

	sctk_free( ptr_string );

    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_comm_create_from_group__ = pmpi_comm_create_from_group__
void pmpi_comm_create_from_group__(MPI_Fint * group, const char * stringtag CHAR_MIXED(size_string), MPI_Fint * info, MPI_Fint * errhandler, MPI_Fint *newcomm, int *ierror CHAR_END(size_string) )
{
	MPI_Group c_group = PMPI_Group_f2c( *group );
	MPI_Info c_info = PMPI_Info_f2c( *info );
	MPI_Errhandler c_errhandler = PMPI_Errhandler_f2c( *errhandler );


	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)stringtag, size_string, &ptr_string);

	MPI_Comm c_newcomm;

	*ierror = PMPI_Comm_create_from_group(c_group, tmp_string, c_info, c_errhandler, &c_newcomm);

	sctk_free( ptr_string );

    if( *ierror== MPI_SUCCESS)
        *newcomm = PMPI_Comm_c2f(c_newcomm);
}

#pragma weak mpi_session_get_pset_info_ = pmpi_session_get_pset_info_
void pmpi_session_get_pset_info_(MPI_Fint *  session, const char *pset_name  CHAR_MIXED(size_string), MPI_Fint *info, int * ierror CHAR_END(size_string) )
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Info c_info;

	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)pset_name, size_string, &ptr_string);

	*ierror = PMPI_Session_get_pset_info( c_session, tmp_string, &c_info );

	sctk_free( ptr_string );

	*info = PMPI_Info_c2f( c_info );
}

#pragma weak mpi_session_get_pset_info__ = pmpi_session_get_pset_info__
void pmpi_session_get_pset_info__(MPI_Fint *  session, const char *pset_name  CHAR_MIXED(size_string), MPI_Fint *info, int * ierror CHAR_END(size_string) )
{
	MPI_Session c_session = PMPI_Session_f2c( *session );
	MPI_Info c_info;

	char *tmp_string = NULL, *ptr_string = NULL;

	tmp_string = char_fortran_to_c( (char *)pset_name, size_string, &ptr_string);

	*ierror = PMPI_Session_get_pset_info( c_session, tmp_string, &c_info );

	sctk_free( ptr_string );

	*info = PMPI_Info_c2f( c_info );
}

#pragma weak mpi_session_get_info__ = pmpi_session_get_info__
void pmpi_session_get_info__(MPI_Fint * session, MPI_Fint * info_used, int *ierror)
{
	pmpi_session_get_info_( session, info_used, ierror );
}

#pragma weak mpi_file_create_errhandler_ = pmpi_file_create_errhandler_
void pmpi_file_create_errhandler_( MPI_File_errhandler_function *errhandler_fn, MPI_Fint *errhandler, int *ierror )
{
	MPI_Errhandler c_errhandler;

	*ierror = PMPI_File_create_errhandler( errhandler_fn, &c_errhandler );
	*errhandler = PMPI_Errhandler_c2f( c_errhandler );
}

#pragma weak mpi_file_create_errhandler__ = pmpi_file_create_errhandler__
void pmpi_file_create_errhandler__( MPI_File_errhandler_function *errhandler_fn, MPI_Fint *errhandler, int *ierror )
{
	pmpi_file_create_errhandler__( errhandler_fn, errhandler, ierror );
}

MPI_File PMPI_File_f2c(int file);

#pragma weak mpi_file_call_errhandler_ = pmpi_file_call_errhandler_
void pmpi_file_call_errhandler_(MPI_Fint *file, int *errorcode, int *ierror)
{
/* MPI_Comm_call_errhandler */
	MPI_File c_file = PMPI_File_f2c(*file);

	*ierror = MPI_File_call_errhandler(c_file, *errorcode);
}

#pragma weak mpi_file_call_errhandler__ = pmpi_file_call_errhandler__
void pmpi_file_call_errhandler__(MPI_Fint *file, int *errorcode, int *ierror)
{
/* MPI_Comm_call_errhandler */
	MPI_File c_file = PMPI_File_f2c(*file);

	*ierror = MPI_File_call_errhandler(c_file, *errorcode);
}

/* NULL Functions */

#pragma weak mpi_null_delete_fn_ = pmpi_null_delete_fn_
void pmpi_null_delete_fn_( MPI_Fint *v1, MPI_Fint *v2,
						  void *v3,
						  void *v4,
						  int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_null_delete_fn__ = pmpi_null_delete_fn__
void pmpi_null_delete_fn__( MPI_Fint *v1, MPI_Fint *v2,
						   void *v3,
						   void *v4,
						   int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_comm_null_delete_fn_ = pmpi_comm_null_delete_fn_
void pmpi_comm_null_delete_fn_( MPI_Fint *v1, MPI_Fint *v2,
							   void *v3,
							   void *v4,
							   int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_comm_null_delete_fn__ = pmpi_comm_null_delete_fn__
void pmpi_comm_null_delete_fn__( MPI_Fint *v1, MPI_Fint *v2,
								void *v3,
								void *v4,
								int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_comm_null_copy_fn_ = pmpi_comm_null_copy_fn_
void pmpi_comm_null_copy_fn_( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, void *v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_comm_null_copy_fn__ = pmpi_comm_null_copy_fn__
void pmpi_comm_null_copy_fn__( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, void *v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_dup_fn_ = pmpi_dup_fn__
void pmpi_dup_fn_ ( MPI_Fint v1, MPI_Fint*v2, void*v3, void**v4, void**v5, MPI_Fint*v6, MPI_Fint *ierr ){
	*v5 = *v4;
	*v6 = (1);
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_dup_fn__ = pmpi_dup_fn_
void pmpi_dup_fn__ ( MPI_Fint v1, MPI_Fint*v2, void*v3, void**v4, void**v5, MPI_Fint*v6, MPI_Fint *ierr ){
	*v5 = *v4;
	*v6 = (1);
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_comm_dup_fn_ = pmpi_comm_dup_fn_
void pmpi_comm_dup_fn_( MPI_Fint v1, MPI_Fint *v2, void *v3, void **v4, void **v5, MPI_Fint *v6, int *ierr )
{
	*v5 = *v4;
	*v6 = 1;
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_comm_dup_fn__ = pmpi_comm_dup_fn__
void pmpi_comm_dup_fn__( MPI_Fint v1, MPI_Fint *v2, void *v3, void **v4, void **v5, MPI_Fint *v6, int *ierr )
{
	*v5 = *v4;
	*v6 = 1;
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_null_copy_fn_ = pmpi_null_copy_fn_
void pmpi_null_copy_fn_( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, void *v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_null_copy_fn__ = pmpi_null_copy_fn__
void pmpi_null_copy_fn__( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, void *v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_type_null_delete_fn_ = pmpi_type_null_delete_fn_
void pmpi_type_null_delete_fn_( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_type_null_delete_fn__ = pmpi_type_null_delete_fn__
void pmpi_type_null_delete_fn__( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_type_dup_fn_ = pmpi_type_dup_fn_
void pmpi_type_dup_fn_( MPI_Fint v1, MPI_Fint *v2, void *v3, void **v4, void **v5, MPI_Fint *v6, int *ierr )
{
	*v5 = *v4;
	*v6 = 1;
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_type_dup_fn__ = pmpi_type_dup_fn__
void pmpi_type_dup_fn__( MPI_Fint v1, MPI_Fint *v2, void *v3, void **v4, void **v5, MPI_Fint *v6, int *ierr )
{
	*v5 = *v4;
	*v6 = 1;
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_type_null_copy_fn_ = pmpi_type_null_copy_fn_
void pmpi_type_null_copy_fn_( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, void *v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_type_null_copy_fn__ = pmpi_type_null_copy_fn__
void pmpi_type_null_copy_fn__( MPI_Fint *v1, MPI_Fint *v2, void *v3, void *v4, void *v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_win_null_delete_fn_ = pmpi_win_null_delete_fn_
void pmpi_win_null_delete_fn_ ( MPI_Fint*v1, MPI_Fint*v2, void*v3, void*v4, int *ierr )
{
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_win_null_delete_fn__ = pmpi_win_null_delete_fn__
void pmpi_win_null_delete_fn__ ( MPI_Fint*v1, MPI_Fint*v2, void*v3, void*v4, int *ierr )
{
    *ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_win_dup_fn_ = pmpi_win_dup_fn_
void pmpi_win_dup_fn_ ( MPI_Fint v1, MPI_Fint*v2, void*v3, void**v4, void**v5, MPI_Fint*v6, int *ierr )
{
	*v5 = *v4;
	*v6 = 1;
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_win_dup_fn__ = pmpi_win_dup_fn__
void pmpi_win_dup_fn__ ( MPI_Fint v1, MPI_Fint*v2, void*v3, void**v4, void**v5, MPI_Fint*v6, int *ierr )
{
	*v5 = *v4;
	*v6 = 1;
	*ierr = MPI_SUCCESS;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
}

#pragma weak mpi_win_null_copy_fn_ = pmpi_win_null_copy_fn_
void pmpi_win_null_copy_fn_ ( MPI_Fint*v1, MPI_Fint*v2, void*v3, void*v4, void*v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_win_null_copy_fn__ = pmpi_win_null_copy_fn__
void pmpi_win_null_copy_fn__ ( MPI_Fint*v1, MPI_Fint*v2, void*v3, void*v4, void*v5, MPI_Fint *v6, int *ierr )
{
	*ierr = MPI_SUCCESS;
	*v6 = 0;
	UNUSED(v1);
	UNUSED(v2);
	UNUSED(v3);
	UNUSED(v4);
	UNUSED(v5);
}

#pragma weak mpi_pcontrol_ = pmpi_pcontrol_
void pmpi_pcontrol_ ( MPI_Fint *v1, MPI_Fint *ierr ){
     *ierr = MPI_Pcontrol( (int)*v1 );
}

#pragma weak mpi_pcontrol__ = pmpi_pcontrol__
void pmpi_pcontrol__ ( MPI_Fint *v1, MPI_Fint *ierr ){
     pmpi_pcontrol_(v1, ierr);
}

#pragma weak mpi_address_ = pmpi_address_
void pmpi_address_ ( void*v1, MPI_Fint *v2, MPI_Fint *ierr ){
     MPI_Aint a;
     *ierr = MPI_Address( v1, &a );
     *v2 = (MPI_Fint)( a );
}

#pragma weak mpi_address__ = pmpi_address__
void pmpi_address__ ( void*v1, MPI_Fint *v2, MPI_Fint *ierr ){
	pmpi_address_(v1, v2, ierr);
}

#pragma weak mpi_errhandler_create_ = pmpi_errhandler_create_
void pmpi_errhandler_create_ ( MPI_Handler_function*v1, MPI_Fint *v2, MPI_Fint *ierr ){
     *ierr = MPI_Errhandler_create( v1, v2 );
}

#pragma weak mpi_errhandler_create__ = pmpi_errhandler_create__
void pmpi_errhandler_create__ ( MPI_Handler_function*v1, MPI_Fint *v2, MPI_Fint *ierr ){
     pmpi_errhandler_create_( v1, v2, ierr );
}
