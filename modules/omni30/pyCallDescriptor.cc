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
// Revision 1.13  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//

#include <omnipy.h>


CORBA::ULong
omniPy::Py_omniCallDescriptor::alignedSize(CORBA::ULong msgsize)
{
  // alignedSize() is called with the interpreter lock
  // released. Reacquire it so we can touch the descriptor objects
  // safely
  reacquireInterpreterLock();

  for (int i=0; i < in_l_; i++)
    msgsize = omniPy::alignedSize(msgsize,
				  PyTuple_GET_ITEM(in_d_,i),
				  PyTuple_GET_ITEM(args_,i),
				  CORBA::COMPLETED_NO);
  return msgsize;
}


void
omniPy::Py_omniCallDescriptor::marshalArguments(GIOP_C& giop_client)
{
  // We should always hold the interpreter lock when entering this
  // function. The call to releaseInterpreterLock() will assert that
  // this is true. It's very unlikely that we will crash in the
  // mean-time if something has gone wrong.

  for (int i=0; i < in_l_; i++)
    omniPy::marshalPyObject(giop_client,
			    PyTuple_GET_ITEM(in_d_,i),
			    PyTuple_GET_ITEM(args_,i));
  releaseInterpreterLock();
}


void
omniPy::Py_omniCallDescriptor::unmarshalReturnedValues(GIOP_C& giop_client)
{
  reacquireInterpreterLock();

  OMNIORB_ASSERT(out_l_ >= 0);
  // out_l_ == -1 if it's a oneway operation, but we should never
  // reach here if that is the case.

  if (out_l_ == 0) {
    Py_INCREF(Py_None);
    result_ = Py_None;
  }
  else if (out_l_ == 1)
    result_ = omniPy::unmarshalPyObject(giop_client,
					PyTuple_GET_ITEM(out_d_, 0));
  else {
    result_ = PyTuple_New(out_l_);
    if (!result_) throw CORBA::NO_MEMORY();

    for (int i=0; i < out_l_; i++) {
      PyTuple_SET_ITEM(result_, i,
		       omniPy::unmarshalPyObject(giop_client,
						 PyTuple_GET_ITEM(out_d_, i)));
    }
  }
}


void
omniPy::Py_omniCallDescriptor::userException(GIOP_C&     giop_client,
					     const char* repoId)
{
  reacquireInterpreterLock();

  PyObject* d_o = PyDict_GetItemString(exc_d_, (char*)repoId);

  if (d_o) { // class, repoId, exc name, name, descriptor, ...

    // Can't use normal exception unmarshalling code, since the repoId
    // has already been unmarshalled.

    PyObject* excclass = PyTuple_GET_ITEM(d_o, 1);
    OMNIORB_ASSERT(PyClass_Check(excclass));

    int       cnt      = (PyTuple_GET_SIZE(d_o) - 4) / 2;
    PyObject* exctuple = PyTuple_New(cnt);

    int i, j;
    for (i=0, j=5; i < cnt; i++, j+=2) {
      PyTuple_SET_ITEM(exctuple, i,
		       unmarshalPyObject(giop_client,
					 PyTuple_GET_ITEM(d_o, j)));
    }
    PyObject* exc_i = PyEval_CallObject(excclass, exctuple);
    Py_DECREF(exctuple);

    PyErr_SetObject(excclass, exc_i);
    throw UserExceptionHandled();
  }
  else {
    giop_client.RequestCompleted(1);
    throw CORBA::MARSHAL(0, CORBA::COMPLETED_MAYBE);
  }
}


void
omniPy::Py_localCallBackFunction(omniCallDescriptor* cd, omniServant* svnt)
{
  Py_omniCallDescriptor* pycd = (Py_omniCallDescriptor*)cd;
  Py_omniServant*        pyos =
    (Py_omniServant*)svnt->_ptrToInterface("Py_omniServant");

  pycd->reacquireInterpreterLock();

  // *** TODO: Copy args which would otherwise have reference semantics

  pycd->result_ = pyos->local_dispatch(pycd->op(), pycd->args_);

  // invokeOp() expects us to come back from oneways with the
  // interpreter unlocked
  if (pycd->is_oneway())
    pycd->releaseInterpreterLock();
}
