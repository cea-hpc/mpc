#include "mpc_mpi.h"
#include "sctk_debug.h"
#include <string.h>

int MPIR_Err_create_code_valist(int a, int b, const char c[], int d, int e, 
				const char f[], const char g[], va_list args )
{


}

int MPIR_Err_is_fatal(int a)
{
	return 0;
}

typedef int (* MPIR_Err_get_class_string_func_t)(int error, char *str, int length);

void MPIR_Err_get_string( int errcode, char *msg, int maxlen, MPIR_Err_get_class_string_func_t fcname )
{
	char buff[128];
	int len;
	
	if( !msg )
		return;
	
	buff[0] = '\0';
	msg[0] = '\0';
	
	PMPC_Error_string (errcode, buff, &len);

	if( strlen( buff ) )
		snprintf( msg, maxlen, "%s", buff );
}


struct MPID_Comm;
int MPID_Abort(struct MPID_Comm *comm, int mpi_errno, int exit_code, const char *error_msg)
{
	sctk_error("FATAL : %d [exit : %d ] : %s", mpi_errno, exit_code, error_msg );
	MPI_Abort( MPI_COMM_WORLD, mpi_errno );
}


int MPIR_Status_set_bytes( MPI_Status status, MPI_Datatype datatype, size_t size )
{
	return MPI_Status_set_elements(&status, MPI_BYTE, size);
}


int PMPI_File_set_errhandler(  void * file,  MPI_Errhandler errhandler )
{
	
	
	
	
}


void MPIR_Get_file_error_routine( MPI_Errhandler a, 
				  void (**errr)(void * , int * , ...), 
				  int * b)
{
	
	
	
}


int MPIR_File_call_cxx_errhandler( void *fh, int *errorcode, 
			   void (*c_errhandler)(void  *, int *, ... ) )
{
	
	
}


int PMPI_Comm_call_errhandler(
  MPI_Comm comm,
  int errorcode
)
{
	
	
}
