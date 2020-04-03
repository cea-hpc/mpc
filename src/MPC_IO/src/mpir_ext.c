#include "mpir_ext.h"

#include <sctk_debug.h>
#include <sctk_alloc.h>

/***********
* YIEDING *
***********/


/** \brief MPICH says check wether the progress engine is blocked assuming
 * "YIELD"
 */
void MPIR_Ext_cs_yield(void)
{
	mpc_thread_yield();
}

/******************
* ERROR HANDLING *
******************/

int MPIR_Err_create_code_valist(__UNUSED__ int lastcode, __UNUSED__ int fatal, const char fcname[],
                                int line, int error_class, const char generic_msg[],
                                const char specific_msg[], __UNUSED__ va_list Argp)
{
	char *buf;
	int   idx;

	buf = (char *)sctk_malloc(1024);
	if(buf != NULL)
	{
		idx += snprintf(buf, 1023, "%s (line %d): ", fcname, line);

		if(specific_msg == NULL)
		{
			snprintf(&buf[idx], 1023 - idx, "%s\n", generic_msg);
		}
		else
		{
			vsnprintf(&buf[idx], 1023 - idx, specific_msg, Argp);
		}
		sctk_error("%s", buf);
		sctk_free(buf);
	}
	return lastcode;
}

void MPIR_Err_get_string(int errcode, char *msg, int maxlen, __UNUSED__ MPIR_Err_get_class_string_func_t fcname)
{
	char buff[128];
	int  len;

	if(!msg)
	{
		return;
	}

	buff[0] = '\0';
	msg[0]  = '\0';

	_mpc_cl_error_string(errcode, buff, &len);

	if(strlen(buff) )
	{
		snprintf(msg, maxlen, "%s", buff);
	}
}

int MPIR_Err_is_fatal(int a)
{
	return 0;
}

void MPIR_Get_file_error_routine(__UNUSED__ MPI_Errhandler a,
                                 __UNUSED__ void(**errr)(void *, int *, ...),
                                 __UNUSED__ int *b)
{
}

int MPIR_Abort(MPI_Comm comm, int mpi_errno, int exit_code, const char *error_msg)
{
	sctk_error("ERRNO %d EXIT %d MSG: %s", mpi_errno, exit_code, error_msg);
	PMPI_Abort(comm, exit_code);
}
