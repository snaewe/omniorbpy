// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyTypeCode.cc              Created on: 1999/07/19
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
//    TypeCode support


// $Id$

// $Log$
// Revision 1.1.2.1  2000/10/13 13:55:27  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>


// Objects to map descriptors to typecode offsets and vice-versa:

class DescriptorOffsetMap {
public:
  DescriptorOffsetMap() :
    dict_(PyDict_New()), base_(0)
  {
  }

  DescriptorOffsetMap(DescriptorOffsetMap& dom, CORBA::Long offset) :
    dict_(dom.getDict()), base_(dom.getBase() + offset)
  {
  }

  ~DescriptorOffsetMap() {
    Py_DECREF(dict_);
  }

  inline void add(PyObject* desc, CORBA::Long offset) {
    PyObject *desc_o = omniPy::newTwin(desc);

    PyObject* oo = PyInt_FromLong(offset + base_);
    PyDict_SetItem(dict_, desc_o, oo);
    Py_DECREF(desc_o);
    Py_DECREF(oo);
  }

  inline CORBA::Boolean lookup(PyObject* desc, CORBA::Long& offset) {
    PyObject *desc_o = omniPy::newTwin(desc);

    PyObject* oo = PyDict_GetItem(dict_, desc_o);
    Py_DECREF(desc_o);
    if (oo) {
      offset = PyInt_AS_LONG(oo) - base_;
      return 1;
    }
    return 0;
  }

protected:
  inline PyObject*   getDict() { Py_INCREF(dict_); return dict_; }
  inline CORBA::Long getBase() { return base_; }

private:
  PyObject*   dict_;
  CORBA::Long base_;

  DescriptorOffsetMap(const DescriptorOffsetMap&);
  DescriptorOffsetMap& operator=(const DescriptorOffsetMap&);
};

class OffsetDescriptorMap {
public:
  OffsetDescriptorMap() :
    dict_(PyDict_New()), base_(0)
  {
  }

  OffsetDescriptorMap(OffsetDescriptorMap& odm, CORBA::Long offset) :
    dict_(odm.getDict()), base_(odm.getBase() + offset)
  {
  }

  ~OffsetDescriptorMap() {
    Py_DECREF(dict_);
  }

  inline void add(PyObject* desc, CORBA::Long offset) {
    PyObject* oo = PyInt_FromLong(offset + base_);
    PyDict_SetItem(dict_, oo, desc);
    Py_DECREF(oo);
  }

  inline CORBA::Boolean lookup(PyObject*& desc, CORBA::Long offset) {
    PyObject* oo = PyInt_FromLong(offset + base_);
    desc = PyDict_GetItem(dict_, oo);
    Py_DECREF(oo);
    if (desc) {
      Py_INCREF(desc);
      return 1;
    }
    return 0;
  }

protected:
  inline PyObject*   getDict() { Py_INCREF(dict_); return dict_; }
  inline CORBA::Long getBase() { return base_; }

private:
  PyObject*   dict_;
  CORBA::Long base_;

  OffsetDescriptorMap(const OffsetDescriptorMap&);
  OffsetDescriptorMap& operator=(const OffsetDescriptorMap&);
};



// Macro to marshal a PyString into a cdrStream
#define MARSHAL_PYSTRING(_stream, _pystring) { \
  CORBA::ULong _slen = PyString_GET_SIZE(_pystring) + 1; \
  _slen >>= _stream; \
  if (_slen > 1) { \
    char* _str = PyString_AS_STRING(_pystring); \
    _stream.put_char_array((const CORBA::Char*)((const char*)_str), _slen); \
  } \
}

static void
r_marshalTypeCode(cdrStream&           stream,
		  PyObject*            d_o,
		  DescriptorOffsetMap& dom)
{
  CORBA::Long tc_offset;

  //  cout << endl << "marshalTypeCode... " << flush;

  // If this TypeCode has already been sent, use an indirection:
  if (omniORB::useTypeCodeIndirections && dom.lookup(d_o, tc_offset)) {

    CORBA::ULong tk_ind = 0xffffffff;
    CORBA::Long  offset = tc_offset - stream.currentOutputPtr();

    tk_ind >>= stream;
    offset >>= stream;

    //    cout << "indirection to " << offset << endl;
  }
  else {
    CORBA::ULong   tk;
    PyObject*      t_o;

    if (PyTuple_Check(d_o)) {
      t_o = PyTuple_GET_ITEM(d_o, 0); OMNIORB_ASSERT(PyInt_Check(t_o));
      tk  = PyInt_AS_LONG(t_o);
    }
    else {
      OMNIORB_ASSERT(PyInt_Check(d_o));
      tk  = PyInt_AS_LONG(d_o);
    }

    // Marshal the kind
    tk >>= stream;

    // Offset of this TypeCode (within the current encapsulation if any):
    tc_offset = stream.currentOutputPtr() - 4;

    //    cout << "offset = " << tc_offset << " " << flush;
    
    switch (tk) {
    case CORBA::tk_null:
    case CORBA::tk_void:
    case CORBA::tk_short:
    case CORBA::tk_long:
    case CORBA::tk_ushort:
    case CORBA::tk_ulong:
    case CORBA::tk_float:
    case CORBA::tk_double:
    case CORBA::tk_boolean:
    case CORBA::tk_char:
    case CORBA::tk_octet:
    case CORBA::tk_any:
    case CORBA::tk_TypeCode:
    case CORBA::tk_Principal:
#ifdef HAS_LongLong
    case CORBA::tk_longlong:
    case CORBA::tk_ulonglong:
#endif
      // Nothing more to be sent
      //      cout << "simple: " << tk << endl;
      break;

    case CORBA::tk_string:
      {
	//	cout << "string" << endl;
	// Send max length
	t_o = PyTuple_GET_ITEM(d_o, 1); OMNIORB_ASSERT(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= stream;
      }
      break;

    case CORBA::tk_objref:
      {
	//	cout << "objref" << endl;
	// Add entry to descriptor offset map:
	dom.add(d_o, tc_offset);

	// Stream for the encapsulation
	cdrEncapsulationStream encap;

	// RepoId and name
	t_o = PyTuple_GET_ITEM(d_o, 1); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_struct:
      {
	//	cout << "struct" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Count
	CORBA::ULong cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;
	cnt >>= encap;

	CORBA::ULong i, j, slen;
	char* str;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o  = PyTuple_GET_ITEM(d_o, j++);
	  OMNIORB_ASSERT(PyString_Check(t_o));
	  str  = PyString_AS_STRING(t_o);
	  slen = PyString_GET_SIZE(t_o) + 1;

	  if (str[0] == '_') { --slen; ++str; }
	  slen >>= encap;
	  if (slen > 1) {
	    encap.put_char_array((const CORBA::Char*)((const char*)str),
				 slen);
	  }
	  // member type
	  r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, j++), edom);
	}
	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_union:
      {
	//	cout << "union" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Discriminant type
	PyObject* discriminant = PyTuple_GET_ITEM(d_o, 4);
	r_marshalTypeCode(encap, discriminant, edom);

	// Default used
	t_o              = PyTuple_GET_ITEM(d_o, 5);
	OMNIORB_ASSERT(PyInt_Check(t_o));
	CORBA::Long defu = PyInt_AS_LONG(t_o);
	defu >>= encap;

	PyObject* mems;
	PyObject* mem;

	// Count
	mems = PyTuple_GET_ITEM(d_o, 6); OMNIORB_ASSERT(PyTuple_Check(mems));
	CORBA::ULong cnt = PyTuple_GET_SIZE(mems);
	cnt >>= encap;

	CORBA::ULong slen;
	char*        str;

	for (CORBA::ULong i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); OMNIORB_ASSERT(PyTuple_Check(mem));

	  // Label value
	  omniPy::marshalPyObject(encap, discriminant,
				  PyTuple_GET_ITEM(mem, 0));

	  // Member name
	  t_o  = PyTuple_GET_ITEM(mem, 1); OMNIORB_ASSERT(PyString_Check(t_o));
	  str  = PyString_AS_STRING(t_o);
	  slen = PyString_GET_SIZE(t_o) + 1;

	  if (str[0] == '_') { --slen; ++str; }
	  slen >>= encap;
	  if (slen > 1) {
	    encap.put_char_array((const CORBA::Char*)((const char*)str),
				 slen);
	  }
	  // Member typecode
	  r_marshalTypeCode(encap, PyTuple_GET_ITEM(mem, 2), edom);
	}
	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_enum:
      {
	//	cout << "enum" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	PyObject* mems;
	PyObject* mem;

	// Count
	mems = PyTuple_GET_ITEM(d_o, 3); OMNIORB_ASSERT(PyTuple_Check(mems));
	CORBA::ULong cnt = PyTuple_GET_SIZE(mems);
	cnt >>= encap;

	for (CORBA::ULong i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i);
	  OMNIORB_ASSERT(PyInstance_Check(mem));

	  // Member name
	  t_o = PyDict_GetItemString(((PyInstanceObject*)mem)->in_dict,
				     (char*)"_n");
	  MARSHAL_PYSTRING(encap, t_o);
	}
      	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_sequence:
      {
	//	cout << "sequence" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// Element type
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 1), edom);

	// Max length
	t_o = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyInt_Check(t_o));
	CORBA::ULong max_len = PyInt_AS_LONG(t_o);
	max_len >>= encap;

	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_array:
      {
	//	cout << "array" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// Element type
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 1), edom);

	// Length
	t_o = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyInt_Check(t_o));
	CORBA::ULong arr_len = PyInt_AS_LONG(t_o);
	arr_len >>= encap;

	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_alias:
      {
	//	cout << "alias" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// TypeCode
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 3), edom);

      	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case CORBA::tk_except:
      {
	//	cout << "except" << endl;
	dom.add(d_o, tc_offset);

	cdrEncapsulationStream encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); OMNIORB_ASSERT(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Count
	CORBA::ULong cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;
	cnt >>= encap;

	CORBA::ULong i, j, slen;
	char* str;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o = PyTuple_GET_ITEM(d_o, j++);
	  OMNIORB_ASSERT(PyString_Check(t_o));

	  str  = PyString_AS_STRING(t_o);
	  slen = PyString_GET_SIZE(t_o) + 1;

	  if (str[0] == '_') { --slen; ++str; }
	  slen >>= encap;
	  if (slen > 1) {
	    encap.put_char_array((const CORBA::Char*)((const char*)str),
				 slen);
	  }
	  // member type
	  r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, j++), edom);
	}
	// Send encapsulation
	::operator>>=((CORBA::ULong)encap.bufSize(), stream);
	stream.put_char_array((CORBA::Char*)encap.bufPtr(), encap.bufSize());
      }
      break;

    case 0xffffffff:
      {
	//	cout << "indirect" << endl;
	t_o = PyTuple_GET_ITEM(d_o, 1); OMNIORB_ASSERT(PyList_Check(t_o));
	t_o = PyList_GET_ITEM(t_o, 0); OMNIORB_ASSERT(t_o);

	CORBA::Long position, offset;

	if (!dom.lookup(t_o, position))
	  OMNIORB_THROW(BAD_TYPECODE, 0, CORBA::COMPLETED_NO);

	offset = position - (tc_offset + 4);

	//	cout << "indirect to " << offset << endl;

	offset >>= stream;
      }
      break;

    default:
      OMNIORB_THROW(BAD_TYPECODE, 0, CORBA::COMPLETED_NO);
    }
  }
  //  cout << "marshalTypeCode() done." << endl;
}



// Macro to unmarshal a PyString from a Mem or NetBufferedStream:
#define UNMARSHAL_PYSTRING(_stream, _pystring) { \
  CORBA::String_member _str_tmp; \
  _str_tmp <<= _stream; \
  _pystring = PyString_FromString(_str_tmp._ptr); \
}

PyObject*
r_unmarshalTypeCode(cdrStream& stream, OffsetDescriptorMap& odm)
{
  PyObject* d_o = 0; // Descriptor object to build
  PyObject* t_o;

  //  cout << "unmarshal typecode... " << flush;

  // Read kind
  CORBA::ULong tk; tk <<= stream;

  // Offset of current TypeCode
  CORBA::Long tc_offset = stream.currentInputPtr() - 4;

  switch (tk) {
  case CORBA::tk_null:
  case CORBA::tk_void:
  case CORBA::tk_short:
  case CORBA::tk_long:
  case CORBA::tk_ushort:
  case CORBA::tk_ulong:
  case CORBA::tk_float:
  case CORBA::tk_double:
  case CORBA::tk_boolean:
  case CORBA::tk_char:
  case CORBA::tk_octet:
  case CORBA::tk_any:
  case CORBA::tk_TypeCode:
  case CORBA::tk_Principal:
#ifdef HAS_LongLong
  case CORBA::tk_longlong:
  case CORBA::tk_ulonglong:
#endif
    {
      //      cout << "simple: " << tk << endl;
      d_o = PyInt_FromLong(tk); odm.add(d_o, tc_offset);
    }
    break;

  case CORBA::tk_string:
    {
      //      cout << "string" << endl;
      d_o = PyTuple_New(2); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      CORBA::ULong len; len <<= stream;

      PyTuple_SET_ITEM(d_o, 1, PyInt_FromLong(len));
    }
    break;

  case CORBA::tk_objref:
    {
      //      cout << "objref" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      d_o = PyTuple_New(3); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      // RepoId and name
      UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 1, t_o);
      UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 2, t_o);
    }
    break;

  case CORBA::tk_struct:
    {
      //      cout << "struct" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the structure
	Py_INCREF(d_o);
	Py_DECREF(repoId);
	//?? Is is worth checking the TypeCodes for equivalence?
      }
      else {
	// Don't know this structure
	OffsetDescriptorMap eodm(odm, tc_offset + 8);

	PyObject* name; UNMARSHAL_PYSTRING(encap, name);

	CORBA::ULong cnt; cnt <<= encap;

	d_o = PyTuple_New(cnt * 2 + 4);	odm.add(d_o, tc_offset);
	PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));
	PyTuple_SET_ITEM(d_o, 2, repoId);
	PyTuple_SET_ITEM(d_o, 3, name);

	PyObject* mems = PyTuple_New(cnt);

	CORBA::ULong i, j;

	PyObject* word;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  UNMARSHAL_PYSTRING(encap, t_o);

	  if ((word = PyDict_GetItem(omniPy::pyomniORBwordMap, t_o))) {
	    Py_DECREF(t_o);
	    t_o = word;
	  }
	  PyTuple_SET_ITEM(d_o, j++, t_o);
	  Py_INCREF(t_o);
	  PyTuple_SET_ITEM(mems, i, t_o);

	  // member type
	  t_o = r_unmarshalTypeCode(encap, eodm);
	  PyTuple_SET_ITEM(d_o, j++, t_o);
	}

	// Create class object:
	// *** Could be made faster by finding the createUnknownStruct
	// function only once, and manually building the argument typle
	t_o = PyObject_GetAttrString(omniPy::pyomniORBmodule,
				     (char*)"createUnknownStruct");
	OMNIORB_ASSERT(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, (char*)"OO", repoId, mems);
	OMNIORB_ASSERT(t_o && PyClass_Check(t_o));

	Py_DECREF(mems);

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case CORBA::tk_union:
    {
      //      cout << "union" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the union
	Py_INCREF(d_o);
	Py_DECREF(repoId);
	//?? Is is worth checking the TypeCodes for equivalence?
      }
      else {
	// Don't know this union
	OffsetDescriptorMap eodm(odm, tc_offset + 8);

	d_o = PyTuple_New(9); odm.add(d_o, tc_offset);
	PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));
	PyTuple_SET_ITEM(d_o, 2, repoId);

	// name
	UNMARSHAL_PYSTRING(encap, t_o);
	PyTuple_SET_ITEM(d_o, 3, t_o);

	// discriminant type
	PyObject* discriminant = r_unmarshalTypeCode(encap, eodm);
	PyTuple_SET_ITEM(d_o, 4, discriminant);

	// default used
	CORBA::Long def_used; def_used <<= encap;
	PyTuple_SET_ITEM(d_o, 5, PyInt_FromLong(def_used));

	if (def_used < 0) {
	  Py_INCREF(Py_None);
	  PyTuple_SET_ITEM(d_o, 7, Py_None);
	}

	// count
	CORBA::ULong cnt; cnt <<= encap;
	PyObject* mems = PyTuple_New(cnt);
	PyTuple_SET_ITEM(d_o, 6, mems);

	PyObject* dict = PyDict_New();
	PyTuple_SET_ITEM(d_o, 8, dict);

	PyObject* mem;
	PyObject* label;
	PyObject* word;

	for (CORBA::ULong i=0; i<cnt; i++) {
	  mem = PyTuple_New(3);

	  // Label value
	  label = omniPy::unmarshalPyObject(encap, discriminant);
	  PyTuple_SET_ITEM(mem, 0, label);

	  // Member name
	  UNMARSHAL_PYSTRING(encap, t_o);

	  if ((word = PyDict_GetItem(omniPy::pyomniORBwordMap, t_o))) {
	    Py_DECREF(t_o);
	    t_o = word;
	  }
	  PyTuple_SET_ITEM(mem, 1, t_o);

	  // Member type
	  t_o = r_unmarshalTypeCode(encap, eodm);
	  PyTuple_SET_ITEM(mem, 2, t_o);

	  PyTuple_SET_ITEM(mems, i, mem);

	  if (def_used > 0 && i == (CORBA::ULong)def_used) {
	    Py_INCREF(mem); // **** Error here?
	    PyTuple_SET_ITEM(d_o, 7, mem);
	  }
	  else {
	    Py_INCREF(mem); // ****
	    Py_INCREF(label); // ****
	    PyDict_SetItem(dict, label, mem);
	  }
	}

	// Create class object
	t_o = PyObject_GetAttrString(omniPy::pyomniORBmodule,
				     (char*)"createUnknownUnion");
	OMNIORB_ASSERT(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, (char*)"OiO", repoId, def_used, mems);
	OMNIORB_ASSERT(t_o && PyClass_Check(t_o));

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      //      cout << "enum" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the enum
	Py_INCREF(d_o);
	Py_DECREF(repoId);
	//?? Is is worth checking the TypeCodes for equivalence?
      }
      else {
	// Don't know this enum
	d_o = PyTuple_New(4); odm.add(d_o, tc_offset);
	PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));
	PyTuple_SET_ITEM(d_o, 1, repoId);

	// name
	UNMARSHAL_PYSTRING(encap, t_o);
	PyTuple_SET_ITEM(d_o, 2, t_o);

	// count
	CORBA::ULong cnt; cnt <<= encap;
	PyObject* mems = PyTuple_New(cnt);
	PyTuple_SET_ITEM(d_o, 3, mems);

	PyObject* eclass = PyObject_GetAttrString(omniPy::pyomniORBmodule,
						  (char*)"EnumItem");
	PyObject* aclass = PyObject_GetAttrString(omniPy::pyomniORBmodule,
						  (char*)"AnonymousEnumItem");
	OMNIORB_ASSERT(eclass && PyClass_Check(eclass));
	OMNIORB_ASSERT(aclass && PyClass_Check(aclass));

	// members
	for (CORBA::ULong i=0; i<cnt; i++) {
	  UNMARSHAL_PYSTRING(encap, t_o);

	  if (PyString_GET_SIZE(t_o) > 0)
	    t_o = PyObject_CallFunction(eclass, (char*)"Oi", t_o, i);
	  else
	    t_o = PyObject_CallFunction(aclass, (char*)"i", i);

	  OMNIORB_ASSERT(t_o && PyInstance_Check(t_o));

	  PyTuple_SET_ITEM(mems, i, t_o);
	}
      }
    }
    break;

  case CORBA::tk_sequence:
    {
      //      cout << "sequence" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      OffsetDescriptorMap eodm(odm, tc_offset + 8);

      d_o = PyTuple_New(3); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      // Element type
      t_o = r_unmarshalTypeCode(encap, eodm);
      PyTuple_SET_ITEM(d_o, 1, t_o);

      // Max length
      CORBA::ULong len; len <<= encap;
      PyTuple_SET_ITEM(d_o, 2, PyInt_FromLong(len));
    }
    break;

  case CORBA::tk_array:
    {
      //      cout << "array" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      OffsetDescriptorMap eodm(odm, tc_offset + 8);

      d_o = PyTuple_New(3); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      // Element type
      t_o = r_unmarshalTypeCode(encap, eodm);
      PyTuple_SET_ITEM(d_o, 1, t_o);

      // Length
      CORBA::ULong len; len <<= encap;
      PyTuple_SET_ITEM(d_o, 2, PyInt_FromLong(len));
    }
    break;

  case CORBA::tk_alias:
    {
      //      cout << "alias" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the alias
	Py_INCREF(d_o);
	Py_DECREF(repoId);
      }
      else {
	OffsetDescriptorMap eodm(odm, tc_offset + 8);

	d_o = PyTuple_New(4); odm.add(d_o, tc_offset);
	PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));
	PyTuple_SET_ITEM(d_o, 1, repoId);

	// repoId and name
	UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 2, t_o);

	// TypeCode
	t_o = r_unmarshalTypeCode(encap, eodm);
	PyTuple_SET_ITEM(d_o, 3, t_o);
      }
    }
    break;

  case CORBA::tk_except:
    {
      //      cout << "except" << endl;
      CORBA::ULong size; size <<= stream;
      cdrEncapsulationStream encap(stream, size);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the exception
	Py_INCREF(d_o);
	Py_DECREF(repoId);
	//?? Is is worth checking the TypeCodes for equivalence?
      }
      else {
	// Don't know this exception
	OffsetDescriptorMap eodm(odm, tc_offset + 8);

	PyObject* name; UNMARSHAL_PYSTRING(encap, name);

	CORBA::ULong cnt; cnt <<= encap;

	d_o = PyTuple_New(cnt * 2 + 4); odm.add(d_o, tc_offset);
	PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));
	PyTuple_SET_ITEM(d_o, 2, repoId);
	PyTuple_SET_ITEM(d_o, 3, name);

	PyObject* mems = PyTuple_New(cnt);

	CORBA::ULong i, j;

	PyObject* word;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  UNMARSHAL_PYSTRING(encap, t_o);

	  if ((word = PyDict_GetItem(omniPy::pyomniORBwordMap, t_o))) {
	    Py_DECREF(t_o);
	    t_o = word;
	  }
	  PyTuple_SET_ITEM(d_o, j++, t_o);
	  Py_INCREF(t_o);
	  PyTuple_SET_ITEM(mems, i, t_o);

	  // member type
	  t_o = r_unmarshalTypeCode(encap, eodm);
	  PyTuple_SET_ITEM(d_o, j++, t_o);
	}

	// Create class object:
	// *** Could be made faster by finding the createUnknownStruct
	// function only once, and manually building the argument typle
	t_o = PyObject_GetAttrString(omniPy::pyomniORBmodule,
				     (char*)"createUnknownUserException");
	OMNIORB_ASSERT(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, (char*)"OO", repoId, mems);
	OMNIORB_ASSERT(t_o && PyClass_Check(t_o));

	Py_DECREF(mems);

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case 0xffffffff:
    {
      //      cout << "indirect" << endl;
      CORBA::ULong position, offset;

      offset  <<= stream;
      position  = tc_offset + 4 + offset;

      if (!odm.lookup(t_o, position))
	OMNIORB_THROW(BAD_TYPECODE, 0, CORBA::COMPLETED_NO);

      d_o = PyTuple_New(2); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      PyObject* olist = PyList_New(1);
      PyList_SET_ITEM(olist, 0, t_o);
      PyTuple_SET_ITEM(d_o, 1, olist);
    }
    break;

  default:
    OMNIORB_THROW(BAD_TYPECODE, 0, CORBA::COMPLETED_NO);
  }

  //  cout << "r_unmarshalTypeCode ended." << endl;
  return d_o;
}



void
omniPy::marshalTypeCode(cdrStream& stream, PyObject* d_o)
{
  DescriptorOffsetMap dom;
  r_marshalTypeCode(stream, d_o, dom);
}


PyObject*
omniPy::unmarshalTypeCode(cdrStream& stream)
{
  OffsetDescriptorMap odm;
  return r_unmarshalTypeCode(stream, odm);
}
