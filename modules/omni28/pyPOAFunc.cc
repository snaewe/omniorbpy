// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyPOAFunc.cc               Created on: 2000/02/04
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
//    POA functions

// $Id$
// $Log$
// Revision 1.2  2000/03/06 18:45:53  dpg1
// Support for deactivate_object().
//
// Revision 1.1  2000/03/03 17:41:42  dpg1
// Major reorganisation to support omniORB 3.0 as well as 2.8.
//

#include <omnipy.h>

#include <ropeFactory.h>   // Internal omniORB interface
#include <objectManager.h> // Internal omniORB interface


#define RAISE_PY_NO_IMPLEMENT \
  CORBA::NO_IMPLEMENT _ex; return omniPy::handleSystemException(_ex)


PyObject*
omniPy::fakePyRootPOAObject(PyObject* pyorb, const CORBA::ORB_ptr cxxorb)
{
  // Return existing POA object if there is one
  PyObject* pypoa = PyObject_GetAttrString(pyorb, (char*)"_omni_fpoa");
  if (pypoa) return pypoa;

  PyErr_Clear();

  // Create a new one
  PyObject* pypoa_class =
    PyObject_GetAttrString(omniPy::pyPortableServerModule, (char*)"POA");
  OMNIORB_ASSERT(pypoa_class);

  pypoa = PyEval_CallObject(pypoa_class, omniPy::pyEmptyTuple);
  OMNIORB_ASSERT(pypoa);

  // Create a POAManager too
  PyObject* pypm_class =
    PyObject_GetAttrString(omniPy::pyPortableServerModule,
			   (char*)"POAManager");
  OMNIORB_ASSERT(pypm_class);

  PyObject* pypm = PyEval_CallObject(pypm_class, omniPy::pyEmptyTuple);
  OMNIORB_ASSERT(pypm);

  PyObject_SetAttrString(pypoa, (char*)"_omni_poamanager", pypm);
  Py_DECREF(pypm);

  // Initialise the omniORB 2.8 BOA
  PyObject* pyargv = PyObject_GetAttrString(pyorb, (char*)"_omni_argv");
  OMNIORB_ASSERT(pyargv && PyList_Check(pyargv));

  int    argc = PyList_GET_SIZE(pyargv);
  char** argv = new char*[argc];

  PyObject* o;
  int i;
  for (i=0; i<argc; i++) {
    o = PyList_GET_ITEM(pyargv, i);
    argv[i] = PyString_AS_STRING(o);
  }

  int orig_argc = argc;

  CORBA::BOA_ptr boa = cxxorb->BOA_init(argc, argv, "omniORB2_BOA");

  // This is extremely horrid -- modify the Python list in place to
  // reflect the changed argv. This leaks PyStringObjects, but they
  // would have hung around for the program's life time anyway...
  if (argc < orig_argc) {
    int j;
    char *s, *t;
    for (i=0, j=0; i<argc; i++, j++) {
      s = argv[i];

      while (1) {
	o = PyList_GET_ITEM(pyargv, j);
	t = PyString_AS_STRING(o);
	if (s == t) break;
	j++;
	OMNIORB_ASSERT(j < orig_argc);
      }
      PyList_SET_ITEM(pyargv, i, o);
    }
    ((PyListObject*)pyargv)->ob_size = argc;
  }

  delete [] argv;

  omniPy::setTwin(pypoa, (CORBA::BOA_ptr)boa, BOA_TWIN);
  PyObject_SetAttrString(pyorb, (char*)"_omni_fpoa", pypoa);
  return pypoa;
}


static
PyObject* raisePOAException(PyObject* pyPOA, const char* ename)
{
  PyObject* excc = PyObject_GetAttrString(pyPOA, (char*)ename);
  OMNIORB_ASSERT(excc);
  PyObject* exci = PyEval_CallObject(excc, omniPy::pyEmptyTuple);
  PyErr_SetObject(excc, exci);
  return 0;
}


extern "C" {

  static PyObject* pyPOA_create_POA(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_find_POA(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_destroy(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    int       eth, wait;

    if (!PyArg_ParseTuple(args, (char*)"Oii",& pyPOA, &eth, &wait))
      return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    // Call the function
    try {
      {
	omniPy::InterpreterUnlocker _u;
	boa->destroy();
      }
      Py_INCREF(Py_None);
      return Py_None;
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
  }

  static PyObject* pyPOA_get_the_name(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    return PyString_FromString((char*)"RootPOA");
  }

  static PyObject* pyPOA_get_the_parent(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_get_the_children(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_get_the_POAManager(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O",&pyPOA)) return 0;

    PyObject* pypm = PyObject_GetAttrString(pyPOA, (char*)"_omni_poamanager");
    OMNIORB_ASSERT(pypm);
    return pypm;
  }

  static PyObject* pyPOA_get_the_activator(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_set_the_activator(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_get_servant_manager(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_set_servant_manager(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_get_servant(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_set_servant(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_activate_object(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyServant)) return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    omniPy::Py_Servant* pyos;
    try {
      pyos = omniPy::getServantForPyObject(pyServant);
      RAISE_PY_BAD_PARAM_IF(!pyos);
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    omniObject* oobj = pyos->PR_getobj();

    CORBA::Octet* key;
    CORBA::ULong  ksize;
    oobj->getKey(key, ksize);

    PyObject* pykey = PyString_FromStringAndSize(0, ksize);
    CORBA::Octet* s = (CORBA::Octet*)PyString_AS_STRING(pykey);

    for (CORBA::ULong i=0; i<ksize; i++)
      s[i] = key[i];

    return pykey;
  }

  static PyObject* pyPOA_activate_object_with_id(PyObject* self,
						 PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_deactivate_object(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#", &pyPOA, &oidstr, &oidlen))
      return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    if (oidlen != sizeof(omniObjectKey))
      return raisePOAException(pyPOA, "ObjectNotActive");

    omniObject* oobj = omni::locatePyObject(omniObjectManager::root(1),
					    *((omniObjectKey*)oidstr));
    if (!oobj)
      return raisePOAException(pyPOA, "ObjectNotActive");

    OMNIORB_ASSERT(!oobj->is_proxy()); // It came from the local object table!

    omniPy::Py_Servant* serv = (omniPy::Py_Servant*)
      oobj->_widenFromTheMostDerivedIntf("Py_Servant", 1);

    RAISE_PY_BAD_PARAM_IF(!serv);

    CORBA::release(serv);
    boa->dispose(serv);
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject* pyPOA_create_reference(PyObject* self, PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_create_reference_with_id(PyObject* self,
						  PyObject* args)
  {
    RAISE_PY_NO_IMPLEMENT;
  }

  static PyObject* pyPOA_servant_to_id(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyServant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyServant)) return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    omniPy::Py_Servant* pyos;
    try {
      pyos = omniPy::getServantForPyObject(pyServant);
      RAISE_PY_BAD_PARAM_IF(!pyos);
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    omniObject* oobj = pyos->PR_getobj();

    CORBA::Octet* key;
    CORBA::ULong  ksize;
    oobj->getKey(key, ksize);

    PyObject* pykey = PyString_FromStringAndSize(0, ksize);
    CORBA::Octet* s = (CORBA::Octet*)PyString_AS_STRING(pykey);

    for (CORBA::ULong i=0; i<ksize; i++)
      s[i] = key[i];

    return pykey;
  }

  static PyObject* pyPOA_servant_to_reference(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyservant;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyservant)) return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    omniPy::Py_Servant* pyos;
    try {
      pyos = omniPy::getServantForPyObject(pyservant);
      RAISE_PY_BAD_PARAM_IF(!pyos);
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    PyObject* pyobjref = PyObject_GetAttrString(pyservant,
						(char*)"_omni_objref");
    OMNIORB_ASSERT(pyobjref);
    return pyobjref;
  }

  static PyObject* pyPOA_reference_to_servant(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyobjref)) return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr objref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!objref);

    omniObject* oobj = objref->PR_getobj();

    if (oobj->is_proxy()) {
      return raisePOAException(pyPOA, "WrongAdapter");
    }
    else {
      omniPy::Py_Servant* local = (omniPy::Py_Servant*)
	oobj->_widenFromTheMostDerivedIntf("Py_Servant", 1);

      RAISE_PY_BAD_PARAM_IF(!local);
      return local->pyServant();
    }
  }

  static PyObject* pyPOA_reference_to_id(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    PyObject* pyobjref;

    if (!PyArg_ParseTuple(args, (char*)"OO", &pyPOA, &pyobjref)) return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    RAISE_PY_BAD_PARAM_IF(!PyInstance_Check(pyobjref));

    CORBA::Object_ptr objref =
      (CORBA::Object_ptr)omniPy::getTwin(pyobjref, OBJREF_TWIN);

    RAISE_PY_BAD_PARAM_IF(!objref);

    omniObject* oobj = objref->PR_getobj();

    if (oobj->is_proxy()) {
      return raisePOAException(pyPOA, "WrongAdapter");
    }
    else {
      CORBA::Octet* key;
      CORBA::ULong  ksize;
      oobj->getKey(key, ksize);

      PyObject* pykey = PyString_FromStringAndSize(0, ksize);
      CORBA::Octet* s = (CORBA::Octet*)PyString_AS_STRING(pykey);

      for (CORBA::ULong i=0; i<ksize; i++)
	s[i] = key[i];

      return pykey;
    }
  }

  static PyObject* pyPOA_id_to_servant(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#", &pyPOA, &oidstr, &oidlen))
      return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    if (oidlen != sizeof(omniObjectKey))
      return raisePOAException(pyPOA, "ObjectNotActive");

    omniObject* oobj = omni::locatePyObject(omniObjectManager::root(1),
					    *((omniObjectKey*)oidstr));
    if (!oobj)
      return raisePOAException(pyPOA, "ObjectNotActive");

    OMNIORB_ASSERT(!oobj->is_proxy()); // It came from the local object table!

    omniPy::Py_Servant* serv = (omniPy::Py_Servant*)
      oobj->_widenFromTheMostDerivedIntf("Py_Servant", 1);

    RAISE_PY_BAD_PARAM_IF(!serv);
    return serv->pyServant();
  }

  static PyObject* pyPOA_id_to_reference(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    char*     oidstr;
    int       oidlen;

    if (!PyArg_ParseTuple(args, (char*)"Os#", &pyPOA, &oidstr, &oidlen))
      return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    if (oidlen != sizeof(omniObjectKey))
      return raisePOAException(pyPOA, "ObjectNotActive");

    omniObject* oobj = omni::locatePyObject(omniObjectManager::root(1),
					    *((omniObjectKey*)oidstr));
    if (!oobj)
      return raisePOAException(pyPOA, "ObjectNotActive");

    OMNIORB_ASSERT(!oobj->is_proxy()); // It came from the local object table!

    omniPy::Py_Servant* serv = (omniPy::Py_Servant*)
      oobj->_widenFromTheMostDerivedIntf("Py_Servant", 1);

    RAISE_PY_BAD_PARAM_IF(!serv);

    PyObject* pyservant = serv->pyServant();
    PyObject* pyobjref  = PyObject_GetAttrString(pyservant,
						 (char*)"_omni_objref");
    OMNIORB_ASSERT(pyobjref);
    Py_DECREF(pyservant);

    return pyobjref;
  }

  static PyObject* pyPOA_releaseRef(PyObject* self, PyObject* args)
  {
    PyObject* pyPOA;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyPOA)) return 0;

    CORBA::BOA_ptr boa = (CORBA::BOA_ptr)omniPy::getTwin(pyPOA, BOA_TWIN);
    OMNIORB_ASSERT(boa);

    CORBA::release(boa);

    Py_INCREF(Py_None);
    return Py_None;
  }

  // The PortableServer::Servant::_this() function doesn't really
  // belong here, but it's silly to have a whole func module just for
  // one function.
  static PyObject* pyPOA_servantThis(PyObject* self, PyObject* args)
  {
    PyObject* pyservant;
    if (!PyArg_ParseTuple(args, (char*)"O", &pyservant)) return 0;

    omniPy::Py_Servant* pyos;
    try {
      pyos = omniPy::getServantForPyObject(pyservant);
      RAISE_PY_BAD_PARAM_IF(!pyos);
    }
    catch (CORBA::SystemException& ex) {
      return omniPy::handleSystemException(ex);
    }
    PyObject* pyobjref = PyObject_GetAttrString(pyservant,
						(char*)"_omni_objref");
    OMNIORB_ASSERT(pyobjref);
    return pyobjref;
  }


  ////////////////////////////////////////////////////////////////////////////
  // Python method table                                                    //
  ////////////////////////////////////////////////////////////////////////////

  static PyMethodDef pyPOA_methods[] = {
    {(char*)"create_POA",           pyPOA_create_POA,            METH_VARARGS},
    {(char*)"find_POA",             pyPOA_find_POA,              METH_VARARGS},
    {(char*)"destroy",              pyPOA_destroy,               METH_VARARGS},
    {(char*)"_get_the_name",        pyPOA_get_the_name,          METH_VARARGS},
    {(char*)"_get_the_parent",      pyPOA_get_the_parent,        METH_VARARGS},
    {(char*)"_get_the_children",    pyPOA_get_the_children,      METH_VARARGS},
    {(char*)"_get_the_POAManager",  pyPOA_get_the_POAManager,    METH_VARARGS},
    {(char*)"_get_the_activator",   pyPOA_get_the_activator,     METH_VARARGS},
    {(char*)"_set_the_activator",   pyPOA_set_the_activator,     METH_VARARGS},
    {(char*)"get_servant_manager",  pyPOA_get_servant_manager,   METH_VARARGS},
    {(char*)"set_servant_manager",  pyPOA_set_servant_manager,   METH_VARARGS},
    {(char*)"get_servant",          pyPOA_get_servant,           METH_VARARGS},
    {(char*)"set_servant",          pyPOA_set_servant,           METH_VARARGS},
    {(char*)"activate_object",      pyPOA_activate_object,       METH_VARARGS},
    {(char*)"activate_object_with_id",
                                    pyPOA_activate_object_with_id,
                                                                 METH_VARARGS},
    {(char*)"deactivate_object",    pyPOA_deactivate_object,     METH_VARARGS},
    {(char*)"create_reference",     pyPOA_create_reference,      METH_VARARGS},
    {(char*)"create_reference_with_id",
                                    pyPOA_create_reference_with_id,
                                                                 METH_VARARGS},
    {(char*)"servant_to_id",        pyPOA_servant_to_id,         METH_VARARGS},
    {(char*)"servant_to_reference", pyPOA_servant_to_reference,  METH_VARARGS},
    {(char*)"reference_to_servant", pyPOA_reference_to_servant,  METH_VARARGS},
    {(char*)"reference_to_id",      pyPOA_reference_to_id,       METH_VARARGS},
    {(char*)"id_to_servant",        pyPOA_id_to_servant,         METH_VARARGS},
    {(char*)"id_to_reference",      pyPOA_id_to_reference,       METH_VARARGS},

    {(char*)"releaseRef",           pyPOA_releaseRef,            METH_VARARGS},

    {(char*)"servantThis",          pyPOA_servantThis,           METH_VARARGS},

    {NULL,NULL}
  };
}

void
omniPy::initPOAFunc(PyObject* d)
{
  PyObject* m = Py_InitModule((char*)"_omnipy.poa_func", pyPOA_methods);
  PyDict_SetItemString(d, (char*)"poa_func", m);
}
