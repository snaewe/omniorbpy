// -*- c++ -*-

// Master header file for omnipy internals

// $Id$

// $Log$
// Revision 1.2  1999/07/19 15:52:07  dpg1
// Things moved into module omniPy.
// Twin type expanded so it can be put into dictionaries.
//
// Revision 1.1  1999/06/07 10:10:31  dpg1
// Initial revision
//

#ifndef _omnipy_h_
#define _omnipy_h_

#include <omniORB2/CORBA.h>
#include <omniORB2/proxyCall.h>

#include <python1.5/Python.h>

#include <iostream.h>

#define OMNIPY_MAJOR 0
#define OMNIPY_MINOR 1


_CORBA_MODULE omniPy
_CORBA_MODULE_BEG


////////////////////////////////////////////////////////////////////////////
// Data structures and functions to manage C++ twins of Python objects    //
////////////////////////////////////////////////////////////////////////////

struct omnipyTwin {
  PyObject_VAR_HEAD
  void* ob_twin;
};

extern "C" {

  static void
  omnipyTwin_dealloc(omnipyTwin* tp)
  {
    PyMem_DEL(tp);
  }

  static int
  omnipyTwin_cmp(omnipyTwin* t1, omnipyTwin* t2)
  {
    // Can't use simple t1->ob_twin - t2->ob_twin in case int is
    // smaller than a pointer
    if      (t1->ob_twin == t2->ob_twin) return 0;
    else if (t1->ob_twin >  t2->ob_twin) return 1;
    else                                 return -1;
  }

  static long
  omnipyTwin_hash(omnipyTwin* tp)
  {
    return (long)tp->ob_twin;
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
      (cmpfunc)omnipyTwin_cmp,          /*tp_compare*/
      0,                                /*tp_repr*/
      0,				/*tp_as_number*/
      0,                                /*tp_as_sequence*/
      0,				/*tp_as_mapping*/
      (hashfunc)omnipyTwin_hash,	/*tp_hash*/
      0,				/*tp_call*/
      0,				/*tp_str*/
      0,				/*tp_getattro*/
      0,				/*tp_setattro*/
      0,                                /*tp_as_buffer*/
      0,				/*tp_xxx4*/
      0,                                /*tp_doc*/
      };
}

_CORBA_MODULE_FN
inline PyObject*
newTwin(void* twin)
{
  omnipyTwin* ot = PyMem_NEW(omnipyTwin, 1);
  ot->ob_type = &omnipyTwinType;
  ot->ob_size = 0;
  ot->ob_twin = (void*)twin;
  _Py_NewReference(ot);
  return (PyObject*)ot;
}


_CORBA_MODULE_FN
inline void
setTwin(PyObject* obj, void* twin)
{
  PyObject* ot = newTwin(twin);

  PyDict_SetItemString(((PyInstanceObject*)obj)->in_dict,
		       "_twin", ot);
  Py_DECREF(ot);
}

_CORBA_MODULE_FN
inline void*
getTwin(PyObject* obj)
{
  PyObject* ot = PyDict_GetItemString(((PyInstanceObject*)obj)->in_dict,
				      "_twin");
  if (ot)
    return ((omnipyTwin*)ot)->ob_twin;
  else
    return 0;
}

_CORBA_MODULE_FN
inline void
remTwin(PyObject* obj)
{
  PyDict_DelItemString(((PyInstanceObject*)obj)->in_dict, "_twin");
}


////////////////////////////////////////////////////////////////////////////
// Global pointers to Python objects                                      //
////////////////////////////////////////////////////////////////////////////

_CORBA_MODULE_VAR PyObject* pyCORBAmodule;	// The CORBA module
_CORBA_MODULE_VAR PyObject* pyCORBAsysExcMap;	//  The system exception map
_CORBA_MODULE_VAR PyObject* pyCORBAnilObject;	//  The nil object
_CORBA_MODULE_VAR PyObject* pyCORBAAnyClass;    //  Any class
_CORBA_MODULE_VAR PyObject* pyomniORBmodule;	// The omniORB module
_CORBA_MODULE_VAR PyObject* pyomniORBproxyMap;	//  The proxy object class map
_CORBA_MODULE_VAR PyObject* pyomniORBtypeMap;   //  Type map
_CORBA_MODULE_VAR PyObject* pyCreateTypeCode;	// Function to create a
						//  TypeCode object



////////////////////////////////////////////////////////////////////////////
// Utility functions                                                      //
////////////////////////////////////////////////////////////////////////////

// Create the Python object relating to a CORBA object:
_CORBA_MODULE_FN
PyObject* createPyCorbaObject(const char* targetRepoId,
			      const CORBA::Object_ptr obj);

// Set the Python execution state to handle a system exception:
_CORBA_MODULE_FN
void handleSystemException(const CORBA::SystemException& ex);



////////////////////////////////////////////////////////////////////////////
// Recursive marshalling functions                                        //
////////////////////////////////////////////////////////////////////////////

// Calculate aligned size of the given argument object a_o, which has
// the type specified by d_o. Assumes that argument types are correct,
// unless checking is necessary.
_CORBA_MODULE_FN
CORBA::ULong alignedSize(CORBA::ULong msgsize,
			 PyObject* d_o,
			 PyObject* a_o);

// Check the type and marshal the given argument object a_o, which
// has the type specified by d_o.
_CORBA_MODULE_FN
void marshalPyObject(NetBufferedStream& stream,
		     PyObject*          d_o,
		     PyObject*          a_o);

_CORBA_MODULE_FN
void marshalPyObject(MemBufferedStream& stream,
		     PyObject*          d_o,
		     PyObject*          a_o);

// Unmarshal a PyObject, which has the type specified by d_o.
_CORBA_MODULE_FN
PyObject* unmarshalPyObject(NetBufferedStream& stream,
			    PyObject*          d_o);

_CORBA_MODULE_FN
PyObject* unmarshalPyObject(MemBufferedStream& stream,
			    PyObject*          d_o);


////////////////////////////////////////////////////////////////////////////
// TypeCode and Any support functions                                     //
////////////////////////////////////////////////////////////////////////////

// Marshalled size of a TypeCode given as a descriptor:
_CORBA_MODULE_FN
CORBA::ULong alignedSizeTypeCode(CORBA::ULong msgsize, PyObject* d_o);


// Marshal a type descriptor as a TypeCode:
_CORBA_MODULE_FN
void marshalTypeCode(NetBufferedStream& stream, PyObject* d_o);

_CORBA_MODULE_FN
void marshalTypeCode(MemBufferedStream& stream, PyObject* d_o);


// Unmarshal a TypeCode, returning a descriptor:
_CORBA_MODULE_FN
PyObject* unmarshalTypeCode(NetBufferedStream& stream);

_CORBA_MODULE_FN
PyObject* unmarshalTypeCode(MemBufferedStream& stream);



////////////////////////////////////////////////////////////////////////////
// Proxy call descriptor object                                           //
////////////////////////////////////////////////////////////////////////////

class Py_OmniProxyCallDesc : public OmniProxyCallDesc {
public:

  Py_OmniProxyCallDesc(const char* op, size_t len, CORBA::Boolean has_exc,
		       PyObject* in_d, PyObject* out_d, PyObject* exc_d,
		       PyObject* args)
    : OmniProxyCallDesc(op, len, has_exc),
      in_d_(in_d),
      out_d_(out_d),
      exc_d_(exc_d),
      args_(args)
  {
    tstate_ = 0;
    in_l_   = PyTuple_GET_SIZE(in_d_);
    out_l_  = PyTuple_GET_SIZE(out_d_);
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
  PyObject*      in_d_;
  int            in_l_;
  PyObject*      out_d_;
  int            out_l_;
  PyObject*      exc_d_;
  PyObject*      args_;
  PyObject*      result_;
  PyThreadState* tstate_;
};



_CORBA_MODULE_END

#endif // _omnipy_h_
