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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#include <string.h>
#include <Python.h>

struct sctk_arpc_context
{
	int dest;
	int rpcode;
	int srvcode;
	void* cxx_pool;
};
typedef struct sctk_arpc_context sctk_arpc_context_t;

int arpc_register_service(void* pool, int srvcode){}
int arpc_free_response(void* addr){}
int arpc_emit_call(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void** response, size_t* resp_size){}
int arpc_recv_call(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void** response, size_t* resp_size){}

int arpc_polling_request(sctk_arpc_context_t* ctx){}

/* injected into the application code when building */
#pragma weak arpc_c_to_cxx_converter
int arpc_c_to_cxx_converter(sctk_arpc_context_t* ctx, const void * request, size_t req_size, void** response, size_t* resp_size){}


char reg_docs[] = "Register a new Service";

static struct PyMethodDef methods[] = {
	{"register_service", (PyCFunction)arpc_register_service, METH_VARARGS, reg_docs },
	{ NULL }
};

char mod_docs[] = "Arpc module for Python (relies on MPC-MPI implementation.)";


#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef _arpc4py_module = {
	PyModuleDef_HEAD_INIT,
	"arpc4py",
	mod_docs, 
	-1,
	methods,
	NULL,
	NULL,
	NULL,
	NULL

};

PyObject* PyInit_arpc4py(void)
{
	return PyModule_Create(&_arpc4py_module);
}

#else
void initarpc4py(void)
{
	Py_InitModule3("arpc4py", methods, mod_docs);
}
#endif

// because MPC does not compile/privatize a Python Script...
#pragma weak mpc_user_main__
int mpc_user_main__(void)
{
	fprintf(stderr, "mpc_user_main__() from Python bindings should never be called ! SEGV\n");
	((void(*)())0x0)();
}
