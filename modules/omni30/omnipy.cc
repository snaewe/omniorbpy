// First quick hack at omniORB -- Python interface

// Client side only

// $Id$

// $Log$
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


#include <omniORB2/CORBA.h>
#include <omniORB2/proxyCall.h>
#include <python1.5/Python.h>

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
// Data structures and functions to manage C++ twins of Python objects    //
////////////////////////////////////////////////////////////////////////////

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
getTwin(PyObject* obj)
{
  PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)obj)->in_dict,
				      "_twin");
  return ((omnipyTwin*)ot)->ob_twin;
}

void
remTwin(PyObject* obj)
{
  PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)obj)->in_dict,
				      "_twin");
  Py_DECREF(ot);
}



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



////////////////////////////////////////////////////////////////////////////
// Proxy call descriptor object                                           //
////////////////////////////////////////////////////////////////////////////

class Py_OmniProxyCallDesc : public OmniProxyCallDesc {
public:

  Py_OmniProxyCallDesc(const char* op, size_t len, CORBA::Boolean has_exc,
		       PyObject* in_d,  int in_l,
		       PyObject* out_d, int out_l,
		       PyObject* exc_d, PyObject* args)
    : OmniProxyCallDesc(op, len, has_exc),
      in_d_(in_d),   in_l_(in_l),
      out_d_(out_d), out_l_(out_l),
      exc_d_(exc_d),
      args_(args)
  {
    tstate_ = 0;
    //    cout << "Py_OmniProxyCallDesc created: " << op << " " << len << endl;
  }

  virtual CORBA::ULong alignedSize(CORBA::ULong msgsize);

  virtual void marshalArguments(GIOP_C& giop_client);

  virtual void unmarshalReturnedValues(GIOP_C& giop_client);

  virtual void userException(GIOP_C& giop_client, const char* repoId);

  inline void systemException(const CORBA::SystemException& ex) {
    if (tstate_) PyEval_RestoreThread(tstate_);
    handleSystemException(ex);
  }

  inline PyObject* result() { return result_; }

private:
  // Recursive marshalling functions.

  // Caculate aligned size of the given argument object a_o, which has
  // the type specified by position d within the in_d tuple. Modify d
  // to reflect the next type.
  CORBA::ULong r_alignedSize(CORBA::ULong msgsize,
			     PyObject* in_d, int& d,
			     PyObject* a_o);

  // Marshal the given argument object a_o, which has the type
  // specified by position d within the in_d tuple. Modify d to
  // reflect the next type.
  void r_marshalArguments(GIOP_C&   giop_client,
			  PyObject* in_d, int& d,
			  PyObject* a_o);

  // Unmarshal the next return value, which has the type specified by
  // position d within the out_d tuple. Modify d to reflect the next
  // return type.
  PyObject* r_unmarshalReturnedValues(GIOP_C&   giop_client,
				      PyObject* out_d, int& d);

  // Functions to skip a descriptor:
  void skipInDescriptor(PyObject*  in_d,  int& d);
  void skipOutDescriptor(PyObject* out_d, int& d);

  PyObject*      in_d_;
  int            in_l_;
  PyObject*      out_d_;
  int            out_l_;
  PyObject*      exc_d_;
  PyObject*      args_;
  PyObject*      result_;
  PyThreadState* tstate_;
};




CORBA::ULong
Py_OmniProxyCallDesc::alignedSize(CORBA::ULong msgsize)
{
  int a;
  int d = 0;

  for (a=0; a < in_l_; a++)
    msgsize = r_alignedSize(msgsize, in_d_, d, PyTuple_GET_ITEM(args_,a));

  return msgsize;
}


CORBA::ULong
Py_OmniProxyCallDesc::r_alignedSize(CORBA::ULong msgsize,
				    PyObject*    in_d, int& d,
				    PyObject*    a_o)
{
  CORBA::ULong  tk;
  PyObject*     d_o; // Current descriptor object

  //  cout << "alignedSize() adding to " << msgsize << "..." << endl;

  d_o = PyTuple_GET_ITEM(in_d, d++);
  tk  = PyInt_AS_LONG(d_o);

  switch (tk) {

    // Simple types

  case CORBA::tk_short:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
      msgsize += 2;
    }
    break;

  case CORBA::tk_long:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_ushort:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
      msgsize += 2;
    }
    break;

  case CORBA::tk_ulong:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_float:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_double:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
      msgsize += 8;
    }
    break;

  case CORBA::tk_boolean:
    {
      msgsize += 1;
    }
    break;

  case CORBA::tk_char:
    {
      msgsize += 1;
    }
    break;

  case CORBA::tk_octet:
    {
      msgsize += 1;
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId
    {
      d_o = PyTuple_GET_ITEM(in_d, d++); // IDL specified repoId

      CORBA::Object_ptr obj = (CORBA::Object_ptr)getTwin(a_o);

      if (!obj) throw CORBA::BAD_PARAM();

      msgsize = CORBA::AlignedObjRef(obj,
				     PyString_AS_STRING(d_o),
				     PyString_GET_SIZE(d_o) + 1,
				     msgsize);
    }
    break;

  case CORBA::tk_struct: // (names), descriptors+
    {
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* names    = PyTuple_GET_ITEM(in_d, d++); // Member names
      PyObject* sdict    = ((PyInstanceObject*)a_o)->in_dict;
      int       namecnt  = PyTuple_GET_SIZE(names);

      PyObject* name;
      PyObject* value;

      for (int n=0; n < namecnt; n++) {
	name    = PyTuple_GET_ITEM(names, n);
	value   = PyDict_GetItem(sdict, name);
	msgsize = r_alignedSize(msgsize, in_d, d, value);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::ULong list_len = PyList_GET_SIZE(a_o);
      
      // Check the maximum length
      d_o                  = PyTuple_GET_ITEM(in_d, d++);
      CORBA::ULong max_len = PyInt_AS_LONG(d_o);

      if (max_len > 0 && list_len > max_len) throw CORBA::BAD_PARAM();

      // Space for list length
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;

      if (list_len > 0) {
	int temp = d;
	for (CORBA::ULong i=0; i < list_len; i++) {
	  d = temp; // Reset d to the same typecode
	  msgsize = r_alignedSize(msgsize, in_d, d, PyList_GET_ITEM(a_o, i));
	}
      }
      else
	skipInDescriptor(in_d, d);
    }
    break;


  case CORBA::tk_string: // max_length
    {
      d_o                 = PyTuple_GET_ITEM(in_d, d++);
      CORBA::Long max_len = PyInt_AS_LONG(d_o);

      if (!PyString_Check(a_o)) throw CORBA::BAD_PARAM();

      if (max_len > 0 && PyString_GET_SIZE(a_o) > max_len)
	throw CORBA::BAD_PARAM();

      msgsize  = omni::align_to(msgsize, omni::ALIGN_4);
      msgsize += 4 + PyString_GET_SIZE(a_o) + 1;
    }
    break;

  default:
    cout << "!!! alignedSize(): unsupported typecode: "
	 << (CORBA::ULong)tk << endl;
  }
  //  cout << "  alignedSize() returning " << msgsize << "." << endl;
  return msgsize;
}



void
Py_OmniProxyCallDesc::marshalArguments(GIOP_C& giop_client)
{
  int a;
  int d = 0;

  for (a=0; a< in_l_; a++)
    r_marshalArguments(giop_client, in_d_, d, PyTuple_GET_ITEM(args_,a));

  tstate_ = PyEval_SaveThread();
}


void
Py_OmniProxyCallDesc::r_marshalArguments(GIOP_C&   giop_client,
					 PyObject* in_d, int& d,
					 PyObject* a_o)
{
  CORBA::ULong  tk;
  PyObject*     d_o; // Current descriptor object

  //  cout << "marshalArguments()..." << endl;

  d_o = PyTuple_GET_ITEM(in_d, d++);
  tk  = PyInt_AS_LONG(d_o);

  switch (tk) {

    // Simple types

  case CORBA::tk_short:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::Short s = PyInt_AS_LONG(a_o);
      s >>= giop_client;
    }
    break;

  case CORBA::tk_long:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::Long l = PyInt_AS_LONG(a_o);
      l >>= giop_client;
    }
    break;

  case CORBA::tk_ushort:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::UShort us = PyInt_AS_LONG(a_o);
      us >>= giop_client;
    }
    break;

  case CORBA::tk_ulong:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::ULong ul = PyInt_AS_LONG(a_o);
      ul >>= giop_client;
    }
    break;

  case CORBA::tk_float:
    {
      if (!PyFloat_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::Float f = (CORBA::Float)PyFloat_AS_DOUBLE(a_o);
      f >>= giop_client;
    }
    break;

  case CORBA::tk_double:
    {
      if (!PyFloat_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::Double d = PyFloat_AS_DOUBLE(a_o);
      d >>= giop_client;
    }
    break;

  case CORBA::tk_boolean:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
      b >>= giop_client;
    }
    break;

  case CORBA::tk_char:
    {
      if (!PyString_Check(a_o))        throw CORBA::BAD_PARAM();
      if (PyString_GET_SIZE(a_o) != 1) throw CORBA::BAD_PARAM();

      char *str = PyString_AS_STRING(a_o);

      CORBA::Char c = str[0];
      c >>= giop_client;
    }
    break;

  case CORBA::tk_octet:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::Octet o = PyInt_AS_LONG(a_o);
      o >>= giop_client;
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId
    {
      d_o = PyTuple_GET_ITEM(in_d, d++); // IDL specified repoId

      CORBA::Object_ptr obj = (CORBA::Object_ptr)getTwin(a_o);

      if (!obj) throw CORBA::BAD_PARAM();

      CORBA::MarshalObjRef(obj,
			   PyString_AS_STRING(d_o),
			   PyString_GET_SIZE(d_o) + 1,
			   giop_client);
    }
    break;

  case CORBA::tk_struct: // (names), descriptors+
    {
      PyObject* names   = PyTuple_GET_ITEM(in_d, d++); // Member names
      PyObject* sdict   = ((PyInstanceObject*)a_o)->in_dict;
      int       namecnt = PyTuple_GET_SIZE(names);

      PyObject* name;
      PyObject* value;

      for (int n=0; n < namecnt; n++) {
	name    = PyTuple_GET_ITEM(names, n);
	value   = PyDict_GetItem(sdict, name);
	r_marshalArguments(giop_client, in_d, d, value);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
					  "_v");
      CORBA::ULong e = PyInt_AS_LONG(ev);
      e >>= giop_client;
    }
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      d++; // Skip max length.

      CORBA::ULong list_len = PyList_GET_SIZE(a_o);
      list_len >>= giop_client;

      if (list_len > 0) {
	int temp = d;
	for (CORBA::ULong i=0; i < list_len; i++) {
	  d = temp; // Reset d to the same typecode
	  r_marshalArguments(giop_client, in_d, d, PyList_GET_ITEM(a_o, i));
	}
      }
      else
	skipInDescriptor(in_d, d);
    }
    break;

  case CORBA::tk_string: // max_length
    {
      d++; // Skip max length

      CORBA::Long slen = PyString_GET_SIZE(a_o) + 1;

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
    cout << "!!! marshalArguments(): unsupported typecode: "
	 << (CORBA::ULong)tk << endl;
  }
  //  cout << "  marshalArguments done." << endl;
}
  


void
Py_OmniProxyCallDesc::unmarshalReturnedValues(GIOP_C& giop_client)
{
  PyEval_RestoreThread(tstate_);
  tstate_ = 0;

  int r;
  int d = 0;

  if (out_l_ == 0) {
    Py_INCREF(Py_None);
    result_ = Py_None;
  }
  else if (out_l_ == 1)
    result_ = r_unmarshalReturnedValues(giop_client, out_d_, d);
  else {
    result_ = PyTuple_New(out_l_);
    if (!result_) throw CORBA::NO_MEMORY();

    for (r=0; r < out_l_; r++) {
      PyTuple_SET_ITEM(result_, r,
		       r_unmarshalReturnedValues(giop_client, out_d_, d));
    }
  }
}



PyObject*
Py_OmniProxyCallDesc::r_unmarshalReturnedValues(GIOP_C&   giop_client,
						PyObject* out_d, int& d)
{
  CORBA::ULong  tk;
  PyObject*     r_o = 0;	// Current result object
  PyObject*     d_o;		// Current descriptor object

  d_o = PyTuple_GET_ITEM(out_d, d++);
  tk  = PyInt_AS_LONG(d_o);

  switch (tk) {

  case CORBA::tk_short:
    {
      CORBA::Short s;
      s <<= giop_client;
      r_o = PyInt_FromLong(s);
    }
    break;

  case CORBA::tk_long:
    {
      CORBA::Long l;
      l <<= giop_client;
      r_o = PyInt_FromLong(l);
    }
    break;

  case CORBA::tk_ushort:
    {
      CORBA::UShort us;
      us <<= giop_client;
      r_o = PyInt_FromLong(us);
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul;
      ul <<= giop_client;
      r_o = PyInt_FromLong(ul);
    }
    break;

  case CORBA::tk_float:
    {
      CORBA::Float f;
      f <<= giop_client;
      r_o = PyFloat_FromDouble((double)f);
    }
    break;

  case CORBA::tk_double:
    {
      CORBA::Double d;
      d <<= giop_client;
      r_o = PyFloat_FromDouble(d);
    }
    break;

  case CORBA::tk_boolean:
    {
      CORBA::Boolean b;
      b <<= giop_client;
      r_o = PyInt_FromLong(b);
    }
    break;

  case CORBA::tk_char:
    {
      CORBA::Char c;
      c <<= giop_client;

      char* str = new char[2];
      str[0]    = c;
      str[1]    = '\0';
      r_o       = PyString_FromStringAndSize(str, 1);
      delete [] str;
    }
    break;

  case CORBA::tk_octet:
    {
      CORBA::Octet o;
      o <<= giop_client;
      r_o = PyInt_FromLong(o);
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId
    {
      d_o                   = PyTuple_GET_ITEM(out_d, d++);
      char*  targetRepoId   = PyString_AS_STRING(d_o);

      CORBA::Object_ptr obj = CORBA::UnMarshalObjRef(targetRepoId,
						     giop_client);
      r_o = createPyCorbaObject(targetRepoId, obj);
    }
    break;

  case CORBA::tk_struct: // struct_class, count, descriptors+
    {
      PyObject* strclass = PyTuple_GET_ITEM(out_d, d++);
      d_o                = PyTuple_GET_ITEM(out_d, d++);
      int       memcnt   = PyInt_AS_LONG(d_o);

      PyObject* strtuple = PyTuple_New(memcnt);

      for (int m=0; m < memcnt; m++) {
	PyTuple_SET_ITEM(strtuple, m,
			 r_unmarshalReturnedValues(giop_client, out_d, d));
      }
      r_o = PyEval_CallObject(strclass, strtuple);
      Py_DECREF(strtuple);
    }
    break;

  case CORBA::tk_enum: // list enum_items
    {
      d_o = PyTuple_GET_ITEM(out_d, d++);

      CORBA::ULong e;
      e <<= giop_client;
	
      PyObject* ev = PyList_GET_ITEM(d_o, e);
      Py_INCREF(ev);
      r_o = ev;
    }
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      d_o                  = PyTuple_GET_ITEM(out_d, d++);
      CORBA::ULong max_len = PyInt_AS_LONG(d_o);

      CORBA::ULong seq_len;
      seq_len <<= giop_client;

      if (max_len > 0 && seq_len > max_len) throw CORBA::BAD_PARAM();

      r_o = PyList_New(seq_len);

      if (seq_len > 0) {
	int temp = d;
	for (CORBA::ULong i=0; i < seq_len; i++) {
	  d = temp;
	  PyList_SET_ITEM(r_o, i,
			  r_unmarshalReturnedValues(giop_client, out_d, d));
	}
      }
      else
	skipOutDescriptor(out_d, d);
    }
    break;

  case CORBA::tk_string: // max_length
    {
      d_o                 = PyTuple_GET_ITEM(out_d, d++);
      CORBA::Long max_len = PyInt_AS_LONG(d_o);

      CORBA::String_member str_tmp;
      str_tmp <<= giop_client;

      r_o = PyString_FromString(str_tmp._ptr);

      if (max_len > 0 && PyString_GET_SIZE(r_o) > max_len)
	throw CORBA::BAD_PARAM();
    }
    break;

  default:
    cout << " !!! unmarshalReturnedValues(): unsupported typecode: "
	 << (CORBA::ULong)tk << endl;
  }
  return r_o;
}


void
Py_OmniProxyCallDesc::userException(GIOP_C& giop_client, const char* repoId)
{
  PyEval_RestoreThread(tstate_);
  tstate_ = 0;

  PyObject* o;
  PyObject* exc_t = PyDict_GetItemString(exc_d_, repoId);

  if (exc_t) {
    PyObject* exc_c = PyTuple_GET_ITEM(exc_t, 0);
    PyObject* exc_d = PyTuple_GET_ITEM(exc_t, 1);
    o               = PyTuple_GET_ITEM(exc_t, 2);
    int       exc_l = PyInt_AS_LONG(o);

    PyObject* exc_a;

    exc_a = PyTuple_New(exc_l);
    if (!exc_a) throw CORBA::NO_MEMORY();

    int r;
    int d = 0;

    for (r=0; r < exc_l; r++) {
      PyTuple_SET_ITEM(exc_a, r,
		       r_unmarshalReturnedValues(giop_client, exc_d, d));
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

void
Py_OmniProxyCallDesc::skipInDescriptor(PyObject* in_d, int& d)
{
  CORBA::ULong  tk;
  PyObject*     d_o; // Current descriptor object

  d_o = PyTuple_GET_ITEM(in_d, d++);
  tk  = PyInt_AS_LONG(d_o);

  switch (tk) {

  case CORBA::tk_short:
  case CORBA::tk_long:
  case CORBA::tk_ushort:
  case CORBA::tk_ulong:
  case CORBA::tk_float:
  case CORBA::tk_double:
  case CORBA::tk_boolean:
  case CORBA::tk_char:
  case CORBA::tk_octet:
    break;

  case CORBA::tk_objref: // repoId
    d++; break;

  case CORBA::tk_struct: // (names), descriptors+
    {
      PyObject* names    = PyTuple_GET_ITEM(in_d, d++); // Member names
      int       namecnt  = PyTuple_GET_SIZE(names);

      for (int n=0; n < namecnt; n++) {
	skipInDescriptor(in_d, d);
      }
    }
    break;

  case CORBA::tk_enum:
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      d++;
      skipInDescriptor(in_d, d);
    }
    break;

  case CORBA::tk_string: // max_length
    d++; break;

  default:
    cout << "!!! skipInDescriptor(): unsupported typecode: "
	 << tk << endl;
  }
}


void
Py_OmniProxyCallDesc::skipOutDescriptor(PyObject* out_d, int& d)
{
  CORBA::ULong  tk;
  PyObject*     d_o; // Current descriptor object

  d_o = PyTuple_GET_ITEM(out_d, d++);
  tk  = PyInt_AS_LONG(d_o);

  switch (tk) {

  case CORBA::tk_short:
  case CORBA::tk_long:
  case CORBA::tk_ushort:
  case CORBA::tk_ulong:
  case CORBA::tk_float:
  case CORBA::tk_double:
  case CORBA::tk_boolean:
  case CORBA::tk_char:
  case CORBA::tk_octet:
    break;

  case CORBA::tk_objref: // repoId
    d++; break;

  case CORBA::tk_struct: // struct_class, count, descriptors+
    {
      d++;
      d_o     = PyTuple_GET_ITEM(out_d, d++);
      int cnt = PyTuple_GET_SIZE(d_o);

      for (int n=0; n < cnt; n++) {
	skipOutDescriptor(out_d, d);
      }
    }
    break;

  case CORBA::tk_enum:
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      d++;
      skipOutDescriptor(out_d, d);
    }
    break;

  case CORBA::tk_string: // max_length
    d++; break;

  default:
    cout << "!!! skipOutDescriptor(): unsupported typecode: "
	 << tk << endl;
  }
}




// Things visible to Python:

extern "C" {

  ////////////////////////////////////////////////////////////////////////////
  // omnipy private functions                                               //
  ////////////////////////////////////////////////////////////////////////////

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
    //  (proxy, op_name, (in_desc, in_len, out_desc, out_len, exc_desc), args)
    //
    //  exc_desc is a dictionary containing a mapping from repoIds to
    //  tuples of the form (exception class, marshal desc., param count)

    PyObject *pyproxy, *in_d, *out_d, *exc_d, *op_args, *result;
    char*  op;
    size_t op_len, in_l, out_l;

    /*
    if (!PyArg_ParseTuple(args, "Os#(OiOiO)O", &pyproxy, &op, &op_len,
			  &in_d, &in_l, &out_d, &out_l, &exc_d, &op_args))
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
    o       = PyTuple_GET_ITEM(desc,1);
    in_l    = PyInt_AS_LONG(o);
    out_d   = PyTuple_GET_ITEM(desc,2);
    o       = PyTuple_GET_ITEM(desc,3);
    out_l   = PyInt_AS_LONG(o);
    exc_d   = PyTuple_GET_ITEM(desc,4);
    op_args = PyTuple_GET_ITEM(args,3);

    if ((size_t)(PyTuple_GET_SIZE(op_args)) != in_l) {
      char* err = new char[160];
      snprintf(err, 159, "%s requires %d argument%s; %d given",
	       op, in_l, (in_l == 1) ? "" : "s", PyTuple_GET_SIZE(op_args));

      PyErr_SetString(PyExc_TypeError, err);
      delete [] err;
      return 0;
    }

    CORBA::Object_ptr cxxproxy = (CORBA::Object_ptr)getTwin(pyproxy);

    CORBA::Boolean has_exc = (exc_d != Py_None);

    Py_OmniProxyCallDesc call_desc(op, op_len + 1, has_exc,
				   in_d, in_l, out_d, out_l, exc_d,
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
