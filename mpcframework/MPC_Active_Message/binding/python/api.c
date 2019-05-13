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

char reg_docs[] = "Register a new Service";

static PyObject* register_service(PyObject* self, PyObject* args)
{
	fprintf(stderr, "Call in %s\n", __FUNCTION__);
	return Py_None;
}

static PyObject* polling_request(PyObject* self, PyObject* args)
{
	fprintf(stderr, "Call in %s\n", __FUNCTION__);
	return Py_None;
}

static PyObject* emit_call(PyObject* self, PyObject* args)
{
	fprintf(stderr, "Call in %s\n", __FUNCTION__);
	return Py_None;
}

static struct PyMethodDef methods[] = {
	{"register_service", (PyCFunction)register_service, METH_VARARGS, NULL },
	{"emit_call", (PyCFunction)emit_call, METH_VARARGS, NULL},
	{"polling_request", (PyCFunction)polling_request, METH_VARARGS, NULL},
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
	return 1;
}
