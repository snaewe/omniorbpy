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
// Revision 1.20  2000/03/28 11:54:24  dpg1
// Refcounting bug in Any unmarshalling.
//
// Revision 1.19  2000/03/24 16:48:58  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.18  2000/03/14 10:58:18  dpg1
// Bug with unmarshalling plain CORBA::Object with omniORB 3.0.
//
// Revision 1.17  2000/03/03 17:41:43  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//
// Revision 1.16  2000/02/15 09:50:22  dpg1
// Bug in union unmashalling.
//
// Revision 1.15  2000/02/04 12:17:10  dpg1
// Support for VMS.
//
// Revision 1.14  2000/01/20 17:47:09  dpg1
// Refcounting bug in any handling.
//
// Revision 1.13  2000/01/10 19:58:59  dpg1
// Struct marshalling is more forgiving. BAD_PARAM exceptions instead of
// assertion failures with incorrect data.
//
// Revision 1.12  1999/12/15 12:17:20  dpg1
// Changes to compile with SunPro CC 5.0.
//
// Revision 1.11  1999/12/07 16:09:19  dpg1
// Sequence and array now cope with tuples as well as lists.
//
// Revision 1.10  1999/09/30 11:27:08  dpg1
// Unmarshalling object references specified as CORBA::Object now works.
//
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

#include <omnipy.h>

#if defined(__DECCXX)
// EDG based compaq cxx is having a problem with taking the address of static
// functions.
PyObject* omnipyCompaqCxxBug() {
  // Oddly, modules that invoke the following function don't have a problem.
  return omniPy::newTwin(0); // never call this.
}
#endif

#define AS_THROW_BAD_PARAM throw CORBA::BAD_PARAM(0,compstatus)

CORBA::ULong
omniPy::alignedSize(CORBA::ULong            msgsize,
		    PyObject*               d_o,
		    PyObject*               a_o,
		    CORBA::CompletionStatus compstatus)
{
  CORBA::ULong   tk;
  CORBA::Boolean tup;
  PyObject*      t_o; // Temporary object

  //  cout << "alignedSize() adding to " << msgsize << "..." << endl;

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
    break;

  case CORBA::tk_short:
    {
      if (!PyInt_Check(a_o)) AS_THROW_BAD_PARAM;
      long l = PyInt_AS_LONG(a_o);
      if (l < -0x8000 || l > 0x7fff) AS_THROW_BAD_PARAM;
      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
      msgsize += 2;
    }
    break;

  case CORBA::tk_long:
    {
      if (!PyInt_Check(a_o)) AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
      long l = PyInt_AS_LONG(a_o);
      if (l < -0x80000000L || l > 0x7fffffffL) AS_THROW_BAD_PARAM;
#endif
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_ushort:
    {
      if (!PyInt_Check(a_o)) AS_THROW_BAD_PARAM;
      long l = PyInt_AS_LONG(a_o);
      if (l < 0 || l > 0xffff) AS_THROW_BAD_PARAM;
      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
      msgsize += 2;
    }
    break;

  case CORBA::tk_ulong:
    {
      if (PyLong_Check(a_o)) {
	unsigned long ul = PyLong_AsUnsignedLong(a_o);
	if (ul == (unsigned long)-1 && PyErr_Occurred())
	  AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	if (ul > 0xffffffffL) AS_THROW_BAD_PARAM;
#endif
      }
      else if (PyInt_Check(a_o)) {
	long l = PyInt_AS_LONG(a_o);
#if SIZEOF_LONG > 4
	if (l < 0 || l > 0xffffffffL) AS_THROW_BAD_PARAM;
#else
	if (l < 0) AS_THROW_BAD_PARAM;
#endif
      }
      else
	AS_THROW_BAD_PARAM;

      if (!(PyInt_Check(a_o) || PyLong_Check(a_o))) AS_THROW_BAD_PARAM;
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_float:
    {
      if (!(PyFloat_Check(a_o) || PyInt_Check(a_o))) AS_THROW_BAD_PARAM;
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_double:
    {
      if (!(PyFloat_Check(a_o) || PyInt_Check(a_o))) AS_THROW_BAD_PARAM;
      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
      msgsize += 8;
    }
    break;

  case CORBA::tk_boolean:
    {
      if (!PyInt_Check(a_o)) AS_THROW_BAD_PARAM;
      msgsize += 1;
    }
    break;

  case CORBA::tk_char:
    {
      if (!(PyString_Check(a_o) && (PyString_GET_SIZE(a_o) == 1)))
	AS_THROW_BAD_PARAM;
      msgsize += 1;
    }
    break;

  case CORBA::tk_octet:
    {
      if (!PyInt_Check(a_o)) AS_THROW_BAD_PARAM;
      long l = PyInt_AS_LONG(a_o);
      if (l < 0 || l > 0xff) AS_THROW_BAD_PARAM;
      msgsize += 1;
    }
    break;

  case CORBA::tk_any:
    {
      if (!PyInstance_Check(a_o)) AS_THROW_BAD_PARAM;

      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // Size of TypeCode
      t_o             = PyDict_GetItemString(adict, (char*)"_t");
      if (!(t_o && PyInstance_Check(t_o))) AS_THROW_BAD_PARAM;

      PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
      if (!desc) AS_THROW_BAD_PARAM;

      msgsize         = alignedSizeTypeCode(msgsize, desc);

      // Size of Any's contents
      t_o             = PyDict_GetItemString(adict, (char*)"_v");
      if (!t_o) AS_THROW_BAD_PARAM;

      msgsize         = alignedSize(msgsize, desc, t_o, compstatus);
    }
    break;

  case CORBA::tk_TypeCode:
    {
      if (!PyInstance_Check(a_o)) AS_THROW_BAD_PARAM;

      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      t_o             = PyDict_GetItemString(tdict, (char*)"_d");
      if (!t_o) AS_THROW_BAD_PARAM;
      msgsize         = alignedSizeTypeCode(msgsize, t_o);
    }
    break;

    // Complex types

  case CORBA::tk_objref: // repoId, name
    {
      OMNIORB_ASSERT(tup);

      CORBA::Object_ptr obj;
      const char*       repoId;

      if (a_o == Py_None) {
	// Nil object reference
	obj    = CORBA::Object::_nil();
	repoId = "";
      }
      else {
	if (!PyInstance_Check(a_o)) AS_THROW_BAD_PARAM;
	obj = (CORBA::Object_ptr)getTwin(a_o, OBJREF_TWIN);
	if (!obj) AS_THROW_BAD_PARAM;
#ifdef OMNIORBPY_FOR_28
	repoId = obj->PR_getobj()->NP_IRRepositoryId();
#else
	repoId = obj->_PR_getobj()->_mostDerivedRepoId();
#endif
      }
      msgsize = CORBA::AlignedObjRef(obj, repoId, strlen(repoId) + 1, msgsize);
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
	    msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++),
				  value, compstatus);
	  }
	  else {
	    // Not such a fast case after all
	    value = PyObject_GetAttr(a_o, name);
	    if (!value) AS_THROW_BAD_PARAM;
	    msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++),
				  value, compstatus);
	    Py_DECREF(value);
	  }
	}
      }
      else {
	for (i=0,j=4; i < cnt; i++) {
	  name    = PyTuple_GET_ITEM(d_o, j++);
	  OMNIORB_ASSERT(PyString_Check(name));
	  value   = PyObject_GetAttr(a_o, name);
	  if (!value) AS_THROW_BAD_PARAM;
	  msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++),
				value, compstatus);
	  Py_DECREF(value);
	}
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
      OMNIORB_ASSERT(tup);
      if (!PyInstance_Check(a_o)) AS_THROW_BAD_PARAM;

      PyObject* udict = ((PyInstanceObject*)a_o)->in_dict;

      PyObject* discriminant = PyDict_GetItemString(udict, (char*)"_d");
      PyObject* value        = PyDict_GetItemString(udict, (char*)"_v");
      if (!(discriminant && value)) AS_THROW_BAD_PARAM;

      t_o = PyTuple_GET_ITEM(d_o, 4); // Discriminant descriptor
      msgsize = alignedSize(msgsize, t_o, discriminant, compstatus);

      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);
      OMNIORB_ASSERT(PyDict_Check(cdict));

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	OMNIORB_ASSERT(PyTuple_Check(t_o));
	msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(t_o, 2),
			      value, compstatus);
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  OMNIORB_ASSERT(PyTuple_Check(t_o));
	  msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(t_o, 2),
				value, compstatus);
	}
      }
    }
    break;

  case CORBA::tk_enum: // repoId, name, item list
    {
      if (!PyInstance_Check(a_o)) AS_THROW_BAD_PARAM;
      PyObject* ev = PyDict_GetItemString(((PyInstanceObject*)a_o)->in_dict,
					  (char*)"_v");
      if (!(ev && PyInt_Check(ev))) AS_THROW_BAD_PARAM;

      t_o    = PyTuple_GET_ITEM(d_o, 3);
      long e = PyInt_AS_LONG(ev);

      if (e >= PyTuple_GET_SIZE(t_o))      AS_THROW_BAD_PARAM;
      if (PyTuple_GET_ITEM(t_o, e) != a_o) AS_THROW_BAD_PARAM;

      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      if (!PyString_Check(a_o)) AS_THROW_BAD_PARAM;

      CORBA::ULong len = PyString_GET_SIZE(a_o);

      if (max_len > 0 && len > max_len) AS_THROW_BAD_PARAM;

      // Annoyingly, we have to scan the string to check there are no
      // nulls
      char* str = PyString_AS_STRING(a_o);
      for (CORBA::ULong i=0; i<len; i++)
	if (str[i] == '\0') AS_THROW_BAD_PARAM;

      msgsize  = omni::align_to(msgsize, omni::ALIGN_4);
      msgsize += 4 + PyString_GET_SIZE(a_o) + 1;
    }
    break;

  case CORBA::tk_sequence: // element_desc, max_length
    {
      OMNIORB_ASSERT(tup);

      // Space for list length
      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
      msgsize += 4;

      t_o                   = PyTuple_GET_ITEM(d_o, 2);
      OMNIORB_ASSERT(PyInt_Check(t_o));
      CORBA::ULong max_len  = PyInt_AS_LONG(t_o);
      PyObject*    elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong  len, i;
      long          long_val;
      unsigned long ulong_val;

      if (PyInt_Check(elm_desc)) { // Simple type
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
	  // Mapping says octet and char use a string
	  if (!PyString_Check(a_o)) AS_THROW_BAD_PARAM;
	  len = PyString_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) AS_THROW_BAD_PARAM;
	  msgsize += len;
	}
	else if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) AS_THROW_BAD_PARAM;

	  switch (etk) {
	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x8000 || long_val > 0x7fff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x80000000L || long_val > 0x7fffffffL)
		AS_THROW_BAD_PARAM;
#endif
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < 0 || long_val > 0xffff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
		  AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) AS_THROW_BAD_PARAM;
#endif
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
		if (long_val < 0 || long_val > 0xffffffffL) AS_THROW_BAD_PARAM;
#else
		if (long_val < 0) AS_THROW_BAD_PARAM;
#endif
	      }
	      else
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_float:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
	      msgsize += 8 * len;
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++)
	      if (!PyInt_Check(PyList_GET_ITEM(a_o, i))) AS_THROW_BAD_PARAM;
	    msgsize += len;
	    break;

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) AS_THROW_BAD_PARAM;

	  switch (etk) {
	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x8000 || long_val > 0x7fff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x80000000L || long_val > 0x7fffffffL)
		AS_THROW_BAD_PARAM;
#endif
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < 0 || long_val > 0xffff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
		  AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) AS_THROW_BAD_PARAM;
#endif
	      }
	      else if (!PyInt_Check(a_o))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_float:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
	      msgsize += 8 * len;
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++)
	      if (!PyInt_Check(PyTuple_GET_ITEM(a_o, i))) AS_THROW_BAD_PARAM;
	    msgsize += len;
	    break;

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
	else
	  AS_THROW_BAD_PARAM;
      }
      else { // Complex type
	if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) AS_THROW_BAD_PARAM;
	  
	  for (i=0; i < len; i++) {
	    msgsize = alignedSize(msgsize, elm_desc,
				  PyList_GET_ITEM(a_o, i), compstatus);
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (max_len > 0 && len > max_len) AS_THROW_BAD_PARAM;

	  for (i=0; i < len; i++) {
	    msgsize = alignedSize(msgsize, elm_desc,
				  PyTuple_GET_ITEM(a_o, i), compstatus);
	  }
	}
	else
	  AS_THROW_BAD_PARAM;
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

      CORBA::ULong  len, i;
      long          long_val;
      unsigned long ulong_val;

      if (PyInt_Check(elm_desc)) { // Simple type
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet || etk == CORBA::tk_char) {
	  // Mapping says octet and char use a string
	  if (!PyString_Check(a_o)) AS_THROW_BAD_PARAM;
	  len = PyString_GET_SIZE(a_o);
	  if (len != arr_len) AS_THROW_BAD_PARAM;
	  msgsize += len;
	}
	else if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) AS_THROW_BAD_PARAM;

	  switch (etk) {
	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x8000 || long_val > 0x7fff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x80000000L || long_val > 0x7fffffffL)
		AS_THROW_BAD_PARAM;
#endif
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < 0 || long_val > 0xffff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
		  AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) AS_THROW_BAD_PARAM;
#endif
	      }
	      else if (PyInt_Check(t_o)) {
		long_val = PyInt_AS_LONG(t_o);
#if SIZEOF_LONG > 4
		if (long_val < 0 || long_val > 0xffffffffL) AS_THROW_BAD_PARAM;
#else
		if (long_val < 0) AS_THROW_BAD_PARAM;
#endif
	      }
	      else
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_float:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyList_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
	      msgsize += 8 * len;
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++)
	      if (!PyInt_Check(PyList_GET_ITEM(a_o, i))) AS_THROW_BAD_PARAM;
	    msgsize += len;
	    break;

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (len != arr_len) AS_THROW_BAD_PARAM;

	  switch (etk) {

	  case CORBA::tk_short:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x8000 || long_val > 0x7fff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_long:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < -0x80000000L || long_val > 0x7fffffffL)
		AS_THROW_BAD_PARAM;
#endif
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_ushort:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!PyInt_Check(t_o)) AS_THROW_BAD_PARAM;
	      long_val = PyInt_AS_LONG(t_o);
	      if (long_val < 0 || long_val > 0xffff) AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_2);
	      msgsize += 2 * len;
	    }
	    break;

	  case CORBA::tk_ulong:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (PyLong_Check(t_o)) {
		ulong_val = PyLong_AsUnsignedLong(t_o);
		if (ulong_val == (unsigned long)-1 && PyErr_Occurred())
		  AS_THROW_BAD_PARAM;
#if SIZEOF_LONG > 4
		if (ulong_val > 0xffffffffL) AS_THROW_BAD_PARAM;
#endif
	      }
	      else if (!PyInt_Check(a_o))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_float:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_4);
	      msgsize += 4 * len;
	    }
	    break;

	  case CORBA::tk_double:
	    for (i=0; i<len; i++) {
	      t_o = PyTuple_GET_ITEM(a_o, i);
	      if (!(PyFloat_Check(t_o) || PyInt_Check(t_o)))
		AS_THROW_BAD_PARAM;
	    }
	    if (len > 0) {
	      msgsize = omni::align_to(msgsize,omni::ALIGN_8);
	      msgsize += 8 * len;
	    }
	    break;

	  case CORBA::tk_boolean:
	    for (i=0; i<len; i++)
	      if (!PyInt_Check(PyTuple_GET_ITEM(a_o, i))) AS_THROW_BAD_PARAM;
	    msgsize += len;
	    break;

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
	else
	  AS_THROW_BAD_PARAM;
      }
      else { // Complex type
	if (PyList_Check(a_o)) {
	  len = PyList_GET_SIZE(a_o);
	  if (len != arr_len) AS_THROW_BAD_PARAM;

	  for (i=0; i < len; i++) {
	    msgsize = alignedSize(msgsize, elm_desc,
				  PyList_GET_ITEM(a_o, i), compstatus);
	  }
	}
	else if (PyTuple_Check(a_o)) {
	  len = PyTuple_GET_SIZE(a_o);
	  if (len != arr_len) AS_THROW_BAD_PARAM;

	  for (i=0; i < len; i++) {
	    msgsize = alignedSize(msgsize, elm_desc,
				  PyTuple_GET_ITEM(a_o, i), compstatus);
	  }
	}
      }
    }
    break;
    
  case CORBA::tk_alias: // repoId, name, descr
    {
      OMNIORB_ASSERT(tup);
      msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, 3),
			    a_o, compstatus);
    }
    break;

  case CORBA::tk_except: // class, repoId, exc name, name, descriptor, ...
    {
      OMNIORB_ASSERT(tup);
      if (!PyInstance_Check(a_o)) AS_THROW_BAD_PARAM;

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
	OMNIORB_ASSERT(PyString_Check(name));
	value   = PyDict_GetItem(sdict, name);
	if (!value) AS_THROW_BAD_PARAM;
	msgsize = alignedSize(msgsize, PyTuple_GET_ITEM(d_o, j++),
			      value, compstatus);
      }
    }
    break;

  case 0xffffffff: // [indirect descriptor]
    {
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyList_Check(t_o));

      msgsize = alignedSize(msgsize, PyList_GET_ITEM(t_o, 0), a_o, compstatus);
    }
    break;

  default:
    omniORB::log << "!!! alignedSize(): unsupported typecode: "
		 << (CORBA::ULong)tk << "\n";
    omniORB::log.flush();
    abort();
  }
  //  cout << "  alignedSize() returning " << msgsize << "." << endl;
  return msgsize;
}
#undef AS_THROW_BAD_PARAM


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

  case CORBA::tk_null:
  case CORBA::tk_void:
    break;

  case CORBA::tk_short:
    {
      CORBA::Short s = PyInt_AS_LONG(a_o);
      s >>= stream;
    }
    break;

  case CORBA::tk_long:
    {
      CORBA::Long l = PyInt_AS_LONG(a_o);
      l >>= stream;
    }
    break;

  case CORBA::tk_ushort:
    {
      CORBA::UShort us = PyInt_AS_LONG(a_o);
      us >>= stream;
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul = 0; // Initialised to stop egcs complaining

      if (PyLong_Check(a_o))
	ul = PyLong_AsUnsignedLong(a_o);
      else // It's an int
	ul = PyInt_AS_LONG(a_o);

      ul >>= stream;
    }
    break;

  case CORBA::tk_float:
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
    break;

  case CORBA::tk_double:
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
    break;

  case CORBA::tk_boolean:
    {
      CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
      b >>= stream;
    }
    break;

  case CORBA::tk_char:
    {
      char *str = PyString_AS_STRING(a_o);

      CORBA::Char c = str[0];
      c >>= stream;
    }
    break;

  case CORBA::tk_octet:
    {
      CORBA::Octet o = PyInt_AS_LONG(a_o);
      o >>= stream;
    }
    break;

  case CORBA::tk_any:
    {
      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // TypeCode
      t_o             = PyDict_GetItemString(adict, (char*)"_t");
      PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
      marshalTypeCode(stream, desc);

      // Any's contents
      t_o             = PyDict_GetItemString(adict, (char*)"_v");
      marshalPyObject(stream, desc, t_o);
    }
    break;

  case CORBA::tk_TypeCode:
    {
      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      t_o             = PyDict_GetItemString(tdict, (char*)"_d");
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
#ifdef OMNIORBPY_FOR_28
	repoId = obj->PR_getobj()->NP_IRRepositoryId();
#else
	repoId = obj->_PR_getobj()->_mostDerivedRepoId();
#endif
      }
      CORBA::MarshalObjRef(obj, repoId, strlen(repoId) + 1, stream);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, {name, descriptor}
    {
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
	    marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	  }
	  else {
	    value = PyObject_GetAttr(a_o, name);
	    marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	    Py_DECREF(value);
	  }
	}
      }
      else {
	for (i=0,j=4; i < cnt; i++) {
	  name  = PyTuple_GET_ITEM(d_o, j++);
	  value = PyObject_GetAttr(a_o, name);
	  marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	  Py_DECREF(value);
	}
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

      PyObject* discriminant = PyDict_GetItemString(udict, (char*)"_d");
      PyObject* value        = PyDict_GetItemString(udict, (char*)"_v");
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
					  (char*)"_v");
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

      CORBA::ULong i, len;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  len = PyString_GET_SIZE(a_o);
	  len >>= stream;
	  CORBA::Octet e;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  len = PyString_GET_SIZE(a_o);
	  len >>= stream;
	  CORBA::Char e;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
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

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else if (PyList_Check(a_o)) {
	len = PyList_GET_SIZE(a_o);
	len >>= stream;
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
      }
      else {
	len = PyTuple_GET_SIZE(a_o);
	len >>= stream;
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyTuple_GET_ITEM(a_o, i));
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong i, len;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  len = PyString_GET_SIZE(a_o);
	  CORBA::Octet e;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  len = PyString_GET_SIZE(a_o);
	  CORBA::Char e;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
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

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else if (PyList_Check(a_o)) {
	len = PyList_GET_SIZE(a_o);
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
      }
      else {
	len = PyTuple_GET_SIZE(a_o);
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyTuple_GET_ITEM(a_o, i));
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
    omniORB::log << "!!! marshalArguments(): unsupported typecode: "
		 << (CORBA::ULong)tk << "\n";
    omniORB::log.flush();
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

  case CORBA::tk_null:
  case CORBA::tk_void:
    break;

  case CORBA::tk_short:
    {
      CORBA::Short s = PyInt_AS_LONG(a_o);
      s >>= stream;
    }
    break;

  case CORBA::tk_long:
    {
      CORBA::Long l = PyInt_AS_LONG(a_o);
      l >>= stream;
    }
    break;

  case CORBA::tk_ushort:
    {
      CORBA::UShort us = PyInt_AS_LONG(a_o);
      us >>= stream;
    }
    break;

  case CORBA::tk_ulong:
    {
      CORBA::ULong ul = 0; // Initialised to stop egcs complaining

      if (PyLong_Check(a_o))
	ul = PyLong_AsUnsignedLong(a_o);
      else // It's an int
	ul = PyInt_AS_LONG(a_o);

      ul >>= stream;
    }
    break;

  case CORBA::tk_float:
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
    break;

  case CORBA::tk_double:
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
    break;

  case CORBA::tk_boolean:
    {
      CORBA::Boolean b = PyInt_AS_LONG(a_o) ? 1:0;
      b >>= stream;
    }
    break;

  case CORBA::tk_char:
    {
      char *str = PyString_AS_STRING(a_o);

      CORBA::Char c = str[0];
      c >>= stream;
    }
    break;

  case CORBA::tk_octet:
    {
      CORBA::Octet o = PyInt_AS_LONG(a_o);
      o >>= stream;
    }
    break;

  case CORBA::tk_any:
    {
      PyObject* adict = ((PyInstanceObject*)a_o)->in_dict;

      // TypeCode
      t_o             = PyDict_GetItemString(adict, (char*)"_t");
      PyObject* tdict = ((PyInstanceObject*)t_o)->in_dict;
      PyObject* desc  = PyDict_GetItemString(tdict, (char*)"_d");
      marshalTypeCode(stream, desc);

      // Any's contents
      t_o             = PyDict_GetItemString(adict, (char*)"_v");
      marshalPyObject(stream, desc, t_o);
    }
    break;

  case CORBA::tk_TypeCode:
    {
      PyObject* tdict = ((PyInstanceObject*)a_o)->in_dict;
      t_o             = PyDict_GetItemString(tdict, (char*)"_d");
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
#ifdef OMNIORBPY_FOR_28
	repoId = obj->PR_getobj()->NP_IRRepositoryId();
#else
	repoId = obj->_PR_getobj()->_mostDerivedRepoId();
#endif
      }
      CORBA::MarshalObjRef(obj, repoId, strlen(repoId) + 1, stream);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, {name, descriptor}
    {
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
	    marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	  }
	  else {
	    value = PyObject_GetAttr(a_o, name);
	    marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	    Py_DECREF(value);
	  }
	}
      }
      else {
	for (i=0,j=4; i < cnt; i++) {
	  name  = PyTuple_GET_ITEM(d_o, j++);
	  value = PyObject_GetAttr(a_o, name);
	  marshalPyObject(stream, PyTuple_GET_ITEM(d_o, j++), value);
	  Py_DECREF(value);
	}
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

      PyObject* discriminant = PyDict_GetItemString(udict, (char*)"_d");
      PyObject* value        = PyDict_GetItemString(udict, (char*)"_v");
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
					  (char*)"_v");
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

      CORBA::ULong i, len;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  len = PyString_GET_SIZE(a_o);
	  len >>= stream;
	  CORBA::Octet e;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  len = PyString_GET_SIZE(a_o);
	  len >>= stream;
	  CORBA::Char e;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
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

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else if (PyList_Check(a_o)) {
	len = PyList_GET_SIZE(a_o);
	len >>= stream;
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
      }
      else {
	len = PyTuple_GET_SIZE(a_o);
	len >>= stream;
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyTuple_GET_ITEM(a_o, i));
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong i, len;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  len = PyString_GET_SIZE(a_o);
	  CORBA::Octet e;
	  CORBA::Octet *l = (CORBA::Octet*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  len = PyString_GET_SIZE(a_o);
	  CORBA::Char e;
	  CORBA::Char *l = (CORBA::Char*)PyString_AS_STRING(a_o);
	  for (i=0; i < len; i++) {
	    e = l[i];
	    e >>= stream;
	  }
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

	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else if (PyList_Check(a_o)) {
	len = PyList_GET_SIZE(a_o);
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyList_GET_ITEM(a_o, i));
      }
      else {
	len = PyTuple_GET_SIZE(a_o);
	for (i=0; i < len; i++)
	  marshalPyObject(stream, elm_desc, PyTuple_GET_ITEM(a_o, i));
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
    omniORB::log << "!!! marshalArguments(): unsupported typecode: "
		 << (CORBA::ULong)tk << "\n";
    omniORB::log.flush();
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

  case CORBA::tk_null:
  case CORBA::tk_void:
    break;

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
      // TypeCode
      PyObject* desc     = unmarshalTypeCode(stream);
      PyObject* argtuple = PyTuple_New(1);
      PyTuple_SET_ITEM(argtuple, 0, desc);
      PyObject* tcobj    = PyEval_CallObject(pyCreateTypeCode, argtuple);

      // Value
      t_o = unmarshalPyObject(stream, desc);

      // This decref may delete the descriptor object, in the case
      // that an identical one was already available. That's why it
      // comes after the call to unmarshal the any's value.
      Py_DECREF(argtuple);

      argtuple = PyTuple_New(2);
      PyTuple_SET_ITEM(argtuple, 0, tcobj);
      PyTuple_SET_ITEM(argtuple, 1, t_o);

      r_o = PyEval_CallObject(pyCORBAAnyClass, argtuple);
      Py_DECREF(argtuple);
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
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);

      const char* targetRepoId;

      if (t_o == Py_None)
	targetRepoId = 0;
      else {
	OMNIORB_ASSERT(PyString_Check(t_o));
	targetRepoId = PyString_AS_STRING(t_o);
	if (targetRepoId[0] == '\0') { // Empty string => CORBA.Object
#ifdef OMNIORBPY_FOR_28
	  targetRepoId = 0;
#else
	  targetRepoId = CORBA::Object::_PD_repoId;
#endif
	}
      }

      CORBA::Object_ptr obj = omniPy::UnMarshalObjRef(targetRepoId,
						      stream);
      r_o = createPyCorbaObjRef(targetRepoId, obj);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, name, descriptor, ...
    {
      OMNIORB_ASSERT(tup);
      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyClass_Check(strclass));

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
      OMNIORB_ASSERT(tup);
      PyObject* unclass = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyClass_Check(unclass));

      t_o = PyTuple_GET_ITEM(d_o, 4);

      PyObject* discriminant = unmarshalPyObject(stream, t_o);
      PyObject* value;
      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	OMNIORB_ASSERT(PyTuple_Check(t_o));
	value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  OMNIORB_ASSERT(PyTuple_Check(t_o));
	  value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
	}
	else {
	  Py_INCREF(Py_None);
	  value = Py_None;
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
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 3);

      OMNIORB_ASSERT(PyTuple_Check(t_o));

      CORBA::ULong e;
      e <<= stream;

      if (e >= (CORBA::ULong)PyTuple_GET_SIZE(t_o)) throw CORBA::MARSHAL();

      PyObject* ev = PyTuple_GET_ITEM(t_o, e);
      Py_INCREF(ev);
      r_o = ev;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      OMNIORB_ASSERT(tup);
      t_o                 = PyTuple_GET_ITEM(d_o, 1);

      OMNIORB_ASSERT(PyInt_Check(t_o));

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
      OMNIORB_ASSERT(tup);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      OMNIORB_ASSERT(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::ULong len;
      len <<= stream;

      if (max_len > 0 && len > max_len) throw CORBA::MARSHAL();

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong i;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Octet  e;
	  CORBA::Octet* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Char  e;
	  CORBA::Char* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else {
	  r_o = PyList_New(len);

	  switch(etk) {
	  case CORBA::tk_short:
	    {
	      CORBA::Short e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_long:
	    {
	      CORBA::Long e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ushort:
	    {
	      CORBA::UShort e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ulong:
	    {
	      CORBA::ULong e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_float:
	    {
	      CORBA::Float e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_double:
	    {
	      CORBA::Double e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    {
	      CORBA::Boolean e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;
	    
	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	for (i=0; i < len; i++)
	  PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 2);

      OMNIORB_ASSERT(PyInt_Check(t_o));

      CORBA::ULong len = PyInt_AS_LONG(t_o);

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong i;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Octet  e;
	  CORBA::Octet* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Char  e;
	  CORBA::Char* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else {
	  r_o = PyList_New(len);

	  switch(etk) {
	  case CORBA::tk_short:
	    {
	      CORBA::Short e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_long:
	    {
	      CORBA::Long e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ushort:
	    {
	      CORBA::UShort e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ulong:
	    {
	      CORBA::ULong e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_float:
	    {
	      CORBA::Float e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_double:
	    {
	      CORBA::Double e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    {
	      CORBA::Boolean e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;
	    
	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	for (i=0; i < len; i++)
	  PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
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
      OMNIORB_ASSERT(tup);

      // Throw away the repoId. By the time we get here, we already
      // know it.
      { 
	CORBA::String_member str_tmp;
	str_tmp <<= stream;
      }

      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyClass_Check(strclass));

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
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyList_Check(t_o));

      r_o = unmarshalPyObject(stream, PyList_GET_ITEM(t_o, 0));
    }
    break;

  default:
    omniORB::log << " !!! unmarshalReturnedValues(): unsupported typecode: "
		 << (CORBA::ULong)tk << "\n";
    omniORB::log.flush();
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

  case CORBA::tk_null:
  case CORBA::tk_void:
    break;

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
      // TypeCode
      PyObject* desc     = unmarshalTypeCode(stream);
      PyObject* argtuple = PyTuple_New(1);
      PyTuple_SET_ITEM(argtuple, 0, desc);
      PyObject* tcobj    = PyEval_CallObject(pyCreateTypeCode, argtuple);

      // Value
      t_o = unmarshalPyObject(stream, desc);

      // This decref may delete the descriptor object, in the case
      // that an identical one was already available. That's why it
      // comes after the call to unmarshal the any's value.
      Py_DECREF(argtuple);

      argtuple = PyTuple_New(2);
      PyTuple_SET_ITEM(argtuple, 0, tcobj);
      PyTuple_SET_ITEM(argtuple, 1, t_o);

      r_o = PyEval_CallObject(pyCORBAAnyClass, argtuple);
      Py_DECREF(argtuple);
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
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);

      const char* targetRepoId;

      if (t_o == Py_None)
	targetRepoId = 0;
      else {
	OMNIORB_ASSERT(PyString_Check(t_o));
	targetRepoId = PyString_AS_STRING(t_o);
	if (targetRepoId[0] == '\0') { // Empty string => CORBA.Object
#ifdef OMNIORBPY_FOR_28
	  targetRepoId = 0;
#else
	  targetRepoId = CORBA::Object::_PD_repoId;
#endif
	}
      }

      CORBA::Object_ptr obj = omniPy::UnMarshalObjRef(targetRepoId,
						      stream);
      r_o = createPyCorbaObjRef(targetRepoId, obj);
    }
    break;

  case CORBA::tk_struct: // class, repoId, struct name, name, descriptor, ...
    {
      OMNIORB_ASSERT(tup);
      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyClass_Check(strclass));

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
      OMNIORB_ASSERT(tup);
      PyObject* unclass = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyClass_Check(unclass));

      t_o = PyTuple_GET_ITEM(d_o, 4);

      PyObject* discriminant = unmarshalPyObject(stream, t_o);
      PyObject* value;
      PyObject* cdict = PyTuple_GET_ITEM(d_o, 8);

      t_o = PyDict_GetItem(cdict, discriminant);
      if (t_o) {
	// Discriminant found in case dictionary
	OMNIORB_ASSERT(PyTuple_Check(t_o));
	value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
      }
      else {
	// Is there a default case?
	t_o = PyTuple_GET_ITEM(d_o, 7);
	if (t_o != Py_None) {
	  OMNIORB_ASSERT(PyTuple_Check(t_o));
	  value = unmarshalPyObject(stream, PyTuple_GET_ITEM(t_o, 2));
	}
	else {
	  Py_INCREF(Py_None);
	  value = Py_None;
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
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 3);

      OMNIORB_ASSERT(PyTuple_Check(t_o));

      CORBA::ULong e;
      e <<= stream;

      if (e >= (CORBA::ULong)PyTuple_GET_SIZE(t_o)) throw CORBA::MARSHAL();

      PyObject* ev = PyTuple_GET_ITEM(t_o, e);
      Py_INCREF(ev);
      r_o = ev;
    }
    break;

  case CORBA::tk_string: // max_length
    {
      OMNIORB_ASSERT(tup);
      t_o                 = PyTuple_GET_ITEM(d_o, 1);

      OMNIORB_ASSERT(PyInt_Check(t_o));

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
      OMNIORB_ASSERT(tup);
      t_o                  = PyTuple_GET_ITEM(d_o, 2);

      OMNIORB_ASSERT(PyInt_Check(t_o));

      CORBA::ULong max_len = PyInt_AS_LONG(t_o);

      CORBA::ULong len;
      len <<= stream;

      if (max_len > 0 && len > max_len) throw CORBA::MARSHAL();

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong i;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Octet  e;
	  CORBA::Octet* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Char  e;
	  CORBA::Char* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else {
	  r_o = PyList_New(len);

	  switch(etk) {
	  case CORBA::tk_short:
	    {
	      CORBA::Short e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_long:
	    {
	      CORBA::Long e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ushort:
	    {
	      CORBA::UShort e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ulong:
	    {
	      CORBA::ULong e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_float:
	    {
	      CORBA::Float e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_double:
	    {
	      CORBA::Double e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    {
	      CORBA::Boolean e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;
	    
	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	for (i=0; i < len; i++)
	  PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
      }
    }
    break;

  case CORBA::tk_array: // element_desc, length
    {
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 2);

      OMNIORB_ASSERT(PyInt_Check(t_o));

      CORBA::ULong len = PyInt_AS_LONG(t_o);

      PyObject* elm_desc = PyTuple_GET_ITEM(d_o, 1);

      CORBA::ULong i;

      if (PyInt_Check(elm_desc)) {
	CORBA::ULong etk = PyInt_AS_LONG(elm_desc);

	if (etk == CORBA::tk_octet) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Octet  e;
	  CORBA::Octet* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else if (etk == CORBA::tk_char) {
	  r_o = PyString_FromStringAndSize(0, len);
	  CORBA::Char  e;
	  CORBA::Char* l = (CORBA::Octet*)PyString_AS_STRING(r_o);
	  for (i=0; i < len; i++) {
	    e <<= stream;
	    l[i] = e;
	  }
	}
	else {
	  r_o = PyList_New(len);

	  switch(etk) {
	  case CORBA::tk_short:
	    {
	      CORBA::Short e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_long:
	    {
	      CORBA::Long e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ushort:
	    {
	      CORBA::UShort e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_ulong:
	    {
	      CORBA::ULong e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyLong_FromUnsignedLong(e));
	      }
	    }
	    break;

	  case CORBA::tk_float:
	    {
	      CORBA::Float e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_double:
	    {
	      CORBA::Double e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyFloat_FromDouble(e));
	      }
	    }
	    break;

	  case CORBA::tk_boolean:
	    {
	      CORBA::Boolean e;
	      for (i=0; i < len; i++) {
		e <<= stream;
		PyList_SET_ITEM(r_o, i, PyInt_FromLong(e));
	      }
	    }
	    break;
	    
	  default:
	    OMNIORB_ASSERT(0);
	  }
	}
      }
      else {
	r_o = PyList_New(len);

	for (i=0; i < len; i++)
	  PyList_SET_ITEM(r_o, i, unmarshalPyObject(stream, elm_desc));
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
      OMNIORB_ASSERT(tup);

      // Throw away the repoId. By the time we get here, we already
      // know it.
      { 
	CORBA::String_member str_tmp;
	str_tmp <<= stream;
      }

      PyObject* strclass = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyClass_Check(strclass));

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
      OMNIORB_ASSERT(tup);
      t_o = PyTuple_GET_ITEM(d_o, 1);
      OMNIORB_ASSERT(PyList_Check(t_o));

      r_o = unmarshalPyObject(stream, PyList_GET_ITEM(t_o, 0));
    }
    break;

  default:
    omniORB::log << " !!! unmarshalReturnedValues(): unsupported typecode: "
		 << (CORBA::ULong)tk << "\n";
    omniORB::log.flush();
    abort();
  }
  return r_o;
}



static inline
PyObject*
setPyBadParam(CORBA::CompletionStatus compstatus)
{
  CORBA::BAD_PARAM ex(0,compstatus);
  omniPy::handleSystemException(ex);
  return 0;
}


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
      if (a_o == Py_None) {
	Py_INCREF(a_o); return a_o;
      }
      else return setPyBadParam(compstatus);
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

      if (a_o == Py_None) {
	Py_INCREF(Py_None); return Py_None;
      }
      else if (PyInstance_Check(a_o)) {
	CORBA::Object_ptr obj = (CORBA::Object_ptr)getTwin(a_o, OBJREF_TWIN);
	if (!obj) return setPyBadParam(compstatus);

	Py_INCREF(a_o); return a_o;
      }
      else
	return setPyBadParam(compstatus);
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

      if (PyInt_Check(elm_desc)) { // Simple type
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

	  int           valid = 1;
	  long          long_val;
	  unsigned long ulong_val;

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
		else {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0;
		  continue;
		}
	      }
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

	  int           valid = 1;
	  long          long_val;
	  unsigned long ulong_val;

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
		else {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0;
		  continue;
		}
	      }
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

      if (PyInt_Check(elm_desc)) { // Simple type
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

	  int           valid = 1;
	  long          long_val;
	  unsigned long ulong_val;

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
		else {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0;
		  continue;
		}
	      }
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

	  int           valid = 1;
	  long          long_val;
	  unsigned long ulong_val;

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
		else {
		  Py_INCREF(Py_None); PyList_SET_ITEM(r_o, i, Py_None);
		  valid = 0;
		  continue;
		}
	      }
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
    OMNIORB_ASSERT(0);
  }
  // Never reach here
  OMNIORB_ASSERT(0);
  return 0;
}
