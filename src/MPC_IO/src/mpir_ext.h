#ifndef MPIR_EXT
#define MPIR_EXT

#include <stdarg.h>
#include <mpc_mpi.h>

int MPIR_Err_create_code_valist(int a, int b, const char c[], int d, int e,
                                const char f[], const char g[], va_list args);


typedef int (*MPIR_Err_get_class_string_func_t)(int error, char *str, int length);

void MPIR_Err_get_string(int errcode, char *msg, int maxlen, MPIR_Err_get_class_string_func_t fcname);

int MPIR_Err_is_fatal(int a);

void MPIR_Get_file_error_routine(MPI_Errhandler a,
                                 void(**errr)(void *, int *, ...),
                                 int *b);

int MPIR_Abort(MPI_Comm comm, int mpi_errno, int exit_code, const char *error_msg);

void MPIR_Ext_cs_yield(void);

#endif /* MPIR_EXT */
