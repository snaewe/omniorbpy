// -*- c++ -*-

// Implementation of Python proxy call wrapper object

// $Id$

// $Log$
// Revision 1.2  1999/06/07 14:58:20  dpg1
// Descriptors unflattened again.
//
// Revision 1.1  1999/06/07 10:11:02  dpg1
// Initial revision
//

#include <omnipy.h>
#include <pyProxyCallWrapper.h>


CORBA::ULong
Py_OmniProxyCallDesc::alignedSize(CORBA::ULong msgsize)
{
  for (int i=0; i < in_l_; i++)
    msgsize = r_alignedSize(msgsize,
			    PyTuple_GET_ITEM(in_d_,i),
			    PyTuple_GET_ITEM(args_,i));
  return msgsize;
}


CORBA::ULong
Py_OmniProxyCallDesc::r_alignedSize(CORBA::ULong msgsize,
				    PyObject*    d_o,
				    PyObject*    a_o)
{
  CORBA::ULong   tk;
  CORBA::Boolean tup;
  PyObject*      t_o; // Temporary object

  //  cout << "alignedSize() adding to " << msgsize << "..." << endl;

  if (PyTuple_Check(d_o)) {
    t_o = PyTuple_GET_ITEM(d_o, 0);
    assert(PyInt_Check(t_o));
    tk  = PyInt_AS_LONG(t_o);
    tup = 1;
  }
  else {
    assert(PyInt_Check(d_o));
    tk  = PyInt_AS_LONG(d_o);
    tup = 0;
  }

  switch (tk) {

    // Simple types

  case CORBA::tk_short:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
      msgsize += 2;
    }
    break;

  case CORBA::tk_long:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_ushort:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
      msgsize += 2;
    }
    break;

  case CORBA::tk_ulong:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_float:
    {
      if (!PyFloat_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_double:
    {
      if (!PyFloat_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
      msgsize += 8;
    }
    break;

  case CORBA::tk_boolean:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize += 1;
    }
    break;

  case CORBA::tk_char:
    {
      if (!PyString_Check(a_o))        throw CORBA::BAD_PARAM();
      if (PyString_GET_SIZE(a_o) != 1) throw CORBA::BAD_PARAM();
      msgsize += 1;
    }
    break;

  case CORBA::tk_octet:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize += 1;
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      assert(PyString_Check(t_o));

      CORBA::Object_ptr obj = (CORBA::Object_ptr)getTwin(a_o);

      if (!obj) throw CORBA::BAD_PARAM();

      msgsize = CORBA::AlignedObjRef(obj,
				     PyString_AS_STRING(t_o),
				     PyString_GET_SIZE(t_o) + 1,
				     msgsize);
    }
    break;

  case CORBA::tk_struct: // name, descriptor, name, descriptor, ...
    {
      assert(tup);
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

      // The descriptor tuple has twice the number of struct members,
      // plus one for the typecode kind.
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 1) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=1; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	assert(PyString_Check(name));
	value   = PyDict_GetItem(sdict, name);
	msgsize = r_alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      assert(tup);
      if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::ULong list_len = PyList_GET_SIZE(a_o);

      t_o                  = PyTuple_GET_ITEM(d_o, 1);
      assert(PyInt_Check(t_o));
      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      if (max_len > 0 && list_len > max_len) throw CORBA::BAD_PARAM();

      // Space for list length
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;

      t_o = PyTuple_GET_ITEM(d_o, 2);

      if (list_len > 0) {
	for (CORBA::ULong i=0; i < list_len; i++) {
	  msgsize = r_alignedSize(msgsize, t_o, PyList_GET_ITEM(a_o, i));
	}
      }
    }
    break;

  case CORBA::tk_array: // length, element_desc
    {
      assert(tup);
      if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();

      CORBA::ULong list_len = PyList_GET_SIZE(a_o);
      
      // Check the length
      t_o                  = PyTuple_GET_ITEM(d_o, 1);

      assert(PyInt_Check(t_o));

      CORBA::ULong arr_len = PyInt_AS_LONG(t_o);

      if (list_len != arr_len) throw CORBA::BAD_PARAM();

      t_o = PyTuple_GET_ITEM(d_o, 2);

      for (CORBA::ULong i=0; i < list_len; i++) {
	msgsize = r_alignedSize(msgsize, t_o, PyList_GET_ITEM(a_o, i));
      }
    }
    break;

  case CORBA::tk_string: // max_length
    {
      assert(tup);
      t_o                 = PyTuple_GET_ITEM(d_o, 1);
      assert(PyInt_Check(t_o));

      CORBA::Long max_len = PyInt_AS_LONG(t_o);

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
    abort();
  }
  //  cout << "  alignedSize() returning " << msgsize << "." << endl;
  return msgsize;
}



void
Py_OmniProxyCallDesc::marshalArguments(GIOP_C& giop_client)
{
  for (int i=0; i < in_l_; i++)
    r_marshalArguments(giop_client,
		       PyTuple_GET_ITEM(in_d_,i),
		       PyTuple_GET_ITEM(args_,i));

  tstate_ = PyEval_SaveThread();
}


void
Py_OmniProxyCallDesc::r_marshalArguments(GIOP_C&   giop_client,
					 PyObject* d_o,
					 PyObject* a_o)
{
  CORBA::ULong   tk;
  PyObject*      t_o; // Temp object

  //  cout << "marshalArguments()..." << endl;

  if (PyTuple_Check(d_o)) {
    t_o = PyTuple_GET_ITEM(d_o, 0);
    tk  = PyInt_AS_LONG(t_o);
  }
  else
    tk  = PyInt_AS_LONG(d_o);

  switch (tk) {

    // Simple types

  case CORBA::tk_short:
    {
      CORBA::Short s = PyInt_AS_LONG(a_o);
      s >>= giop_client;
    }
    break;

  case CORBA::tk_long:
    {
      CORBA::Long l = PyInt_AS_LONG(a_o);
      l >>= giop_client;
    }
    break;

  case CORBA::tk_ushort:
    {
      CORBA::UShort us = PyInt_AS_LONG(a_o);
      us >>= giop_client;
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul = PyInt_AS_LONG(a_o);
      ul >>= giop_client;
    }
    break;

  case CORBA::tk_float:
    {
      CORBA::Float f = (CORBA::Float)PyFloat_AS_DOUBLE(a_o);
      f >>= giop_client;
    }
    break;

  case CORBA::tk_double:
    {
      CORBA::Double d = PyFloat_AS_DOUBLE(a_o);
      d >>= giop_client;
    }
    break;

  case CORBA::tk_boolean:
    {
      CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
      b >>= giop_client;
    }
    break;

  case CORBA::tk_char:
    {
      char *str = PyString_AS_STRING(a_o);

      CORBA::Char c = str[0];
      c >>= giop_client;
    }
    break;

  case CORBA::tk_octet:
    {
      CORBA::Octet o = PyInt_AS_LONG(a_o);
      o >>= giop_client;
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId
    {
      t_o = PyTuple_GET_ITEM(d_o, 1);

      CORBA::Object_ptr obj = (CORBA::Object_ptr)getTwin(a_o);

      CORBA::MarshalObjRef(obj,
			   PyString_AS_STRING(t_o),
			   PyString_GET_SIZE(t_o) + 1,
			   giop_client);
    }
    break;

  case CORBA::tk_struct: // name, descriptor, name, descriptor, ...
    {
      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 1) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=1; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	value   = PyDict_GetItem(sdict, name);
	r_marshalArguments(giop_client, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
					  "_v");
      if (!ev) throw CORBA::BAD_PARAM();
      CORBA::ULong e = PyInt_AS_LONG(ev);
      e >>= giop_client;
    }
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      CORBA::ULong list_len = PyList_GET_SIZE(a_o);
      list_len >>= giop_client;

      t_o = PyTuple_GET_ITEM(d_o, 2);

      if (list_len > 0) {
	for (CORBA::ULong i=0; i < list_len; i++) {
	  r_marshalArguments(giop_client, t_o, PyList_GET_ITEM(a_o, i));
	}
      }
    }
    break;

  case CORBA::tk_array: // length, element_desc
    {
      CORBA::ULong list_len = PyList_GET_SIZE(a_o);

      t_o = PyTuple_GET_ITEM(d_o, 2);

      for (CORBA::ULong i=0; i < list_len; i++) {
	r_marshalArguments(giop_client, t_o, PyList_GET_ITEM(a_o, i));
      }
    }
    break;

  case CORBA::tk_string: // max_length
    {
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
    abort();
  }
  //  cout << "  marshalArguments done." << endl;
}
  


void
Py_OmniProxyCallDesc::unmarshalReturnedValues(GIOP_C& giop_client)
{
  PyEval_RestoreThread(tstate_);
  tstate_ = 0;

  if (out_l_ == 0) {
    Py_INCREF(Py_None);
    result_ = Py_None;
  }
  else if (out_l_ == 1)
    result_ = r_unmarshalReturnedValues(giop_client,
					PyTuple_GET_ITEM(out_d_, 0));
  else {
    result_ = PyTuple_New(out_l_);
    if (!result_) throw CORBA::NO_MEMORY();

    for (int i=0; i < out_l_; i++) {
      PyTuple_SET_ITEM(result_, i,
		       r_unmarshalReturnedValues(giop_client,
						 PyTuple_GET_ITEM(out_d_, i)));
    }
  }
}


PyObject*
Py_OmniProxyCallDesc::r_unmarshalReturnedValues(GIOP_C&   giop_client,
						PyObject* d_o)
{
  CORBA::ULong   tk;
  PyObject*      r_o = 0;	// Current result object
  PyObject*      t_o;		// Temp object
  CORBA::Boolean tup;

  if (PyTuple_Check(d_o)) {
    t_o = PyTuple_GET_ITEM(d_o, 0);
    assert(PyInt_Check(t_o));
    tk  = PyInt_AS_LONG(t_o);
    tup = 1;
  }
  else {
    assert(PyInt_Check(d_o));
    tk  = PyInt_AS_LONG(d_o);
    tup = 0;
  }

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
      t_o                   = PyTuple_GET_ITEM(d_o, 1);
      assert(PyString_Check(t_o));

      char*  targetRepoId   = PyString_AS_STRING(t_o);

      CORBA::Object_ptr obj = CORBA::UnMarshalObjRef(targetRepoId,
						     giop_client);
      r_o = createPyCorbaObject(targetRepoId, obj);
    }
    break;

  case CORBA::tk_struct: // struct_class, name, descriptor, name, descriptor...
    {
      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);

      assert(PyClass_Check(strclass));

      int       cnt      = (PyTuple_GET_SIZE(d_o) - 2) / 2;
      PyObject* strtuple = PyTuple_New(cnt);

      int i, j;
      for (i=0, j=3; i < cnt; i++, j+=2) {
	PyTuple_SET_ITEM(strtuple, i,
			 r_unmarshalReturnedValues(giop_client,
						   PyTuple_GET_ITEM(d_o, j)));
      }
      r_o = PyEval_CallObject(strclass, strtuple);
      Py_DECREF(strtuple);
    }
    break;

  case CORBA::tk_enum: // list enum_items
    {
      t_o = PyTuple_GET_ITEM(d_o, 1);

      assert(PyList_Check(t_o));

      CORBA::ULong e;
      e <<= giop_client;
	
      PyObject* ev = PyList_GET_ITEM(t_o, e);
      Py_INCREF(ev);
      r_o = ev;
    }
    break;

  case CORBA::tk_sequence: // max_length, element_desc
    {
      t_o                  = PyTuple_GET_ITEM(d_o, 1);

      assert(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::ULong seq_len;
      seq_len <<= giop_client;

      if (max_len > 0 && seq_len > max_len) throw CORBA::BAD_PARAM();

      r_o = PyList_New(seq_len);
      t_o = PyTuple_GET_ITEM(d_o, 2);

      if (seq_len > 0) {
	for (CORBA::ULong i=0; i < seq_len; i++) {
	  PyList_SET_ITEM(r_o, i,
			  r_unmarshalReturnedValues(giop_client, t_o));
	}
      }
    }
    break;

  case CORBA::tk_array: // length, element_desc
    {
      t_o                  = PyTuple_GET_ITEM(d_o, 1);

      assert(PyInt_Check(t_o));

      CORBA::ULong arr_len = PyInt_AS_LONG(t_o);
      r_o                  = PyList_New(arr_len);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      for (CORBA::ULong i=0; i < arr_len; i++) {
	PyList_SET_ITEM(r_o, i,
			r_unmarshalReturnedValues(giop_client, t_o));
      }
    }
    break;

  case CORBA::tk_string: // max_length
    {
      t_o                 = PyTuple_GET_ITEM(d_o, 1);

      assert(PyInt_Check(t_o));

      CORBA::Long max_len = PyInt_AS_LONG(t_o);

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
    abort();
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
    int       exc_l = PyTuple_GET_SIZE(exc_d);

    PyObject* exc_a;

    exc_a = PyTuple_New(exc_l);
    if (!exc_a) throw CORBA::NO_MEMORY();

    for (int i=0; i < exc_l; i++) {
      PyTuple_SET_ITEM(exc_a, i,
		       r_unmarshalReturnedValues(giop_client,
						 PyTuple_GET_ITEM(exc_d, i)));
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
