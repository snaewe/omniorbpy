// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyServant.cc               Created on: 1999/07/29
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
//    Implementation of Python servant object

// $Id$

// $Log$
// Revision 1.1.2.8  2001/06/15 10:59:26  dpg1
// Apply fixes from omnipy1_develop.
//
// Revision 1.1.2.7  2001/06/01 11:09:26  dpg1
// Make use of new omni::ptrStrCmp() and omni::strCmp().
//
// Revision 1.1.2.6  2001/05/29 17:10:14  dpg1
// Support for in process identity.
//
// Revision 1.1.2.5  2001/05/14 12:47:22  dpg1
// Fix memory leaks.
//
// Revision 1.1.2.4  2001/05/10 15:16:03  dpg1
// Big update to support new omniORB 4 internals.
//
// Revision 1.1.2.3  2001/03/13 10:38:08  dpg1
// Fixes from omnipy1_develop
//
// Revision 1.1.2.2  2000/12/04 18:57:24  dpg1
// Fix deadlock when trying to lock omniORB internal lock while holding
// the Python interpreter lock.
//
// Revision 1.1.2.1  2000/10/13 13:55:27  dpg1
// Initial support for omniORB 4.
//


#include <omnipy.h>
#include <pyThreadCache.h>

#include <omniORB4/callHandle.h>
#include <omniORB4/IOP_S.h>


// Implementation classes for ServantManagers and AdapterActivator

class Py_ServantActivator: public virtual POA_PortableServer::ServantActivator,
			   public virtual omniPy::Py_omniServant
{
public:
  Py_ServantActivator(PyObject* pysa, PyObject* opdict, const char* repoId);
  virtual ~Py_ServantActivator();

  PortableServer::Servant incarnate(const PortableServer::ObjectId& oid,
				    PortableServer::POA_ptr         poa);

  void etherealize(const PortableServer::ObjectId& oid,
		   PortableServer::POA_ptr         poa,
		   PortableServer::Servant         serv,
		   CORBA::Boolean                  cleanup_in_progress,
		   CORBA::Boolean                  remaining_activations);

  void*                   _ptrToInterface(const char* repoId);

  CORBA::Boolean          _is_a(const char* logical_type_id) {
    return PY_OMNISERVANT_BASE::_is_a(logical_type_id);
  }
  PortableServer::POA_ptr _default_POA() {
    return PY_OMNISERVANT_BASE::_default_POA();
  }
  const char* _mostDerivedRepoId() {
    return PY_OMNISERVANT_BASE::_mostDerivedRepoId();
  }
  CORBA::Boolean _dispatch(omniCallHandle& handle) {
    return PY_OMNISERVANT_BASE::_dispatch(handle);
  }

private:
  PyObject* pysa_;

  // Not implemented
  Py_ServantActivator(const Py_ServantActivator&);
  Py_ServantActivator& operator=(const Py_ServantActivator&);
};

class Py_ServantLocator: public virtual POA_PortableServer::ServantLocator,
			 public virtual omniPy::Py_omniServant
{
public:
  Py_ServantLocator(PyObject* pysl, PyObject* opdict, const char* repoId);
  virtual ~Py_ServantLocator();

  PortableServer::Servant preinvoke(const PortableServer::ObjectId& oid,
				    PortableServer::POA_ptr         poa,
				    const char*                     operation,
				    void*&                          cookie);

  void postinvoke(const PortableServer::ObjectId& oid,
		  PortableServer::POA_ptr         poa,
		  const char*                     operation,
		  void*                           cookie,
		  PortableServer::Servant         serv);

  void*                   _ptrToInterface(const char* repoId);

  CORBA::Boolean          _is_a(const char* logical_type_id) {
    return PY_OMNISERVANT_BASE::_is_a(logical_type_id);
  }
  PortableServer::POA_ptr _default_POA() {
    return PY_OMNISERVANT_BASE::_default_POA();
  }
  const char* _mostDerivedRepoId() {
    return PY_OMNISERVANT_BASE::_mostDerivedRepoId();
  }
  CORBA::Boolean _dispatch(omniCallHandle& handle) {
    return PY_OMNISERVANT_BASE::_dispatch(handle);
  }

private:
  PyObject* pysl_;

  // Not implemented
  Py_ServantLocator(const Py_ServantLocator&);
  Py_ServantLocator& operator=(const Py_ServantLocator&);
};


class Py_AdapterActivator: public virtual POA_PortableServer::AdapterActivator,
			   public virtual omniPy::Py_omniServant
{
public:
  Py_AdapterActivator(PyObject* pyaa, PyObject* opdict, const char* repoId);
  virtual ~Py_AdapterActivator();

  CORBA::Boolean unknown_adapter(PortableServer::POA_ptr parent,
				 const char*             name);

  void*                   _ptrToInterface(const char* repoId);

  CORBA::Boolean          _is_a(const char* logical_type_id) {
    return PY_OMNISERVANT_BASE::_is_a(logical_type_id);
  }
  PortableServer::POA_ptr _default_POA() {
    return PY_OMNISERVANT_BASE::_default_POA();
  }
  const char* _mostDerivedRepoId() {
    return PY_OMNISERVANT_BASE::_mostDerivedRepoId();
  }
  CORBA::Boolean _dispatch(omniCallHandle& handle) {
    return PY_OMNISERVANT_BASE::_dispatch(handle);
  }

private:
  PyObject* pyaa_;

  // Not implemented
  Py_AdapterActivator(const Py_AdapterActivator&);
  Py_AdapterActivator& operator=(const Py_AdapterActivator&);
};



// Implementation of Py_omniServant

omniPy::
Py_omniServant::Py_omniServant(PyObject* pyservant, PyObject* opdict,
			       const char* repoId)
  : pyservant_(pyservant), opdict_(opdict), refcount_(1)
{
  repoId_ = CORBA::string_dup(repoId);

  OMNIORB_ASSERT(PyInstance_Check(pyservant));
  OMNIORB_ASSERT(PyDict_Check(opdict));
  Py_INCREF(pyservant_);
  Py_INCREF(opdict_);

  pyskeleton_ = PyObject_GetAttrString(pyservant_, (char*)"_omni_skeleton");
  OMNIORB_ASSERT(pyskeleton_ && PyClass_Check(pyskeleton_));

  omniPy::setTwin(pyservant, (omniPy::Py_omniServant*)this, SERVANT_TWIN);
}

omniPy::
Py_omniServant::~Py_omniServant()
{
  omniPy::remTwin(pyservant_, SERVANT_TWIN);
  Py_DECREF(pyservant_);
  Py_DECREF(opdict_);
  Py_DECREF(pyskeleton_);
  CORBA::string_free(repoId_);
}


void
omniPy::
Py_omniServant::_add_ref()
{
  omnipyThreadCache::lock _t;
  OMNIORB_ASSERT(refcount_ > 0);
  ++refcount_;
}

void
omniPy::
Py_omniServant::_locked_add_ref()
{
  OMNIORB_ASSERT(refcount_ > 0);
  ++refcount_;
}

void
omniPy::
Py_omniServant::_remove_ref()
{
  omnipyThreadCache::lock _t;
  if (--refcount_ > 0) return;

  OMNIORB_ASSERT(refcount_ == 0);
  delete this;
}

void
omniPy::
Py_omniServant::_locked_remove_ref()
{
  if (--refcount_ > 0) return;

  OMNIORB_ASSERT(refcount_ == 0);
  delete this;
}


void*
omniPy::
Py_omniServant::_ptrToInterface(const char* repoId)
{
  OMNIORB_ASSERT(repoId);

  if (omni::ptrStrMatch(repoId, omniPy::string_Py_omniServant))
    return (Py_omniServant*)this;

  if (omni::ptrStrMatch(repoId, CORBA::Object::_PD_repoId))
    return (void*)1;

  return 0;
}


const char*
omniPy::
Py_omniServant::_mostDerivedRepoId()
{
  return repoId_;
}


PortableServer::POA_ptr
omniPy::
Py_omniServant::_default_POA()
{
  {
    omnipyThreadCache::lock _t;
    PyObject* pyPOA = PyObject_CallMethod(pyservant_,
					  (char*)"_default_POA", 0);
    if (pyPOA) {
      PortableServer::POA_ptr poa =
	(PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);

      Py_DECREF(pyPOA);
      if (poa) {
	return PortableServer::POA::_duplicate(poa);
      }
      else {
	if (omniORB::trace(1)) {
	  omniORB::logger l;
	  l <<
	    "Python servant returned an invalid object from `_default_POA'.\n"
	    "Returning Root POA\n";
	}
      }      
    }
    else {
      if (omniORB::trace(1)) {
	omniORB::logger l;
	l <<
	  "Exception while trying to call _default_POA(). "
	  "Returning Root POA\n";
	PyErr_Print();
      }
      else
	PyErr_Clear();
    }
  }
  CORBA::Object_var obj = omniPy::orb->resolve_initial_references("RootPOA");
  return PortableServer::POA::_narrow(obj);
}


PyObject*
omniPy::
Py_omniServant::py_this()
{
  CORBA::Object_ptr lobjref;
  {
    omniPy::InterpreterUnlocker _u;
    {
      CORBA::Object_var objref;
      objref  = (CORBA::Object_ptr)_do_this(CORBA::Object::_PD_repoId);
      lobjref = omniPy::makeLocalObjRef(repoId_, objref);
    }
  }
  return omniPy::createPyCorbaObjRef(repoId_, lobjref);
}


static void
HandleLocationForward(PyObject* evalue)
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
    OMNIORB_THROW(BAD_PARAM,0,CORBA::COMPLETED_NO);
  }
}


CORBA::Boolean
omniPy::
Py_omniServant::_is_a(const char* logical_type_id)
{
  if (omni::ptrStrMatch(logical_type_id, repoId_))
    return 1;
  else if (omni::ptrStrMatch(logical_type_id, CORBA::Object::_PD_repoId))
    return 1;
  else {
    omnipyThreadCache::lock _t;
    PyObject* pyisa = PyObject_CallMethod(omniPy::pyomniORBmodule,
					  (char*)"static_is_a", (char*)"Os",
					  pyskeleton_, logical_type_id);
    if (!pyisa) PyErr_Print();
    OMNIORB_ASSERT(pyisa && PyInt_Check(pyisa));

    CORBA::Boolean isa = PyInt_AS_LONG(pyisa);
    Py_DECREF(pyisa);

    if (isa)
      return 1;

    // Last resort -- does the servant have an _is_a method?
    if (PyObject_HasAttrString(pyservant_, (char*)"_is_a")) {

      pyisa = PyObject_CallMethod(pyservant_, (char*)"_is_a",
				  (char*)"s", logical_type_id);

      if (pyisa && PyInt_Check(pyisa)) {
	CORBA::Boolean isa = PyInt_AS_LONG(pyisa);
	Py_DECREF(pyisa);
	return isa;
      }
      if (!pyisa) {
	// Some sort of exception
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
	  OMNIORB_THROW(UNKNOWN, 0,CORBA::COMPLETED_MAYBE);
	}

	Py_DECREF(etype);
	Py_XDECREF(etraceback);

	// Is it a LOCATION_FORWARD?
	if (omni::strMatch(PyString_AS_STRING(erepoId),
			   "omniORB.LOCATION_FORWARD")) {
	  Py_DECREF(erepoId);
	  HandleLocationForward(evalue);
	}

	// System exception
	omniPy::produceSystemException(evalue, erepoId);
      }
    }
  }
  return 0;
}


CORBA::Boolean
omniPy::
Py_omniServant::_dispatch(omniCallHandle& handle)
{
  int i;
  omnipyThreadCache::lock _t;

  const char* op   = handle.operation_name();
  PyObject*   desc = PyDict_GetItemString(opdict_, (char*)op);

  if (!desc) return 0; // Unknown operation name

  OMNIORB_ASSERT(PyTuple_Check(desc));

  PyObject* in_d  = PyTuple_GET_ITEM(desc,0);
  PyObject* out_d = PyTuple_GET_ITEM(desc,1);
  PyObject* exc_d = PyTuple_GET_ITEM(desc,2);

  Py_omniCallDescriptor call_desc(op, 0,
				  (out_d == Py_None),
				  in_d, out_d, exc_d, 0, 1);
  try {
    omniPy::InterpreterUnlocker _u;
    handle.upcall(this, call_desc);
  }
  catch (omniPy::PyUserException& ex) {
    if (handle.iop_s()) {
      {
	omniPy::InterpreterUnlocker _u;
	handle.iop_s()->SendException(&ex);
      }
      ex.decrefPyException();
    }
    else
      throw;
  }
  return 1;
}

void
omniPy::
Py_omniServant::remote_dispatch(Py_omniCallDescriptor* pycd)
{
  const char* op     = pycd->op();
  PyObject*   method = PyObject_GetAttrString(pyservant_, (char*)op);

  if (!method) {
    if (omniORB::trace(1)) {
      omniORB::logger l;
      l << "Python servant for `" << repoId_ << "' has no method named `"
	<< op << "'.\n";
    }
    PyErr_Clear();
    OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  }

  PyObject* args   = pycd->args();
  PyObject* result = PyEval_CallObject(method, args);
  Py_DECREF(method);
  Py_DECREF(args);

  if (result) {
    // No exception was thrown. Set the return value
    pycd->setAndValidateReturnedValues(result);
  }
  else {
    // An exception of some sort was thrown
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
      OMNIORB_THROW(UNKNOWN, 0,CORBA::COMPLETED_MAYBE);
    }
    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    PyObject* exc_d = pycd->exc_d_;

    // Is it a user exception?
    if (exc_d != Py_None) {
      OMNIORB_ASSERT(PyDict_Check(exc_d));

      PyObject* edesc = PyDict_GetItem(exc_d, erepoId);
      Py_DECREF(erepoId);

      if (edesc) {
	PyUserException ex(edesc, evalue, CORBA::COMPLETED_MAYBE);
	ex._raise();
      }
    }

    // Is it a LOCATION_FORWARD?
    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       "omniORB.LOCATION_FORWARD")) {
      Py_DECREF(erepoId);
      HandleLocationForward(evalue);
    }

    // System exception or unknown user exception
    omniPy::produceSystemException(evalue, erepoId);
  }
}


void
omniPy::
Py_omniServant::local_dispatch(Py_omniCallDescriptor* pycd)
{
  const char* op     = pycd->op();
  PyObject*   method = PyObject_GetAttrString(pyservant_, (char*)op);

  if (!method) {
    if (omniORB::trace(1)) {
      omniORB::logger l;
      l << "Python servant for `" << repoId_ << "' has no method named `"
	<< op << "'.\n";
    }
    PyErr_Clear();
    OMNIORB_THROW(NO_IMPLEMENT, 0,CORBA::COMPLETED_NO);
  }

  PyObject* in_d  = pycd->in_d_;
  int       in_l  = pycd->in_l_;
  PyObject* out_d = pycd->out_d_;
  int       out_l = pycd->out_l_;
  PyObject* exc_d = pycd->exc_d_;

  PyObject* args  = pycd->args();

  // Copy args which would otherwise have reference semantics
  PyObject* argtuple = PyTuple_New(in_l);
  PyObject* t_o;

  int i;

  try {
    for (i=0; i < in_l; ++i) {
      t_o = copyArgument(PyTuple_GET_ITEM(in_d, i),
			 PyTuple_GET_ITEM(args, i),
			 CORBA::COMPLETED_NO);
      OMNIORB_ASSERT(t_o);
      PyTuple_SET_ITEM(argtuple, i, t_o);
    }
  }
  catch (...) {
    Py_DECREF(args);
    Py_DECREF(argtuple);
    Py_DECREF(method);
    throw;
  }

  Py_DECREF(args);

  //
  // Do the call

  PyObject* result = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  if (result) {
    PyObject* retval = 0;

    try {
      if (out_l == -1 || out_l == 0) {
	if (result == Py_None) {
	  pycd->setReturnedValues(result);
	  return;
	}
	else
	  OMNIORB_THROW(BAD_PARAM,0,CORBA::COMPLETED_MAYBE);
      }
      else if (out_l == 1) {
	retval = copyArgument(PyTuple_GET_ITEM(out_d, 0),
			      result, CORBA::COMPLETED_MAYBE);
      }
      else {
	retval = PyTuple_New(out_l);
	
	for (i=0; i < out_l; ++i) {
	  t_o = copyArgument(PyTuple_GET_ITEM(out_d, i),
			     PyTuple_GET_ITEM(result, i),
			     CORBA::COMPLETED_MAYBE);

	  PyTuple_SET_ITEM(retval, i, t_o);
	}
      }
    }
    catch (...) {
      Py_DECREF(result);
      Py_XDECREF(retval);
      throw;
    }
    Py_DECREF(result);
    pycd->setReturnedValues(retval);
  }
  else {
    // The call raised a Python exception
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
      OMNIORB_THROW(UNKNOWN, 0, CORBA::COMPLETED_MAYBE);
    }
    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    // Is it a user exception?
    if (exc_d != Py_None) {
      OMNIORB_ASSERT(PyDict_Check(exc_d));

      PyObject* edesc = PyDict_GetItem(exc_d, erepoId);
      Py_DECREF(erepoId);

      if (edesc) {
	PyUserException ex(edesc, evalue, CORBA::COMPLETED_MAYBE);
	ex._raise();
      }
    }

    // Is it a LOCATION_FORWARD?
    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       "omniORB.LOCATION_FORWARD")) {
      Py_DECREF(erepoId);
      HandleLocationForward(evalue);
    }

    // System exception or unknown user exception
    omniPy::produceSystemException(evalue, erepoId);
  }
}


// Implementation of Py_ServantActivator

Py_ServantActivator::Py_ServantActivator(PyObject*   pysa,
					 PyObject*   opdict,
					 const char* repoId)
  : PY_OMNISERVANT_BASE(pysa, opdict, repoId), pysa_(pysa)
{
  Py_INCREF(pysa);
}

Py_ServantActivator::~Py_ServantActivator()
{
  Py_DECREF(pysa_);
}

PortableServer::Servant
Py_ServantActivator::incarnate(const PortableServer::ObjectId& oid,
			       PortableServer::POA_ptr         poa)
{
  PyObject *method, *argtuple, *pyservant;
  omnipyThreadCache::lock _t;

  method = PyObject_GetAttrString(pysa_, (char*)"incarnate");
  if (!method) {
    PyErr_Clear();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_MAYBE);
    // *** Good choice of exn?
  }
  PortableServer::POA::_duplicate(poa);
  argtuple = Py_BuildValue((char*)"s#N",
			   (const char*)oid.NP_data(), oid.length(),
			   omniPy::createPyPOAObject(poa));

  // Do the up-call
  pyservant = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  if (pyservant) {
    omniPy::Py_omniServant* servant = omniPy::getServantForPyObject(pyservant);
    Py_DECREF(pyservant);

    if (servant) return servant;
    else         OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_NO);
  }
  else {
    // An exception of some sort was thrown
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
      OMNIORB_THROW(UNKNOWN, 0,CORBA::COMPLETED_MAYBE);
    }
    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       PortableServer::ForwardRequest::_PD_repoId)) {
      Py_DECREF(erepoId);
      PyObject* pyfr = PyObject_GetAttrString(evalue,
					      (char*)"forward_reference");
      Py_DECREF(evalue);
      if (pyfr) {
	CORBA::Object_ptr fr = (CORBA::Object_ptr)omniPy::getTwin(pyfr,
								  OBJREF_TWIN);
	if (fr) {
	  PortableServer::ForwardRequest ex(fr);
	  Py_DECREF(pyfr);
	  throw ex;
	}
      }
      else {
	PyErr_Clear();
	OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_NO);
      }
    }

    // Is it a LOCATION_FORWARD?
    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       "omniORB.LOCATION_FORWARD")) {
      Py_DECREF(erepoId);
      HandleLocationForward(evalue);
    }

    // System exception or unknown user exception
    omniPy::produceSystemException(evalue, erepoId);
  }
  OMNIORB_ASSERT(0); // Never reach here
  return 0;
}


void
Py_ServantActivator::etherealize(const PortableServer::ObjectId& oid,
				 PortableServer::POA_ptr poa,
				 PortableServer::Servant serv,
				 CORBA::Boolean          cleanup_in_progress,
				 CORBA::Boolean          remaining_activations)
{
  PyObject *method, *argtuple, *result;
  omnipyThreadCache::lock _t;

  omniPy::Py_omniServant* pyos;
  pyos = (omniPy::Py_omniServant*)serv->
                                _ptrToInterface(omniPy::string_Py_omniServant);
  if (!pyos) {
    omniPy::InterpreterUnlocker _u;
    serv->_remove_ref();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_NO);
  }

  method = PyObject_GetAttrString(pysa_, (char*)"etherealize");
  if (!method) {
    PyErr_Clear();
    omniPy::InterpreterUnlocker _u;
    serv->_remove_ref();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_NO);
  }
  PortableServer::POA::_duplicate(poa);
  argtuple = Py_BuildValue((char*)"s#NNii",
			   (const char*)oid.NP_data(), oid.length(),
			   omniPy::createPyPOAObject(poa),
			   pyos->pyServant(),
			   (int)cleanup_in_progress,
			   (int)remaining_activations);
  // Do the up-call
  result = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  pyos->_locked_remove_ref();

  if (result)
    Py_DECREF(result);
  else {
    if (omniORB::trace(5))
      omniORB::logf("omniORBpy: Servant etherealization raised an exception!");
    if (omniORB::trace(10)) {
      omniORB::logf("omniORBpy: Traceback follows:");
      PyErr_Print();
    }
    else
      PyErr_Clear();
  }
}

void*
Py_ServantActivator::_ptrToInterface(const char* repoId)
{
  if (omni::ptrStrMatch(repoId, PortableServer::ServantActivator::_PD_repoId))
    return (PortableServer::_impl_ServantActivator*)this;
  if (omni::ptrStrMatch(repoId, omniPy::string_Py_omniServant))
    return (omniPy::Py_omniServant*)this;
  if (omni::ptrStrMatch(repoId, PortableServer::ServantManager::_PD_repoId))
    return (PortableServer::_impl_ServantManager*)this;
  if (omni::ptrStrMatch(repoId, CORBA::Object::_PD_repoId))
    return (void*)1;

  return 0;
}


// Implementation of Py_ServantLocator

Py_ServantLocator::Py_ServantLocator(PyObject*   pysl,
				     PyObject*   opdict,
				     const char* repoId)
  : PY_OMNISERVANT_BASE(pysl, opdict, repoId), pysl_(pysl)
{
  Py_INCREF(pysl);
}

Py_ServantLocator::~Py_ServantLocator()
{
  Py_DECREF(pysl_);
}

PortableServer::Servant
Py_ServantLocator::preinvoke(const PortableServer::ObjectId& oid,
			     PortableServer::POA_ptr         poa,
			     const char*                     operation,
			     void*&                          cookie)
{
  PyObject *method, *argtuple, *rettuple, *pyservant, *pycookie;
  omnipyThreadCache::lock _t;

  method = PyObject_GetAttrString(pysl_, (char*)"preinvoke");
  if (!method) {
    PyErr_Clear();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_NO);
    // *** Good choice of exn?
  }
  PortableServer::POA::_duplicate(poa);
  argtuple = Py_BuildValue((char*)"s#Ns",
			   (const char*)oid.NP_data(), oid.length(),
			   omniPy::createPyPOAObject(poa),
			   operation);

  // Do the up-call
  rettuple = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  if (rettuple) {
    if (PyTuple_Size(rettuple) != 2) {
      Py_DECREF(rettuple);
      OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_NO);
    }
    pyservant = PyTuple_GET_ITEM(rettuple, 0);
    pycookie  = PyTuple_GET_ITEM(rettuple, 1);

    omniPy::Py_omniServant* servant = omniPy::getServantForPyObject(pyservant);

    if (servant) {
      Py_INCREF(pycookie);
      cookie = pycookie;
      Py_DECREF(rettuple);
      return servant;
    }
    else {
      Py_DECREF(rettuple);
      OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_NO);
    }
  }
  else {
    // An exception of some sort was thrown
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
      OMNIORB_THROW(UNKNOWN, 0,CORBA::COMPLETED_MAYBE);
    }
    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       PortableServer::ForwardRequest::_PD_repoId)) {
      Py_DECREF(erepoId);
      PyObject* pyfr = PyObject_GetAttrString(evalue,
					      (char*)"forward_reference");
      Py_DECREF(evalue);
      if (pyfr) {
	CORBA::Object_ptr fr = (CORBA::Object_ptr)omniPy::getTwin(pyfr,
								  OBJREF_TWIN);
	if (fr) {
	  PortableServer::ForwardRequest ex(fr);
	  Py_DECREF(pyfr);
	  throw ex;
	}
      }
      else {
	PyErr_Clear();
	OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_NO);
      }
    }
    // Is it a LOCATION_FORWARD?
    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       "omniORB.LOCATION_FORWARD")) {
      Py_DECREF(erepoId);
      HandleLocationForward(evalue);
    }

    // System exception or unknown user exception
    omniPy::produceSystemException(evalue, erepoId);
  }
  OMNIORB_ASSERT(0); // Never reach here
  return 0;
}


void
Py_ServantLocator::postinvoke(const PortableServer::ObjectId& oid,
			      PortableServer::POA_ptr         poa,
			      const char*                     operation,
			      void*                           cookie,
			      PortableServer::Servant         serv)
{
  PyObject *method, *argtuple, *result;
  omnipyThreadCache::lock _t;

  omniPy::Py_omniServant* pyos;
  pyos = (omniPy::Py_omniServant*)serv->
                                _ptrToInterface(omniPy::string_Py_omniServant);
  if (!pyos) {
    omniPy::InterpreterUnlocker _u;
    serv->_remove_ref();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_NO);
  }

  method = PyObject_GetAttrString(pysl_, (char*)"postinvoke");
  if (!method) {
    PyErr_Clear();
    omniPy::InterpreterUnlocker _u;
    serv->_remove_ref();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_NO);
  }
  PortableServer::POA::_duplicate(poa);
  argtuple = Py_BuildValue((char*)"s#NsNN",
			   (const char*)oid.NP_data(), oid.length(),
			   omniPy::createPyPOAObject(poa),
			   operation,
			   (PyObject*)cookie,
			   pyos->pyServant());
  // Do the up-call
  result = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  pyos->_locked_remove_ref();

  if (result) {
    Py_DECREF(result);
    return;
  }
  else {
    // An exception of some sort was thrown
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
	  l << "Caught an unexpected Python exception during postinvoke.\n";
	}
	PyErr_Restore(etype, evalue, etraceback);
	PyErr_Print();
      }
      OMNIORB_THROW(UNKNOWN, 0,CORBA::COMPLETED_MAYBE);
    }
    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    // Is it a LOCATION_FORWARD?
    if (omni::strMatch(PyString_AS_STRING(erepoId),
		       "omniORB.LOCATION_FORWARD")) {
      Py_DECREF(erepoId);
      HandleLocationForward(evalue);
    }

    // System exception or unknown user exception
    omniPy::produceSystemException(evalue, erepoId);
  }
}

void*
Py_ServantLocator::_ptrToInterface(const char* repoId)
{
  if (omni::ptrStrMatch(repoId, PortableServer::ServantLocator::_PD_repoId))
    return (PortableServer::_impl_ServantLocator*)this;
  if (omni::ptrStrMatch(repoId, omniPy::string_Py_omniServant))
    return (omniPy::Py_omniServant*)this;
  if (omni::ptrStrMatch(repoId, PortableServer::ServantManager::_PD_repoId))
    return (PortableServer::_impl_ServantManager*)this;
  if (omni::ptrStrMatch(repoId, CORBA::Object::_PD_repoId))
    return (void*)1;

  return 0;
}


// Implementation of Py_AdapterActivator

Py_AdapterActivator::Py_AdapterActivator(PyObject*   pyaa,
					 PyObject*   opdict,
					 const char* repoId)
  : PY_OMNISERVANT_BASE(pyaa, opdict, repoId), pyaa_(pyaa)
{
  Py_INCREF(pyaa);
}

Py_AdapterActivator::~Py_AdapterActivator()
{
  Py_DECREF(pyaa_);
}

CORBA::Boolean
Py_AdapterActivator::unknown_adapter(PortableServer::POA_ptr parent,
				     const char*             name)
{
  PyObject *method, *argtuple, *pyresult;
  omnipyThreadCache::lock _t;

  method = PyObject_GetAttrString(pyaa_, (char*)"unknown_adapter");
  if (!method) {
    PyErr_Clear();
    OMNIORB_THROW(OBJ_ADAPTER, 0, CORBA::COMPLETED_NO);
    // *** Good choice of exn?
  }
  PortableServer::POA::_duplicate(parent);
  argtuple = Py_BuildValue((char*)"Ns",
			   omniPy::createPyPOAObject(parent), name);

  // Do the up-call
  pyresult = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  if (pyresult) {
    if (!PyInt_Check(pyresult)) {
      Py_DECREF(pyresult);
      OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_NO);
    }
    CORBA::Boolean result = PyInt_AS_LONG(pyresult);
    Py_DECREF(pyresult);
    return result;
  }
  else {
    if (omniORB::trace(5))
      omniORB::logf("omniORBpy: AdapterActivator::unknown_adapter "
		    "raised an exception!");
    if (omniORB::trace(10)) {
      omniORB::logf("omniORBpy: Traceback follows:");
      PyErr_Print();
    }
    else
      PyErr_Clear();
  }
  return 0;
}

void*
Py_AdapterActivator::_ptrToInterface(const char* repoId)
{
  if (omni::ptrStrMatch(repoId, PortableServer::AdapterActivator::_PD_repoId))
    return (PortableServer::_impl_AdapterActivator*)this;
  if (omni::ptrStrMatch(repoId, omniPy::string_Py_omniServant))
    return (omniPy::Py_omniServant*)this;
  if (omni::ptrStrMatch(repoId, CORBA::Object::_PD_repoId))
    return (void*)1;

  return 0;
}




// Functions to create Py_omniServant objects

static
omniPy::Py_omniServant*
newSpecialServant(PyObject* pyservant, PyObject* opdict, char* repoId)
{
  if (omni::ptrStrMatch(repoId, PortableServer::ServantActivator::_PD_repoId))
    return new Py_ServantActivator(pyservant, opdict, repoId);

  if (omni::ptrStrMatch(repoId, PortableServer::ServantLocator::_PD_repoId))
    return new Py_ServantLocator(pyservant, opdict, repoId);

  if (omni::ptrStrMatch(repoId, PortableServer::AdapterActivator::_PD_repoId))
    return new Py_AdapterActivator(pyservant, opdict, repoId);

  OMNIORB_ASSERT(0);
  return 0;
}


omniPy::Py_omniServant*
omniPy::getServantForPyObject(PyObject* pyservant)
{
  if (!PyInstance_Check(pyservant))
    return 0;

  Py_omniServant* pyos;

  // Is there a Py_omniServant already?
  pyos = (omniPy::Py_omniServant*)omniPy::getTwin(pyservant, SERVANT_TWIN);
  if (pyos) {
    pyos->_locked_add_ref();
    return pyos;
  }

  // Is it an instance of the right class?
  PyClassObject* pysc = ((PyInstanceObject*)pyservant)->in_class;

  if (!PyClass_IsSubclass((PyObject*)pysc, omniPy::pyServantClass))
    return 0;

  PyObject* opdict = PyObject_GetAttrString(pyservant, (char*)"_omni_op_d");
  if (!(opdict && PyDict_Check(opdict)))
    return 0;

  PyObject* pyrepoId = PyObject_GetAttrString(pyservant,
					      (char*)"_NP_RepositoryId");
  if (!(pyrepoId && PyString_Check(pyrepoId))) {
    Py_DECREF(opdict);
    return 0;
  }
  if (PyObject_HasAttrString(pyservant, (char*)"_omni_special")) {

    pyos = newSpecialServant(pyservant, opdict, PyString_AS_STRING(pyrepoId));
  }
  else {
    pyos = new omniPy::Py_omniServant(pyservant, opdict,
				      PyString_AS_STRING(pyrepoId));
  }
  Py_DECREF(opdict);
  Py_DECREF(pyrepoId);

  return pyos;
}
