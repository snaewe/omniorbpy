// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// omnipy.cc                  Created on: 1999/06/01
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
//    Main entry points for _omnipy Python module

#define OMNIPY_MAJOR 0
#define OMNIPY_MINOR 2

// $Id$

// $Log$
// Revision 1.26  1999/12/14 15:28:40  dpg1
// Catch ORB::InvalidName exception in resolve_initial_references().
//
// Revision 1.25  1999/12/02 17:35:57  dpg1
// _narrow, _is_a, _is_equivalent weren't unlocking the interpreter.
//
// Revision 1.24  1999/11/25 11:49:32  dpg1
// Minor version number bumped since server-side _is_a() required an
// incompatible change.
//
// Revision 1.23  1999/11/16 17:32:36  dpg1
// Changes for AIX.
//
// Revision 1.22  1999/11/09 17:27:34  dpg1
// narrow() now properly handles CORBA system exceptions.
//
// Revision 1.21  1999/11/08 11:43:35  dpg1
// Changes for NT support.
//
// Revision 1.20  1999/10/15 17:07:23  dpg1
// Narrow now properly returns nil if the object is not of the right
// type.
//
// Revision 1.19  1999/10/15 16:25:46  dpg1
// Fixed refcounting bug with Python servants.
//
// Revision 1.18  1999/10/01 11:07:21  dpg1
// Error reporting if up-call raises an unexpected exception.
//
// Revision 1.17  1999/09/29 15:46:51  dpg1
// lockWithNewThreadState now creates a dummy threading.Thread object so
// threading doesn't get upset that it's not there. Very dependent on the
// implementation of threading.py.
//
// Revision 1.16  1999/09/29 11:25:56  dpg1
// Nil objects now map to None. They work too, which is more than can be
// said for the old mapping...
//
// Revision 1.15  1999/09/29 09:05:03  dpg1
// Now releases the Python interpreter lock before invoke's call to
// _is_a().
//
// Revision 1.14  1999/09/28 16:19:41  dpg1
// Small memory management issues fixed.
//
// Revision 1.13  1999/09/27 09:20:49  dpg1
// Fixed bug in stringToObject() exception handling.
//
// Revision 1.12  1999/09/24 09:22:04  dpg1
// Added copyright notices.
//
// Revision 1.11  1999/09/23 16:29:45  dpg1
// Forgot to update initial references code with twin changes,
//
// Revision 1.10  1999/09/23 16:27:45  dpg1
// Initial references support was accidentally missed out. Replaced.
//
// Revision 1.9  1999/09/22 15:46:13  dpg1
// Fake POA implemented.
//
// Revision 1.8  1999/07/29 14:18:48  dpg1
// Server side.
//
// Revision 1.7  1999/07/19 15:50:38  dpg1
// Things put into module omniPy. Small fixes.
//
// Revision 1.6  1999/06/07 14:58:45  dpg1
// Descriptors unflattened again.
//
// Revision 1.5  1999/06/07 10:11:18  dpg1
// Split into separate files.
//
// Revision 1.4  1999/06/04 16:34:29  dpg1
// Descriptors flattened.
//
// Revision 1.3  1999/06/04 13:59:54  dpg1
// Structs, enums and sequences.
//
// Revision 1.2  1999/06/03 10:36:51  dpg1
// Different proxy scheme.
// Thread control.
// All simple types.
//
// Revision 1.1  1999/06/01 11:44:49  dpg1
// Initial revision
//

#ifdef __WIN32__
#define DLL_EXPORT _declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include <omnipy.h>


////////////////////////////////////////////////////////////////////////////
// Global pointers to Python objects                                      //
////////////////////////////////////////////////////////////////////////////

#if defined(HAS_Cplusplus_Namespace) && defined(_MSC_VER)
// MSVC++ does not give the variables external linkage otherwise. It's a bug.
namespace omniPy {

PyInterpreterState* pyInterpreter;
omni_mutex*         pyInterpreterLock;

PyObject* pyCORBAmodule;	// The CORBA module
PyObject* pyCORBAsysExcMap;	//  The system exception map
PyObject* pyCORBAAnyClass;	//  Any class
PyObject* pyomniORBmodule;	// The omniORB module
PyObject* pyomniORBobjrefMap;	//  The objref class map
PyObject* pyomniORBtypeMap;     //  The repoId to descriptor mapping
PyObject* pyomniORBwordMap;     //  Reserved word map
PyObject* pyCreateTypeCode;	// Function to create a TypeCode object
PyObject* pyDummyThreadClass;   // threading module dummy thread class
PyObject* pyEmptyTuple;         // Zero element tuple
}

#else
PyInterpreterState* omniPy::pyInterpreter;
omni_mutex*         omniPy::pyInterpreterLock;

PyObject* omniPy::pyCORBAmodule;	// The CORBA module
PyObject* omniPy::pyCORBAsysExcMap;	//  The system exception map
PyObject* omniPy::pyCORBAAnyClass;	//  Any class
PyObject* omniPy::pyomniORBmodule;	// The omniORB module
PyObject* omniPy::pyomniORBobjrefMap;	//  The objref class map
PyObject* omniPy::pyomniORBtypeMap;     //  The repoId to descriptor mapping
PyObject* omniPy::pyomniORBwordMap;     //  Reserved word map
PyObject* omniPy::pyCreateTypeCode;	// Function to create a TypeCode object
PyObject* omniPy::pyDummyThreadClass;   // threading module dummy
                                        //  thread class
PyObject* omniPy::pyEmptyTuple;         // Zero element tuple.
#endif


// Things visible to Python:

extern "C" {

  ////////////////////////////////////////////////////////////////////////////
  // omnipy private functions                                               //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_checkVersion(PyObject* self, PyObject* args)
  {
    int   maj, min;
    char* mod;

    if (!PyArg_ParseTuple(args, "iis", &maj, &min, &mod))
      return NULL;

    if (maj != OMNIPY_MAJOR || (maj == 0 && min != OMNIPY_MINOR)) {
      omniORB::log << "omniORBpy: ***\n"
		   << "omniORBpy: *** WARNING! _omnipy module version "
		   << OMNIPY_MAJOR << "." << OMNIPY_MINOR << "\n"
		   << "omniORBpy: *** Stubs in " << mod << " are version "
		   << maj << "." << min << "\n"
		   << "omniORBpy: *** You may experience strange errors "
		   << "until you recompile your IDL\n";
      omniORB::log.flush();
    }
    Py_INCREF(Py_None);
    return Py_None;
  }


  static PyObject*
  omnipy_registerPyObjects(PyObject* self, PyObject* args)
  {
    PyObject* temp;
    PyObject* pythreading;

    // Get a pointer to the interpreter state
    PyThreadState* tstate = PyThreadState_Get();
    omniPy::pyInterpreter = tstate->interp;

    if (!PyArg_ParseTuple(args, "OO", &omniPy::pyomniORBmodule, &pythreading))
      return NULL;

    assert(PyModule_Check(omniPy::pyomniORBmodule));
    assert(PyModule_Check(pythreading));

    omniPy::pyCORBAmodule =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, "CORBA");

    assert(omniPy::pyCORBAmodule && PyModule_Check(omniPy::pyCORBAmodule));

    omniPy::pyCORBAsysExcMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, "sysExceptionMapping");

    temp =
      PyObject_GetAttrString(omniPy::pyCORBAmodule, "Object");

    omniPy::pyCORBAAnyClass =
      PyObject_GetAttrString(omniPy::pyCORBAmodule, "Any");

    omniPy::pyomniORBobjrefMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, "objrefMapping");

    omniPy::pyomniORBtypeMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, "typeMapping");

    omniPy::pyomniORBwordMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, "keywordMapping");

    temp =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, "tcInternal");

    omniPy::pyCreateTypeCode = PyObject_GetAttrString(temp, "createTypeCode");

    omniPy::pyDummyThreadClass = PyObject_GetAttrString(pythreading,
							"_DummyThread");
    omniPy::pyEmptyTuple = PyTuple_New(0);

    assert(omniPy::pyCORBAsysExcMap);
    assert(PyDict_Check(omniPy::pyCORBAsysExcMap));
    assert(omniPy::pyCORBAAnyClass);
    assert(PyClass_Check(omniPy::pyCORBAAnyClass));
    assert(omniPy::pyomniORBobjrefMap);
    assert(PyDict_Check(omniPy::pyomniORBobjrefMap));
    assert(omniPy::pyomniORBtypeMap);
    assert(PyDict_Check(omniPy::pyomniORBtypeMap));
    assert(omniPy::pyomniORBwordMap);
    assert(PyDict_Check(omniPy::pyomniORBwordMap));
    assert(omniPy::pyCreateTypeCode);
    assert(PyFunction_Check(omniPy::pyCreateTypeCode));
    assert(omniPy::pyDummyThreadClass);
    assert(PyClass_Check(omniPy::pyDummyThreadClass));
    assert(omniPy::pyEmptyTuple);

    //    cout << "Python objects registered." << endl;

    Py_INCREF(Py_None);
    return Py_None;
  }


  ////////////////////////////////////////////////////////////////////////////
  // CORBA:: functions                                                      //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_ORB_init(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    PyObject* pyargv;
    char*     orbid;
    int       argc;
    char**    argv;

    if (!PyArg_ParseTuple(args, "OOs", &pyorb, &pyargv, &orbid))
      return NULL;

    if (!PyList_Check(pyargv)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 2: parameter must be an argument list");
      return NULL;
    }

    argc = PyList_GET_SIZE(pyargv);
    argv = new char*[argc];

    PyObject* o;
    int i;
    for (i=0; i<argc; i++) {
      o = PyList_GET_ITEM(pyargv, i);
      argv[i] = PyString_AS_STRING(o);
    }

    int orig_argc = argc;

    CORBA::ORB_ptr orb = CORBA::ORB_init(argc, argv, orbid);

    // This is extremely horrid -- modify the Python list in place to
    // reflect the changed argv. This leaks PyStringObjects, but they
    // would have hung around for the program's life time anyway...
    if (argc < orig_argc) {
      int j;
      char *s, *t;
      for (i=0, j=0; i<argc; i++, j++) {
	s = argv[i];

	while (1) {
	  o = PyList_GET_ITEM(pyargv, j);
	  t = PyString_AS_STRING(o);
	  if (s == t) break;
	  j++;
	  assert(j < orig_argc);
	}
	PyList_SET_ITEM(pyargv, i, o);
      }
      ((PyListObject*)pyargv)->ob_size = argc;
    }

    delete [] argv;

    omniPy::setTwin(pyorb, orb, ORB_TWIN);

    Py_INCREF(Py_None);
    return Py_None;
  }


  ////////////////////////////////////////////////////////////////////////////
  // CORBA::ORB functions                                                   //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_BOA_init(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    PyObject* pyboa;
    PyObject* pyargv;
    char*     boaid;
    int       argc;
    char**    argv;

    if (!PyArg_ParseTuple(args, "OOOs", &pyboa, &pyorb, &pyargv, &boaid))
      return NULL;

    if (!PyList_Check(pyargv)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 3: parameter must be an argument list");
      return NULL;
    }

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);

    assert(orb);

    argc = PyList_GET_SIZE(pyargv);
    argv = new char*[argc];

    PyObject* o;
    int i;
    for (i=0; i<argc; i++) {
      o = PyList_GET_ITEM(pyargv, i);
      argv[i] = PyString_AS_STRING(o);
    }

    int orig_argc = argc;

    CORBA::BOA_ptr boa = orb->BOA_init(argc, argv, boaid);

    // This is extremely horrid -- modify the Python list in place to
    // reflect the changed argv. This leaks PyStringObjects, but they
    // would have hung around for the program's life time anyway...
    if (argc < orig_argc) {
      int j;
      char *s, *t;
      for (i=0, j=0; i<argc; i++, j++) {
	s = argv[i];

	while (1) {
	  o = PyList_GET_ITEM(pyargv, j);
	  t = PyString_AS_STRING(o);
	  if (s == t) break;
	  j++;
	  assert(j < orig_argc);
	}
	PyList_SET_ITEM(pyargv, i, o);
      }
      ((PyListObject*)pyargv)->ob_size = argc;
    }

    delete [] argv;

    omniPy::setTwin(pyboa, boa, BOA_TWIN);

    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject*
  omnipy_stringToObject(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    char* s;

    if (!PyArg_ParseTuple(args, "Os", &pyorb, &s))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);

    assert(orb);

    if (!s || strlen(s) == 0) {
      PyErr_SetString(PyExc_TypeError, "Invalid IOR");
      return 0;
    }

    CORBA::Object_ptr objref;

    try {
      omniObject* oobj = omniPy::stringToObject(s);
      if (oobj)
	objref = (CORBA::Object_ptr)(oobj->_widenFromTheMostDerivedIntf(0));
      else
	objref = CORBA::Object::_nil();
    }
    catch (CORBA::SystemException& ex) {
      omniPy::handleSystemException(ex);
      return 0;
    }

    return omniPy::createPyCorbaObjRef(0, objref);
  }

  static PyObject*
  omnipy_objectToString(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, "OO", &pyorb, &pyobjref))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);

    assert(orb);

    CORBA::Object_ptr objref;

    if (pyobjref == Py_None) {
      objref = CORBA::Object::_nil();
    }
    else {
      if (!PyInstance_Check(pyobjref)) {
	PyErr_SetString(PyExc_TypeError,
			"Argument must be an object reference.");
	return NULL;
      }	
      objref = (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);
    }

    assert(objref);

    CORBA::String_var str = orb->object_to_string(objref);

    return PyString_FromString((char*)str);
  }

  static PyObject*
  omnipy_listInitialServices(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;

    if (!PyArg_ParseTuple(args, "O", &pyorb))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);
    assert(orb);

    CORBA::ORB::ObjectIdList_var ids = orb->list_initial_services();

    PyObject* pyids = PyList_New(ids->length());

    for (CORBA::ULong i=0; i<ids->length(); i++) {
      PyList_SetItem(pyids, i, PyString_FromString(ids[i]));
    }
    return pyids;
  }

  static PyObject*
  omnipy_resolveInitialReferences(PyObject* self, PyObject* args)
  {
    PyObject* pyorb;
    char*     id;

    if (!PyArg_ParseTuple(args, "Os", &pyorb, &id))
      return NULL;

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)omniPy::getTwin(pyorb, ORB_TWIN);
    assert(orb);

    CORBA::Object_ptr objref;

    try {
      objref = orb->resolve_initial_references(id);
    }
    catch (const CORBA::SystemException& ex) {
      omniPy::handleSystemException(ex);
      return 0;
    }
    catch (CORBA::ORB::InvalidName& ex) {
      // *** Should propogate this back to Python land
      CORBA::BAD_PARAM bp;
      omniPy::handleSystemException(bp);
      return 0;
    }
    return omniPy::createPyCorbaObjRef(0, objref);
  }



  ////////////////////////////////////////////////////////////////////////////
  // CORBA::BOA functions                                                   //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_objectIsReady(PyObject* self, PyObject* args)
  {
    PyObject* pyboa;
    PyObject* pyservant;

    if (!PyArg_ParseTuple(args, "OO", &pyboa, &pyservant))
      return NULL;

    assert(pyservant && PyInstance_Check(pyservant));

    PyObject* opdict = PyObject_GetAttrString(pyservant, "_op_d");
    assert(opdict && PyDict_Check(opdict));

    PyObject* repoId = PyObject_GetAttrString(pyservant, "_NP_RepositoryId");
    assert(repoId && PyString_Check(repoId));

    omniPy::Py_Servant* cxxservant;
    cxxservant = new omniPy::Py_Servant(pyservant, opdict,
					PyString_AS_STRING(repoId));
    Py_DECREF(repoId);
    Py_DECREF(opdict);

    omniPy_objectIsReady(cxxservant);

    CORBA::Object_ptr objref = (CORBA::Object_ptr)cxxservant;

    PyObject* pyobjref = omniPy::createPyCorbaObjRef(0, objref);

    PyObject_SetAttrString(pyservant, "_objref", pyobjref);
    Py_DECREF(pyobjref);

    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject*
  omnipy_implIsReady(PyObject* self, PyObject* args)
  {
    PyObject* pyboa;
    int z, noblock;

    if (!PyArg_ParseTuple(args, "Oii", &pyboa, &z, &noblock))
      return NULL;

    assert(pyboa && PyInstance_Check(pyboa));

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyboa, BOA_TWIN);

    assert(boa);

    Py_BEGIN_ALLOW_THREADS

    boa->impl_is_ready(0, noblock);

    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;
  }


  ////////////////////////////////////////////////////////////////////////////
  // POA functions                                                          //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_referenceToServant(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, "O", &pyobjref))
      return NULL;

    assert(pyobjref && PyInstance_Check(pyobjref));

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    omniObject* oobj = cxxobjref->PR_getobj();

    if (oobj->is_proxy()) {
      PyErr_SetString(PyExc_TypeError, "Not a local object");
      return 0;
    }
    else {
      omniPy::Py_Servant* local = (omniPy::Py_Servant*)
	oobj->_widenFromTheMostDerivedIntf("Py_Servant", 1);

      assert(local);
      return local->pyServant();
    }
  }

  static PyObject*
  omnipy_referenceToId(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, "O", &pyobjref))
      return NULL;

    assert(pyobjref && PyInstance_Check(pyobjref));

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    omniObject* oobj = cxxobjref->PR_getobj();

    if (oobj->is_proxy()) {
      PyErr_SetString(PyExc_TypeError, "Not a local object");
      return 0;
    }
    else {
      CORBA::Octet* key;
      CORBA::ULong  ksize;
      oobj->getKey(key, ksize);

      PyObject* pykey = PyString_FromStringAndSize(0, ksize);
      CORBA::Octet* s = (CORBA::Octet*)PyString_AS_STRING(pykey);

      for (CORBA::ULong i=0; i<ksize; i++)
	s[i] = key[i];

      return pykey;
    }
  }


  ////////////////////////////////////////////////////////////////////////////
  // Functions operating on object references                               //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_invokeOp(PyObject* self, PyObject* args)
  {
    // Arg format
    //  (objref, op_name, (in_desc, out_desc, exc_desc), args)
    //
    //  exc_desc is a dictionary containing a mapping from repoIds to
    //  tuples of the form (exception class, marshal desc., param count)

    PyObject *pyobjref, *in_d, *out_d, *exc_d, *op_args, *result;
    char*  op;
    size_t op_len;

    /*
    if (!PyArg_ParseTuple(args, "Os#(OOO)O", &pyobjref, &op, &op_len,
			  &in_d, &out_d, &exc_d, &op_args))
      return NULL;
    */
    PyObject* o;
    PyObject* desc;

    pyobjref = PyTuple_GET_ITEM(args,0);

    o        = PyTuple_GET_ITEM(args,1);
    op       = PyString_AS_STRING(o);
    op_len   = PyString_GET_SIZE(o);

    desc     = PyTuple_GET_ITEM(args,2);

    in_d     = PyTuple_GET_ITEM(desc,0);
    out_d    = PyTuple_GET_ITEM(desc,1);
    exc_d    = PyTuple_GET_ITEM(desc,2);

    op_args  = PyTuple_GET_ITEM(args,3);

    if (PyTuple_GET_SIZE(op_args) != PyTuple_GET_SIZE(in_d)) {
      char* err = new char[80];
      sprintf(err, "Operation requires %d argument%s; %d given",
	      PyTuple_GET_SIZE(in_d),
	      (PyTuple_GET_SIZE(in_d) == 1) ? "" : "s",
	      PyTuple_GET_SIZE(op_args));

      PyErr_SetString(PyExc_TypeError, err);
      delete [] err;
      return 0;
    }

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    omniObject* oobj = cxxobjref->PR_getobj();

    if (oobj->is_proxy()) {

      if (out_d != Py_None) {
	CORBA::Boolean has_exc = (exc_d != Py_None);

	omniPy::Py_OmniProxyCallDesc call_desc(op, op_len + 1, has_exc,
					       in_d, out_d, exc_d,
					       op_args);
	try {
	  call_desc.releaseInterpreterLock();
	  OmniProxyCallWrapper::invoke(oobj, call_desc);
	  return call_desc.result();
	}
	catch (const CORBA::SystemException& ex) {
	  call_desc.systemException(ex);
	}
	catch (const CORBA::UserException& ex) {
	  // Exception has been handled by the call descriptor
	}
      }
      else {
	// Operation is oneway
	omniPy::Py_OmniOWProxyCallDesc call_desc(op, op_len + 1,
						 in_d, op_args);
	try {
	  call_desc.releaseInterpreterLock();
	  OmniProxyCallWrapper::one_way(oobj, call_desc);
	  call_desc.reacquireInterpreterLock();

	  Py_INCREF(Py_None);
	  return Py_None;
	}
	catch (const CORBA::SystemException& ex) {
	  call_desc.systemException(ex);
	}
      }
      return 0;
    }
    else {
      // Local object
      omniPy::Py_Servant* local = (omniPy::Py_Servant*)
	oobj->_widenFromTheMostDerivedIntf("Py_Servant", 1);

      assert(local);
      return local->local_dispatch(op, op_args);
    }
  }

  static PyObject*
  omnipy_releaseObjref(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, "O", &pyobjref))
      return NULL;

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    CORBA::release(cxxobjref);
    //    cout << "C++ objref released." << endl;

    omniPy::remTwin(pyobjref, OBJREF_TWIN);

    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject*
  omnipy_isA(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;
    char*     repoId;

    if (!PyArg_ParseTuple(args, "Os", &pyobjref, &repoId))
      return NULL;

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    try {
      omniPy::InterpreterUnlocker ul;
      CORBA::Boolean isa = cxxobjref->_is_a(repoId);
      return PyInt_FromLong(isa);
    }
    catch (const CORBA::SystemException& ex) {
      omniPy::handleSystemException(ex);
    }
    return 0;
  }

  static PyObject*
  omnipy_nonExistent(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, "O", &pyobjref))
      return NULL;

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    try {
      omniPy::InterpreterUnlocker ul;
      CORBA::Boolean nex = cxxobjref->_non_existent();
      return PyInt_FromLong(nex);
    }
    catch (const CORBA::SystemException& ex) {
      omniPy::handleSystemException(ex);
    }
    return 0;
  }

  static PyObject*
  omnipy_isEquivalent(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref1;
    PyObject* pyobjref2;

    if (!PyArg_ParseTuple(args, "OO", &pyobjref1, &pyobjref2))
      return NULL;

    CORBA::Object_ptr cxxobjref1, cxxobjref2;

    cxxobjref1 = (CORBA::Object_ptr)omniPy::getTwin(pyobjref1, OBJREF_TWIN);
    cxxobjref2 = (CORBA::Object_ptr)omniPy::getTwin(pyobjref2, OBJREF_TWIN);

    try {
      omniPy::InterpreterUnlocker ul;
      CORBA::Boolean ise = cxxobjref1->_is_equivalent(cxxobjref2);
      return PyInt_FromLong(ise);
    }
    catch (const CORBA::SystemException& ex) {
      omniPy::handleSystemException(ex);
    }
    return 0;
  }

  static PyObject*
  omnipy_hash(PyObject* self, PyObject* args)
  {
    PyObject*    pyobjref;
    CORBA::ULong max;

    if (!PyArg_ParseTuple(args, "Oi", &pyobjref, &max))
      return NULL;

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    CORBA::ULong h = cxxobjref->_hash(max);

    return PyInt_FromLong(h);
  }


  static PyObject*
  omnipy_narrow(PyObject* self, PyObject* args)
  {
    PyObject* pysource;
    char*     repoId;

    if (!PyArg_ParseTuple(args, "Os", &pysource, &repoId))
      return NULL;

    CORBA::Object_ptr cxxsource =
      (CORBA::Object_ptr)omniPy::getTwin(pysource, OBJREF_TWIN);

    CORBA::Boolean isa;
    try {
      omniPy::InterpreterUnlocker ul;
      isa = cxxsource->_is_a(repoId);
    }
    catch (const CORBA::SystemException& ex) {
      omniPy::handleSystemException(ex);
      return 0;
    }
    if (isa) {
      omniObject* oosource = cxxsource->PR_getobj();
      omniObject* oodest = omniPy::createObjRef(oosource->NP_IRRepositoryId(),
						repoId,
						oosource->iopProfiles(), 0);
      CORBA::Object_ptr cxxdest =
	(CORBA::Object_ptr)(oodest->_widenFromTheMostDerivedIntf(0));

      return omniPy::createPyCorbaObjRef(repoId, cxxdest);
    }
    else {
      Py_INCREF(Py_None);
      return Py_None;
    }
  }


  ////////////////////////////////////////////////////////////////////////////
  // Python method table                                                    //
  ////////////////////////////////////////////////////////////////////////////

  static PyMethodDef omnipy_methods[] = {

    // omnipy specific things:
    {"checkVersion",             omnipy_checkVersion,            METH_VARARGS},
    {"registerPyObjects",        omnipy_registerPyObjects,       METH_VARARGS},

    // Wrappers for functions in CORBA::
    {"ORB_init",                 omnipy_ORB_init,                METH_VARARGS},

    // Wrappers for functions in CORBA::ORB::
    {"BOA_init",                 omnipy_BOA_init,                METH_VARARGS},
    {"stringToObject",           omnipy_stringToObject,          METH_VARARGS},
    {"objectToString",           omnipy_objectToString,          METH_VARARGS},
    {"listInitialServices",      omnipy_listInitialServices,     METH_VARARGS},
    {"resolveInitialReferences", omnipy_resolveInitialReferences,METH_VARARGS},

    // Wrappers for functions in CORBA::BOA::
    {"objectIsReady",            omnipy_objectIsReady,           METH_VARARGS},
    {"implIsReady",              omnipy_implIsReady,             METH_VARARGS},

    // Wrappers for POA functions
    {"referenceToServant",       omnipy_referenceToServant,      METH_VARARGS},
    {"referenceToId",            omnipy_referenceToId,           METH_VARARGS},

    // Functions for CORBA objects:
    {"invokeOp",                 omnipy_invokeOp,                METH_VARARGS},
    {"releaseObjref",            omnipy_releaseObjref,           METH_VARARGS},
    {"isA",                      omnipy_isA,                     METH_VARARGS},
    {"nonExistent",              omnipy_nonExistent,             METH_VARARGS},
    {"isEquivalent",             omnipy_isEquivalent,            METH_VARARGS},
    {"hash",                     omnipy_hash,                    METH_VARARGS},
    {"narrow",                   omnipy_narrow,                  METH_VARARGS},
    {NULL,NULL}
  };

  void DLL_EXPORT init_omnipy()
  {
    // Make sure Python is running multi-threaded
    PyEval_InitThreads();

    omniPy::pyInterpreterLock = new omni_mutex;

    PyObject *m  = Py_InitModule("_omnipy", omnipy_methods);
    PyObject *d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "omnipyTwinType",
			 (PyObject*)&omnipyTwinType);

    //    cout << "_omnipy initialised." << endl;
  }
}
