// Python -- omniORB interface

// $Id$

// $Log$
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


#include <python1.5/Python.h>

#include <omnipy.h>
#include <pyProxyCallWrapper.h>

#include <iostream.h>


////////////////////////////////////////////////////////////////////////////
// Pointers to Python objects                                             //
////////////////////////////////////////////////////////////////////////////

PyObject* pyCORBAmodule;	// The CORBA module
PyObject* pyCORBAmoduleDict;	//  Its dictionary
PyObject* pyCORBAsysExcMap;	//  The system exception map
PyObject* pyomniORBmodule;	// The omniORB module
PyObject* pyomniORBmoduleDict;	//  Its dictionary
PyObject* pyomniORBproxyMap;	//  The proxy object class map



////////////////////////////////////////////////////////////////////////////
// Useful functions                                                       //
////////////////////////////////////////////////////////////////////////////

PyObject*
createPyCorbaObject(const char* targetRepoId, const CORBA::Object_ptr obj)
{
  if (CORBA::is_nil(obj)) {
    cout << "Obj is nil." << endl;
    Py_INCREF(Py_None);
    return Py_None;
  }
  cout << "Obj is not nil." << endl;

  omniObject* oobj = obj->PR_getobj();

  if (!oobj->is_proxy()) {
    cerr << "Received a non-proxy object. Not yet implemented!" << endl;
    abort();
  }
  const char* actualRepoId = oobj->NP_IRRepositoryId();

  PyObject*      proxyClass;
  CORBA::Boolean fullTypeUnknown = 0;

  // Try to find proxy class for most derived type:
  proxyClass = PyDict_GetItemString(pyomniORBproxyMap, actualRepoId);

  if (!proxyClass && targetRepoId) {
    // No proxy class for the most derived type -- try to find one for
    // the target type:
    proxyClass      = PyDict_GetItemString(pyomniORBproxyMap, targetRepoId);
    fullTypeUnknown = 1;
  }
  if (!proxyClass) {
    // No target type, or stub code bug:
    proxyClass      = PyDict_GetItemString(pyCORBAmoduleDict, "Object");
    fullTypeUnknown = 1;
  }

  assert(proxyClass); // Couldn't even find CORBA.Object!

  PyObject* arglist    = Py_BuildValue("()");
  PyObject* pyproxy    = PyEval_CallObject(proxyClass, arglist);
  Py_DECREF(arglist);

  assert(PyInstance_Check(pyproxy));

  if (fullTypeUnknown) {
    PyObject* idstr = PyString_FromString(actualRepoId);
    PyDict_SetItemString(((PyInstanceObject*)pyproxy)->in_dict,
			 "_NP_RepositoryId", idstr);
  }

  setTwin(pyproxy, (CORBA::Object_ptr)obj);

  cout << "Returning proxy..." << endl;

  return pyproxy;
}


void
handleSystemException(const CORBA::SystemException& ex)
{
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					ex.NP_RepositoryId());

  PyObject* exca = Py_BuildValue("(ii)", ex.minor(), ex.completed());
  PyObject* exci = PyEval_CallObject(excc, exca);
  Py_DECREF(exca);
  PyErr_SetObject(excc, exci);
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

    if (!PyArg_ParseTuple(args, "iis", &maj, &min, &mod))
      return NULL;

    if (maj != OMNIPY_MAJOR) {
      omniORB::log << "***\n*** WARNING! omnipy module version "
		   << OMNIPY_MAJOR << "." << OMNIPY_MINOR
		   << ", stubs in " << mod << " are version "
		   << maj << "." << min
		   << "\n***\n";
      omniORB::log.flush();
    }
    Py_INCREF(Py_None);
    return Py_None;
  }


  static PyObject*
  omnipy_registerPyObjects(PyObject* self, PyObject* args)
  {
    if (!PyArg_ParseTuple(args, "OO", &pyCORBAmodule, &pyomniORBmodule))
      return NULL;

    if (!PyModule_Check(pyCORBAmodule)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 1: parameter must be a module");
      return NULL;
    }
    if (!PyModule_Check(pyomniORBmodule)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 2: parameter must be a module");
      return NULL;
    }
    pyCORBAmoduleDict = PyModule_GetDict(pyCORBAmodule);
    pyCORBAsysExcMap  = PyDict_GetItemString(pyCORBAmoduleDict,
					     "__sysExceptionMapping");

    pyomniORBmoduleDict = PyModule_GetDict(pyomniORBmodule);
    pyomniORBproxyMap   = PyDict_GetItemString(pyomniORBmoduleDict,
					       "proxyObjectMapping");

    cout << "Python objects registered." << endl;

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

    setTwin(pyorb, orb);

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

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)getTwin(pyorb);

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

    setTwin(pyboa, boa);

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

    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)getTwin(pyorb);

    CORBA::Object_ptr obj = orb->string_to_object(s);

    cout << "Made object." << (unsigned long)obj << endl;

    return createPyCorbaObject(0, obj);
  }


  ////////////////////////////////////////////////////////////////////////////
  // Functions operating on proxy objects                                   //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_invokeOp(PyObject* self, PyObject* args)
  {
    // Arg format
    //  (proxy, op_name, (in_desc, out_desc, exc_desc), args)
    //
    //  exc_desc is a dictionary containing a mapping from repoIds to
    //  tuples of the form (exception class, marshal desc., param count)

    PyObject *pyproxy, *in_d, *out_d, *exc_d, *op_args, *result;
    char*  op;
    size_t op_len;

    /*
    if (!PyArg_ParseTuple(args, "Os#(OOO)O", &pyproxy, &op, &op_len,
			  &in_d, &out_d, &exc_d, &op_args))
      return NULL;
    */
    PyObject* o;
    PyObject* desc;

    pyproxy = PyTuple_GET_ITEM(args,0);

    o       = PyTuple_GET_ITEM(args,1);
    op      = PyString_AS_STRING(o);
    op_len  = PyString_GET_SIZE(o);

    desc    = PyTuple_GET_ITEM(args,2);

    in_d    = PyTuple_GET_ITEM(desc,0);
    out_d   = PyTuple_GET_ITEM(desc,1);
    exc_d   = PyTuple_GET_ITEM(desc,2);

    op_args = PyTuple_GET_ITEM(args,3);

    if (PyTuple_GET_SIZE(op_args) != PyTuple_GET_SIZE(in_d)) {
      char* err = new char[160];
      snprintf(err, 159, "%s requires %d argument%s; %d given", op,
	       PyTuple_GET_SIZE(in_d),
	       (PyTuple_GET_SIZE(in_d) == 1) ? "" : "s",
	       PyTuple_GET_SIZE(op_args));

      PyErr_SetString(PyExc_TypeError, err);
      delete [] err;
      return 0;
    }

    CORBA::Object_ptr cxxproxy = (CORBA::Object_ptr)getTwin(pyproxy);

    CORBA::Boolean has_exc = (exc_d != Py_None);

    Py_OmniProxyCallDesc call_desc(op, op_len + 1, has_exc,
				   in_d, out_d, exc_d,
				   op_args);
    try {
      OmniProxyCallWrapper::invoke(cxxproxy->PR_getobj(), call_desc);
      return call_desc.result();
    }
    catch (const CORBA::SystemException& ex) {
      call_desc.systemException(ex);
    }
    catch (const CORBA::UserException& ex) {
    }
    return 0;
  }

  static PyObject*
  omnipy_releaseProxy(PyObject* self, PyObject* args)
  {
    PyObject* pyproxy;

    if (!PyArg_ParseTuple(args, "O", &pyproxy))
      return NULL;

    CORBA::Object_ptr cxxproxy = (CORBA::Object_ptr)getTwin(pyproxy);

    CORBA::release(cxxproxy);
    cout << "Proxy released." << endl;

    remTwin(pyproxy);

    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject*
  omnipy_isA(PyObject* self, PyObject* args)
  {
    PyObject* pyproxy;
    char*     repoId;

    if (!PyArg_ParseTuple(args, "Os", &pyproxy, &repoId))
      return NULL;

    CORBA::Object_ptr cxxproxy = (CORBA::Object_ptr)getTwin(pyproxy);

    try {
      CORBA::Boolean isa = cxxproxy->_is_a(repoId);
      return PyInt_FromLong(isa);
    }
    catch (const CORBA::SystemException& ex) {
      handleSystemException(ex);
    }
    return 0;
  }

  static PyObject*
  omnipy_nonExistent(PyObject* self, PyObject* args)
  {
    PyObject* pyproxy;

    if (!PyArg_ParseTuple(args, "O", &pyproxy))
      return NULL;

    CORBA::Object_ptr cxxproxy = (CORBA::Object_ptr)getTwin(pyproxy);

    try {
      CORBA::Boolean nex = cxxproxy->_non_existent();
      return PyInt_FromLong(nex);
    }
    catch (const CORBA::SystemException& ex) {
      handleSystemException(ex);
    }
    return 0;
  }

  static PyObject*
  omnipy_isEquivalent(PyObject* self, PyObject* args)
  {
    PyObject* pyproxy1;
    PyObject* pyproxy2;

    if (!PyArg_ParseTuple(args, "OO", &pyproxy1, &pyproxy2))
      return NULL;

    CORBA::Object_ptr cxxproxy1 = (CORBA::Object_ptr)getTwin(pyproxy1);
    CORBA::Object_ptr cxxproxy2 = (CORBA::Object_ptr)getTwin(pyproxy2);

    try {
      CORBA::Boolean ise = cxxproxy1->_is_equivalent(cxxproxy2);
      return PyInt_FromLong(ise);
    }
    catch (const CORBA::SystemException& ex) {
      handleSystemException(ex);
    }
    return 0;
  }

  static PyObject*
  omnipy_hash(PyObject* self, PyObject* args)
  {
    PyObject*    pyproxy;
    CORBA::ULong max;

    if (!PyArg_ParseTuple(args, "Oi", &pyproxy, &max))
      return NULL;

    CORBA::Object_ptr cxxproxy = (CORBA::Object_ptr)getTwin(pyproxy);
    CORBA::ULong      h        = cxxproxy->_hash(max);

    return PyInt_FromLong(h);
  }


  static PyObject*
  omnipy_narrow(PyObject* self, PyObject* args)
  {
    PyObject* pysource;
    char*     repoId;

    if (!PyArg_ParseTuple(args, "Os", &pysource, &repoId))
      return NULL;

    CORBA::Object_ptr cxxsource = (CORBA::Object_ptr)getTwin(pysource);

    omniObject* oosource = cxxsource->PR_getobj();

    omniObject* oodest   = omni::createObjRef(oosource->NP_IRRepositoryId(),
					      repoId,
					      oosource->iopProfiles(), 0);

    CORBA::Object_ptr cxxdest =
      (CORBA::Object_ptr)(oodest->_widenFromTheMostDerivedIntf(0));

    return createPyCorbaObject(repoId, cxxdest);
  }


  ////////////////////////////////////////////////////////////////////////////
  // Python method table                                                    //
  ////////////////////////////////////////////////////////////////////////////

  static PyMethodDef omnipy_methods[] = {

    // omnipy specific things:
    {"checkVersion",      omnipy_checkVersion,      METH_VARARGS},
    {"registerPyObjects", omnipy_registerPyObjects, METH_VARARGS},

    // Wrappers for functions in CORBA::
    {"ORB_init",          omnipy_ORB_init,          METH_VARARGS},

    // Wrappers for functions in CORBA::ORB::
    {"BOA_init",          omnipy_BOA_init,          METH_VARARGS},
    {"stringToObject",    omnipy_stringToObject,    METH_VARARGS},

    // Functions for CORBA objects:
    {"invokeOp",          omnipy_invokeOp,          METH_VARARGS},
    {"releaseProxy",      omnipy_releaseProxy,      METH_VARARGS},
    {"isA",               omnipy_isA,               METH_VARARGS},
    {"nonExistent",       omnipy_nonExistent,       METH_VARARGS},
    {"isEquivalent",      omnipy_isEquivalent,      METH_VARARGS},
    {"hash",              omnipy_hash,              METH_VARARGS},
    {"narrow",            omnipy_narrow,            METH_VARARGS},
    {NULL,NULL}
  };

  void init_omnipy()
  {
    cout << "init_omnipy() called." << endl;
    PyObject *m  = Py_InitModule("_omnipy", omnipy_methods);
    PyObject *d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "omnipyTwinType", (PyObject*)&omnipyTwinType);
    cout << "_omnipy initialised." << endl;
  }
}
