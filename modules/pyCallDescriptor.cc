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
// Revision 1.1.2.2  2001/01/10 12:00:07  dpg1
// Release the Python interpreter lock when doing potentially blocking
// stream calls.
//
// Revision 1.1.2.1  2000/10/13 13:55:24  dpg1
// Initial support for omniORB 4.
//

#include <omnipy.h>
#include <iostream.h>


class PyUnlockingCdrStream : public cdrStreamAdapter {
public:
  PyUnlockingCdrStream(cdrStream& stream, omniPy::Py_omniCallDescriptor* cd)
    : cdrStreamAdapter(stream), callDescriptor_(cd)
  {
  }

  ~PyUnlockingCdrStream() { }

  // Override virtual functions in cdrStreamAdapter...
  void put_octet_array(const _CORBA_Octet* b, int size,
		       omni::alignment_t align=omni::ALIGN_1);
  void get_octet_array(_CORBA_Octet* b,int size,
		       omni::alignment_t align=omni::ALIGN_1);
  void skipInput(_CORBA_ULong size);
  void fetchInputData(omni::alignment_t,size_t);
  _CORBA_Boolean reserveOutputSpace(omni::alignment_t,size_t);

private:
  omniPy::Py_omniCallDescriptor* callDescriptor_;
};


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
}


void
omniPy::Py_omniCallDescriptor::marshalArguments(cdrStream& giop_client)
{
  // We should always hold the interpreter lock when entering this
  // function. The call to releaseInterpreterLock() will assert that
  // this is true. It's very unlikely that we will crash in the
  // mean-time if something has gone wrong.

  if (in_marshal_) {
    // Re-entered to figure out the size
    for (int i=0; i < in_l_; i++)
      omniPy::marshalPyObject(giop_client,
			      PyTuple_GET_ITEM(in_d_,i),
			      PyTuple_GET_ITEM(args_,i));
  }
  else {
    in_marshal_ = 1;
    PyUnlockingCdrStream pystream(giop_client, this);

    for (int i=0; i < in_l_; i++)
      omniPy::marshalPyObject(pystream,
			      PyTuple_GET_ITEM(in_d_,i),
			      PyTuple_GET_ITEM(args_,i));
    in_marshal_ = 0;
    releaseInterpreterLock();
  }
}


void
omniPy::Py_omniCallDescriptor::unmarshalReturnedValues(cdrStream& giop_client)
{
  if (out_l_ == -1) return;  // Oneway operation

  reacquireInterpreterLock();

  if (out_l_ == 0) {
    Py_INCREF(Py_None);
    result_ = Py_None;
  }
  else {
    PyUnlockingCdrStream pystream(giop_client, this);

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
omniPy::Py_omniCallDescriptor::userException(GIOP_C& giop_client,
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
    try {
      PyUnlockingCdrStream pystream(giop_client, this);

      for (i=0, j=5; i < cnt; i++, j+=2) {
	PyTuple_SET_ITEM(exctuple, i,
			 unmarshalPyObject(pystream,
					   PyTuple_GET_ITEM(d_o, j)));
      }
    }
    catch (...) {
      Py_DECREF(exctuple);
      releaseInterpreterLock();
      giop_client.RequestCompleted();
      throw;
    }
    PyObject* exc_i = PyEval_CallObject(excclass, exctuple);
    Py_DECREF(exctuple);

    if (exc_i) {
      PyErr_SetObject(excclass, exc_i);
      Py_DECREF(exc_i);
    }
    releaseInterpreterLock();
    giop_client.RequestCompleted();
    throw UserExceptionHandled();
  }
  else {
    releaseInterpreterLock();
    giop_client.RequestCompleted(1);
    OMNIORB_THROW(MARSHAL, 0, CORBA::COMPLETED_MAYBE);
  }
}


class reacquireLockInScope {
public:
  inline reacquireLockInScope(omniPy::Py_omniCallDescriptor* pycd)
    : pycd_(pycd) { pycd->reacquireInterpreterLock(); }

  inline ~reacquireLockInScope() {
    pycd_->releaseInterpreterLock();
  }
private:
  omniPy::Py_omniCallDescriptor* pycd_;
};


void
omniPy::Py_localCallBackFunction(omniCallDescriptor* cd, omniServant* svnt)
{
  Py_omniCallDescriptor* pycd = (Py_omniCallDescriptor*)cd;
  Py_omniServant*        pyos =
    (Py_omniServant*)svnt->_ptrToInterface("Py_omniServant");

  {
    // local_dispatch() can throw CORBA system exceptions, hence the
    // helper class
    reacquireLockInScope _l(pycd);
    pycd->result_ = pyos->local_dispatch(pycd->op(),
					 pycd->in_d_,  pycd->in_l_,
					 pycd->out_d_, pycd->out_l_,
					 pycd->exc_d_, pycd->args_);
  }
}


void
omniPy::Py_omniCallDescriptor::unmarshalArguments(cdrStream& giop_s)
{
  OMNIORB_ASSERT(args_ == 0);

  args_ = PyTuple_New(in_l_);

  PyUnlockingCdrStream pystream(giop_s, this);

  for (int i=0; i < in_l_; i++) {
    PyTuple_SET_ITEM(args_, i,
		     omniPy::unmarshalPyObject(pystream,
					       PyTuple_GET_ITEM(in_d_, i)));
  }
}

void
omniPy::Py_omniCallDescriptor::validateReturnedValues(PyObject* result)
{
  OMNIORB_ASSERT(result_ == 0);
  result_ = result;

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
omniPy::Py_omniCallDescriptor::marshalReturnedValues(cdrStream& giop_s)
{
  PyUnlockingCdrStream pystream(giop_s, this);

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



void
PyUnlockingCdrStream::put_octet_array(const _CORBA_Octet* b, int size,
				      omni::alignment_t align)
{
  callDescriptor_->releaseInterpreterLock();
  cdrStreamAdapter::put_octet_array(b, size, align);
  callDescriptor_->reacquireInterpreterLock();
}

void
PyUnlockingCdrStream::get_octet_array(_CORBA_Octet* b,int size,
				      omni::alignment_t align)
{
  callDescriptor_->releaseInterpreterLock();
  cdrStreamAdapter::get_octet_array(b, size, align);
  callDescriptor_->reacquireInterpreterLock();
}
  
void
PyUnlockingCdrStream::skipInput(_CORBA_ULong size)
{
  callDescriptor_->releaseInterpreterLock();
  cdrStreamAdapter::skipInput(size);
  callDescriptor_->reacquireInterpreterLock();
}

void
PyUnlockingCdrStream::fetchInputData(omni::alignment_t align, size_t required)
{
  callDescriptor_->releaseInterpreterLock();
  cdrStreamAdapter::fetchInputData(align, required);
  callDescriptor_->reacquireInterpreterLock();
}

_CORBA_Boolean
PyUnlockingCdrStream::reserveOutputSpace(omni::alignment_t align,
					 size_t required)
{
  callDescriptor_->releaseInterpreterLock();
  _CORBA_Boolean ret = cdrStreamAdapter::reserveOutputSpace(align, required);
  callDescriptor_->reacquireInterpreterLock();
  return ret;
}
