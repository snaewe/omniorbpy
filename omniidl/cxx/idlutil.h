// -*- c++ -*-
//                          Package   : omniidl
// idlutil.h                Created on: 1999/10/11
//			    Author    : Duncan Grisby (dpg1)
//
//    Copyright (C) 1999 AT&T Laboratories Cambridge
//
//  This file is part of omniidl.
//
//  omniidl is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.
//
// Description:
//   
//   Utility functions

// $Id$
// $Log$
// Revision 1.1  1999/10/27 14:05:53  dpg1
// *** empty log message ***
//

#ifndef _idlutil_h_
#define _idlutil_h_

#include <assert.h>

// Types
#include <CORBA_sysdep.h>    // ***
#include <CORBA_basetypes.h> // ***

#ifdef HAS_LongLong
typedef _CORBA_ULongLong IdlIntLiteral;
#else
typedef _CORBA_ULong IdlIntLiteral;
#endif

#ifdef HAS_LongDouble
typedef _CORBA_LongDouble IdlFloatLiteral;
#else
typedef _CORBA_Double IdlFloatLiteral;
#endif

// Version of strdup which uses new
char* idl_strdup(const char* s);
_CORBA_WChar* idl_wstrdup(const _CORBA_WChar* s);

// strlen, strcpy and strcat for wstring
int           idl_wstrlen(const _CORBA_WChar* s);
_CORBA_WChar* idl_wstrcpy(_CORBA_WChar* a, const _CORBA_WChar* b);
_CORBA_WChar* idl_wstrcat(_CORBA_WChar* a, const _CORBA_WChar* b);

#endif // _idlutil_h_
