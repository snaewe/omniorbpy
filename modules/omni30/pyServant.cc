// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyServant.cc               Created on: 1999/07/29
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
//    Implementation of Python servant object


// $Id$

// $Log$
// Revision 1.7  1999/09/29 15:46:50  dpg1
// lockWithNewThreadState now creates a dummy threading.Thread object so
// threading doesn't get upset that it's not there. Very dependent on the
// implementation of threading.py.
//
// Revision 1.6  1999/09/29 11:38:29  dpg1
// Comments removed.
//
// Revision 1.5  1999/09/29 09:53:23  dpg1
// Workaround to Python's lack of concurrency control on its
// PyInterpreterState.
//
// Revision 1.4  1999/09/24 09:22:02  dpg1
// Added copyright notices.
//
// Revision 1.3  1999/09/22 15:46:12  dpg1
// Fake POA implemented.
//
// Revision 1.2  1999/09/20 14:56:12  dpg1
// GCC 2.95 is more pedantic than egcs.
//
// Revision 1.1  1999/07/29 14:20:49  dpg1
// Initial revision
//


#include <omnipy.h>

#include <iostream.h>


// Helper class to create a Python ThreadState object and grab the
// Python interpreter lock, then release the lock and delete the
// object when it goes out of state.

class lockWithNewThreadState {
public:
  lockWithNewThreadState() {
    // Create thread state
    omniPy::pyInterpreterLock.lock();
    newstate_ = PyThreadState_New(omniPy::pyInterpreter);
    omniPy::pyInterpreterLock.unlock();

    // Acquire global interpreter lock
    PyEval_AcquireLock();
    oldstate_ = PyThreadState_Swap(newstate_);

    // Create a dummy threading.Thread object, so the threading module
    // is happy. *** This is very dependent on the implementation of
    // the threading module.
    dummy_thread_ = PyEval_CallObject(omniPy::pyDummyThreadClass,
				      empty_tuple_);
    assert(dummy_thread_);
  }

  ~lockWithNewThreadState() {
    // Remove the dummy Thread object
    PyObject* meth = PyObject_GetAttrString(dummy_thread_, "_Thread__delete");
    assert(meth);
    PyEval_CallObject(meth, empty_tuple_);
    Py_DECREF(meth);
    Py_DECREF(dummy_thread_);

    // Return to the previous thread state
    PyThreadState_Swap(oldstate_);

    // We would like to release the interpreter lock here, before
    // deleting the ThreadState struct. Unfortunately, if we do that
    // the Python program may end before we get to do the delete. In
    // that situation, we might call Delete() while Python is clearing
    // up its interpreter state, leading to a segfault. So we have to
    // delete the ThreadState first, then release the interpreter
    // lock. Python really ought to do some concurrency control on the
    // PyInterpreterState structure.

    omniPy::pyInterpreterLock.lock();
    PyThreadState_Delete(newstate_);
    omniPy::pyInterpreterLock.unlock();

    PyEval_ReleaseLock();
  }

private:
  PyThreadState*   newstate_;
  PyThreadState*   oldstate_;
  PyObject*        dummy_thread_;
  static PyObject* empty_tuple_;
};

PyObject* lockWithNewThreadState::empty_tuple_ = PyTuple_New(0);


omniPy::
Py_Servant::Py_Servant(PyObject* pyservant, PyObject* opdict,
		       const char* repoId)
  : pyservant_(pyservant), opdict_(opdict)
{
  assert(PyInstance_Check(pyservant));
  assert(PyDict_Check(opdict));
  Py_INCREF(pyservant_);
  Py_INCREF(opdict_);

  omniObject::PR_IRRepositoryId(repoId);
  this->PR_setobj(this);
}

omniPy::
Py_Servant::~Py_Servant()
{
  //  cout << "Py_Servant destructor." << endl;
  Py_DECREF(pyservant_);
  Py_DECREF(opdict_);
}

omniORB::objectKey
omniPy::
Py_Servant::_key()
{
  omniRopeAndKey l;
  getRopeAndKey(l);
  return (*((omniORB::objectKey*)l.key()));
}

void*
omniPy::
Py_Servant::_widenFromTheMostDerivedIntf(const char* repoId,
					 CORBA::Boolean is_cxx_type_id)
{
  if (is_cxx_type_id) {
    if (strcmp(repoId, "Py_Servant") == 0)
      return (void*)this;
    else
      return 0;
  }
  else if (repoId == 0)
    return (void*)((CORBA::Object_ptr)this);
  else if (!strcmp(repoId, NP_IRRepositoryId()))
    return (void*)this;
  else
    return 0;
}


CORBA::Boolean
omniPy::
Py_Servant::dispatch(GIOP_S&        giop_server,
		     const char*    op,
		     CORBA::Boolean response)
{
  int i;

  //  cout << "dispatch()..." << op << endl;

  lockWithNewThreadState _t;

  PyObject* desc = PyDict_GetItemString(opdict_, (char*)op);

  if (!desc) return 0; // Unknown operation name

  assert(PyTuple_Check(desc));

  PyObject *in_d, *out_d, *exc_d;
  int       in_l,  out_l;

  in_d    = PyTuple_GET_ITEM(desc,0);
  out_d   = PyTuple_GET_ITEM(desc,1);
  exc_d   = PyTuple_GET_ITEM(desc,2);

  assert(PyTuple_Check(in_d));

  in_l  = PyTuple_GET_SIZE(in_d);

  if (out_d == Py_None)
    out_l = -1;
  else
    out_l = PyTuple_GET_SIZE(out_d);

  // Unmarshal in arguments
  PyObject* argtuple = PyTuple_New(in_l);

  for (i=0; i < in_l; i++) {
    PyTuple_SET_ITEM(argtuple, i,
		     omniPy::unmarshalPyObject(giop_server,
					       PyTuple_GET_ITEM(in_d, i)));
  }

  giop_server.RequestReceived();

  // Do the up-call
  PyObject* method = PyObject_GetAttrString(pyservant_, (char*)op);

  if (!method) {
    cerr << "Couldn't find method called " << op << "!" << endl;
    Py_DECREF(argtuple);
    throw CORBA::NO_IMPLEMENT(0,CORBA::COMPLETED_NO);
  }

  PyObject* result = PyEval_CallObject(method, argtuple);
  Py_DECREF(method);
  Py_DECREF(argtuple);

  if (result) {
    // No exception was thrown. Marshal the return value
    if (out_l >= 0) {
      CORBA::ULong msgsize = GIOP_S::ReplyHeaderSize();

      if (out_l == 1) {
	msgsize = omniPy::alignedSize(msgsize,
				      PyTuple_GET_ITEM(out_d, 0),
				      result);
      }
      else if (out_l > 1) {
	for (i=0; i < out_l; i++) {
	  msgsize = omniPy::alignedSize(msgsize,
					PyTuple_GET_ITEM(out_d,  i),
					PyTuple_GET_ITEM(result, i));
	}
      }
      giop_server.InitialiseReply(GIOP::NO_EXCEPTION, msgsize);

      if (out_l == 1) {
	omniPy::marshalPyObject(giop_server,
				PyTuple_GET_ITEM(out_d, 0),
				result);
      }
      else if (out_l > 1) {
	for (i=0; i < out_l; i++) {
	  omniPy::marshalPyObject(giop_server,
				  PyTuple_GET_ITEM(out_d,  i),
				  PyTuple_GET_ITEM(result, i));
	}
      }
    }
    else {
      if (response) {
	Py_DECREF(result);
	throw CORBA::BAD_OPERATION(0,CORBA::COMPLETED_NO);
      }
    }
    Py_DECREF(result);
    giop_server.ReplyCompleted();

    //    cout << "dispatch done: " << op << endl;
    return 1;
  }
  else {
    // An exception of some sort was thrown
    PyObject *etype, *evalue, *etraceback;

    //    cout << "Exception..." << endl;

    PyErr_Fetch(&etype, &evalue, &etraceback);

    assert(etype);

    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    if (!(evalue && PyInstance_Check(evalue)))
      throw CORBA::UNKNOWN(0,CORBA::COMPLETED_NO);

    //    cout << "Exception may be one we know..." << endl;

    PyObject* erepoId = PyObject_GetAttrString(evalue, "_NP_RepositoryId");

    if (!erepoId) {
      Py_DECREF(evalue);
      throw CORBA::UNKNOWN(0,CORBA::COMPLETED_NO);
    }

    // Is it a user exception?
    if (exc_d != Py_None) {
      assert(PyDict_Check(exc_d));

      PyObject* edesc = PyDict_GetItem(exc_d, erepoId);

      if (edesc) {
	//	cout << "User exception. marshalling..." << endl;
	CORBA::ULong msgsize = GIOP_S::ReplyHeaderSize();
	msgsize = omniPy::alignedSize(msgsize, edesc, evalue);
	giop_server.InitialiseReply(GIOP::USER_EXCEPTION, msgsize);
	omniPy::marshalPyObject(giop_server, edesc, evalue);
	giop_server.ReplyCompleted();
	Py_DECREF(erepoId);
	Py_DECREF(evalue);
	return 1;
      }
    }
    omniPy::produceSystemException(evalue, erepoId);
  }
  assert(0); // Never reach here.
  return 0;
}
