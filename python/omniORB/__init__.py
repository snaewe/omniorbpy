# omniORB module -- omniORB specific stuff

# $Id$

# $Log$
# Revision 1.2  1999/07/29 14:16:03  dpg1
# Server side support.
#
# Revision 1.1  1999/07/19 15:53:26  dpg1
# Initial revision
#

import threading

import _omnipy

# ORB:
orb_lock = threading.Lock()
orb      = None


# Maps for object reference classes and IDL-defined types:

map_lock = threading.Lock()

objrefMapping   = {}
typeMapping     = {}
typeCodeMapping = {}


def registerObjref(repoId, objref):
    map_lock.acquire()
    objrefMapping[repoId] = objref
    map_lock.release()

def registerType(repoId, desc, tc):
#    print "type:", repoId, desc
    map_lock.acquire()
    typeMapping[repoId]     = desc
    typeCodeMapping[repoId] = tc
    map_lock.release()

def findType(repoId):
    map_lock.acquire()
    try:
        d = typeMapping[repoId]
        map_lock.release()
        return d
    except KeyError:
        map_lock.release()
        return None

def findTypeCode(repoId):
    map_lock.acquire()
    try:
        tc = typeCodeMapping[repoId]
        map_lock.release()
        return tc
    except KeyError:
        map_lock.release()
        return None


# Classes to support IDL type mapping

class EnumItem:
    def __init__(self, name, value):
        self._n = name
        self._v = value
        return
    def __str__(self):
        return self._n

    def __repr__(self):
        return self._n

class AnonymousEnumItem (EnumItem):
    def __init__(self, value):
        self._n = ""
        self._v = value
    
    def __repr__(self):
        return "anonymous enum item"


class Enum:
    def __init__(self, repoId, items):
        self._NP_RepositoryId = repoId
        self._items = items

    def _item(self, n):
        return self._items[n]



class Union:
    _def_m = None

    def __init__(self, *args, **kw):
        if len(args) == 2:
            self._d = args[0]
            self._v = args[1]
        else:
            ks = kw.keys()
            if len(args) != 0 or len(ks) != 1:
                raise AttributeError("require 2 arguments or one keyword argument.")
            k = ks[0]
            self.__setattr__(k, kw[k])

    def __str__(self):
        try:
            mem = self._d_to_m[self._d]
            return "_d = " + str(self._d) + ", " + mem + " = " + str(self._v)
        except KeyError:
            if self._def_m is not None:
                return "_d = " + str(self._d) + ", " + self._def_m + \
                       " = " + str(self._v)
            else:
                return "_d = " + str(self._d) + ", " + str(self._v) + \
                       " (malformed)"

    def __getattr__(self, mem):
        try:
            cmem = self._d_to_m[self._d]
            if mem == cmem:
                return self._v
            else:
                if mem == self._def_m or self._m_to_d.has_key(mem):
                    raise CORBA.BAD_PARAM
                else:
                    raise AttributeError(mem)
        except KeyError:
            if mem == self._def_m:
                return self._v
            else:
                if self._m_to_d.has_key(mem):
                    raise CORBA.BAD_PARAM
                else:
                    raise AttributeError(mem)

    def __setattr__(self, mem, val):
        if mem[0] == "_":
            self.__dict__[mem] = val
        else:
            try:
                disc = self._m_to_d[mem]
                self.__dict__["_d"] = disc
                self.__dict__["_v"] = val
            except KeyError:
                if mem == self._def_m:
                    self.__dict__["_d"] = self._def_d
                    self.__dict__["_v"] = val
                else:
                    raise AttributeError(mem)

# Import sub-modules
import CORBA, tcInternal



def createUnknownStruct(repoId, members):

    class UnknownStruct:
        def __init__(self, *args):
            if len(args) < len(self._members):
                raise TypeError("not enough arguments; expected " + \
                                str(len(self._members)) + ", got " + \
                                str(len(args)))
            elif len(args) > len(self._members):
                raise TypeError("too many arguments; expected " + \
                                str(len(self._members)) + ", got " + \
                                str(len(args)))

            self._values = args

            for i in range(len(args)):
                if self._members[i] != "":
                    setattr(self, self._members[i], args[i])

    UnknownStruct._NP_RepositoryId = repoId
    UnknownStruct._members         = members
    return UnknownStruct


def createUnknownUnion(repoId, def_used, members):

    class UnknownUnion (Union):
        pass

    UnknownUnion._NP_RepositoryId = repoId
    UnknownUnion._d_to_m          = {}
    UnknownUnion._m_to_d          = {}

    for i in range(len(members)):
        if i == def_used:
            UnknownUnion._def_d = members[i][0]
            UnknownUnion._def_m = members[i][1]
        else:
            UnknownUnion._d_to_m[members[i][0]] = members[i][1]
            UnknownUnion._m_to_d[members[i][1]] = members[i][0]

    return UnknownUnion


def createUnknownUserException(repoId, members):

    class UnknownUserException (CORBA.UserException):
        def __init__(self, *args):
            if len(args) < len(self._members):
                raise TypeError("not enough arguments; expected " + \
                                str(len(self._members)) + ", got " + \
                                str(len(args)))
            elif len(args) > len(self._members):
                raise TypeError("too many arguments; expected " + \
                                str(len(self._members)) + ", got " + \
                                str(len(args)))

            self._values = args

            for i in range(len(args)):
                if self._members[i] != "":
                    setattr(self, self._members[i], args[i])

    UnknownUserException._NP_RepositoryId = repoId
    UnknownUserException._members         = members
    return UnknownUserException


# System exception mapping:
sysExceptionMapping = {
    "IDL:omg.org/CORBA/UNKNOWN":                CORBA.UNKNOWN,
    "IDL:omg.org/CORBA/BAD_PARAM":              CORBA.BAD_PARAM,
    "IDL:omg.org/CORBA/NO_MEMORY":              CORBA.NO_MEMORY,
    "IDL:omg.org/CORBA/IMP_LIMIT":              CORBA.IMP_LIMIT,
    "IDL:omg.org/CORBA/COMM_FAILURE":           CORBA.COMM_FAILURE,
    "IDL:omg.org/CORBA/INV_OBJREF":             CORBA.INV_OBJREF,
    "IDL:omg.org/CORBA/OBJECT_NOT_EXIST":       CORBA.OBJECT_NOT_EXIST,
    "IDL:omg.org/CORBA/NO_PERMISSION":          CORBA.NO_PERMISSION,
    "IDL:omg.org/CORBA/INTERNAL":               CORBA.INTERNAL,
    "IDL:omg.org/CORBA/MARSHAL":                CORBA.MARSHAL,
    "IDL:omg.org/CORBA/INITIALIZE":             CORBA.INITIALIZE,
    "IDL:omg.org/CORBA/NO_IMPLEMENT":           CORBA.NO_IMPLEMENT,
    "IDL:omg.org/CORBA/BAD_TYPECODE":           CORBA.BAD_TYPECODE,
    "IDL:omg.org/CORBA/BAD_OPERATION":          CORBA.BAD_OPERATION,
    "IDL:omg.org/CORBA/NO_RESOURCES":           CORBA.NO_RESOURCES,
    "IDL:omg.org/CORBA/NO_RESPONSE":            CORBA.NO_RESPONSE,
    "IDL:omg.org/CORBA/PERSIST_STORE":          CORBA.PERSIST_STORE,
    "IDL:omg.org/CORBA/BAD_INV_ORDER":          CORBA.BAD_INV_ORDER,
    "IDL:omg.org/CORBA/TRANSIENT":              CORBA.TRANSIENT,
    "IDL:omg.org/CORBA/FREE_MEM":               CORBA.FREE_MEM,
    "IDL:omg.org/CORBA/INV_IDENT":              CORBA.INV_IDENT,
    "IDL:omg.org/CORBA/INV_FLAG":               CORBA.INV_FLAG,
    "IDL:omg.org/CORBA/INTF_REPOS":             CORBA.INTF_REPOS,
    "IDL:omg.org/CORBA/BAD_CONTEXT":            CORBA.BAD_CONTEXT,
    "IDL:omg.org/CORBA/OBJ_ADAPTER":            CORBA.OBJ_ADAPTER,
    "IDL:omg.org/CORBA/DATA_CONVERSION":        CORBA.DATA_CONVERSION,
    "IDL:omg.org/CORBA/TRANSACTION_REQUIRED":   CORBA.TRANSACTION_REQUIRED,
    "IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK": CORBA.TRANSACTION_ROLLEDBACK,
    "IDL:omg.org/CORBA/INVALID_TRANSACTION":    CORBA.INVALID_TRANSACTION,
    "IDL:omg.org/CORBA/WRONG_TRANSACTION":      CORBA.WRONG_TRANSACTION
    }

# Reserved word mapping:
keywordMapping = {
    "access":   "_access",
    "and":      "_and",
    "assert":   "_assert",
    "break":    "_break",
    "class":    "_class",
    "continue": "_continue",
    "def":      "_def",
    "del":      "_del",
    "elif":     "_elif",
    "else":     "_else",
    "except":   "_except",
    "finally":  "_finally",
    "for":      "_for",
    "from":     "_from",
    "global":   "_global",
    "if":       "_if",
    "import":   "_import",
    "in":       "_in",
    "is":       "_is",
    "lambda":   "_lambda",
    "not":      "_not",
    "or":       "_or",
    "pass":     "_pass",
    "print":    "_print",
    "raise":    "_raise",
    "return":   "_return",
    "try":      "_try",
    "while":    "_while"
    }


# Register this module with omnipy:
import omniORB
_omnipy.registerPyObjects(omniORB)
del omniORB
