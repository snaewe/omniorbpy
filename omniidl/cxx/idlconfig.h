// -*- c++ -*-
//                          Package   : omniidl
// idlconfig.h              Created on: 2000/03/06
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
// Revision 1.1  2000/03/06 15:16:10  dpg1
// Minor bug fixes to omniidl. New -nf and -k flags.
//
// Revision 1.1.2.1  2000/03/06 15:03:48  dpg1
// Minor bug fixes to omniidl. New -nf and -k flags.
//

#ifndef _idlconfig_h_
#define _idlconfig_h_

#include <idlutil.h>


class Config {
public:
  static _CORBA_Boolean quiet;           // Don't make any output
  static _CORBA_Boolean forwardWarning;  // Warn about unresolved forwards
  static _CORBA_Boolean keepComments;    // Keep comments from source
};


#endif // _idlconfig_h_