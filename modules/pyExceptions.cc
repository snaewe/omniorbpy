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
// Revision 1.1.2.3  2001/05/10 15:16:02  dpg1
// Big update to support new omniORB 4 internals.
//
// Revision 1.1.2.2  2000/11/22 14:42:05  dpg1
// Remove dead omniORB 2.8 code.
//
// Revision 1.1.2.1  2000/10/13 13:55:24  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>
#include <pyThreadCache.h>


PyObject*
omniPy::handleSystemException(const CORBA::SystemException& ex)
{
  int dummy;
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex._NP_repoId(&dummy));
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
  int dummy;
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					(char*)ex._NP_repoId(&dummy));
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

#define THROW_SYSTEM_EXCEPTION_IF_MATCH(ex) \
  if (!strcmp(repoId, "IDL:omg.org/CORBA/" #ex ":1.0")) { \
    Py_DECREF(erepoId); OMNIORB_THROW(ex, minor, status); \
  }

  OMNIORB_FOR_EACH_SYS_EXCEPTION(THROW_SYSTEM_EXCEPTION_IF_MATCH)

#undef THROW_SYSTEM_EXCEPTION_IF_MATCH

  Py_DECREF(erepoId); OMNIORB_THROW(UNKNOWN, 0, CORBA::COMPLETED_MAYBE);
}


//
// Implementation of PyUserException
//

omniPy::
PyUserException::PyUserException(PyObject* desc)
  : desc_(desc), exc_(0)
{
  OMNIORB_ASSERT(desc_);
  Py_INCREF(desc_);
  pd_insertToAnyFn    = 0;
  pd_insertToAnyFnNCP = 0;
}

omniPy::
PyUserException::PyUserException(PyObject* desc, PyObject* exc,
				 CORBA::CompletionStatus comp_status)
  : desc_(desc), exc_(exc)
{
  OMNIORB_ASSERT(desc_);
  OMNIORB_ASSERT(exc_);

  try {
    omniPy::validateType(desc_, exc_, comp_status);
  }
  catch (...) {
    Py_DECREF(exc_);
    throw;
  }

  Py_INCREF(desc_);

  pd_insertToAnyFn    = 0;
  pd_insertToAnyFnNCP = 0;
}

omniPy::
PyUserException::PyUserException(const PyUserException& e)
  : desc_(e.desc_), exc_(e.exc_)
{
  if (desc_)  Py_INCREF(desc_);
  if (exc_)   Py_INCREF(exc_);
  pd_insertToAnyFn    = 0;
  pd_insertToAnyFnNCP = 0;
}

omniPy::
PyUserException::~PyUserException()
{
  Py_XDECREF(desc_);
  Py_XDECREF(exc_);
}

PyObject*
omniPy::
PyUserException::setPyExceptionState() const
{
  OMNIORB_ASSERT(desc_);
  OMNIORB_ASSERT(exc_);

  PyObject* excclass = PyTuple_GET_ITEM(desc_, 1);
  OMNIORB_ASSERT(PyClass_Check(excclass));
  
  PyErr_SetObject(excclass, exc_);
  return 0;
}


void
omniPy::
PyUserException::operator>>=(cdrStream& stream) const
{
  PyUnlockingCdrStream pystream(stream);

  PyObject* sdict = ((PyInstanceObject*)exc_)->in_dict;
  int       cnt   = (PyTuple_GET_SIZE(desc_) - 4) / 2;

  PyObject* name;
  PyObject* value;

  int i, j;
  for (i=0,j=4; i < cnt; i++) {
    name  = PyTuple_GET_ITEM(desc_, j++);
    value = PyDict_GetItem(sdict, name);
    omniPy::marshalPyObject(pystream, PyTuple_GET_ITEM(desc_, j++), value);
  }
}

void
omniPy::
PyUserException::operator<<=(cdrStream& stream)
{
  PyUnlockingCdrStream pystream(stream);

  PyObject* excclass = PyTuple_GET_ITEM(desc_, 1);
  OMNIORB_ASSERT(PyClass_Check(excclass));

  int       cnt      = (PyTuple_GET_SIZE(desc_) - 4) / 2;
  PyObject* exctuple = PyTuple_New(cnt);

  int i, j;
  try {
    for (i=0, j=5; i < cnt; i++, j+=2) {
      PyTuple_SET_ITEM(exctuple, i,
		       unmarshalPyObject(pystream,
					 PyTuple_GET_ITEM(desc_, j)));
    }
  }
  catch (...) {
    Py_DECREF(exctuple);
    throw;
  }
  exc_ = PyEval_CallObject(excclass, exctuple);
  Py_DECREF(exctuple);

  if (!exc_) {
    // Oh dear. Python exception constructor threw an exception.
    if (omniORB::trace(1)) {
      {
	omniORB::logger l;
	l <<
	  "Caught internal omniORB error trying to create an exception:\n";
      }
      PyErr_Print();
    }
    OMNIORB_THROW(INTERNAL, 0, CORBA::COMPLETED_MAYBE);
  }
}

void
omniPy::
PyUserException::_raise()
{
  OMNIORB_ASSERT(desc_);
  OMNIORB_ASSERT(exc_);
  throw *this;
}

const char*
omniPy::
PyUserException::_NP_repoId(int* size) const
{
  PyObject* pyrepoId = PyTuple_GET_ITEM(desc_, 2);
  OMNIORB_ASSERT(PyString_Check(pyrepoId));

  *size = PyString_GET_SIZE(pyrepoId) + 1;
  return PyString_AS_STRING(pyrepoId);
}

void
omniPy::
PyUserException::_NP_marshal(cdrStream& stream) const
{
  *this >>= stream;
}

CORBA::Exception*
omniPy::
PyUserException::_NP_duplicate() const
{
  return new PyUserException(*this);
}

const char*
omniPy::
PyUserException::_NP_typeId() const
{
  int cannot_downcast = 0;
  OMNIORB_ASSERT(cannot_downcast);
  return 0;
}
