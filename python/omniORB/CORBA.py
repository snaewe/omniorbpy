# CORBA module

# $Id$

# $Log$
# Revision 1.1  1999/06/08 16:21:52  dpg1
# Initial revision
#


import threading

import _omnipy
import omniORB
import Enum


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

class Exception:
    pass

# Completion status:

COMPLETED_YES     = Enum.Item("COMPLETED_YES",   0)
COMPLETED_NO      = Enum.Item("COMPLETED_NO",    1)
COMPLETED_MAYBE   = Enum.Item("COMPLETED_MAYBE", 2)
completion_status = Enum.Enum("IDL:omg.org/CORBA/completion_status:1.0",
                              [COMPLETED_YES, COMPLETED_NO, COMPLETED_MAYBE])

class SystemException (Exception):
    def __init__(self, minor=0, completed=COMPLETED_NO):
        self.minor     = minor
        if type(completed) == type(1):
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

__sysExceptionMapping = {
    "IDL:omg.org/CORBA/UNKNOWN":                UNKNOWN,
    "IDL:omg.org/CORBA/BAD_PARAM":              BAD_PARAM,
    "IDL:omg.org/CORBA/NO_MEMORY":              NO_MEMORY,
    "IDL:omg.org/CORBA/IMP_LIMIT":              IMP_LIMIT,
    "IDL:omg.org/CORBA/COMM_FAILURE":           COMM_FAILURE,
    "IDL:omg.org/CORBA/INV_OBJREF":             INV_OBJREF,
    "IDL:omg.org/CORBA/OBJECT_NOT_EXIST":       OBJECT_NOT_EXIST,
    "IDL:omg.org/CORBA/NO_PERMISSION":          NO_PERMISSION,
    "IDL:omg.org/CORBA/INTERNAL":               INTERNAL,
    "IDL:omg.org/CORBA/MARSHAL":                MARSHAL,
    "IDL:omg.org/CORBA/INITIALIZE":             INITIALIZE,
    "IDL:omg.org/CORBA/NO_IMPLEMENT":           NO_IMPLEMENT,
    "IDL:omg.org/CORBA/BAD_TYPECODE":           BAD_TYPECODE,
    "IDL:omg.org/CORBA/BAD_OPERATION":          BAD_OPERATION,
    "IDL:omg.org/CORBA/NO_RESOURCES":           NO_RESOURCES,
    "IDL:omg.org/CORBA/NO_RESPONSE":            NO_RESPONSE,
    "IDL:omg.org/CORBA/PERSIST_STORE":          PERSIST_STORE,
    "IDL:omg.org/CORBA/BAD_INV_ORDER":          BAD_INV_ORDER,
    "IDL:omg.org/CORBA/TRANSIENT":              TRANSIENT,
    "IDL:omg.org/CORBA/FREE_MEM":               FREE_MEM,
    "IDL:omg.org/CORBA/INV_IDENT":              INV_IDENT,
    "IDL:omg.org/CORBA/INV_FLAG":               INV_FLAG,
    "IDL:omg.org/CORBA/INTF_REPOS":             INTF_REPOS,
    "IDL:omg.org/CORBA/BAD_CONTEXT":            BAD_CONTEXT,
    "IDL:omg.org/CORBA/OBJ_ADAPTER":            OBJ_ADAPTER,
    "IDL:omg.org/CORBA/DATA_CONVERSION":        DATA_CONVERSION,
    "IDL:omg.org/CORBA/TRANSACTION_REQUIRED":   TRANSACTION_REQUIRED,
    "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK": TRANSACTION_ROLLEDBACK,
    "IDL:omg.org/CORBA/INVALID_TRANSACTION":    INVALID_TRANSACTION,
    "IDL:omg.org/CORBA/WRONG_TRANSACTION":      WRONG_TRANSACTION
    }


#############################################################################
#                                                                           #
# TypeCodes                                                                 #
#                                                                           #
#############################################################################

tk_null      = 0
tk_void      = 1
tk_short     = 2
tk_long      = 3
tk_ushort    = 4
tk_ulong     = 5
tk_float     = 6
tk_double    = 7
tk_boolean   = 8
tk_char	     = 9
tk_octet     = 10
tk_any	     = 11
tk_TypeCode  = 12
tk_Principal = 13
tk_objref    = 14
tk_struct    = 15
tk_union     = 16
tk_enum	     = 17
tk_string    = 18
tk_sequence  = 19
tk_array     = 20
tk_alias     = 21
tk_except    = 22
tk__indirect = 0xffffffff



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
        print "ORB initialised."

    def BOA_init(self, argv=[], boa_identifier = BOA_ID):
        return BOA(self, argv, boa_identifier)

    def string_to_object(self, ior):
        return _omnipy.stringToObject(self, ior)




#############################################################################
#                                                                           #
# BOA                                                                       #
#                                                                           #
#############################################################################

class BOA:
    """ omnipy BOA object

    Soon to be superceded by the POA...
    """

    def __init__(self, orb, argv, boa_identifier):
        _omnipy.BOA_init(self, orb, argv, boa_identifier)
        print "BOA initialised"





#############################################################################
#                                                                           #
# OBJECT                                                                    #
#                                                                           #
#############################################################################


class Object:
    """ CORBA::Object base class """

    _NP_RepositoryId = "IDL:omg.org/CORBA/Object:1.0"

    _nil = None

    def __init__(self):
        print "CORBA.Object created."

    def __del__(self):
        print "CORBA.Object destructor."

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







# Register this module with omnipy:
import CORBA, omniORB
_omnipy.registerPyObjects(CORBA, omniORB)
