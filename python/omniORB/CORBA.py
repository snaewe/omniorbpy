# -*- Mode: Python; -*-
#                            Package   : omniORBpy
# CORBA.py                   Created on: 1999/06/08
#                            Author    : Duncan Grisby (dpg1)
#
#    Copyright (C) 1999 AT&T Laboratories Cambridge
#
#    This file is part of the omniORBpy library
#
#    The omniORBpy library is free software; you can redistribute it
#    and/or modify it under the terms of the GNU Lesser General
#    Public License as published by the Free Software Foundation;
#    either version 2.1 of the License, or (at your option) any later
#    version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free
#    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
#    MA 02111-1307, USA
#
#
# Description:
#    Definitions for CORBA module


# $Id$

# $Log$
# Revision 1.8  1999/09/24 13:28:37  dpg1
# RootPOA added to list_initial_services() list.
#
# Revision 1.7  1999/09/24 09:22:01  dpg1
# Added copyright notices.
#
# Revision 1.6  1999/09/22 15:46:11  dpg1
# Fake POA implemented.
#
# Revision 1.5  1999/09/13 14:51:46  dpg1
# Any coercion implemented. TypeCode() constructor now complies to
# latest spec.
#
# Revision 1.4  1999/09/13 09:55:02  dpg1
# Initial references support. __methods__ added.
#
# Revision 1.3  1999/07/29 14:16:56  dpg1
# Server side, TypeCode creation interface.
#
# Revision 1.2  1999/07/19 15:47:05  dpg1
# TypeCode and Any support.
#
# Revision 1.1  1999/06/08 16:21:52  dpg1
# Initial revision
#


import threading, types, exceptions

import _omnipy
import omniORB


#############################################################################
#                                                                           #
# MISCELLANEOUS STUFF                                                       #
#                                                                           #
#############################################################################

TRUE  = 1
FALSE = 0



#############################################################################
#                                                                           #
# EXCEPTIONS                                                                #
#                                                                           #
#############################################################################

class Exception (exceptions.Exception):
    pass

# Completion status:

COMPLETED_YES     = omniORB.EnumItem("COMPLETED_YES",   0)
COMPLETED_NO      = omniORB.EnumItem("COMPLETED_NO",    1)
COMPLETED_MAYBE   = omniORB.EnumItem("COMPLETED_MAYBE", 2)
completion_status = omniORB.Enum("IDL:omg.org/CORBA/completion_status:1.0",
                                 (COMPLETED_YES,
                                  COMPLETED_NO,
                                  COMPLETED_MAYBE))

class SystemException (Exception):
    def __init__(self, minor=0, completed=COMPLETED_NO):
        self.minor = minor
        if type(completed) == types.IntType:
            self.completed = completion_status._item(completed)
        else:
            self.completed = completed

    def __str__(self):
        return "Minor: " + str(self.minor) + \
               ", Completed: " + str(self.completed) + "."

class UserException (Exception):
    pass


# All the standard system exceptions...

class UNKNOWN (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/UNKNOWN"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class BAD_PARAM (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_PARAM"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class NO_MEMORY (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_MEMORY"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)
        return

class IMP_LIMIT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/IMP_LIMIT"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class COMM_FAILURE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/COMM_FAILURE"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INV_OBJREF (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INV_OBJREF"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class OBJECT_NOT_EXIST (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/OBJECT_NOT_EXIST"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class NO_PERMISSION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_PERMISSION"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INTERNAL (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INTERNAL"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class MARSHAL (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/MARSHAL"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INITIALIZE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INITIALIZE"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class NO_IMPLEMENT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_IMPLEMENT"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class BAD_TYPECODE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_TYPECODE"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class BAD_OPERATION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_OPERATION"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class NO_RESOURCES (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_RESOURCES"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class NO_RESPONSE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_RESPONSE"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class PERSIST_STORE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/PERSIST_STORE"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class BAD_INV_ORDER (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_INV_ORDER"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class TRANSIENT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/TRANSIENT"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class FREE_MEM (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/FREE_MEM"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INV_IDENT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INV_IDENT"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INV_FLAG (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INV_FLAG"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INTF_REPOS (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INTF_REPOS"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class BAD_CONTEXT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_CONTEXT"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class OBJ_ADAPTER (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/OBJ_ADAPTER"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class DATA_CONVERSION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/DATA_CONVERSION"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class TRANSACTION_REQUIRED (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/TRANSACTION_REQUIRED"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class TRANSACTION_ROLLEDBACK (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class INVALID_TRANSACTION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INVALID_TRANSACTION"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)

class WRONG_TRANSACTION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/WRONG_TRANSACTION"
    def __init__(self, minor=0, completed=COMPLETED_NO):
        SystemException.__init__(self, minor, completed)


#############################################################################
#                                                                           #
# TypeCode                                                                  #
#                                                                           #
#############################################################################

tk_null               = omniORB.EnumItem("CORBA.tk_null",               0)
tk_void               = omniORB.EnumItem("CORBA.tk_void",               1)
tk_short              = omniORB.EnumItem("CORBA.tk_short",              2)
tk_long               = omniORB.EnumItem("CORBA.tk_long",               3)
tk_ushort             = omniORB.EnumItem("CORBA.tk_ushort",             4)
tk_ulong              = omniORB.EnumItem("CORBA.tk_ulong",              5)
tk_float              = omniORB.EnumItem("CORBA.tk_float",              6)
tk_double             = omniORB.EnumItem("CORBA.tk_double",             7)
tk_boolean            = omniORB.EnumItem("CORBA.tk_boolean",            8)
tk_char	              = omniORB.EnumItem("CORBA.tk_char",               9)
tk_octet              = omniORB.EnumItem("CORBA.tk_octet",              10)
tk_any	              = omniORB.EnumItem("CORBA.tk_any",        	11)
tk_TypeCode           = omniORB.EnumItem("CORBA.tk_TypeCode",           12)
tk_Principal          = omniORB.EnumItem("CORBA.tk_Principal",          13)
tk_objref             = omniORB.EnumItem("CORBA.tk_objref",             14)
tk_struct             = omniORB.EnumItem("CORBA.tk_struct",             15)
tk_union              = omniORB.EnumItem("CORBA.tk_union",              16)
tk_enum	              = omniORB.EnumItem("CORBA.tk_enum",               17)
tk_string             = omniORB.EnumItem("CORBA.tk_string",             18)
tk_sequence           = omniORB.EnumItem("CORBA.tk_sequence",           19)
tk_array              = omniORB.EnumItem("CORBA.tk_array",              20)
tk_alias              = omniORB.EnumItem("CORBA.tk_alias",              21)
tk_except             = omniORB.EnumItem("CORBA.tk_except",             22)
tk_longlong           = omniORB.EnumItem("CORBA.tk_longlong",           23)
tk_ulonglong          = omniORB.EnumItem("CORBA.tk_ulonglong",          24)
tk_longdouble         = omniORB.EnumItem("CORBA.tk_longdouble",         25)
tk_wchar              = omniORB.EnumItem("CORBA.tk_wchar",              26)
tk_wstring            = omniORB.EnumItem("CORBA.tk_wstring",            27)
tk_fixed              = omniORB.EnumItem("CORBA.tk_fixed",              28)
tk_value              = omniORB.EnumItem("CORBA.tk_value",              29)
tk_value_box          = omniORB.EnumItem("CORBA.tk_value_box",          30)
tk_native             = omniORB.EnumItem("CORBA.tk_native",             31)
tk_abstract_interface = omniORB.EnumItem("CORBA.tk_abstract_interface", 32)

TCKind = omniORB.Enum("IDL:omg.org/CORBA/TCKind:1.0",
                      (tk_null, tk_void, tk_short, tk_long, tk_ushort,
                       tk_ulong, tk_float, tk_double, tk_boolean,
                       tk_char, tk_octet, tk_any, tk_TypeCode,
                       tk_Principal, tk_objref, tk_struct, tk_union,
                       tk_enum, tk_string, tk_sequence, tk_array,
                       tk_alias, tk_except, tk_longlong, tk_ulonglong,
                       tk_longdouble, tk_wchar, tk_wstring, tk_fixed,
                       tk_value, tk_value_box, tk_native,
                       tk_abstract_interface))

class TypeCode:
    class Bounds (UserException):
        pass
    class BadKind (UserException):
        pass

    def __init__(self, cr):
        self._t = tcInternal.typeCodeFromClassOrRepoId(cr)
        self._d = self._t._d
        self._k = self._t._k

    def equal(self, tc):                return self._t.equal(tc)
    def equivalent(self, tc):           return self._t.equivalent(tc)
    def get_compact_typecode(self):     return self._t.get_compact_typecode()
    def kind(self):                     return self._t.kind()
    
    # Operations which are only available for some kinds:
    def id(self):                       return self._t.id()
    def name(self):                     return self._t.name()
    def member_count(self):             return self._t.member_count()
    def member_name(self, index):       return self._t.member_name(index)
    def member_type(self, index):       return self._t.member_type(index)
    def member_label(self, index):      return self._t.member_label(index)

    def discriminator_type(self):       return self._t.discriminator_type()
    def default_index(self):            return self._t.default_index()
    def length(self):                   return self._t.length()
    def content_type(self):             return self._t.content_type()

    # Things for types we don't support:
    def fixed_digits(self):             return self._t.fixed_digits()
    def fixed_scale(self):              return self._t.fixed_scale()
    def member_visibility(self, index): return self._t.member_visibility(index)
    def type_modifier(self):            return self._t.type_modifier()
    def concrete_base_type(self):       return self._t.concrete_base_type()

    __methods__ = ["equal", "equivalent", "get_compact_typecode",
                   "kind", "id", "name", "member_count", "member_name",
                   "member_type", "member_label", "discriminator_type",
                   "default_index", "length", "content_type"]

import tcInternal
_d_TypeCode = tcInternal.tv_TypeCode


# TypeCodes of basic types:

_tc_null      = tcInternal.createTypeCode(tcInternal.tv_null)
_tc_void      = tcInternal.createTypeCode(tcInternal.tv_void)
_tc_short     = tcInternal.createTypeCode(tcInternal.tv_short)
_tc_long      = tcInternal.createTypeCode(tcInternal.tv_long)
_tc_ushort    = tcInternal.createTypeCode(tcInternal.tv_ushort)
_tc_ulong     = tcInternal.createTypeCode(tcInternal.tv_ulong)
_tc_float     = tcInternal.createTypeCode(tcInternal.tv_float)
_tc_double    = tcInternal.createTypeCode(tcInternal.tv_double)
_tc_boolean   = tcInternal.createTypeCode(tcInternal.tv_boolean)
_tc_char      = tcInternal.createTypeCode(tcInternal.tv_char)
_tc_octet     = tcInternal.createTypeCode(tcInternal.tv_octet)
_tc_any       = tcInternal.createTypeCode(tcInternal.tv_any)
_tc_TypeCode  = tcInternal.createTypeCode(tcInternal.tv_TypeCode)
_tc_Principal = tcInternal.createTypeCode(tcInternal.tv_Principal)
_tc_string    = tcInternal.createTypeCode((tcInternal.tv_string,0))


#############################################################################
#                                                                           #
# Any                                                                       #
#                                                                           #
#############################################################################

class Any:
    def __init__(self, t, v):
        if not isinstance(t, TypeCode):
            raise TypeError("Argument 1 must be a TypeCode.")
        self._t = t
        self._v = v

    def typecode(self):
        return self._t

    def value(self, coerce=None):
        if coerce is None:
            return self._v

        if not isinstance(coerce, TypeCode):
            raise TypeError("Argument 1 must be a TypeCode if present.")

        return omniORB.coerceAny(self._v, self._t._d, coerce._d)

    __methods__ = ["typecode", "value"]

_d_any = tcInternal.tv_any


#############################################################################
#                                                                           #
# ORB                                                                       #
#                                                                           #
#############################################################################

ORB_ID = "omniORB2"
BOA_ID = "omniORB2_BOA"

def ORB_init(argv=[], orb_identifier = ORB_ID):
    omniORB.orb_lock.acquire()
    try:
        if omniORB.orb is None:
            omniORB.orb = ORB(argv, orb_identifier)
    finally:
        omniORB.orb_lock.release()

    return omniORB.orb


class ORB:
    """ omnipy ORB object """

    def __init__(self, argv, orb_identifier):
        _omnipy.ORB_init(self, argv, orb_identifier)
        self.__argv = argv
        self.__poa  = None

#    def BOA_init(self, argv=[], boa_identifier = BOA_ID):
#        return BOA(self, argv, boa_identifier)

    def string_to_object(self, ior):
        return _omnipy.stringToObject(self, ior)

    def object_to_string(self, obj):
        return _omnipy.objectToString(self, obj)

    def list_initial_services(self):
        isl = _omnipy.listInitialServices(self)
        isl.append("RootPOA")
        return isl

    def resolve_initial_references(self, identifier):

        if identifier == "RootPOA":
            if self.__poa is None:
                self.__poa = omniORB.PortableServer.POA(self, self.__argv)
            return self.__poa

        return _omnipy.resolveInitialReferences(self, identifier)

    # TypeCode operations
    def create_struct_tc(self, id, name, members):
        return tcInternal.createStructTC(id, name, members)

    def create_union_tc(self, id, name, discriminator_type, members):
        return tcInternal.createUnionTC(id, name, discriminator_type, members)

    def create_enum_tc(self, id, name, members):
        return tcInternal.createEnumTC(id, name, members)

    def create_alias_tc(self, id, name, original_type):
        return tcInternal.createAliasTC(id, name, original_type)

    def create_exception_tc(self, id, name, members):
        return tcInternal.createExceptionTC(id, name, members)

    def create_interface_tc(self, id, name):
        return tcInternal.createInterfaceTC(id, name)

    def create_string_tc(self, bound):
        return tcInternal.createStringTC(bound)

    def create_sequence_tc(self, bound, element_type):
        return tcInternal.createSequenceTC(bound, element_type)
    
    def create_array_tc(self, length, element_type):
        return tcInternal.createArrayTC(length, element_type)

    def create_recursive_tc(self, id):
        return tcInternal.createRecursiveTC(id)

    def run(self):
        poa = self.resolve_initial_references("RootPOA")
        _omnipy.implIsReady(poa, 0, 0)


    __methods__ = ["string_to_object", "object_to_string",
                   "list_initial_services", "resolve_initial_references",
                   "create_struct_tc", "create_union_tc",
                   "create_enum_tc", "create_alias_tc",
                   "create_exception_tc", "create_interface_tc",
                   "create_string_tc", "create_sequence_tc",
                   "create_array_tc", "create_recursive_tc",
                   "run"]


#############################################################################
#                                                                           #
# BOA                                                                       #
#                                                                           #
#############################################################################

#  class BOA:
#      """
#      omnipy BOA object

#      Soon to be superceded by the POA...
#      """

#      def __init__(self, orb, argv, boa_identifier):
#          _omnipy.BOA_init(self, orb, argv, boa_identifier)
#          print "BOA initialised"

#      def obj_is_ready(self, obj):
#          _omnipy.objectIsReady(self, obj)
#          print "Object activated"

#      def impl_is_ready(self, z=0, noblock=0):
#          _omnipy.implIsReady(self, z, noblock)

#      __methods__ = ["obj_is_ready", "impl_is_ready"]


#############################################################################
#                                                                           #
# OBJECT                                                                    #
#                                                                           #
#############################################################################


class Object:
    """ CORBA::Object base class """

    _NP_RepositoryId = ""

    _nil = None

    def __init__(self):
        pass

    def __del__(self):
        pass

    def _get_interface(self):
        # ***
        raise NO_IMPLEMENT
    
    def _is_a(self, repoId):
        if self._NP_RepositoryId == repoId: return TRUE
        return _omnipy.isA(self, repoId)
    
    def _non_existent(self):
        return _omnipy.nonExistent(self)
    
    def _is_equivalent(self, other_object):
        if self == other_object: return TRUE
        return _omnipy.isEquivalent(self, other_object)
    
    def _hash(self, maximum):
        return _omnipy.hash(self, maximum)
    
    def _duplicate(self, obj):
        return self
    
    def _release(self):
        return

    def _narrow(self, dest):
        return _omnipy.narrow(self, dest._NP_RepositoryId)

    __methods__ = ["_is_a", "_non_existent", "_is_equivalent",
                   "_hash", "_narrow"]

_d_Object  = (omniORB.tcInternal.tv_objref, Object._NP_RepositoryId, "Object")
_tc_Object = omniORB.tcInternal.createTypeCode(_d_Object)


class _nil_Object (Object):
    """ Class for all nil objects """

    _NP_RepositoryId = ""

    def __nonzero__(self):
        return 0

    def _is_a(self, repoId):
        if repoId == "": return TRUE
        return FALSE

    def _non_existent(self):
        return TRUE

    def _is_equivalent(self, other_object):
        if is_nil(other_object): return TRUE
        return FALSE

    def _hash(self, maximum):
        return 0

    def _narrow(self, dest):
        return self

    __methods__ = Object.__methods__

Object._nil = _nil_Object()



#############################################################################
#                                                                           #
# CORBA module functions                                                    #
#                                                                           #
#############################################################################

def is_nil(obj):
    if isinstance(obj, _nil_Object): return TRUE
    return FALSE


#############################################################################
#                                                                           #
# Interface Repository stuff                                                #
#                                                                           #
#############################################################################

# typedef string Identifier
_d_Identifier  = (tcInternal.tv_string, 0)
_ad_Identifier = (tcInternal.tv_alias,
                  "IDL:omg.org/CORBA/Identifier:1.0",
                  "Identifier", _d_Identifier)
_tc_Identifier = tcInternal.createTypeCode(_ad_Identifier)

# typedef string ScopedName
_d_ScopedName  = (tcInternal.tv_string, 0)
_ad_ScopedName = (tcInternal.tv_alias,
                  "IDL:omg.org/CORBA/ScopedName:1.0",
                  "ScopedName", _d_ScopedName)
_tc_ScopedName = tcInternal.createTypeCode(_ad_ScopedName)

# typedef string RepositoryId
_d_RepositoryId  = (tcInternal.tv_string, 0)
_ad_RepositoryId = (tcInternal.tv_alias,
                    "IDL:omg.org/CORBA/RepositoryId:1.0",
                    "RepositoryId", _d_RepositoryId)
_tc_RepositoryId = tcInternal.createTypeCode(_ad_RepositoryId)

# interface IDLType;
_d_IDLType = (tcInternal.tv_objref,
              "IDL:omg.org/CORBA/IDLType:2.3",
              "IDLType")


# struct StructMember
class StructMember:
    _NP_RepositoryId = "IDL:omg.org/CORBA/StructMember:1.0"

    def __init__(self, name, type, type_def):
        self.name     = name
        self.type     = type
        self.type_def = type_def

_d_StructMember  = (tcInternal.tv_struct, StructMember,
                    StructMember._NP_RepositoryId, "StructMember",
                    "name",     _d_Identifier,
                    "type",     _d_TypeCode,
                    "type_def", _d_IDLType)
_tc_StructMember = tcInternal.createTypeCode(_d_StructMember)

# typedef sequence <StructMember> StructMemberSeq;
_d_StructMemberSeq  = (omniORB.tcInternal.tv_sequence, _d_StructMember, 0)
_ad_StructMemberSeq = (omniORB.tcInternal.tv_alias,
                       "IDL:omg.org/CORBA/StructMemberSeq:1.0",
                       "StructMemberSeq", _d_StructMemberSeq)
_tc_StructMemberSeq = omniORB.tcInternal.createTypeCode(_ad_StructMemberSeq)

# struct UnionMember
class UnionMember:
    _NP_RepositoryId = "IDL:omg.org/CORBA/UnionMember:1.0"

    def __init__(self, name, label, type, type_def):
        self.name     = name
        self.label    = label
        self.type     = type
        self.type_def = type_def

_d_UnionMember  = (tcInternal.tv_struct, UnionMember,
                   UnionMember._NP_RepositoryId, "UnionMember",
                   "name",     _d_Identifier,
                   "label",    _d_any,
                   "type",     _d_TypeCode,
                   "type_def", _d_IDLType)
_tc_UnionMember = tcInternal.createTypeCode(_d_UnionMember)

# typedef sequence <UnionMember> UnionMemberSeq;
_d_UnionMemberSeq  = (omniORB.tcInternal.tv_sequence, _d_UnionMember, 0)
_ad_UnionMemberSeq = (omniORB.tcInternal.tv_alias,
                      "IDL:omg.org/CORBA/UnionMemberSeq:1.0",
                      "UnionMemberSeq", _d_UnionMemberSeq)
_tc_UnionMemberSeq = omniORB.tcInternal.createTypeCode(_ad_UnionMemberSeq)

# typedef sequence <Identifier> EnumMemberSeq;
_d_EnumMemberSeq  = (omniORB.tcInternal.tv_sequence, _d_Identifier, 0)
_ad_EnumMemberSeq = (omniORB.tcInternal.tv_alias,
                     "IDL:omg.org/CORBA/EnumMemberSeq:1.0",
                     "EnumMemberSeq", _d_EnumMemberSeq)
_tc_EnumMemberSeq = omniORB.tcInternal.createTypeCode(_ad_EnumMemberSeq)
