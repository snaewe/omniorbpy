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
// Revision 1.1.2.1  2000/10/13 13:55:25  dpg1
// Initial support for omniORB 4.
//


#include <omnipy.h>

#if defined(__DECCXX)
// EDG based compaq cxx is having a problem with taking the address of static
// functions.
PyObject* omnipyCompaqCxxBug() {
  // Oddly, modules that invoke the following function don't have a problem.
  return omniPy::newTwin(0); // never call this.
}
#endif

// PyLongFromLongLong is broken in Python 1.5.2. Workaround here:
#ifdef HAS_LongLong
#  if !defined(PY_VERSION_HEX) || (PY_VERSION_HEX < 0X01050200)
#    error "omniidl requires Python 1.5.2 or higher"

#  elif (PY_VERSION_HEX < 0x02000000)

// Don't know when it was fixed -- certainly in 2.0.0

static inline PyObject* MyPyLong_FromLongLong(_CORBA_LongLong ll)
{
  if (ll >= 0) // Positive numbers work OK
    return PyLong_FromLongLong(ll);
  else {
    _CORBA_ULongLong ull = (~ll) + 1; // Hope integers are 2's complement...
    PyObject* p = PyLong_FromUnsignedLongLong(ull);
    PyObject* n = PyNumber_Negative(p);
    Py_DECREF(p);
    return n;
  }
}
#  else
#    define MyPyLong_FromLongLong(ll) PyLong_FromLongLong(ll)
#  endif
#endif


// Small function to indicate whether a descriptor represents a type
// for which we have unrolled sequence marshalling code
static inline int
sequenceOptimisedType(PyObject* desc)
{
  if (PyInt_Check(desc)) {
    CORBA::ULong tk = PyInt_AS_LONG(desc);

    return !(tk == CORBA::tk_any ||
	     tk == CORBA::tk_TypeCode ||
	     tk == CORBA::tk_Principal);
  }
  return 0;
}
	

#define VT_THROW_BAD_PARAM OMNIORB_THROW(BAD_PARAM, 0,compstatus)


static void
validateTypeNull(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (a_o != Py_None)
    VT_THROW_BAD_PARAM;
}

static void
validateTypeVoid(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (a_o != Py_None)
    VT_THROW_BAD_PARAM;
}

static void
validateTypeShort(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (!PyInt_Check(a_o)) VT_THROW_BAD_PARAM;
  long l = PyInt_AS_LONG(a_o);
  if (l < -0x8000 || l > 0x7fff) VT_THROW_BAD_PARAM;
}

static void
validateTypeLong(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (!PyInt_Check(a_o)) VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
  long l = PyInt_AS_LONG(a_o);
  if (l < -0x80000000L || l > 0x7fffffffL) VT_THROW_BAD_PARAM;
#endif
}

static void
validateTypeUShort(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{
  if (!PyInt_Check(a_o)) VT_THROW_BAD_PARAM;
  long l = PyInt_AS_LONG(a_o);
  if (l < 0 || l > 0xffff) VT_THROW_BAD_PARAM;
}

static void
validateTypeULong(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (PyLong_Check(a_o)) {
    unsigned long ul = PyLong_AsUnsignedLong(a_o);
    if (ul == (unsigned long)-1 && PyErr_Occurred())
      VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
    if (ul > 0xffffffffL) VT_THROW_BAD_PARAM;
#endif
  }
  else if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
#if SIZEOF_LONG > 4
    if (l < 0 || l > 0xffffffffL) VT_THROW_BAD_PARAM;
#else
    if (l < 0) VT_THROW_BAD_PARAM;
#endif
  }
  else
    VT_THROW_BAD_PARAM;
}

static void
validateTypeFloat(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (!(PyFloat_Check(a_o) || PyInt_Check(a_o))) VT_THROW_BAD_PARAM;
}

static void
validateTypeDouble(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{
  if (!(PyFloat_Check(a_o) || PyInt_Check(a_o))) VT_THROW_BAD_PARAM;
}

static void
validateTypeBoolean(PyObject* d_o, PyObject* a_o,
		    CORBA::CompletionStatus compstatus)
{
  if (!PyInt_Check(a_o)) VT_THROW_BAD_PARAM;
}

static void
validateTypeChar(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (!(PyString_Check(a_o) && (PyString_GET_SIZE(a_o) == 1)))
    VT_THROW_BAD_PARAM;
}

static void
validateTypeOctet(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (!PyInt_Check(a_o)) VT_THROW_BAD_PARAM;
  long l = PyInt_AS_LONG(a_o);
  if (l < 0 || l > 0xff) VT_THROW_BAD_PARAM;
}

static void
validateTypeAny(PyObject* d_o, PyObject* a_o,
		CORBA::CompletionStatus compstatus)
{
  if (!PyInstance_Check(a_o)) VT_THROW_BAD_PARAM;

  PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

  // Validate TypeCode
  PyObject* t_o   = PyDict_GetItemString(adict, (char*)"_t");
  if (!(t_o && PyInstance_Check(t_o))) VT_THROW_BAD_PARAM;

  PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
  PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
  if (!desc) VT_THROW_BAD_PARAM;

  // Any's contents
  t_o             = PyDict_GetItemString(adict, (char*)"_v");
  if (!t_o) VT_THROW_BAD_PARAM;

  omniPy::validateType(desc, t_o, compstatus);
}

static void
validateTypeTypeCode(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{
  if (!PyInstance_Check(a_o)) VT_THROW_BAD_PARAM;

  PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
  PyObject* t_o   = PyDict_GetItemString(tdict, (char*)"_d");
  if (!t_o) VT_THROW_BAD_PARAM;
}

static void
validateTypePrincipal(PyObject* d_o, PyObject* a_o,
		      CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeObjref(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // repoId, name
  if (a_o != Py_None) {
    if (!PyInstance_Check(a_o)) VT_THROW_BAD_PARAM;
    CORBA::Object_ptr obj =
      (CORBA::Object_ptr)omniPy::getTwin(a_o, OBJREF_TWIN);
    if (!obj) VT_THROW_BAD_PARAM;
  }
}

static void
validateTypeStruct(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // class, repoId, struct name, name, descriptor, ...

  // The descriptor tuple has twice the number of struct members,
  // plus 4 -- the typecode kind, the Python class, the repoId,
  // and the struct name
  int cnt = (PyTuple_GET_SIZE(d_o) - 4) / 2;

  PyObject* name;
  PyObject* value;

  int i, j;

  // Optimise for the fast case, where the object is a class
  // instance with all attributes in its own dictionary
  if (PyInstance_Check(a_o)) {

    PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

    for (i=0,j=4; i < cnt; i++) {
      name    = PyTuple_GET_ITEM(d_o, j++);
      OMNIORB_ASSERT(PyString_Check(name));
      value   = PyDict_GetItem(sdict, name);

      if (value) {
	omniPy::validateType(PyTuple_GET_ITEM(d_o, j++), value, compstatus);
      }
      else {
	// Not such a fast case after all
	value = PyObject_GetAttr(a_o, name);
	if (!value) VT_THROW_BAD_PARAM;
	// DECREF now in case validateType() throws an exception. Safe
	// because the struct object still holds a reference to the
	// value.
	Py_DECREF(value);
	omniPy::validateType(PyTuple_GET_ITEM(d_o, j++), value, compstatus);
      }
    }
  }
  else {
    for (i=0,j=4; i < cnt; i++) {
      name    = PyTuple_GET_ITEM(d_o, j++);
      OMNIORB_ASSERT(PyString_Check(name));
      value   = PyObject_GetAttr(a_o, name);
      if (!value) VT_THROW_BAD_PARAM;
      Py_DECREF(value);
      omniPy::validateType(PyTuple_GET_ITEM(d_o, j++), value, compstatus);
    }
  }
}

static void
validateTypeUnion(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{ // class,
  // repoId,
  // name,
  // discriminant descr,
  // default used,
  // ((label value, member name, member descr), ...),
  // default (label, name, descr) or None,
  // {label: (label, name, descr), ...}

  if (!PyInstance_Check(a_o)) VT_THROW_BAD_PARAM;

  PyObject* udict        = ((PyInstanceObject*)a_o)->in_dict;
  PyObject* discriminant = PyDict_GetItemString(udict, (char*)"_d");
  PyObject* value        = PyDict_GetItemString(udict, (char*)"_v");
  if (!(discriminant && value)) VT_THROW_BAD_PARAM;

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 4); // Discriminant descriptor
  omniPy::validateType(t_o, discriminant, compstatus);

  PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);
  OMNIORB_ASSERT(PyDict_Check(cdict));

  t_o = PyDict_GetItem(cdict, discriminant);
  if (t_o) {
    // Discriminant found in case dictionary
    OMNIORB_ASSERT(PyTuple_Check(t_o));
    omniPy::validateType(PyTuple_GET_ITEM(t_o, 2), value, compstatus);
  }
  else {
    // Is there a default case?
    t_o = PyTuple_GET_ITEM(d_o, 7);
    if (t_o != Py_None) {
      OMNIORB_ASSERT(PyTuple_Check(t_o));
      omniPy::validateType(PyTuple_GET_ITEM(t_o, 2), value, compstatus);
    }

  }
}

static void
validateTypeEnum(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{ // repoId, name, item list

  if (!PyInstance_Check(a_o)) VT_THROW_BAD_PARAM;
  PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
				      (char*)"_v");
  if (!(ev && PyInt_Check(ev))) VT_THROW_BAD_PARAM;

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 3);
  long      e   = PyInt_AS_LONG(ev);

  if (e >= PyTuple_GET_SIZE(t_o))      VT_THROW_BAD_PARAM;
  if (PyTuple_GET_ITEM(t_o, e) != a_o) VT_THROW_BAD_PARAM;
}

static void
validateTypeString(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // max_length

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 1);
  OMNIORB_ASSERT(PyInt_Check(t_o));

  CORBA::ULong max_len = PyInt_AS_LONG(t_o);

  if (!PyString_Check(a_o)) VT_THROW_BAD_PARAM;

  CORBA::ULong len = PyString_GET_SIZE(a_o);

  if (max_len > 0 && len > max_len) VT_THROW_BAD_PARAM;

  // Annoyingly, we have to scan the string to check there are no
  // nulls
  char* str = PyString_AS_STRING(a_o);
  for (CORBA::ULong i=0; i<len; i++)
    if (str[i] == '\0') VT_THROW_BAD_PARAM;
}


static void
validateTypeSequence(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{ // element_desc, max_length

  PyObject*     t_o      = PyTuple_GET_ITEM(d_o, 2);
  OMNIORB_ASSERT(PyInt_Check(t_o));
  CORBA::ULong  max_len  = PyInt_AS_LONG(t_o);
  PyObject*     elm_desc = PyTuple_GET_ITEM(d_o, 1);

  CORBA::ULong  len, i;
  long          long_val;
  unsigned long ulong_val;

  if (sequenceOptimisedType(elm_desc)) {
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
      // Mapping says octet and char use a string
      if (!PyString_Check(a_o)) VT_THROW_BAD_PARAM;
      len = PyString_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) VT_THROW_BAD_PARAM;
      return;
    }
    else if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) VT_THROW_BAD_PARAM;

      switch (etk) {
      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x8000 || long_val > 0x7fff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x80000000L || long_val > 0x7fffffffL)
	    VT_THROW_BAD_PARAM;
#endif
	}
	return;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < 0 || long_val > 0xffff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) VT_THROW_BAD_PARAM;
#endif
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
	    if (long_val < 0 || long_val > 0xffffffffL) VT_THROW_BAD_PARAM;
#else
	    if (long_val < 0) VT_THROW_BAD_PARAM;
#endif
	  }
	  else
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_float:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++)
	  if (!PyInt_Check(PyList_GET_ITEM(a_o, i))) VT_THROW_BAD_PARAM;
	return;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::LongLong ll = PyLong_AsLongLong(t_o);
	    if (ll == -1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (!PyInt_Check(t_o)) {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::ULongLong ull = PyLong_AsUnsignedLongLong(t_o);
	    if (ull == (CORBA::ULongLong)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (PyInt_Check(t_o)) {
	    long l = PyInt_AS_LONG(t_o);
	    if (l < 0)
	      VT_THROW_BAD_PARAM;
	  }
	  else {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;
#else
      case 23:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);

      case 24:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) VT_THROW_BAD_PARAM;

      switch (etk) {
      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x8000 || long_val > 0x7fff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x80000000L || long_val > 0x7fffffffL)
	    VT_THROW_BAD_PARAM;
#endif
	}
	return;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < 0 || long_val > 0xffff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) VT_THROW_BAD_PARAM;
#endif
	  }
	  else if (!PyInt_Check(a_o))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_float:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++)
	  if (!PyInt_Check(PyTuple_GET_ITEM(a_o, i))) VT_THROW_BAD_PARAM;
	return;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::LongLong ll = PyLong_AsLongLong(t_o);
	    if (ll == -1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (!PyInt_Check(t_o)) {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::ULongLong ull = PyLong_AsUnsignedLongLong(t_o);
	    if (ull == (CORBA::ULongLong)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (PyInt_Check(t_o)) {
	    long l = PyInt_AS_LONG(t_o);
	    if (l < 0)
	      VT_THROW_BAD_PARAM;
	  }
	  else {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;
#else
      case 23:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);

      case 24:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
    else
      VT_THROW_BAD_PARAM;
  }
  else { // Complex type
    if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) VT_THROW_BAD_PARAM;
	  
      for (i=0; i < len; i++) {
	omniPy::validateType(elm_desc, PyList_GET_ITEM(a_o, i), compstatus);
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) VT_THROW_BAD_PARAM;

      for (i=0; i < len; i++) {
	omniPy::validateType(elm_desc, PyTuple_GET_ITEM(a_o, i), compstatus);
      }
    }
    else
      VT_THROW_BAD_PARAM;
  }
}

static void
validateTypeArray(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{ // element_desc, length

  PyObject*    t_o      = PyTuple_GET_ITEM(d_o, 2);
  OMNIORB_ASSERT(PyInt_Check(t_o));
  CORBA::ULong arr_len  = PyInt_AS_LONG(t_o);
  PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

  CORBA::ULong  len, i;
  long          long_val;
  unsigned long ulong_val;

  if (sequenceOptimisedType(elm_desc)) {
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
      // Mapping says octet and char use a string
      if (!PyString_Check(a_o)) VT_THROW_BAD_PARAM;
      len = PyString_GET_SIZE(a_o);
      if (len != arr_len) VT_THROW_BAD_PARAM;
    }
    else if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (len != arr_len) VT_THROW_BAD_PARAM;

      switch (etk) {
      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x8000 || long_val > 0x7fff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x80000000L || long_val > 0x7fffffffL)
	    VT_THROW_BAD_PARAM;
#endif
	}
	return;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < 0 || long_val > 0xffff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) VT_THROW_BAD_PARAM;
#endif
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
	    if (long_val < 0 || long_val > 0xffffffffL) VT_THROW_BAD_PARAM;
#else
	    if (long_val < 0) VT_THROW_BAD_PARAM;
#endif
	  }
	  else
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_float:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++)
	  if (!PyInt_Check(PyList_GET_ITEM(a_o, i))) VT_THROW_BAD_PARAM;
	return;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::LongLong ll = PyLong_AsLongLong(t_o);
	    if (ll == -1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (!PyInt_Check(t_o)) {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::ULongLong ull = PyLong_AsUnsignedLongLong(t_o);
	    if (ull == (CORBA::ULongLong)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (PyInt_Check(t_o)) {
	    long l = PyInt_AS_LONG(t_o);
	    if (l < 0)
	      VT_THROW_BAD_PARAM;
	  }
	  else {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;
#else
      case 23:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);

      case 24:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (len != arr_len) VT_THROW_BAD_PARAM;

      switch (etk) {

      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x8000 || long_val > 0x7fff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < -0x80000000L || long_val > 0x7fffffffL)
	    VT_THROW_BAD_PARAM;
#endif
	}
	return;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!PyInt_Check(t_o)) VT_THROW_BAD_PARAM;
	  long_val = PyInt_AS_LONG(t_o);
	  if (long_val < 0 || long_val > 0xffff) VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) VT_THROW_BAD_PARAM;
#endif
	  }
	  else if (!PyInt_Check(a_o))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_float:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
	    VT_THROW_BAD_PARAM;
	}
	return;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++)
	  if (!PyInt_Check(PyTuple_GET_ITEM(a_o, i))) VT_THROW_BAD_PARAM;
	return;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::LongLong ll = PyLong_AsLongLong(t_o);
	    if (ll == -1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (!PyInt_Check(t_o)) {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    CORBA::ULongLong ull = PyLong_AsUnsignedLongLong(t_o);
	    if (ull == (CORBA::ULongLong)-1 && PyErr_Occurred())
	      VT_THROW_BAD_PARAM;
	  }
	  else if (PyInt_Check(t_o)) {
	    long l = PyInt_AS_LONG(t_o);
	    if (l < 0)
	      VT_THROW_BAD_PARAM;
	  }
	  else {
	    VT_THROW_BAD_PARAM;
	  }
	}
	return;
#else
      case 23:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);

      case 24:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
    else
      VT_THROW_BAD_PARAM;
  }
  else { // Complex type
    if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (len != arr_len) VT_THROW_BAD_PARAM;

      for (i=0; i < len; i++) {
	omniPy::validateType(elm_desc, PyList_GET_ITEM(a_o, i), compstatus);
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (len != arr_len) VT_THROW_BAD_PARAM;

      for (i=0; i < len; i++) {
	omniPy::validateType(elm_desc, PyTuple_GET_ITEM(a_o, i), compstatus);
      }
    }
  }
}

static void
validateTypeAlias(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{ // repoId, name, descr

  omniPy::validateType(PyTuple_GET_ITEM(d_o, 3), a_o, compstatus);
}

static void
validateTypeExcept(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // class, repoId, exc name, name, descriptor, ...

  if (!PyInstance_Check(a_o)) VT_THROW_BAD_PARAM;

  PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

  // As with structs, the descriptor tuple has twice the number of
  // struct members plus 4.
  int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

  PyObject* name;
  PyObject* value;

  int i, j;
  for (i=0,j=4; i < cnt; i++) {
    name    = PyTuple_GET_ITEM(d_o, j++);
    OMNIORB_ASSERT(PyString_Check(name));
    value   = PyDict_GetItem(sdict, name);
    if (!value) VT_THROW_BAD_PARAM;
    omniPy::validateType(PyTuple_GET_ITEM(d_o, j++), value, compstatus);
  }
}

static void
validateTypeLongLong(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{
#ifdef HAS_LongLong
  if (PyLong_Check(a_o)) {
    CORBA::LongLong ll = PyLong_AsLongLong(a_o);
    if (ll == -1 && PyErr_Occurred())
      VT_THROW_BAD_PARAM;
  }
  else if (!PyInt_Check(a_o)) {
    VT_THROW_BAD_PARAM;
  }
#else
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
#endif
}

static void
validateTypeULongLong(PyObject* d_o, PyObject* a_o,
		      CORBA::CompletionStatus compstatus)
{
#ifdef HAS_LongLong
  if (PyLong_Check(a_o)) {
    CORBA::ULongLong ull = PyLong_AsUnsignedLongLong(a_o);
    if (ull == (CORBA::ULongLong)-1 && PyErr_Occurred())
      VT_THROW_BAD_PARAM;
  }
  else if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    if (l < 0) VT_THROW_BAD_PARAM;
  }
  else {
    VT_THROW_BAD_PARAM;
  }
#else
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
#endif
}

static void
validateTypeLongDouble(PyObject* d_o, PyObject* a_o,
		       CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeWChar(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeWString(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeFixed(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeValue(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeValueBox(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}

static void
validateTypeNative(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(BAD_TYPECODE, 0, compstatus);
}

static void
validateTypeAbstractInterface(PyObject* d_o, PyObject* a_o,
			      CORBA::CompletionStatus compstatus)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, compstatus);
}


const omniPy::ValidateTypeFn omniPy::validateTypeFns[] = {
  validateTypeNull,
  validateTypeVoid,
  validateTypeShort,
  validateTypeLong,
  validateTypeUShort,
  validateTypeULong,
  validateTypeFloat,
  validateTypeDouble,
  validateTypeBoolean,
  validateTypeChar,
  validateTypeOctet,
  validateTypeAny,
  validateTypeTypeCode,
  validateTypePrincipal,
  validateTypeObjref,
  validateTypeStruct,
  validateTypeUnion,
  validateTypeEnum,
  validateTypeString,
  validateTypeSequence,
  validateTypeArray,
  validateTypeAlias,
  validateTypeExcept,
  validateTypeLongLong,
  validateTypeULongLong,
  validateTypeLongDouble,
  validateTypeWChar,
  validateTypeWString,
  validateTypeFixed,
  validateTypeValue,
  validateTypeValueBox,
  validateTypeNative,
  validateTypeAbstractInterface
};


static void
marshalPyObjectNull(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
}

static void
marshalPyObjectVoid(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
}

static void
marshalPyObjectShort(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::Short s = PyInt_AS_LONG(a_o);
  s >>= stream;
}

static void
marshalPyObjectLong(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::Long l = PyInt_AS_LONG(a_o);
  l >>= stream;
}

static void
marshalPyObjectUShort(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::UShort us = PyInt_AS_LONG(a_o);
  us >>= stream;
}

static void
marshalPyObjectULong(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::ULong ul = 0; // Initialised to stop egcs complaining

  if (PyLong_Check(a_o))
    ul = PyLong_AsUnsignedLong(a_o);
  else // It's an int
    ul = PyInt_AS_LONG(a_o);

  ul >>= stream;
}

static void
marshalPyObjectFloat(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::Float f = 0; // Initialised to stop egcs complaining

  if (PyFloat_Check(a_o))
    f = (CORBA::Float)PyFloat_AS_DOUBLE(a_o);
  else { // It's an int
    CORBA::Long i = PyInt_AS_LONG(a_o);
    f = i;
  }
  f >>= stream;
}

static void
marshalPyObjectDouble(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::Double d = 0; // Initialised to stop egcs complaining

  if (PyFloat_Check(a_o))
    d = (CORBA::Double)PyFloat_AS_DOUBLE(a_o);
  else { // It's an int
    CORBA::Long i = PyInt_AS_LONG(a_o);
    d = i;
  }
  d >>= stream;
}

static void
marshalPyObjectBoolean(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
  b >>= stream;
}

static void
marshalPyObjectChar(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  char *str = PyString_AS_STRING(a_o);
  CORBA::Char c = str[0];
  c >>= stream;
}

static void
marshalPyObjectOctet(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  CORBA::Octet o = PyInt_AS_LONG(a_o);
  o >>= stream;
}

static void
marshalPyObjectAny(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

  // TypeCode
  PyObject* t_o   = PyDict_GetItemString(adict, (char*)"_t");
  PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
  PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
  omniPy::marshalTypeCode(stream, desc);

  // Any's contents
  t_o             = PyDict_GetItemString(adict, (char*)"_v");
  omniPy::marshalPyObject(stream, desc, t_o);
}

static void
marshalPyObjectTypeCode(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
  PyObject* t_o   = PyDict_GetItemString(tdict, (char*)"_d");
  omniPy::marshalTypeCode(stream, t_o);
}

static void
marshalPyObjectPrincipal(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectObjref(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // repoId, name

  CORBA::Object_ptr obj;

  if (a_o == Py_None)
    obj = CORBA::Object::_nil();
  else
    obj = (CORBA::Object_ptr)omniPy::getTwin(a_o, OBJREF_TWIN);

  CORBA::Object::_marshalObjRef(obj, stream);
}

static void
marshalPyObjectStruct(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // class, repoId, struct name, {name, descriptor}

  int i, j;
  int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;
  PyObject* name;
  PyObject* value;

  if (PyInstance_Check(a_o)) {
    PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;
    for (i=0,j=4; i < cnt; i++) {
      name  = PyTuple_GET_ITEM(d_o, j++);
      value = PyDict_GetItem(sdict, name);

      if (value) {
	omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
      }
      else {
	value = PyObject_GetAttr(a_o, name);
	omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	Py_DECREF(value);
      }
    }
  }
  else {
    for (i=0,j=4; i < cnt; i++) {
      name  = PyTuple_GET_ITEM(d_o, j++);
      value = PyObject_GetAttr(a_o, name);
      omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
      Py_DECREF(value);
    }
  }
}

static void
marshalPyObjectUnion(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // class,
  // repoId,
  // name,
  // discriminant descr,
  // default used,
  // ((label value, member name, member descr), ...),
  // default (label, name, descr) or None,
  // {label: (label, name, descr), ...}

  PyObject* udict = ((PyInstanceObject*)a_o)->in_dict;

  PyObject* discriminant = PyDict_GetItemString(udict, (char*)"_d");
  PyObject* value        = PyDict_GetItemString(udict, (char*)"_v");
  PyObject* t_o          = PyTuple_GET_ITEM(d_o, 4); // Discriminant descriptor
  PyObject* cdict        = PyTuple_GET_ITEM(d_o, 8);

  omniPy::marshalPyObject(stream, t_o, discriminant);

  t_o = PyDict_GetItem(cdict, discriminant);
  if (t_o) {
    // Discriminant found in case dictionary
    omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2), value);
  }
  else {
    // Is there a default case?
    t_o = PyTuple_GET_ITEM(d_o, 7);
    if (t_o != Py_None) {
      omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2), value);
    }
  }
}

static void
marshalPyObjectEnum(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // repoId, name, item list

  PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
				      (char*)"_v");
  CORBA::ULong e = PyInt_AS_LONG(ev);
  e >>= stream;
}

static void
marshalPyObjectString(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // max_length

  CORBA::ULong slen = PyString_GET_SIZE(a_o) + 1;

  slen >>= stream;

  if (slen > 1) {
    char* str = PyString_AS_STRING(a_o);
    stream.put_char_array((const CORBA::Char*)((const char*)str), slen);
  }
  else {
    CORBA::Char('\0') >>= stream;
  }
}

static void
marshalPyObjectSequence(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // element_desc, max_length

  PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);
  PyObject* t_o;

  CORBA::ULong i, len;

  if (sequenceOptimisedType(elm_desc)) {
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet) {
      len = PyString_GET_SIZE(a_o);
      len >>= stream;
      CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
      stream.put_char_array((const CORBA::Char*)l, len);
    }
    else if (etk == CORBA::tk_char) {
      len = PyString_GET_SIZE(a_o);
      len >>= stream;
      CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
      stream.put_char_array((const CORBA::Char*)l, len);
    }
    else if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      len >>= stream;

      switch (etk) {
      case CORBA::tk_short:
	{
	  CORBA::Short e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_long:
	{
	  CORBA::Long e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ushort:
	{
	  CORBA::UShort e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulong:
	{
	  CORBA::ULong e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsUnsignedLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_float:
	{
	  CORBA::Float e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_double:
	{
	  CORBA::Double e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_boolean:
	{
	  CORBA::Boolean e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	{
	  CORBA::LongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	{
	  CORBA::ULongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
    else {
      OMNIORB_ASSERT(PyTuple_Check(a_o));
      len = PyTuple_GET_SIZE(a_o);
      len >>= stream;

      switch (etk) {
      case CORBA::tk_short:
	{
	  CORBA::Short e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_long:
	{
	  CORBA::Long e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ushort:
	{
	  CORBA::UShort e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulong:
	{
	  CORBA::ULong e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsUnsignedLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_float:
	{
	  CORBA::Float e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_double:
	{
	  CORBA::Double e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_boolean:
	{
	  CORBA::Boolean e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	{
	  CORBA::LongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	{
	  CORBA::ULongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
  }
  else if (PyList_Check(a_o)) {
    len = PyList_GET_SIZE(a_o);
    len >>= stream;
    for (i=0; i < len; i++)
      omniPy::marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
  }
  else {
    len = PyTuple_GET_SIZE(a_o);
    len >>= stream;
    for (i=0; i < len; i++)
      omniPy::marshalPyObject(stream, elm_desc, PyTuple_GET_ITEM(a_o, i));
  }
}


static void
marshalPyObjectArray(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // element_desc, length

  PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);
  PyObject* t_o;

  CORBA::ULong i, len;

  if (sequenceOptimisedType(elm_desc)) {
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet) {
      len = PyString_GET_SIZE(a_o);
      CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
      stream.put_char_array((const CORBA::Char*)l, len);
    }
    else if (etk == CORBA::tk_char) {
      len = PyString_GET_SIZE(a_o);
      CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
      stream.put_char_array((const CORBA::Char*)l, len);
    }
    else if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);

      switch (etk) {
      case CORBA::tk_short:
	{
	  CORBA::Short e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_long:
	{
	  CORBA::Long e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ushort:
	{
	  CORBA::UShort e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulong:
	{
	  CORBA::ULong e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsUnsignedLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_float:
	{
	  CORBA::Float e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_double:
	{
	  CORBA::Double e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_boolean:
	{
	  CORBA::Boolean e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	{
	  CORBA::LongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	{
	  CORBA::ULongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyList_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
    else {
      OMNIORB_ASSERT(PyTuple_Check(a_o));
      len = PyTuple_GET_SIZE(a_o);

      switch (etk) {
      case CORBA::tk_short:
	{
	  CORBA::Short e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_long:
	{
	  CORBA::Long e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ushort:
	{
	  CORBA::UShort e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulong:
	{
	  CORBA::ULong e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsUnsignedLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_float:
	{
	  CORBA::Float e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_double:
	{
	  CORBA::Double e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyFloat_Check(t_o))
	      e = PyFloat_AS_DOUBLE(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_boolean:
	{
	  CORBA::Boolean e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	{
	  CORBA::LongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	{
	  CORBA::ULongLong e;
	  for (i=0; i < len; i++) {
	    t_o = PyTuple_GET_ITEM(a_o, i);
	    if (PyLong_Check(t_o))
	      e = PyLong_AsLongLong(t_o);
	    else
	      e = PyInt_AS_LONG(t_o);
	    e >>= stream;
	  }
	}
	break;
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
  }
  else if (PyList_Check(a_o)) {
    len = PyList_GET_SIZE(a_o);
    for (i=0; i < len; i++)
      omniPy::marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
  }
  else {
    len = PyTuple_GET_SIZE(a_o);
    for (i=0; i < len; i++)
      omniPy::marshalPyObject(stream, elm_desc, PyTuple_GET_ITEM(a_o, i));
  }
}

static void
marshalPyObjectAlias(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // repoId, name, descr

  omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(d_o, 3), a_o);
}

static void
marshalPyObjectExcept(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{ // class, repoId, exc name, name, descriptor, ...

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
    name  = PyTuple_GET_ITEM(d_o, j++);
    value = PyDict_GetItem(sdict, name);
    omniPy::marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
  }
}

static void
marshalPyObjectLongLong(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
#ifdef HAS_LongLong
  CORBA::LongLong ll = 0; // Initialised to stop egcs complaining

  if (PyLong_Check(a_o))
    ll = PyLong_AsLongLong(a_o);
  else // It's an int
    ll = PyInt_AS_LONG(a_o);

  ll >>= stream;
#else
  OMNIORB_ASSERT(0);
#endif
}

static void
marshalPyObjectULongLong(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
#ifdef HAS_LongLong
  CORBA::ULongLong ull = 0; // Initialised to stop egcs complaining

  if (PyLong_Check(a_o))
    ull = PyLong_AsUnsignedLongLong(a_o);
  else // It's an int
    ull = PyInt_AS_LONG(a_o);

  ull >>= stream;
#else
  OMNIORB_ASSERT(0);
#endif
}

static void
marshalPyObjectLongDouble(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectWChar(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectWString(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectFixed(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectValue(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectValueBox(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectNative(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}

static void
marshalPyObjectAbstractInterface(cdrStream& stream, PyObject* d_o, PyObject* a_o)
{
  OMNIORB_ASSERT(0);
}


const omniPy::MarshalPyObjectFn omniPy::marshalPyObjectFns[] = {
  marshalPyObjectNull,
  marshalPyObjectVoid,
  marshalPyObjectShort,
  marshalPyObjectLong,
  marshalPyObjectUShort,
  marshalPyObjectULong,
  marshalPyObjectFloat,
  marshalPyObjectDouble,
  marshalPyObjectBoolean,
  marshalPyObjectChar,
  marshalPyObjectOctet,
  marshalPyObjectAny,
  marshalPyObjectTypeCode,
  marshalPyObjectPrincipal,
  marshalPyObjectObjref,
  marshalPyObjectStruct,
  marshalPyObjectUnion,
  marshalPyObjectEnum,
  marshalPyObjectString,
  marshalPyObjectSequence,
  marshalPyObjectArray,
  marshalPyObjectAlias,
  marshalPyObjectExcept,
  marshalPyObjectLongLong,
  marshalPyObjectULongLong,
  marshalPyObjectLongDouble,
  marshalPyObjectWChar,
  marshalPyObjectWString,
  marshalPyObjectFixed,
  marshalPyObjectValue,
  marshalPyObjectValueBox,
  marshalPyObjectNative,
  marshalPyObjectAbstractInterface
};



static PyObject*
unmarshalPyObjectNull(cdrStream& stream, PyObject* d_o)
{
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
unmarshalPyObjectVoid(cdrStream& stream, PyObject* d_o)
{
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
unmarshalPyObjectShort(cdrStream& stream, PyObject* d_o)
{
  CORBA::Short s;
  s <<= stream;
  return PyInt_FromLong(s);
}

static PyObject*
unmarshalPyObjectLong(cdrStream& stream, PyObject* d_o)
{
  CORBA::Long l;
  l <<= stream;
  return PyInt_FromLong(l);
}

static PyObject*
unmarshalPyObjectUShort(cdrStream& stream, PyObject* d_o)
{
  CORBA::UShort us;
  us <<= stream;
  return PyInt_FromLong(us);
}

static PyObject*
unmarshalPyObjectULong(cdrStream& stream, PyObject* d_o)
{
  CORBA::ULong ul;
  ul <<= stream;
  return PyLong_FromUnsignedLong(ul);
}

static PyObject*
unmarshalPyObjectFloat(cdrStream& stream, PyObject* d_o)
{
  CORBA::Float f;
  f <<= stream;
  return PyFloat_FromDouble((double)f);
}

static PyObject*
unmarshalPyObjectDouble(cdrStream& stream, PyObject* d_o)
{
  CORBA::Double d;
  d <<= stream;
  return PyFloat_FromDouble(d);
}

static PyObject*
unmarshalPyObjectBoolean(cdrStream& stream, PyObject* d_o)
{
  CORBA::Boolean b;
  b <<= stream;
  return PyInt_FromLong(b);
}

static PyObject*
unmarshalPyObjectChar(cdrStream& stream, PyObject* d_o)
{
  CORBA::Char c;
  c <<= stream;

  char* str     = new char[2];
  str[0]        = c;
  str[1]        = '\0';
  PyObject* r_o = PyString_FromStringAndSize(str, 1);
  delete [] str;
  return r_o;
}

static PyObject*
unmarshalPyObjectOctet(cdrStream& stream, PyObject* d_o)
{
  CORBA::Octet o;
  o <<= stream;
  return PyInt_FromLong(o);
}

static PyObject*
unmarshalPyObjectAny(cdrStream& stream, PyObject* d_o)
{
  // TypeCode
  PyObject* desc     = omniPy::unmarshalTypeCode(stream);
  PyObject* argtuple = PyTuple_New(1);
  PyTuple_SET_ITEM(argtuple, 0, desc);
  PyObject* tcobj    = PyEval_CallObject(omniPy::pyCreateTypeCode, argtuple);

  PyObject* value;
  try {
    value = omniPy::unmarshalPyObject(stream, desc);
  }
  catch (...) {
    Py_DECREF(tcobj);
    Py_DECREF(argtuple);
    throw;
  }

  Py_DECREF(argtuple);

  argtuple = PyTuple_New(2);
  PyTuple_SET_ITEM(argtuple, 0, tcobj);
  PyTuple_SET_ITEM(argtuple, 1, value);

  PyObject* r_o = PyEval_CallObject(omniPy::pyCORBAAnyClass, argtuple);
  Py_DECREF(argtuple);
  return r_o;
}

static PyObject*
unmarshalPyObjectTypeCode(cdrStream& stream, PyObject* d_o)
{
  PyObject* t_o      = omniPy::unmarshalTypeCode(stream);
  PyObject* argtuple = PyTuple_New(1);
  PyTuple_SET_ITEM(argtuple, 0, t_o);
  PyObject* r_o      = PyEval_CallObject(omniPy::pyCreateTypeCode, argtuple);
  Py_DECREF(argtuple);
  return r_o;
}

static PyObject*
unmarshalPyObjectPrincipal(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectObjref(cdrStream& stream, PyObject* d_o)
{ // repoId, name

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 1);

  const char* targetRepoId;

  if (t_o == Py_None)
    targetRepoId = 0;
  else {
    OMNIORB_ASSERT(PyString_Check(t_o));
    targetRepoId = PyString_AS_STRING(t_o);
    if (targetRepoId[0] == '\0') { // Empty string => CORBA.Object
      targetRepoId = CORBA::Object::_PD_repoId;
    }
  }
  CORBA::Object_ptr obj = omniPy::UnMarshalObjRef(targetRepoId, stream);
  return omniPy::createPyCorbaObjRef(targetRepoId, obj);
}

static PyObject*
unmarshalPyObjectStruct(cdrStream& stream, PyObject* d_o)
{ // class, repoId, struct name, name, descriptor, ...

  PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
  OMNIORB_ASSERT(PyClass_Check(strclass));

  int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
  PyObject* strtuple = PyTuple_New(cnt);

  int i, j;
  try {
    for (i=0, j=5; i < cnt; i++, j+=2) {
      PyTuple_SET_ITEM(strtuple, i,
		       omniPy::unmarshalPyObject(stream,
						 PyTuple_GET_ITEM(d_o, j)));
    }
  }
  catch (...) {
    Py_DECREF(strtuple);
    throw;
  }
  PyObject* r_o = PyEval_CallObject(strclass, strtuple);
  Py_DECREF(strtuple);
  return r_o;
}

static PyObject*
unmarshalPyObjectUnion(cdrStream& stream, PyObject* d_o)
{ // class,
  // repoId,
  // name,
  // discriminant descr,
  // default used,
  // ((label value, member name, member descr), ...),
  // default (label, name, descr) or None,
  // {label: (label, name, descr), ...}

  PyObject* unclass = PyTuple_GET_ITEM(d_o, 1);
  OMNIORB_ASSERT(PyClass_Check(unclass));

  PyObject* t_o          = PyTuple_GET_ITEM(d_o, 4);
  PyObject* discriminant = omniPy::unmarshalPyObject(stream, t_o);
  PyObject* value;
  PyObject* cdict        = PyTuple_GET_ITEM(d_o, 8);

  t_o = PyDict_GetItem(cdict, discriminant);
  try {
    if (t_o) {
      // Discriminant found in case dictionary
      OMNIORB_ASSERT(PyTuple_Check(t_o));
      value = omniPy::unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
    }
    else {
      // Is there a default case?
      t_o = PyTuple_GET_ITEM(d_o, 7);
      if (t_o != Py_None) {
	OMNIORB_ASSERT(PyTuple_Check(t_o));
	value = omniPy::unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
      }
      else {
	Py_INCREF(Py_None);
	value = Py_None;
      }
    }
  }
  catch (...) {
    Py_DECREF(discriminant);
    throw;
  }
  PyObject* untuple = PyTuple_New(2);
  PyTuple_SET_ITEM(untuple, 0, discriminant);
  PyTuple_SET_ITEM(untuple, 1, value);

  PyObject* r_o = PyEval_CallObject(unclass, untuple);
  Py_DECREF(untuple);
  return r_o;
}

static PyObject*
unmarshalPyObjectEnum(cdrStream& stream, PyObject* d_o)
{ // repoId, name, item list

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 3);

  OMNIORB_ASSERT(PyTuple_Check(t_o));

  CORBA::ULong e;
  e <<= stream;

  if (e >= (CORBA::ULong)PyTuple_GET_SIZE(t_o))
    OMNIORB_THROW(MARSHAL, 0, CORBA::COMPLETED_NO);

  PyObject* ev = PyTuple_GET_ITEM(t_o, e);
  Py_INCREF(ev);
  return ev;
}

static PyObject*
unmarshalPyObjectString(cdrStream& stream, PyObject* d_o)
{ // max_length

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 1);

  OMNIORB_ASSERT(PyInt_Check(t_o));

  CORBA::ULong max_len = PyInt_AS_LONG(t_o);
  CORBA::ULong len;
  len <<= stream;

  if (max_len > 0 && len > max_len)
    OMNIORB_THROW(MARSHAL, 0, CORBA::COMPLETED_NO);

  if (!stream.checkInputOverrun(1,len))
    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

  PyObject*    r_o = PyString_FromStringAndSize(0, len - 1);
  CORBA::Char* c   = (CORBA::Char*)PyString_AS_STRING(r_o);
  stream.get_char_array(c, len);

  if (c[len] != '\0')
    OMNIORB_THROW(MARSHAL, 0, CORBA::COMPLETED_NO);

  return r_o;
}

static PyObject*
unmarshalPyObjectSequence(cdrStream& stream, PyObject* d_o)
{ // element_desc, max_length

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 2);
  PyObject* r_o = 0;

  OMNIORB_ASSERT(PyInt_Check(t_o));

  CORBA::ULong max_len = PyInt_AS_LONG(t_o);
  CORBA::ULong len;
  len <<= stream;

  if (max_len > 0 && len > max_len)
    OMNIORB_THROW(MARSHAL, 0, CORBA::COMPLETED_NO);

  PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

  CORBA::ULong i;

  if (sequenceOptimisedType(elm_desc)) {
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet) {
      if (!stream.checkInputOverrun(1, len))
	OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

      r_o = PyString_FromStringAndSize(0, len);
      CORBA::Char* c = (CORBA::Char*)PyString_AS_STRING(r_o);
      stream.get_char_array(c, len);
      return r_o;
    }
    else if (etk == CORBA::tk_char) {
      if (!stream.checkInputOverrun(1, len))
	OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

      r_o = PyString_FromStringAndSize(0, len);
      CORBA::Char* c = (CORBA::Char*)PyString_AS_STRING(r_o);
      stream.get_char_array(c, len);
      return r_o;
    }
    else {
      switch(etk) {
      case CORBA::tk_short:
	{
	  if (!stream.checkInputOverrun(2, len, omni::ALIGN_2))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Short e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	return r_o;

      case CORBA::tk_long:
	{
	  if (!stream.checkInputOverrun(4, len, omni::ALIGN_4))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Long e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	return r_o;

      case CORBA::tk_ushort:
	{
	  if (!stream.checkInputOverrun(2, len, omni::ALIGN_2))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::UShort e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	return r_o;

      case CORBA::tk_ulong:
	{
	  if (!stream.checkInputOverrun(4, len, omni::ALIGN_4))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::ULong e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLong(e));
	  }
	}
	return r_o;

      case CORBA::tk_float:
	{
	  if (!stream.checkInputOverrun(4, len, omni::ALIGN_4))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Float e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	  }
	}
	return r_o;

      case CORBA::tk_double:
	{
	  if (!stream.checkInputOverrun(8, len, omni::ALIGN_8))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Double e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	  }
	}
	return r_o;

      case CORBA::tk_boolean:
	{
	  if (!stream.checkInputOverrun(1, len, omni::ALIGN_1))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Boolean e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	return r_o;
	    
#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	{
	  if (!stream.checkInputOverrun(8, len, omni::ALIGN_8))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::LongLong e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, MyPyLong_FromLongLong(e));
	  }
	}
	return r_o;

      case CORBA::tk_ulonglong:
	{
	  if (!stream.checkInputOverrun(8, len, omni::ALIGN_8))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::ULongLong e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLongLong(e));
	  }
	}
	return r_o;
#else
      case 23:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
	return 0;

      case 24:
	OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
	return 0;
#endif
      default:
	OMNIORB_ASSERT(0);
	return 0;
      }
    }
  }
  else {
    // Slight abuse of max giop message size, but it gives us a handy
    // measure of what we consider to be a "too long" sequence.
    if (len > GIOP_Basetypes::max_giop_message_size)
      OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

    r_o = PyList_New(len);

    try {
      for (i=0; i < len; i++)
	PyList_SET_ITEM(r_o, i, omniPy::unmarshalPyObject(stream, elm_desc));
    }
    catch (...) {
      Py_DECREF(r_o);
      throw;
    }
  }
  return r_o;
}

static PyObject*
unmarshalPyObjectArray(cdrStream& stream, PyObject* d_o)
{ // element_desc, length

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 2);
  PyObject* r_o = 0;

  OMNIORB_ASSERT(PyInt_Check(t_o));

  CORBA::ULong len      = PyInt_AS_LONG(t_o);
  PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);
  CORBA::ULong i;

  if (sequenceOptimisedType(elm_desc)) {
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet) {
      if (!stream.checkInputOverrun(1, len))
	OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

      r_o = PyString_FromStringAndSize(0, len);
      CORBA::Char* c = (CORBA::Char*)PyString_AS_STRING(r_o);
      stream.get_char_array(c, len);
      return r_o;
    }
    else if (etk == CORBA::tk_char) {
      if (!stream.checkInputOverrun(1, len))
	OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

      r_o = PyString_FromStringAndSize(0, len);
      CORBA::Char* c = (CORBA::Char*)PyString_AS_STRING(r_o);
      stream.get_char_array(c, len);
      return r_o;
    }
    else {
      switch(etk) {
      case CORBA::tk_short:
	{
	  if (!stream.checkInputOverrun(2, len, omni::ALIGN_2))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Short e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	break;

      case CORBA::tk_long:
	{
	  if (!stream.checkInputOverrun(4, len, omni::ALIGN_4))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Long e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	break;

      case CORBA::tk_ushort:
	{
	  if (!stream.checkInputOverrun(2, len, omni::ALIGN_2))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::UShort e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	break;

      case CORBA::tk_ulong:
	{
	  if (!stream.checkInputOverrun(4, len, omni::ALIGN_4))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::ULong e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLong(e));
	  }
	}
	break;

      case CORBA::tk_float:
	{
	  if (!stream.checkInputOverrun(4, len, omni::ALIGN_4))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Float e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	  }
	}
	break;

      case CORBA::tk_double:
	{
	  if (!stream.checkInputOverrun(8, len, omni::ALIGN_8))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Double e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	  }
	}
	break;

      case CORBA::tk_boolean:
	{
	  if (!stream.checkInputOverrun(1, len, omni::ALIGN_1))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::Boolean e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	  }
	}
	break;
	    
#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	{
	  if (!stream.checkInputOverrun(8, len, omni::ALIGN_8))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::LongLong e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, MyPyLong_FromLongLong(e));
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	{
	  if (!stream.checkInputOverrun(8, len, omni::ALIGN_8))
	    OMNIORB_THROW(MARSHAL,0, CORBA::COMPLETED_NO);

	  r_o = PyList_New(len);
	  CORBA::ULongLong e;
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLongLong(e));
	  }
	}
	break;
#else
      case 23:
	{
	  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
	}
	break;

      case 24:
	{
	  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
	}
	break;
#endif
      default:
	OMNIORB_ASSERT(0);
      }
    }
  }
  else {
    r_o = PyList_New(len);

    try {
      for (i=0; i < len; i++)
	PyList_SET_ITEM(r_o, i, omniPy::unmarshalPyObject(stream, elm_desc));
    }
    catch (...) {
      Py_DECREF(r_o);
      throw;
    }
  }
  return r_o;
}


static PyObject*
unmarshalPyObjectAlias(cdrStream& stream, PyObject* d_o)
{ // repoId, name, descr

  return omniPy::unmarshalPyObject(stream, PyTuple_GET_ITEM(d_o, 3));
}

static PyObject*
unmarshalPyObjectExcept(cdrStream& stream, PyObject* d_o)
{ // class, repoId, exc name, name, descriptor, ...

  // Throw away the repoId. By the time we get here, we already
  // know it.
  // *** Should maybe check to see if it's what we're expecting
  { 
    CORBA::String_member str_tmp;
    str_tmp <<= stream;
  }

  PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
  OMNIORB_ASSERT(PyClass_Check(strclass));

  int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
  PyObject* strtuple = PyTuple_New(cnt);

  int i, j;
  try {
    for (i=0, j=5; i < cnt; i++, j+=2) {
      PyTuple_SET_ITEM(strtuple, i,
		       omniPy::unmarshalPyObject(stream,
						 PyTuple_GET_ITEM(d_o, j)));
    }
  }
  catch (...) {
    Py_DECREF(strtuple);
    throw;
  }
  PyObject* r_o = PyEval_CallObject(strclass, strtuple);
  Py_DECREF(strtuple);
  return r_o;
}

static PyObject*
unmarshalPyObjectLongLong(cdrStream& stream, PyObject* d_o)
{
#ifdef HAS_LongLong
  CORBA::LongLong ll;
  ll <<= stream;
  return MyPyLong_FromLongLong(ll);
#else
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
#endif
}

static PyObject*
unmarshalPyObjectULongLong(cdrStream& stream, PyObject* d_o)
{
#ifdef HAS_LongLong
  CORBA::ULongLong ull;
  ull <<= stream;
  return PyLong_FromUnsignedLongLong(ull);
#else
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
#endif
}

static PyObject*
unmarshalPyObjectLongDouble(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectWChar(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectWString(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectFixed(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectValue(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectValueBox(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectNative(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(BAD_TYPECODE, 0, CORBA::COMPLETED_NO);
  return 0;
}

static PyObject*
unmarshalPyObjectAbstractInterface(cdrStream& stream, PyObject* d_o)
{
  OMNIORB_THROW(NO_IMPLEMENT, 0, CORBA::COMPLETED_NO);
  return 0;
}


const omniPy::UnmarshalPyObjectFn omniPy::unmarshalPyObjectFns[] = {
  unmarshalPyObjectNull,
  unmarshalPyObjectVoid,
  unmarshalPyObjectShort,
  unmarshalPyObjectLong,
  unmarshalPyObjectUShort,
  unmarshalPyObjectULong,
  unmarshalPyObjectFloat,
  unmarshalPyObjectDouble,
  unmarshalPyObjectBoolean,
  unmarshalPyObjectChar,
  unmarshalPyObjectOctet,
  unmarshalPyObjectAny,
  unmarshalPyObjectTypeCode,
  unmarshalPyObjectPrincipal,
  unmarshalPyObjectObjref,
  unmarshalPyObjectStruct,
  unmarshalPyObjectUnion,
  unmarshalPyObjectEnum,
  unmarshalPyObjectString,
  unmarshalPyObjectSequence,
  unmarshalPyObjectArray,
  unmarshalPyObjectAlias,
  unmarshalPyObjectExcept,
  unmarshalPyObjectLongLong,
  unmarshalPyObjectULongLong,
  unmarshalPyObjectLongDouble,
  unmarshalPyObjectWChar,
  unmarshalPyObjectWString,
  unmarshalPyObjectFixed,
  unmarshalPyObjectValue,
  unmarshalPyObjectValueBox,
  unmarshalPyObjectNative,
  unmarshalPyObjectAbstractInterface
};



static inline
PyObject*
setPyBadParam(CORBA::CompletionStatus compstatus)
{
  CORBA::BAD_PARAM ex(0,compstatus);
  omniPy::handleSystemException(ex);
  return 0;
}

static inline
PyObject*
setPyNoImplement(CORBA::CompletionStatus compstatus)
{
  CORBA::NO_IMPLEMENT ex(0,compstatus);
  omniPy::handleSystemException(ex);
  return 0;
}


static PyObject*
copyArgumentNull(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (a_o != Py_None) return setPyBadParam(compstatus);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
copyArgumentVoid(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (a_o != Py_None) return setPyBadParam(compstatus);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
copyArgumentShort(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    if (l < -0x8000 || l > 0x7fff) return setPyBadParam(compstatus);
    Py_INCREF(a_o); return a_o;
  }
  else return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentLong(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if (PyInt_Check(a_o)) {
#if SIZEOF_LONG > 4
    long l = PyInt_AS_LONG(a_o);
    if (l < -0x80000000L || l > 0x7fffffffL)
      return setPyBadParam(compstatus);
#endif
    Py_INCREF(a_o); return a_o;
  }
  else return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentUShort(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{
  if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    if (l < 0 || l > 0xffff) return setPyBadParam(compstatus);
    Py_INCREF(a_o); return a_o;
  }
  else return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentULong(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (PyLong_Check(a_o)) {
    unsigned long ul = PyLong_AsUnsignedLong(a_o);
    if (ul == (unsigned long)-1 && PyErr_Occurred())
      return setPyBadParam(compstatus);
#if SIZEOF_LONG > 4
    if (ul > 0xffffffffL) return setPyBadParam(compstatus);
#endif
    Py_INCREF(a_o); return a_o;
  }
  else if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
#if SIZEOF_LONG > 4
    if (l < 0 || l > 0xffffffffL) return setPyBadParam(compstatus);
#else
    if (l < 0) return setPyBadParam(compstatus);
#endif
    return PyLong_FromLong(l);
  }
  else
    return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentFloat(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (PyFloat_Check(a_o)) {
    Py_INCREF(a_o); return a_o;
  }
  else if (PyInt_Check(a_o)) {
    return PyFloat_FromDouble((double)(PyInt_AS_LONG(a_o)));
  }
  else
    return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentDouble(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{
  if (PyFloat_Check(a_o)) {
    Py_INCREF(a_o); return a_o;
  }
  else if (PyInt_Check(a_o)) {
    return PyFloat_FromDouble((double)(PyInt_AS_LONG(a_o)));
  }
  else
    return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentBoolean(PyObject* d_o, PyObject* a_o,
		    CORBA::CompletionStatus compstatus)
{
  if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    if (l == 0 || l == 1) {
      Py_INCREF(a_o); return a_o;
    }
    else {
      return PyInt_FromLong(1); // Normalise true value to 1
    }
  }
  else
    return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentChar(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{
  if ((PyString_Check(a_o) && (PyString_GET_SIZE(a_o) == 1))) {
    Py_INCREF(a_o); return a_o;
  }
  else
    return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentOctet(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    if (l < 0 || l > 0xff) return setPyBadParam(compstatus);
    Py_INCREF(a_o); return a_o;
  }
  else
    return setPyBadParam(compstatus);
}

static PyObject*
copyArgumentAny(PyObject* d_o, PyObject* a_o,
		CORBA::CompletionStatus compstatus)
{
  if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

  PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

  // TypeCode
  PyObject* tc = PyDict_GetItemString(adict, (char*)"_t");

  if (!(tc && PyInstance_Check(tc))) return setPyBadParam(compstatus);

  PyObject* tdict = ((PyInstanceObject*)tc)->in_dict;
  PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
  if (!desc) return setPyBadParam(compstatus);

  // Any's contents
  PyObject* val = PyDict_GetItemString(adict, (char*)"_v");
  if (!val) return setPyBadParam(compstatus);

  // Copy contents
  PyObject* cval = omniPy::copyArgument(desc, val, compstatus);

  if (!cval) return setPyBadParam(compstatus);

  // Construct new Any
  PyObject* argtuple = PyTuple_New(2);
  Py_INCREF(tc);
  PyTuple_SET_ITEM(argtuple, 0, tc);
  PyTuple_SET_ITEM(argtuple, 1, cval);
  PyObject* r_o = PyEval_CallObject(omniPy::pyCORBAAnyClass, argtuple);
  Py_DECREF(argtuple);

  return r_o;
}

static PyObject*
copyArgumentTypeCode(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{
  if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

  PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
  PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
  if (!desc) return setPyBadParam(compstatus);

  Py_INCREF(a_o); return a_o;
}

static PyObject*
copyArgumentPrincipal(PyObject* d_o, PyObject* a_o,
		      CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentObjref(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // repoId, name

  return omniPy::copyObjRefArgument(PyTuple_GET_ITEM(d_o, 1),
				    a_o, compstatus);
}

static PyObject*
copyArgumentStruct(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // class, repoId, struct name, name, descriptor, ...

  // The descriptor tuple has twice the number of struct members,
  // plus 4 -- the typecode kind, the Python class, the repoId,
  // and the struct name
  int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

  PyObject* t_o;
  PyObject* name;
  PyObject* value;
  PyObject* argtuple = PyTuple_New(cnt);

  int i, j;

  // Optimise for the fast case, where the object is a class
  // instance with all attributes in its own dictionary
  if (PyInstance_Check(a_o)) {

    PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

    for (i=0,j=4; i < cnt; i++) {
      name  = PyTuple_GET_ITEM(d_o, j++);
      OMNIORB_ASSERT(PyString_Check(name));
      value = PyDict_GetItem(sdict, name);

      if (value) {
	t_o = omniPy::copyArgument(PyTuple_GET_ITEM(d_o, j++),
				   value, compstatus);
      }
      else {
	// Not such a fast case after all
	value = PyObject_GetAttr(a_o, name);
	if (value) {
	  t_o = omniPy::copyArgument(PyTuple_GET_ITEM(d_o, j++),
				     value, compstatus);
	  Py_DECREF(value);
	}
      }
      if (value && t_o) {
	PyTuple_SET_ITEM(argtuple, i, t_o);
      }
      else {
	Py_DECREF(argtuple);
	return setPyBadParam(compstatus);
      }
    }
  }
  else {
    for (i=0,j=4; i < cnt; i++) {
      name  = PyTuple_GET_ITEM(d_o, j++);
      OMNIORB_ASSERT(PyString_Check(name));
      value = PyObject_GetAttr(a_o, name);

      if (value) {
	t_o = omniPy::copyArgument(PyTuple_GET_ITEM(d_o, j++),
				   value, compstatus);
	if (t_o)
	  PyTuple_SET_ITEM(argtuple, i, t_o);
	Py_DECREF(value);
      }
      if (!(value && t_o)) {
	Py_DECREF(argtuple);
	return setPyBadParam(compstatus);
      }
    }
  }
  PyObject* r_o = PyEval_CallObject(PyTuple_GET_ITEM(d_o, 1), argtuple);
  OMNIORB_ASSERT(r_o);
  Py_DECREF(argtuple);
  return r_o;
}

static PyObject*
copyArgumentUnion(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{ // class,
  // repoId,
  // name,
  // discriminant descr,
  // default used,
  // ((label value, member name, member descr), ...),
  // default (label, name, descr) or None,
  // {label: (label, name, descr), ...}

  if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

  PyObject* udict  = ((PyInstanceObject*)a_o)->in_dict;
  PyObject* discr  = PyDict_GetItemString(udict, (char*)"_d");
  PyObject* value  = PyDict_GetItemString(udict, (char*)"_v");

  if (!(discr && value)) return setPyBadParam(compstatus);

  PyObject* t_o    = PyTuple_GET_ITEM(d_o, 4);
  PyObject* cdiscr = omniPy::copyArgument(t_o, discr, compstatus);

  if (!cdiscr) return setPyBadParam(compstatus);

  PyObject* cvalue = 0;
  PyObject* cdict  = PyTuple_GET_ITEM(d_o, 8);
  t_o              = PyDict_GetItem(cdict, discr);
  if (t_o) {
    // Discriminant found in case dictionary
    OMNIORB_ASSERT(PyTuple_Check(t_o));
    cvalue = omniPy::copyArgument(PyTuple_GET_ITEM(t_o, 2), value, compstatus);
  }
  else {
    // Is there a default case?
    t_o = PyTuple_GET_ITEM(d_o, 7);
    if (t_o == Py_None) {
      // No default
      Py_INCREF(Py_None);
      cvalue = Py_None;
    }
    else {
      OMNIORB_ASSERT(PyTuple_Check(t_o));
      cvalue = omniPy::copyArgument(PyTuple_GET_ITEM(t_o, 2),
				    value, compstatus);
    }
  }
  if (cvalue) {
    t_o = PyTuple_New(2);
    PyTuple_SET_ITEM(t_o, 0, cdiscr);
    PyTuple_SET_ITEM(t_o, 1, cvalue);
    PyObject* r_o = PyEval_CallObject(PyTuple_GET_ITEM(d_o, 1), t_o);
    OMNIORB_ASSERT(r_o);
    Py_DECREF(t_o);
    return r_o;
  }
  else {
    Py_DECREF(cdiscr);
    return setPyBadParam(compstatus);
  }
}

static PyObject*
copyArgumentEnum(PyObject* d_o, PyObject* a_o,
		 CORBA::CompletionStatus compstatus)
{ // repoId, name, item list

  if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

  PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
				      (char*)"_v");

  if (!(ev && PyInt_Check(ev))) return setPyBadParam(compstatus);

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 3);
  long      e   = PyInt_AS_LONG(ev);

  if (e >= PyTuple_GET_SIZE(t_o))      return setPyBadParam(compstatus);
  if (PyTuple_GET_ITEM(t_o, e) != a_o) return setPyBadParam(compstatus);

  Py_INCREF(a_o); return a_o;
}

static PyObject*
copyArgumentString(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // max_length

  PyObject* t_o = PyTuple_GET_ITEM(d_o, 1);
  OMNIORB_ASSERT(PyInt_Check(t_o));

  CORBA::ULong max_len = PyInt_AS_LONG(t_o);

  if (!PyString_Check(a_o)) return setPyBadParam(compstatus);

  CORBA::ULong len = PyString_GET_SIZE(a_o);

  if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

  // Annoyingly, we have to scan the string to check there are no
  // nulls
  char* str = PyString_AS_STRING(a_o);
  for (CORBA::ULong i=0; i<len; i++)
    if (str[i] == '\0') return setPyBadParam(compstatus);

  // After all that, we don't actually have to copy the string,
  // since they're immutable
  Py_INCREF(a_o);
  return a_o;
}

static PyObject*
copyArgumentSequence(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{ // element_desc, max_length

  PyObject*    r_o;
  PyObject*    t_o      = PyTuple_GET_ITEM(d_o, 2);
  OMNIORB_ASSERT(PyInt_Check(t_o));
  CORBA::ULong max_len  = PyInt_AS_LONG(t_o);
  PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

  CORBA::ULong len, i;

  if (sequenceOptimisedType(elm_desc)) { // Simple type
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
      // Mapping says octet and char use a string
      if (!PyString_Check(a_o)) return setPyBadParam(compstatus);
      len = PyString_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);
      Py_INCREF(a_o);
      return a_o;
    }
    else if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      int              valid = 1;
      long             long_val;
      unsigned long    ulong_val;
#ifdef HAS_LongLong
      CORBA::LongLong  llong_val;
      CORBA::ULongLong ullong_val;
#endif

      switch (etk) {
      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x8000 && long_val <= 0x7fff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0 && long_val <= 0xffff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) {
	      valid = 0; break;
	    }
#endif
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
	    if (long_val >= 0 && long_val <= 0xffffffffL) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#else
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_float:
      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyFloat_Check(t_o)) {
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	  }
	  else if (PyInt_Check(t_o)) {
	    PyList_SET_ITEM(r_o, i,
			    PyFloat_FromDouble((double)
					       PyInt_AS_LONG(t_o)));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val == 0 || long_val == 1) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	    }
	    else {
	      PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
	    }
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    llong_val = PyLong_AsLongLong(t_o);
	    if (llong_val == -1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ullong_val = PyLong_AsUnsignedLongLong(t_o);
	    if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
	  }
	  valid = 0; break;
	}
	break;
#else
      case 23:
      case 24:
	{
	  Py_DECREF(r_o);
	  return setPyNoImplement(compstatus);
	}
#endif
      default:
	OMNIORB_ASSERT(0);
      }
      if (valid)
	return r_o;
      else {
	Py_DECREF(r_o);
	return setPyBadParam(compstatus);
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      int              valid = 1;
      long             long_val;
      unsigned long    ulong_val;
#ifdef HAS_LongLong
      CORBA::LongLong  llong_val;
      CORBA::ULongLong ullong_val;
#endif

      switch (etk) {

      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x8000 && long_val <= 0x7fff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0 && long_val <= 0xffff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) {
	      valid = 0; break;
	    }
#endif
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
	    if (long_val >= 0 && long_val <= 0xffffffffL) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#else
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_float:
      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyFloat_Check(t_o)) {
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	  }
	  else if (PyInt_Check(t_o)) {
	    PyList_SET_ITEM(r_o, i,
			    PyFloat_FromDouble((double)
					       PyInt_AS_LONG(t_o)));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val == 0 || long_val == 1) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	    }
	    else {
	      PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
	    }
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    llong_val = PyLong_AsLongLong(t_o);
	    if (llong_val == -1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ullong_val = PyLong_AsUnsignedLongLong(t_o);
	    if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
	  }
	  valid = 0; break;
	}
	break;
#else
      case 23:
      case 24:
	{
	  Py_DECREF(r_o);
	  return setPyNoImplement(compstatus);
	}
#endif
      default:
	OMNIORB_ASSERT(0);
      }
      if (valid)
	return r_o;
      else {
	Py_DECREF(r_o);
	return setPyBadParam(compstatus);
      }
    }
    else
      return setPyBadParam(compstatus);
  }
  else { // Complex type
    int valid = 1;

    if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      for (i=0; i < len; i++) {
	t_o = omniPy::copyArgument(elm_desc, PyList_GET_ITEM(a_o, i),
				   compstatus);
	if (t_o)
	  PyList_SET_ITEM(r_o, i, t_o);
	else {
	  valid = 0; break;
	}
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      for (i=0; i < len; i++) {
	t_o = omniPy::copyArgument(elm_desc, PyTuple_GET_ITEM(a_o, i),
				   compstatus);
	if (t_o)
	  PyList_SET_ITEM(r_o, i, t_o);
	else {
	  valid = 0; break;
	}
      }
    }
    else
      return setPyBadParam(compstatus);

    if (valid)
      return r_o;
    else {
      Py_DECREF(r_o);
      return setPyBadParam(compstatus);
    }
  }
}

static PyObject*
copyArgumentArray(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{ // element_desc, length

  PyObject*    r_o;
  PyObject*    t_o      = PyTuple_GET_ITEM(d_o, 2);
  OMNIORB_ASSERT(PyInt_Check(t_o));
  CORBA::ULong arr_len  = PyInt_AS_LONG(t_o);
  PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

  CORBA::ULong len, i;

  if (sequenceOptimisedType(elm_desc)) { // Simple type
    CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

    if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
      // Mapping says octet and char use a string
      if (!PyString_Check(a_o)) return setPyBadParam(compstatus);
      len = PyString_GET_SIZE(a_o);
      if (len != arr_len) return setPyBadParam(compstatus);
      Py_INCREF(a_o);
      return a_o;
    }
    else if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (len != arr_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      int              valid = 1;
      long             long_val;
      unsigned long    ulong_val;
#ifdef HAS_LongLong
      CORBA::LongLong  llong_val;
      CORBA::ULongLong ullong_val;
#endif

      switch (etk) {
      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x8000 && long_val <= 0x7fff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0 && long_val <= 0xffff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) {
	      valid = 0; break;
	    }
#endif
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
	    if (long_val >= 0 && long_val <= 0xffffffffL) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#else
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_float:
      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyFloat_Check(t_o)) {
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	  }
	  else if (PyInt_Check(t_o)) {
	    PyList_SET_ITEM(r_o, i,
			    PyFloat_FromDouble((double)
					       PyInt_AS_LONG(t_o)));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val == 0 || long_val == 1) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	    }
	    else {
	      PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
	    }
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    llong_val = PyLong_AsLongLong(t_o);
	    if (llong_val == -1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyList_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ullong_val = PyLong_AsUnsignedLongLong(t_o);
	    if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
	  }
	  valid = 0; break;
	}
	break;
#else
      case 23:
      case 24:
	{
	  Py_DECREF(r_o);
	  return setPyNoImplement(compstatus);
	}
#endif
      default:
	OMNIORB_ASSERT(0);
      }
      if (valid)
	return r_o;
      else {
	Py_DECREF(r_o);
	return setPyBadParam(compstatus);
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (len != arr_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      int              valid = 1;
      long             long_val;
      unsigned long    ulong_val;
#ifdef HAS_LongLong
      CORBA::LongLong  llong_val;
      CORBA::ULongLong ullong_val;
#endif

      switch (etk) {

      case CORBA::tk_short:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x8000 && long_val <= 0x7fff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_long:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ushort:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0 && long_val <= 0xffff) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	    }
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_ulong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ulong_val = PyLong_AsUnsignedLong(t_o);
	    if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
#if SIZEOF_LONG > 4
	    if (ulong_val > 0xffffffffL) {
	      valid = 0; break;;
	    }
#endif
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
	    if (long_val >= 0 && long_val <= 0xffffffffL) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#else
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
#endif
	  }
	  valid = 0; break;
	}
	break;

      case CORBA::tk_float:
      case CORBA::tk_double:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyFloat_Check(t_o)) {
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	  }
	  else if (PyInt_Check(t_o)) {
	    PyList_SET_ITEM(r_o, i,
			    PyFloat_FromDouble((double)
					       PyInt_AS_LONG(t_o)));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_boolean:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val == 0 || long_val == 1) {
	      Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	    }
	    else {
	      PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
	    }
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

#ifdef HAS_LongLong

      case CORBA::tk_longlong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    llong_val = PyLong_AsLongLong(t_o);
	    if (llong_val == -1 && PyErr_Occurred()) {
	      valid = 0; break;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	  }
	  else {
	    valid = 0; break;
	  }
	}
	break;

      case CORBA::tk_ulonglong:
	for (i=0; i<len; i++) {
	  t_o = PyTuple_GET_ITEM(a_o, i);
	  if (PyLong_Check(t_o)) {
	    ullong_val = PyLong_AsUnsignedLongLong(t_o);
	    if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
	      valid = 0; break;;
	    }
	    Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	  }
	  else if (PyInt_Check(t_o)) {
	    long_val = PyInt_AS_LONG(t_o);
	    if (long_val >= 0) {
	      PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      continue;
	    }
	  }
	  valid = 0; break;
	}
	break;
#else
      case 23:
      case 24:
	{
	  Py_DECREF(r_o);
	  return setPyNoImplement(compstatus);
	}
#endif
      default:
	OMNIORB_ASSERT(0);
      }
      if (valid)
	return r_o;
      else {
	Py_DECREF(r_o);
	return setPyBadParam(compstatus);
      }
    }
    else
      return setPyBadParam(compstatus);
  }
  else { // Complex type
    int valid = 1;

    if (PyList_Check(a_o)) {
      len = PyList_GET_SIZE(a_o);
      if (len != arr_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      for (i=0; i < len; i++) {
	t_o = omniPy::copyArgument(elm_desc, PyList_GET_ITEM(a_o, i),
				   compstatus);
	if (t_o)
	  PyList_SET_ITEM(r_o, i, t_o);
	else {
	  valid = 0; break;
	}
      }
    }
    else if (PyTuple_Check(a_o)) {
      len = PyTuple_GET_SIZE(a_o);
      if (len != arr_len) return setPyBadParam(compstatus);

      r_o = PyList_New(len);

      for (i=0; i < len; i++) {
	t_o = omniPy::copyArgument(elm_desc, PyTuple_GET_ITEM(a_o, i),
				   compstatus);
	if (t_o)
	  PyList_SET_ITEM(r_o, i, t_o);
	else {
	  valid = 0; break;
	}
      }
    }
    else
      return setPyBadParam(compstatus);

    if (valid)
      return r_o;
    else {
      Py_DECREF(r_o);
      return setPyBadParam(compstatus);
    }
  }
}

static PyObject*
copyArgumentAlias(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{ // repoId, name, descr

  return omniPy::copyArgument(PyTuple_GET_ITEM(d_o, 3), a_o, compstatus);
}

static PyObject*
copyArgumentExcept(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{ // class, repoId, exc name, name, descriptor, ...

  if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

  PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

  // As with structs, the descriptor tuple has twice the number of
  // struct members plus 4.
  int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

  PyObject* t_o;
  PyObject* name;
  PyObject* value;
  PyObject* argtuple = PyTuple_New(cnt);

  int i, j;
  for (i=0,j=4; i < cnt; i++) {
    name    = PyTuple_GET_ITEM(d_o, j++);
    OMNIORB_ASSERT(PyString_Check(name));
    value   = PyDict_GetItem(sdict, name);
    if (!value) return setPyBadParam(compstatus);

    t_o = omniPy::copyArgument(PyTuple_GET_ITEM(d_o, j++), value, compstatus);

    if (t_o) {
      PyTuple_SET_ITEM(argtuple, i, t_o);
    }
    else {
      Py_DECREF(argtuple);
      return setPyBadParam(compstatus);
    }
  }
  PyObject* r_o = PyEval_CallObject(PyTuple_GET_ITEM(d_o, 1), argtuple);
  OMNIORB_ASSERT(r_o);
  Py_DECREF(argtuple);
  return r_o;
}

static PyObject*
copyArgumentLongLong(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{
#ifdef HAS_LongLong
  if (PyLong_Check(a_o)) {
    CORBA::LongLong ll = PyLong_AsLongLong(a_o);
    if (ll == -1 && PyErr_Occurred())
      return setPyBadParam(compstatus);
    Py_INCREF(a_o); return a_o;
  }
  else if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    return PyLong_FromLong(l);
  }
  else
    return setPyBadParam(compstatus);
#else
  return setPyNoImplement(compstatus);
#endif
}

static PyObject*
copyArgumentULongLong(PyObject* d_o, PyObject* a_o,
		      CORBA::CompletionStatus compstatus)
{
#ifdef HAS_LongLong
  if (PyLong_Check(a_o)) {
    CORBA::ULongLong ll = PyLong_AsUnsignedLongLong(a_o);
    if (ll == (CORBA::ULongLong)-1 && PyErr_Occurred())
      return setPyBadParam(compstatus);
    Py_INCREF(a_o); return a_o;
  }
  else if (PyInt_Check(a_o)) {
    long l = PyInt_AS_LONG(a_o);
    if (l < 0) return setPyBadParam(compstatus);
    return PyLong_FromLong(l);
  }
  else
    return setPyBadParam(compstatus);
#else
  return setPyNoImplement(compstatus);
#endif
}

static PyObject*
copyArgumentLongDouble(PyObject* d_o, PyObject* a_o,
		       CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentWChar(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentWString(PyObject* d_o, PyObject* a_o,
		    CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentFixed(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentValue(PyObject* d_o, PyObject* a_o,
		  CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentValueBox(PyObject* d_o, PyObject* a_o,
		     CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentNative(PyObject* d_o, PyObject* a_o,
		   CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}

static PyObject*
copyArgumentAbstractInterface(PyObject* d_o, PyObject* a_o,
			      CORBA::CompletionStatus compstatus)
{
  return setPyNoImplement(compstatus);
}


const omniPy::CopyArgumentFn omniPy::copyArgumentFns[] = {
  copyArgumentNull,
  copyArgumentVoid,
  copyArgumentShort,
  copyArgumentLong,
  copyArgumentUShort,
  copyArgumentULong,
  copyArgumentFloat,
  copyArgumentDouble,
  copyArgumentBoolean,
  copyArgumentChar,
  copyArgumentOctet,
  copyArgumentAny,
  copyArgumentTypeCode,
  copyArgumentPrincipal,
  copyArgumentObjref,
  copyArgumentStruct,
  copyArgumentUnion,
  copyArgumentEnum,
  copyArgumentString,
  copyArgumentSequence,
  copyArgumentArray,
  copyArgumentAlias,
  copyArgumentExcept,
  copyArgumentLongLong,
  copyArgumentULongLong,
  copyArgumentLongDouble,
  copyArgumentWChar,
  copyArgumentWString,
  copyArgumentFixed,
  copyArgumentValue,
  copyArgumentValueBox,
  copyArgumentNative,
  copyArgumentAbstractInterface
};






#if 0

PyObject*
omniPy::copyArgument(PyObject*               d_o,
		     PyObject*               a_o,
		     CORBA::CompletionStatus compstatus)
{
  CORBA::ULong   tk;
  CORBA::Boolean tup;
  PyObject*      t_o;     // Temporary object
  PyObject*      r_o = 0; // Result object

  if (PyTuple_Check(d_o)) {
    t_o = PyTuple_GET_ITEM(d_o, 0);
    OMNIORB_ASSERT(PyInt_Check(t_o));
    tk  = PyInt_AS_LONG(t_o);
    tup = 1;
  }
  else {
    OMNIORB_ASSERT(PyInt_Check(d_o));
    tk  = PyInt_AS_LONG(d_o);
    tup = 0;
  }

  switch (tk) {

    // Simple types

  case CORBA::tk_null:
  case CORBA::tk_void:
    {
      Py_INCREF(Py_None);
      return Py_None;
    }
    break;

  case CORBA::tk_short:
    {
      if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
	if (l < -0x8000 || l > 0x7fff) return setPyBadParam(compstatus);
	Py_INCREF(a_o); return a_o;
      }
      else return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_long:
    {
      if (PyInt_Check(a_o)) {
#if SIZEOF_LONG > 4
	long l = PyInt_AS_LONG(a_o);
	if (l < -0x80000000L || l > 0x7fffffffL)
	  return setPyBadParam(compstatus);
#endif
	Py_INCREF(a_o); return a_o;
      }
      else return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_ushort:
    {
      if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
	if (l < 0 || l > 0xffff) return setPyBadParam(compstatus);
	Py_INCREF(a_o); return a_o;
      }
      else return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_ulong:
    {
      if (PyLong_Check(a_o)) {
	unsigned long ul = PyLong_AsUnsignedLong(a_o);
	if (ul == (unsigned long)-1 && PyErr_Occurred())
	  return setPyBadParam(compstatus);
#if SIZEOF_LONG > 4
	if (ul > 0xffffffffL) return setPyBadParam(compstatus);
#endif
	Py_INCREF(a_o); return a_o;
      }
      else if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
#if SIZEOF_LONG > 4
	if (l < 0 || l > 0xffffffffL) return setPyBadParam(compstatus);
#else
	if (l < 0) return setPyBadParam(compstatus);
#endif
	return PyLong_FromLong(l);
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_float:
    {
      if (PyFloat_Check(a_o)) {
	Py_INCREF(a_o); return a_o;
      }
      else if (PyInt_Check(a_o)) {
	return PyFloat_FromDouble((double)(PyInt_AS_LONG(a_o)));
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_double:
    {
      if (PyFloat_Check(a_o)) {
	Py_INCREF(a_o); return a_o;
      }
      else if (PyInt_Check(a_o)) {
	return PyFloat_FromDouble((double)(PyInt_AS_LONG(a_o)));
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_boolean:
    {
      if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
	if (l == 0 || l == 1) {
	  Py_INCREF(a_o); return a_o;
	}
	else {
	  return PyInt_FromLong(1); // Normalise true value to 1
	}
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_char:
    {
      if ((PyString_Check(a_o) && (PyString_GET_SIZE(a_o) == 1))) {
	Py_INCREF(a_o); return a_o;
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_octet:
    {
      if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
	if (l < 0 || l > 0xff) return setPyBadParam(compstatus);
	Py_INCREF(a_o); return a_o;
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

#ifdef HAS_LongLong

  case CORBA::tk_longlong:
    {
      if (PyLong_Check(a_o)) {
	CORBA::LongLong ll = PyLong_AsLongLong(a_o);
	if (ll == -1 && PyErr_Occurred())
	  return setPyBadParam(compstatus);
	Py_INCREF(a_o); return a_o;
      }
      else if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
	return PyLong_FromLong(l);
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

  case CORBA::tk_ulonglong:
    {
      if (PyLong_Check(a_o)) {
	CORBA::ULongLong ll = PyLong_AsUnsignedLongLong(a_o);
	if (ll == (CORBA::ULongLong)-1 && PyErr_Occurred())
	  return setPyBadParam(compstatus);
	Py_INCREF(a_o); return a_o;
      }
      else if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
	if (l < 0) return setPyBadParam(compstatus);
	return PyLong_FromLong(l);
      }
      else
	return setPyBadParam(compstatus);
    }
    break;

#else

  case 23:
  case 24:
    {
      return setPyNoImplement(compstatus);
    }
    break;

#endif

  case CORBA::tk_any:
    {
      if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // TypeCode
      PyObject* tc = PyDict_GetItemString(adict, (char*)"_t");

      if (!(tc && PyInstance_Check(tc))) return setPyBadParam(compstatus);

      PyObject* tdict = ((PyInstanceObject*)tc)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
      if (!desc) return setPyBadParam(compstatus);

      // Any's contents
      PyObject* val = PyDict_GetItemString(adict, (char*)"_v");
      if (!val) return setPyBadParam(compstatus);

      // Copy contents
      PyObject* cval = copyArgument(desc, val, compstatus);

      if (!cval) return setPyBadParam(compstatus);

      // Construct new Any
      t_o = PyTuple_New(2);
      Py_INCREF(tc);
      PyTuple_SET_ITEM(t_o, 0, tc);
      PyTuple_SET_ITEM(t_o, 1, cval);
      r_o = PyEval_CallObject(pyCORBAAnyClass, t_o);
      Py_DECREF(t_o);

      return r_o;
    }
    break;

  case CORBA::tk_TypeCode:
    {
      if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
      if (!desc) return setPyBadParam(compstatus);

      Py_INCREF(a_o); return a_o;
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      OMNIORB_ASSERT(tup);

      return omniPy::copyObjRefArgument(PyTuple_GET_ITEM(d_o, 1),
					a_o, compstatus);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, name, descriptor, ...
    {
      OMNIORB_ASSERT(tup);

      // The descriptor tuple has twice the number of struct members,
      // plus 4 -- the typecode kind, the Python class, the repoId,
      // and the struct name
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;
      PyObject* argtuple = PyTuple_New(cnt);

      int i, j;

      // Optimise for the fast case, where the object is a class
      // instance with all attributes in its own dictionary
      if (PyInstance_Check(a_o)) {

	PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

	for (i=0,j=4; i < cnt; i++) {
	  name  = PyTuple_GET_ITEM(d_o, j++);
	  OMNIORB_ASSERT(PyString_Check(name));
	  value = PyDict_GetItem(sdict, name);

	  if (value) {
	    t_o = copyArgument(PyTuple_GET_ITEM(d_o, j++), value, compstatus);
	  }
	  else {
	    // Not such a fast case after all
	    value = PyObject_GetAttr(a_o, name);
	    if (value) {
	      t_o = copyArgument(PyTuple_GET_ITEM(d_o, j++),
				 value, compstatus);
	      Py_DECREF(value);
	    }
	  }
	  if (value && t_o) {
	    PyTuple_SET_ITEM(argtuple, i, t_o);
	  }
	  else {
	    // Fill in the remainder of the argtuple with Py_Nones
	    // then destroy it
	    for (; i < cnt; i++) {
	      Py_INCREF(Py_None);
	      PyTuple_SET_ITEM(argtuple, i, Py_None);
	    }
	    Py_DECREF(argtuple);
	    return setPyBadParam(compstatus);
	  }
	}
      }
      else {
	for (i=0,j=4; i < cnt; i++) {
	  name  = PyTuple_GET_ITEM(d_o, j++);
	  OMNIORB_ASSERT(PyString_Check(name));
	  value = PyObject_GetAttr(a_o, name);

	  if (value) {
	    t_o = copyArgument(PyTuple_GET_ITEM(d_o, j++), value, compstatus);
	    if (t_o)
	      PyTuple_SET_ITEM(argtuple, i, t_o);
	    Py_DECREF(value);
	  }
	  if (!(value && t_o)) {
	    // Fill in the remainder of the argtuple with Py_Nones
	    // then destroy it
	    for (; i < cnt; i++) {
	      Py_INCREF(Py_None);
	      PyTuple_SET_ITEM(argtuple, i, Py_None);
	    }
	    Py_DECREF(argtuple);
	    return setPyBadParam(compstatus);
	  }
	}
      }
      r_o = PyEval_CallObject(PyTuple_GET_ITEM(d_o, 1), argtuple);
      OMNIORB_ASSERT(r_o);
      Py_DECREF(argtuple);
      return r_o;
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
      OMNIORB_ASSERT(tup);
      if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

      PyObject* udict  = ((PyInstanceObject*)a_o)->in_dict;
      PyObject* discr  = PyDict_GetItemString(udict, (char*)"_d");
      PyObject* value  = PyDict_GetItemString(udict, (char*)"_v");

      if (!(discr && value)) return setPyBadParam(compstatus);

      t_o              = PyTuple_GET_ITEM(d_o, 4);
      PyObject* cdiscr = copyArgument(t_o, discr, compstatus);

      if (!cdiscr) return setPyBadParam(compstatus);

      PyObject* cvalue = 0;
      PyObject* cdict  = PyTuple_GET_ITEM(d_o, 8);
      t_o              = PyDict_GetItem(cdict, discr);
      if (t_o) {
	// Discriminant found in case dictionary
	OMNIORB_ASSERT(PyTuple_Check(t_o));
	cvalue = copyArgument(PyTuple_GET_ITEM(t_o, 2), value, compstatus);
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o == Py_None) {
	  // No default
	  Py_INCREF(Py_None);
	  cvalue = Py_None;
	}
	else {
	  OMNIORB_ASSERT(PyTuple_Check(t_o));
	  cvalue = copyArgument(PyTuple_GET_ITEM(t_o, 2), value, compstatus);
	}
      }
      if (cvalue) {
	t_o = PyTuple_New(2);
	PyTuple_SET_ITEM(t_o, 0, cdiscr);
	PyTuple_SET_ITEM(t_o, 1, cvalue);
	r_o = PyEval_CallObject(PyTuple_GET_ITEM(d_o, 1), t_o);
	OMNIORB_ASSERT(r_o);
	Py_DECREF(t_o);
	return r_o;
      }
      else {
	Py_DECREF(cdiscr);
	return setPyBadParam(compstatus);
      }
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

      PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
					  (char*)"_v");

      if (!(ev && PyInt_Check(ev))) return setPyBadParam(compstatus);

      t_o    = PyTuple_GET_ITEM(d_o, 3);
      long e = PyInt_AS_LONG(ev);

      if (e >= PyTuple_GET_SIZE(t_o))      return setPyBadParam(compstatus);
      if (PyTuple_GET_ITEM(t_o, e) != a_o) return setPyBadParam(compstatus);

      Py_INCREF(a_o); return a_o;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      if (!PyString_Check(a_o)) return setPyBadParam(compstatus);

      CORBA::ULong len = PyString_GET_SIZE(a_o);

      if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

      // Annoyingly, we have to scan the string to check there are no
      // nulls
      char* str = PyString_AS_STRING(a_o);
      for (CORBA::ULong i=0; i<len; i++)
	if (str[i] == '\0') return setPyBadParam(compstatus);

      // After all that, we don't actually have to copy the string,
      // since they're immutable
      Py_INCREF(a_o);
      return a_o;
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      OMNIORB_ASSERT(tup);

      t_o                   = PyTuple_GET_ITEM(d_o, 2);
      OMNIORB_ASSERT(PyInt_Check(t_o));
      CORBA::ULong max_len  = PyInt_AS_LONG(t_o);
      PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong len, i;

      if (sequenceOptimisedType(elm_desc)) { // Simple type
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
	  // Mapping says octet and char use a string
	  if (!PyString_Check(a_o)) return setPyBadParam(compstatus);
	  len = PyString_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);
	  Py_INCREF(a_o);
	  return a_o;
	}
	else if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  int              valid = 1;
	  long             long_val;
	  unsigned long    ulong_val;
#ifdef HAS_LongLong
	  CORBA::LongLong  llong_val;
	  CORBA::ULongLong ullong_val;
#endif

	  switch (etk) {
	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x8000 && long_val <= 0x7fff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0 && long_val <= 0xffff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#endif
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
		if (long_val >= 0 && long_val <= 0xffffffffL) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#else
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;

	  case CORBA::tk_float:
	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyFloat_Check(t_o)) {
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	      }
	      else if (PyInt_Check(t_o)) {
		PyList_SET_ITEM(r_o, i,
				PyFloat_FromDouble((double)
						   PyInt_AS_LONG(t_o)));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val == 0 || long_val == 1) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
		}
		else {
		  PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
		}
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

#ifdef HAS_LongLong

	  case CORBA::tk_longlong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		llong_val = PyLong_AsLongLong(t_o);
		if (llong_val == -1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_ulonglong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ullong_val = PyLong_AsUnsignedLongLong(t_o);
		if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;
#else
	  case 23:
	  case 24:
	    {
	      for (i=0; i<len; i++)
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      Py_DECREF(r_o);
	      return setPyNoImplement(compstatus);
	    }
#endif
	  default:
	    OMNIORB_ASSERT(0);
	  }
	  if (valid)
	    return r_o;
	  else {
	    Py_DECREF(r_o);
	    return setPyBadParam(compstatus);
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  int              valid = 1;
	  long             long_val;
	  unsigned long    ulong_val;
#ifdef HAS_LongLong
	  CORBA::LongLong  llong_val;
	  CORBA::ULongLong ullong_val;
#endif

	  switch (etk) {

	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x8000 && long_val <= 0x7fff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0 && long_val <= 0xffff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#endif
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
		if (long_val >= 0 && long_val <= 0xffffffffL) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#else
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;

	  case CORBA::tk_float:
	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyFloat_Check(t_o)) {
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	      }
	      else if (PyInt_Check(t_o)) {
		PyList_SET_ITEM(r_o, i,
				PyFloat_FromDouble((double)
						   PyInt_AS_LONG(t_o)));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val == 0 || long_val == 1) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
		}
		else {
		  PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
		}
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

#ifdef HAS_LongLong

	  case CORBA::tk_longlong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		llong_val = PyLong_AsLongLong(t_o);
		if (llong_val == -1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_ulonglong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ullong_val = PyLong_AsUnsignedLongLong(t_o);
		if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;
#else
	  case 23:
	  case 24:
	    {
	      for (i=0; i<len; i++)
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      Py_DECREF(r_o);
	      return setPyNoImplement(compstatus);
	    }
#endif
	  default:
	    OMNIORB_ASSERT(0);
	  }
	  if (valid)
	    return r_o;
	  else {
	    Py_DECREF(r_o);
	    return setPyBadParam(compstatus);
	  }
	}
	else
	  return setPyBadParam(compstatus);
      }
      else { // Complex type
	int valid = 1;

	if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  for (i=0; i < len; i++) {
	    t_o = copyArgument(elm_desc, PyList_GET_ITEM(a_o, i), compstatus);
	    if (t_o)
	      PyList_SET_ITEM(r_o, i, t_o);
	    else {
	      Py_INCREF(Py_None);
	      PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  for (i=0; i < len; i++) {
	    t_o = copyArgument(elm_desc, PyTuple_GET_ITEM(a_o, i), compstatus);
	    if (t_o)
	      PyList_SET_ITEM(r_o, i, t_o);
	    else {
	      Py_INCREF(Py_None);
	      PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	  }
	}
	else
	  return setPyBadParam(compstatus);

	if (valid)
	  return r_o;
	else {
	  Py_DECREF(r_o);
	  return setPyBadParam(compstatus);
	}
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      OMNIORB_ASSERT(tup);

      t_o                   = PyTuple_GET_ITEM(d_o, 2);
      OMNIORB_ASSERT(PyInt_Check(t_o));
      CORBA::ULong arr_len  = PyInt_AS_LONG(t_o);
      PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong len, i;

      if (sequenceOptimisedType(elm_desc)) { // Simple type
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
	  // Mapping says octet and char use a string
	  if (!PyString_Check(a_o)) return setPyBadParam(compstatus);
	  len = PyString_GET_SIZE(a_o);
	  if (len != arr_len) return setPyBadParam(compstatus);
	  Py_INCREF(a_o);
	  return a_o;
	}
	else if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  int              valid = 1;
	  long             long_val;
	  unsigned long    ulong_val;
#ifdef HAS_LongLong
	  CORBA::LongLong  llong_val;
	  CORBA::ULongLong ullong_val;
#endif

	  switch (etk) {
	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x8000 && long_val <= 0x7fff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0 && long_val <= 0xffff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#endif
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
		if (long_val >= 0 && long_val <= 0xffffffffL) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#else
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;

	  case CORBA::tk_float:
	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyFloat_Check(t_o)) {
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	      }
	      else if (PyInt_Check(t_o)) {
		PyList_SET_ITEM(r_o, i,
				PyFloat_FromDouble((double)
						   PyInt_AS_LONG(t_o)));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val == 0 || long_val == 1) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
		}
		else {
		  PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
		}
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

#ifdef HAS_LongLong

	  case CORBA::tk_longlong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		llong_val = PyLong_AsLongLong(t_o);
		if (llong_val == -1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_ulonglong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ullong_val = PyLong_AsUnsignedLongLong(t_o);
		if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;
#else
	  case 23:
	  case 24:
	    {
	      for (i=0; i<len; i++)
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      Py_DECREF(r_o);
	      return setPyNoImplement(compstatus);
	    }
#endif
	  default:
	    OMNIORB_ASSERT(0);
	  }
	  if (valid)
	    return r_o;
	  else {
	    Py_DECREF(r_o);
	    return setPyBadParam(compstatus);
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (len != arr_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  int              valid = 1;
	  long             long_val;
	  unsigned long    ulong_val;
#ifdef HAS_LongLong
	  CORBA::LongLong  llong_val;
	  CORBA::ULongLong ullong_val;
#endif

	  switch (etk) {

	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x8000 && long_val <= 0x7fff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
#if SIZEOF_LONG > 4
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= -0x80000000L && long_val <= 0x7fffffffL) {
#endif
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
#if SIZEOF_LONG > 4
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0 && long_val <= 0xffff) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None); valid = 0;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
#endif
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
		if (long_val >= 0 && long_val <= 0xffffffffL) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#else
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
#endif
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;

	  case CORBA::tk_float:
	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyFloat_Check(t_o)) {
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
	      }
	      else if (PyInt_Check(t_o)) {
		PyList_SET_ITEM(r_o, i,
				PyFloat_FromDouble((double)
						   PyInt_AS_LONG(t_o)));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val == 0 || long_val == 1) {
		  Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o);
		}
		else {
		  PyList_SET_ITEM(r_o, i, PyInt_FromLong(1));
		}
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

#ifdef HAS_LongLong

	  case CORBA::tk_longlong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		llong_val = PyLong_AsLongLong(t_o);
		if (llong_val == -1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
	      }
	      else {
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		valid = 0;
	      }
	    }
	    break;

	  case CORBA::tk_ulonglong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ullong_val = PyLong_AsUnsignedLongLong(t_o);
		if (ullong_val == (CORBA::ULongLong)-1 && PyErr_Occurred()) {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0; continue;
		}
		Py_INCREF(t_o); PyList_SET_ITEM(r_o, i, t_o); continue;
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
		if (long_val >= 0) {
		  PyList_SET_ITEM(r_o, i, PyLong_FromLong(long_val));
		  continue;
		}
	      }
	      Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	    break;
#else
	  case 23:
	  case 24:
	    {
	      for (i=0; i<len; i++)
		Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
	      Py_DECREF(r_o);
	      return setPyNoImplement(compstatus);
	    }
#endif
	  default:
	    OMNIORB_ASSERT(0);
	  }
	  if (valid)
	    return r_o;
	  else {
	    Py_DECREF(r_o);
	    return setPyBadParam(compstatus);
	  }
	}
	else
	  return setPyBadParam(compstatus);
      }
      else { // Complex type
	int valid = 1;

	if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  for (i=0; i < len; i++) {
	    t_o = copyArgument(elm_desc, PyList_GET_ITEM(a_o, i), compstatus);
	    if (t_o)
	      PyList_SET_ITEM(r_o, i, t_o);
	    else {
	      Py_INCREF(Py_None);
	      PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (len != arr_len) return setPyBadParam(compstatus);

	  r_o = PyList_New(len);

	  for (i=0; i < len; i++) {
	    t_o = copyArgument(elm_desc, PyTuple_GET_ITEM(a_o, i), compstatus);
	    if (t_o)
	      PyList_SET_ITEM(r_o, i, t_o);
	    else {
	      Py_INCREF(Py_None);
	      PyList_SET_ITEM(r_o, i, Py_None);
	      valid = 0;
	    }
	  }
	}
	else
	  return setPyBadParam(compstatus);

	if (valid)
	  return r_o;
	else {
	  Py_DECREF(r_o);
	  return setPyBadParam(compstatus);
	}
      }
    }
    break;
    
  case CORBA::tk_alias: // repoId, name, descr
    {
      OMNIORB_ASSERT(tup);
      return copyArgument(PyTuple_GET_ITEM(d_o, 3), a_o, compstatus);
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      OMNIORB_ASSERT(tup);
      if (!PyInstance_Check(a_o)) return setPyBadParam(compstatus);

      PyObject* sdict = ((PyInstanceObject*)a_o)->in_dict;

      // As with structs, the descriptor tuple has twice the number of
      // struct members plus 4.
      int       cnt   = (PyTuple_GET_SIZE(d_o) - 4) / 2;

      PyObject* name;
      PyObject* value;
      PyObject* argtuple = PyTuple_New(cnt);

      int i, j;
      for (i=0,j=4; i < cnt; i++) {
	name    = PyTuple_GET_ITEM(d_o, j++);
	OMNIORB_ASSERT(PyString_Check(name));
	value   = PyDict_GetItem(sdict, name);
	if (!value) return setPyBadParam(compstatus);

	t_o = copyArgument(PyTuple_GET_ITEM(d_o, j++), value, compstatus);

	if (t_o) {
	  PyTuple_SET_ITEM(argtuple, i, t_o);
	}
	else {
	  // Fill in the remainder of the argtuple with Py_Nones
	  // then destroy it
	  for (; i < cnt; i++) {
	    Py_INCREF(Py_None);
	    PyTuple_SET_ITEM(argtuple, i, Py_None);
	  }
	  Py_DECREF(argtuple);
	  return setPyBadParam(compstatus);
	}
      }
      r_o = PyEval_CallObject(PyTuple_GET_ITEM(d_o, 1), argtuple);
      OMNIORB_ASSERT(r_o);
      Py_DECREF(argtuple);
      return r_o;
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyList_Check(t_o));
      return copyArgument(PyList_GET_ITEM(t_o, 0), a_o, compstatus);
    }
    break;

  default:
    omniORB::log << "!!! copyArgument(): unsupported typecode: "
		 << (CORBA::ULong)tk << "\n";
    omniORB::log.flush();
    OMNIORB_THROW(MARSHAL,0,CORBA::COMPLETED_NO);
  }
  // Never reach here
  OMNIORB_ASSERT(0);
  return 0;
}

#endif
