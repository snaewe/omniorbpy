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
// Revision 1.6  1999/12/15 11:04:05  dpg1
// Small bug in alignedSize() for enum.
//
// Revision 1.5  1999/09/24 09:22:02  dpg1
// Added copyright notices.
//
// Revision 1.4  1999/09/22 15:46:12  dpg1
// Fake POA implemented.
//
// Revision 1.3  1999/07/29 14:21:38  dpg1
// Handling of reserved words.
//
// Revision 1.2  1999/07/19 14:41:03  dpg1
// createPyTypeCodeObject() removed.
//
// Revision 1.1  1999/07/19 12:47:43  dpg1
// Initial revision
//

#include <omnipy.h>

#include <iostream.h>


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


// Function prototypes
void r_marshalTypeCode(NetBufferedStream&   stream,
		       PyObject*            d_o,
		       DescriptorOffsetMap& dom);

void r_marshalTypeCode(MemBufferedStream&   stream,
		       PyObject*            d_o,
		       DescriptorOffsetMap& dom);

PyObject* r_unmarshalTypeCode(NetBufferedStream&   stream,
			      OffsetDescriptorMap& odm);

PyObject* r_unmarshalTypeCode(MemBufferedStream&   stream,
			      OffsetDescriptorMap& odm);


CORBA::ULong
r_alignedSizeTypeCode(CORBA::ULong msgsize, PyObject* d_o,
		      DescriptorOffsetMap& dom)
{
  CORBA::Long tc_offset;

  // If this TypeCode has already been sent, use an indirection:
  if (omniORB::useTypeCodeIndirections && dom.lookup(d_o, tc_offset)) {
    return omni::align_to(msgsize, omni::ALIGN_4) + 8;
  }
  else {
    CORBA::ULong   tk;
    PyObject*      t_o;
    CORBA::Boolean tup;

    if (PyTuple_Check(d_o)) {
      t_o = PyTuple_GET_ITEM(d_o, 0); assert(PyInt_Check(t_o));
      tk  = PyInt_AS_LONG(t_o);
      tup = 1;
    }
    else {
      assert(PyInt_Check(d_o));
      tk  = PyInt_AS_LONG(d_o);
      tup = 0;
    }

    // Offset of this TypeCode:
    tc_offset = omni::align_to(msgsize, omni::ALIGN_4);

    // Make space for the kind:
    msgsize = tc_offset + 4;

    switch (tk) {
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
      // Nothing more to be sent
      break;

    case CORBA::tk_string:
      // Another ULong to be sent. We're already aligned
      msgsize += 4;
      break;

    case CORBA::tk_objref:
      {
	// Add entry to descriptor offset map:
	dom.add(d_o, tc_offset);

	// Space for encapsulation length (already aligned) and byte order
	msgsize += 5;

	// RepoId
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	// Name
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;
      }
      break;

    case CORBA::tk_struct:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// RepoId, name
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	t_o = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	// Count
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4;

	int cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;

	int i, j;
	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o = PyTuple_GET_ITEM(d_o, j++); assert(PyString_Check(t_o));
	  msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	    PyString_GET_SIZE(t_o) + 1;

	  // If the member name starts with '_', it has been named
	  // that way to avoid a clash with a Python reserved word. It
	  // needs to be sent without the _
	  if (PyString_AS_STRING(t_o)[0] == '_') --msgsize;

	  // member type
	  msgsize = r_alignedSizeTypeCode(msgsize, PyTuple_GET_ITEM(d_o, j++),
					  dom);
	}
      }
      break;

    case CORBA::tk_union:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// RepoId, name
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	t_o = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	// Discriminant type
	t_o = PyTuple_GET_ITEM(d_o, 4);
	msgsize = r_alignedSizeTypeCode(msgsize, t_o, dom);

	CORBA::ULong      dtype, dsize;
	omni::alignment_t dalign;

	if (PyTuple_Check(t_o)) t_o = PyTuple_GET_ITEM(t_o, 0);
	assert(PyInt_Check(t_o));
	dtype = PyInt_AS_LONG(t_o);

	switch (dtype) {
	case CORBA::tk_boolean:
	case CORBA::tk_char:
	case CORBA::tk_octet:
	  dsize = 1; dalign = omni::ALIGN_1; break;
	case CORBA::tk_short:
	case CORBA::tk_ushort:
	  dsize = 2; dalign = omni::ALIGN_2; break;
	case CORBA::tk_long:
	case CORBA::tk_ulong:
	case CORBA::tk_enum:
	  dsize = 4; dalign = omni::ALIGN_4; break;
	default:
	  throw CORBA::BAD_TYPECODE();
	}

	// Default used, count
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 8;

	// Members
	PyObject* mems;
	PyObject* mem;
	mems = PyTuple_GET_ITEM(d_o, 6); assert(PyTuple_Check(mems));
	int cnt = PyTuple_GET_SIZE(mems);

	for (int i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); assert(PyTuple_Check(mem));

	  // Label value
	  msgsize = omni::align_to(msgsize, dalign) + dsize;

	  // Member name
	  t_o = PyTuple_GET_ITEM(mem, 1); assert(PyString_Check(t_o));
	  msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	    PyString_GET_SIZE(t_o) + 1;

	  // If the member name starts with '_', it has been named
	  // that way to avoid a clash with a Python reserved word. It
	  // needs to be sent without the _
	  if (PyString_AS_STRING(t_o)[0] == '_') --msgsize;
	  
	  // Member type
	  msgsize = r_alignedSizeTypeCode(msgsize, PyTuple_GET_ITEM(mem, 2),
					  dom);
	}
      }
      break;

    case CORBA::tk_enum:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// RepoId, name
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	// Member count
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4;

	// Members
	PyObject* mems = PyTuple_GET_ITEM(d_o, 3); assert(PyTuple_Check(mems));
	PyObject* mem;

	int cnt = PyTuple_GET_SIZE(mems);

	for (int i=0; i<cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); assert(PyInstance_Check(mem));
	  t_o = PyObject_GetAttrString(mem, "_n"); assert(PyString_Check(t_o));
	  msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	    PyString_GET_SIZE(t_o) + 1;
	}
      }
      break;

    case CORBA::tk_sequence:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// Element type
	msgsize = r_alignedSizeTypeCode(msgsize, PyTuple_GET_ITEM(d_o, 1),
					dom);

	// Max length
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4;
      }
      break;

    case CORBA::tk_array:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// Element type
	msgsize = r_alignedSizeTypeCode(msgsize, PyTuple_GET_ITEM(d_o, 1),
					dom);

	// Max length
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4;
      }
      break;

    case CORBA::tk_alias:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// RepoId, name
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	// alias type
	msgsize = r_alignedSizeTypeCode(msgsize, PyTuple_GET_ITEM(d_o, 3),
					dom);
      }
      break;

    case CORBA::tk_except:
      {
	dom.add(d_o, tc_offset);
	msgsize += 5;

	// RepoId, name
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	t_o = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	  PyString_GET_SIZE(t_o) + 1;

	// Count
	msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4;

	int cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;

	int i, j;
	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o = PyTuple_GET_ITEM(d_o, j++); assert(PyString_Check(t_o));
	  msgsize = omni::align_to(msgsize, omni::ALIGN_4) + 4 +
	    PyString_GET_SIZE(t_o) + 1;

	  // If the member name starts with '_', it has been named
	  // that way to avoid a clash with a Python reserved word. It
	  // needs to be sent without the _
	  if (PyString_AS_STRING(t_o)[0] == '_') --msgsize;

	  // member type
	  msgsize = r_alignedSizeTypeCode(msgsize, PyTuple_GET_ITEM(d_o, j++),
					  dom);
	}
      }
      break;

    case 0xffffffff:
      msgsize += 4;
      break;

    default:
      throw CORBA::BAD_TYPECODE();
    }
  }
  //  cout << "alignedSizeTypecode returning " << msgsize << endl;
  return msgsize;
}


// Macro to marshal a PyString into a Mem or NetBufferedStream:
#define MARSHAL_PYSTRING(_stream, _pystring) { \
  CORBA::ULong _slen = PyString_GET_SIZE(_pystring) + 1; \
  _slen >>= _stream; \
  if (_slen > 1) { \
    char* _str = PyString_AS_STRING(_pystring); \
    _stream.put_char_array((const CORBA::Char*)((const char*)_str), _slen); \
  } \
}

void
r_marshalTypeCode(NetBufferedStream&   stream,
		  PyObject*            d_o,
		  DescriptorOffsetMap& dom)
{
  CORBA::Long tc_offset;

  //  cout << endl << "marshalTypeCode... " << flush;

  // If this TypeCode has already been sent, use an indirection:
  if (omniORB::useTypeCodeIndirections && dom.lookup(d_o, tc_offset)) {

    CORBA::ULong tk_ind = 0xffffffff;
    CORBA::Long  offset = tc_offset - stream.WrMessageAlreadyWritten();

    tk_ind >>= stream;
    offset >>= stream;

    //    cout << "indirection to " << offset << endl;
  }
  else {
    CORBA::ULong   tk;
    PyObject*      t_o;

    if (PyTuple_Check(d_o)) {
      t_o = PyTuple_GET_ITEM(d_o, 0); assert(PyInt_Check(t_o));
      tk  = PyInt_AS_LONG(t_o);
    }
    else {
      assert(PyInt_Check(d_o));
      tk  = PyInt_AS_LONG(d_o);
    }

    // Marshal the kind
    tk >>= stream;

    // Offset of this TypeCode (within the current encapsulation if any):
    tc_offset = stream.WrMessageAlreadyWritten() - 4;

    //    cout << "offset = " << tc_offset << " " << flush;
    
    switch (tk) {
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
      // Nothing more to be sent
      //      cout << "simple: " << tk << endl;
      break;

    case CORBA::tk_string:
      {
	//	cout << "string" << endl;
	// Send max length
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= stream;
      }
      break;

    case CORBA::tk_objref:
      {
	//	cout << "objref" << endl;
	// Add entry to descriptor offset map:
	dom.add(d_o, tc_offset);

	// MemBufferedStream for the encapsulation
	MemBufferedStream encap;
	encap.byteOrder() >>= encap;

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_struct:
      {
	//	cout << "struct" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Count
	CORBA::ULong cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;
	cnt >>= encap;

	CORBA::ULong i, j, slen;
	char* str;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o  = PyTuple_GET_ITEM(d_o, j++); assert(PyString_Check(t_o));
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
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_union:
      {
	//	cout << "union" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Discriminant type
	PyObject* discriminant = PyTuple_GET_ITEM(d_o, 4);
	r_marshalTypeCode(encap, discriminant, edom);

	// Default used
	t_o              = PyTuple_GET_ITEM(d_o, 5); assert(PyInt_Check(t_o));
	CORBA::Long defu = PyInt_AS_LONG(t_o);
	defu >>= encap;

	PyObject* mems;
	PyObject* mem;

	// Count
	mems = PyTuple_GET_ITEM(d_o, 6); assert(PyTuple_Check(mems));
	CORBA::ULong cnt = PyTuple_GET_SIZE(mems);
	cnt >>= encap;

	CORBA::ULong slen;
	char*        str;

	for (CORBA::ULong i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); assert(PyTuple_Check(mem));

	  // Label value
	  omniPy::marshalPyObject(encap, discriminant,
				  PyTuple_GET_ITEM(mem, 0));

	  // Member name
	  t_o  = PyTuple_GET_ITEM(mem, 1); assert(PyString_Check(t_o));
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
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_enum:
      {
	//	cout << "enum" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	PyObject* mems;
	PyObject* mem;

	// Count
	mems = PyTuple_GET_ITEM(d_o, 3); assert(PyTuple_Check(mems));
	CORBA::ULong cnt = PyTuple_GET_SIZE(mems);
	cnt >>= encap;

	for (CORBA::ULong i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); assert(PyInstance_Check(mem));

	  // Member name
	  t_o = PyDict_GetItemString(((PyInstanceObject*)mem)->in_dict, "_n");
	  MARSHAL_PYSTRING(encap, t_o);
	}
      	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_sequence:
      {
	//	cout << "sequence" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// Element type
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 1), edom);

	// Max length
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= encap;

	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_array:
      {
	//	cout << "array" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// Element type
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 1), edom);

	// Length
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= encap;

	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_alias:
      {
	//	cout << "alias" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// TypeCode
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 3), edom);

      	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_except:
      {
	//	cout << "except" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Count
	CORBA::ULong cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;
	cnt >>= encap;

	CORBA::ULong i, j, slen;
	char* str;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o = PyTuple_GET_ITEM(d_o, j++); assert(PyString_Check(t_o));

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
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case 0xffffffff:
      {
	//	cout << "indirect" << endl;
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyList_Check(t_o));
	t_o = PyList_GET_ITEM(t_o, 0); assert(t_o);

	CORBA::Long position, offset;

	if (!dom.lookup(t_o, position))
	  throw CORBA::BAD_TYPECODE();

	offset = position - (tc_offset + 4);

	//	cout << "indirect to " << offset << endl;

	offset >>= stream;
      }
      break;

    default:
      throw CORBA::BAD_TYPECODE();
    }
  }
  //  cout << "marshalTypeCode() done." << endl;
}


void
r_marshalTypeCode(MemBufferedStream&   stream,
		  PyObject*            d_o,
		  DescriptorOffsetMap& dom)
{
  CORBA::Long tc_offset;

  //  cout << endl << "marshalTypeCode... " << flush;

  // If this TypeCode has already been sent, use an indirection:
  if (omniORB::useTypeCodeIndirections && dom.lookup(d_o, tc_offset)) {

    CORBA::ULong tk_ind = 0xffffffff;
    CORBA::Long  offset = tc_offset - stream.WrMessageAlreadyWritten();

    tk_ind >>= stream;
    offset >>= stream;

    //    cout << "indirection to " << offset << endl;
  }
  else {
    CORBA::ULong   tk;
    PyObject*      t_o;

    if (PyTuple_Check(d_o)) {
      t_o = PyTuple_GET_ITEM(d_o, 0); assert(PyInt_Check(t_o));
      tk  = PyInt_AS_LONG(t_o);
    }
    else {
      assert(PyInt_Check(d_o));
      tk  = PyInt_AS_LONG(d_o);
    }

    // Marshal the kind
    tk >>= stream;

    // Offset of this TypeCode (within the current encapsulation if any):
    tc_offset = stream.WrMessageAlreadyWritten() - 4;

    //    cout << "offset = " << tc_offset << " " << flush;
    
    switch (tk) {
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
      // Nothing more to be sent
      //      cout << "simple: " << tk << endl;
      break;

    case CORBA::tk_string:
      {
	//	cout << "string" << endl;
	// Send max length
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= stream;
      }
      break;

    case CORBA::tk_objref:
      {
	//	cout << "objref" << endl;
	// Add entry to descriptor offset map:
	dom.add(d_o, tc_offset);

	// MemBufferedStream for the encapsulation
	MemBufferedStream encap;
	encap.byteOrder() >>= encap;

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_struct:
      {
	//	cout << "struct" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Count
	CORBA::ULong cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;
	cnt >>= encap;

	CORBA::ULong i, j, slen;
	char* str;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o  = PyTuple_GET_ITEM(d_o, j++); assert(PyString_Check(t_o));
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
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_union:
      {
	//	cout << "union" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Discriminant type
	PyObject* discriminant = PyTuple_GET_ITEM(d_o, 4);
	r_marshalTypeCode(encap, discriminant, edom);

	// Default used
	t_o              = PyTuple_GET_ITEM(d_o, 5); assert(PyInt_Check(t_o));
	CORBA::Long defu = PyInt_AS_LONG(t_o);
	defu >>= encap;

	PyObject* mems;
	PyObject* mem;

	// Count
	mems = PyTuple_GET_ITEM(d_o, 6); assert(PyTuple_Check(mems));
	CORBA::ULong cnt = PyTuple_GET_SIZE(mems);
	cnt >>= encap;

	CORBA::ULong slen;
	char*        str;

	for (CORBA::ULong i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); assert(PyTuple_Check(mem));

	  // Label value
	  omniPy::marshalPyObject(encap, discriminant,
				  PyTuple_GET_ITEM(mem, 0));

	  // Member name
	  t_o  = PyTuple_GET_ITEM(mem, 1); assert(PyString_Check(t_o));
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
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_enum:
      {
	//	cout << "enum" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	PyObject* mems;
	PyObject* mem;

	// Count
	mems = PyTuple_GET_ITEM(d_o, 3); assert(PyTuple_Check(mems));
	CORBA::ULong cnt = PyTuple_GET_SIZE(mems);
	cnt >>= encap;

	for (CORBA::ULong i=0; i < cnt; i++) {
	  mem = PyTuple_GET_ITEM(mems, i); assert(PyInstance_Check(mem));

	  // Member name
	  t_o = PyDict_GetItemString(((PyInstanceObject*)mem)->in_dict, "_n");
	  MARSHAL_PYSTRING(encap, t_o);
	}
      	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_sequence:
      {
	//	cout << "sequence" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// Element type
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 1), edom);

	// Max length
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= encap;

	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_array:
      {
	//	cout << "array" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// Element type
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 1), edom);

	// Length
	t_o = PyTuple_GET_ITEM(d_o, 2); assert(PyInt_Check(t_o));
	CORBA::ULong len = PyInt_AS_LONG(t_o);
	len >>= encap;

	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_alias:
      {
	//	cout << "alias" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 1); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// TypeCode
	r_marshalTypeCode(encap, PyTuple_GET_ITEM(d_o, 3), edom);

      	// Send encapsulation
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case CORBA::tk_except:
      {
	//	cout << "except" << endl;
	dom.add(d_o, tc_offset);

	MemBufferedStream encap;
	encap.byteOrder() >>= encap;
	DescriptorOffsetMap edom(dom, tc_offset + 8);

	// RepoId and name
	t_o    = PyTuple_GET_ITEM(d_o, 2); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	t_o    = PyTuple_GET_ITEM(d_o, 3); assert(PyString_Check(t_o));
	MARSHAL_PYSTRING(encap, t_o);

	// Count
	CORBA::ULong cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;
	cnt >>= encap;

	CORBA::ULong i, j, slen;
	char* str;

	for (i=0, j=4; i < cnt; i++) {
	  // member name
	  t_o = PyTuple_GET_ITEM(d_o, j++); assert(PyString_Check(t_o));

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
	CORBA::ULong elen = encap.alreadyWritten();
	elen >>= stream;
	stream.put_char_array((CORBA::Char*)encap.data(), elen);
      }
      break;

    case 0xffffffff:
      {
	//	cout << "indirect" << endl;
	t_o = PyTuple_GET_ITEM(d_o, 1); assert(PyList_Check(t_o));
	t_o = PyList_GET_ITEM(t_o, 0); assert(t_o);

	CORBA::Long position, offset;

	if (!dom.lookup(t_o, position))
	  throw CORBA::BAD_TYPECODE();

	offset = position - (tc_offset + 4); // Position in stream of
	                                     // indirect pointer

	//	cout << "indirect to " << offset << endl;

	offset >>= stream;
      }
      break;

    default:
      throw CORBA::BAD_TYPECODE();
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
r_unmarshalTypeCode(NetBufferedStream& stream, OffsetDescriptorMap& odm)
{
  PyObject* d_o; // Descriptor object to build
  PyObject* t_o;

  //  cout << "unmarshal typecode... " << flush;

  // Read kind
  CORBA::ULong tk; tk <<= stream;

  // Offset of current TypeCode
  CORBA::Long tc_offset = stream.RdMessageAlreadyRead() - 4;

  switch (tk) {
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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the structure
	Py_INCREF(d_o);
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
				     "createUnknownStruct");
	assert(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, "OO", repoId, mems);
	assert(t_o && PyClass_Check(t_o));

	Py_DECREF(mems);

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case CORBA::tk_union:
    {
      //      cout << "union" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the union
	Py_INCREF(d_o);
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
	    Py_INCREF(mem);
	    PyTuple_SET_ITEM(d_o, 7, mem);
	  }
	  else {
	    Py_INCREF(mem);
	    Py_INCREF(label);
	    PyDict_SetItem(dict, label, mem);
	  }
	}

	// Create class object
	t_o = PyObject_GetAttrString(omniPy::pyomniORBmodule,
				     "createUnknownUnion");
	assert(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, "OiO", repoId, def_used, mems);
	assert(t_o && PyClass_Check(t_o));

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      //      cout << "enum" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the structure
	Py_INCREF(d_o);
	//?? Is is worth checking the TypeCodes for equivalence?
      }
      else {
	// Don't know this structure
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
						  "EnumItem");
	PyObject* aclass = PyObject_GetAttrString(omniPy::pyomniORBmodule,
						  "AnonymousEnumItem");
	assert(eclass && PyClass_Check(eclass));
	assert(aclass && PyClass_Check(aclass));

	// members
	for (CORBA::ULong i=0; i<cnt; i++) {
	  UNMARSHAL_PYSTRING(encap, t_o);

	  if (PyString_GET_SIZE(t_o) > 0)
	    t_o = PyObject_CallFunction(eclass, "Oi", t_o, i);
	  else
	    t_o = PyObject_CallFunction(aclass, "i", i);

	  assert(t_o && PyInstance_Check(t_o));

	  PyTuple_SET_ITEM(mems, i, t_o);
	}
      }
    }
    break;

  case CORBA::tk_sequence:
    {
      //      cout << "sequence" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      OffsetDescriptorMap eodm(odm, tc_offset + 8);

      d_o = PyTuple_New(4); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      // repoId and name
      UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 1, t_o);
      UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 2, t_o);

      // TypeCode
      t_o = r_unmarshalTypeCode(encap, eodm);
      PyTuple_SET_ITEM(d_o, 3, t_o);
    }
    break;

  case CORBA::tk_except:
    {
      //      cout << "except" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the exception
	Py_INCREF(d_o);
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
				     "createUnknownUserException");
	assert(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, "OO", repoId, mems);
	assert(t_o && PyClass_Check(t_o));

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
	throw CORBA::BAD_TYPECODE();

      d_o = PyTuple_New(2); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      PyObject* olist = PyList_New(1);
      PyList_SET_ITEM(olist, 0, t_o);
      PyTuple_SET_ITEM(d_o, 1, olist);
    }
    break;

  default:
    throw CORBA::BAD_TYPECODE();
  }

  //  cout << "r_unmarshalTypeCode ended." << endl;
  return d_o;
}



PyObject*
r_unmarshalTypeCode(MemBufferedStream& stream, OffsetDescriptorMap& odm)
{
  PyObject* d_o; // Descriptor object to build
  PyObject* t_o;

  //  cout << "unmarshal typecode... " << flush;

  // Read kind
  CORBA::ULong tk; tk <<= stream;

  // Offset of current TypeCode
  CORBA::Long tc_offset = stream.RdMessageAlreadyRead() - 4;

  switch (tk) {
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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the structure
	Py_INCREF(d_o);
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
				     "createUnknownStruct");
	assert(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, "OO", repoId, mems);
	assert(t_o && PyClass_Check(t_o));

	Py_DECREF(mems);

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case CORBA::tk_union:
    {
      //      cout << "union" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the union
	Py_INCREF(d_o);
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
	    Py_INCREF(mem);
	    PyTuple_SET_ITEM(d_o, 7, mem);
	  }
	  else {
	    Py_INCREF(mem);
	    Py_INCREF(label);
	    PyDict_SetItem(dict, label, mem);
	  }
	}

	// Create class object
	t_o = PyObject_GetAttrString(omniPy::pyomniORBmodule,
				     "createUnknownUnion");
	assert(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, "OiO", repoId, def_used, mems);
	assert(t_o && PyClass_Check(t_o));

	PyTuple_SET_ITEM(d_o, 1, t_o);
      }
    }
    break;

  case CORBA::tk_enum:
    {
      //      cout << "enum" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the structure
	Py_INCREF(d_o);
	//?? Is is worth checking the TypeCodes for equivalence?
      }
      else {
	// Don't know this structure
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
						  "EnumItem");
	PyObject* aclass = PyObject_GetAttrString(omniPy::pyomniORBmodule,
						  "AnonymousEnumItem");
	assert(eclass && PyClass_Check(eclass));
	assert(aclass && PyClass_Check(aclass));

	// members
	for (CORBA::ULong i=0; i<cnt; i++) {
	  UNMARSHAL_PYSTRING(encap, t_o);

	  if (PyString_GET_SIZE(t_o) > 0)
	    t_o = PyObject_CallFunction(eclass, "Oi", t_o, i);
	  else
	    t_o = PyObject_CallFunction(aclass, "i", i);

	  assert(t_o && PyInstance_Check(t_o));

	  PyTuple_SET_ITEM(mems, i, t_o);
	}
      }
    }
    break;

  case CORBA::tk_sequence:
    {
      //      cout << "sequence" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

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
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      OffsetDescriptorMap eodm(odm, tc_offset + 8);

      d_o = PyTuple_New(4); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      // repoId and name
      UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 1, t_o);
      UNMARSHAL_PYSTRING(encap, t_o); PyTuple_SET_ITEM(d_o, 2, t_o);

      // TypeCode
      t_o = r_unmarshalTypeCode(encap, eodm);
      PyTuple_SET_ITEM(d_o, 3, t_o);
    }
    break;

  case CORBA::tk_except:
    {
      //      cout << "except" << endl;
      CORBA::ULong size; size <<= stream;
      MemBufferedStream encap(size);
      if (stream.overrun(size)) throw CORBA::MARSHAL();
      encap.copy_from(stream, size);
      CORBA::Boolean bo; bo <<= encap; encap.byteOrder(bo);

      PyObject* repoId; UNMARSHAL_PYSTRING(encap, repoId);

      d_o = PyDict_GetItem(omniPy::pyomniORBtypeMap, repoId);

      if (d_o) {
	// Static knowledge of the exception
	Py_INCREF(d_o);
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
				     "createUnknownUserException");
	assert(t_o && PyFunction_Check(t_o));

	t_o = PyObject_CallFunction(t_o, "OO", repoId, mems);
	assert(t_o && PyClass_Check(t_o));

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
	throw CORBA::BAD_TYPECODE();

      d_o = PyTuple_New(2); odm.add(d_o, tc_offset);
      PyTuple_SET_ITEM(d_o, 0, PyInt_FromLong(tk));

      PyObject* olist = PyList_New(1);
      PyList_SET_ITEM(olist, 0, t_o);
      PyTuple_SET_ITEM(d_o, 1, olist);
    }
    break;

  default:
    throw CORBA::BAD_TYPECODE();
  }

  //  cout << "r_unmarshalTypeCode ended." << endl;
  return d_o;
}




CORBA::ULong
omniPy::alignedSizeTypeCode(CORBA::ULong msgsize, PyObject* d_o)
{
  DescriptorOffsetMap dom;
  return r_alignedSizeTypeCode(msgsize, d_o, dom);
}


void
omniPy::marshalTypeCode(NetBufferedStream& stream, PyObject* d_o)
{
  DescriptorOffsetMap dom;
  r_marshalTypeCode(stream, d_o, dom);
}

void
omniPy::marshalTypeCode(MemBufferedStream& stream, PyObject* d_o)
{
  DescriptorOffsetMap dom;
  r_marshalTypeCode(stream, d_o, dom);
}


PyObject*
omniPy::unmarshalTypeCode(NetBufferedStream& stream)
{
  OffsetDescriptorMap odm;
  return r_unmarshalTypeCode(stream, odm);
}

PyObject*
omniPy::unmarshalTypeCode(MemBufferedStream& stream)
{
  OffsetDescriptorMap odm;
  return r_unmarshalTypeCode(stream, odm);
}
