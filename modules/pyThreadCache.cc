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
// Revision 1.1.2.3  2001/09/20 10:13:03  dpg1
// Avoid deadlock on exit due to new ORB core cleanup.
//
// Revision 1.1.2.2  2001/08/01 10:12:36  dpg1
// Main thread policy.
//
// Revision 1.1.2.1  2000/10/13 13:55:27  dpg1
// Initial support for omniORB 4.
//


#include <omnipy.h>
#include "pyThreadCache.h"

static int static_cleanup = 0;
// Set true when static data is being destroyed. Used to make sure
// Python things aren't used after they have gone away.

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

class omnipyThreadData : public omni_thread::value_t {
public:
  ~omnipyThreadData() {
    omnipyThreadCache::threadExit();
  }
};


static omnipyThreadScavenger* the_scavenger = 0;
static omni_thread::key_t     omnithread_key;


void
omnipyThreadCache::
init()
{
  omnithread_key = omni_thread::allocate_key();
  guard          = new omni_mutex();
  table          = new CacheNode*[tableSize];
  for (unsigned int i=0; i < tableSize; i++) table[i] = 0;

  the_scavenger  = new omnipyThreadScavenger();
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
  cn->can_scavenge = 1;

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

  omni_thread* ot = omni_thread::self();
  if (ot) {
    if (ot->set_value(omnithread_key, new omnipyThreadData)) {
      cn->can_scavenge = 0;
    }
  }
  return cn;
}


void
omnipyThreadCache::
threadExit()
{
  if (table && !static_cleanup) {
    long         id   = PyThread_get_thread_ident();
    unsigned int hash = id % tableSize; 

    {
      omni_mutex_lock _l(*guard);
      CacheNode* cn = table[hash];
      while (cn && cn->id != id) cn = cn->next;

      if (cn) {
	OMNIORB_ASSERT(!cn->active);
	if (omniORB::trace(20)) {
	  omniORB::logger l;
	  l << "Deleting Python state for thread id " << cn->id
	    << " (thread exit)\n";
	}

	// Acquire Python thread lock and remove Python-world things
	PyEval_AcquireLock();
	PyThreadState* oldState = PyThreadState_Swap(cn->threadState);
	if (cn->workerThread) {
	  PyObject* argtuple = PyTuple_New(1);
	  PyTuple_SET_ITEM(argtuple, 0, cn->workerThread);

	  PyObject* tmp = PyEval_CallObject(omniPy::pyWorkerThreadDel,
					    argtuple);
	  Py_XDECREF(tmp);
	  Py_DECREF(argtuple);
	}
	PyThreadState_Swap(oldState);
	PyThreadState_Clear(cn->threadState);
	PyThreadState_Delete(cn->threadState);
	PyEval_ReleaseLock();

	// Remove the CacheNode
	CacheNode* cnn = cn->next;
	*(cn->back) = cnn;
	if (cnn) cnn->back = cn->back;
	delete cn;
      }
    }
  }
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
	if (cn->can_scavenge && !cn->active) {
	  if (cn->used)
	    cn->used = 0;
	  else {
	    if (omniORB::trace(20)) {
	      omniORB::logger l;
	      l << "Deleting Python state for thread id "
		<< cn->id << " (scavenged)\n";
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
	l << "Deleting Python state for thread id "
	  << cn->id << " (shutdown)\n";
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

  // Remove this thread's Python state
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


class _omnipy_cleapup_detector {
public:
  inline ~_omnipy_cleapup_detector() { static_cleanup = 1; }
};

static _omnipy_cleapup_detector _the_omnipy_cleapup_detector;
