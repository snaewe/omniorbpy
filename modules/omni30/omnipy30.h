// -*- c++ -*-

// Master header file for omnipy internals

// $Id$

// $Log$
// Revision 1.1  1999/06/07 10:10:31  dpg1
// Initial revision
//

#ifndef _omnipy_h_
#define _omnipy_h_

#include <omniORB2/CORBA.h>

#include <python1.5/Python.h>


#define OMNIPY_MAJOR 1
#define OMNIPY_MINOR 0


////////////////////////////////////////////////////////////////////////////
// Useful functions                                                       //
////////////////////////////////////////////////////////////////////////////

PyObject* createPyCorbaObject(const char* targetRepoId,
			      const CORBA::Object_ptr obj);

void handleSystemException(const CORBA::SystemException& ex);




////////////////////////////////////////////////////////////////////////////
// Pointers to Python objects                                             //
////////////////////////////////////////////////////////////////////////////

extern PyObject* pyCORBAmodule;	      // The CORBA module
extern PyObject* pyCORBAmoduleDict;   //  Its dictionary
extern PyObject* pyCORBAsysExcMap;    //  The system exception map
extern PyObject* pyomniORBmodule;     // The omniORB module
extern PyObject* pyomniORBmoduleDict; //  Its dictionary
extern PyObject* pyomniORBproxyMap;   //  The proxy object class map


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

inline void
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

inline void
remTwin(PyObject* obj)
{
  PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)obj)->in_dict,
				      "_twin");
  Py_DECREF(ot);
}


#endif // _omnipy_h_
