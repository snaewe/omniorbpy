// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyORBFunc.cc               Created on: 2000/02/04
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
//    ORB functions

// $Id$

// $Log$
// Revision 1.1  2000/03/03 17:41:43  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//


#include <omnipy.h>


extern "C" {

  static PyObject*
  pyORB_string_to_object(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    char* s;

    if (!PyArg_ParseTuple(args, (char*)"Os", &pyorb, &s))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);

    OMNIORB_ASSERT(orb);

    if (!s || strlen(s) == 0) {
      PyErr_SetString(PyExc_TypeError, "Invalid IOR");
      return 0;
    }
    CORBA::Object_ptr objref;

    try {
      omniObject* oobj = omniPy::stringToObject(s);
      if (oobj)
	objref = (CORBA::Object_ptr)(oobj->_widenFromTheMostDerivedIntf(0));
      else
	objref = CORBA::Object::_nil();
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    return omniPy::createPyCorbaObjRef(0, objref);
  }

  static PyObject*
  pyORB_object_to_string(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyorb, &pyobjref))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);

    OMNIORB_ASSERT(orb);

    CORBA::Object_ptr objref;

    if (pyobjref == Py_None) {
      objref = CORBA::Object::_nil();
    }
    else {
      if (!PyInstance_Check(pyobjref)) {
	PyErr_SetString(PyExc_TypeError,
			(char*)"Argument must be an object reference.");
	return NULL;
      }	
      objref = (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);
    }
    RAISE_PY_BAD_PARAM_IF(!objref);

    CORBA::String_var str = orb->object_to_string(objref);

    return PyString_FromString((char*)str);
  }

  static PyObject*
  pyORB_list_initial_services(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;

    if (!PyArg_ParseTuple(args, (char*)"O", &pyorb))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);
    OMNIORB_ASSERT(orb);

    CORBA::ORB::ObjectIdList_var ids = orb->list_initial_services();

    PyObject* pyids = PyList_New(ids->length() + 1);

    CORBA::ULong i;
    for (i=0; i<ids->length(); i++) {
      PyList_SetItem(pyids, i, PyString_FromString(ids[i]));
    }
    PyList_SetItem(pyids, i, PyString_FromString((char*)"RootPOA"));
    return pyids;
  }

  static PyObject*
  pyORB_resolve_initial_references(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    char*     id;

    if (!PyArg_ParseTuple(args, (char*)"Os", &pyorb, &id))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);
    OMNIORB_ASSERT(orb);

    // Special case -- RootPOA doesn't really exist in omniORB 2.8
    if (!strcmp(id, "RootPOA"))
      return omniPy::fakePyRootPOAObject(pyorb, orb);

    CORBA::Object_ptr objref;
    try {
      objref = orb->resolve_initial_references(id);
    }
    catch (CORBA::ORB::InvalidName& ex) {
      PyObject* excc = PyObject_GetAttrString(pyorb, (char*)"InvalidName");
      OMNIORB_ASSERT(excc);
      PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
      PyErr_SetObject(excc, exci);
      return 0;
    }
    catch (const CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    return omniPy::createPyCorbaObjRef(0, objref);
  }

  ////////////////////////////////////////////////////////////////////////////
  // Python method table                                                    //
  ////////////////////////////////////////////////////////////////////////////

  static PyMethodDef pyORB_methods[] = {
    {(char*)"string_to_object", pyORB_string_to_object,          METH_VARARGS},
    {(char*)"object_to_string", pyORB_object_to_string,          METH_VARARGS},
    {(char*)"list_initial_services",
                                pyORB_list_initial_services,     METH_VARARGS},
    {(char*)"resolve_initial_references",
                                pyORB_resolve_initial_references,METH_VARARGS},
    {NULL,NULL}
  };
}

void
omniPy::initORBFunc(PyObject* d)
{
  PyObject* m = Py_InitModule((char*)"_omnipy.orb_func", pyORB_methods);
  PyDict_SetItemString(d, (char*)"orb_func", m);
}
