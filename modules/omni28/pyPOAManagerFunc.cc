// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyPOAManagerFunc.cc        Created on: 2000/02/04
//                            Author    : Duncan Grisby (dpg1)
//
//    Copyright (C) 1999 AT&T Laboratories Cambridge
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
//    POAManager functions

// $Id$

// $Log$
// Revision 1.1  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//

#include <omnipy.h>

#define RAISE_PY_NO_IMPLEMENT \
  CORBA::NO_IMPLEMENT _ex; return omniPy::handleSystemException(_ex)

extern "C" {

  static PyObject* pyPM_activate(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPM)) return NULL;

    PyObject* active = PyObject_GetAttrString(pyPM, (char*)"_omni_active");
    if (active) return active;

    PyErr_Clear();

    CORBA::BOA::getBOA()->impl_is_ready(0,1);

    PyObject_SetAttrString(pyPM, (char*)"_omni_active", Py_None);
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject* pyPM_hold_requests(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPM_discard_requests(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPM_deactivate(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPM_get_state(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPM)) return NULL;

    int active = PyObject_HasAttrString(pyPM, (char*)"_omni_active");

    if (active) {
      return PyInt_FromLong(1); // ACTIVE
    }
    else
      return PyInt_FromLong(0);
  }

  static PyObject* pyPM_releaseRef(PyObject* self, PyObject* args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }


  static PyMethodDef pyPM_methods[] = {
    {(char*)"activate",         pyPM_activate,         METH_VARARGS},
    {(char*)"hold_requests",    pyPM_hold_requests,    METH_VARARGS},
    {(char*)"discard_requests", pyPM_discard_requests, METH_VARARGS},
    {(char*)"deactivate",       pyPM_deactivate,       METH_VARARGS},
    {(char*)"get_state",        pyPM_get_state,        METH_VARARGS},

    {(char*)"releaseRef",       pyPM_releaseRef,       METH_VARARGS},
    {NULL,NULL}
  };
}

void
omniPy::initPOAManagerFunc(PyObject* d)
{
  PyObject* m = Py_InitModule((char*)"_omnipy.poamanager_func", pyPM_methods);
  PyDict_SetItemString(d, (char*)"poamanager_func", m);
}
