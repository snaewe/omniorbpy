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


void
omniPy::handleSystemException(const CORBA::SystemException& ex)
{
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex.NP_RepositoryId());

  PyObject* exca = Py_BuildValue((char*)"(ii)", ex.minor(), ex.completed());
  PyObject* exci = PyEval_CallObject(excc, exca);
  Py_DECREF(exca);
  PyErr_SetObject(excc, exci);
}



void
omniPy::produceSystemException(PyObject* eobj, PyObject* erepoId)
{
  CORBA::ULong            minor;
  CORBA::CompletionStatus status;

  PyObject *a, *b;

  a = PyObject_GetAttrString(eobj, (char*)"minor");
  if (a && PyInt_Check(a)) {
    minor = PyInt_AS_LONG(a);
    Py_DECREF(a);

    a = PyObject_GetAttrString(eobj, (char*)"completed"); assert(a);
    b = PyObject_GetAttrString(a,    (char*)"_v"); assert(b && PyInt_Check(b));
    status = (CORBA::CompletionStatus)PyInt_AS_LONG(b);
    Py_DECREF(a); Py_DECREF(b);
  }
  else {
    minor  = 0;
    status = CORBA::COMPLETED_MAYBE;
  }

  char* repoId = PyString_AS_STRING(erepoId);

  Py_DECREF(eobj);

  if (!strcmp(repoId, "IDL:omg.org/CORBA/UNKNOWN")) {
    Py_DECREF(erepoId); throw CORBA::UNKNOWN(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_PARAM")) {
    Py_DECREF(erepoId); throw CORBA::BAD_PARAM(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_MEMORY")) {
    Py_DECREF(erepoId); throw CORBA::NO_MEMORY(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/IMP_LIMIT")) {
    Py_DECREF(erepoId); throw CORBA::IMP_LIMIT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/COMM_FAILURE")) {
    Py_DECREF(erepoId); throw CORBA::COMM_FAILURE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_OBJREF")) {
    Py_DECREF(erepoId); throw CORBA::INV_OBJREF(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/OBJECT_NOT_EXIST")) {
    Py_DECREF(erepoId); throw CORBA::OBJECT_NOT_EXIST(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_PERMISSION")) {
    Py_DECREF(erepoId); throw CORBA::NO_PERMISSION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INTERNAL")) {
    Py_DECREF(erepoId); throw CORBA::INTERNAL(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/MARSHAL")) {
    Py_DECREF(erepoId); throw CORBA::MARSHAL(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INITIALIZE")) {
    Py_DECREF(erepoId); throw CORBA::INITIALIZE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_IMPLEMENT")) {
    Py_DECREF(erepoId); throw CORBA::NO_IMPLEMENT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_TYPECODE")) {
    Py_DECREF(erepoId); throw CORBA::BAD_TYPECODE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_OPERATION")) {
    Py_DECREF(erepoId); throw CORBA::BAD_OPERATION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_RESOURCES")) {
    Py_DECREF(erepoId); throw CORBA::NO_RESOURCES(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_RESPONSE")) {
    Py_DECREF(erepoId); throw CORBA::NO_RESPONSE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/PERSIST_STORE")) {
    Py_DECREF(erepoId); throw CORBA::PERSIST_STORE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_INV_ORDER")) {
    Py_DECREF(erepoId); throw CORBA::BAD_INV_ORDER(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSIENT")) {
    Py_DECREF(erepoId); throw CORBA::TRANSIENT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/FREE_MEM")) {
    Py_DECREF(erepoId); throw CORBA::FREE_MEM(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_IDENT")) {
    Py_DECREF(erepoId); throw CORBA::INV_IDENT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_FLAG")) {
    Py_DECREF(erepoId); throw CORBA::INV_FLAG(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INTF_REPOS")) {
    Py_DECREF(erepoId); throw CORBA::INTF_REPOS(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_CONTEXT")) {
    Py_DECREF(erepoId); throw CORBA::BAD_CONTEXT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/OBJ_ADAPTER")) {
    Py_DECREF(erepoId); throw CORBA::OBJ_ADAPTER(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/DATA_CONVERSION")) {
    Py_DECREF(erepoId); throw CORBA::DATA_CONVERSION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSACTION_REQUIRED")) {
    Py_DECREF(erepoId); throw CORBA::TRANSACTION_REQUIRED(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK")) {
    Py_DECREF(erepoId); throw CORBA::TRANSACTION_ROLLEDBACK(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INVALID_TRANSACTION")) {
    Py_DECREF(erepoId); throw CORBA::INVALID_TRANSACTION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/WRONG_TRANSACTION")) {
    Py_DECREF(erepoId); throw CORBA::WRONG_TRANSACTION(minor, status);
  }
  else {
    Py_DECREF(erepoId); throw CORBA::UNKNOWN(0, CORBA::COMPLETED_NO);
  }
}
