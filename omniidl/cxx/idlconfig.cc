// -*- c++ -*-
//                          Package   : omniidl
// idlconfig.cc             Created on: 2000/03/06
//			    Author    : Duncan Grisby (dpg1)
//
//    Copyright (C) 2000 AT&T Laboratories Cambridge
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
//   Global configuration for omniidl

// $Id$
// $Log$
// Revision 1.2  2000/06/05 18:13:25  dpg1
// Comments can be attached to subsequent declarations (with -K). Better
// idea of most recent decl in operation declarations
//
// Revision 1.1.2.1  2000/03/06 15:03:48  dpg1
// Minor bug fixes to omniidl. New -nf and -k flags.
//

#include <idlconfig.h>


_CORBA_Boolean Config::quiet          = 0;
_CORBA_Boolean Config::forwardWarning = 1;
_CORBA_Boolean Config::keepComments   = 0;
_CORBA_Boolean Config::commentsFirst  = 0;
