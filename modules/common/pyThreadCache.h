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
// Revision 1.2  2000/10/02 17:35:01  dpg1
// Merge for 1.2 release
//
// Revision 1.1.2.1  2000/09/27 15:57:21  dpg1
// #include<python1.5/pythread.h> generalised for other Python versions
//
// Revision 1.1  2000/05/26 15:33:31  dpg1
// Python thread states are now cached. Operation dispatch time is
// roughly halved!
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
    CORBA::Boolean active;

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

      cn = table[hash];
      while (cn && cn->id != id) cn = cn->next;
      if (!cn) cn = addNewNode(id, hash);

      cn->used   = 1;
      cn->active = 1;
    }
    return cn;
  }

  static inline void releaseNode(CacheNode* cn) {
    omni_mutex_lock _l(*guard);
    cn->used   = 1;
    cn->active = 0;
  }

  static CacheNode* addNewNode(long id, unsigned int hash);
};
