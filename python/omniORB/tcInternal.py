# TypeCode internal implementation

# $Id$

# $Log$
# Revision 1.2  1999/07/19 15:48:40  dpg1
# All sorts of fixes.
#
# Revision 1.1  1999/06/24 15:23:28  dpg1
# Initial revision
#

import omniORB, CORBA
import types

# The TypeCode implementation is based upon descriptors consisting of
# Python tuples, as used by the marshalling code. Although the public
# interface to TypeCodes presents a graph of connected TypeCode
# objects, the graph is actually only fully-stored by the descriptor
# tuple. A root TypeCode object has a reference to the top of the
# descriptor tuple. Non-root TypeCode objects are only created when
# requested.
#
# Recursive descriptors pose a problem for Python's garbage collector.
# To ensure they are properly collected, all non-root TypeCode objects
# keep a reference to the root TypeCode object. The root TypeCode is
# therefore only deleted once all "views" of the TypeCode have gone.
# When the root TypeCode object is deleted, it recursively descends
# its descriptor, and removes any indirections it finds. All loops are
# thus removed from the descriptor, making it a candidate for
# collection.
#
# This approach means that the descriptor becomes invalid once the
# root TypeCode object is deleted, even if there are other active
# references to the descriptor. BEWARE!

# Kinds as numbers:
tv_null               = 0
tv_void               = 1
tv_short              = 2
tv_long               = 3
tv_ushort             = 4
tv_ulong              = 5
tv_float              = 6
tv_double             = 7
tv_boolean            = 8
tv_char	              = 9
tv_octet              = 10
tv_any	              = 11
tv_TypeCode           = 12
tv_Principal          = 13
tv_objref             = 14
tv_struct             = 15
tv_union              = 16
tv_enum	              = 17
tv_string             = 18
tv_sequence           = 19
tv_array              = 20
tv_alias              = 21
tv_except             = 22
tv_longlong           = 23
tv_ulonglong          = 24
tv_longdouble         = 25
tv_wchar              = 26
tv_wstring            = 27
tv_fixed              = 28
tv_value              = 29
tv_value_box          = 30
tv_native             = 31
tv_abstract_interface = 32
tv__indirect          = 0xffffffff


# Create a TypeCode given a class or a repoId

def typecode(t):
    if type(t) is types.ClassType:
        try:
            t = t._NP_RepositoryId
        except AttributeError:
            raise TypeError("Class must be a CORBA class.")

    if type(t) is not types.StringType:
        raise TypeError("Argument must be CORBA class or repository id.")

    d = omniORB.findType(t)
    if d is None:
        raise TypeError("Unknown CORBA type.")

    return createTypeCode(d)


# Function to create a TypeCode object given a descriptor. Returns a
# static (stub generated) TypeCode object if possible.

def createTypeCode(d, parent=None):
    if type(d) == types.TupleType:
        k = d[0]
    else:
        k = d

    print "createTypeCode():", d
    
    if   k == tv_null:      return TypeCode_empty(d)
    elif k == tv_void:      return TypeCode_empty(d)
    elif k == tv_short:     return TypeCode_empty(d)
    elif k == tv_long:      return TypeCode_empty(d)
    elif k == tv_ushort:    return TypeCode_empty(d)
    elif k == tv_ulong:     return TypeCode_empty(d)
    elif k == tv_float:     return TypeCode_empty(d)
    elif k == tv_double:    return TypeCode_empty(d)
    elif k == tv_boolean:   return TypeCode_empty(d)
    elif k == tv_char:      return TypeCode_empty(d)
    elif k == tv_octet:     return TypeCode_empty(d)
    elif k == tv_any:       return TypeCode_empty(d)
    elif k == tv_TypeCode:  return TypeCode_empty(d)
    elif k == tv_Principal: return TypeCode_empty(d)
    elif k == tv_string:    return TypeCode_string(d)

    elif k == tv_objref:
        tc = omniORB.findTypeCode(d[1])
        if tc is None:
            tc = TypeCode_objref(d)
        return tc

    elif k == tv_struct:
        tc = omniORB.findTypeCode(d[2])
        if tc is None:
            tc = TypeCode_struct(d, parent)
        return tc
    
    elif k == tv_union:
        tc = omniORB.findTypeCode(d[2])
        if tc is None:
            tc = TypeCode_union(d, parent)
        return tc
    
    elif k == tv_enum:
        tc = omniORB.findTypeCode(d[1])
        if tc is None:
            tc = TypeCode_enum(d)
        return tc

    elif k == tv_sequence:  return TypeCode_sequence(d, parent)
    elif k == tv_array:     return TypeCode_array(d, parent)

    elif k == tv_alias:
        tc = omniORB.findTypeCode(d[1])
        if tc is None:
            tc = TypeCode_alias(d, parent)
        return tc
    
    elif k == tv_except:
        tc = omniORB.findTypeCode(d[2])
        if tc is None:
            tc = TypeCode_except(d, parent)
        return tc

    elif k == tv__indirect: return createTypeCode(d[1][0], parent)

    raise CORBA.INTERNAL


# Class for short, long, ushort, ulong, float, double, boolean, char,
# octet, any, TypeCode, Principal:

class TypeCode_empty (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.IntType: raise CORBA.INTERNAL
        if desc != tv_null       and \
           desc != tv_void       and \
           desc != tv_short      and \
           desc != tv_long       and \
           desc != tv_ushort     and \
           desc != tv_ulong      and \
           desc != tv_float      and \
           desc != tv_double     and \
           desc != tv_boolean    and \
           desc != tv_char       and \
           desc != tv_octet      and \
           desc != tv_any        and \
           desc != tv_TypeCode   and \
           desc != tv_Principal: raise CORBA.INTERNAL

        self._d = desc
        self._k = CORBA.TCKind._item(desc)


# string:
class TypeCode_string (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != tv_string:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_string

    def length(self):
        return self._d[1]


# objref:
class TypeCode_objref (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != tv_objref:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_objref

    def id(self):   return self._d[1]
    def name(self): return self._d[2]


# struct:
class TypeCode_struct (CORBA.TypeCode):
    def __init__(self, desc, parent):
        if type(desc) != types.TupleType or \
           desc[0] != tv_struct:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_struct
        self._p = parent

    def __del__(self):
        if self._p is None and removeIndirections is not None:
            removeIndirections(self._d)

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_struct(getCompactDescriptor(self._d))

    def id(self):                 return self._d[2]
    def name(self):               return self._d[3]
    def member_count(self):       return (len(self._d) - 4) / 2
    def member_name(self, index):
        off = index * 2 + 4
        if index < 0 or off >= len(self._d): raise CORBA.TypeCode.Bounds
        return self._d[off]

    def member_type(self, index):
        off = index * 2 + 5
        if index < 0 or off >= len(self._d): raise CORBA.TypeCode.Bounds
        if self._p is None and removeIndirections is not None:
            return createTypeCode(self._d[off], self)
        else:
            return createTypeCode(self._d[off], self._p)
    
# union:
class TypeCode_union (CORBA.TypeCode):
    def __init__(self, desc, parent):
        if type(desc) != types.TupleType or \
           desc[0] != tv_union:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_union
        self._p = parent

    def __del__(self):
        if self._p is None and removeIndirections is not None:
            removeIndirections(self._d)

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_union(getCompactDescriptor(self._d))

    def id(self):                  return self._d[2]
    def name(self):                return self._d[3]
    def member_count(self):        return len(self._d[6])

    def member_name(self, index):
        if index < 0 or index >= len(self._d[6]): raise CORBA.TypeCode.Bounds
        return self._d[6][index][1]
    
    def member_type(self, index):
        if index < 0 or index >= len(self._d[6]): raise CORBA.TypeCode.Bounds
        if self._p is None and removeIndirections is not None:
            return createTypeCode(self._d[6][index][2], self)
        else:
            return createTypeCode(self._d[6][index][2], self._p)

    def member_label(self, index):
        if index < 0 or index >= len(self._d[6]): raise CORBA.TypeCode.Bounds
        if index == self._d[5]: return 0
        return self._d[6][index][0]

    def discriminator_type(self): return createTypeCode(self._d[4])

    def default_index(self):
        if self._d[5] >= 0: return self._d[5]
        return -1

# enum:
class TypeCode_enum (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != tv_enum:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_enum

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_enum(getCompactDescriptor(self._d))

    def id(self):           return self._d[1]
    def name(self):         return self._d[2]
    def member_count(self): return len(self._d[3])

    def member_name(self, index):
        if index < 0 or index >= len(self._d[3]): raise CORBA.TypeCode.Bounds
        return self._d[3][index]._n

# sequence:
class TypeCode_sequence (CORBA.TypeCode):
    def __init__(self, desc, parent):
        if type(desc) != types.TupleType or \
           desc[0] != tv_sequence:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_sequence
        self._p = parent

    def __del__(self):
        if self._p is None and removeIndirections is not None:
            removeIndirections(self._d)

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_sequence(getCompactDescriptor(self._d))

    def length(self):       return self._d[2]
    def content_type(self):
        if self._p is None and removeIndirections is not None:
            return createTypeCode(self._d[1], self)
        else:
            return createTypeCode(self._d[1], self._p)

# array:
class TypeCode_array (CORBA.TypeCode):
    def __init__(self, desc, parent):
        if type(desc) != types.TupleType or \
           desc[0] != tv_array:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_array
        self._p = parent

    def __del__(self):
        if self._p is None and removeIndirections is not None:
            removeIndirections(self._d)

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_sequence(getCompactDescriptor(self._d))

    def length(self):       return self._d[2]
    def content_type(self): return createTypeCode(self._d[1])

# alias:
class TypeCode_alias (CORBA.TypeCode):
    def __init__(self, desc, parent):
        if type(desc) != types.TupleType or \
           desc[0] != tv_alias:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_alias
        self._p = parent

    def __del__(self):
        if self._p is None and removeIndirections is not None:
            removeIndirections(self._d)

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_alias(getCompactDescriptor(self._d))

    def id(self):           return self._d[1]
    def name(self):         return self._d[2]
    def content_type(self): return createTypeCode(self._d[3])

# except:
class TypeCode_except (CORBA.TypeCode):
    def __init__(self, desc, parent):
        if type(desc) != types.TupleType or \
           desc[0] != tv_except:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_except
        self._p = parent

    def __del__(self):
        if self._p is None and removeIndirections is not None:
            removeIndirections(self._d)

    def equivalent(self, tc):
        # ***
        pass

    def get_compact_typecode(self):
        return TypeCode_except(getCompactDescriptor(self._d))

    def id(self):                 return self._d[2]
    def name(self):               return self._d[3]
    def member_count(self):       return (len(self._d) - 4) / 2
    def member_name(self, index):
        off = index * 2 + 4
        if index < 0 or off >= len(self._d): raise CORBA.TypeCode.Bounds
        return self._d[off]

    def member_type(self, index):
        off = index * 2 + 5
        if index < 0 or off >= len(self._d): raise CORBA.TypeCode.Bounds
        if self._p is None and removeIndirections is not None:
            return createTypeCode(self._d[off], self)
        else:
            return createTypeCode(self._d[off], self._p)


# Functions to compact descriptors:
def getCompactDescriptor(d):
    seen = {}
    ind  = []
    r = r_getCompactDescriptor(d, seen, ind)

    # Fix up indirections:
    for i in ind:
        try:
            i[0] = seen[id(i[0])]
        except KeyError:
            raise CORBA.BAD_TYPECODE

    return r

def r_getCompactDescriptor(d, seen, ind):
    if type(d) == types.TupleType:
        k = d[0]
    else:
        k = d

    if   k == tv_short:     r = d
    elif k == tv_long:      r = d
    elif k == tv_ushort:    r = d
    elif k == tv_ulong:     r = d
    elif k == tv_float:     r = d
    elif k == tv_double:    r = d
    elif k == tv_boolean:   r = d
    elif k == tv_char:      r = d
    elif k == tv_octet:     r = d
    elif k == tv_any:       r = d
    elif k == tv_TypeCode:  r = d
    elif k == tv_Principal: r = d
    elif k == tv_string:    r = d
    elif k == tv_objref:    r = d
    
    elif k == tv_struct:
        c = list(d)
        c[2] = ""
        c[3] = ""
        for i in range(4, len(c), 2):
            c[i]   = ""
            c[i+1] = r_getCompactDescriptor(d[i+1], seen, ind)

        r = tuple(c)
    
    elif k == tv_union:
        c = list(d)
        c[2] = ""
        c[3] = ""
        c[4] = r_getCompactDescriptor(d[4], seen, ind)

        m = []
        for u in d[6]:
            m.append((u[0], "", r_getCompactDescriptor(u[2], seen, ind)))

        c[6] = tuple(m)

        if d[7] is not None:
            c[7] = (d[7][0], "", r_getCompactDescriptor(d[7][2], seen, ind))

        r = tuple(c)
        
    elif k == tv_enum:
        m = []
        for e in d[3]:
            m.append(omniORB.AnonymousEnumItem(e._v))
        r = (k, "", "", tuple(m))

    elif k == tv_sequence:
        r = (k, r_getCompactDescriptor(d[1], seen, ind), d[2])
        
    elif k == tv_array:
        r = (k, r_getCompactDescriptor(d[1], seen, ind), d[2])

    elif k == tv_alias:
        r = (k, "", "", r_getCompactDescriptor(d[3], seen, ind))

    elif k == tv_except:
        c = list(d)
        c[3] = ""
        for i in range(4, len(c), 2):
            c[i]   = ""
            c[i+1] = r_getCompactDescriptor(d[i+1], seen, ind)

        r = tuple(c)

    elif k == tv__indirect:
        l = [d[1][0]]
        ind.append(l)
        r = (k, l)

    else: raise CORBA.INTERNAL

    seen[id(d)] = r
    return r


# Function to remove indirections from a descriptor, so it can be
# collected by Python's reference counting garbage collector:

def removeIndirections(desc):
    if type(desc) != types.TupleType: return

    k = desc[0]

    if k == tv_struct:
        for i in range(5, len(desc), 2):
            removeIndirections(desc[i])

    elif k == tv_union:
        for t in desc[6]:
            removeIndirections(t[2])
        if desc[7] is not None:
            removeIndirections(desc[7][2])

    elif k == tv_sequence:
        removeIndirections(desc[1])

    elif k == tv_array:
        removeIndirections(desc[1])

    elif k == tv_alias:
        removeIndirections(desc[3])

    elif k == tv_except:
        for i in range(5, len(desc), 2):
            removeIndirections(desc[i])

    elif k == tv__indirect:
        del(desc[1][0])
