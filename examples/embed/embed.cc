// embed.cc -- C++ part of embedding test

#include <iostream.h>
#include <echo.hh>
#include PYTHON_INCLUDE


#ifdef __WIN32__
#define DLL_EXPORT _declspec(dllexport)
#else
#define DLL_EXPORT
#endif

class Echo_i : public POA_Echo,
	       public PortableServer::RefCountServantBase
{
public:
  inline Echo_i() {}
  virtual ~Echo_i() {}
  virtual char* echoString(const char* mesg);
};


char* Echo_i::echoString(const char* mesg)
{
  cout << "C++ upcall '" << mesg << "'" << endl;
  return CORBA::string_dup(mesg);
}

class InterpreterUnlocker {
public:
  InterpreterUnlocker() {
    tstate_ = PyEval_SaveThread();
  }
  ~InterpreterUnlocker() {
    PyEval_RestoreThread(tstate_);
  }
private:
  PyThreadState* tstate_;
};

extern "C" {

  static PyObject* EmbedGetIOR(PyObject* self, PyObject* args)
  {
    if (!PyArg_ParseTuple(args, (char*)"")) return 0;

    int   argc   = 1;
    char* argv[] = { "embed" };

    CORBA::ORB_var          orb = CORBA::ORB_init(argc, argv);
    CORBA::Object_var       obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

    Echo_i* myecho = new Echo_i();
      
    PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

    obj = myecho->_this();
    CORBA::String_var sior(orb->object_to_string(obj));

    myecho->_remove_ref();

    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();

    return Py_BuildValue((char*)"s", (char*)sior);
  }

  static PyObject* EmbedPutIOR(PyObject* self, PyObject* args)
  {
    char* ior;
    if (!PyArg_ParseTuple(args, (char*)"s", &ior)) return 0;

    int   argc   = 1;
    char* argv[] = { "embed" };

    CORBA::ORB_var    orb = CORBA::ORB_init(argc, argv);
    CORBA::Object_var obj = orb->string_to_object(ior);

    Echo_var eobj = Echo::_narrow(obj);

    if (CORBA::is_nil(eobj)) {
      cout << "C++ received an objref which was not an Echo" << endl;
      Py_INCREF(Py_None);
      return Py_None;
    }

    {
      InterpreterUnlocker _u;
      cout << "\nTrying call to Python object..." << endl;
      CORBA::String_var ret = eobj->echoString("Hello from C++");
      cout << "The result was '" << (char*)ret << "'" << endl;
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyMethodDef embed_methods[] = {
    {(char*)"getIOR", EmbedGetIOR, METH_VARARGS},
    {(char*)"putIOR", EmbedPutIOR, METH_VARARGS},
    {NULL, NULL}
  };

  void DLL_EXPORT init_embed()
  {
    PyObject* m = Py_InitModule((char*)"_embed", embed_methods);
  }
}
