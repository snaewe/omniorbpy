// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyomniFunc.cc              Created on: 2000/06/07
//                            Author    : Duncan Grisby (dpg1)
//
//    Copyright (C) 2000 AT&T Laboratories Cambridge
//
//    This file is part of the omniORBpy library
//
//    The omniORBpy library is free software; you can redistribute it
//    and/or modify it under the terms of the GNU Lesser General
//    Public License as published by the Free Software Foundation;
//    either version 2.1 of the License, or (at your option) any later
//    version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//    MA 02111-1307, USA
//
//
// Description:
//    omniORB API functions

// $Id$

// $Log$
// Revision 1.1.2.1  2000/10/13 13:55:28  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>
#include <pyThreadCache.h>


static PyObject* transientEHtuple   = 0;
static PyObject* commFailureEHtuple = 0;
static PyObject* systemEHtuple      = 0;


static CORBA::Boolean transientEH(void* cookie, CORBA::ULong retries,
				  const CORBA::TRANSIENT& ex)
{
  PyObject* tuple;

  if (cookie)
    tuple = (PyObject*)cookie;
  else
    tuple = transientEHtuple;

  OMNIORB_ASSERT(PyTuple_Check(tuple));

  PyObject* pyfn     = PyTuple_GET_ITEM(tuple, 0);
  PyObject* pycookie = PyTuple_GET_ITEM(tuple, 1);
  PyObject* r;
  CORBA::Boolean ret;

  {
    omnipyThreadCache::lock _t;
    PyObject* pyex = omniPy::createPySystemException(ex);
    r = PyObject_CallFunction(pyfn, (char*)"OiN", pycookie, retries, pyex);
    
    if (!r) {
      if (omniORB::trace(1)) {
	omniORB::logf("Python TRANSIENT exception handler failed. "
		      "Traceback follows:");
	PyErr_Print();
      }
      else
	PyErr_Clear();
      return 0;
    }
    if (!PyInt_Check(r)) {
      if (omniORB::trace(1))
	omniORB::logf("Python TRANSIENT exception handler returned an "
		      "invalid object.");
      Py_DECREF(r);
      return 0;
    }
    ret = PyInt_AS_LONG(r);
    Py_DECREF(r);
  }
  return ret;
}


static CORBA::Boolean commFailureEH(void* cookie, CORBA::ULong retries,
				    const CORBA::COMM_FAILURE& ex)
{
  PyObject* tuple;

  if (cookie)
    tuple = (PyObject*)cookie;
  else
    tuple = commFailureEHtuple;

  OMNIORB_ASSERT(PyTuple_Check(tuple));

  PyObject* pyfn     = PyTuple_GET_ITEM(tuple, 0);
  PyObject* pycookie = PyTuple_GET_ITEM(tuple, 1);
  PyObject* r;
  CORBA::Boolean ret;

  {
    omnipyThreadCache::lock _t;
    PyObject* pyex = omniPy::createPySystemException(ex);
    r = PyObject_CallFunction(pyfn, (char*)"OiN", pycookie, retries, pyex);
    
    if (!r) {
      if (omniORB::trace(1)) {
	omniORB::logf("Python COMM_FAILURE exception handler failed. "
		      "Traceback follows:");
	PyErr_Print();
      }
      else
	PyErr_Clear();
      return 0;
    }
    if (!PyInt_Check(r)) {
      if (omniORB::trace(1))
	omniORB::logf("Python COMM_FAILURE exception handler returned an "
		      "invalid object.");
      Py_DECREF(r);
      return 0;
    }
    ret = PyInt_AS_LONG(r);
    Py_DECREF(r);
  }
  return ret;
}

static CORBA::Boolean systemEH(void* cookie, CORBA::ULong retries,
			       const CORBA::SystemException& ex)
{
  PyObject* tuple;

  if (cookie)
    tuple = (PyObject*)cookie;
  else
    tuple = systemEHtuple;

  OMNIORB_ASSERT(PyTuple_Check(tuple));

  PyObject* pyfn     = PyTuple_GET_ITEM(tuple, 0);
  PyObject* pycookie = PyTuple_GET_ITEM(tuple, 1);
  PyObject* r;
  CORBA::Boolean ret;

  {
    omnipyThreadCache::lock _t;
    PyObject* pyex = omniPy::createPySystemException(ex);
    r = PyObject_CallFunction(pyfn, (char*)"OiN", pycookie, retries, pyex);
    
    if (!r) {
      if (omniORB::trace(1)) {
	omniORB::logf("Python SystemException handler failed. "
		      "Traceback follows:");
	PyErr_Print();
      }
      else
	PyErr_Clear();
      return 0;
    }
    if (!PyInt_Check(r)) {
      if (omniORB::trace(1))
	omniORB::logf("Python SystemException handler returned an "
		      "invalid object.");
      Py_DECREF(r);
      return 0;
    }
    ret = PyInt_AS_LONG(r);
    Py_DECREF(r);
  }
  return ret;
}



extern "C" {

  static char transient_doc [] =
  "installTransientExceptionHandler(cookie, function [, object]) -> None\n"
  "\n"
  "Install a handler for TRANSIENT exceptions, for all objects or a\n"
  "specified object. The function must have the signature:\n"
  "\n"
  "  function(cookie, retries, exc) -> boolean\n"
  "\n"
  "where cookie is the object passed on installation, retries is the\n"
  "number of times this operation invocation has been retried, and exc is\n"
  "the TRANSIENT exception object. If the function returns TRUE, the\n"
  "operation invocation is retried; if it returns FALSE, the TRANSIENT\n"
  "exception is thrown to the application.\n";

  static PyObject* pyomni_installTransientExceptionHandler(PyObject* self,
							   PyObject* args)
  {
    PyObject *pycookie, *pyfn, *pyobjref = 0;

    if (!PyArg_ParseTuple(args, (char*)"OO|O", &pycookie, &pyfn, &pyobjref))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyCallable_Check(pyfn));
    RAISE_PY_BAD_PARAM_IF(pyobjref && !PyInstance_Check(pyobjref));

    if (pyobjref) {
      CORBA::Object_ptr objref =
	(CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

      RAISE_PY_BAD_PARAM_IF(!objref);

      PyObject* tuple = Py_BuildValue((char*)"OO", pyfn, pycookie);
      PyObject_SetAttrString(pyobjref, (char*)"__omni_transient", tuple);
      omniORB::installTransientExceptionHandler(objref, (void*)tuple,
						transientEH);
    }
    else {
      Py_XDECREF(transientEHtuple);
      transientEHtuple = Py_BuildValue((char*)"OO", pyfn, pycookie);
      OMNIORB_ASSERT(transientEHtuple);
      omniORB::installTransientExceptionHandler((void*)transientEHtuple,
						transientEH);
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  static char commfailure_doc [] =
  "installCommFailureExceptionHandler(cookie, function [, object]) -> None\n"
  "\n"
  "Install a handler for COMM_FAILURE exceptions, for all objects or a\n"
  "specified object. The function must have the signature:\n"
  "\n"
  "  function(cookie, retries, exc) -> boolean\n"
  "\n"
  "where cookie is the object passed on installation, retries is the\n"
  "number of times this operation invocation has been retried, and exc is\n"
  "the COMM_FAILURE exception object. If the function returns TRUE, the\n"
  "operation invocation is retried; if it returns FALSE, the COMM_FAILURE\n"
  "exception is thrown to the application.\n";

  static PyObject* pyomni_installCommFailureExceptionHandler(PyObject* self,
							     PyObject* args)
  {
    PyObject *pycookie, *pyfn, *pyobjref = 0;

    if (!PyArg_ParseTuple(args, (char*)"OO|O", &pycookie, &pyfn, &pyobjref))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyCallable_Check(pyfn));
    RAISE_PY_BAD_PARAM_IF(pyobjref && !PyInstance_Check(pyobjref));

    if (pyobjref) {
      CORBA::Object_ptr objref =
	(CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

      RAISE_PY_BAD_PARAM_IF(!objref);

      PyObject* tuple = Py_BuildValue((char*)"OO", pyfn, pycookie);
      PyObject_SetAttrString(pyobjref, (char*)"__omni_commfailure", tuple);
      omniORB::installCommFailureExceptionHandler(objref, (void*)tuple,
						  commFailureEH);
    }
    else {
      Py_XDECREF(commFailureEHtuple);
      commFailureEHtuple = Py_BuildValue((char*)"OO", pyfn, pycookie);
      OMNIORB_ASSERT(commFailureEHtuple);
      omniORB::installCommFailureExceptionHandler((void*)commFailureEHtuple,
						  commFailureEH);
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  static char system_doc [] =
  "installSystemExceptionHandler(cookie, function [, object]) -> None\n"
  "\n"
  "Install a handler for system exceptions other than TRANSIENT and\n"
  "COMM_FAILURE, for all objects or a specified object. The function must\n"
  "have the signature:\n"
  "\n"
  "  function(cookie, retries, exc) -> boolean\n"
  "\n"
  "where cookie is the object passed on installation, retries is the\n"
  "number of times this operation invocation has been retried, and exc is\n"
  "the SystemException object. If the function returns TRUE, the\n"
  "operation invocation is retried; if it returns FALSE, the exception is\n"
  "thrown to the application.\n";

  static PyObject* pyomni_installSystemExceptionHandler(PyObject* self,
							PyObject* args)
  {
    PyObject *pycookie, *pyfn, *pyobjref = 0;

    if (!PyArg_ParseTuple(args, (char*)"OO|O", &pycookie, &pyfn, &pyobjref))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyCallable_Check(pyfn));
    RAISE_PY_BAD_PARAM_IF(pyobjref && !PyInstance_Check(pyobjref));

    if (pyobjref) {
      CORBA::Object_ptr objref =
	(CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

      RAISE_PY_BAD_PARAM_IF(!objref);

      PyObject* tuple = Py_BuildValue((char*)"OO", pyfn, pycookie);
      PyObject_SetAttrString(pyobjref, (char*)"__omni_systemex", tuple);
      omniORB::installSystemExceptionHandler(objref, (void*)tuple,
					     systemEH);
    }
    else {
      Py_XDECREF(systemEHtuple);
      systemEHtuple = Py_BuildValue((char*)"OO", pyfn, pycookie);
      OMNIORB_ASSERT(systemEHtuple);
      omniORB::installSystemExceptionHandler((void*)systemEHtuple,
					     systemEH);
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  static char traceLevel_doc [] =
  "traceLevel(int) -> None\n"
  "traceLevel()    -> int\n"
  "\n"
  "Set or get the omniORB debug trace level.\n";

  static PyObject* pyomni_traceLevel(PyObject* self, PyObject* args)
  {
    if (PyTuple_GET_SIZE(args) == 0) {
      return PyInt_FromLong(omniORB::traceLevel);
    }
    else if (PyTuple_GET_SIZE(args) == 1) {
      PyObject* pytl = PyTuple_GET_ITEM(args, 0);

      if (PyInt_Check(pytl)) {
	omniORB::traceLevel = PyInt_AS_LONG(pytl);
	Py_INCREF(Py_None);
	return Py_None;
      }
    }
    PyErr_SetString(PyExc_TypeError,
		    (char*)"Operation requires a single integer argument");
    return 0;
  }

  static PyMethodDef pyomni_methods[] = {
    {(char*)"installTransientExceptionHandler",
     pyomni_installTransientExceptionHandler,
     METH_VARARGS, transient_doc},

    {(char*)"installCommFailureExceptionHandler",
     pyomni_installCommFailureExceptionHandler,
     METH_VARARGS, commfailure_doc},

    {(char*)"installSystemExceptionHandler",
     pyomni_installSystemExceptionHandler,
     METH_VARARGS, system_doc},

    {(char*)"traceLevel",
     pyomni_traceLevel,
     METH_VARARGS, traceLevel_doc},

    {NULL,NULL}
  };
}

void
omniPy::initomniFunc(PyObject* d)
{
  PyObject* m = Py_InitModule((char*)"_omnipy.omni_func", pyomni_methods);
  PyDict_SetItemString(d, (char*)"omni_func", m);
}
