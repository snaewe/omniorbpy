// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// omnipy30.h                 Created on: 1999/06/07
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
//    Master header file for omnipy internals, omniORB 3.0 version.

// $Id$

// $Log$
// Revision 1.25  2000/06/27 15:13:11  dpg1
// New copyObjRefArgument() function
//
// Revision 1.24  2000/06/12 15:36:08  dpg1
// Support for exception handler functions. Under omniORB 3, local
// operation dispatch modified so exceptions handlers are run.
//
// Revision 1.23  2000/05/11 11:58:25  dpg1
// Throw system exceptions with OMNIORB_THROW.
//
// Revision 1.22  2000/04/27 11:01:52  dpg1
// Interoperable Naming Service
//
// Change to stringToObject() signature.
//
// Revision 1.21  2000/03/24 16:48:57  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.20  2000/03/07 16:52:16  dpg1
// Support for compilers which do not allow exceptions to be caught by
// base class. (Like MSVC 5, surprise surprise.)
//
// Revision 1.19  2000/03/06 18:48:28  dpg1
// Support for our favourite compiler, MSVC.
//
// Revision 1.18  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//
// Revision 1.17  2000/02/04 12:17:11  dpg1
// Support for VMS.
//
// Revision 1.16  1999/12/15 12:17:20  dpg1
// Changes to compile with SunPro CC 5.0.
//
// Revision 1.15  1999/12/02 17:35:57  dpg1
// _narrow, _is_a, _is_equivalent weren't unlocking the interpreter.
//
// Revision 1.14  1999/11/25 11:21:37  dpg1
// Proper support for server-side _is_a().
//
// Revision 1.13  1999/11/16 17:32:36  dpg1
// Changes for AIX.
//
// Revision 1.12  1999/11/08 12:07:25  dpg1
// Adding python1.5/ to #include path caused a conflict with thread.h on
// Solaris.
//
// Revision 1.11  1999/11/08 11:43:35  dpg1
// Changes for NT support.
//
// Revision 1.10  1999/09/29 15:46:50  dpg1
// lockWithNewThreadState now creates a dummy threading.Thread object so
// threading doesn't get upset that it's not there. Very dependent on the
// implementation of threading.py.
//
// Revision 1.9  1999/09/29 11:25:56  dpg1
// Nil objects now map to None. They work too, which is more than can be
// said for the old mapping...
//
// Revision 1.8  1999/09/29 09:05:03  dpg1
// Now releases the Python interpreter lock before invoke's call to
// _is_a().
//
// Revision 1.7  1999/09/28 14:23:30  dpg1
// Fixed some bugs in handling the Python interpreter lock.
//
// Revision 1.6  1999/09/24 09:33:34  dpg1
// Version numbers now come from make file.
//
// Revision 1.5  1999/09/24 09:22:03  dpg1
// Added copyright notices.
//
// Revision 1.4  1999/09/22 15:46:13  dpg1
// Fake POA implemented.
//
// Revision 1.3  1999/09/20 14:53:29  dpg1
// Various fixes. Support for oneway.
//
// Revision 1.2  1999/07/19 15:52:07  dpg1
// Things moved into module omniPy.
// Twin type expanded so it can be put into dictionaries.
//
// Revision 1.1  1999/06/07 10:10:31  dpg1
// Initial revision
//

#ifndef _omnipy30_h_
#define _omnipy30_h_

#if defined(__WIN32__) || defined(__VMS)
#include <Python.h>
#else
#include <python1.5/Python.h>
#endif

#include <omniORB3/CORBA.h>
#include <omniORB3/callDescriptor.h>
#include <exceptiondefs.h>
#include "omnipy30_sysdep.h"

////////////////////////////////////////////////////////////////////////////
// Data structure to manage C++ twins of Python objects                   //
////////////////////////////////////////////////////////////////////////////

extern "C" {
  struct omnipyTwin {
    PyObject_VAR_HEAD
    void* ob_twin;
  };
}

// Twin attribute names
#define ORB_TWIN        (char*)"__omni_orb"           // ORB_ptr
#define OBJREF_TWIN     (char*)"__omni_obj"           // Object_ptr
#define SERVANT_TWIN    (char*)"__omni_svt"           // Py_omniServant
#define POA_TWIN        (char*)"__omni_poa"           // POA_ptr
#define POAMANAGER_TWIN (char*)"__omni_poaManager"    // POAManager_ptr


// Useful macro
#define RAISE_PY_BAD_PARAM_IF(x) \
  if (x) { \
    CORBA::BAD_PARAM _ex; \
    return omniPy::handleSystemException(_ex); \
  }

class omniPy {
public:

  ////////////////////////////////////////////////////////////////////////////
  // The global Python interpreter state                                    //
  ////////////////////////////////////////////////////////////////////////////

  static PyInterpreterState* pyInterpreter;
  static omni_mutex*         pyInterpreterLock;

  ////////////////////////////////////////////////////////////////////////////
  // Global pointers to Python objects                                      //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject* pyCORBAmodule;      // The CORBA module
  static PyObject* pyCORBAsysExcMap;   //  The system exception map
  static PyObject* pyCORBAAnyClass;    //  Any class
  static PyObject* pyomniORBmodule;    // The omniORB module
  static PyObject* pyomniORBobjrefMap; //  The objref class map
  static PyObject* pyomniORBtypeMap;   //  Type map
  static PyObject* pyomniORBwordMap;   //  Reserved word map
  static PyObject* pyPortableServerModule; // Portable server module
  static PyObject* pyServantClass;     // Servant class
  static PyObject* pyCreateTypeCode;   // Function to create a TypeCode object
  static PyObject* pyWorkerThreadClass;// Worker thread class
  static PyObject* pyWorkerThreadDel;  // Method to delete worker thread
  static PyObject* pyEmptyTuple;       // Zero element tuple

  ////////////////////////////////////////////////////////////////////////////
  // Pointer to the ORB                                                     //
  ////////////////////////////////////////////////////////////////////////////

  static CORBA::ORB_ptr orb;

  ////////////////////////////////////////////////////////////////////////////
  // Twin object handling                                                   //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject* newTwin(void* twin);

  static
  inline void
  setTwin(PyObject* obj, void* twin, char* name)
  {
    PyObject* ot = newTwin(twin);

    PyDict_SetItemString(((PyInstanceObject*)obj)->in_dict,
			 name, ot);
    Py_DECREF(ot);
  }

  static
  inline void*
  getTwin(PyObject* obj, char* name)
  {
    PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)obj)->in_dict,
					name);
    if (ot)
      return ((omnipyTwin*)ot)->ob_twin;
    else
      return 0;
  }

  static
  inline void
  remTwin(PyObject* obj, char* name)
  {
    PyDict_DelItemString(((PyInstanceObject*)obj)->in_dict, name);
  }

  ////////////////////////////////////////////////////////////////////////////
  // Module initialisation functions                                        //
  ////////////////////////////////////////////////////////////////////////////

  static void initORBFunc       (PyObject* d);
  static void initPOAFunc       (PyObject* d);
  static void initPOAManagerFunc(PyObject* d);
  static void initomniFunc      (PyObject* d);


  ////////////////////////////////////////////////////////////////////////////
  // Utility functions                                                      //
  ////////////////////////////////////////////////////////////////////////////

  // Set the Python execution state to handle a system exception.
  // Returns a NULL PyObject so you can say
  //   return handleSystemException(ex).
  static
  PyObject* handleSystemException(const CORBA::SystemException& ex);

  // Create a new Python object for the given system exception
  static
  PyObject* createPySystemException(const CORBA::SystemException& ex);

  // Throw a C++ system exception equivalent to the given Python exception
  static
  void produceSystemException(PyObject* eobj, PyObject* erepoId);


  ////////////////////////////////////////////////////////////////////////////
  // Python object creation functions                                       //
  ////////////////////////////////////////////////////////////////////////////

  static
  PyObject* createPyPOAObject(const PortableServer::POA_ptr poa);

  static
  PyObject* createPyPOAManagerObject(const PortableServer::POAManager_ptr pm);


  ////////////////////////////////////////////////////////////////////////////
  // Object reference functions                                             //
  ////////////////////////////////////////////////////////////////////////////

  // Create the Python object relating to a CORBA object reference:
  static
  PyObject* createPyCorbaObjRef(const char* targetRepoId,
				const CORBA::Object_ptr objref);

  static
  PyObject* createPyPseudoObjRef(const CORBA::Object_ptr objref);


  // Functions which mirror omni::createObjRef(). These versions don't
  // look for C++ proxy factories, and spot local Python servants.
  static
  omniObjRef* createObjRef(const char*             mostDerivedRepoId,
			   const char*             targetRepoId,
			   IOP::TaggedProfileList* profiles,
			   CORBA::Boolean          release_profiles,
			   CORBA::Boolean          locked,
			   CORBA::Boolean          type_verified = 0);

  static
  omniObjRef* createObjRef(const char*             mostDerivedRepoId,
			   const char*             targetRepoId,
			   omniLocalIdentity*      local_id,
			   IOP::TaggedProfileList* profiles,
			   CORBA::Boolean          release_profiles,
			   CORBA::Octet*           key,
			   CORBA::Boolean          type_verified = 0);

  // When a POA creates a reference to a Python servant, it does not
  // have a proxy object factory for it, so it creates an
  // omniAnonObjRef. This function converts one of them into a
  // Py_omniObjRef with a reference to the local servant. It
  // decrements the refcount of the original objref.
  static
  CORBA::Object_ptr makeLocalObjRef(const char* targetRepoId,
				    CORBA::Object_ptr objref);

  // Copy a Python object reference in an argument or return value.
  // Compares the type of the objref with the target type, and creates
  // a new objref of the target type if they are not compatible. Sets
  // Python exception status to BAD_PARAM and returns 0 if the Python
  // object is not an object reference.
  static
  PyObject* copyObjRefArgument(PyObject*               pytargetRepoId,
			       PyObject*               pyobjref,
			       CORBA::CompletionStatus compstatus);

  // Mirror of omniURI::stringToObject()
  static
  CORBA::Object_ptr stringToObject(const char* uri);

  // Mirrors of CORBA::UnMarshalObjRef()
  static
  CORBA::Object_ptr UnMarshalObjRef(const char* repoId, NetBufferedStream& s);

  static
  CORBA::Object_ptr UnMarshalObjRef(const char* repoId, MemBufferedStream& s);



  ////////////////////////////////////////////////////////////////////////////
  // Recursive marshalling functions                                        //
  ////////////////////////////////////////////////////////////////////////////
  
  // Calculate aligned size of the given argument object a_o, which
  // has the type specified by d_o. Checks all argument types, and
  // throws BAD_PARAM if they are wrong. If BAD_PARAM is thrown, its
  // completion status is set to the given value.
  static
  CORBA::ULong alignedSize(CORBA::ULong            msgsize,
			   PyObject*               d_o,
			   PyObject*               a_o,
			   CORBA::CompletionStatus compstatus);

  // Check the type and marshal the given argument object a_o, which
  // has the type specified by d_o. These functions MUST NOT be called
  // without having first called alignedSize with the same arguments,
  // since they perform no argument type checking.
  static
  void marshalPyObject(NetBufferedStream& stream,
		       PyObject*          d_o,
		       PyObject*          a_o);
  static
  void marshalPyObject(MemBufferedStream& stream,
		       PyObject*          d_o,
		       PyObject*          a_o);

  // Unmarshal a PyObject, which has the type specified by d_o.
  static
  PyObject* unmarshalPyObject(NetBufferedStream& stream,
			      PyObject*          d_o);
  static
  PyObject* unmarshalPyObject(MemBufferedStream& stream,
			      PyObject*          d_o);

  // Take a descriptor and an argument object, and return a "copy" of
  // the argument. Immutable types need not be copied. If the argument
  // does not match the descriptor, set Python's exception status to
  // BAD_PARAM and return 0.
  static
  PyObject* copyArgument(PyObject*               d_o,
			 PyObject*               a_o,
			 CORBA::CompletionStatus compstatus);


  ////////////////////////////////////////////////////////////////////////////
  // TypeCode and Any support functions                                     //
  ////////////////////////////////////////////////////////////////////////////

  // Marshalled size of a TypeCode given as a descriptor:
  static
  CORBA::ULong alignedSizeTypeCode(CORBA::ULong msgsize, PyObject* d_o);


  // Marshal a type descriptor as a TypeCode:
  static
  void marshalTypeCode(NetBufferedStream& stream, PyObject* d_o);

  static
  void marshalTypeCode(MemBufferedStream& stream, PyObject* d_o);


  // Unmarshal a TypeCode, returning a descriptor:
  static
  PyObject* unmarshalTypeCode(NetBufferedStream& stream);

  static
  PyObject* unmarshalTypeCode(MemBufferedStream& stream);



  ////////////////////////////////////////////////////////////////////////////
  // Proxy call descriptor object                                           //
  ////////////////////////////////////////////////////////////////////////////

  static
  void Py_localCallBackFunction(omniCallDescriptor* cd, omniServant* svnt);

  class Py_omniCallDescriptor : public omniCallDescriptor {
  public:

    inline Py_omniCallDescriptor(const char* op, int op_len,
				 CORBA::Boolean oneway,
				 PyObject* in_d, PyObject* out_d,
				 PyObject* exc_d, PyObject* args)

      : omniCallDescriptor(Py_localCallBackFunction, op, op_len, oneway),
	in_d_(in_d),
	out_d_(out_d),
	exc_d_(exc_d),
	args_(args)
    {
      OMNIORB_ASSERT(PyTuple_Check(in_d));
      tstate_ = 0;
      in_l_   = PyTuple_GET_SIZE(in_d_);
      if (oneway) {
	OMNIORB_ASSERT(out_d_ == Py_None);
	out_l_ = -1;
      }
      else {
	OMNIORB_ASSERT(PyTuple_Check(out_d));
	out_l_ = PyTuple_GET_SIZE(out_d_);
      }
    }

    virtual ~Py_omniCallDescriptor() {
      OMNIORB_ASSERT(!tstate_);
    }

    inline void releaseInterpreterLock() {
      OMNIORB_ASSERT(!tstate_);
      tstate_ = PyEval_SaveThread();
    }

    inline void reacquireInterpreterLock() {
      OMNIORB_ASSERT(tstate_);
      PyEval_RestoreThread(tstate_);
      tstate_ = 0;
    }

    virtual CORBA::ULong alignedSize(CORBA::ULong msgsize);

    virtual void marshalArguments(GIOP_C& giop_client);
    virtual void unmarshalReturnedValues(GIOP_C& giop_client);
    virtual void userException(GIOP_C& giop_client, const char* repoId);

    inline void systemException(const CORBA::SystemException& ex) {
      if (tstate_) {
	PyEval_RestoreThread(tstate_);
	tstate_ = 0;
      }
      handleSystemException(ex);
    }

    inline PyObject* result() { return result_; }

    // These should be private, but MSVC won't let me declare
    // Py_localCallBackFunction to be a friend :-(
  public:
    PyObject*      in_d_;
    int            in_l_;
    PyObject*      out_d_;
    int            out_l_;
    PyObject*      exc_d_;
    PyObject*      args_;
    PyObject*      result_;

  private:
    PyThreadState* tstate_;

    Py_omniCallDescriptor(const Py_omniCallDescriptor&);
    Py_omniCallDescriptor& operator=(const Py_omniCallDescriptor&);
  };


  ////////////////////////////////////////////////////////////////////////////
  // Servant object                                                         //
  ////////////////////////////////////////////////////////////////////////////

  class Py_omniServant : public virtual PortableServer::RefCountServantBase {

  public:

    Py_omniServant(PyObject* pyservant, PyObject* opdict, const char* repoId);

    virtual ~Py_omniServant();

    virtual CORBA::Boolean _dispatch(GIOP_S& giop_s);

    PyObject* local_dispatch(const char* op,
			     PyObject*   in_d,  int in_l,
			     PyObject*   out_d, int out_l,
			     PyObject*   exc_d,
			     PyObject*   args);

    PyObject* py_this();

    virtual void*                   _ptrToInterface(const char* repoId);
    virtual const char*             _mostDerivedRepoId();
    virtual CORBA::Boolean          _is_a(const char* logical_type_id);
    virtual PortableServer::POA_ptr _default_POA();

    inline PyObject* pyServant() { Py_INCREF(pyservant_); return pyservant_; }

  private:

    PyObject* pyservant_;	// Python servant object
    PyObject* opdict_;		// Operation descriptor dictionary
    PyObject* pyskeleton_;	// Skeleton class object
    char*     repoId_;

    // Not implemented:
    Py_omniServant(const Py_omniServant&);
    Py_omniServant& operator=(const Py_omniServant&);
  };

  // Function to find or create a Py_omniServant object for a Python
  // servant object. If the Python object is not an instance of a
  // class derived from PortableServer.Servant, returns 0.
  static Py_omniServant* getServantForPyObject(PyObject* pyservant);


  ////////////////////////////////////////////////////////////////////////////
  // InterpreterUnlocker releases the Python interpreter lock               //
  ////////////////////////////////////////////////////////////////////////////

  class InterpreterUnlocker {
  public:
    InterpreterUnlocker() {
      tstate_ = PyEval_SaveThread();
    }
    ~InterpreterUnlocker() {
      PyEval_RestoreThread(tstate_);
    }
  private:
    PyThreadState* tstate_;
  };

  ////////////////////////////////////////////////////////////////////////////
  // UserExceptionHandled is thrown when we've handled a user exception     //
  ////////////////////////////////////////////////////////////////////////////

  class UserExceptionHandled {
  public:
    UserExceptionHandled() { }
    ~UserExceptionHandled() { }
  };

};

#endif // _omnipy30_h_
