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
// Revision 1.13  2000/06/27 15:13:11  dpg1
// New copyObjRefArgument() function
//
// Revision 1.12  2000/06/16 17:33:18  dpg1
// When creating an object reference with target type A, when the object
// claims to be B, but A and B are unrelated, now creates a reference of
// type A, where before it would be B.
//
// Revision 1.11  2000/03/24 16:48:58  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.10  2000/03/03 17:41:43  dpg1
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
#include <ropeFactory.h>   // Internal omniORB interface
#include <objectManager.h> // Internal omniORB interface


#if defined(HAS_Cplusplus_Namespace)
using omniORB::operator==;
#endif


class PyProxyObject : public virtual omniObject,
		      public virtual CORBA::Object 
{
public:
  PyProxyObject(const char* repoId,
		Rope* r,
		CORBA::Octet* key,
		size_t keysize,
		IOP::TaggedProfileList* profiles,
		CORBA::Boolean release) :
    omniObject(repoId,r,key,keysize,profiles,release) 
  {
    this->PR_setobj(this);
    // We use PR_IRRepositoryId() to indicate that we don't really know
    // anything about the interface type. Any subsequent queries about its
    // type, such as in interface narrowing, must be answered by
    // the object itself.
    PR_IRRepositoryId(repoId);
    omni::objectIsReady(this);
  }
  virtual ~PyProxyObject() {}
  
protected:
  virtual void* _widenFromTheMostDerivedIntf(const char* repoId,
					     CORBA::Boolean is_cxx_type_id);

private:
  PyProxyObject();
  PyProxyObject (const PyProxyObject&);
  PyProxyObject &operator=(const PyProxyObject&);
};

void*
PyProxyObject::_widenFromTheMostDerivedIntf(const char* repoId,
					    CORBA::Boolean is_cxx_type_id)
{
  if (is_cxx_type_id)
    return 0;
  if (!repoId)
    return (void*)((CORBA::Object_ptr)this);
  else
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
  omniObject*    oobj = objref->PR_getobj();

  const char*    actualRepoId = oobj->NP_IRRepositoryId();
  PyObject*      objrefClass;
  CORBA::Boolean fullTypeUnknown = 0;

  // Try to find objref class for most derived type:
  objrefClass = PyDict_GetItemString(pyomniORBobjrefMap, (char*)actualRepoId);

  if (targetRepoId &&
      strcmp(targetRepoId, actualRepoId)) {

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

  //  cout << "Returning objref..." << endl;

  return pyobjref;
}





omniObject*
omniPy::createObjRef(const char* mostDerivedRepoId,
		     const char* targetRepoId,
		     IOP::TaggedProfileList* profiles,
		     CORBA::Boolean release)
{
  CORBA::Octet *objkey = 0;

  size_t ksize = 0;

  omniObject* localobj;

  Rope_var rope;
  localobj = ropeFactory::iopProfilesToRope(profiles,objkey,ksize,rope);

  if (localobj) {
    // The reference was to a local C++ object. Get the rope of the
    // local loop-back interface, and make a proxy to it.
    rope = localobj->_objectManager()->defaultLoopBack();
    rope->incrRefCount();
    omni::objectRelease(localobj);
    localobj = 0;
  }
  else {
    // Check to see if the object is a local Python object
    localobj = omni::locatePyObject(omniObjectManager::root(1),
				    *((omniObjectKey*)objkey));
  }

  try {
    if (!localobj) {
      // Create a proxy object
      if (release) {
	CORBA::Object_ptr objptr;

	if (targetRepoId == 0) {
	  // The target is just the pseudo object CORBA::Object. Make
	  // a proxy object with the object's claimed repoId.
	  objptr =  new PyProxyObject(mostDerivedRepoId,rope,
				      objkey,ksize,profiles,1);
	}
	else {
	  objptr = new PyProxyObject(targetRepoId,rope,
				     objkey,ksize,profiles,1);

	  // The ctor of the proxy object sets its IR repository ID
	  // to <targetRepoId>, we reset it to <mostDerivedRepoId> because
	  // this identifies the true type of the object.
	  objptr->PR_getobj()->PR_IRRepositoryId(mostDerivedRepoId);
	}
	rope._ptr = 0;
	return objptr->PR_getobj();
      }
      else {
	IOP::TaggedProfileList *localcopy = 
	  new IOP::TaggedProfileList(*profiles);
	if (!localcopy) {
	  throw CORBA::NO_MEMORY(0,CORBA::COMPLETED_NO);
	}
	try {
	  CORBA::Object_ptr objptr;
	  if (targetRepoId == 0) {
	    // The target is just the pseudo object CORBA::Object.
	    // Make a proxy object with the object's claimed repoId.
	    objptr =  new PyProxyObject(mostDerivedRepoId,rope,
					objkey,ksize,localcopy,1);
	  }
	  else {
	    objptr = new PyProxyObject(targetRepoId,rope,
				       objkey,ksize,localcopy,1);
	    // The ctor of the proxy object sets its IR repository ID
	    // to <targetRepoId>, we reset it to <mostDerivedRepoId> because
	    // this identifies the true type of the object.
	    objptr->PR_getobj()->PR_IRRepositoryId(mostDerivedRepoId);
	  }
	  rope._ptr = 0;
	  return objptr->PR_getobj();
	}
	catch (...) {
	  delete localcopy;
	  throw;
	}
      }
    }
    else {
      //      cout << "Local Python object received by createObjRef()." << endl;
      delete [] objkey;
      if (release)
	delete profiles;
      return localobj;

      abort();
      // A local object
      if (targetRepoId && !localobj->_real_is_a(targetRepoId)) {
	// According to the local object, it is neither the exact interface
	// nor a derived interface identified by <targetRepoId>
	omni::objectRelease(localobj);
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_NO);
      }
      delete [] objkey;
      if (release)
	delete profiles;
      return localobj;
    }
  }
  catch (...) {
    if (objkey) delete [] objkey;
    throw;
  }
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
  omniObject* ooref        = objref->PR_getobj();
  const char* actualRepoId = ooref->NP_IRRepositoryId();
  const char* targetRepoId = PyString_AS_STRING(pytargetRepoId);

  if (targetRepoId[0] == '\0') targetRepoId = 0;

  omniObject* newooref     = omniPy::createObjRef(actualRepoId,
						  targetRepoId,
						  ooref->iopProfiles(), 0);
  return createPyCorbaObjRef(targetRepoId,
			     (CORBA::Object_ptr)newooref->
			                     _widenFromTheMostDerivedIntf(0));
}




omniObject*
omniPy::stringToObject(const char* str)
{
  char* repoId;
  IOP::TaggedProfileList* profiles;

  IOP::EncapStrToIor((const CORBA::Char*)str, (CORBA::Char*&)repoId, profiles);
  if (*repoId == '\0' && profiles->length() == 0) {
    // nil object reference
    delete [] repoId;
    delete profiles;
    return 0;
  }

  try {
    omniObject* newobj = omniPy::createObjRef(repoId,0,profiles,1);
    delete [] repoId;
    return newobj;
  }
  catch (...) {
    delete [] repoId;	
    delete profiles;
    throw;
  }
}


extern void
omniPy_objectIsReady(omniObject* obj)
{
  omniObject::objectTableLock.lock();
  if (obj->getRefCount() != 0) {
    omniObject::objectTableLock.unlock();
    throw CORBA::INV_OBJREF(0,CORBA::COMPLETED_NO);
  }
    
  if (obj->is_proxy())
    {
      obj->pd_next = omniObject::proxyObjectTable;
      omniObject::proxyObjectTable = obj;
    }
  else
    {
      omniObject **p = &omniObject::localPyObjectTable[omniORB::hash(obj->pd_objkey.native)];
      omniObject **pp = p;
      while (*p) {
	if ((*p)->pd_objkey.native == obj->pd_objkey.native) {
	  obj->pd_next = 0;
	  omniObject::objectTableLock.unlock();
	  throw CORBA::INV_OBJREF(0,CORBA::COMPLETED_NO);
	}
	p = &((*p)->pd_next);
      }
      obj->pd_next = (*pp);
      *pp = obj;
    }
  obj->setRefCount(obj->getRefCount()+1);
  omniObject::objectTableLock.unlock();
  return;
}



CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, NetBufferedStream& s)
{
  CORBA::ULong idlen;
  CORBA::Char* id = 0;
  IOP::TaggedProfileList* profiles = 0;

  try {
    idlen <<= s;

    switch (idlen) {

    case 0:
#ifdef NO_SLOPPY_NIL_REFERENCE
      throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
#else
      // According to the CORBA specification 2.0 section 10.6.2:
      //   Null object references are indicated by an empty set of
      //   profiles, and by a NULL type ID (a string which contain
      //   only *** a single terminating character ***).
      //
      // Therefore the idlen should be 1.
      // Visibroker for C++ (Orbeline) 2.0 Release 1.51 gets it wrong
      // and sends out a 0 len string.
      // We quietly accept it here. Turn this off by defining
      //   NO_SLOPPY_NIL_REFERENCE
      id = new CORBA::Char[1];
      id[0] = (CORBA::Char)'\0';
#endif	
      break;

    case 1:
      id = new CORBA::Char[1];
      id[0] <<= s;
      if (id[0] != (CORBA::Char)'\0')
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
      idlen = 0;
      break;

    default:
      if (idlen > s.RdMessageUnRead())
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
      id = new CORBA::Char[idlen];
      if( !id )  throw CORBA::NO_MEMORY(0,CORBA::COMPLETED_MAYBE);
      s.get_char_array(id, idlen);
      if( id[idlen - 1] != '\0' )
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
      break;
    }

    profiles = new IOP::TaggedProfileList();
    if( !profiles )  throw CORBA::NO_MEMORY(0,CORBA::COMPLETED_MAYBE);
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
      omniObject* objptr = omniPy::createObjRef((const char*) id, repoId,
						profiles, 1);
      profiles = 0;
      delete [] id;
      id = 0;
      return (CORBA::Object_ptr)(objptr->_widenFromTheMostDerivedIntf(0));
    }
  }
  catch (...) {
    if( id )        delete[] id;
    if( profiles )  delete profiles;
    throw;
  }
}


CORBA::Object_ptr
omniPy::UnMarshalObjRef(const char* repoId, MemBufferedStream& s)
{
  CORBA::ULong idlen;
  CORBA::Char* id = 0;
  IOP::TaggedProfileList* profiles = 0;

  try {
    idlen <<= s;

    switch (idlen) {

    case 0:
#ifdef NO_SLOPPY_NIL_REFERENCE
      throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
#else
      // According to the CORBA specification 2.0 section 10.6.2:
      //   Null object references are indicated by an empty set of
      //   profiles, and by a NULL type ID (a string which contain
      //   only *** a single terminating character ***).
      //
      // Therefore the idlen should be 1.
      // Visibroker for C++ (Orbeline) 2.0 Release 1.51 gets it wrong
      // and sends out a 0 len string.
      // We quietly accept it here. Turn this off by defining
      //   NO_SLOPPY_NIL_REFERENCE
      id = new CORBA::Char[1];
      id[0] = (CORBA::Char)'\0';
#endif	
      break;

    case 1:
      id = new CORBA::Char[1];
      id[0] <<= s;
      if (id[0] != (CORBA::Char)'\0')
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
      idlen = 0;
      break;

    default:
      if (idlen > s.RdMessageUnRead())
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
      id = new CORBA::Char[idlen];
      if( !id )  throw CORBA::NO_MEMORY(0,CORBA::COMPLETED_MAYBE);
      s.get_char_array(id, idlen);
      if( id[idlen - 1] != '\0' )
	throw CORBA::MARSHAL(0,CORBA::COMPLETED_MAYBE);
      break;
    }

    profiles = new IOP::TaggedProfileList();
    if( !profiles )  throw CORBA::NO_MEMORY(0,CORBA::COMPLETED_MAYBE);
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
      omniObject* objptr = omniPy::createObjRef((const char*) id, repoId,
						profiles, 1);
      profiles = 0;
      delete [] id;
      id = 0;
      return (CORBA::Object_ptr)(objptr->_widenFromTheMostDerivedIntf(0));
    }
  }
  catch (...) {
    if( id )        delete[] id;
    if( profiles )  delete profiles;
    throw;
  }
}
