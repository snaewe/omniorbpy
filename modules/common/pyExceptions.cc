// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyExceptions.cc            Created on: 1999/07/29
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
//    Exception handling functions


// $Id$

// $Log$
// Revision 1.12  2001/02/21 14:21:47  dpg1
// Merge from omnipy1_develop for 1.3 release.
//
// Revision 1.9.2.2  2000/08/22 16:12:44  dpg1
// Properly clear Python error status after unexpected user exception
//
// Revision 1.9.2.1  2000/08/17 08:43:34  dpg1
// Fix possible obscure assertion failure with unexpected user exception
// containing a member named "minor".
//
// Revision 1.9  2000/06/12 15:36:08  dpg1
// Support for exception handler functions. Under omniORB 3, local
// operation dispatch modified so exceptions handlers are run.
//
// Revision 1.8  2000/05/11 11:58:24  dpg1
// Throw system exceptions with OMNIORB_THROW.
//
// Revision 1.7  2000/03/24 16:48:58  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.6  2000/03/03 17:41:43  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//
// Revision 1.5  2000/01/31 10:51:42  dpg1
// Fix to exception throwing.
//
// Revision 1.4  1999/12/15 12:17:20  dpg1
// Changes to compile with SunPro CC 5.0.
//
// Revision 1.3  1999/09/24 09:22:03  dpg1
// Added copyright notices.
//
// Revision 1.2  1999/09/20 14:54:19  dpg1
// GCC 2.95 is more pedantic than egcs.
//
// Revision 1.1  1999/07/29 14:18:58  dpg1
// Initial revision
//

#include <omnipy.h>


PyObject*
omniPy::handleSystemException(const CORBA::SystemException& ex)
{
#ifdef OMNIORBPY_FOR_28
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex.NP_RepositoryId());
#else
  int dummy;
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex._NP_repoId(&dummy));
#endif
  OMNIORB_ASSERT(excc);

  PyObject* exca = Py_BuildValue((char*)"(ii)", ex.minor(), ex.completed());
  PyObject* exci = PyEval_CallObject(excc, exca);
  Py_DECREF(exca);
  if (exci) {
    // If we couldn't create the exception object, there will be a
    // suitable error set already
    PyErr_SetObject(excc, exci);
    Py_DECREF(exci);
  }
  return 0;
}

PyObject*
omniPy::createPySystemException(const CORBA::SystemException& ex)
{
#ifdef OMNIORBPY_FOR_28
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex.NP_RepositoryId());
#else
  int dummy;
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex._NP_repoId(&dummy));
#endif
  OMNIORB_ASSERT(excc);

  PyObject* exca = Py_BuildValue((char*)"(ii)", ex.minor(), ex.completed());
  PyObject* exci = PyEval_CallObject(excc, exca);
  Py_DECREF(exca);

  return exci;
}


void
omniPy::produceSystemException(PyObject* eobj, PyObject* erepoId)
{
  CORBA::ULong            minor  = 0;
  CORBA::CompletionStatus status = CORBA::COMPLETED_MAYBE;

  PyObject *m = 0, *c = 0, *v = 0;

  m = PyObject_GetAttrString(eobj, (char*)"minor");
  if (m && PyInt_Check(m)) {
    minor = PyInt_AS_LONG(m);

    c = PyObject_GetAttrString(eobj, (char*)"completed");

    if (c) {
      v = PyObject_GetAttrString(c, (char*)"_v");

      if (v && PyInt_Check(v))
	status = (CORBA::CompletionStatus)PyInt_AS_LONG(v);
    }
  }
  Py_XDECREF(m); Py_XDECREF(c); Py_XDECREF(v);

  // Clear any errors raised by the GetAttrs
  if (PyErr_Occurred()) PyErr_Clear();

  char* repoId = PyString_AS_STRING(erepoId);

  Py_DECREF(eobj);

  if (!strcmp(repoId, "IDL:omg.org/CORBA/UNKNOWN:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(UNKNOWN, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_PARAM:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(BAD_PARAM, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_MEMORY:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(NO_MEMORY, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/IMP_LIMIT:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(IMP_LIMIT, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/COMM_FAILURE:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(COMM_FAILURE, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_OBJREF:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INV_OBJREF, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(OBJECT_NOT_EXIST, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_PERMISSION:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(NO_PERMISSION, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INTERNAL:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INTERNAL, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/MARSHAL:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(MARSHAL, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INITIALIZE:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INITIALIZE, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_IMPLEMENT:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(NO_IMPLEMENT, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_TYPECODE:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(BAD_TYPECODE, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_OPERATION:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(BAD_OPERATION, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_RESOURCES:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(NO_RESOURCES, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_RESPONSE:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(NO_RESPONSE, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/PERSIST_STORE:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(PERSIST_STORE, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_INV_ORDER:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(BAD_INV_ORDER, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSIENT:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(TRANSIENT, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/FREE_MEM:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(FREE_MEM, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_IDENT:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INV_IDENT, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_FLAG:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INV_FLAG, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INTF_REPOS:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INTF_REPOS, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_CONTEXT:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(BAD_CONTEXT, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/OBJ_ADAPTER:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(OBJ_ADAPTER, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/DATA_CONVERSION:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(DATA_CONVERSION, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSACTION_REQUIRED:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(TRANSACTION_REQUIRED, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(TRANSACTION_ROLLEDBACK, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INVALID_TRANSACTION:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(INVALID_TRANSACTION, minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/WRONG_TRANSACTION:1.0")) {
    Py_DECREF(erepoId); OMNIORB_THROW(WRONG_TRANSACTION, minor, status);
  }
  else {
    Py_DECREF(erepoId); OMNIORB_THROW(UNKNOWN, 0, CORBA::COMPLETED_NO);
  }
}
