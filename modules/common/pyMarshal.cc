// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyMarshal.cc               Created on: 1999/07/05
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
//    Marshalling / unmarshalling of Python objects


// $Id$

// $Log$
// Revision 1.9  1999/09/29 11:38:59  dpg1
// Oops -- fix to Nil object references was broken.
//
// Revision 1.8  1999/09/29 11:25:55  dpg1
// Nil objects now map to None. They work too, which is more than can be
// said for the old mapping...
//
// Revision 1.7  1999/09/24 09:22:03  dpg1
// Added copyright notices.
//
// Revision 1.6  1999/09/22 15:46:12  dpg1
// Fake POA implemented.
//
// Revision 1.5  1999/08/24 11:21:10  dpg1
// Fixed typo in ulong marshalling.
//
// Revision 1.4  1999/08/03 09:03:34  dpg1
// Unions with no default member fixed.
//
// Revision 1.3  1999/07/29 14:19:57  dpg1
// Various fixes.
//
// Revision 1.2  1999/07/19 15:46:01  dpg1
// Any support, various fixes.
//
// Revision 1.1  1999/07/05 11:17:36  dpg1
// Initial revision
//

#include <iostream.h>
#include <omnipy.h>


CORBA::ULong
omniPy::alignedSize(CORBA::ULong msgsize,
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

  case CORBA::tk_any:
    {
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // Size of TypeCode
      t_o             = PyDict_GetItemString(adict, "_t");
      assert(t_o && PyInstance_Check(t_o));
      PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, "_d");
      assert(desc);
      msgsize         = alignedSizeTypeCode(msgsize, desc);

      // Size of Any's contents
      t_o             = PyDict_GetItemString(adict, "_v");
      assert(t_o);
      msgsize         = alignedSize(msgsize, desc, t_o);
    }
    break;

  case CORBA::tk_TypeCode:
    {
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      t_o             = PyDict_GetItemString(tdict, "_d");
      assert(t_o);
      msgsize         = alignedSizeTypeCode(msgsize, t_o);
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      assert(tup);

      CORBA::Object_ptr obj;
      const char*       repoId;

      if (a_o == Py_None) {
	// Nil object reference
	obj    = CORBA::Object::_nil();
	repoId = "";
      }
      else {
	obj = (CORBA::Object_ptr)getTwin(a_o, OBJREF_TWIN);
	if (!obj) throw CORBA::BAD_PARAM();
	repoId = obj->PR_getobj()->NP_IRRepositoryId();
      }
      msgsize = CORBA::AlignedObjRef(obj, repoId, strlen(repoId) + 1, msgsize);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, name, descriptor, ...
    {
      assert(tup);
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

      // The descriptor tuple has twice the number of struct members,
      // plus 4 -- the typecode kind, the Python class, the repoId,
      // and the struct name
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	assert(PyString_Check(name));
	value   = PyDict_GetItem(sdict, name);
	msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case CORBA::tk_union: // class,
			// repoId,
			// name,
			// discriminant descr,
			// default used,
			// ((label value, member name, member descr), ...),
			// default (label, name, descr) or None,
			// {label: (label, name, descr), ...}
    {
      assert(tup);
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      PyObject* udict = ((PyInstanceObject*)a_o)->in_dict;

      PyObject* discriminant = PyDict_GetItemString(udict, "_d");
      PyObject* value        = PyDict_GetItemString(udict, "_v");
      assert(discriminant && value);

      t_o = PyTuple_GET_ITEM(d_o, 4); // Discriminant descriptor
      msgsize = alignedSize(msgsize, t_o, discriminant);

      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);
      assert(PyDict_Check(cdict));

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	assert(PyTuple_Check(t_o));
	msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(t_o, 2), value);
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  assert(PyTuple_Check(t_o));
	  msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(t_o, 2), value);
	}
      }
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      assert(tup);
      t_o                 = PyTuple_GET_ITEM(d_o, 1);
      assert(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      if (!PyString_Check(a_o)) throw CORBA::BAD_PARAM();

      if (max_len > 0 && (CORBA::ULong)PyString_GET_SIZE(a_o) > max_len)
	throw CORBA::BAD_PARAM();

      msgsize  = omni::align_to(msgsize, omni::ALIGN_4);
      msgsize += 4 + PyString_GET_SIZE(a_o) + 1;
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      assert(tup);

      // Space for list length
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;

      t_o                   = PyTuple_GET_ITEM(d_o, 2);
      assert(PyInt_Check(t_o));
      CORBA::ULong max_len  = PyInt_AS_LONG(t_o);
      PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong len;

      if (PyInt_Check(elm_desc)) { // Simple type
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	switch (etk) {
	case CORBA::tk_octet: // Mapping says octet and char use a string
	case CORBA::tk_char:
	  if (!PyString_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyString_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) throw CORBA::BAD_PARAM();
	  msgsize += len;
	  break;

	case CORBA::tk_boolean:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) throw CORBA::BAD_PARAM();
	  msgsize += len;
	  break;

	case CORBA::tk_short:
	case CORBA::tk_ushort:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) throw CORBA::BAD_PARAM();
	  if (len > 0) {
	    msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	    msgsize += 2 * len;
	  }
	  break;
	  
	case CORBA::tk_long:
	case CORBA::tk_ulong:
	case CORBA::tk_float:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) throw CORBA::BAD_PARAM();
	  if (len > 0) {
	    msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	    msgsize += 4 * len;
	  }
	  break;

	case CORBA::tk_double:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) throw CORBA::BAD_PARAM();
	  if (len > 0) {
	    msgsize = omni::align_to(msgsize,omni::ALIGN_8);
	    msgsize += 8 * len;
	  }
	  break;

	default:
	  abort();
	}
      }
      else {
	if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  
	len = PyList_GET_SIZE(a_o);
	if (max_len > 0 && len > max_len) throw CORBA::BAD_PARAM();

	if (len > 0) {
	  for (CORBA::ULong i=0; i < len; i++) {
	    msgsize = alignedSize(msgsize, elm_desc,
				  PyList_GET_ITEM(a_o, i));
	  }
	}
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      assert(tup);

      t_o                   = PyTuple_GET_ITEM(d_o, 2);
      assert(PyInt_Check(t_o));
      CORBA::ULong arr_len  = PyInt_AS_LONG(t_o);
      PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong len;

      if (PyInt_Check(elm_desc)) { // Simple type
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	switch (etk) {
	case CORBA::tk_octet: // Mapping says octet and char use a string
	case CORBA::tk_char:
	  if (!PyString_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyString_GET_SIZE(a_o);
	  if (len != arr_len) throw CORBA::BAD_PARAM();
	  msgsize += len;
	  break;

	case CORBA::tk_boolean:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) throw CORBA::BAD_PARAM();
	  msgsize += len;
	  break;

	case CORBA::tk_short:
	case CORBA::tk_ushort:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) throw CORBA::BAD_PARAM();
	  msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	  msgsize += 2 * len;
	  break;
	  
	case CORBA::tk_long:
	case CORBA::tk_ulong:
	case CORBA::tk_float:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) throw CORBA::BAD_PARAM();
	  msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	  msgsize += 4 * len;
	  break;

	case CORBA::tk_double:
	  if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) throw CORBA::BAD_PARAM();
	  msgsize = omni::align_to(msgsize,omni::ALIGN_8);
	  msgsize += 8 * len;
	  break;

	default:
	  abort();
	}
      }
      else {
	if (!PyList_Check(a_o)) throw CORBA::BAD_PARAM();
	  
	len = PyList_GET_SIZE(a_o);
	if (len != arr_len) throw CORBA::BAD_PARAM();

	for (CORBA::ULong i=0; i < len; i++) {
	  msgsize = alignedSize(msgsize, elm_desc,
				PyList_GET_ITEM(a_o, i));
	}
      }
    }
    break;

  case CORBA::tk_alias: // repoId, name, descr
    {
      assert(tup);
      msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, 3), a_o);
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      assert(tup);
      if (!PyInstance_Check(a_o)) throw CORBA::BAD_PARAM();

      // repoId string:
      msgsize  = omni::align_to(msgsize, omni::ALIGN_4);
      msgsize += 4 + PyString_GET_SIZE(PyTuple_GET_ITEM(d_o, 2)) + 1;

      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

      // As with structs, the descriptor tuple has twice the number of
      // struct members plus 4.
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	assert(PyString_Check(name));
	value   = PyDict_GetItem(sdict, name);
	msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      assert(PyList_Check(t_o));

      msgsize = alignedSize(msgsize, PyList_GET_ITEM(t_o, 0), a_o);
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
omniPy::marshalPyObject(NetBufferedStream& stream,
			PyObject*          d_o,
			PyObject*          a_o)
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
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Short s = PyInt_AS_LONG(a_o);
      s >>= stream;
    }
    break;

  case CORBA::tk_long:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Long l = PyInt_AS_LONG(a_o);
      l >>= stream;
    }
    break;

  case CORBA::tk_ushort:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::UShort us = PyInt_AS_LONG(a_o);
      us >>= stream;
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul = 0; // Initialised to stop egcs complaining

      if (PyLong_Check(a_o))
	ul = PyLong_AsUnsignedLong(a_o);
      else if (PyInt_Check(a_o))
	ul = PyInt_AS_LONG(a_o);
      else throw CORBA::BAD_PARAM();

      ul >>= stream;
    }
    break;

  case CORBA::tk_float:
    {
      CORBA::Float f = 0; // Initialised to stop egcs complaining

      if (PyFloat_Check(a_o))
	f = (CORBA::Float)PyFloat_AS_DOUBLE(a_o);
      else if (PyInt_Check(a_o)) {
	CORBA::Long i = PyInt_AS_LONG(a_o);
	f = i;
      }
      else throw CORBA::BAD_PARAM();

      f >>= stream;
    }
    break;

  case CORBA::tk_double:
    {
      CORBA::Double d = 0; // Initialised to stop egcs complaining

      if (PyFloat_Check(a_o))
	d = (CORBA::Double)PyFloat_AS_DOUBLE(a_o);
      else if (PyInt_Check(a_o)) {
	CORBA::Long i = PyInt_AS_LONG(a_o);
	d = i;
      }
      else throw CORBA::BAD_PARAM();

      d >>= stream;
    }
    break;

  case CORBA::tk_boolean:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
      b >>= stream;
    }
    break;

  case CORBA::tk_char:
    {
      if (!PyString_Check(a_o))        throw CORBA::BAD_PARAM();
      if (PyString_GET_SIZE(a_o) != 1) throw CORBA::BAD_PARAM();

      char *str = PyString_AS_STRING(a_o);

      CORBA::Char c = str[0];
      c >>= stream;
    }
    break;

  case CORBA::tk_octet:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Octet o = PyInt_AS_LONG(a_o);
      o >>= stream;
    }
    break;

  case CORBA::tk_any:
    {
      //      cout << "about to marshal Any's TypeCode..." << endl;
      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // TypeCode
      t_o             = PyDict_GetItemString(adict, "_t");
      PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, "_d");
      marshalTypeCode(stream, desc);

      //      cout << "about to marshal Any's contents..." << endl;

      // Any's contents
      t_o             = PyDict_GetItemString(adict, "_v");
      marshalPyObject(stream, desc, t_o);
      //      cout << "Any marshalled." << endl;
    }
    break;

  case CORBA::tk_TypeCode:
    {
      //      cout << "about to marshal TypeCode..." << endl;
      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      t_o             = PyDict_GetItemString(tdict, "_d"); assert(t_o);
      marshalTypeCode(stream, t_o);
      //      cout << "TypeCode marshalled." << endl;
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      CORBA::Object_ptr obj;
      const char*       repoId;

      if (a_o == Py_None) {
	// Nil object reference
	obj    = CORBA::Object::_nil();
	repoId = "";
      }
      else {
	obj    = (CORBA::Object_ptr)getTwin(a_o, OBJREF_TWIN);
	repoId = obj->PR_getobj()->NP_IRRepositoryId();
      }
      CORBA::MarshalObjRef(obj, repoId, strlen(repoId) + 1, stream);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, {name, descriptor}
    {
      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	value   = PyDict_GetItem(sdict, name);
	marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case CORBA::tk_union: // class,
			// repoId,
			// name,
			// discriminant descr,
			// default used,
			// ((label value, member name, member descr), ...),
			// default (label, name, descr) or None,
			// {label: (label, name, descr), ...}
    {
      PyObject* udict = ((PyInstanceObject*)a_o)->in_dict;

      PyObject* discriminant = PyDict_GetItemString(udict, "_d");
      PyObject* value        = PyDict_GetItemString(udict, "_v");
      t_o = PyTuple_GET_ITEM(d_o, 4); // Discriminant descriptor

      marshalPyObject(stream, t_o, discriminant);

      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	marshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2), value);
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  marshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2), value);
	}
      }
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
					  "_v");
      if (!ev) throw CORBA::BAD_PARAM();
      CORBA::ULong e = PyInt_AS_LONG(ev);
      e >>= stream;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      CORBA::ULong slen = PyString_GET_SIZE(a_o) + 1;

      slen >>= stream;

      if (slen > 1) {
	char* str = PyString_AS_STRING(a_o);
	stream.put_char_array((const CORBA::Char*)((const char*)str),
			      slen);
      }
      else {
	CORBA::Char('\0') >>= stream;
      }
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::Boolean is_string = 0;
      CORBA::ULong   i;
      CORBA::ULong   etk;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }

      if (is_string) {
	CORBA::ULong len = PyString_GET_SIZE(a_o);
	len >>= stream;

	if (len > 0) {
	  if (etk == CORBA::tk_octet) {
	    CORBA::Octet o;
	    CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	    for (i=0; i < len; i++) {
	      o = l[i];
	      o >>= stream;
	    }
	  }
	  else { // (etk == CORBA::tk_char)
	    CORBA::Char c;
	    CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	    for (i=0; i < len; i++) {
	      c = l[i];
	      c >>= stream;
	    }
	  }
	}
      }
      else {
	CORBA::ULong len = PyList_GET_SIZE(a_o);
	len >>= stream;

	if (len > 0) {
	  for (i=0; i < len; i++) {
	    marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
	  }
	}
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::Boolean is_string = 0;
      CORBA::ULong   i;
      CORBA::ULong   etk;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }

      if (is_string) {
	CORBA::ULong len = PyString_GET_SIZE(a_o);

	if (etk == CORBA::tk_octet) {
	  CORBA::Octet o;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    o = l[i];
	    o >>= stream;
	  }
	}
	else { // (etk == CORBA::tk_char)
	  CORBA::Char c;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    c = l[i];
	    c >>= stream;
	  }
	}
      }
      else {
	CORBA::ULong len = PyList_GET_SIZE(a_o);

	for (i=0; i < len; i++) {
	  marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
	}
      }
    }
    break;

  case CORBA::tk_alias: // repoId, name, descr
    {
      marshalPyObject(stream, PyTuple_GET_ITEM(d_o, 3), a_o);
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      PyObject*    t_o  = PyTuple_GET_ITEM(d_o, 2);
      CORBA::ULong slen = PyString_GET_SIZE(t_o) + 1;
      slen >>= stream;

      if (slen > 1) {
	char* str = PyString_AS_STRING(t_o);
	stream.put_char_array((const CORBA::Char*)((const char*)str), slen);
      }
      else {
	CORBA::Char('\0') >>= stream;
      }

      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	value   = PyDict_GetItem(sdict, name);
	marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      t_o = PyTuple_GET_ITEM(d_o, 1);
      marshalPyObject(stream, PyList_GET_ITEM(t_o, 0), a_o);
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
omniPy::marshalPyObject(MemBufferedStream& stream,
			PyObject*          d_o,
			PyObject*          a_o)
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
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Short s = PyInt_AS_LONG(a_o);
      s >>= stream;
    }
    break;

  case CORBA::tk_long:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Long l = PyInt_AS_LONG(a_o);
      l >>= stream;
    }
    break;

  case CORBA::tk_ushort:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::UShort us = PyInt_AS_LONG(a_o);
      us >>= stream;
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul = 0; // Initialised to stop egcs complaining

      if (PyLong_Check(a_o))
	ul = PyLong_AsUnsignedLong(a_o);
      else if (PyInt_Check(a_o))
	ul = PyInt_AS_LONG(a_o);
      else throw CORBA::BAD_PARAM();

      ul >>= stream;
    }
    break;

  case CORBA::tk_float:
    {
      CORBA::Float f = 0; // Initialised to stop egcs complaining

      if (PyFloat_Check(a_o))
	f = (CORBA::Float)PyFloat_AS_DOUBLE(a_o);
      else if (PyInt_Check(a_o)) {
	CORBA::Long i = PyInt_AS_LONG(a_o);
	f = i;
      }
      else throw CORBA::BAD_PARAM();

      f >>= stream;
    }
    break;

  case CORBA::tk_double:
    {
      CORBA::Double d = 0; // Initialised to stop egcs complaining

      if (PyFloat_Check(a_o))
	d = (CORBA::Double)PyFloat_AS_DOUBLE(a_o);
      else if (PyInt_Check(a_o)) {
	CORBA::Long i = PyInt_AS_LONG(a_o);
	d = i;
      }
      else throw CORBA::BAD_PARAM();

      d >>= stream;
    }
    break;

  case CORBA::tk_boolean:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
      b >>= stream;
    }
    break;

  case CORBA::tk_char:
    {
      if (!PyString_Check(a_o))        throw CORBA::BAD_PARAM();
      if (PyString_GET_SIZE(a_o) != 1) throw CORBA::BAD_PARAM();

      char *str = PyString_AS_STRING(a_o);

      CORBA::Char c = str[0];
      c >>= stream;
    }
    break;

  case CORBA::tk_octet:
    {
      if (!PyInt_Check(a_o)) throw CORBA::BAD_PARAM();
      CORBA::Octet o = PyInt_AS_LONG(a_o);
      o >>= stream;
    }
    break;

  case CORBA::tk_any:
    {
      //      cout << "about to marshal Any's TypeCode..." << endl;
      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // TypeCode
      t_o             = PyDict_GetItemString(adict, "_t");
      PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, "_d");
      marshalTypeCode(stream, desc);

      //      cout << "about to marshal Any's contents..." << endl;

      // Any's contents
      t_o             = PyDict_GetItemString(adict, "_v");
      marshalPyObject(stream, desc, t_o);
      //      cout << "Any marshalled." << endl;
    }
    break;

  case CORBA::tk_TypeCode:
    {
      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      t_o             = PyDict_GetItemString(tdict, "_d"); assert(t_o);
      marshalTypeCode(stream, t_o);
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      CORBA::Object_ptr obj;
      const char*       repoId;

      if (a_o == Py_None) {
	// Nil object reference
	obj    = CORBA::Object::_nil();
	repoId = "";
      }
      else {
	obj    = (CORBA::Object_ptr)getTwin(a_o, OBJREF_TWIN);
	repoId = obj->PR_getobj()->NP_IRRepositoryId();
      }
      CORBA::MarshalObjRef(obj, repoId, strlen(repoId) + 1, stream);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, {name, descriptor}
    {
      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	value   = PyDict_GetItem(sdict, name);
	marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case CORBA::tk_union: // class,
			// repoId,
			// name,
			// discriminant descr,
			// default used,
			// ((label value, member name, member descr), ...),
			// default (label, name, descr) or None,
			// {label: (label, name, descr), ...}
    {
      PyObject* udict = ((PyInstanceObject*)a_o)->in_dict;

      PyObject* discriminant = PyDict_GetItemString(udict, "_d");
      PyObject* value        = PyDict_GetItemString(udict, "_v");
      t_o = PyTuple_GET_ITEM(d_o, 4); // Discriminant descriptor

      marshalPyObject(stream, t_o, discriminant);

      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	marshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2), value);
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  marshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2), value);
	}
      }
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
					  "_v");
      if (!ev) throw CORBA::BAD_PARAM();
      CORBA::ULong e = PyInt_AS_LONG(ev);
      e >>= stream;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      CORBA::ULong slen = PyString_GET_SIZE(a_o) + 1;

      slen >>= stream;

      if (slen > 1) {
	char* str = PyString_AS_STRING(a_o);
	stream.put_char_array((const CORBA::Char*)((const char*)str),
			      slen);
      }
      else {
	CORBA::Char('\0') >>= stream;
      }
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::Boolean is_string = 0;
      CORBA::ULong   i;
      CORBA::ULong   etk;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }

      if (is_string) {
	CORBA::ULong len = PyString_GET_SIZE(a_o);
	len >>= stream;

	if (len > 0) {
	  if (etk == CORBA::tk_octet) {
	    CORBA::Octet o;
	    CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	    for (i=0; i < len; i++) {
	      o = l[i];
	      o >>= stream;
	    }
	  }
	  else { // (etk == CORBA::tk_char)
	    CORBA::Char c;
	    CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	    for (i=0; i < len; i++) {
	      c = l[i];
	      c >>= stream;
	    }
	  }
	}
      }
      else {
	CORBA::ULong len = PyList_GET_SIZE(a_o);
	len >>= stream;

	if (len > 0) {
	  for (i=0; i < len; i++) {
	    marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
	  }
	}
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::Boolean is_string = 0;
      CORBA::ULong   i;
      CORBA::ULong   etk;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }

      if (is_string) {
	CORBA::ULong len = PyString_GET_SIZE(a_o);

	if (etk == CORBA::tk_octet) {
	  CORBA::Octet o;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    o = l[i];
	    o >>= stream;
	  }
	}
	else { // (etk == CORBA::tk_char)
	  CORBA::Char c;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    c = l[i];
	    c >>= stream;
	  }
	}
      }
      else {
	CORBA::ULong len = PyList_GET_SIZE(a_o);

	for (i=0; i < len; i++) {
	  marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
	}
      }
    }
    break;

  case CORBA::tk_alias: // repoId, name, descr
    {
      marshalPyObject(stream, PyTuple_GET_ITEM(d_o, 3), a_o);
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      PyObject*    t_o  = PyTuple_GET_ITEM(d_o, 2);
      CORBA::ULong slen = PyString_GET_SIZE(t_o) + 1;
      slen >>= stream;

      if (slen > 1) {
	char* str = PyString_AS_STRING(t_o);
	stream.put_char_array((const CORBA::Char*)((const char*)str), slen);
      }
      else {
	CORBA::Char('\0') >>= stream;
      }

      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	value   = PyDict_GetItem(sdict, name);
	marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
      }
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      t_o = PyTuple_GET_ITEM(d_o, 1);
      marshalPyObject(stream, PyList_GET_ITEM(t_o, 0), a_o);
    }
    break;

  default:
    cout << "!!! marshalArguments(): unsupported typecode: "
	 << (CORBA::ULong)tk << endl;
    abort();
  }
  //  cout << "  marshalArguments done." << endl;
}




PyObject*
omniPy::unmarshalPyObject(NetBufferedStream& stream,
			  PyObject*          d_o)
{
  CORBA::ULong   tk;
  PyObject*      r_o = 0;	// Current result object
  PyObject*      t_o;		// Temp object
  CORBA::Boolean tup;

  //  cout << "unmarshalPyObject() starts." << endl;

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
      s <<= stream;
      r_o = PyInt_FromLong(s);
    }
    break;

  case CORBA::tk_long:
    {
      CORBA::Long l;
      l <<= stream;
      r_o = PyInt_FromLong(l);
    }
    break;

  case CORBA::tk_ushort:
    {
      CORBA::UShort us;
      us <<= stream;
      r_o = PyInt_FromLong(us);
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul;
      ul <<= stream;
      r_o = PyLong_FromUnsignedLong(ul);
    }
    break;

  case CORBA::tk_float:
    {
      CORBA::Float f;
      f <<= stream;
      r_o = PyFloat_FromDouble((double)f);
    }
    break;

  case CORBA::tk_double:
    {
      CORBA::Double d;
      d <<= stream;
      r_o = PyFloat_FromDouble(d);
    }
    break;

  case CORBA::tk_boolean:
    {
      CORBA::Boolean b;
      b <<= stream;
      r_o = PyInt_FromLong(b);
    }
    break;

  case CORBA::tk_char:
    {
      CORBA::Char c;
      c <<= stream;

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
      o <<= stream;
      r_o = PyInt_FromLong(o);
    }
    break;

  case CORBA::tk_any:
    {
      //      cout << "about to unmarshal Any's TypeCode..." << endl;

      // TypeCode
      PyObject* desc     = unmarshalTypeCode(stream);
      PyObject* argtuple = PyTuple_New(1);
      PyTuple_SET_ITEM(argtuple, 0, desc);
      PyObject* tcobj    = PyEval_CallObject(pyCreateTypeCode, argtuple);
      Py_DECREF(argtuple);

      //      cout << "about to unmarshal Any's value..." << endl;

      // Value
      t_o = unmarshalPyObject(stream, desc);

      argtuple = PyTuple_New(2);
      PyTuple_SET_ITEM(argtuple, 0, tcobj);
      PyTuple_SET_ITEM(argtuple, 1, t_o);

      r_o = PyEval_CallObject(pyCORBAAnyClass, argtuple);
      Py_DECREF(argtuple);

      //      cout << "Any unmarshalled." << endl;
    }
    break;

  case CORBA::tk_TypeCode:
    {
      t_o                = unmarshalTypeCode(stream);
      PyObject* argtuple = PyTuple_New(1);
      PyTuple_SET_ITEM(argtuple, 0, t_o);
      r_o                = PyEval_CallObject(pyCreateTypeCode, argtuple);
      Py_DECREF(argtuple);
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);

      const char* targetRepoId;

      if (t_o == Py_None)
	targetRepoId = 0;
      else {
	assert(PyString_Check(t_o));
	targetRepoId = PyString_AS_STRING(t_o);
      }

      CORBA::Object_ptr obj = omniPy::UnMarshalObjRef(targetRepoId,
						      stream);
      r_o = createPyCorbaObjRef(targetRepoId, obj);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, name, descriptor, ...
    {
      assert(tup);
      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      assert(PyClass_Check(strclass));

      int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
      PyObject* strtuple = PyTuple_New(cnt);

      int i, j;
      for (i=0, j=5; i < cnt; i++, j+=2) {
	PyTuple_SET_ITEM(strtuple, i,
			 unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, j)));
      }
      r_o = PyEval_CallObject(strclass, strtuple);
      Py_DECREF(strtuple);
    }
    break;

  case CORBA::tk_union: // class,
			// repoId,
			// name,
			// discriminant descr,
			// default used,
			// ((label value, member name, member descr), ...),
			// default (label, name, descr) or None,
			// {label: (label, name, descr), ...}
    {
      assert(tup);
      PyObject* unclass = PyTuple_GET_ITEM(d_o, 1);
      assert(PyClass_Check(unclass));

      t_o = PyTuple_GET_ITEM(d_o, 4);

      PyObject* discriminant = unmarshalPyObject(stream, t_o);
      PyObject* value;
      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	assert(PyTuple_Check(t_o));
	value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  assert(PyTuple_Check(t_o));
	  value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
	}
      }

      PyObject* untuple = PyTuple_New(2);
      PyTuple_SET_ITEM(untuple, 0, discriminant);
      PyTuple_SET_ITEM(untuple, 1, value);

      r_o = PyEval_CallObject(unclass, untuple);
      Py_DECREF(untuple);
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 3);

      assert(PyTuple_Check(t_o));

      CORBA::ULong e;
      e <<= stream;
	
      PyObject* ev = PyTuple_GET_ITEM(t_o, e);
      Py_INCREF(ev);
      r_o = ev;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      assert(tup);
      t_o                 = PyTuple_GET_ITEM(d_o, 1);

      assert(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::String_member str_tmp;
      str_tmp <<= stream;

      r_o = PyString_FromString(str_tmp._ptr);

      if (max_len > 0 && (CORBA::ULong)PyString_GET_SIZE(r_o) > max_len)
	throw CORBA::MARSHAL();
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      assert(tup);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      assert(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::ULong len;
      len <<= stream;

      if (max_len > 0 && len > max_len) throw CORBA::MARSHAL();

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong   etk;
      CORBA::ULong   i;
      CORBA::Boolean is_string = 0;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }
	  
      if (is_string) {
	r_o = PyString_FromStringAndSize(0, len);

	if (len > 0) {
	  if (etk == CORBA::tk_octet) {
	    CORBA::Octet o;
	    CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	    for (i=0; i < len; i++) {
	      o <<= stream;
	      l[i] = o;
	    }
	  }
	  else {// (etk == CORBA::tk_char)
	    CORBA::Char c;
	    CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(r_o);
	    for (i=0; i < len; i++) {
	      c <<= stream;
	      l[i] = c;
	    }
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	if (len > 0) {
	  for (i=0; i < len; i++) {
	    PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
	  }
	}
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      assert(tup);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      assert(PyInt_Check(t_o));

      CORBA::ULong len = PyInt_AS_LONG(t_o);

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong   etk;
      CORBA::ULong   i;
      CORBA::Boolean is_string = 0;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }
	  
      if (is_string) {
	r_o = PyString_FromStringAndSize(0, len);

	if (etk == CORBA::tk_octet) {
	  CORBA::Octet o;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    o <<= stream;
	    l[i] = o;
	  }
	}
	else {// (etk == CORBA::tk_char)
	  CORBA::Char c;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    c <<= stream;
	    l[i] = c;
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	for (i=0; i < len; i++) {
	  PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
	}
      }
    }
    break;

  case CORBA::tk_alias: // repoId, name, descr
    {
      r_o = unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, 3));
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      assert(tup);

      // Throw away the repoId. By the time we get here, we already
      // know it.
      { 
	CORBA::String_member str_tmp;
	str_tmp <<= stream;
      }

      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      assert(PyClass_Check(strclass));

      int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
      PyObject* strtuple = PyTuple_New(cnt);

      int i, j;
      for (i=0, j=5; i < cnt; i++, j+=2) {
	PyTuple_SET_ITEM(strtuple, i,
			 unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, j)));
      }
      r_o = PyEval_CallObject(strclass, strtuple);
      Py_DECREF(strtuple);
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      assert(PyList_Check(t_o));

      r_o = unmarshalPyObject(stream, PyList_GET_ITEM(t_o, 0));
    }
    break;

  default:
    cout << " !!! unmarshalReturnedValues(): unsupported typecode: "
	 << (CORBA::ULong)tk << endl;
    abort();
  }
  //  cout << "unmarshalPyObject() ends." << endl;

  return r_o;
}



PyObject*
omniPy::unmarshalPyObject(MemBufferedStream& stream,
			  PyObject*          d_o)
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
      s <<= stream;
      r_o = PyInt_FromLong(s);
    }
    break;

  case CORBA::tk_long:
    {
      CORBA::Long l;
      l <<= stream;
      r_o = PyInt_FromLong(l);
    }
    break;

  case CORBA::tk_ushort:
    {
      CORBA::UShort us;
      us <<= stream;
      r_o = PyInt_FromLong(us);
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul;
      ul <<= stream;
      r_o = PyLong_FromUnsignedLong(ul);
    }
    break;

  case CORBA::tk_float:
    {
      CORBA::Float f;
      f <<= stream;
      r_o = PyFloat_FromDouble((double)f);
    }
    break;

  case CORBA::tk_double:
    {
      CORBA::Double d;
      d <<= stream;
      r_o = PyFloat_FromDouble(d);
    }
    break;

  case CORBA::tk_boolean:
    {
      CORBA::Boolean b;
      b <<= stream;
      r_o = PyInt_FromLong(b);
    }
    break;

  case CORBA::tk_char:
    {
      CORBA::Char c;
      c <<= stream;

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
      o <<= stream;
      r_o = PyInt_FromLong(o);
    }
    break;

  case CORBA::tk_any:
    {
      //      cout << "about to unmarshal Any's TypeCode..." << endl;

      // TypeCode
      PyObject* desc     = unmarshalTypeCode(stream);
      PyObject* argtuple = PyTuple_New(1);
      PyTuple_SET_ITEM(argtuple, 0, desc);
      PyObject* tcobj    = PyEval_CallObject(pyCreateTypeCode, argtuple);
      Py_DECREF(argtuple);

      //      cout << "about to unmarshal Any's value..." << endl;

      // Value
      t_o = unmarshalPyObject(stream, desc);

      argtuple = PyTuple_New(2);
      PyTuple_SET_ITEM(argtuple, 0, tcobj);
      PyTuple_SET_ITEM(argtuple, 1, t_o);

      r_o = PyEval_CallObject(pyCORBAAnyClass, argtuple);
      Py_DECREF(argtuple);

      //      cout << "Any unmarshalled." << endl;
    }
    break;

  case CORBA::tk_TypeCode:
    {
      t_o                = unmarshalTypeCode(stream);
      PyObject* argtuple = PyTuple_New(1);
      PyTuple_SET_ITEM(argtuple, 0, t_o);
      r_o                = PyEval_CallObject(pyCreateTypeCode, argtuple);
      Py_DECREF(argtuple);
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);

      const char* targetRepoId;

      if (t_o == Py_None)
	targetRepoId = 0;
      else {
	assert(PyString_Check(t_o));
	targetRepoId = PyString_AS_STRING(t_o);
      }

      CORBA::Object_ptr obj = omniPy::UnMarshalObjRef(targetRepoId,
						      stream);
      r_o = createPyCorbaObjRef(targetRepoId, obj);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, name, descriptor, ...
    {
      assert(tup);
      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      assert(PyClass_Check(strclass));

      int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
      PyObject* strtuple = PyTuple_New(cnt);

      int i, j;
      for (i=0, j=5; i < cnt; i++, j+=2) {
	PyTuple_SET_ITEM(strtuple, i,
			 unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, j)));
      }
      r_o = PyEval_CallObject(strclass, strtuple);
      Py_DECREF(strtuple);
    }
    break;

  case CORBA::tk_union: // class,
			// repoId,
			// name,
			// discriminant descr,
			// default used,
			// ((label value, member name, member descr), ...),
			// default (label, name, descr) or None,
			// {label: (label, name, descr), ...}
    {
      assert(tup);
      PyObject* unclass = PyTuple_GET_ITEM(d_o, 1);
      assert(PyClass_Check(unclass));

      t_o = PyTuple_GET_ITEM(d_o, 4);

      PyObject* discriminant = unmarshalPyObject(stream, t_o);
      PyObject* value;
      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	assert(PyTuple_Check(t_o));
	value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  assert(PyTuple_Check(t_o));
	  value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
	}
      }

      PyObject* untuple = PyTuple_New(2);
      PyTuple_SET_ITEM(untuple, 0, discriminant);
      PyTuple_SET_ITEM(untuple, 1, value);

      r_o = PyEval_CallObject(unclass, untuple);
      Py_DECREF(untuple);
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 3);

      assert(PyTuple_Check(t_o));

      CORBA::ULong e;
      e <<= stream;
	
      PyObject* ev = PyTuple_GET_ITEM(t_o, e);
      Py_INCREF(ev);
      r_o = ev;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      assert(tup);
      t_o                 = PyTuple_GET_ITEM(d_o, 1);

      assert(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::String_member str_tmp;
      str_tmp <<= stream;

      r_o = PyString_FromString(str_tmp._ptr);

      if (max_len > 0 && (CORBA::ULong)PyString_GET_SIZE(r_o) > max_len)
	throw CORBA::MARSHAL();
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      assert(tup);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      assert(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::ULong len;
      len <<= stream;

      if (max_len > 0 && len > max_len) throw CORBA::MARSHAL();

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong   etk;
      CORBA::ULong   i;
      CORBA::Boolean is_string = 0;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }
	  
      if (is_string) {
	r_o = PyString_FromStringAndSize(0, len);

	if (len > 0) {
	  if (etk == CORBA::tk_octet) {
	    CORBA::Octet o;
	    CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	    for (i=0; i < len; i++) {
	      o <<= stream;
	      l[i] = o;
	    }
	  }
	  else {// (etk == CORBA::tk_char)
	    CORBA::Char c;
	    CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(r_o);
	    for (i=0; i < len; i++) {
	      c <<= stream;
	      l[i] = c;
	    }
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	if (len > 0) {
	  for (i=0; i < len; i++) {
	    PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
	  }
	}
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      assert(tup);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      assert(PyInt_Check(t_o));

      CORBA::ULong len = PyInt_AS_LONG(t_o);

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong   etk;
      CORBA::ULong   i;
      CORBA::Boolean is_string = 0;

      if (PyInt_Check(elm_desc)) {
	etk = PyInt_AS_LONG(elm_desc);
	if (etk == CORBA::tk_octet || etk == CORBA::tk_char)
	  is_string = 1;
      }
	  
      if (is_string) {
	r_o = PyString_FromStringAndSize(0, len);

	if (etk == CORBA::tk_octet) {
	  CORBA::Octet o;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    o <<= stream;
	    l[i] = o;
	  }
	}
	else {// (etk == CORBA::tk_char)
	  CORBA::Char c;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    c <<= stream;
	    l[i] = c;
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	for (i=0; i < len; i++) {
	  PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
	}
      }
    }
    break;

  case CORBA::tk_alias: // repoId, name, descr
    {
      r_o = unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, 3));
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      assert(tup);

      // Throw away the repoId. By the time we get here, we already
      // know it.
      { 
	CORBA::String_member str_tmp;
	str_tmp <<= stream;
      }

      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      assert(PyClass_Check(strclass));

      int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
      PyObject* strtuple = PyTuple_New(cnt);

      int i, j;
      for (i=0, j=5; i < cnt; i++, j+=2) {
	PyTuple_SET_ITEM(strtuple, i,
			 unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, j)));
      }
      r_o = PyEval_CallObject(strclass, strtuple);
      Py_DECREF(strtuple);
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      assert(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      assert(PyList_Check(t_o));

      r_o = unmarshalPyObject(stream, PyList_GET_ITEM(t_o, 0));
    }
    break;

  default:
    cout << " !!! unmarshalReturnedValues(): unsupported typecode: "
	 << (CORBA::ULong)tk << endl;
    abort();
  }
  return r_o;
}

