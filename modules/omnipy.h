// -*- Mode: C++; -*-
//                            Package   : omniORBpy
// omnipy.h                   Created on: 2000/02/24
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
//    Master header file for omnipy internals

#ifndef _omnipy_h_
#define _omnipy_h_


#if   defined(OMNIORBPY_FOR_30)
#  include "omni30/omnipy30.h"
#elif defined(OMNIORBPY_FOR_28)
#  include "omni28/omnipy28.h"
#else
#  error "No omniORB version specified!"
#endif


#ifdef HAS_Cplusplus_catch_exception_by_base

#define OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS \
catch (const CORBA::SystemException& ex) { \
  return omniPy::handleSystemException(ex); \
}
#else

#ifndef OMNIORB_FOR_EACH_SYS_EXCEPTION
#define OMNIORB_FOR_EACH_SYS_EXCEPTION(doit) \
 \
doit (UNKNOWN) \
doit (BAD_PARAM) \
doit (NO_MEMORY) \
doit (IMP_LIMIT) \
doit (COMM_FAILURE) \
doit (INV_OBJREF) \
doit (OBJECT_NOT_EXIST) \
doit (NO_PERMISSION) \
doit (INTERNAL) \
doit (MARSHAL) \
doit (INITIALIZE) \
doit (NO_IMPLEMENT) \
doit (BAD_TYPECODE) \
doit (BAD_OPERATION) \
doit (NO_RESOURCES) \
doit (NO_RESPONSE) \
doit (PERSIST_STORE) \
doit (BAD_INV_ORDER) \
doit (TRANSIENT) \
doit (FREE_MEM) \
doit (INV_IDENT) \
doit (INV_FLAG) \
doit (INTF_REPOS) \
doit (BAD_CONTEXT) \
doit (OBJ_ADAPTER) \
doit (DATA_CONVERSION) \
doit (TRANSACTION_REQUIRED) \
doit (TRANSACTION_ROLLEDBACK) \
doit (INVALID_TRANSACTION) \
doit (WRONG_TRANSACTION)

#endif

#define OMNIPY_CATCH_AND_HANDLE_SPECIFIED_EXCEPTION(exc) \
catch (const CORBA::exc& ex) { \
  return omniPy::handleSystemException(ex); \
}
#define OMNIPY_CATCH_AND_HANDLE_SYSTEM_EXCEPTIONS \
  OMNIORB_FOR_EACH_SYS_EXCEPTION(OMNIPY_CATCH_AND_HANDLE_SPECIFIED_EXCEPTION)

#endif


#endif // _omnipy_h_
