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
// Revision 1.17.2.2  2000/09/01 14:13:00  dpg1
// Memory leak when returning invalid data
//
// Revision 1.17.2.1  2000/08/14 16:10:32  dpg1
// Missed out some explicit casts to (char*) for string constants.
//
// Revision 1.17  2000/05/26 15:33:32  dpg1
// Python thread states are now cached. Operation dispatch time is
// roughly halved!
//
// Revision 1.16  2000/04/06 14:12:40  dpg1
// Incorrect format character in PyObject_CallMethod() caused a reference
// count leak.
//
// Revision 1.15  2000/04/03 11:21:00  dpg1
// Error report if a method does not exist on upcall.
//
// Revision 1.14  2000/03/24 16:48:58  dpg1
// Local calls now have proper pass-by-value semantics.
// Lots of little stability improvements.
// Memory leaks fixed.
//
// Revision 1.13  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//
// Revision 1.12  1999/12/15 12:17:19  dpg1
// Changes to compile with SunPro CC 5.0.
//
// Revision 1.11  1999/11/25 11:21:36  dpg1
// Proper support for server-side _is_a().
//
// Revision 1.10  1999/11/16 17:32:36  dpg1
// Changes for AIX.
//
// Revision 1.9  1999/11/08 11:43:35  dpg1
// Changes for NT support.
//
// Revision 1.8  1999/10/01 11:07:20  dpg1
// Error reporting if up-call raises an unexpected exception.
//
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
#include <common/pyThreadCache.h>


omniPy::
Py_Servant::Py_Servant(PyObject* pyservant, PyObject* opdict,
		       const char* repoId)
  : pyservant_(pyservant), opdict_(opdict)
{
  OMNIORB_ASSERT(PyInstance_Check(pyservant));
  OMNIORB_ASSERT(PyDict_Check(opdict));
  Py_INCREF(pyservant_);
  Py_INCREF(opdict_);

  pyskeleton_ = PyObject_GetAttrString(pyservant_, (char*)"_omni_skeleton");
  OMNIORB_ASSERT(pyskeleton_ && PyClass_Check(pyskeleton_));

  omniPy::setTwin(pyservant, (omniPy::Py_Servant*)this, SERVANT_TWIN);

  omniObject::PR_IRRepositoryId(repoId);
  this->PR_setobj(this);
}

omniPy::
Py_Servant::~Py_Servant()
{
  omnipyThreadCache::lock _t;
  omniPy::remTwin(pyservant_, SERVANT_TWIN);
  Py_DECREF(pyservant_);
  Py_DECREF(opdict_);
  Py_DECREF(pyskeleton_);
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
  else {
    omnipyThreadCache::lock _t;
    PyObject* isa = PyObject_CallMethod(omniPy::pyomniORBmodule,
					(char*)"static_is_a", (char*)"Os",
					pyskeleton_, repoId);
    if (!isa)
      PyErr_Print();
      
    OMNIORB_ASSERT(isa && PyInt_Check(isa));
    if (PyInt_AS_LONG(isa))
      return (void*)this;
  }
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

  omnipyThreadCache::lock _t;

  PyObject* desc = PyDict_GetItemString(opdict_, (char*)op);

  if (!desc) return 0; // Unknown operation name

  OMNIORB_ASSERT(PyTuple_Check(desc));

  PyObject *in_d, *out_d, *exc_d;
  int       in_l,  out_l;

  in_d    = PyTuple_GET_ITEM(desc,0);
  out_d   = PyTuple_GET_ITEM(desc,1);
  exc_d   = PyTuple_GET_ITEM(desc,2);

  OMNIORB_ASSERT(PyTuple_Check(in_d));

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
    if (omniORB::trace(1)) {
      omniORB::logger l;
      l << "Python servant for `" << NP_IRRepositoryId()
	<< "' has no method named `" << op << "'.\n";
    }
    PyErr_Clear();
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

      try {
	if (out_l == 1) {
	  msgsize = omniPy::alignedSize(msgsize,
					PyTuple_GET_ITEM(out_d, 0),
					result,
					CORBA::COMPLETED_MAYBE);
	}
	else if (out_l > 1) {
	  for (i=0; i < out_l; i++) {
	    msgsize = omniPy::alignedSize(msgsize,
					  PyTuple_GET_ITEM(out_d,  i),
					  PyTuple_GET_ITEM(result, i),
					  CORBA::COMPLETED_MAYBE);
	  }
	}
      }
      catch (...) {
	// alignedSize() can throw BAD_PARAM and others
	Py_DECREF(result);
	throw;
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
    PyObject *erepoId = 0;
    PyErr_Fetch(&etype, &evalue, &etraceback);
    OMNIORB_ASSERT(etype);

    if (evalue && PyInstance_Check(evalue))
      erepoId = PyObject_GetAttrString(evalue, (char*)"_NP_RepositoryId");

    if (!erepoId) {
      if (omniORB::trace(1)) {
	{
	  omniORB::logger l;
	  l << "Caught an unexpected Python exception during up-call.\n";
	}
	PyErr_Restore(etype, evalue, etraceback);
	PyErr_Print();
      }
      throw CORBA::UNKNOWN(0,CORBA::COMPLETED_NO);
    }
    Py_DECREF(etype);
    Py_XDECREF(etraceback);

    // Is it a user exception?
    if (exc_d != Py_None) {
      OMNIORB_ASSERT(PyDict_Check(exc_d));

      PyObject* edesc = PyDict_GetItem(exc_d, erepoId);

      if (edesc) {
	//	cout << "User exception. marshalling..." << endl;
	CORBA::ULong msgsize = GIOP_S::ReplyHeaderSize();
	msgsize = omniPy::alignedSize(msgsize, edesc, evalue,
				      CORBA::COMPLETED_MAYBE);
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
  OMNIORB_ASSERT(0); // Never reach here.
  return 0;
}


PyObject*
omniPy::
Py_Servant::local_dispatch(const char* op,
			   PyObject*   in_d,  int in_l,
			   PyObject*   out_d, int out_l,
			   PyObject*   exc_d,
			   PyObject*   args)
{
  PyObject* method = PyObject_GetAttrString(pyservant_, (char*)op);
  if (!method) {
    if (omniORB::trace(1)) {
      omniORB::logger l;
      l << "Python servant for `" << NP_IRRepositoryId()
	<< "' has no method named `" << op << "'.\n";
    }
    PyErr_Clear();
    CORBA::NO_IMPLEMENT ex;
    return omniPy::handleSystemException(ex);
  }

  // Copy args which would otherwise have reference semantics
  PyObject* argtuple = PyTuple_New(in_l);
  PyObject* t_o;

  int i, valid = 1;
  for (i=0; i < in_l; i++) {
    t_o = copyArgument(PyTuple_GET_ITEM(in_d, i),
		       PyTuple_GET_ITEM(args, i),
		       CORBA::COMPLETED_NO);
    if (t_o)
      PyTuple_SET_ITEM(argtuple, i, t_o);
    else {
      Py_INCREF(Py_None);
      PyTuple_SET_ITEM(argtuple, i, Py_None);
      valid = 0;
    }
  }

  if (valid) {
    // Do the call
    PyObject* result = PyEval_CallObject(method, argtuple);
    Py_DECREF(method);
    Py_DECREF(argtuple);

    if (result) {
      PyObject* retval;

      if (out_l == -1 || out_l == 0) {
	if (result == Py_None) {
	  return result;
	}
	else {
	  Py_DECREF(result);
	  CORBA::BAD_PARAM ex(0,CORBA::COMPLETED_MAYBE);
	  return omniPy::handleSystemException(ex);
	}
      }
      else if (out_l == 1) {
	retval = copyArgument(PyTuple_GET_ITEM(out_d, 0),
			      result, CORBA::COMPLETED_MAYBE);
      }
      else {
	valid = 1;
	retval = PyTuple_New(out_l);
	
	for (i=0; i < out_l; i++) {
	  t_o = copyArgument(PyTuple_GET_ITEM(out_d, i),
			     PyTuple_GET_ITEM(result, i),
			     CORBA::COMPLETED_MAYBE);
	  if (t_o)
	    PyTuple_SET_ITEM(retval, i, t_o);
	  else {
	    Py_INCREF(Py_None);
	    PyTuple_SET_ITEM(retval, i, Py_None);
	    valid = 0;
	  }
	}
	if (!valid) {
	  Py_DECREF(retval);
	  retval = 0;
	}
      }
      Py_DECREF(result);
      return retval;
    }
    else {
      // The call raised a Python exception
      PyObject *etype, *evalue, *etraceback;
      PyObject *erepoId = 0;
      PyErr_Fetch(&etype, &evalue, &etraceback);
      OMNIORB_ASSERT(etype);

      if (evalue && PyInstance_Check(evalue))
	erepoId = PyObject_GetAttrString(evalue, (char*)"_NP_RepositoryId");

      if (!erepoId) {
	if (omniORB::trace(1)) {
	  {
	    omniORB::logger l;
	    l << "Caught an unexpected Python exception during up-call.\n";
	  }
	  PyErr_Restore(etype, evalue, etraceback);
	  PyErr_Print();
	}
	CORBA::UNKNOWN ex(0,CORBA::COMPLETED_MAYBE);
	return omniPy::handleSystemException(ex);
      }

      Py_DECREF(etype);
      Py_XDECREF(etraceback);

      // Is it a user exception?
      if (exc_d != Py_None) {
	OMNIORB_ASSERT(PyDict_Check(exc_d));

	PyObject* edesc = PyDict_GetItem(exc_d, erepoId);

	if (edesc) {
	  PyObject* cevalue = copyArgument(edesc, evalue,
					   CORBA::COMPLETED_MAYBE);
	  Py_DECREF(erepoId);
	  Py_DECREF(evalue);

	  if (cevalue) {
	    PyErr_SetObject(PyTuple_GET_ITEM(edesc, 1), cevalue);
	    Py_DECREF(cevalue);
	  }
	  else{
	    CORBA::MARSHAL ex(0,CORBA::COMPLETED_MAYBE);
	    omniPy::handleSystemException(ex);
	  }
	  return 0;
	}
      }
      // System exception?
      PyObject* excc = PyDict_GetItem(pyCORBAsysExcMap, erepoId);
      if (excc) {
	PyObject *pyminor, *pycompl;
	pyminor = PyObject_GetAttrString(evalue, (char*)"minor");
	pycompl = PyObject_GetAttrString(evalue, (char*)"completed");
	OMNIORB_ASSERT(pyminor && PyInt_Check(pyminor));
	OMNIORB_ASSERT(pycompl && PyInstance_Check(pycompl));
	PyObject* exca = Py_BuildValue((char*)"(NN)", pyminor, pycompl);
	PyObject* exci = PyEval_CallObject(excc, exca);
	Py_DECREF(exca);
	Py_DECREF(erepoId);
	Py_DECREF(evalue);
	if (exci) {
	  PyErr_SetObject(excc, exci);
	  Py_DECREF(exci);
	}
	return 0;
      }
      Py_DECREF(erepoId);
      Py_DECREF(evalue);
      CORBA::UNKNOWN ex(0, CORBA::COMPLETED_MAYBE);
      return omniPy::handleSystemException(ex);
    }
  }
  else {
    // Args were in invalid
    Py_DECREF(argtuple);
    Py_DECREF(method);
    return 0;
  }
}


void
omniPy::
Py_Servant::deactivate(CORBA::BOA_ptr boa)
{
  int r = PyObject_DelAttrString(pyservant_, (char*)"_omni_objref");
  {
    omniPy::InterpreterUnlocker _u;
    boa->dispose(this);
  }
}


extern void omniPy_objectIsReady(omniObject* obj);

omniPy::Py_Servant*
omniPy::getServantForPyObject(PyObject* pyservant)
{
  if (!PyInstance_Check(pyservant))
    return 0;

  Py_Servant* pyos;

  // Is there a Py_Servant already?
  pyos = (omniPy::Py_Servant*)omniPy::getTwin(pyservant, SERVANT_TWIN);
  if (pyos) {
    return pyos;
  }

  // Is it an instance of the right class?
  PyClassObject* pysc = ((PyInstanceObject*)pyservant)->in_class;

  if (!PyClass_IsSubclass((PyObject*)pysc, omniPy::pyServantClass))
    return 0;

  PyObject* opdict = PyObject_GetAttrString(pyservant, (char*)"_omni_op_d");
  if (!(opdict && PyDict_Check(opdict)))
    return 0;

  PyObject* pyrepoId = PyObject_GetAttrString(pyservant,
					      (char*)"_NP_RepositoryId");
  if (!(pyrepoId && PyString_Check(pyrepoId))) {
    Py_DECREF(opdict);
    return 0;
  }

  pyos = new omniPy::Py_Servant(pyservant, opdict,
				PyString_AS_STRING(pyrepoId));
  Py_DECREF(pyrepoId);
  Py_DECREF(opdict);

  omniPy_objectIsReady(pyos);

  CORBA::Object_ptr objref = (CORBA::Object_ptr)pyos;

  PyObject* pyobjref = omniPy::createPyCorbaObjRef(0, objref);

  PyObject_SetAttrString(pyservant, (char*)"_omni_objref", pyobjref);
  Py_DECREF(pyobjref);

  return pyos;
}
