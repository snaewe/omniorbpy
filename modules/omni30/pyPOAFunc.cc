// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyPOAFunc.cc               Created on: 2000/02/04
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
//    POA functions

// $Id$
// $Log$
// Revision 1.6  2000/03/31 14:20:18  dpg1
// Arguments to activate_object_with_id() were in the wrong order.
//
// Revision 1.5  2000/03/30 13:01:14  dpg1
// Locking fixed for ~Py_omniServant().
//
// Revision 1.4  2000/03/24 16:48:57  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.3  2000/03/07 16:52:16  dpg1
// Support for compilers which do not allow exceptions to be caught by
// base class. (Like MSVC 5, surprise surprise.)
//
// Revision 1.2  2000/03/06 18:48:28  dpg1
// Support for our favourite compiler, MSVC.
//
// Revision 1.1  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//

#include <omnipy.h>


PyObject*
omniPy::createPyPOAObject(const PortableServer::POA_ptr poa)
{
  if (CORBA::is_nil(poa)) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject* pypoa_class =
    PyObject_GetAttrString(omniPy::pyPortableServerModule, (char*)"POA");
  OMNIORB_ASSERT(pypoa_class);

  PyObject* pypoa = PyEval_CallObject(pypoa_class, omniPy::pyEmptyTuple);
  if (!pypoa) {
    // Oh dear!  Return the exception to python
    return 0;
  }
  omniPy::setTwin(pypoa, (PortableServer::POA_ptr)poa, POA_TWIN);
  omniPy::setTwin(pypoa, (CORBA::Object_ptr)      poa, OBJREF_TWIN);
  return pypoa;
}


static
PyObject* raisePOAException(PyObject* pyPOA, const char* ename)
{
  PyObject* excc = PyObject_GetAttrString(pyPOA, (char*)ename);
  OMNIORB_ASSERT(excc);
  PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
  PyErr_SetObject(excc, exci);
  return 0;
}

static
CORBA::Policy_ptr createPolicyObject(PortableServer::POA_ptr poa,
				     PyObject* pypolicy)
{
  CORBA::Policy_ptr policy = 0;

  PyObject* pyptype  = PyObject_GetAttrString(pypolicy, (char*)"_policy_type");
  PyObject* pyvalue  = PyObject_GetAttrString(pypolicy, (char*)"_value");
  PyObject* pyivalue = 0;

  if (pyptype && PyInt_Check(pyptype) &&
      pyvalue && PyInstance_Check(pyvalue)) {

    pyivalue = PyObject_GetAttrString(pyvalue, (char*)"_v");

    if (pyivalue && PyInt_Check(pyivalue)) {
      CORBA::ULong ivalue = PyInt_AS_LONG(pyivalue);

      switch (PyInt_AS_LONG(pyptype)) {

      case 16: // ThreadPolicy
	policy = poa->
	  create_thread_policy((PortableServer::
				ThreadPolicyValue)
			       ivalue);
	break;

      case 17: // LifespanPolicy
	policy = poa->
	  create_lifespan_policy((PortableServer::
				  LifespanPolicyValue)
				 ivalue);
	break;

      case 18: // IdUniquenessPolicy
	policy = poa->
	  create_id_uniqueness_policy((PortableServer::
				       IdUniquenessPolicyValue)
				      ivalue);
	break;

      case 19: // IdAssignmentPolicy
	policy = poa->
	  create_id_assignment_policy((PortableServer::
				       IdAssignmentPolicyValue)
				      ivalue);
	break;

      case 20: // ImplicitActivationPolicy
	policy = poa->
	  create_implicit_activation_policy((PortableServer::
					     ImplicitActivationPolicyValue)
					    ivalue);
	break;

      case 21: // ServantRetentionPolicy
	policy = poa->
	  create_servant_retention_policy((PortableServer::
					   ServantRetentionPolicyValue)
					  ivalue);
	break;

      case 22: // RequestProcessingPolicy
	policy = poa->
	  create_request_processing_policy((PortableServer::
					    RequestProcessingPolicyValue)
					   ivalue);
	break;
      }
    }
  }
  Py_XDECREF(pyptype);
  Py_XDECREF(pyvalue);
  Py_XDECREF(pyivalue);
  Py_DECREF(pypolicy);

  if (policy) return policy;

  PyErr_Clear();
  throw CORBA::BAD_PARAM();
  return 0; // For MSVC
}
  

extern "C" {

  static PyObject* pyPOA_create_POA(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     name;
    PyObject* pyPM;
    PyObject* pypolicies;

    if (!PyArg_ParseTuple(args, (char*)"OsOO",
			  &pyPOA, &name, &pyPM, &pypolicies))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PySequence_Check(pypolicies));

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    PortableServer::POAManager_ptr pm;

    if (pyPM == Py_None)
      pm = PortableServer::POAManager::_nil();
    else {
      pm = (PortableServer::POAManager_ptr)omniPy::getTwin(pyPM,
							   POAMANAGER_TWIN);
      OMNIORB_ASSERT(pm);
    }

    try {
      // Convert Python Policy objects to C++ Policy objects
      CORBA::ULong numpolicies = PySequence_Length(pypolicies);
      CORBA::PolicyList policies(numpolicies);
      policies.length(numpolicies);

      for (CORBA::ULong i=0; i < numpolicies; i++) {
	policies[i] = createPolicyObject(poa,
					 PySequence_GetItem(pypolicies, i));
      }

      // Call the function
      PortableServer::POA_ptr child;
      {
	omniPy::InterpreterUnlocker _u;
	child = poa->create_POA(name, pm, policies);
      }
      return omniPy::createPyPOAObject(child);
    }
    catch (PortableServer::POA::AdapterAlreadyExists& ex) {
      return raisePOAException(pyPOA, "AdapterAlreadyExists");
    }
    catch (PortableServer::POA::InvalidPolicy& ex) {
      PyObject* excc = PyObject_GetAttrString(pyPOA,
					      (char*)"InvalidPolicy");
      OMNIORB_ASSERT(excc);
      PyObject* exci = PyObject_CallFunction(excc, (char*)"i", ex.index);
      PyErr_SetObject(excc, exci);
      return 0;
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_find_POA(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     name;
    int       activate_it;

    if (!PyArg_ParseTuple(args, (char*)"Osi", &pyPOA, &name, &activate_it))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    // Call the function
    try {
      PortableServer::POA_ptr found;
      {
	omniPy::InterpreterUnlocker _u;
	found = poa->find_POA(name, activate_it);
      }
      return omniPy::createPyPOAObject(found);
    }
    catch (PortableServer::POA::AdapterNonExistent& ex) {
      return raisePOAException(pyPOA, "AdapterNonExistent");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_destroy(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    int       eth, wait;

    if (!PyArg_ParseTuple(args, (char*)"Oii",& pyPOA, &eth, &wait))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    // Call the function
    try {
      {
	omniPy::InterpreterUnlocker _u;
	poa->destroy(eth, wait);
      }
      Py_INCREF(Py_None);
      return Py_None;
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_the_name(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      return PyString_FromString(poa->the_name());
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_the_parent(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      return omniPy::createPyPOAObject(poa->the_parent());
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_the_children(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::POAList_var pl = poa->the_children();

      PyObject* pypl = PyList_New(pl->length());

      for (CORBA::ULong i=0; i < pl->length(); i++)
	PyList_SetItem(pypl, i, omniPy::createPyPOAObject(pl[i]));

      return pypl;
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_the_POAManager(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      return omniPy::createPyPOAManagerObject(poa->the_POAManager());
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_the_activator(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::AdapterActivator_var act = poa->the_activator();

      if (CORBA::is_nil(act)) {
	Py_INCREF(Py_None);
	return Py_None;
      }
      else {
	const char* repoId = act->_PR_getobj()->_mostDerivedRepoId();
	CORBA::Object_ptr lobjref = omniPy::makeLocalObjRef(repoId, act);
	return omniPy::createPyCorbaObjRef(repoId, lobjref);
      }
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_set_the_activator(PyObject* self, PyObject* args)
  {
    PyObject *pyPOA, *pyact;
    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyact)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    CORBA::Object_ptr actobj = (CORBA::Object_ptr)omniPy::getTwin(pyact,
								  OBJREF_TWIN);
    RAISE_PY_BAD_PARAM_IF(!actobj);

    PortableServer::AdapterActivator_var act;
    {
      omniPy::InterpreterUnlocker _u;
      act = PortableServer::AdapterActivator::_narrow(actobj);
    }
    RAISE_PY_BAD_PARAM_IF(CORBA::is_nil(act));

    try {
      poa->the_activator(act);
      Py_INCREF(Py_None);
      return Py_None;
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_servant_manager(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::ServantManager_var sm = poa->get_servant_manager();

      if (CORBA::is_nil(sm)) {
	Py_INCREF(Py_None);
	return Py_None;
      }
      else {
	const char* repoId = sm->_PR_getobj()->_mostDerivedRepoId();
	CORBA::Object_ptr lobjref = omniPy::makeLocalObjRef(repoId, sm);
	return omniPy::createPyCorbaObjRef(repoId, lobjref);
      }
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_set_servant_manager(PyObject* self, PyObject* args)
  {
    PyObject *pyPOA, *pymgr;
    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pymgr)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    CORBA::Object_ptr mgrobj = (CORBA::Object_ptr)omniPy::getTwin(pymgr,
								  OBJREF_TWIN);
    RAISE_PY_BAD_PARAM_IF(!mgrobj);

    PortableServer::ServantManager_var mgr;
    {
      omniPy::InterpreterUnlocker _u;
      mgr = PortableServer::ServantManager::_narrow(mgrobj);
    }
    RAISE_PY_BAD_PARAM_IF(CORBA::is_nil(mgr));

    try {
      poa->set_servant_manager(mgr);
      Py_INCREF(Py_None);
      return Py_None;
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_get_servant(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;

    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::Servant servant;
      omniPy::Py_omniServant* pyos;
      PyObject*               pyservant;
      {
	omniPy::InterpreterUnlocker _u;
	servant = poa->get_servant();
	pyos =
	  (omniPy::Py_omniServant*)servant->_ptrToInterface("Py_omniServant");

	if (pyos) {
	  pyservant = pyos->pyServant();
	  pyos->_remove_ref();
	}
      }
      if (pyos) {
	return pyservant;
      }
      else {
	// Oh dear -- the servant is C++, not Python. OBJ_ADAPTER
	// seems the most sensible choice of exception.
	CORBA::OBJ_ADAPTER ex;
	return omniPy::handleSystemException(ex);
      }
    }
    catch (PortableServer::POA::NoServant& ex) {
      return raisePOAException(pyPOA, "NoServant");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_set_servant(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyServant)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    omniPy::Py_omniServant* pyos = omniPy::getServantForPyObject(pyServant);
    RAISE_PY_BAD_PARAM_IF(!pyos);

    try {
      {
	omniPy::InterpreterUnlocker _u;
	poa->set_servant(pyos);
	pyos->_remove_ref();
      }
      Py_INCREF(Py_None);
      return Py_None;
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_activate_object(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyServant)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    omniPy::Py_omniServant* pyos = omniPy::getServantForPyObject(pyServant);
    RAISE_PY_BAD_PARAM_IF(!pyos);

    try {
      PortableServer::ObjectId_var oid;
      {
	omniPy::InterpreterUnlocker _u;
	oid = poa->activate_object(pyos);
	pyos->_remove_ref();
      }
      PyObject* pyoid = PyString_FromStringAndSize((const char*)oid->NP_data(),
						   oid->length());
      return pyoid;
    }
    catch (PortableServer::POA::ServantAlreadyActive& ex) {
      return raisePOAException(pyPOA, "ServantAlreadyActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_activate_object_with_id(PyObject* self,
						 PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#O",
			  &pyPOA, &oidstr, &oidlen, &pyServant))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    omniPy::Py_omniServant* pyos = omniPy::getServantForPyObject(pyServant);
    RAISE_PY_BAD_PARAM_IF(!pyos);

    try {
      PortableServer::ObjectId oid(oidlen, oidlen, (CORBA::Octet*)oidstr, 0);
      {
	omniPy::InterpreterUnlocker _u;
	poa->activate_object_with_id(oid, pyos);
	pyos->_remove_ref();
      }
      Py_INCREF(Py_None);
      return Py_None;
    }
    catch (PortableServer::POA::ServantAlreadyActive& ex) {
      return raisePOAException(pyPOA, "ServantAlreadyActive");
    }
    catch (PortableServer::POA::ObjectAlreadyActive& ex) {
      return raisePOAException(pyPOA, "ObjectAlreadyActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_deactivate_object(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#", &pyPOA, &oidstr, &oidlen))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::ObjectId oid(oidlen, oidlen, (CORBA::Octet*)oidstr, 0);
      {
	omniPy::InterpreterUnlocker _u;
	poa->deactivate_object(oid);
      }
      Py_INCREF(Py_None);
      return Py_None;
    }
    catch (PortableServer::POA::ObjectNotActive& ex) {
      return raisePOAException(pyPOA, "ObjectNotActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_create_reference(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     repoId;

    if (!PyArg_ParseTuple(args, (char*)"Os", &pyPOA, &repoId))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      CORBA::Object_ptr objref;
      {
	omniPy::InterpreterUnlocker _u;
	objref = poa->create_reference(repoId);
      }
      CORBA::Object_ptr lobjref = omniPy::makeLocalObjRef(repoId, objref);
      return omniPy::createPyCorbaObjRef(repoId, lobjref);
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_create_reference_with_id(PyObject* self,
						  PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;
    char*     repoId;

    if (!PyArg_ParseTuple(args, (char*)"Os#s",
			  &pyPOA, &oidstr, &oidlen, &repoId))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::ObjectId oid(oidlen, oidlen, (CORBA::Octet*)oidstr, 0);
      CORBA::Object_ptr objref;
      {
	omniPy::InterpreterUnlocker _u;
	objref = poa->create_reference_with_id(oid, repoId);
      }
      CORBA::Object_ptr lobjref = omniPy::makeLocalObjRef(repoId, objref);
      return omniPy::createPyCorbaObjRef(repoId, lobjref);
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_servant_to_id(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyServant)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    omniPy::Py_omniServant* pyos = omniPy::getServantForPyObject(pyServant);
    RAISE_PY_BAD_PARAM_IF(!pyos);

    try {
      PortableServer::ObjectId_var oid;
      {
	omniPy::InterpreterUnlocker _u;
	oid = poa->servant_to_id(pyos);
	pyos->_remove_ref();
      }
      PyObject* pyoid = PyString_FromStringAndSize((const char*)oid->NP_data(),
						   oid->length());
      return pyoid;
    }
    catch (PortableServer::POA::ServantNotActive& ex) {
      return raisePOAException(pyPOA, "ServantNotActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_servant_to_reference(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyServant)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    omniPy::Py_omniServant* pyos = omniPy::getServantForPyObject(pyServant);
    RAISE_PY_BAD_PARAM_IF(!pyos);

    try {
      CORBA::Object_ptr objref;
      {
	omniPy::InterpreterUnlocker _u;
	objref = poa->servant_to_reference(pyos);
      }
      CORBA::Object_ptr lobjref =
	omniPy::makeLocalObjRef(pyos->_mostDerivedRepoId(), objref);

      PyObject* result =
	omniPy::createPyCorbaObjRef(pyos->_mostDerivedRepoId(), lobjref);
      {
	omniPy::InterpreterUnlocker _u;
	pyos->_remove_ref();
      }
      return result;
    }
    catch (PortableServer::POA::ServantNotActive& ex) {
      return raisePOAException(pyPOA, "ServantNotActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_reference_to_servant(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyobjref)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr objref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!objref);

    try {
      PortableServer::Servant servant;
      omniPy::Py_omniServant* pyos;
      PyObject*               pyservant;
      {
	omniPy::InterpreterUnlocker _u;
	servant = poa->reference_to_servant(objref);
	pyos =
	  (omniPy::Py_omniServant*)servant->_ptrToInterface("Py_omniServant");

	if (pyos) {
	  PyObject* pyservant = pyos->pyServant();
	  pyos->_remove_ref();
	}
      }
      if (pyos) {
	return pyservant;
      }
      else {
	// Oh dear -- the servant is C++, not Python. OBJ_ADAPTER
	// seems the most sensible choice of exception.
	CORBA::OBJ_ADAPTER ex;
	return omniPy::handleSystemException(ex);
      }
    }
    catch (PortableServer::POA::ObjectNotActive& ex) {
      return raisePOAException(pyPOA, "ObjectNotActive");
    }
    catch (PortableServer::POA::WrongAdapter& ex) {
      return raisePOAException(pyPOA, "WrongAdapter");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_reference_to_id(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyobjref)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr objref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!objref);

    try {
      PortableServer::ObjectId_var oid;
      {
	omniPy::InterpreterUnlocker _u;
	oid = poa->reference_to_id(objref);
      }
      PyObject* pyoid = PyString_FromStringAndSize((const char*)oid->NP_data(),
						   oid->length());
      return pyoid;
    }
    catch (PortableServer::POA::WrongAdapter& ex) {
      return raisePOAException(pyPOA, "WrongAdapter");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_id_to_servant(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#", &pyPOA, &oidstr, &oidlen))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::ObjectId oid(oidlen, oidlen, (CORBA::Octet*)oidstr, 0);
      PortableServer::Servant servant;
      omniPy::Py_omniServant* pyos;
      PyObject*               pyservant;
      {
	omniPy::InterpreterUnlocker _u;
	servant = poa->id_to_servant(oid);
	pyos =
	  (omniPy::Py_omniServant*)servant->_ptrToInterface("Py_omniServant");

	if (pyos) {
	  pyservant = pyos->pyServant();
	  pyos->_remove_ref();
	}
      }
      if (pyos) {
	return pyservant;
      }
      else {
	// Oh dear -- the servant is C++, not Python. OBJ_ADAPTER
	// seems the most sensible choice of exception.
	CORBA::OBJ_ADAPTER ex;
	return omniPy::handleSystemException(ex);
      }
    }
    catch (PortableServer::POA::ObjectNotActive& ex) {
      return raisePOAException(pyPOA, "ObjectNotActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_id_to_reference(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#", &pyPOA, &oidstr, &oidlen))
      return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);
    OMNIORB_ASSERT(poa);

    try {
      PortableServer::ObjectId oid(oidlen, oidlen, (CORBA::Octet*)oidstr, 0);
      CORBA::Object_ptr objref;
      {
	omniPy::InterpreterUnlocker _u;
	objref = poa->id_to_reference(oid);
      }
      const char*       mdri    = objref->_PR_getobj()->_mostDerivedRepoId();
      CORBA::Object_ptr lobjref = omniPy::makeLocalObjRef(mdri, objref);
      return omniPy::createPyCorbaObjRef(mdri, lobjref);
    }
    catch (PortableServer::POA::ObjectNotActive& ex) {
      return raisePOAException(pyPOA, "ObjectNotActive");
    }
    catch (PortableServer::POA::WrongPolicy& ex) {
      return raisePOAException(pyPOA, "WrongPolicy");
    }
    OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS
  }

  static PyObject* pyPOA_releaseRef(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    PortableServer::POA_ptr poa =
      (PortableServer::POA_ptr)omniPy::getTwin(pyPOA, POA_TWIN);

    if (poa) {
      {
	omniPy::InterpreterUnlocker _u;
	CORBA::release(poa);
      }
      omniPy::remTwin(pyPOA, POA_TWIN);
      omniPy::remTwin(pyPOA, OBJREF_TWIN);
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  // The PortableServer::Servant::_this() function doesn't really
  // belong here, but it's silly to have a whole func module just for
  // one function.
  static PyObject* pyPOA_servantThis(PyObject* self, PyObject* args)
  {
    PyObject* pyservant;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyservant)) return 0;

    omniPy::Py_omniServant* pyos = omniPy::getServantForPyObject(pyservant);

    RAISE_PY_BAD_PARAM_IF(!pyos);
    PyObject* result = pyos->py_this();
    {
      omniPy::InterpreterUnlocker _u;
      pyos->_remove_ref();
    }
    return result;
  }


  ////////////////////////////////////////////////////////////////////////////
  // Python method table                                                    //
  ////////////////////////////////////////////////////////////////////////////

  static PyMethodDef pyPOA_methods[] = {
    {(char*)"create_POA",           pyPOA_create_POA,            METH_VARARGS},
    {(char*)"find_POA",             pyPOA_find_POA,              METH_VARARGS},
    {(char*)"destroy",              pyPOA_destroy,               METH_VARARGS},
    {(char*)"_get_the_name",        pyPOA_get_the_name,          METH_VARARGS},
    {(char*)"_get_the_parent",      pyPOA_get_the_parent,        METH_VARARGS},
    {(char*)"_get_the_children",    pyPOA_get_the_children,      METH_VARARGS},
    {(char*)"_get_the_POAManager",  pyPOA_get_the_POAManager,    METH_VARARGS},
    {(char*)"_get_the_activator",   pyPOA_get_the_activator,     METH_VARARGS},
    {(char*)"_set_the_activator",   pyPOA_set_the_activator,     METH_VARARGS},
    {(char*)"get_servant_manager",  pyPOA_get_servant_manager,   METH_VARARGS},
    {(char*)"set_servant_manager",  pyPOA_set_servant_manager,   METH_VARARGS},
    {(char*)"get_servant",          pyPOA_get_servant,           METH_VARARGS},
    {(char*)"set_servant",          pyPOA_set_servant,           METH_VARARGS},
    {(char*)"activate_object",      pyPOA_activate_object,       METH_VARARGS},
    {(char*)"activate_object_with_id",
                                    pyPOA_activate_object_with_id,
                                                                 METH_VARARGS},
    {(char*)"deactivate_object",    pyPOA_deactivate_object,     METH_VARARGS},
    {(char*)"create_reference",     pyPOA_create_reference,      METH_VARARGS},
    {(char*)"create_reference_with_id",
                                    pyPOA_create_reference_with_id,
                                                                 METH_VARARGS},
    {(char*)"servant_to_id",        pyPOA_servant_to_id,         METH_VARARGS},
    {(char*)"servant_to_reference", pyPOA_servant_to_reference,  METH_VARARGS},
    {(char*)"reference_to_servant", pyPOA_reference_to_servant,  METH_VARARGS},
    {(char*)"reference_to_id",      pyPOA_reference_to_id,       METH_VARARGS},
    {(char*)"id_to_servant",        pyPOA_id_to_servant,         METH_VARARGS},
    {(char*)"id_to_reference",      pyPOA_id_to_reference,       METH_VARARGS},

    {(char*)"releaseRef",           pyPOA_releaseRef,            METH_VARARGS},

    {(char*)"servantThis",          pyPOA_servantThis,           METH_VARARGS},

    {NULL,NULL}
  };
}

void
omniPy::initPOAFunc(PyObject* d)
{
  PyObject* m = Py_InitModule((char*)"_omnipy.poa_func", pyPOA_methods);
  PyDict_SetItemString(d, (char*)"poa_func", m);
}
