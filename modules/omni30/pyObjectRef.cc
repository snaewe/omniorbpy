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
#include <exception.h>
#include <ropeFactory.h>
#include <localIdentity.h>
#include <remoteIdentity.h>
#include <objectAdapter.h>


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

  if (!objrefClass &&
      targetRepoId &&
      strcmp(targetRepoId, CORBA::Object::_PD_repoId)) {

    // No objref class for the most derived type -- try to find one for
    // the target type:
    objrefClass     = PyDict_GetItemString(pyomniORBobjrefMap,
					   (char*)targetRepoId);
    fullTypeUnknown = 1;
  }
  if (!objrefClass) {
    // No target type, or stub code bug:
    objrefClass     = PyObject_GetAttrString(pyCORBAmodule, (char*)"Object");
    fullTypeUnknown = 1;
  }

  OMNIORB_ASSERT(objrefClass); // Couldn't even find CORBA.Object!

  PyObject* pyobjref = PyEval_CallObject(objrefClass, omniPy::pyEmptyTuple);

  OMNIORB_ASSERT(objref && PyInstance_Check(pyobjref));

  if (fullTypeUnknown) {
    PyObject* idstr = PyString_FromString(actualRepoId);
    PyDict_SetItemString(((PyInstanceObject*)pyobjref)->in_dict,
			 (char*)"_NP_RepositoryId", idstr);
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

  omniRemoteIdentity* id = new omniRemoteIdentity(rope, key, keysize);

  if (omniORB::trace(10)) {
    omniORB::logger l;
    l << "Creating ref to remote: " << id << "\n"
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
	  objref->_ptrToObjRef("Py_omniObjRef") ) {

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

  if (servant && !servant->_ptrToInterface("Py_omniServant"))
    servant = 0;

  if (servant) {
    remote_id = 0;
    type_verified = 1;
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
omniPy::makeLocalObjRef(const char* targetRepoId, CORBA::Object_ptr objref)
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
  CORBA::release(objref);
  return (CORBA::Object_ptr)newooref->_ptrToObjRef(CORBA::Object::_PD_repoId);
}


int
omniPy::stringToObject(omniObjRef*& objref, const char* sior)
{
  char* repoId;
  IOP::TaggedProfileList* profiles;

  try {
    IOP::EncapStrToIor((const CORBA::Char*) sior,
		       (CORBA::Char*&) repoId,
		       profiles);
  }
  catch(...) {
    return 0;
  }

  if( *repoId == '\0' && profiles->length() == 0 ) {
    // nil object reference
    delete[] repoId;
    delete profiles;
    objref = 0;
    return 1;
  }

  objref = omniPy::createObjRef(repoId, CORBA::Object::_PD_repoId,
				profiles, 1, 0);
  delete[] repoId;

  return objref ? 1 : 0;
}



CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, NetBufferedStream& s)
{
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


CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, MemBufferedStream& s)
{
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
