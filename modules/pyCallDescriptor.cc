// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyCallDescriptor.cc        Created on: 2000/02/02
//                            Author    : Duncan Grisby (dpg1)
//
//    Copyright (C) 2000 AT&T Laboratories Cambridge
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
//    Implementation of Python call descriptor object

// $Id$

// $Log$
// Revision 1.1.2.6  2001/06/29 15:11:12  dpg1
// Fix for clients using GIOP 1.0.
//
// Revision 1.1.2.5  2001/06/29 09:53:56  dpg1
// Fix for clients using GIOP 1.0.
//
// Revision 1.1.2.4  2001/06/01 11:09:26  dpg1
// Make use of new omni::ptrStrCmp() and omni::strCmp().
//
// Revision 1.1.2.3  2001/05/10 15:16:02  dpg1
// Big update to support new omniORB 4 internals.
//
// Revision 1.1.2.2  2001/01/10 12:00:07  dpg1
// Release the Python interpreter lock when doing potentially blocking
// stream calls.
//
// Revision 1.1.2.1  2000/10/13 13:55:24  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>
#include <pyThreadCache.h>
#include <omniORB4/IOP_C.h>


omniPy::Py_omniCallDescriptor::~Py_omniCallDescriptor()
{
  OMNIORB_ASSERT(!tstate_);
  Py_XDECREF(args_);
  Py_XDECREF(result_);
}


void
omniPy::Py_omniCallDescriptor::initialiseCall(cdrStream&)
{
  // initialiseCall() is called with the interpreter lock
  // released. Reacquire it so we can touch the descriptor objects
  // safely
  reacquireInterpreterLock();

  for (int i=0; i < in_l_; i++)
    omniPy::validateType(PyTuple_GET_ITEM(in_d_,i),
			 PyTuple_GET_ITEM(args_,i),
			 CORBA::COMPLETED_NO);

  releaseInterpreterLock();
}


void
omniPy::Py_omniCallDescriptor::marshalArguments(cdrStream& stream)
{
  if (in_marshal_) {
    if (omniORB::trace(1)) {
      omniORB::logger l;
      l <<
	"Warning: omniORBpy marshalArguments re-entered. "
	"Untested code may fail.\n";
    }
    // Re-entered to figure out the size
    for (int i=0; i < in_l_; i++)
      omniPy::marshalPyObject(stream,
			      PyTuple_GET_ITEM(in_d_,i),
			      PyTuple_GET_ITEM(args_,i));
  }
  else {
    reacquireInterpreterLock();

    in_marshal_ = 1;
    PyUnlockingCdrStream pystream(stream);

    for (int i=0; i < in_l_; i++)
      omniPy::marshalPyObject(pystream,
			      PyTuple_GET_ITEM(in_d_,i),
			      PyTuple_GET_ITEM(args_,i));
    in_marshal_ = 0;

    releaseInterpreterLock();
  }
}


void
omniPy::Py_omniCallDescriptor::unmarshalReturnedValues(cdrStream& stream)
{
  if (out_l_ == -1) return;  // Oneway operation

  reacquireInterpreterLock();

  if (out_l_ == 0) {
    Py_INCREF(Py_None);
    result_ = Py_None;
  }
  else {
    PyUnlockingCdrStream pystream(stream);

    if (out_l_ == 1)
      result_ = omniPy::unmarshalPyObject(pystream,
					  PyTuple_GET_ITEM(out_d_, 0));
    else {
      result_ = PyTuple_New(out_l_);
      if (!result_) OMNIORB_THROW(NO_MEMORY,0,CORBA::COMPLETED_MAYBE);

      for (int i=0; i < out_l_; i++) {
	PyTuple_SET_ITEM(result_, i,
			 omniPy::unmarshalPyObject(pystream,
						   PyTuple_GET_ITEM(out_d_,
								    i)));
      }
    }
  }
  releaseInterpreterLock();
}


void
omniPy::Py_omniCallDescriptor::userException(_OMNI_NS(IOP_C)& iop_client,
					     const char* repoId)
{
  reacquireInterpreterLock();

  PyObject* d_o = PyDict_GetItemString(exc_d_, (char*)repoId);

  if (d_o) { // class, repoId, exc name, name, descriptor, ...

    try {
      PyUserException ex(d_o);
      
      cdrStream& stream = iop_client.getStream();

      ex <<= stream;
      ex._raise();
    }
    catch (...) {
      releaseInterpreterLock();
      iop_client.RequestCompleted();
      throw;
    }
    OMNIORB_ASSERT(0); // Never reach here
  }
  else {
    releaseInterpreterLock();
    iop_client.RequestCompleted(1);
    OMNIORB_THROW(MARSHAL, 0, CORBA::COMPLETED_MAYBE);
  }
}


void
omniPy::Py_localCallBackFunction(omniCallDescriptor* cd, omniServant* svnt)
{
  Py_omniCallDescriptor* pycd = (Py_omniCallDescriptor*)cd;
  Py_omniServant*        pyos =
    (Py_omniServant*)svnt->_ptrToInterface(omniPy::string_Py_omniServant);

  // Unfortunately, we can't use the call descriptor's
  // reacquireInterpreterLock() function, since this call-back may be
  // running in a different thread to the creator of the call
  // descriptor.

  if (cd->is_upcall()) {
    omnipyThreadCache::lock _t;
    pyos->remote_dispatch(pycd);
  }
  else {
    omnipyThreadCache::lock _t;
    pyos->local_dispatch(pycd);
  }
}


void
omniPy::Py_omniCallDescriptor::unmarshalArguments(cdrStream& stream)
{
  OMNIORB_ASSERT(args_ == 0);

  omnipyThreadCache::lock _t;

  args_ = PyTuple_New(in_l_);

  PyUnlockingCdrStream pystream(stream);

  for (int i=0; i < in_l_; i++) {
    PyTuple_SET_ITEM(args_, i,
		     omniPy::unmarshalPyObject(pystream,
					       PyTuple_GET_ITEM(in_d_, i)));
  }
}

void
omniPy::Py_omniCallDescriptor::setAndValidateReturnedValues(PyObject* result)
{
  OMNIORB_ASSERT(result_ == 0);
  result_ = result;

  if (out_l_ == -1 || out_l_ == 0) {
    if (result_ != Py_None)
      OMNIORB_THROW(BAD_PARAM, 0, CORBA::COMPLETED_MAYBE);
  }

  if (out_l_ == 1) {
    omniPy::validateType(PyTuple_GET_ITEM(out_d_,0),
			 result,
			 CORBA::COMPLETED_MAYBE);
  }
  else {
    for (int i=0; i < out_l_; i++) {
      omniPy::validateType(PyTuple_GET_ITEM(out_d_,i),
			   PyTuple_GET_ITEM(result,i),
			   CORBA::COMPLETED_MAYBE);
    }
  }
}

void
omniPy::Py_omniCallDescriptor::marshalReturnedValues(cdrStream& stream)
{
  omnipyThreadCache::lock _t;
  PyUnlockingCdrStream pystream(stream);

  if (out_l_ == 1) {
    omniPy::marshalPyObject(pystream,
			    PyTuple_GET_ITEM(out_d_, 0),
			    result_);
  }
  else {
    for (int i=0; i < out_l_; i++) {
      omniPy::marshalPyObject(pystream,
			      PyTuple_GET_ITEM(out_d_,i),
			      PyTuple_GET_ITEM(result_,i));
    }
  }
}