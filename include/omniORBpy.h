// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// omniORBpy.h                Created on: 2002/05/25
//                            Author    : Duncan Grisby (dgrisby)
//
//    Copyright (C) 2002 Duncan Grisby
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
// Description:
//    Header for the C++ API to omniORBpy

#ifndef _omniORBpy_h_
#define _omniORBpy_h_

// The file including this file must include the correct Python.h
// header. This file does not include it, to avoid difficulties with
// its name.

#include <omniORB4/CORBA.h>

// The omniORBpy C++ API consists of a singleton structure containing
// function pointers. A pointer to the API struct is stored as a
// PyCObject in the _omnipy module with the name API. Access it with
// code like:
//
//      PyObject*     omnipy = PyImport_ImportModule((char*)"_omnipy");
//      PyObject*     pyapi  = PyObject_GetAttrString(omnipy, (char*)"API");
//      omniORBpyAPI* api    = (omniORBpyAPI*)PyCObject_AsVoidPtr(pyapi);
//      Py_DECREF(pyapi);
//
// Obviously, you MUST NOT modify the function pointers!
//
// This arrangement of things means you do not have to link to the
// _omnipymodule library to be able to use the API.


struct omniORBpyAPI {

  PyObject* (*cxxObjRefToPyObjRef)(const CORBA::Object_ptr cxx_obj,
				   CORBA::Boolean hold_lock);
  // Convert a C++ object reference to a Python object reference.
  // If <hold_lock> is true, caller holds the Python interpreter lock.

  CORBA::Object_ptr (*pyObjRefToCxxObjRef)(PyObject* py_obj,
					   CORBA::Boolean hold_lock);
  // Convert a Python object reference to a C++ object reference.
  // Raises BAD_PARAM if the Python object is not an object reference.
  // If <hold_lock> is true, caller holds the Python interpreter lock.


  omniORBpyAPI();
  // Constructor for the singleton. Sets up the function pointers.
};


// Extensions to omniORB / omniORBpy may create their own pseudo
// object reference types. To provide a Python mapping for these, a
// function must be provided that takes a CORBA::Object_ptr and
// returns a suitable PyObject. Functions are registered by appending
// PyCObjects to the list _omnipy.pseudoFns. The CObjects must contain
// pointers to functions with this signature:

typedef PyObject* (*omniORBpyPseudoFn)(const CORBA::Object_ptr);



#endif // _omniORBpy_h_
