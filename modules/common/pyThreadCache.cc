// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyThreadCache.cc           Created on: 2000/05/26
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
//    Cached mapping from threads to PyThreadState and
//    threading.Thread objects

// $Id$

// $Log$
// Revision 1.3  2000/06/02 09:59:53  dpg1
// Thread cache now calls PyThreadState_Clear() when it is deleting a
// thread state
//
// Revision 1.2  2000/05/30 08:57:10  dpg1
// Accidentally set hash table size to 3 elements.
//
// Revision 1.1  2000/05/26 15:33:31  dpg1
// Python thread states are now cached. Operation dispatch time is
// roughly halved!
//


#include <omnipy.h>
#include "pyThreadCache.h"


omni_mutex*                    omnipyThreadCache::guard      = 0;
const unsigned int             omnipyThreadCache::tableSize  = 67;
omnipyThreadCache::CacheNode** omnipyThreadCache::table      = 0;
unsigned int                   omnipyThreadCache::scanPeriod = 30;

class omnipyThreadScavenger : public omni_thread {
public:
  omnipyThreadScavenger() : dying_(0), cond_(omnipyThreadCache::guard) {
    start_undetached();
  }
  ~omnipyThreadScavenger() { }

  void kill() {
    {
      omni_mutex_lock l(*omnipyThreadCache::guard);
      dying_ = 1;
      cond_.signal();
    }
    join(0);
  }

  void* run_undetached(void*);
private:
  CORBA::Boolean dying_;
  omni_condition cond_;
  PyThreadState* threadState_;
  PyObject*      workerThread_;
};

static omnipyThreadScavenger* the_scavenger = 0;


void
omnipyThreadCache::
init()
{
  guard         = new omni_mutex();
  table         = new CacheNode*[tableSize];
  for (unsigned int i=0; i < tableSize; i++) table[i] = 0;

  the_scavenger = new omnipyThreadScavenger();
}


void
omnipyThreadCache::
shutdown()
{
  if (the_scavenger) the_scavenger->kill();
  the_scavenger = 0;

  if (table) delete[] table;
  delete guard;
  table = 0;
  guard = 0;
}


omnipyThreadCache::CacheNode*
omnipyThreadCache::
addNewNode(long id, unsigned int hash)
{
  CacheNode* cn    = new CacheNode;
  cn->id           = id;

  PyEval_AcquireLock();
  cn->threadState  = PyThreadState_New(omniPy::pyInterpreter);

  PyThreadState* oldState = PyThreadState_Swap(cn->threadState);

  cn->workerThread = PyEval_CallObject(omniPy::pyWorkerThreadClass,
				       omniPy::pyEmptyTuple);
  PyThreadState_Swap(oldState);
  PyEval_ReleaseLock();

  cn->used         = 0;
  cn->active       = 0;

  // Insert into hash table
  CacheNode* he = table[hash];
  cn->next = he;
  cn->back = &(table[hash]);
  if (he) he->back = &(cn->next);
  table[hash] = cn;

  if (omniORB::trace(20)) {
    omniORB::logger l;
    l << "Creating new Python state for thread id " << id << "\n";
  }
  return cn;
}


void*
omnipyThreadScavenger::
run_undetached(void*)
{
  unsigned long abs_sec, abs_nsec;
  unsigned int  i;
  omnipyThreadCache::CacheNode *cn, *cnn;

  omniORB::logs(15, "Python thread state scavenger start.");

  omni_mutex_lock l(*omnipyThreadCache::guard);

  // Create a thread state for the scavenger thread itself
  PyThreadState* oldState;
  PyEval_AcquireLock();
  threadState_  = PyThreadState_New(omniPy::pyInterpreter);
  oldState      = PyThreadState_Swap(threadState_);
  workerThread_ = PyEval_CallObject(omniPy::pyWorkerThreadClass,
				    omniPy::pyEmptyTuple);
  PyThreadState_Swap(oldState);
  PyEval_ReleaseLock();

  // Main loop
  while (!dying_) {
    omni_thread::get_time(&abs_sec,&abs_nsec);
    abs_sec += omnipyThreadCache::scanPeriod;
    cond_.timedwait(abs_sec, abs_nsec);

    if (dying_) break;

    omniORB::logs(15, "Scanning Python thread states.");
    
    for (i=0; i < omnipyThreadCache::tableSize; i++) {
      cn = omnipyThreadCache::table[i];

      while (cn) {
	if (!cn->active) {
	  if (cn->used)
	    cn->used = 0;
	  else {
	    if (omniORB::trace(20)) {
	      omniORB::logger l;
	      l << "Deleting Python state for thread id " << cn->id << "\n";
	    }

	    // Acquire Python thread lock and remove Python-world things
	    PyEval_AcquireLock();
	    oldState = PyThreadState_Swap(threadState_);
	    if (cn->workerThread) {
	      PyObject* argtuple = PyTuple_New(1);
	      PyTuple_SET_ITEM(argtuple, 0, cn->workerThread);

	      PyObject* tmp = PyEval_CallObject(omniPy::pyWorkerThreadDel,
						argtuple);
	      Py_XDECREF(tmp);
	      Py_DECREF(argtuple);
	    }
	    PyThreadState_Clear(cn->threadState);
	    PyThreadState_Delete(cn->threadState);
	    PyThreadState_Swap(oldState);
	    PyEval_ReleaseLock();

	    // Remove the CacheNode
	    cnn = cn->next;
	    *(cn->back) = cnn;
	    if (cnn) cnn->back = cn->back;
	    delete cn;
	    cn = cnn;
	    continue;
	  }
	}
	cn = cn->next;
      }
    }
  }

  // Delete all table entries
  PyEval_AcquireLock();
  oldState = PyThreadState_Swap(threadState_);

  for (i=0; i < omnipyThreadCache::tableSize; i++) {
    cn = omnipyThreadCache::table[i];

    while (cn) {
      if (omniORB::trace(20)) {
	omniORB::logger l;
	l << "Deleting Python state for thread id " << cn->id << "\n";
      }

      if (cn->workerThread) {
	PyObject* argtuple = PyTuple_New(1);
	PyTuple_SET_ITEM(argtuple, 0, cn->workerThread);

	PyObject* tmp = PyEval_CallObject(omniPy::pyWorkerThreadDel,
					  argtuple);
	Py_XDECREF(tmp);
	Py_DECREF(argtuple);
      }
      PyThreadState_Clear(cn->threadState);
      PyThreadState_Delete(cn->threadState);

      // Remove the CacheNode
      cnn = cn->next;
      delete cn;
      cn = cnn;
    }
  }
  PyThreadState_Swap(oldState);
  PyEval_ReleaseLock();


  // Remove this thread's Python state
  PyEval_AcquireLock();
  oldState = PyThreadState_Swap(threadState_);
  if (workerThread_) {
    PyObject* argtuple = PyTuple_New(1);
    PyTuple_SET_ITEM(argtuple, 0, workerThread_);

    PyObject* tmp = PyEval_CallObject(omniPy::pyWorkerThreadDel,
				      argtuple);
    Py_XDECREF(tmp);
    Py_DECREF(argtuple);
  }
  PyThreadState_Swap(oldState);
  PyThreadState_Clear(threadState_);
  PyThreadState_Delete(threadState_);
  PyEval_ReleaseLock();

  omniORB::logs(15, "Python thread state scavenger exit.");

  return 0;
}
