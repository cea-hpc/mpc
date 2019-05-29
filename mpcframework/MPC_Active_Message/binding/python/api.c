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
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string.h>
#include <arpc.h>
#include <arpc_common.h>

PyObject * convert_cb = NULL;

static PyObject* reg_cb(PyObject* self, PyObject* args, PyObject* keywords)
{
	PyObject* tmp;

	static char *kwds[] = {"callback"};
	if(PyArg_ParseTupleAndKeywords(args, keywords, "O:register_callback", kwds, &tmp))
	{
		if(!PyCallable_Check(tmp))
		{
			PyErr_SetString(PyExc_TypeError, "Provided argument must be callable");
			return NULL;
		}
		Py_XINCREF(tmp);
		Py_XDECREF(convert_cb);
		convert_cb = tmp;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* polling_request(PyObject* self, PyObject* args, PyObject* keywords)
{
	//fprintf(stderr, "Call in %s\n", __FUNCTION__);

	sctk_arpc_context_t ctx;
	arpc_polling_request(&ctx);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* emit_call(PyObject* self, PyObject* args, PyObject* keywords)
{
	//fprintf(stderr, "Call in %s\n", __FUNCTION__);

	PyObject* ret = NULL;
	sctk_arpc_context_t ctx;
	void* request = NULL, *response = NULL;
	size_t req_sz = 0, resp_sz = 1;
	int need_resp = 0;

	static char * kwds[] = {"service", "rpc", "dest", "request", "with_resp"};

	if(!PyArg_ParseTupleAndKeywords(args, keywords, "iiiy#i", kwds, &ctx.srvcode, &ctx.rpcode, &ctx.dest, &request, &req_sz, &need_resp))
	{
		return NULL;
	}

	arpc_emit_call(&ctx, request, req_sz, &response, &resp_sz);

	if(need_resp)
	{
		ret = Py_BuildValue("y#", response, resp_sz);
		arpc_free_response(response);
	}
	else
	{
		ret = Py_BuildValue("(,i)", 0);
	}
	Py_INCREF(ret);

	return ret;
}

static struct PyMethodDef methods[] = {
	{"register_callback" , (PyCFunction)reg_cb          , METH_VARARGS | METH_KEYWORDS, NULL} ,
	{"emit_call"         , (PyCFunction)emit_call       , METH_VARARGS | METH_KEYWORDS, NULL} ,
	{"polling_request"   , (PyCFunction)polling_request , METH_VARARGS | METH_KEYWORDS, NULL} ,
	{ NULL               , NULL                         , 0            , NULL}
};

char mod_doc[] = "Python bindings to support ARPC interface";
char *argv[] = {"arpc4py.so"};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef _arpc4py_module = {
	PyModuleDef_HEAD_INIT,
	"arpc4py",
	mod_doc, 
	-1,
	methods,
};

PyMODINIT_FUNC PyInit_arpc4py(void)
{
	sctk_init_mpc_runtime();
	return PyModule_Create(&_arpc4py_module);
}

#else
void initarpc4py(void)
{
	main_c(1, argv);
	Py_InitModule3("arpc4py", methods, mod_doc);
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


int arpc_c_to_cxx_converter(sctk_arpc_context_t* ctx, const void * request, size_t req_size, void** response, size_t* resp_size)
{
	PyObject* args = NULL, *result = NULL;

	if(!convert_cb)
		return -1;

	args = Py_BuildValue("(iiiy#)", ctx->srvcode, ctx->rpcode, ctx->dest, request, req_size);
	result = PyObject_CallObject(convert_cb, args);
	Py_XDECREF(args);
	if(result && PyBytes_Check(result))
	{
		char * tmp_resp = NULL;
		if(PyBytes_AsStringAndSize(result, &tmp_resp, (Py_ssize_t*)resp_size) == -1)
		{
			return -1;
		}

		*response = strndup(tmp_resp, *resp_size);
		Py_XDECREF(result);
	}

	return 0;
}
