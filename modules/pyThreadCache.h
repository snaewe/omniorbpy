// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// pyThreadCache.h            Created on: 2000/05/26
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
// Revision 1.1.2.4  2005/02/22 11:02:37  dgrisby
// On Python >= 2.3, the thread state cache could incorrectly cache
// thread state for a thread that had exited, and a new one started with
// the same id.
//
// Revision 1.1.2.3  2005/01/24 17:28:58  dgrisby
// Unbelievably unlikely race condition in thread cache / Python worker
// thread creation. Of course it happened anyway.
//
// Revision 1.1.2.2  2003/07/29 14:52:10  dgrisby
// Reuse Python thread state if possible. (Python 2.3.)
//
// Revision 1.1.2.1  2000/10/13 13:55:27  dpg1
// Initial support for omniORB 4.
//

#if defined(__VMS)
#include <pythread.h>
#else
#include PYTHON_THREAD_INC
#endif


class omnipyThreadCache {
public:

  static omni_mutex* guard;

  static void init();
  static void shutdown();

  struct CacheNode {
    long           id;
    PyThreadState* threadState;
    PyObject*      workerThread;

    CORBA::Boolean used;
    CORBA::Boolean can_scavenge;
    CORBA::Boolean reused_state;
    int            active;

    CacheNode*     next;
    CacheNode**    back;
  };

  // Fixed-size open hash table of cacheNodes
  static const unsigned int tableSize;
  static CacheNode**        table;

  // Time in seconds between runs of the node scavenger
  static unsigned int       scanPeriod;

  // Class lock acquires the Python interpreter lock when it is
  // created, and releases it again when it is deleted.
  class lock {
  public:
    inline lock() {
      long id    = PyThread_get_thread_ident();
      cacheNode_ = acquireNode(id);
      PyEval_AcquireLock();
      oldstate_  = PyThreadState_Swap(cacheNode_->threadState);
    }

    inline ~lock() {
      PyThreadState_Swap(oldstate_);
      PyEval_ReleaseLock();
      releaseNode(cacheNode_);
    }
  private:
    CacheNode*     cacheNode_;
    PyThreadState* oldstate_;
  };


  static inline CacheNode* acquireNode(long id) {
    unsigned int hash = id % tableSize; 
    CacheNode* cn;
    {
      omni_mutex_lock _l(*guard);
      OMNIORB_ASSERT(table);

      cn = table[hash];
      while (cn && cn->id != id) cn = cn->next;
      if (cn) {
	cn->used = 1;
	cn->active++;
#if PY_VERSION_HEX >= 0x02030000
	if (cn->reused_state) {
	  cn->threadState = PyGILState_GetThisThreadState();
	  OMNIORB_ASSERT(cn->threadState);
	}
#endif
	return cn;
      }
    }
    return addNewNode(id, hash);
  }

  static inline void releaseNode(CacheNode* cn) {
    omni_mutex_lock _l(*guard);
    cn->used = 1;
    cn->active--;
  }

  static CacheNode* addNewNode(long id, unsigned int hash);

  static void threadExit();
};
