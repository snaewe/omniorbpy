// First quick hack at omniORB -- Python interface

// Client side only

// $Id$

// $Log$
// Revision 1.1  1999/06/01 11:44:49  dpg1
// Initial revision
//


#include <omniORB2/CORBA.h>
#include <omniORB2/proxyCall.h>
#include <python1.5/Python.h>

#include <iostream.h>


// Python object to refer to C++ twin objects:

struct omnipyTwin {
  PyObject_VAR_HEAD
  void* ob_twin;
};

extern "C" {

  static void
  omnipyTwin_dealloc(omnipyTwin *tp)
  {
    cout << "Deleting omnipyTwin..." << endl;
    PyMem_DEL(tp);
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
      0,		                /*tp_compare*/
      0,                                /*tp_repr*/
      0,				/*tp_as_number*/
      0,                                /*tp_as_sequence*/
      0,				/*tp_as_mapping*/
      0, 				/*tp_hash*/
      0,				/*tp_call*/
      0,				/*tp_str*/
      0,				/*tp_getattro*/
      0,				/*tp_setattro*/
      0,                                /*tp_as_buffer*/
      0,				/*tp_xxx4*/
      0,                                /*tp_doc*/
      };
}

// Functions to manage twins:

void
setTwin(PyObject* obj, void* twin)
{
  omnipyTwin* ot = PyMem_NEW(omnipyTwin, 1);

  ot->ob_type = &omnipyTwinType;
  ot->ob_size = 0;
  ot->ob_twin = (void*)twin;
  _Py_NewReference(ot);

  PyDict_SetItemString(((PyInstanceObject*)obj)->in_dict,
		       "_twin", (PyObject*)ot);
}

inline void*
getTwin(PyObject*obj)
{
  PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)obj)->in_dict,
				      "_twin");
  return ((omnipyTwin*)ot)->ob_twin;
}






// Proxy object

class _proxy__Py : public virtual omniObject, public virtual CORBA::Object {
public:

  _proxy__Py(Rope*                   r,
	     CORBA::Octet*           key,
	     size_t                  keysize,
	     IOP::TaggedProfileList* profiles,
	     CORBA::Boolean          release,
	     const char*             repoId,
	     PyObject*               twin);

  virtual ~_proxy__Py();

  PyObject* get_twin() {
    return twin_;
  }

  virtual void *_widenFromTheMostDerivedIntf(const char*    repoId,
					     CORBA::Boolean is_cxx_type_id);
private:
  const char* repoId_;
  PyObject*   twin_;
};


// Proxy object factory to be registered with the ORB:

class _Py_proxyObjectFactory : public proxyObjectFactory {

public:

  _Py_proxyObjectFactory(const char *repoId, PyObject *twin);
  virtual ~_Py_proxyObjectFactory();

  virtual const char *irRepoId() const;

  virtual CORBA::Object_ptr newProxyObject(Rope*                   r,
					   CORBA::Octet*           key,
					   size_t                  keysize,
					   IOP::TaggedProfileList* profiles,
					   CORBA::Boolean          release);

  virtual CORBA::Boolean is_a(const char *base_repoId) const;

  // _nil() ?

private:
  char*     repoId_;
  PyObject* twin_;
};


// OmniProxyCallDesc object to do marshalling:

class Py_OmniProxyCallDesc : public OmniProxyCallDesc {
public:

  Py_OmniProxyCallDesc(const char* op, size_t len, CORBA::Boolean has_exc,
		       PyObject* in_d, PyObject* out_d, size_t out_l,
		       PyObject* exc_d, PyObject* args)
    : OmniProxyCallDesc(op, len, has_exc),
      in_d_(in_d), out_d_(out_d), out_l_(out_l), exc_d_(exc_d), args_(args)
  {
    //    cout << "Py_OmniProxyCallDesc created: " << op << " " << len << endl;
  }

  virtual CORBA::ULong alignedSize(CORBA::ULong msgsize) {
    return r_alignedSize(msgsize, in_d_, args_);
  }

  virtual void marshalArguments(GIOP_C& giop_client) {
    r_marshalArguments(giop_client, in_d_, args_);
  }

  virtual void unmarshalReturnedValues(GIOP_C& giop_client) {
    result_ = r_unmarshalReturnedValues(giop_client, out_d_, out_l_);
  }

  virtual void userException(GIOP_C& giop_client, const char* repoId);

  inline PyObject* result() { return result_; }

private:
  // Recursive marshalling functions:
  CORBA::ULong r_alignedSize(CORBA::ULong msgsize,
			     PyObject* in_d, PyObject* args);

  void r_marshalArguments(GIOP_C& giop_client,
			  PyObject* in_d, PyObject* args);

  PyObject* r_unmarshalReturnedValues(GIOP_C& giop_client,
				      PyObject* out_d,
				      size_t    r_l);
  PyObject* in_d_;
  PyObject* out_d_;
  size_t    out_l_;
  PyObject* exc_d_;
  PyObject* args_;
  PyObject* result_;
};



// Member function implementations:

_proxy__Py::_proxy__Py(Rope*                   r,
		       CORBA::Octet*           key,
		       size_t                  keysize,
		       IOP::TaggedProfileList* profiles,
		       CORBA::Boolean          release,
		       const char*             repoId,
		       PyObject*               twin)
  : omniObject(repoId, r, key, keysize, profiles, release),
    repoId_(repoId), twin_(twin)
{
  this->PR_setobj(this);
  omni::objectIsReady(this);
  cout << "_proxy__Py created. repoId=" << repoId << endl;
}


_proxy__Py::~_proxy__Py()
{
  cout << "_proxy__Py deleted." << endl;
}


void*
_proxy__Py::_widenFromTheMostDerivedIntf(const char*    repoId,
					 CORBA::Boolean is_cxx_type_id)
{
  cout << "_widen...(): " << repoId << endl;

  if (is_cxx_type_id) {
    cout << "is_cxx_type_id..." << endl;
    if (strcmp(repoId, "_proxy__Py") == 0)
      return (void*)this;
    else
      return 0;
  }

  if (!repoId)
    return (void*)((CORBA::Object_ptr)this);

  if (strcmp(repoId, repoId_) == 0)
    return (void*)this;

  return 0;
}


_Py_proxyObjectFactory::_Py_proxyObjectFactory(const char* repoId,
					       PyObject*   twin)
{
  repoId_ = CORBA::string_dup(repoId);
  Py_INCREF(twin);
  twin_ = twin;
  cout << "_Py_proxyObjectFactory created. repoId=" << repoId << endl;
}

_Py_proxyObjectFactory::~_Py_proxyObjectFactory()
{
  Py_DECREF(twin_);
  CORBA::string_free(repoId_);
  cout << "_Py_proxyObjectFactory deleted." << endl;
}

CORBA::Boolean
_Py_proxyObjectFactory::is_a(const char *base_repoId) const
{
  cout << "is_a: " << base_repoId << " =? " << repoId_ << endl;
  if (strcmp(base_repoId, repoId_) == 0)
    return 1;
  else
    return 0;
}

const char*
_Py_proxyObjectFactory::irRepoId() const
{
  return (const char*)repoId_;
}


CORBA::Object_ptr
_Py_proxyObjectFactory::newProxyObject(Rope*                   r,
				       CORBA::Octet*           key,
				       size_t                  keysize,
				       IOP::TaggedProfileList* profiles,
				       CORBA::Boolean          release)
{
  cout << "newProxyObject() called." << endl;

  PyObject* arglist = Py_BuildValue("()");
  PyObject* pyproxy = PyEval_CallObject(twin_, arglist);
  Py_DECREF(arglist);

  assert(PyInstance_Check(pyproxy));

  _proxy__Py* p = new _proxy__Py(r, key, keysize, profiles, release,
				 repoId_, pyproxy);
  if (!p)
    throw CORBA::NO_MEMORY(0,CORBA::COMPLETED_NO);

  /*
  omnipyTwin* tp = PyMem_NEW(omnipyTwin, 1);

  tp->ob_type = &omnipyTwinType;
  tp->ob_size = 0;
  tp->ob_twin = (void*)p;
  _Py_NewReference(tp);

  PyDict_SetItemString(((PyInstanceObject*)pyproxy)->in_dict,
		       "_twin", (PyObject*)tp);
  */
  setTwin(pyproxy, p);

  return (CORBA::Object_ptr)p;
}


CORBA::ULong
Py_OmniProxyCallDesc::r_alignedSize(CORBA::ULong msgsize,
				    PyObject*    in_d,
				    PyObject*    args)
{
  size_t        a, d, a_l, d_l;
  CORBA::TCKind tk;
  PyObject*     a_o; // Current argument object
  PyObject*     d_o; // Current descriptor object

  //  cout << "alignedSize() adding to " << msgsize << "..." << endl;

  a_l = PyTuple_Size(args);
  d_l = PyTuple_Size(in_d);

  for (a=0, d=0; a < a_l; a++) {
    assert(d < d_l);

    //    cout << "  processing arg " << a << "..." << endl;

    d_o = PyTuple_GET_ITEM(in_d, d++);
    tk  = (CORBA::TCKind)PyInt_AS_LONG(d_o);

    switch (tk) {
    case CORBA::tk_string:
      //      cout << "    string." << endl;
      {
	CORBA::Long max_len;
	d_o     = PyTuple_GET_ITEM(in_d, d++);
	max_len = PyInt_AS_LONG(d_o);

	a_o = PyTuple_GET_ITEM(args, a);
	assert(PyString_Check(a_o));

	msgsize  = omni::align_to(msgsize, omni::ALIGN_4);
	msgsize += 4 + PyString_GET_SIZE(a_o) + 1;
      }
      break;

    default:
      cout << "    unsupported typecode: " << (CORBA::ULong)tk << endl;
    }
  }
  //  cout << "  alignedSize() returning " << msgsize << "." << endl;
  return msgsize;
}



void
Py_OmniProxyCallDesc::r_marshalArguments(GIOP_C&   giop_client,
					 PyObject* in_d,
					 PyObject* args)
{
  size_t        a, d, a_l, d_l;
  CORBA::TCKind tk;
  PyObject*     a_o; // Current argument object
  PyObject*     d_o; // Current descriptor object

  //  cout << "marshalArguments()..." << endl;

  a_l = PyTuple_Size(args);
  d_l = PyTuple_Size(in_d);

  for (a=0, d=0; a < a_l; a++) {
    assert(d < d_l);

    //    cout << "  processing arg " << a << "..." << endl;

    a_o = PyTuple_GET_ITEM(args, a);
    d_o = PyTuple_GET_ITEM(in_d, d++);
    tk  = (CORBA::TCKind)PyInt_AS_LONG(d_o);

    switch (tk) {
    case CORBA::tk_string:
      //      cout << "    string." << endl;
      {
	CORBA::Long max_len;
	d_o     = PyTuple_GET_ITEM(in_d, d++);
	max_len = PyInt_AS_LONG(d_o);

	assert(PyString_Check(a_o));

	CORBA::Long slen = PyString_GET_SIZE(a_o) + 1;

	if (max_len > 0 && slen > max_len) {
	  //	  cout << "  string longer than max length!" << endl;
	}

	slen >>= giop_client;

	if (slen > 1) {
	  char* str = PyString_AS_STRING(a_o);
	  giop_client.put_char_array((const CORBA::Char*)((const char*)str),
				     slen);
	}
	else {
	  CORBA::Char('\0') >>= giop_client;
	}
      }
      break;

    default:
      cout << "    unsupported typecode: " << (CORBA::ULong)tk << endl;
    }
  }
  //  cout << "  marshalArguments done." << endl;
}
  

PyObject*
Py_OmniProxyCallDesc::r_unmarshalReturnedValues(GIOP_C& giop_client,
						PyObject* out_d,
						size_t    r_l)
{
  size_t        r, d, d_l;
  CORBA::TCKind tk;
  PyObject*     r_o = 0;	// Current result object
  PyObject*     d_o;		// Current descriptor object
  PyObject*     result = 0;	// Complete result object

  //  cout << "unmarshalReturnedValues()..." << endl;

  if (r_l == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  d_l = PyTuple_Size(out_d);

  if (r_l > 1)
    result = PyTuple_New(r_l);

  for (r=0,d=0; r < r_l; r++) {
    assert(d < d_l);

    //    cout << "  processing result " << r << "..." << endl;

    d_o = PyTuple_GET_ITEM(out_d, d++);
    tk  = (CORBA::TCKind)PyInt_AS_LONG(d_o);

    switch (tk) {
    case CORBA::tk_string:
      //      cout << "    string." << endl;
      {
	CORBA::Long max_len;
	d_o     = PyTuple_GET_ITEM(out_d, d++);
	max_len = PyInt_AS_LONG(d_o);

	CORBA::String_member str_tmp;
	str_tmp <<= giop_client;

	r_o = PyString_FromString(str_tmp._ptr);

	if (max_len > 0 && PyString_GET_SIZE(r_o) > max_len) {
	  //	  cout << "  string longer than max length!" << endl;
	}
      }
      break;

    default:
      cout << "    unsupported typecode: " << (CORBA::ULong)tk << endl;
    }
    if (r_l > 1)
      PyTuple_SET_ITEM(result, r, r_o);
    else
      result = r_o;
  }
  //  cout << "  unmarshalReturnedValues done." << endl;
  return result;
}


void
Py_OmniProxyCallDesc::userException(GIOP_C& giop_client, const char* repoId)
{
  PyObject* o;
  PyObject* exc_t = PyDict_GetItemString(exc_d_, repoId);

  if (exc_t) {
    PyObject* exc_c = PyTuple_GET_ITEM(exc_t, 0);
    PyObject* exc_d = PyTuple_GET_ITEM(exc_t, 1);
    o               = PyTuple_GET_ITEM(exc_t, 2);
    size_t    exc_l = PyInt_AS_LONG(o);

    PyObject* exc_a;

    if (exc_l == 0)
      exc_a = PyTuple_New(0);
    else {
      o = r_unmarshalReturnedValues(giop_client, exc_d, exc_l);

      if (exc_l == 1) {
	exc_a = PyTuple_New(1);
	PyTuple_SET_ITEM(exc_a, 0, o);
      }
      else
	exc_a = o;
    }
    giop_client.RequestCompleted();

    PyObject* exc_i = PyEval_CallObject(exc_c, exc_a);
    Py_DECREF(exc_a);
    PyErr_SetObject(exc_c, exc_i);
    throw CORBA::UserException();
  }
  else {
    giop_client.RequestCompleted(1);
    throw CORBA::MARSHAL(0, CORBA::COMPLETED_MAYBE);
  }
}


// Things initialised just once:

PyObject* pyCORBAmodule;
PyObject* pyCORBAmoduleDict;
PyObject* pyCORBAsysExcMap;


// Things visible to Python:

extern "C" {

  ////////////////////////////////////////////////////////////////////////////
  // omnipy private functions                                               //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_registerPyObjects(PyObject* self, PyObject* args)
  {
    if (!PyArg_ParseTuple(args, "O", &pyCORBAmodule))
      return NULL;

    if (!PyModule_Check(pyCORBAmodule)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 1: parameter must be a module");
      return NULL;
    }
    pyCORBAmoduleDict = PyModule_GetDict(pyCORBAmodule);
    pyCORBAsysExcMap  = PyDict_GetItemString(pyCORBAmoduleDict,
					     "__sysExceptionMapping");

    cout << "Python objects registered." << endl;

    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject*
  omnipy_registerProxyFactory(PyObject* self, PyObject* args)
  {
    char*     repoId;
    PyObject* pyproxy;

    if (!PyArg_ParseTuple(args, "sO", &repoId, &pyproxy))
      return NULL;

    if (!PyCallable_Check(pyproxy)) {
      PyErr_SetString(PyExc_TypeError,
		      "argument 2: parameter must be callable");
      return NULL;
    }

    _Py_proxyObjectFactory* pof = new _Py_proxyObjectFactory(repoId, pyproxy);

    cout << "Proxy factory registered." << endl;

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

    /*
    omnipyTwin* tp = PyMem_NEW(omnipyTwin, 1);

    tp->ob_type = &omnipyTwinType;
    tp->ob_size = 0;
    tp->ob_twin = (void*)orb;
    _Py_NewReference(tp);

    PyDict_SetItemString(((PyInstanceObject*)pyorb)->in_dict,
			 "_twin", (PyObject*)tp);
    */
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

    /*
    PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)pyorb)->in_dict,
					"_twin");
    CORBA::ORB_ptr orb = (CORBA::ORB_ptr)(((omnipyTwin*)ot)->ob_twin);
    */
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

    /*
    omnipyTwin* tp = PyMem_NEW(omnipyTwin, 1);

    tp->ob_type = &omnipyTwinType;
    tp->ob_size = 0;
    tp->ob_twin = (void*)boa;
    _Py_NewReference(tp);

    PyDict_SetItemString(((PyInstanceObject*)pyboa)->in_dict,
			 "_twin", (PyObject*)tp);
    */
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

    if (CORBA::is_nil(obj))
      cout << "Obj is nil." << endl;
    else
      cout << "Obj is not nil." << endl;

    _proxy__Py* pp = (_proxy__Py*)
      (obj->PR_getobj()->_widenFromTheMostDerivedIntf("_proxy__Py", 1));

    cout << "Widened..." << endl;

    assert(pp);

    PyObject* twin = pp->get_twin();

    //    Py_INCREF(twin);
    return twin;
  }


  ////////////////////////////////////////////////////////////////////////////
  // Functions operating on proxy objects                                   //
  ////////////////////////////////////////////////////////////////////////////

  static PyObject*
  omnipy_invokeOp(PyObject* self, PyObject* args)
  {
    // Arg format
    //  (proxy, op_name, (in_desc, out_desc, out_len, exc_desc), args)
    //
    //  exc_desc is a dictionary containing a mapping from repoIds to
    //  tuples of the form (exception class, marshal desc., param count)

    PyObject *pyproxy, *in_d, *out_d, *exc_d, *op_args, *result;
    char*  op;
    size_t op_len, out_l;

    /*
    if (!PyArg_ParseTuple(args, "Os#(OOiO)O", &pyproxy, &op, &op_len,
			  &in_d, &out_d, &out_l, &exc_d, &op_args))
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
    o       = PyTuple_GET_ITEM(desc,2);
    out_l   = PyInt_AS_LONG(o);
    exc_d   = PyTuple_GET_ITEM(desc,3);
    op_args = PyTuple_GET_ITEM(args,3);

    /*
    PyObject* pt = PyDict_GetItemString(((PyInstanceObject*)pyproxy)->in_dict,
					"_twin");

    _proxy__Py* proxy = (_proxy__Py*)(((omnipyTwin*)pt)->ob_twin);
    */
    _proxy__Py* proxy = (_proxy__Py*)getTwin(pyproxy);

    CORBA::Boolean has_exc = (exc_d != Py_None);

    Py_OmniProxyCallDesc call_desc(op, op_len + 1, has_exc,
				   in_d, out_d, out_l, exc_d,
				   op_args);
    try {
      OmniProxyCallWrapper::invoke(proxy, call_desc);
      return call_desc.result();
    }
    catch (const CORBA::SystemException& ex) {
      PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					    ex.NP_RepositoryId());

      PyObject* exca = Py_BuildValue("(ii)", ex.minor(), ex.completed());
      PyObject* exci = PyEval_CallObject(excc, exca);
      Py_DECREF(exca);
      PyErr_SetObject(excc, exci);
    }
    catch (const CORBA::UserException& ex) {
    }
    return 0;
  }

  static PyObject*
  omnipy_releaseProxy(PyObject* self, PyObject* args)
  {
    PyObject *pyproxy;

    if (!PyArg_ParseTuple(args, "O", &pyproxy))
      return NULL;

    /*
    PyObject* pt = PyDict_GetItemString(((PyInstanceObject*)pyproxy)->in_dict,
					"_twin");
    _proxy__Py* proxy = (_proxy__Py*)(((omnipyTwin*)pt)->ob_twin);
    */
    _proxy__Py* proxy = (_proxy__Py*)getTwin(pyproxy);

    CORBA::release(proxy);
    cout << "Proxy released." << endl;

    Py_INCREF(Py_None);
    return Py_None;
  }


  static PyMethodDef omnipy_methods[] = {

    // omnipy specific things:
    {"registerPyObjects", omnipy_registerPyObjects, METH_VARARGS},
    {"registerProxyFactory", omnipy_registerProxyFactory, METH_VARARGS},

    // Wrappers for functions in CORBA::
    {"ORB_init", omnipy_ORB_init, METH_VARARGS},

    // Wrappers for functions in CORBA::ORB::
    {"BOA_init", omnipy_BOA_init, METH_VARARGS},
    {"stringToObject", omnipy_stringToObject, METH_VARARGS},

    // Functions for proxy objects
    {"invokeOp", omnipy_invokeOp, METH_VARARGS},
    {"releaseProxy", omnipy_releaseProxy, METH_VARARGS},
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
