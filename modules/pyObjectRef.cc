// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyObjectRef.cc             Created on: 1999/07/29
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
//    Versions of ORB object ref functions which deal with Python
//    objects, rather than C++ objects

// $Id$

// $Log$
// Revision 1.1.2.2  2000/11/22 14:42:56  dpg1
// Fix segfault in string_to_object and resolve_initial_references with
// nil objref.
//
// Revision 1.1.2.1  2000/10/13 13:55:26  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>

 // Internal omniORB interfaces
#include <ropeFactory.h>
#include <localIdentity.h>
#include <remoteIdentity.h>
#include <objectAdapter.h>
#include <omniORB4/omniURI.h>


#if defined(HAS_Cplusplus_Namespace)
using omniORB::operator==;
#endif


class Py_omniObjRef : public virtual CORBA::Object,
		      public virtual omniObjRef
{
public:
  Py_omniObjRef(const char*        repoId,
		omniIOR*           ior,
		omniIdentity*      id,
		omniLocalIdentity* lid)

    : omniObjRef(repoId, ior, id, lid)
  {
    _PR_setobj(this);
  }
  virtual ~Py_omniObjRef() { }

  virtual CORBA::Boolean _compatibleServant(omniServant* svnt);

private:
  virtual void* _ptrToObjRef(const char* target);

  // Not implemented:
  Py_omniObjRef(const Py_omniObjRef&);
  Py_omniObjRef& operator=(const Py_omniObjRef&);
};

CORBA::Boolean
Py_omniObjRef::_compatibleServant(omniServant* svnt)
{
  if (svnt->_ptrToInterface("Py_omniServant"))
    return 1;
  else
    return 0;
}

void*
Py_omniObjRef::_ptrToObjRef(const char* target)
{
  if (!strcmp(target, CORBA::Object::_PD_repoId))
    return (CORBA::Object_ptr)this;

  if (!strcmp(target, "Py_omniObjRef"))
    return (Py_omniObjRef*)this;

  return 0;
}


PyObject*
omniPy::createPyCorbaObjRef(const char*             targetRepoId,
			    const CORBA::Object_ptr objref)
{
  if (CORBA::is_nil(objref)) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (objref->_NP_is_pseudo())
    return createPyPseudoObjRef(objref);

  omniObjRef* ooref = objref->_PR_getobj();

  const char*    actualRepoId = ooref->_mostDerivedRepoId();
  PyObject*      objrefClass;
  CORBA::Boolean fullTypeUnknown = 0;

  // Try to find objref class for most derived type:
  objrefClass = PyDict_GetItemString(pyomniORBobjrefMap, (char*)actualRepoId);

  if (targetRepoId &&
      strcmp(targetRepoId, actualRepoId) &&
      strcmp(targetRepoId, CORBA::Object::_PD_repoId)) {

    // targetRepoId is not plain CORBA::Object, and is different from
    // actualRepoId

    if (objrefClass) {
      // We've got an objref class for the most derived type. Is it a
      // subclass of the target type?
      PyObject* targetClass = PyDict_GetItemString(pyomniORBobjrefMap,
						   (char*)targetRepoId);

      if (!PyClass_IsSubclass(objrefClass, targetClass)) {
	// Actual type is not derived from the target. Surprisingly
	// enough, this is valid -- the repoId in an object reference
	// is not necessarily that of the most derived type for the
	// object. If we are expecting interface A, and actually get
	// unrelated B, the object might actually have interface C,
	// derived from both A and B.
	//
	// In this situation, we must create an object reference of
	// the target type, not the object's claimed type.
	objrefClass     = targetClass;
	fullTypeUnknown = 1;
      }
    }
    else {
      // No objref class for the most derived type -- try to find one for
      // the target type:
      objrefClass     = PyDict_GetItemString(pyomniORBobjrefMap,
					     (char*)targetRepoId);
      fullTypeUnknown = 1;
    }
  }
  if (!objrefClass) {
    // No target type, or stub code bug:
    objrefClass     = PyObject_GetAttrString(pyCORBAmodule, (char*)"Object");
    fullTypeUnknown = 1;
  }

  OMNIORB_ASSERT(objrefClass); // Couldn't even find CORBA.Object!

  PyObject* pyobjref = PyEval_CallObject(objrefClass, omniPy::pyEmptyTuple);

  if (!pyobjref) {
    // Oh dear -- return the error to the program
    return 0;
  }

  if (fullTypeUnknown) {
    PyObject* idstr = PyString_FromString(actualRepoId);
    PyDict_SetItemString(((PyInstanceObject*)pyobjref)->in_dict,
			 (char*)"_NP_RepositoryId", idstr);
    Py_DECREF(idstr);
  }
  omniPy::setTwin(pyobjref, (CORBA::Object_ptr)objref, OBJREF_TWIN);

  return pyobjref;
}


PyObject*
omniPy::createPyPseudoObjRef(const CORBA::Object_ptr objref)
{
  {
    PortableServer::POA_var poa = PortableServer::POA::_narrow(objref);
    if (!CORBA::is_nil(poa)) return createPyPOAObject(poa);
  }
  {
    PortableServer::POAManager_var pm =
      PortableServer::POAManager::_narrow(objref);
    if (!CORBA::is_nil(pm)) return createPyPOAManagerObject(pm);
  }
  CORBA::MARSHAL ex;
  return handleSystemException(ex);
}



omniObjRef*
omniPy::createObjRef(const char*    targetRepoId,
		     omniIOR*       ior,
		     CORBA::Boolean locked,
		     CORBA::Boolean type_verified)
{
  ASSERT_OMNI_TRACEDMUTEX_HELD(*omni::internalLock, locked);
  OMNIORB_ASSERT(targetRepoId);
  OMNIORB_ASSERT(ior);

  Rope*          rope = 0;
  CORBA::Boolean is_local = 0;

  if (!ropeFactory::iorToRope(ior, rope, is_local)) {
    ior->release();
    return 0;
  }

  if (is_local) {
    CORBA::Octet* key     = ior->iiop.object_key.get_buffer();
    int           keysize = ior->iiop.object_key.length();
    CORBA::ULong  hashv   = omni::hash(key, keysize);

    omni_optional_lock sync(*omni::internalLock, locked, locked);

    // If the identity does not exist in the local object table,
    // locateIdentity() will insert a dummy entry.

    omniLocalIdentity* local_id = omni::locateIdentity(key, keysize, hashv, 1);

    return omniPy::createObjRef(targetRepoId, local_id, ior, type_verified);
  }

  ior->duplicate();

  omniRemoteIdentity* id = new omniRemoteIdentity(ior, rope);

  if (omniORB::trace(10)) {
    omniORB::logger l;
    l << "Creating Python ref to remote: " << id << "\n"
      " target id      : " << targetRepoId << "\n"
      " most derived id: " << (const char*)ior->repositoryID << "\n";
  }
  omniObjRef* objref = new Py_omniObjRef(targetRepoId, ior, id, 0);

  if (!type_verified && strcmp(targetRepoId, CORBA::Object::_PD_repoId))
    objref->pd_flags.type_verified = 0;

  if (!locked) omni::internalLock->lock();
  id->gainObjRef(objref);
  if (!locked) omni::internalLock->unlock();

  return objref;
}


omniObjRef*
omniPy::createObjRef(const char*        targetRepoId,
		     omniLocalIdentity* local_id,
		     omniIOR*           ior,
		     CORBA::Boolean     type_verified)
{
  ASSERT_OMNI_TRACEDMUTEX_HELD(*omni::internalLock, 1);
  OMNIORB_ASSERT(targetRepoId);
  OMNIORB_ASSERT(ior);

  omniIdentity* remote_id = 0;

  // See if a suitable reference exists in the local ref list.
  // Suitable means having the same most-derived-intf-repo-id, and
  // also supporting the <targetRepoId>.
  {
    omniObjRef* objref = local_id->localRefList();

    while (objref) {

      if (!strcmp(ior->repositoryID, objref->_mostDerivedRepoId()) &&
	  objref->_ptrToObjRef("Py_omniObjRef") &&
	  !strcmp(targetRepoId, objref->pd_intfRepoId)) {

	omniORB::logs(15, "omniPy::createObjRef -- reusing reference"
		      " from local ref list.");

	// We just need to check that the ref count is not zero here,
	// 'cos if it is then the objref is about to be deleted!
	// See omni::releaseObjRef().
	
	omni::objref_rc_lock->lock();
	int dying = objref->pd_refCount == 0;
	if (!dying) objref->pd_refCount++;
	omni::objref_rc_lock->unlock();

	if (!dying) {
	  ior->release();
	  return objref;
	}
      }

      if (objref->_identity() != objref->_localId())
	// If there is a remote id available, just keep a
	// note of it in case we need it.
	remote_id = objref->_identity();

      objref = objref->_nextInLocalRefList();
    }
  }

  omniServant* servant = local_id->servant();

  if (omniORB::trace(10)) {
    omniORB::logger l;
    l << "Creating Python ref to local: " << local_id << "\n"
      " target id      : " << targetRepoId << "\n"
      " most derived id: " << ior->repositoryID << "\n";
  }

  if (servant && !servant->_ptrToInterface("Py_omniServant"))
    servant = 0;

  if (servant) {
    remote_id = 0;
  }
  else {

    // There are two possibilities here. Either:
    //  1. This object has not yet been activated, and the object
    //     table contains a dummy entry.
    //  2. The object has been activated, but does not support
    //     the c++ type interface that we require.
    //
    // We use a remote id -- so that the usual mechanisms will be
    // used to indicate that there is a problem.  If the object has
    // not been activated yet, we will replace this remote id with
    // the local id when it is.

    if (!remote_id) {
      Rope* rope = omniObjAdapter::defaultLoopBack();
      rope->incrRefCount();
      ior->duplicate();
      remote_id = new omniRemoteIdentity(ior, rope);
    }
  }

  omniObjRef* objref = new Py_omniObjRef(targetRepoId,
					 ior,
					 remote_id ? remote_id : local_id,
					 local_id);

  if (!type_verified && strcmp(targetRepoId, CORBA::Object::_PD_repoId))
    objref->pd_flags.type_verified = 0;

  local_id->gainObjRef(objref);
  if (remote_id) remote_id->gainObjRef(objref);

  return objref;
}


CORBA::Object_ptr
omniPy::makeLocalObjRef(const char* targetRepoId, CORBA::Object_ptr objref)
{
  ASSERT_OMNI_TRACEDMUTEX_HELD(*omni::internalLock, 0);

  omniObjRef* ooref = objref->_PR_getobj();
  omniObjRef* newooref;
  omniObjKey  key;

  {
    omni_tracedmutex_lock sync(*omni::internalLock);
    newooref = omniPy::createObjRef(targetRepoId, ooref->_localId(),
				    ooref->_getIOR(), 1);
  }
  CORBA::release(objref);
  return (CORBA::Object_ptr)newooref->_ptrToObjRef(CORBA::Object::_PD_repoId);
}


PyObject*
omniPy::copyObjRefArgument(PyObject* pytargetRepoId, PyObject* pyobjref,
			   CORBA::CompletionStatus compstatus)
{
  if (pyobjref == Py_None) {
    // Nil objref
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (!PyInstance_Check(pyobjref)) {
    // Not an objref
    CORBA::BAD_PARAM ex(0,compstatus);
    return omniPy::handleSystemException(ex);
  }
  CORBA::Object_ptr objref = (CORBA::Object_ptr)getTwin(pyobjref, OBJREF_TWIN);
  if (!objref) {
    // Not an objref
    CORBA::BAD_PARAM ex(0,compstatus);
    return omniPy::handleSystemException(ex);
  }

  // To copy an object reference, we have to take a number of things
  // into account. When the C++ object reference was created, it was
  // initialised with a most-derived repoId and a target repoId. If we
  // knew that the most-derived interface is compatible with the
  // target, then the Python objref is of the most derived type. If we
  // did not know the most-derived interface, or we did know it and
  // believed it to be incompatible with the target, then the Python
  // objref is of the target type, and it has a string attribute named
  // "_NP_RepositoryId" containing the most derived repoId.
  //
  // Now, as we are copying this objref, we have a target repoId,
  // which is possibly different from the objref's original target.
  // It's also possible that some time after we created the Python
  // objref, some new stubs were imported, so we now know about the
  // objref's most derived type when before we didn't.
  //
  // So, to copy the reference, we first see if the Python objref has
  // an attribute named "_NP_RepositoryId". If it does, all bets are
  // off, and we have to create a new C++ objref from scratch. If it
  // doesn't have the attribute, we look to see if the objref's class
  // is a subclass of the target objref class (or the same class). If
  // so, we can just incref the existing Python objref and return it;
  // if not, we have to build a new C++ objref.

  if (!PyDict_GetItemString(((PyInstanceObject*)pyobjref)->in_dict,
			    (char*)"_NP_RepositoryId")) {

    PyObject* targetClass = PyDict_GetItem(pyomniORBobjrefMap,
					   pytargetRepoId);
    OMNIORB_ASSERT(targetClass);

    if (PyClass_IsSubclass((PyObject*)((PyInstanceObject*)pyobjref)->in_class,
			   targetClass)) {
      Py_INCREF(pyobjref);
      return pyobjref;
    }
  }
  // Create new C++ and Python objrefs with the right target type
  omniObjRef* ooref        = objref->_PR_getobj();
  const char* targetRepoId = PyString_AS_STRING(pytargetRepoId);

  if (targetRepoId[0] == '\0') targetRepoId = CORBA::Object::_PD_repoId;

  omniObjRef* newooref     = omniPy::createObjRef(targetRepoId,
						  ooref->_getIOR(), 0, 0);
  return createPyCorbaObjRef(targetRepoId,
			     (CORBA::Object_ptr)newooref->
			             _ptrToObjRef(CORBA::Object::_PD_repoId));
}


CORBA::Object_ptr
omniPy::stringToObject(const char* uri)
{
  CORBA::Object_ptr cxxobj;

  cxxobj = omniURI::stringToObject(uri);

  if (CORBA::is_nil(cxxobj) || cxxobj->_NP_is_pseudo()) {
    return cxxobj;
  }
  omniObjRef* cxxobjref = cxxobj->_PR_getobj();
  omniObjRef* objref    = omniPy::createObjRef(CORBA::Object::_PD_repoId,
					       cxxobjref->_getIOR(), 0, 0);
  CORBA::release(cxxobj);
  return (CORBA::Object_ptr)objref->_ptrToObjRef(CORBA::Object::_PD_repoId);
}


CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, cdrStream& s)
{
  CORBA::String_var          id;
  IOP::TaggedProfileList_var profiles;

  id = IOP::IOR::unmarshaltype_id(s);

  profiles = new IOP::TaggedProfileList();
  (IOP::TaggedProfileList&)profiles <<= s;

  if (profiles->length() == 0 && strlen(id) == 0) {
    // Nil object reference
    return CORBA::Object::_nil();
  }
  else {
    // It is possible that we reach here with the id string = '\0'.
    // That is alright because the actual type of the object will be
    // verified using _is_a() at the first invocation on the object.
    //
    // Apparently, some ORBs such as ExperSoft's do that. Furthermore,
    // this has been accepted as a valid behaviour in GIOP 1.1/IIOP 1.1.
    // 
    omniIOR* ior = new omniIOR(id._retn(),profiles._retn());
    omniObjRef* objref = omniPy::createObjRef(repoId,ior,0);

    if (!objref) OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_MAYBE);
    return 
      (CORBA::Object_ptr)objref->_ptrToObjRef(CORBA::Object::_PD_repoId);
  }
  return 0; // To shut GCC up
}
