// Exception handling functions

// $Id$

// $Log$
// Revision 1.1  1999/07/29 14:18:58  dpg1
// Initial revision
//

#include <omnipy.h>

#include <iostream.h>


void
omniPy::handleSystemException(const CORBA::SystemException& ex)
{
  PyObject* excc = PyDict_GetItemString(pyCORBAsysExcMap,
					ex.NP_RepositoryId());

  PyObject* exca = Py_BuildValue("(ii)", ex.minor(), ex.completed());
  PyObject* exci = PyEval_CallObject(excc, exca);
  Py_DECREF(exca);
  PyErr_SetObject(excc, exci);
}



void
omniPy::produceSystemException(PyObject* eobj, PyObject* erepoId)
{
  CORBA::ULong            minor;
  CORBA::CompletionStatus status;

  PyObject *a, *b;

  a     = PyObject_GetAttrString(eobj, "minor"); assert(a && PyInt_Check(a));
  minor = PyInt_AS_LONG(a);
  Py_DECREF(a);

  a      = PyObject_GetAttrString(eobj, "completed"); assert(a);
  b      = PyObject_GetAttrString(a,    "_v"); assert(b && PyInt_Check(b));
  status = (CORBA::CompletionStatus)PyInt_AS_LONG(b);
  Py_DECREF(a); Py_DECREF(b);

  char* repoId = PyString_AS_STRING(erepoId);

  Py_DECREF(eobj);

  if (!strcmp(repoId, "IDL:omg.org/CORBA/UNKNOWN")) {
    Py_DECREF(erepoId); throw CORBA::UNKNOWN(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_PARAM")) {
    Py_DECREF(erepoId); throw CORBA::BAD_PARAM(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_MEMORY")) {
    Py_DECREF(erepoId); throw CORBA::NO_MEMORY(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/IMP_LIMIT")) {
    Py_DECREF(erepoId); throw CORBA::IMP_LIMIT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/COMM_FAILURE")) {
    Py_DECREF(erepoId); throw CORBA::COMM_FAILURE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_OBJREF")) {
    Py_DECREF(erepoId); throw CORBA::INV_OBJREF(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/OBJECT_NOT_EXIST")) {
    Py_DECREF(erepoId); throw CORBA::OBJECT_NOT_EXIST(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_PERMISSION")) {
    Py_DECREF(erepoId); throw CORBA::NO_PERMISSION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INTERNAL")) {
    Py_DECREF(erepoId); throw CORBA::INTERNAL(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/MARSHAL")) {
    Py_DECREF(erepoId); throw CORBA::MARSHAL(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INITIALIZE")) {
    Py_DECREF(erepoId); throw CORBA::INITIALIZE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_IMPLEMENT")) {
    Py_DECREF(erepoId); throw CORBA::NO_IMPLEMENT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_TYPECODE")) {
    Py_DECREF(erepoId); throw CORBA::BAD_TYPECODE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_OPERATION")) {
    Py_DECREF(erepoId); throw CORBA::BAD_OPERATION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_RESOURCES")) {
    Py_DECREF(erepoId); throw CORBA::NO_RESOURCES(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/NO_RESPONSE")) {
    Py_DECREF(erepoId); throw CORBA::NO_RESPONSE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/PERSIST_STORE")) {
    Py_DECREF(erepoId); throw CORBA::PERSIST_STORE(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_INV_ORDER")) {
    Py_DECREF(erepoId); throw CORBA::BAD_INV_ORDER(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSIENT")) {
    Py_DECREF(erepoId); throw CORBA::TRANSIENT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/FREE_MEM")) {
    Py_DECREF(erepoId); throw CORBA::FREE_MEM(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_IDENT")) {
    Py_DECREF(erepoId); throw CORBA::INV_IDENT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INV_FLAG")) {
    Py_DECREF(erepoId); throw CORBA::INV_FLAG(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INTF_REPOS")) {
    Py_DECREF(erepoId); throw CORBA::INTF_REPOS(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/BAD_CONTEXT")) {
    Py_DECREF(erepoId); throw CORBA::BAD_CONTEXT(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/OBJ_ADAPTER")) {
    Py_DECREF(erepoId); throw CORBA::OBJ_ADAPTER(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/DATA_CONVERSION")) {
    Py_DECREF(erepoId); throw CORBA::DATA_CONVERSION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSACTION_REQUIRED")) {
    Py_DECREF(erepoId); throw CORBA::TRANSACTION_REQUIRED(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK")) {
    Py_DECREF(erepoId); throw CORBA::TRANSACTION_ROLLEDBACK(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/INVALID_TRANSACTION")) {
    Py_DECREF(erepoId); throw CORBA::INVALID_TRANSACTION(minor, status);
  }
  else if (!strcmp(repoId, "IDL:omg.org/CORBA/WRONG_TRANSACTION")) {
    Py_DECREF(erepoId); throw CORBA::WRONG_TRANSACTION(minor, status);
  }
  else {
    Py_DECREF(erepoId); throw CORBA::UNKNOWN(0, CORBA::COMPLETED_NO);
  }
}
