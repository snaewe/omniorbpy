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
// Revision 1.1.2.11  2003/09/25 13:17:16  dgrisby
// Report unexpected user exceptions.
//
// Revision 1.1.2.10  2003/08/28 12:13:57  dgrisby
// Properly report unknown user exception, rather than system exception.
//
// Revision 1.1.2.9  2003/05/28 10:13:01  dgrisby
// Preliminary interceptor support. General clean-up.
//
// Revision 1.1.2.8  2001/10/18 15:48:39  dpg1
// Track ORB core changes.
//
// Revision 1.1.2.7  2001/09/24 10:48:25  dpg1
// Meaningful minor codes.
//
// Revision 1.1.2.6  2001/08/01 10:12:36  dpg1
// Main thread policy.
//
// Revision 1.1.2.5  2001/05/29 17:10:14  dpg1
// Support for in process identity.
//
// Revision 1.1.2.4  2001/05/14 12:47:21  dpg1
// Fix memory leaks.
//
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

  if (omniORB::trace(1)) {
    omniORB::logger l;
    l << "Caught an unexpected CORBA exception during up-call: "
      << PyString_AS_STRING(erepoId) << "\n";
  }
  Py_DECREF(erepoId);
  if (m && c && v)
    OMNIORB_THROW(UNKNOWN, UNKNOWN_SystemException, CORBA::COMPLETED_MAYBE);
  else
    OMNIORB_THROW(UNKNOWN, UNKNOWN_UserException, CORBA::COMPLETED_MAYBE);
}


void
omniPy::handlePythonException()
{
  OMNIORB_ASSERT(PyErr_Occurred());

  PyObject *etype, *evalue, *etraceback;
  PyObject *erepoId = 0;
  PyErr_Fetch(&etype, &evalue, &etraceback);
  OMNIORB_ASSERT(etype);

  if (evalue && PyInstance_Check(evalue))
    erepoId = PyObject_GetAttrString(evalue, (char*)"_NP_RepositoryId");

  if (!(erepoId && PyString_Check(erepoId))) {
    Py_XDECREF(erepoId);
    if (omniORB::trace(1)) {
      {
	omniORB::logger l;
	l << "Caught an unexpected Python exception during up-call.\n";
      }
      PyErr_Restore(etype, evalue, etraceback);
      PyErr_Print();
    }
    OMNIORB_THROW(UNKNOWN, UNKNOWN_PythonException,
		  CORBA::COMPLETED_MAYBE);
  }

  Py_DECREF(etype);
  Py_XDECREF(etraceback);

  // Is it a LOCATION_FORWARD?
  if (omni::strMatch(PyString_AS_STRING(erepoId),
		     "omniORB.LOCATION_FORWARD")) {
    Py_DECREF(erepoId);
    omniPy::handleLocationForward(evalue);
  }

  // System exception
  omniPy::produceSystemException(evalue, erepoId);
}


void
omniPy::handleLocationForward(PyObject* evalue)
{
  PyObject* pyfwd  = PyObject_GetAttrString(evalue, (char*)"_forward");
  PyObject* pyperm = PyObject_GetAttrString(evalue, (char*)"_perm");
  OMNIORB_ASSERT(pyfwd);
  OMNIORB_ASSERT(pyperm);

  CORBA::Boolean perm;

  if (PyInt_Check(pyperm)) {
    perm = PyInt_AS_LONG(pyperm) ? 1 : 0;
  }
  else {
    omniORB::logs(1, "Bad 'permanent' field in LOCATION_FORWARD. "
		  "Using FALSE.");
    perm = 0;
  }
  CORBA::Object_ptr fwd =
    (CORBA::Object_ptr)omniPy::getTwin(pyfwd, OBJREF_TWIN);

  Py_DECREF(pyfwd);
  Py_DECREF(pyperm);
  Py_DECREF(evalue);
  if (fwd)
    throw omniORB::LOCATION_FORWARD(CORBA::Object::_duplicate(fwd), perm);
  else {
    omniORB::logs(1, "Invalid object reference inside "
		  "omniORB.LOCATION_FORWARD exception");
    OMNIORB_THROW(BAD_PARAM, BAD_PARAM_WrongPythonType, CORBA::COMPLETED_NO);
  }
}





//
// Implementation of PyUserException
//

omniPy::
PyUserException::PyUserException(PyObject* desc)
  : desc_(desc), exc_(0), decref_on_del_(0)
{
  OMNIORB_ASSERT(desc_);
  pd_insertToAnyFn    = 0;
  pd_insertToAnyFnNCP = 0;

  if (omniORB::trace(25)) {
    omniORB::logger l;
    const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
    l << "Prepare to unmarshal Python user exception " << repoId << "\n";
  }
}

omniPy::
PyUserException::PyUserException(PyObject* desc, PyObject* exc,
				 CORBA::CompletionStatus comp_status)
  : desc_(desc), exc_(exc), decref_on_del_(1)
{
  OMNIORB_ASSERT(desc_);
  OMNIORB_ASSERT(exc_);

  if (omniORB::trace(25)) {
    omniORB::logger l;
    const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
    l << "Construct Python user exception " << repoId << "\n";
  }

  try {
    omniPy::validateType(desc_, exc_, comp_status);
  }
  catch (...) {
    Py_DECREF(exc_);
    throw;
  }

  pd_insertToAnyFn    = 0;
  pd_insertToAnyFnNCP = 0;
}

omniPy::
PyUserException::PyUserException(const PyUserException& e)
  : desc_(e.desc_), exc_(e.exc_), decref_on_del_(1)
{
  pd_insertToAnyFn    = 0;
  pd_insertToAnyFnNCP = 0;

  // Oh dear. We need to mark the exception being copied to say that
  // the exception object should not be DECREF'd when it is deleted.
  ((PyUserException&)e).decref_on_del_ = 0;
}

omniPy::
PyUserException::~PyUserException()
{
  if (decref_on_del_) {
    if (omniORB::trace(25)) {
      omniORB::logger l;
      const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
      l << "Python user exception state " << repoId << " dropped unused\n";
    }
    omnipyThreadCache::lock _t;
    OMNIORB_ASSERT(exc_);
    Py_DECREF(exc_);
  }
}

PyObject*
omniPy::
PyUserException::setPyExceptionState()
{
  OMNIORB_ASSERT(desc_);
  OMNIORB_ASSERT(exc_);

  PyObject* excclass = PyTuple_GET_ITEM(desc_, 1);
  OMNIORB_ASSERT(PyClass_Check(excclass));
  
  if (omniORB::trace(25)) {
    omniORB::logger l;
    const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
    l << "Set Python user exception state " << repoId << "\n";
  }
  PyErr_SetObject(excclass, exc_);
  Py_DECREF(exc_);
  decref_on_del_ = 0;
  exc_ = 0;
  return 0;
}

void
omniPy::
PyUserException::decrefPyException()
{
  OMNIORB_ASSERT(exc_);
  Py_DECREF(exc_);
  decref_on_del_ = 0;
  exc_ = 0;
}


void
omniPy::
PyUserException::operator>>=(cdrStream& stream) const
{
  OMNIORB_ASSERT(exc_);

  if (omniORB::trace(25)) {
    omniORB::logger l;
    const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
    l << "Marshal Python user exception " << repoId << "\n";
  }

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
  if (omniORB::trace(25)) {
    omniORB::logger l;
    const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
    l << "Unmarshal Python user exception " << repoId << "\n";
  }

  PyUnlockingCdrStream pystream(stream);

  PyObject* excclass = PyTuple_GET_ITEM(desc_, 1);
  OMNIORB_ASSERT(PyClass_Check(excclass));

  int       cnt      = (PyTuple_GET_SIZE(desc_) - 4) / 2;
  PyObject* exctuple = PyTuple_New(cnt);
  omniPy::PyRefHolder exctuple_holder(exctuple);

  int i, j;
  for (i=0, j=5; i < cnt; i++, j+=2) {
    PyTuple_SET_ITEM(exctuple, i,
		     unmarshalPyObject(pystream,
				       PyTuple_GET_ITEM(desc_, j)));
  }
  exc_ = PyEval_CallObject(excclass, exctuple);

  if (!exc_) {
    // Oh dear. Python exception constructor threw an exception.
    if (omniORB::trace(25)) {
      {
	omniORB::logger l;
	l << "Caught unexpected error trying to create an exception:\n";
      }
      PyErr_Print();
    }
    OMNIORB_THROW(INTERNAL, 0, CORBA::COMPLETED_MAYBE);
  }
}

void
omniPy::
PyUserException::_raise() const
{
  OMNIORB_ASSERT(desc_);
  OMNIORB_ASSERT(exc_);

  if (omniORB::trace(25)) {
    omniORB::logger l;
    const char* repoId = PyString_AS_STRING(PyTuple_GET_ITEM(desc_, 2));
    l << "C++ throw of Python user exception " << repoId << "\n";
  }
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
  omnipyThreadCache::lock _t;
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
