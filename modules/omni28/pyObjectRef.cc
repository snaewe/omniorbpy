// Versions of ORB object ref functions which deal with Python
// objects, rather than C++ objects

// $Id$

// $Log$
// Revision 1.1  1999/07/29 14:20:05  dpg1
// Initial revision
//


#include <omnipy.h>
#include <omniORB2/ropeFactory.h>
#include <omniORB2/objectManager.h>

#include <iostream.h>


#if defined(HAS_Cplusplus_Namespace)
using omniORB::operator==;
#endif


PyObject*
omniPy::createPyCorbaObjRef(const char*             targetRepoId,
			    const CORBA::Object_ptr objref)
{
  if (CORBA::is_nil(objref)) {
    Py_INCREF(pyCORBAnilObject);
    return pyCORBAnilObject;
  }
  omniObject* oobj = objref->PR_getobj();

  /*
  if (!oobj->is_proxy()) {
    cerr << "Received a non-proxy object. Not yet implemented!" << endl;
    abort();
  }
  */
  const char*    actualRepoId = oobj->NP_IRRepositoryId();

  PyObject*      objrefClass;
  CORBA::Boolean fullTypeUnknown = 0;

  // Try to find objref class for most derived type:
  objrefClass = PyDict_GetItemString(pyomniORBobjrefMap, actualRepoId);

  if (!objrefClass && targetRepoId) {
    // No objref class for the most derived type -- try to find one for
    // the target type:
    objrefClass     = PyDict_GetItemString(pyomniORBobjrefMap, targetRepoId);
    fullTypeUnknown = 1;
  }
  if (!objrefClass) {
    // No target type, or stub code bug:
    objrefClass     = PyObject_GetAttrString(pyCORBAmodule, "Object");
    fullTypeUnknown = 1;
  }

  assert(objrefClass); // Couldn't even find CORBA.Object!

  PyObject* arglist    = PyTuple_New(0);
  PyObject* pyobjref   = PyEval_CallObject(objrefClass, arglist);
  Py_DECREF(arglist);

  assert(PyInstance_Check(pyobjref));

  if (fullTypeUnknown) {
    PyObject* idstr = PyString_FromString(actualRepoId);
    PyDict_SetItemString(((PyInstanceObject*)pyobjref)->in_dict,
			 "_NP_RepositoryId", idstr);
  }

  omniPy::setTwin(pyobjref, (CORBA::Object_ptr)objref);

  cout << "Returning objref..." << endl;

  return pyobjref;
}



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
      cout << "Local Python object received by createObjRef()." << endl;
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
