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
# Revision 1.30  2001/06/18 09:40:01  dpg1
# 1.4 release.
#
# Revision 1.27.2.3  2001/05/17 14:05:01  dpg1
# Implement _get_interface()
#
# Revision 1.27.2.2  2000/08/17 08:46:06  dpg1
# Support for omniORB.LOCATION_FORWARD exception
#
# Revision 1.27.2.1  2000/08/07 09:19:24  dpg1
# Long long support
#
# Revision 1.27  2000/07/10 18:44:26  dpg1
# Remove partial IR stubs. Add TypeCodes for System Exceptions.
#
# Revision 1.26  2000/06/27 15:09:01  dpg1
# Fix to _is_a(). CORBA.Object registered in the objref map.
#
# Revision 1.25  2000/06/02 14:25:51  dpg1
# orb.run() now properly exits when the ORB is shut down
#
# Revision 1.24  2000/05/16 10:43:24  dpg1
# Add TC_foo names for TypeCode constants.
#
# Revision 1.23  2000/04/27 11:05:26  dpg1
# Add perform_work(), work_pending(), shutdown(), and destroy() operations.
#
# Revision 1.22  2000/03/29 10:15:47  dpg1
# Exceptions now more closely follow the interface of
# exceptions.Exception.
#
# Revision 1.21  2000/03/28 14:46:36  dpg1
# Undo the last change.
#
# Revision 1.20  2000/03/28 09:33:21  dpg1
# CORBA.Exception no longer derives from exceptions.Exception.
#
# Revision 1.19  2000/03/03 17:41:28  dpg1
# Major reorganisation to support omniORB 3.0 as well as 2.8.
#
# Revision 1.17  2000/01/31 10:51:42  dpg1
# Fix to exception throwing.
#
# Revision 1.16  2000/01/18 17:14:13  dpg1
# Support for pickle
#
# Revision 1.15  1999/12/07 12:35:33  dpg1
# id() function added.
#
# Revision 1.14  1999/11/25 14:12:34  dpg1
# sleep()ing for maxint seconds wasn't a good idea, since some platforms
# use milliseconds for their sleep system call.
#
# Revision 1.13  1999/11/25 14:01:45  dpg1
# orb.run() now uses time.sleep() to sleep, rather than blocking in
# impl_is_ready(). This means Python can interrupt the sleep.
#
# Revision 1.12  1999/11/10 16:08:21  dpg1
# Some types weren't registered properly.
#
# Revision 1.11  1999/10/18 08:25:57  dpg1
# _is_a() now works properly for local objects.
#
# Revision 1.10  1999/09/29 11:25:55  dpg1
# Nil objects now map to None. They work too, which is more than can be
# said for the old mapping...
#
# Revision 1.9  1999/09/27 09:06:37  dpg1
# Friendly error message if there is no thread support.
#
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


import _omnipy
import omniORB

import threading, types, time, exceptions


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

OMGVMCID = 0x4f4d0000

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
        Exception.__init__(self, minor, self.completed)

    def __str__(self):
        return "Minor: " + str(self.minor) + \
               ", Completed: " + str(self.completed) + "."

class UserException (Exception):
    def __init__(self, *args):
        self.__args = args

    def __str__(self):
        if not self.__args:
            return "User exception with no members"
        elif len(self.__args) == 1:
            return "User exception member: " + str(self.__args[0])
        else:
            return "User exception members: " + str(self.__args)

    def __getitem__(self, i):
        return self.__args[i]


# All the standard system exceptions...

class UNKNOWN (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/UNKNOWN:1.0"

class BAD_PARAM (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_PARAM:1.0"

class NO_MEMORY (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_MEMORY:1.0"

class IMP_LIMIT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/IMP_LIMIT:1.0"

class COMM_FAILURE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/COMM_FAILURE:1.0"

class INV_OBJREF (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INV_OBJREF:1.0"

class OBJECT_NOT_EXIST (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0"

class NO_PERMISSION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_PERMISSION:1.0"

class INTERNAL (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INTERNAL:1.0"

class MARSHAL (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/MARSHAL:1.0"

class INITIALIZE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INITIALIZE:1.0"

class NO_IMPLEMENT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_IMPLEMENT:1.0"

class BAD_TYPECODE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_TYPECODE:1.0"

class BAD_OPERATION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_OPERATION:1.0"

class NO_RESOURCES (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_RESOURCES:1.0"

class NO_RESPONSE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/NO_RESPONSE:1.0"

class PERSIST_STORE (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/PERSIST_STORE:1.0"

class BAD_INV_ORDER (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_INV_ORDER:1.0"

class TRANSIENT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/TRANSIENT:1.0"

class FREE_MEM (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/FREE_MEM:1.0"

class INV_IDENT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INV_IDENT:1.0"

class INV_FLAG (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INV_FLAG:1.0"

class INTF_REPOS (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INTF_REPOS:1.0"

class BAD_CONTEXT (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/BAD_CONTEXT:1.0"

class OBJ_ADAPTER (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/OBJ_ADAPTER:1.0"

class DATA_CONVERSION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/DATA_CONVERSION:1.0"

class TRANSACTION_REQUIRED (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/TRANSACTION_REQUIRED:1.0"

class TRANSACTION_ROLLEDBACK (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK:1.0"

class INVALID_TRANSACTION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/INVALID_TRANSACTION:1.0"

class WRONG_TRANSACTION (SystemException):
    _NP_RepositoryId = "IDL:omg.org/CORBA/WRONG_TRANSACTION:1.0"


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


# TypeCodes of basic types. The CORBA mapping says the TypeCode
# constants should start TC, but omniORBpy previously used _tc, so we
# support both:

TC_null     = _tc_null     = tcInternal.createTypeCode(tcInternal.tv_null)
TC_void     = _tc_void     = tcInternal.createTypeCode(tcInternal.tv_void)
TC_short    = _tc_short    = tcInternal.createTypeCode(tcInternal.tv_short)
TC_long     = _tc_long     = tcInternal.createTypeCode(tcInternal.tv_long)
TC_ushort   = _tc_ushort   = tcInternal.createTypeCode(tcInternal.tv_ushort)
TC_ulong    = _tc_ulong    = tcInternal.createTypeCode(tcInternal.tv_ulong)
TC_float    = _tc_float    = tcInternal.createTypeCode(tcInternal.tv_float)
TC_double   = _tc_double   = tcInternal.createTypeCode(tcInternal.tv_double)
TC_boolean  = _tc_boolean  = tcInternal.createTypeCode(tcInternal.tv_boolean)
TC_char     = _tc_char     = tcInternal.createTypeCode(tcInternal.tv_char)
TC_octet    = _tc_octet    = tcInternal.createTypeCode(tcInternal.tv_octet)
TC_any      = _tc_any      = tcInternal.createTypeCode(tcInternal.tv_any)
TC_TypeCode = _tc_TypeCode = tcInternal.createTypeCode(tcInternal.tv_TypeCode)
TC_Principal= _tc_Principal= tcInternal.createTypeCode(tcInternal.tv_Principal)
TC_string   = _tc_string   =tcInternal.createTypeCode((tcInternal.tv_string,0))
TC_longlong = _tc_longlong = tcInternal.createTypeCode(tcInternal.tv_longlong)
TC_ulonglong= _tc_ulonglong= tcInternal.createTypeCode(tcInternal.tv_ulonglong)
TC_longdouble = _tc_longdouble \
            = tcInternal.createTypeCode(tcInternal.tv_longdouble)

# id() function returns the repository ID of an object
def id(obj):
    try:
        return obj._NP_RepositoryId
    except AttributeError:
        raise BAD_PARAM()


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

if _omnipy.coreVersion() == "2.8.0":
    ORB_ID = "omniORB2"
else:
    ORB_ID = "omniORB3"

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
        self._omni_argv = argv # For omniORB 2.8 to initialise its BOA
        self._shutdown = 0

    def string_to_object(self, ior):
        return _omnipy.orb_func.string_to_object(self, ior)

    def object_to_string(self, obj):
        return _omnipy.orb_func.object_to_string(self, obj)

    def list_initial_services(self):
        return _omnipy.orb_func.list_initial_services(self)

    def resolve_initial_references(self, identifier):
        return _omnipy.orb_func.resolve_initial_references(self, identifier)

    def work_pending(self):
        return 0

    def perform_work(self):
        return

    def run(self):
        omniORB.orb_lock.acquire()
        while not self._shutdown:
            omniORB.orb_cond.wait(60)
        omniORB.orb_lock.release()

    def shutdown(self, wait_for_completion):
        _omnipy.orb_func.shutdown(self, wait_for_completion)

    def destroy(self):
        _omnipy.orb_func.destroy(self)

    def _has_shutdown(self):
        # Callback from the C++ world to kill threads blocked in run()
        omniORB.orb_lock.acquire()
        self._shutdown = 1
        omniORB.orb_cond.notifyAll()
        omniORB.orb_lock.release()


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


    __methods__ = ["string_to_object", "object_to_string",
                   "list_initial_services", "resolve_initial_references",
                   "work_pending", "perform_work", "run",
                   "shutdown", "destroy",
                   "create_struct_tc", "create_union_tc",
                   "create_enum_tc", "create_alias_tc",
                   "create_exception_tc", "create_interface_tc",
                   "create_string_tc", "create_sequence_tc",
                   "create_array_tc", "create_recursive_tc"]

    class InvalidName (UserException):
        _NP_RepositoryId = "IDL:omg.org/CORBA/ORB/InvalidName:1.0"

    _d_InvalidName  = (omniORB.tcInternal.tv_except, InvalidName,
                       InvalidName._NP_RepositoryId, "InvalidName")
    _tc_InvalidName = omniORB.tcInternal.createTypeCode(_d_InvalidName)
    omniORB.registerType(InvalidName._NP_RepositoryId,
                         _d_InvalidName, _tc_InvalidName)



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
        if _omnipy is not None:
            _omnipy.releaseObjref(self)

    def __getstate__(self):
        return ORB_init().object_to_string(self)

    def __setstate__(self, state):
        o = ORB_init().string_to_object(state)
        self.__dict__.update(o.__dict__)
        def dummy(): pass # Why doesn't dummy want an argument? ***
        o.__del__ = dummy

    def _get_interface(self):
        import omniORB
        if omniORB.orb is None:
            raise BAD_INV_ORDER(0, COMPLETED_NO)

        import omniORB.ir_idl # Make sure IR stubs are loaded

        ir = omniORB.orb.resolve_initial_references("InterfaceRepository")
        ir = ir._narrow(Repository)
        if ir is None:
            raise INTF_REPOS(0, COMPLETED_NO)
        interf = ir.lookup_id(self._NP_RepositoryId)
        return interf._narrow(InterfaceDef)
    
    def _is_a(self, repoId):
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
TC_Object  = _tc_Object = omniORB.tcInternal.createTypeCode(_d_Object)
omniORB.registerType(Object._NP_RepositoryId, _d_Object, _tc_Object)
omniORB.registerObjref(Object._NP_RepositoryId, Object)


#############################################################################
#                                                                           #
# Policy                                                                    #
#                                                                           #
#############################################################################

class Policy (Object):
    _NP_ReposotoryId = "IDL:omg.org/CORBA/Policy:1.0"

    def __init__(self):
        raise RuntimeError("Cannot construct objects of this type.")

    def _get_policy_type(self):
        return self._policy_type

    def copy(self):
        return self

    def destroy(self):
        pass

    def _is_a(self, repoId):
        return omniORB.static_is_a(self.__class__, repoId)

    def _non_existent(self):
        return 0

    def _is_equivalent(self, other_object):
        return self == other_object

    def _hash(self, maximum):
        return hash(self) % maximum

    def _narrow(self, dest):
        if self._is_a(dest._NP_RepositoryId):
            return self
        else:
            return None

    __methods__ = ["_get_policy_type", "copy", "destroy"] + Object.__methods__




#############################################################################
#                                                                           #
# CORBA module functions                                                    #
#                                                                           #
#############################################################################

def is_nil(obj):
    if obj is None:
        return 1
    if isinstance(obj, Object):
        return 0
    raise BAD_PARAM()


#############################################################################
#                                                                           #
# Interface Repository stuff                                                #
#                                                                           #
#############################################################################

# Note that we do not include the majority of the IfR declarations
# here, because that would cause lots of bloat. Code which requires it
# should compile ir.idl and manually import ir_idl.

# typedef string Identifier
class Identifier:
    _NP_RepositoryId = "IDL:omg.org/CORBA/Identifier:1.0"
    def __init__(self):
        raise RuntimeError("Cannot construct objects of this type.")
_d_Identifier  = (tcInternal.tv_string,0)
_ad_Identifier = (tcInternal.tv_alias, Identifier._NP_RepositoryId, "Identifier", (tcInternal.tv_string,0))
_tc_Identifier = tcInternal.createTypeCode(_ad_Identifier)
omniORB.registerType(Identifier._NP_RepositoryId, _ad_Identifier, _tc_Identifier)


#############################################################################
#                                                                           #
# TypeCodes for System Exceptions                                           #
#                                                                           #
#############################################################################

_d_completion_status  = (tcInternal.tv_enum, completion_status._NP_RepositoryId, "completion_status", completion_status._items)
_tc_completion_status = tcInternal.createTypeCode(_d_completion_status)
omniORB.registerType(completion_status._NP_RepositoryId, _d_completion_status, _tc_completion_status)

# Strings to put in the descriptors, so all descriptors share the same
# strings.
_minor     = "minor"
_completed = "completed"

_d_UNKNOWN  = (tcInternal.tv_except, UNKNOWN, UNKNOWN._NP_RepositoryId, "UNKNOWN", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_UNKNOWN = tcInternal.createTypeCode(_d_UNKNOWN)
omniORB.registerType(UNKNOWN._NP_RepositoryId, _d_UNKNOWN, _tc_UNKNOWN)

_d_BAD_PARAM  = (tcInternal.tv_except, BAD_PARAM, BAD_PARAM._NP_RepositoryId, "BAD_PARAM", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_BAD_PARAM = tcInternal.createTypeCode(_d_BAD_PARAM)
omniORB.registerType(BAD_PARAM._NP_RepositoryId, _d_BAD_PARAM, _tc_BAD_PARAM)

_d_NO_MEMORY  = (tcInternal.tv_except, NO_MEMORY, NO_MEMORY._NP_RepositoryId, "NO_MEMORY", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_NO_MEMORY = tcInternal.createTypeCode(_d_NO_MEMORY)
omniORB.registerType(NO_MEMORY._NP_RepositoryId, _d_NO_MEMORY, _tc_NO_MEMORY)

_d_IMP_LIMIT  = (tcInternal.tv_except, IMP_LIMIT, IMP_LIMIT._NP_RepositoryId, "IMP_LIMIT", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_IMP_LIMIT = tcInternal.createTypeCode(_d_IMP_LIMIT)
omniORB.registerType(IMP_LIMIT._NP_RepositoryId, _d_IMP_LIMIT, _tc_IMP_LIMIT)

_d_COMM_FAILURE  = (tcInternal.tv_except, COMM_FAILURE, COMM_FAILURE._NP_RepositoryId, "COMM_FAILURE", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_COMM_FAILURE = tcInternal.createTypeCode(_d_COMM_FAILURE)
omniORB.registerType(COMM_FAILURE._NP_RepositoryId, _d_COMM_FAILURE, _tc_COMM_FAILURE)

_d_INV_OBJREF  = (tcInternal.tv_except, INV_OBJREF, INV_OBJREF._NP_RepositoryId, "INV_OBJREF", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INV_OBJREF = tcInternal.createTypeCode(_d_INV_OBJREF)
omniORB.registerType(INV_OBJREF._NP_RepositoryId, _d_INV_OBJREF, _tc_INV_OBJREF)

_d_OBJECT_NOT_EXIST  = (tcInternal.tv_except, OBJECT_NOT_EXIST, OBJECT_NOT_EXIST._NP_RepositoryId, "OBJECT_NOT_EXIST", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_OBJECT_NOT_EXIST = tcInternal.createTypeCode(_d_OBJECT_NOT_EXIST)
omniORB.registerType(OBJECT_NOT_EXIST._NP_RepositoryId, _d_OBJECT_NOT_EXIST, _tc_OBJECT_NOT_EXIST)

_d_NO_PERMISSION  = (tcInternal.tv_except, NO_PERMISSION, NO_PERMISSION._NP_RepositoryId, "NO_PERMISSION", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_NO_PERMISSION = tcInternal.createTypeCode(_d_NO_PERMISSION)
omniORB.registerType(NO_PERMISSION._NP_RepositoryId, _d_NO_PERMISSION, _tc_NO_PERMISSION)

_d_INTERNAL  = (tcInternal.tv_except, INTERNAL, INTERNAL._NP_RepositoryId, "INTERNAL", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INTERNAL = tcInternal.createTypeCode(_d_INTERNAL)
omniORB.registerType(INTERNAL._NP_RepositoryId, _d_INTERNAL, _tc_INTERNAL)

_d_MARSHAL  = (tcInternal.tv_except, MARSHAL, MARSHAL._NP_RepositoryId, "MARSHAL", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_MARSHAL = tcInternal.createTypeCode(_d_MARSHAL)
omniORB.registerType(MARSHAL._NP_RepositoryId, _d_MARSHAL, _tc_MARSHAL)

_d_INITIALIZE  = (tcInternal.tv_except, INITIALIZE, INITIALIZE._NP_RepositoryId, "INITIALIZE", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INITIALIZE = tcInternal.createTypeCode(_d_INITIALIZE)
omniORB.registerType(INITIALIZE._NP_RepositoryId, _d_INITIALIZE, _tc_INITIALIZE)

_d_NO_IMPLEMENT  = (tcInternal.tv_except, NO_IMPLEMENT, NO_IMPLEMENT._NP_RepositoryId, "NO_IMPLEMENT", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_NO_IMPLEMENT = tcInternal.createTypeCode(_d_NO_IMPLEMENT)
omniORB.registerType(NO_IMPLEMENT._NP_RepositoryId, _d_NO_IMPLEMENT, _tc_NO_IMPLEMENT)

_d_BAD_TYPECODE  = (tcInternal.tv_except, BAD_TYPECODE, BAD_TYPECODE._NP_RepositoryId, "BAD_TYPECODE", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_BAD_TYPECODE = tcInternal.createTypeCode(_d_BAD_TYPECODE)
omniORB.registerType(BAD_TYPECODE._NP_RepositoryId, _d_BAD_TYPECODE, _tc_BAD_TYPECODE)

_d_BAD_OPERATION  = (tcInternal.tv_except, BAD_OPERATION, BAD_OPERATION._NP_RepositoryId, "BAD_OPERATION", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_BAD_OPERATION = tcInternal.createTypeCode(_d_BAD_OPERATION)
omniORB.registerType(BAD_OPERATION._NP_RepositoryId, _d_BAD_OPERATION, _tc_BAD_OPERATION)

_d_NO_RESOURCES  = (tcInternal.tv_except, NO_RESOURCES, NO_RESOURCES._NP_RepositoryId, "NO_RESOURCES", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_NO_RESOURCES = tcInternal.createTypeCode(_d_NO_RESOURCES)
omniORB.registerType(NO_RESOURCES._NP_RepositoryId, _d_NO_RESOURCES, _tc_NO_RESOURCES)

_d_NO_RESPONSE  = (tcInternal.tv_except, NO_RESPONSE, NO_RESPONSE._NP_RepositoryId, "NO_RESPONSE", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_NO_RESPONSE = tcInternal.createTypeCode(_d_NO_RESPONSE)
omniORB.registerType(NO_RESPONSE._NP_RepositoryId, _d_NO_RESPONSE, _tc_NO_RESPONSE)

_d_PERSIST_STORE  = (tcInternal.tv_except, PERSIST_STORE, PERSIST_STORE._NP_RepositoryId, "PERSIST_STORE", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_PERSIST_STORE = tcInternal.createTypeCode(_d_PERSIST_STORE)
omniORB.registerType(PERSIST_STORE._NP_RepositoryId, _d_PERSIST_STORE, _tc_PERSIST_STORE)

_d_BAD_INV_ORDER  = (tcInternal.tv_except, BAD_INV_ORDER, BAD_INV_ORDER._NP_RepositoryId, "BAD_INV_ORDER", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_BAD_INV_ORDER = tcInternal.createTypeCode(_d_BAD_INV_ORDER)
omniORB.registerType(BAD_INV_ORDER._NP_RepositoryId, _d_BAD_INV_ORDER, _tc_BAD_INV_ORDER)

_d_TRANSIENT  = (tcInternal.tv_except, TRANSIENT, TRANSIENT._NP_RepositoryId, "TRANSIENT", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_TRANSIENT = tcInternal.createTypeCode(_d_TRANSIENT)
omniORB.registerType(TRANSIENT._NP_RepositoryId, _d_TRANSIENT, _tc_TRANSIENT)

_d_FREE_MEM  = (tcInternal.tv_except, FREE_MEM, FREE_MEM._NP_RepositoryId, "FREE_MEM", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_FREE_MEM = tcInternal.createTypeCode(_d_FREE_MEM)
omniORB.registerType(FREE_MEM._NP_RepositoryId, _d_FREE_MEM, _tc_FREE_MEM)

_d_INV_IDENT  = (tcInternal.tv_except, INV_IDENT, INV_IDENT._NP_RepositoryId, "INV_IDENT", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INV_IDENT = tcInternal.createTypeCode(_d_INV_IDENT)
omniORB.registerType(INV_IDENT._NP_RepositoryId, _d_INV_IDENT, _tc_INV_IDENT)

_d_INV_FLAG  = (tcInternal.tv_except, INV_FLAG, INV_FLAG._NP_RepositoryId, "INV_FLAG", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INV_FLAG = tcInternal.createTypeCode(_d_INV_FLAG)
omniORB.registerType(INV_FLAG._NP_RepositoryId, _d_INV_FLAG, _tc_INV_FLAG)

_d_INTF_REPOS  = (tcInternal.tv_except, INTF_REPOS, INTF_REPOS._NP_RepositoryId, "INTF_REPOS", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INTF_REPOS = tcInternal.createTypeCode(_d_INTF_REPOS)
omniORB.registerType(INTF_REPOS._NP_RepositoryId, _d_INTF_REPOS, _tc_INTF_REPOS)

_d_BAD_CONTEXT  = (tcInternal.tv_except, BAD_CONTEXT, BAD_CONTEXT._NP_RepositoryId, "BAD_CONTEXT", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_BAD_CONTEXT = tcInternal.createTypeCode(_d_BAD_CONTEXT)
omniORB.registerType(BAD_CONTEXT._NP_RepositoryId, _d_BAD_CONTEXT, _tc_BAD_CONTEXT)

_d_OBJ_ADAPTER  = (tcInternal.tv_except, OBJ_ADAPTER, OBJ_ADAPTER._NP_RepositoryId, "OBJ_ADAPTER", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_OBJ_ADAPTER = tcInternal.createTypeCode(_d_OBJ_ADAPTER)
omniORB.registerType(OBJ_ADAPTER._NP_RepositoryId, _d_OBJ_ADAPTER, _tc_OBJ_ADAPTER)

_d_DATA_CONVERSION  = (tcInternal.tv_except, DATA_CONVERSION, DATA_CONVERSION._NP_RepositoryId, "DATA_CONVERSION", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_DATA_CONVERSION = tcInternal.createTypeCode(_d_DATA_CONVERSION)
omniORB.registerType(DATA_CONVERSION._NP_RepositoryId, _d_DATA_CONVERSION, _tc_DATA_CONVERSION)

_d_TRANSACTION_REQUIRED  = (tcInternal.tv_except, TRANSACTION_REQUIRED, TRANSACTION_REQUIRED._NP_RepositoryId, "TRANSACTION_REQUIRED", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_TRANSACTION_REQUIRED = tcInternal.createTypeCode(_d_TRANSACTION_REQUIRED)
omniORB.registerType(TRANSACTION_REQUIRED._NP_RepositoryId, _d_TRANSACTION_REQUIRED, _tc_TRANSACTION_REQUIRED)

_d_TRANSACTION_ROLLEDBACK  = (tcInternal.tv_except, TRANSACTION_ROLLEDBACK, TRANSACTION_ROLLEDBACK._NP_RepositoryId, "TRANSACTION_ROLLEDBACK", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_TRANSACTION_ROLLEDBACK = tcInternal.createTypeCode(_d_TRANSACTION_ROLLEDBACK)
omniORB.registerType(TRANSACTION_ROLLEDBACK._NP_RepositoryId, _d_TRANSACTION_ROLLEDBACK, _tc_TRANSACTION_ROLLEDBACK)

_d_INVALID_TRANSACTION  = (tcInternal.tv_except, INVALID_TRANSACTION, INVALID_TRANSACTION._NP_RepositoryId, "INVALID_TRANSACTION", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_INVALID_TRANSACTION = tcInternal.createTypeCode(_d_INVALID_TRANSACTION)
omniORB.registerType(INVALID_TRANSACTION._NP_RepositoryId, _d_INVALID_TRANSACTION, _tc_INVALID_TRANSACTION)

_d_WRONG_TRANSACTION  = (tcInternal.tv_except, WRONG_TRANSACTION, WRONG_TRANSACTION._NP_RepositoryId, "WRONG_TRANSACTION", _minor, tcInternal.tv_long, _completed, _d_completion_status)
_tc_WRONG_TRANSACTION = tcInternal.createTypeCode(_d_WRONG_TRANSACTION)
omniORB.registerType(WRONG_TRANSACTION._NP_RepositoryId, _d_WRONG_TRANSACTION, _tc_WRONG_TRANSACTION)

del _minor, _completed
