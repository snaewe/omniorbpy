// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyProxyCallWrapper.cc      Created on: 1999/06/07
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
//    Implementation of Python proxy call wrapper object


// $Id$

// $Log$
// Revision 1.11  1999/09/29 09:05:03  dpg1
// Now releases the Python interpreter lock before invoke's call to
// _is_a().
//
// Revision 1.10  1999/09/28 14:23:30  dpg1
// Fixed some bugs in handling the Python interpreter lock.
//
// Revision 1.9  1999/09/24 09:22:02  dpg1
// Added copyright notices.
//
// Revision 1.8  1999/09/20 14:55:37  dpg1
// GCC 2.95 is more pedantic than egcs.
//
// Revision 1.7  1999/07/29 14:20:44  dpg1
// Oneway support. Exception handling modified.
//
// Revision 1.6  1999/07/19 16:49:32  dpg1
// Recursive marshalling functions moved out of proxy call class into
// omniPy module so the can be shared with the server-side.
//
// Revision 1.5  1999/06/23 12:44:28  dpg1
// Descriptors extended to contain all TypeCode-required info.
//
// Revision 1.4  1999/06/10 10:42:59  dpg1
// Marshalling changed to reflect new type mapping.
//
// Revision 1.3  1999/06/08 16:20:47  dpg1
// All types except any.
//
// Revision 1.2  1999/06/07 14:58:20  dpg1
// Descriptors unflattened again.
//
// Revision 1.1  1999/06/07 10:11:02  dpg1
// Initial revision
//

#include <iostream.h>
#include <omnipy.h>


CORBA::ULong
omniPy::Py_OmniProxyCallDesc::alignedSize(CORBA::ULong msgsize)
{
  // alignedSize() is called with the interpreter lock
  // released. Reacquire it so we can touch the descriptor objects
  // safely
  reacquireInterpreterLock();

  for (int i=0; i < in_l_; i++)
    msgsize = omniPy::alignedSize(msgsize,
				  PyTuple_GET_ITEM(in_d_,i),
				  PyTuple_GET_ITEM(args_,i));
  return msgsize;
}


void
omniPy::Py_OmniProxyCallDesc::marshalArguments(GIOP_C& giop_client)
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
omniPy::Py_OmniProxyCallDesc::unmarshalReturnedValues(GIOP_C& giop_client)
{
  reacquireInterpreterLock();

  assert(out_l_ >= 0);
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
omniPy::Py_OmniProxyCallDesc::userException(GIOP_C&     giop_client,
					    const char* repoId)
{
  reacquireInterpreterLock();

  PyObject* d_o = PyDict_GetItemString(exc_d_, (char*)repoId);

  if (d_o) { // class, repoId, exc name, name, descriptor, ...

    // Can't use normal exception unmarshalling code, since the repoId
    // has already been unmarshalled.

    PyObject* excclass = PyTuple_GET_ITEM(d_o, 1);
    assert(PyClass_Check(excclass));

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

    PyErr_SetObject(PyTuple_GET_ITEM(d_o, 1), exc_i);
    throw CORBA::UserException();
  }
  else {
    giop_client.RequestCompleted(1);
    throw CORBA::MARSHAL(0, CORBA::COMPLETED_MAYBE);
  }
}


CORBA::ULong
omniPy::Py_OmniOWProxyCallDesc::alignedSize(CORBA::ULong msgsize)
{
  reacquireInterpreterLock();

  for (int i=0; i < in_l_; i++)
    msgsize = omniPy::alignedSize(msgsize,
				  PyTuple_GET_ITEM(in_d_,i),
				  PyTuple_GET_ITEM(args_,i));
  return msgsize;
}


void
omniPy::Py_OmniOWProxyCallDesc::marshalArguments(GIOP_C& giop_client)
{
  for (int i=0; i < in_l_; i++)
    omniPy::marshalPyObject(giop_client,
			    PyTuple_GET_ITEM(in_d_,i),
			    PyTuple_GET_ITEM(args_,i));
  releaseInterpreterLock();
}
