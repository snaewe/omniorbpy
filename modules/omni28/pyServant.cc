// -*- c++ -*-

// Implementation of Python servant object

// $Id$

// $Log$
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
  }

  ~lockWithNewThreadState() {
    PyThreadState_Swap(oldstate_);
    PyEval_ReleaseLock();

    omniPy::pyInterpreterLock.lock();
    PyThreadState_Delete(newstate_);
    omniPy::pyInterpreterLock.unlock();
  }

private:
  PyThreadState* newstate_;
  PyThreadState* oldstate_;
};


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

  //  cout << "dispatch()..." << endl;

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
