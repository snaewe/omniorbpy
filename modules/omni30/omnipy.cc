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

// $Id$

// $Log$
// Revision 1.30  2000/03/06 18:46:25  dpg1
// (char*)s for Solaris.
//
// Revision 1.29  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//
// Revision 1.28  2000/02/04 12:17:11  dpg1
// Support for VMS.
//
// Revision 1.27  1999/12/15 12:17:20  dpg1
// Changes to compile with SunPro CC 5.0.
//
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

PyInterpreterState* omniPy::pyInterpreter;
omni_mutex*         omniPy::pyInterpreterLock;

PyObject* omniPy::pyCORBAmodule;	// The CORBA module
PyObject* omniPy::pyCORBAsysExcMap;	//  The system exception map
PyObject* omniPy::pyCORBAAnyClass;	//  Any class
PyObject* omniPy::pyomniORBmodule;	// The omniORB module
PyObject* omniPy::pyomniORBobjrefMap;	//  The objref class map
PyObject* omniPy::pyomniORBtypeMap;     //  The repoId to descriptor mapping
PyObject* omniPy::pyomniORBwordMap;     //  Reserved word map
PyObject* omniPy::pyPortableServerModule; // Portable server module
PyObject* omniPy::pyServantClass;       // Servant class
PyObject* omniPy::pyCreateTypeCode;	// Function to create a TypeCode object
PyObject* omniPy::pyWorkerThreadClass;  // Worker thread class
PyObject* omniPy::pyWorkerThreadDel;    // Method to delete worker thread
PyObject* omniPy::pyEmptyTuple;         // Zero element tuple.


////////////////////////////////////////////////////////////////////////////
// Pointer to the ORB                                                     //
////////////////////////////////////////////////////////////////////////////

CORBA::ORB_ptr omniPy::orb;


////////////////////////////////////////////////////////////////////////////
// Twin type                                                              //
////////////////////////////////////////////////////////////////////////////

extern "C" {

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
    (char*)"omnipyTwin",
    sizeof(omnipyTwin),
    0,
    (destructor)omnipyTwin_dealloc,   /*tp_dealloc*/
    0,			              /*tp_print*/
    0,                                /*tp_getattr*/
    0,				      /*tp_setattr*/
    (cmpfunc)omnipyTwin_cmp,          /*tp_compare*/
    0,                                /*tp_repr*/
    0,				      /*tp_as_number*/
    0,                                /*tp_as_sequence*/
    0,				      /*tp_as_mapping*/
    (hashfunc)omnipyTwin_hash,	      /*tp_hash*/
    0,				      /*tp_call*/
    0,				      /*tp_str*/
    0,				      /*tp_getattro*/
    0,				      /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    0,				      /*tp_xxx4*/
    0,                                /*tp_doc*/
  };
}

PyObject*
omniPy::newTwin(void* twin)
{
  omnipyTwin* ot = PyMem_NEW(omnipyTwin, 1);
  ot->ob_type = &omnipyTwinType;
  ot->ob_size = 0;
  ot->ob_twin = (void*)twin;
  _Py_NewReference(ot);
  return (PyObject*)ot;
}


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

    if (!PyArg_ParseTuple(args, (char*)"iis", &maj, &min, &mod))
      return 0;

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
  omnipy_coreVersion(PyObject* self, PyObject* args)
  {
    static char* cv = (char*)OMNIORB_VERSION_STRING;

    if (!PyArg_ParseTuple(args, (char*)"")) return 0;
    return Py_BuildValue((char*)"s", cv);
  }

  static PyObject*
  omnipy_registerPyObjects(PyObject* self, PyObject* args)
  {
    PyObject* temp;

    // Get a pointer to the interpreter state
    PyThreadState* tstate = PyThreadState_Get();
    omniPy::pyInterpreter = tstate->interp;

    if (!PyArg_ParseTuple(args, (char*)"O", &omniPy::pyomniORBmodule))
      return 0;

    OMNIORB_ASSERT(PyModule_Check(omniPy::pyomniORBmodule));

    omniPy::pyCORBAmodule =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"CORBA");

    OMNIORB_ASSERT(omniPy::pyCORBAmodule &&
		   PyModule_Check(omniPy::pyCORBAmodule));

    omniPy::pyCORBAsysExcMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule,
			     (char*)"sysExceptionMapping");

    temp =
      PyObject_GetAttrString(omniPy::pyCORBAmodule, (char*)"Object");

    omniPy::pyCORBAAnyClass =
      PyObject_GetAttrString(omniPy::pyCORBAmodule, (char*)"Any");

    omniPy::pyomniORBobjrefMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"objrefMapping");

    omniPy::pyomniORBtypeMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"typeMapping");

    omniPy::pyomniORBwordMap =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"keywordMapping");

    omniPy::pyPortableServerModule =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"PortableServer");

    OMNIORB_ASSERT(omniPy::pyPortableServerModule);
    OMNIORB_ASSERT(PyModule_Check(omniPy::pyPortableServerModule));

    omniPy::pyServantClass =
      PyObject_GetAttrString(omniPy::pyPortableServerModule, (char*)"Servant");

    temp =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"tcInternal");

    omniPy::pyCreateTypeCode = PyObject_GetAttrString(temp,
						      (char*)"createTypeCode");

    omniPy::pyWorkerThreadClass =
      PyObject_GetAttrString(omniPy::pyomniORBmodule, (char*)"WorkerThread");

    omniPy::pyWorkerThreadDel =
      PyObject_GetAttrString(omniPy::pyWorkerThreadClass, (char*)"delete");

    omniPy::pyEmptyTuple = PyTuple_New(0);

    OMNIORB_ASSERT(omniPy::pyCORBAsysExcMap);
    OMNIORB_ASSERT(PyDict_Check(omniPy::pyCORBAsysExcMap));
    OMNIORB_ASSERT(omniPy::pyCORBAAnyClass);
    OMNIORB_ASSERT(PyClass_Check(omniPy::pyCORBAAnyClass));
    OMNIORB_ASSERT(omniPy::pyomniORBobjrefMap);
    OMNIORB_ASSERT(PyDict_Check(omniPy::pyomniORBobjrefMap));
    OMNIORB_ASSERT(omniPy::pyomniORBtypeMap);
    OMNIORB_ASSERT(PyDict_Check(omniPy::pyomniORBtypeMap));
    OMNIORB_ASSERT(omniPy::pyomniORBwordMap);
    OMNIORB_ASSERT(PyDict_Check(omniPy::pyomniORBwordMap));
    OMNIORB_ASSERT(omniPy::pyServantClass);
    OMNIORB_ASSERT(PyClass_Check(omniPy::pyServantClass));
    OMNIORB_ASSERT(omniPy::pyCreateTypeCode);
    OMNIORB_ASSERT(PyFunction_Check(omniPy::pyCreateTypeCode));
    OMNIORB_ASSERT(omniPy::pyWorkerThreadClass);
    OMNIORB_ASSERT(PyClass_Check(omniPy::pyWorkerThreadClass));
    OMNIORB_ASSERT(omniPy::pyWorkerThreadDel);
    OMNIORB_ASSERT(PyMethod_Check(omniPy::pyWorkerThreadDel));
    OMNIORB_ASSERT(omniPy::pyEmptyTuple);

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

    if (!PyArg_ParseTuple(args, (char*)"OOs", &pyorb, &pyargv, &orbid))
      return 0;

    if (!PyList_Check(pyargv)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 2: parameter must be an argument list");
      return 0;
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
    omniPy::orb = orb;

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
	  OMNIORB_ASSERT(j < orig_argc);
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
  // CDR stream marshalling/unmarshalling                                   //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_cdrMarshal(PyObject* self, PyObject* args)
  {
    PyObject *desc, *data;

    if (!PyArg_ParseTuple(args, (char*)"OO", &desc, &data)) return 0;

    try {
      size_t size = 1; // One byte for the byteorder
      size = omniPy::alignedSize(size, desc, data, CORBA::COMPLETED_NO);
      MemBufferedStream stream(size);
      stream.byteOrder() >>= stream;
      omniPy::marshalPyObject(stream, desc, data);
      return PyString_FromStringAndSize((char*)stream.data(),
					stream.alreadyWritten());
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
  }

  static PyObject*
  omnipy_cdrUnmarshal(PyObject* self, PyObject* args)
  {
    PyObject* desc;
    char*     encap;
    size_t    size;
    
    if (!PyArg_ParseTuple(args, (char*)"Os#", &desc, &encap, &size)) return 0;

    try {
      MemBufferedStream stream(size);
      stream.put_char_array((CORBA::Char*)encap, size);
      CORBA::Char byteOrder;
      byteOrder <<= stream;
      stream.byteOrder(byteOrder);
      return omniPy::unmarshalPyObject(stream, desc);
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
  }


  ////////////////////////////////////////////////////////////////////////////
  // Functions operating on object references                               //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_invoke(PyObject* self, PyObject* args)
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
      return 0;
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

    omniObjRef*    oobjref   = cxxobjref->_PR_getobj();
    CORBA::Boolean is_oneway = (out_d == Py_None);

    omniPy::Py_omniCallDescriptor call_desc(op, op_len + 1, is_oneway,
					    in_d, out_d, exc_d, op_args);
    try {
      call_desc.releaseInterpreterLock();
      oobjref->_invoke(call_desc);
      if (!is_oneway) {
	return call_desc.result();
      }
      else {
	call_desc.reacquireInterpreterLock();
	Py_INCREF(Py_None);
	return Py_None;
      }
    }
    catch (const CORBA::SystemException& ex) {
      call_desc.systemException(ex);
    }
    catch (const omniPy::UserExceptionHandled& ex) {
      // Exception has been handled by the call descriptor
    }
    return 0;
  }

  static PyObject*
  omnipy_releaseObjref(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"O", &pyobjref))
      return 0;

    OMNIORB_ASSERT(PyInstance_Check(pyobjref));

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    if (cxxobjref) {
      CORBA::release(cxxobjref);
      omniPy::remTwin(pyobjref, OBJREF_TWIN);
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject*
  omnipy_isA(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;
    char*     repoId;

    if (!PyArg_ParseTuple(args, (char*)"Os", &pyobjref, &repoId))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!cxxobjref);

    try {
      omniPy::InterpreterUnlocker ul;
      CORBA::Boolean isa = cxxobjref->_is_a(repoId);
      return PyInt_FromLong(isa);
    }
    catch (const CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
  }

  static PyObject*
  omnipy_nonExistent(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"O", &pyobjref))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!cxxobjref);

    try {
      omniPy::InterpreterUnlocker ul;
      CORBA::Boolean nex = cxxobjref->_non_existent();
      return PyInt_FromLong(nex);
    }
    catch (const CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
  }

  static PyObject*
  omnipy_isEquivalent(PyObject* self, PyObject* args)
  {
    PyObject* pyobjref1;
    PyObject* pyobjref2;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyobjref1, &pyobjref2))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref1) ||
			  !PyInstance_Check(pyobjref2));

    CORBA::Object_ptr cxxobjref1, cxxobjref2;
    cxxobjref1 = (CORBA::Object_ptr)omniPy::getTwin(pyobjref1, OBJREF_TWIN);
    cxxobjref2 = (CORBA::Object_ptr)omniPy::getTwin(pyobjref2, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!cxxobjref1 || !cxxobjref2);

    try {
      omniPy::InterpreterUnlocker ul;
      CORBA::Boolean ise = cxxobjref1->_is_equivalent(cxxobjref2);
      return PyInt_FromLong(ise);
    }
    catch (const CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
  }

  static PyObject*
  omnipy_hash(PyObject* self, PyObject* args)
  {
    PyObject*    pyobjref;
    CORBA::ULong max;

    if (!PyArg_ParseTuple(args, (char*)"Oi", &pyobjref, &max))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr cxxobjref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!cxxobjref);

    CORBA::ULong h = cxxobjref->_hash(max);
    return PyInt_FromLong(h);
  }


  static PyObject*
  omnipy_narrow(PyObject* self, PyObject* args)
  {
    PyObject* pysource;
    char*     repoId;

    if (!PyArg_ParseTuple(args, (char*)"Os", &pysource, &repoId))
      return 0;

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pysource));

    CORBA::Object_ptr cxxsource =
      (CORBA::Object_ptr)omniPy::getTwin(pysource, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!cxxsource);

    CORBA::Boolean isa;
    try {
      omniPy::InterpreterUnlocker ul;
      isa = cxxsource->_is_a(repoId);
    }
    catch (const CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    if (isa) {
      omniObjRef* oosource = cxxsource->_PR_getobj();
      omniObjRef* oodest = omniPy::createObjRef(oosource->_mostDerivedRepoId(),
						repoId,
						oosource->_iopProfiles(),
						0, 0);
      CORBA::Object_ptr cxxdest =
	(CORBA::Object_ptr)(oodest->_ptrToObjRef(CORBA::Object::_PD_repoId));

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
    {(char*)"checkVersion",      omnipy_checkVersion,            METH_VARARGS},
    {(char*)"coreVersion",       omnipy_coreVersion,             METH_VARARGS},
    {(char*)"registerPyObjects", omnipy_registerPyObjects,       METH_VARARGS},
    {(char*)"cdrMarshal",        omnipy_cdrMarshal,              METH_VARARGS},
    {(char*)"cdrUnmarshal",      omnipy_cdrUnmarshal,            METH_VARARGS},

    // Wrappers for functions in CORBA::
    {(char*)"ORB_init",          omnipy_ORB_init,                METH_VARARGS},

    // Functions for CORBA objects:
    {(char*)"invoke",            omnipy_invoke,                  METH_VARARGS},
    {(char*)"releaseObjref",     omnipy_releaseObjref,           METH_VARARGS},
    {(char*)"isA",               omnipy_isA,                     METH_VARARGS},
    {(char*)"nonExistent",       omnipy_nonExistent,             METH_VARARGS},
    {(char*)"isEquivalent",      omnipy_isEquivalent,            METH_VARARGS},
    {(char*)"hash",              omnipy_hash,                    METH_VARARGS},
    {(char*)"narrow",            omnipy_narrow,                  METH_VARARGS},
    {0,0}
  };

  void DLL_EXPORT init_omnipy()
  {
    // Make sure Python is running multi-threaded
    PyEval_InitThreads();

    omniPy::pyInterpreterLock = new omni_mutex;

    PyObject* m = Py_InitModule((char*)"_omnipy", omnipy_methods);
    PyObject* d = PyModule_GetDict(m);
    PyDict_SetItemString(d, (char*)"omnipyTwinType",
			 (PyObject*)&omnipyTwinType);

    omniPy::initORBFunc(d);
    omniPy::initPOAFunc(d);
    omniPy::initPOAManagerFunc(d);
  }
}
