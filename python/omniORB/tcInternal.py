# TypeCode internal implementation

# $Id$

# $Log$
# Revision 1.1  1999/06/24 15:23:28  dpg1
# Initial revision
#

import omniORB, CORBA
import types


# Function to create a TypeCode object given a descriptor:

def createTypeCode(d):
    if type(d) == types.TupleType:
        k = d[0]
    else:
        k = d

    if   k == CORBA.tk_short:     return TypeCode_empty(d)
    elif k == CORBA.tk_long:      return TypeCode_empty(d)
    elif k == CORBA.tk_ushort:    return TypeCode_empty(d)
    elif k == CORBA.tk_ulong:     return TypeCode_empty(d)
    elif k == CORBA.tk_float:     return TypeCode_empty(d)
    elif k == CORBA.tk_double:    return TypeCode_empty(d)
    elif k == CORBA.tk_boolean:   return TypeCode_empty(d)
    elif k == CORBA.tk_char:      return TypeCode_empty(d)
    elif k == CORBA.tk_octet:     return TypeCode_empty(d)
    elif k == CORBA.tk_any:       return TypeCode_empty(d)
    elif k == CORBA.tk_TypeCode:  return TypeCode_empty(d)
    elif k == CORBA.tk_Principal: return TypeCode_empty(d)
    elif k == CORBA.tk_string:    return TypeCode_string(d)
    elif k == CORBA.tk_objref:    return TypeCode_objref(d)
    elif k == CORBA.tk_struct:    return TypeCode_struct(d)
    elif k == CORBA.tk_union:     return TypeCode_union(d)
    elif k == CORBA.tk_enum:      return TypeCode_enum(d)
    elif k == CORBA.tk_sequence:  return TypeCode_sequence(d)
    elif k == CORBA.tk_array:     return TypeCode_array(d)
    elif k == CORBA.tk_alias:     return TypeCode_alias(d)
    elif k == CORBA.tk_except:    return TypeCode_except(d)
    elif k == CORBA.tk__indirect: return createTypeCode(d[1][0])

    raise CORBA.INTERNAL


def getCompactDescriptor(d):
    # ***
    pass


# Class for short, long, ushort, ulong, float, double, boolean, char,
# octet, any, TypeCode, Principal:

class TypeCode_empty (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.IntType: raise CORBA.INTERNAL
        if desc != CORBA.tk_short      and \
           desc != CORBA.tk_long       and \
           desc != CORBA.tk_ushort     and \
           desc != CORBA.tk_ulong      and \
           desc != CORBA.tk_float      and \
           desc != CORBA.tk_double     and \
           desc != CORBA.tk_boolean    and \
           desc != CORBA.tk_char       and \
           desc != CORBA.tk_octet      and \
           desc != CORBA.tk_any        and \
           desc != CORBA.tk_TypeCode   and \
           desc != CORBA.tk_Principal: raise CORBA.INTERNAL

        self._d = desc
        self._k = desc


# string:
class TypeCode_string (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_string:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_string

    def length(self):
        return self._d[1]


# objref:
class TypeCode_objref (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_objref:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_objref

    def id(self):   return self._d[1]
    def name(self): return self._d[2]


# struct:
class TypeCode_struct (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_struct:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_struct

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
        return createTypeCode(self._d[off])
    
# union:
class TypeCode_union (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_union:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_union

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
        return createTypeCode(self._d[6][index][2])

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
           desc[0] != CORBA.tk_enum:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_enum

    def get_compact_typecode(self):
        return TypeCode_enum(getCompactDescriptor(self._d))

    def id(self):           return self._d[1]
    def name(self):         return self._d[2]
    def member_count(self): return len(self._d[3])

    def member_name(self, index):
        if index < 0 or index >= len(self._d[3]): raise CORBA.TypeCode.Bounds
        return self._d[3][index]

# sequence:
class TypeCode_sequence (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_sequence:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_sequence

    def length(self):       return self._d[2]
    def content_type(self): return createTypeCode(self._d[1])

# array:
class TypeCode_array (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_array:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_array

    def length(self):       return self._d[2]
    def content_type(self): return createTypeCode(self._d[1])

# alias:
class TypeCode_alias (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_alias:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_alias

    def get_compact_typecode(self):
        return TypeCode_alias(getCompactDescriptor(self._d))

    def id(self):           return self._d[1]
    def name(self):         return self._d[2]
    def content_type(self): return createTypeCode(self._d[3])

# except:
class TypeCode_except (CORBA.TypeCode):
    def __init__(self, desc):
        if type(desc) != types.TupleType or \
           desc[0] != CORBA.tk_except:
            raise CORBA.INTERNAL
        self._d = desc
        self._k = CORBA.tk_except

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
        return createTypeCode(self._d[off])
