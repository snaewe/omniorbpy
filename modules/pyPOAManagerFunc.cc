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
// Revision 1.1.2.1  2000/10/13 13:55:26  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>


PyObject*
omniPy::createPyPOAManagerObject(const PortableServer::POAManager_ptr pm)
{
  if (CORBA::is_nil(pm)) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject* pypm_class =
    PyObject_GetAttrString(omniPy::pyPortableServerModule,
			   (char*)"POAManager");
  OMNIORB_ASSERT(pypm_class);

  PyObject* pypm = PyEval_CallObject(pypm_class, omniPy::pyEmptyTuple);
  OMNIORB_ASSERT(pypm);

  omniPy::setTwin(pypm, (PortableServer::POAManager_ptr)pm, POAMANAGER_TWIN);
  omniPy::setTwin(pypm, (CORBA::Object_ptr)             pm, OBJREF_TWIN);
  return pypm;
}


extern "C" {

  static PyObject* pyPM_activate(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPM)) return NULL;

    PortableServer::POAManager_ptr pm =
      (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM, POAMANAGER_TWIN);

    OMNIORB_ASSERT(pm);

    try {
      pm->activate();
    }
    catch (PortableServer::POAManager::AdapterInactive& ex) {
      PyObject* excc = PyObject_GetAttrString(pyPM, (char*)"AdapterInactive");
      OMNIORB_ASSERT(excc);
      PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
      PyErr_SetObject(excc, exci);
      return 0;
    }
    Py_INCREF(Py_None); return Py_None;
  }

  static PyObject* pyPM_hold_requests(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    int       wfc;
    if (!PyArg_ParseTuple(args, (char*)"Oi", &pyPM, &wfc)) return NULL;

    PortableServer::POAManager_ptr pm =
      (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM, POAMANAGER_TWIN);

    OMNIORB_ASSERT(pm);

    try {
      pm->hold_requests(wfc);
    }
    catch (PortableServer::POAManager::AdapterInactive& ex) {
      PyObject* excc = PyObject_GetAttrString(pyPM, (char*)"AdapterInactive");
      OMNIORB_ASSERT(excc);
      PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
      PyErr_SetObject(excc, exci);
      return 0;
    }
    Py_INCREF(Py_None); return Py_None;
  }

  static PyObject* pyPM_discard_requests(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    int       wfc;
    if (!PyArg_ParseTuple(args, (char*)"Oi", &pyPM, &wfc)) return NULL;

    PortableServer::POAManager_ptr pm =
      (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM, POAMANAGER_TWIN);

    OMNIORB_ASSERT(pm);

    try {
      pm->discard_requests(wfc);
    }
    catch (PortableServer::POAManager::AdapterInactive& ex) {
      PyObject* excc = PyObject_GetAttrString(pyPM, (char*)"AdapterInactive");
      OMNIORB_ASSERT(excc);
      PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
      PyErr_SetObject(excc, exci);
      return 0;
    }
    Py_INCREF(Py_None); return Py_None;
  }

  static PyObject* pyPM_deactivate(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    int       eo, wfc;
    if (!PyArg_ParseTuple(args, (char*)"Oii", &pyPM, &eo, &wfc)) return NULL;

    PortableServer::POAManager_ptr pm =
      (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM, POAMANAGER_TWIN);

    OMNIORB_ASSERT(pm);

    try {
      pm->deactivate(eo, wfc);
    }
    catch (PortableServer::POAManager::AdapterInactive& ex) {
      PyObject* excc = PyObject_GetAttrString(pyPM, (char*)"AdapterInactive");
      OMNIORB_ASSERT(excc);
      PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
      PyErr_SetObject(excc, exci);
      return 0;
    }
    Py_INCREF(Py_None); return Py_None;
  }

  static PyObject* pyPM_get_state(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPM)) return NULL;

    PortableServer::POAManager_ptr pm =
      (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM, POAMANAGER_TWIN);

    OMNIORB_ASSERT(pm);

    PortableServer::POAManager::State s = pm->get_state();
    return PyInt_FromLong((int)s);
  }


  static PyObject* pyPM_releaseRef(PyObject* self, PyObject* args)
  {
    PyObject* pyPM;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPM)) return NULL;

    PortableServer::POAManager_ptr pm =
      (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM, POAMANAGER_TWIN);

    OMNIORB_ASSERT(pm);
    CORBA::release(pm);

    omniPy::remTwin(pyPM, POAMANAGER_TWIN);
    omniPy::remTwin(pyPM, OBJREF_TWIN);

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
