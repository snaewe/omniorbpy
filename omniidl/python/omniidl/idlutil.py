# -*- python -*-
#                           Package   : omniidl
# idlutil.py                Created on: 1999/10/27
#			    Author    : Duncan Grisby (dpg1)
#
#    Copyright (C) 1999 AT&T Laboratories Cambridge
#
#  This file is part of omniidl.
#
#  omniidl is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#
# Description:
#   
#   Utility functions

# $Id$
# $Log$
# Revision 1.7  2001/02/21 14:21:44  dpg1
# Merge from omnipy1_develop for 1.3 release.
#
# Revision 1.5.2.1  2000/08/29 15:20:28  dpg1
# New relativeScope() function. New -i flag to enter interactive loop
# after parsing
#
# Revision 1.5  2000/03/03 17:41:30  dpg1
# Major reorganisation to support omniORB 3.0 as well as 2.8.
#
# Revision 1.3  1999/11/15 15:49:23  dpg1
# Documentation strings.
#
# Revision 1.2  1999/11/01 20:18:30  dpg1
# Added string escaping
#
# Revision 1.1  1999/10/29 15:47:07  dpg1
# First revision.
#

"""Utility functions for IDL compilers

escapifyString() -- return a string with non-printing characters escaped.
slashName()      -- format a scoped name with '/' separating components.
dotName()        -- format a scoped name with '.' separating components.
ccolonName()     -- format a scoped name with '::' separating components.
pruneScope()     -- remove common prefix from a scoped name.
relativeScope()  -- give a minimal name for one scope relative to another."""

import string
import _omniidl

def slashName(scopedName, our_scope=[]):
    """slashName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '/' characters. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, "/")

def dotName(scopedName, our_scope=[]):
    """dotName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '.' characters. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, ".")

def ccolonName(scopedName, our_scope=[]):
    """ccolonName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '::' strings. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, "::")

def pruneScope(target_scope, our_scope):
    """pruneScope(list A, list B) -> list

Given two lists of strings (scoped names), return a copy of list A
with any prefix it shares with B removed.

  e.g. pruneScope(['A', 'B', 'C', 'D'], ['A', 'B', 'D']) -> ['C', 'D']"""

    tscope = target_scope[:]
    i = 0
    while len(tscope) > 0 and \
          i < len(our_scope) and \
          tscope[0] == our_scope[i]:
        del tscope[0]
        i = i + 1
    return tscope

def escapifyString(str):
    """escapifyString(string) -> string

Return the given string with any non-printing characters escaped."""
    
    l = list(str)
    vis = string.letters + string.digits + " _!$%^&*()-=+[]{};'#:@~,./<>?|`"
    for i in range(len(l)):
        if l[i] not in vis:
            l[i] = "\\%03o" % ord(l[i])
    return string.join(l, "")


def relativeScope(fromScope, destScope):
    """relativeScope(fromScope, destScope) -> list

Given two globally-scoped names, return a minimal scoped name list
which identifies the destination scope, without clashing with another
identifier. For example, given IDL:

  module M {
    typedef short A;
    typedef long  B;

    module N {
      typedef string B;
      interface I {
        void op(in ::M::A x, in ::M::B y);
      };
    };
  };

relativeScope(["M", "N", "I"], ["M", "A"]) -> ["A"]
relativeScope(["M", "N", "I"], ["M", "B"]) -> ["M", "B"]

If the only valid result is a globally-scoped name, the result list is
prefixed with None:

  module O {
    typedef short C;
  };
  module P {
    module O {
      interface J {
        void op(in ::O::C z);
      };
    };
  };

relativeScope(["P", "O", "J"], ["O", "C"]) -> [None, "O", "C"]

If either scoped name does not exist, returns None."""

    return _omniidl.relativeScopedName(fromScope, destScope)
