// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// omnipy.h                   Created on: 1999/06/07
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
//    Master header file for omnipy internals


// $Id$

// $Log$
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

#ifndef _omnipy_h_
#define _omnipy_h_

#ifdef __WIN32__
#include <Python.h>
#else
#include <python1.5/Python.h>
#endif

#include <omniORB2/CORBA.h>
#include <omniORB2/proxyCall.h>

////////////////////////////////////////////////////////////////////////////
// Data structures and functions to manage C++ twins of Python objects    //
////////////////////////////////////////////////////////////////////////////

extern "C" {

  struct omnipyTwin {
    PyObject_VAR_HEAD
    void* ob_twin;
  };

  static void
  omnipyTwin_dealloc(omnipyTwin* tp)
  {
    PyMem_DEL(tp);
  }

  static int
  omnipyTwin_cmp(omnipyTwin* t1, omnipyTwin* t2)
  {
    // Can't use simple t1->ob_twin - t2->ob_twin in case int is
    // smaller than a pointer
    if      (t1->ob_twin == t2->ob_twin) return 0;
    else if (t1->ob_twin >  t2->ob_twin) return 1;
    else                                 return -1;
  }

  static long
  omnipyTwin_hash(omnipyTwin* tp)
  {
    return (long)tp->ob_twin;
  }

  static PyTypeObject omnipyTwinType = {
    PyObject_HEAD_INIT(&PyType_Type)
      0,
      "omnipyTwin",
      sizeof(omnipyTwin),
      0,
      (destructor)omnipyTwin_dealloc,   /*tp_dealloc*/
      0,			        /*tp_print*/
      0,                                /*tp_getattr*/
      0,				/*tp_setattr*/
      (cmpfunc)omnipyTwin_cmp,          /*tp_compare*/
      0,                                /*tp_repr*/
      0,				/*tp_as_number*/
      0,                                /*tp_as_sequence*/
      0,				/*tp_as_mapping*/
      (hashfunc)omnipyTwin_hash,	/*tp_hash*/
      0,				/*tp_call*/
      0,				/*tp_str*/
      0,				/*tp_getattro*/
      0,				/*tp_setattro*/
      0,                                /*tp_as_buffer*/
      0,				/*tp_xxx4*/
      0,                                /*tp_doc*/
      };
}


_CORBA_MODULE omniPy
_CORBA_MODULE_BEG

_CORBA_MODULE_FN
inline PyObject*
newTwin(void* twin)
{
  omnipyTwin* ot = PyMem_NEW(omnipyTwin, 1);
  ot->ob_type = &omnipyTwinType;
  ot->ob_size = 0;
  ot->ob_twin = (void*)twin;
  _Py_NewReference(ot);
  return (PyObject*)ot;
}


_CORBA_MODULE_FN
inline void
setTwin(PyObject* obj, void* twin, char* name)
{
  PyObject* ot = newTwin(twin);

  PyDict_SetItemString(((PyInstanceObject*)obj)->in_dict,
		       name, ot);
  Py_DECREF(ot);
}

_CORBA_MODULE_FN
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

_CORBA_MODULE_FN
inline void
remTwin(PyObject* obj, char* name)
{
  PyDict_DelItemString(((PyInstanceObject*)obj)->in_dict, name);
}

#define ORB_TWIN    "__twin_orb"
#define BOA_TWIN    "__twin_boa"
#define OBJREF_TWIN "__twin_obj"



////////////////////////////////////////////////////////////////////////////
// The global Python interpreter state                                    //
////////////////////////////////////////////////////////////////////////////

_CORBA_MODULE_VAR PyInterpreterState* pyInterpreter;
_CORBA_MODULE_VAR omni_mutex*         pyInterpreterLock;

////////////////////////////////////////////////////////////////////////////
// Global pointers to Python objects                                      //
////////////////////////////////////////////////////////////////////////////

_CORBA_MODULE_VAR PyObject* pyCORBAmodule;	// The CORBA module
_CORBA_MODULE_VAR PyObject* pyCORBAsysExcMap;	//  The system exception map
_CORBA_MODULE_VAR PyObject* pyCORBAAnyClass;    //  Any class
_CORBA_MODULE_VAR PyObject* pyomniORBmodule;	// The omniORB module
_CORBA_MODULE_VAR PyObject* pyomniORBobjrefMap;	//  The objref class map
_CORBA_MODULE_VAR PyObject* pyomniORBtypeMap;   //  Type map
_CORBA_MODULE_VAR PyObject* pyomniORBwordMap;   //  Reserved word map
_CORBA_MODULE_VAR PyObject* pyCreateTypeCode;	// Function to create a
						//  TypeCode object
_CORBA_MODULE_VAR PyObject* pyDummyThreadClass; // threading module dummy
                                                //  thread class
_CORBA_MODULE_VAR PyObject* pyEmptyTuple;       // Zero element tuple


////////////////////////////////////////////////////////////////////////////
// Utility functions                                                      //
////////////////////////////////////////////////////////////////////////////

// Set the Python execution state to handle a system exception:
_CORBA_MODULE_FN
void handleSystemException(const CORBA::SystemException& ex);

// Throw a system exception equivalent to the given Python exception
_CORBA_MODULE_FN
void produceSystemException(PyObject* eobj, PyObject* erepoId);


////////////////////////////////////////////////////////////////////////////
// Object reference functions                                             //
////////////////////////////////////////////////////////////////////////////

// Create the Python object relating to a CORBA object reference:
_CORBA_MODULE_FN
PyObject* createPyCorbaObjRef(const char* targetRepoId,
			      const CORBA::Object_ptr obj);

_CORBA_MODULE_FN
omniObject* createObjRef(const char* mostDerivedRepoId,
			 const char* targetRepoId,
			 IOP::TaggedProfileList* profiles,
			 CORBA::Boolean release);

_CORBA_MODULE_FN
omniObject* stringToObject(const char* str);

_CORBA_MODULE_FN
CORBA::Object_ptr UnMarshalObjRef(const char* repoId, NetBufferedStream& s);

_CORBA_MODULE_FN
CORBA::Object_ptr UnMarshalObjRef(const char* repoId, MemBufferedStream& s);



////////////////////////////////////////////////////////////////////////////
// Recursive marshalling functions                                        //
////////////////////////////////////////////////////////////////////////////

// Calculate aligned size of the given argument object a_o, which has
// the type specified by d_o. Assumes that argument types are correct,
// unless checking is necessary.
_CORBA_MODULE_FN
CORBA::ULong alignedSize(CORBA::ULong msgsize,
			 PyObject* d_o,
			 PyObject* a_o);

// Check the type and marshal the given argument object a_o, which
// has the type specified by d_o.
_CORBA_MODULE_FN
void marshalPyObject(NetBufferedStream& stream,
		     PyObject*          d_o,
		     PyObject*          a_o);

_CORBA_MODULE_FN
void marshalPyObject(MemBufferedStream& stream,
		     PyObject*          d_o,
		     PyObject*          a_o);

// Unmarshal a PyObject, which has the type specified by d_o.
_CORBA_MODULE_FN
PyObject* unmarshalPyObject(NetBufferedStream& stream,
			    PyObject*          d_o);

_CORBA_MODULE_FN
PyObject* unmarshalPyObject(MemBufferedStream& stream,
			    PyObject*          d_o);


////////////////////////////////////////////////////////////////////////////
// TypeCode and Any support functions                                     //
////////////////////////////////////////////////////////////////////////////

// Marshalled size of a TypeCode given as a descriptor:
_CORBA_MODULE_FN
CORBA::ULong alignedSizeTypeCode(CORBA::ULong msgsize, PyObject* d_o);


// Marshal a type descriptor as a TypeCode:
_CORBA_MODULE_FN
void marshalTypeCode(NetBufferedStream& stream, PyObject* d_o);

_CORBA_MODULE_FN
void marshalTypeCode(MemBufferedStream& stream, PyObject* d_o);


// Unmarshal a TypeCode, returning a descriptor:
_CORBA_MODULE_FN
PyObject* unmarshalTypeCode(NetBufferedStream& stream);

_CORBA_MODULE_FN
PyObject* unmarshalTypeCode(MemBufferedStream& stream);



////////////////////////////////////////////////////////////////////////////
// Proxy call descriptor object                                           //
////////////////////////////////////////////////////////////////////////////

class Py_OmniProxyCallDesc : public OmniProxyCallDesc {
public:

  Py_OmniProxyCallDesc(const char* op, size_t len, CORBA::Boolean has_exc,
		       PyObject* in_d, PyObject* out_d, PyObject* exc_d,
		       PyObject* args)
    : OmniProxyCallDesc(op, len, has_exc),
      in_d_(in_d),
      out_d_(out_d),
      exc_d_(exc_d),
      args_(args)
  {
    assert(PyTuple_Check(in_d));
    assert(PyTuple_Check(out_d));
    tstate_ = 0;
    in_l_   = PyTuple_GET_SIZE(in_d_);
    out_l_  = PyTuple_GET_SIZE(out_d_);
    //    cout << "Py_OmniProxyCallDesc created: " << op << " " << len << endl;
  }

  virtual ~Py_OmniProxyCallDesc() {
    assert(!tstate_);
  }

  inline void releaseInterpreterLock() {
    assert(!tstate_);
    tstate_ = PyEval_SaveThread();
  }

  inline void reacquireInterpreterLock() {
    assert(tstate_);
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

private:
  PyObject*      in_d_;
  int            in_l_;
  PyObject*      out_d_;
  int            out_l_;
  PyObject*      exc_d_;
  PyObject*      args_;
  PyObject*      result_;
  PyThreadState* tstate_;

  Py_OmniProxyCallDesc(const Py_OmniProxyCallDesc&);
  Py_OmniProxyCallDesc& operator=(const Py_OmniProxyCallDesc&);
};


// Version for oneway:

class Py_OmniOWProxyCallDesc : public OmniOWProxyCallDesc {
public:

  Py_OmniOWProxyCallDesc(const char* op, size_t len,
		       PyObject* in_d, PyObject* args)
    : OmniOWProxyCallDesc(op, len),
      in_d_(in_d),
      args_(args)
  {
    tstate_ = 0;
    assert(PyTuple_Check(in_d));
    in_l_   = PyTuple_GET_SIZE(in_d_);
    //    cout << "Py_OmniProxyCallDesc created: " << op << " " << len << endl;
  }

  virtual ~Py_OmniOWProxyCallDesc() {
    assert(!tstate_);
  }

  inline void releaseInterpreterLock() {
    tstate_ = PyEval_SaveThread();
  }

  inline void reacquireInterpreterLock() {
    assert(tstate_);
    PyEval_RestoreThread(tstate_);
    tstate_ = 0;
  }

  virtual CORBA::ULong alignedSize(CORBA::ULong msgsize);

  virtual void marshalArguments(GIOP_C& giop_client);

  inline void systemException(const CORBA::SystemException& ex) {
    if (tstate_) {
      PyEval_RestoreThread(tstate_);
      tstate_ = 0;
    }
    handleSystemException(ex);
  }

private:
  PyObject*      in_d_;
  int            in_l_;
  PyObject*      args_;
  PyThreadState* tstate_;

  Py_OmniOWProxyCallDesc(const Py_OmniOWProxyCallDesc&);
  Py_OmniOWProxyCallDesc& operator=(const Py_OmniOWProxyCallDesc&);
};



////////////////////////////////////////////////////////////////////////////
// Servant object                                                         //
////////////////////////////////////////////////////////////////////////////

class Py_Servant : public virtual omniObject, public virtual CORBA::Object {

public:

  Py_Servant(PyObject* pyservant, PyObject* opdict, const char* repoId);

  virtual ~Py_Servant();

  omniORB::objectKey _key();

  virtual CORBA::Boolean dispatch(GIOP_S&        giop_server,
				  const char*    op,
				  CORBA::Boolean response);

  inline PyObject* local_dispatch(const char* op, PyObject* args) {
    PyObject* method = PyObject_GetAttrString(pyservant_, (char*)op);
    if (method) {
      PyObject* ret = PyEval_CallObject(method, args);
      Py_DECREF(method);
      return ret;
    }
    else {
      PyObject* err =
	PyString_FromString("Local servant has no operation named ");

      PyString_ConcatAndDel(&err, PyString_FromString(op));
      PyErr_SetObject(PyExc_AttributeError, err);
      Py_DECREF(err);
      return 0;
    }
  }

  inline PyObject* pyServant() {
    Py_INCREF(pyservant_);
    return pyservant_;
  }

private:
  PyObject* pyservant_;		// Python servant object
  PyObject* opdict_;		// Operation descriptor dictionary

  Py_Servant(const Py_Servant&);
  Py_Servant& operator=(const Py_Servant&);

protected:
  virtual void* _widenFromTheMostDerivedIntf(const char* repoId,
					     CORBA::Boolean is_cxx_type_id);
};




_CORBA_MODULE_END

#endif // _omnipy_h_
