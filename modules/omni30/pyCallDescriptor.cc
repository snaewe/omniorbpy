// -*- c++ -*-

// Implementation of Python proxy call wrapper object

// $Id$

// $Log$
// Revision 1.1  1999/06/07 10:11:02  dpg1
// Initial revision
//

#include <omnipy.h>
#include <pyProxyCallWrapper.h>


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
