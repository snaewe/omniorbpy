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
// Revision 1.17.2.4  2001/05/03 15:25:02  dpg1
// Various places released object references while holding the
// interpreter lock. Object reference deletion locks omni::internalLock,
// so this could cause deadlocks against Servant::_add_ref().
//
// Revision 1.17.2.3  2001/02/14 15:22:20  dpg1
// Fix bug using repoId strings after deletion.
//
// Revision 1.17.2.2  2000/11/29 17:11:18  dpg1
// Fix deadlock when trying to lock omniORB internal lock while holding
// the Python interpreter lock.
//
// Revision 1.17.2.1  2000/11/21 10:59:58  dpg1
// Segfault when string_to_object returns a nil objref.
//
// Revision 1.17  2000/06/27 15:13:12  dpg1
// New copyObjRefArgument() function
//
// Revision 1.16  2000/06/16 17:33:18  dpg1
// When creating an object reference with target type A, when the object
// claims to be B, but A and B are unrelated, now creates a reference of
// type A, where before it would be B.
//
// Revision 1.15  2000/05/11 11:58:25  dpg1
// Throw system exceptions with OMNIORB_THROW.
//
// Revision 1.14  2000/04/27 11:03:40  dpg1
// Support for ORB core Interoperable Naming Service changes.
//
// Revision 1.13  2000/03/28 11:03:37  dpg1
// Failed to copy IOP profiles if release_profiles was false.
//
// Revision 1.12  2000/03/24 16:48:57  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.11  2000/03/17 15:57:07  dpg1
// Correct, and more consistent handling of invalid strings in
// string_to_object().
//
// Revision 1.10  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//
// Revision 1.9  1999/12/15 12:17:19  dpg1
// Changes to compile with SunPro CC 5.0.
//
// Revision 1.8  1999/11/16 17:32:36  dpg1
// Changes for AIX.
//
// Revision 1.7  1999/10/11 16:52:51  dpg1
// Fixed reference counting bug when receiving a reference to a local C++
// object.
//
// Revision 1.6  1999/09/29 11:25:55  dpg1
// Nil objects now map to None. They work too, which is more than can be
// said for the old mapping...
//
// Revision 1.5  1999/09/24 09:22:03  dpg1
// Added copyright notices.
//
// Revision 1.4  1999/09/22 15:46:12  dpg1
// Fake POA implemented.
//
// Revision 1.3  1999/09/20 16:01:54  dpg1
// *** empty log message ***
//
// Revision 1.2  1999/09/20 14:55:04  dpg1
// GCC 2.95 is more pedantic than egcs.
//
// Revision 1.1  1999/07/29 14:20:05  dpg1
// Initial revision
//

#include <omnipy.h>

 // Internal omniORB interfaces
#include <ropeFactory.h>
#include <localIdentity.h>
#include <remoteIdentity.h>
#include <objectAdapter.h>
#include <omniORB3/omniURI.h>


#if defined(HAS_Cplusplus_Namespace)
using omniORB::operator==;
#endif


class Py_omniObjRef : public virtual CORBA::Object,
		      public virtual omniObjRef
{
public:
  Py_omniObjRef(const char* repoId,
		const char* mdri,
		IOP::TaggedProfileList* p,
		omniIdentity* id,
		omniLocalIdentity* lid)

    : omniObjRef(repoId, mdri, p, id, lid)
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
omniPy::createObjRef(const char*             mostDerivedRepoId,
		     const char*             targetRepoId,
		     IOP::TaggedProfileList* profiles,
		     CORBA::Boolean          release_profiles,
		     CORBA::Boolean          locked,
		     CORBA::Boolean          type_verified)
{
  ASSERT_OMNI_TRACEDMUTEX_HELD(*omni::internalLock, locked);
  OMNIORB_ASSERT(mostDerivedRepoId);
  OMNIORB_ASSERT(targetRepoId);
  OMNIORB_ASSERT(profiles);

  // NB. <mostDerivedRepoId> can be the empty string "".

  int            keysize = 0;
  CORBA::Octet*  key = 0;
  Rope*          rope = 0;
  CORBA::Boolean is_local = 0;

  if (!ropeFactory::iopProfilesToRope(*profiles, key, keysize,
				      rope, is_local)) {
    if (release_profiles) delete profiles;
    return 0;
  }

  if (is_local) {
    CORBA::ULong hashv = omni::hash(key, keysize);

    omni_optional_lock sync(*omni::internalLock, locked, locked);

    // If the identity does not exist in the local object table,
    // locateIdentity() will insert a dummy entry.

    omniLocalIdentity* local_id = omni::locateIdentity(key, keysize, hashv, 1);

    return omniPy::createObjRef(mostDerivedRepoId, targetRepoId, local_id,
				profiles, release_profiles, key,
				type_verified);
  }

  if (!release_profiles) profiles = new IOP::TaggedProfileList(*profiles);
  omniRemoteIdentity*    id       = new omniRemoteIdentity(rope, key, keysize);

  if (omniORB::trace(10)) {
    omniORB::logger l;
    l << "Creating Python ref to remote: " << id << "\n"
      " target id      : " << targetRepoId << "\n"
      " most derived id: " << mostDerivedRepoId << "\n";
  }
  omniObjRef* objref = new Py_omniObjRef(targetRepoId, mostDerivedRepoId,
					 profiles, id, 0);

  if (!type_verified && strcmp(targetRepoId, CORBA::Object::_PD_repoId))
    objref->pd_flags.type_verified = 0;

  if (!locked) omni::internalLock->lock();
  id->gainObjRef(objref);
  if (!locked) omni::internalLock->unlock();

  return objref;
}


omniObjRef*
omniPy::createObjRef(const char*             mostDerivedRepoId,
		     const char*             targetRepoId,
		     omniLocalIdentity*      local_id,
		     IOP::TaggedProfileList* profiles,
		     CORBA::Boolean          release_profiles,
		     CORBA::Octet*           key,
		     CORBA::Boolean          type_verified)
{
  ASSERT_OMNI_TRACEDMUTEX_HELD(*omni::internalLock, 1);
  OMNIORB_ASSERT(mostDerivedRepoId);
  OMNIORB_ASSERT(targetRepoId);

  // NB. <mostDerivedRepoId> can be the empty string "".

  omniIdentity* remote_id = 0;

  // See if a suitable reference exists in the local ref list.
  // Suitable means having the same most-derived-intf-repo-id, and
  // also supporting the <targetRepoId>.
  {
    omniObjRef* objref = local_id->localRefList();

    while (objref) {

      if (!strcmp(mostDerivedRepoId, objref->_mostDerivedRepoId()) &&
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
	  if (key) delete[] key;
	  if (profiles && release_profiles) delete profiles;
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
      " most derived id: " << mostDerivedRepoId << "\n";
  }

  if (servant && !servant->_ptrToInterface("Py_omniServant"))
    servant = 0;

  if (servant) {
    remote_id = 0;
    if (key) delete[] key;
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
      if (!key) {
	key = new CORBA::Octet[local_id->keysize()];
	memcpy(key, local_id->key(), local_id->keysize());
      }
      remote_id = new omniRemoteIdentity(rope, key, local_id->keysize());
    }
    else
      if (key) delete[] key;
  }

  if (profiles) {
    if (!release_profiles)  profiles = new IOP::TaggedProfileList(*profiles);
  }
  else {
    omniObjRef* other_local_ref = local_id->localRefList();
    if (other_local_ref)
      profiles = new IOP::TaggedProfileList(*other_local_ref->_iopProfiles());
    else {
      profiles = new IOP::TaggedProfileList;
      ropeFactory_iterator iter(omniObjAdapter::incomingRopeFactories());
      incomingRopeFactory* rp;
      while ((rp = (incomingRopeFactory*) iter()))
	rp->getIncomingIOPprofiles(local_id->key(),
				   local_id->keysize(),
				   *profiles);
    }
  }

  omniObjRef* objref = new Py_omniObjRef(targetRepoId, mostDerivedRepoId,
					 profiles,
					 remote_id ? remote_id : local_id,
					 local_id);

  if (!type_verified && strcmp(targetRepoId, CORBA::Object::_PD_repoId))
    objref->pd_flags.type_verified = 0;

  local_id->gainObjRef(objref);
  if (remote_id) remote_id->gainObjRef(objref);

  return objref;
}


CORBA::Object_ptr
omniPy::makeLocalObjRef(const char* targetRepoId,
			const CORBA::Object_ptr objref)
{
  ASSERT_OMNI_TRACEDMUTEX_HELD(*omni::internalLock, 0);

  omniObjRef* ooref = objref->_PR_getobj();
  omniObjRef* newooref;
  omniObjKey  key;

  {
    omni_tracedmutex_lock sync(*omni::internalLock);
    ooref->_getTheKey(key, 1);
    newooref = omniPy::createObjRef(ooref->_mostDerivedRepoId(),
				    targetRepoId,
				    ooref->_localId(),
				    ooref->_iopProfiles(), 0,
				    key.return_key(), 1);
  }
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
  const char* actualRepoId = ooref->_mostDerivedRepoId();
  const char* targetRepoId = PyString_AS_STRING(pytargetRepoId);

  if (targetRepoId[0] == '\0') targetRepoId = CORBA::Object::_PD_repoId;

  omniObjRef* newooref;
  {
    omniPy::InterpreterUnlocker _u;
    newooref = omniPy::createObjRef(actualRepoId,
				    targetRepoId,
				    ooref->_iopProfiles(), 0,
				    0, 0);
  }
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
  omniObjRef* objref;
  {
    omniPy::InterpreterUnlocker _u;
    objref = omniPy::createObjRef(cxxobjref->_mostDerivedRepoId(),
				  CORBA::Object::_PD_repoId,
				  cxxobjref->_iopProfiles(),
				  0, 0);
    CORBA::release(cxxobj);
  }
  return (CORBA::Object_ptr)objref->_ptrToObjRef(CORBA::Object::_PD_repoId);
}


CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, NetBufferedStream& s)
{
  omniPy::InterpreterUnlocker _u;
  OMNIORB_ASSERT(repoId);

  CORBA::ULong idlen;
  CORBA::Char* id = 0;
  IOP::TaggedProfileList* profiles = 0;

  try {
    idlen <<= s;

    switch (idlen) {

    case 0:
      // According to the CORBA specification 2.0 section 10.6.2:
      //   Null object references are indicated by an empty set of
      //   profiles, and by a NULL type ID (a string which contain
      //   only *** a single terminating character ***).
      //
      // Therefore the idlen should be 1.
      // Visibroker for C++ (Orbeline) 2.0 Release 1.51 gets it wrong
      // and sends out a 0 len string.
      id = new CORBA::Char[1];
      id[0] = (CORBA::Char)'\0';
      break;

    case 1:
      id = new CORBA::Char[1];
      id[0] <<= s;
      if (id[0] != (CORBA::Char)'\0')
	OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_MAYBE);
      idlen = 0;
      break;

    default:
      if (idlen > s.RdMessageUnRead())
	OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_MAYBE);
      id = new CORBA::Char[idlen];
      if (!id) OMNIORB_THROW(NO_MEMORY,0,CORBA::COMPLETED_MAYBE);
      s.get_char_array(id, idlen);
      if (id[idlen - 1] != '\0')
	OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_MAYBE);
      break;
    }

    profiles = new IOP::TaggedProfileList();
    if (!profiles) OMNIORB_THROW(NO_MEMORY,0,CORBA::COMPLETED_MAYBE);
    *profiles <<= s;

    if (profiles->length() == 0 && idlen == 0) {
      // This is a nil object reference
      delete profiles;
      delete[] id;
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
      omniObjRef* objref = omniPy::createObjRef((const char*) id, repoId,
						profiles, 1, 0);
      profiles = 0;
      delete [] id;
      id = 0;

      if (!objref) OMNIORB_THROW(INV_OBJREF,0, CORBA::COMPLETED_MAYBE);
      return 
	(CORBA::Object_ptr)objref->_ptrToObjRef(CORBA::Object::_PD_repoId);
    }
  }
  catch (...) {
    if (id)        delete[] id;
    if (profiles)  delete profiles;
    throw;
  }
}


CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, MemBufferedStream& s)
{
  omniPy::InterpreterUnlocker _u;
  OMNIORB_ASSERT(repoId);

  CORBA::ULong idlen;
  CORBA::Char* id = 0;
  IOP::TaggedProfileList* profiles = 0;

  try {
    idlen <<= s;

    switch (idlen) {

    case 0:
      // According to the CORBA specification 2.0 section 10.6.2:
      //   Null object references are indicated by an empty set of
      //   profiles, and by a NULL type ID (a string which contain
      //   only *** a single terminating character ***).
      //
      // Therefore the idlen should be 1.
      // Visibroker for C++ (Orbeline) 2.0 Release 1.51 gets it wrong
      // and sends out a 0 len string.
      id = new CORBA::Char[1];
      id[0] = (CORBA::Char)'\0';
      break;

    case 1:
      id = new CORBA::Char[1];
      id[0] <<= s;
      if (id[0] != (CORBA::Char)'\0')
	OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_MAYBE);
      idlen = 0;
      break;

    default:
      if (idlen > s.RdMessageUnRead())
	OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_MAYBE);
      id = new CORBA::Char[idlen];
      if (!id) OMNIORB_THROW(NO_MEMORY,0,CORBA::COMPLETED_MAYBE);
      s.get_char_array(id, idlen);
      if (id[idlen - 1] != '\0')
	OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_MAYBE);
      break;
    }

    profiles = new IOP::TaggedProfileList();
    if (!profiles) OMNIORB_THROW(NO_MEMORY,0,CORBA::COMPLETED_MAYBE);
    *profiles <<= s;

    if (profiles->length() == 0 && idlen == 0) {
      // This is a nil object reference
      delete profiles;
      delete[] id;
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
      omniObjRef* objref = omniPy::createObjRef((const char*) id, repoId,
						profiles, 1, 0);
      profiles = 0;
      delete [] id;
      id = 0;

      if (!objref) OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_MAYBE);
      return 
	(CORBA::Object_ptr)objref->_ptrToObjRef(CORBA::Object::_PD_repoId);
    }
  }
  catch (...) {
    if (id)        delete[] id;
    if (profiles)  delete profiles;
    throw;
  }
}
