# -*- python -*-
#                           Package   : omniidl
# idlast.py                 Created on: 1999/10/27
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
#   Python definitions for abstract syntax tree classes

# $Id$
# $Log$
# Revision 1.9  1999/11/15 15:49:23  dpg1
# Documentation strings.
#
# Revision 1.8  1999/11/11 15:55:30  dpg1
# Python back-end interface now supports valuetype declarations.
# Back-ends still don't support them, though.
#
# Revision 1.7  1999/11/02 17:07:24  dpg1
# Changes to compile on Solaris.
#
# Revision 1.6  1999/11/02 10:01:46  dpg1
# Minor fixes.
#
# Revision 1.5  1999/11/01 20:19:55  dpg1
# Support for union switch types declared inside the switch statement.
#
# Revision 1.4  1999/11/01 16:39:01  dpg1
# Small fixes and cosmetic changes.
#
# Revision 1.3  1999/11/01 10:05:01  dpg1
# New file attribute to AST.
#
# Revision 1.2  1999/10/29 18:19:39  dpg1
# Clean up
#
# Revision 1.1  1999/10/29 15:47:08  dpg1
# First revision.
#

"""Classes and functions for handling the IDL Abstract Syntax Tree

Function:

  findDecl(scopedName) -- find a Decl object given a fully-scoped
                          name, represented as a list of strings.
                          eg. ::foo::bar::baz is represented as
                          ['foo', 'bar', 'baz'].
Classes:

  AST          -- top level of Abstract Syntax Tree.
  Decl         -- base of all declarations.
  DeclRepoId   -- mixin class for Decls with repository ids.
  Module       -- module declaration.
  Interface    -- interface declaration.
  Forward      -- forward-declared interface.
  Const        -- constant declaration.
  Declarator   -- declarator used in typedef, struct members, etc.
  Typedef      -- typedef.
  Member       -- member of a struct or exception.
  Struct       -- struct declaration.
  Exception    -- exception declaration.
  CaseLabel    -- case label within a union.
  UnionCase    -- one case within a union.
  Union        -- union declaration.
  Enumerator   -- enumerator of an enum.
  Enum         -- enum declaration.
  Attribute    -- attribute declaration.
  Parameter    -- parameter of an operation of factory.
  Operation    -- operation declaration.
  Native       -- native declaration.
  StateMember  -- state member of a valuetype.
  Factory      -- factory method of a valuetype.
  ValueForward -- forward-declared valuetype.
  ValueBox     -- boxed value declaration.
  ValueAbs     -- abstract valuetype declaration.
  Value        -- valuetype declaration."""

import idlutil
import idlvisitor

class AST:
    """Class for top-level Abstract Syntax Tree.

Functions:

  file()          -- the file name of the main IDL file.
  declarations()  -- list of Decl objects corresponding to declarations
                     at file scope.
  pragmas()       -- list of strings containing #pragmas which occurred
                     before any declarations. Later #pragmas are
                     attached to Decl objects.
  accept(visitor) -- visitor pattern accept. See idlvisitor.py."""

    def __init__(self, file, declarations, pragmas):
        self.__file         = file
        self.__declarations = declarations
        self.__pragmas      = pragmas

    def file(self):            return self.__file
    def declarations(self):    return self.__declarations
    def pragmas(self):         return self.__pragmas
    def accept(self, visitor): visitor.visitAST(self)


class Decl:
    """Base class for all declarations.

Functions:

  file()          -- the IDL file this declaration came from.
  line()          -- the line number within the file.
  mainFile()      -- boolean: true if the file was the main IDL file;
                     false if it was an included file.
  pragmas()       -- list of strings containing #pragmas which
                     immediately followed this declaration.
  accept(visitor) -- visitor pattern accept. See idlvisitor.py."""

    def __init__(self, file, line, mainFile, pragmas):
        self.__file     = file
        self.__line     = line
        self.__mainFile = mainFile
        self.__pragmas  = pragmas

    def accept(self, visitor): pass

    def file(self):     return self.__file
    def line(self):     return self.__line
    def mainFile(self): return self.__mainFile
    def pragmas(self):  return self.__pragmas


class DeclRepoId :
    """Mixin class for Decls which have a Repository Id

Functions:

  identifier() -- name of the declaration as a string
  scopedName() -- list of strings forming the fully-scoped name of the
                  declaration. e.g. ::foo::bar::baz is represented as
                  ['foo', 'bar', 'baz'].
  repoId()     -- repository identifier for this declaration."""

    def __init__(self, identifier, scopedName, repoId):
        self.__identifier = identifier
        self.__scopedName = scopedName
        self.__repoId     = repoId

    def identifier(self): return self.__identifier
    def scopedName(self): return self.__scopedName
    def repoId(self):     return self.__repoId


class Module (Decl, DeclRepoId):
    """Module declaration (Decl, DeclRepoId)

Functions:

  definitions()   -- list of Decl objects declared within this module.
  continuations() -- list containing continuations of this module.
                     When modules are re-opened, multiple Module
                     objects with the same name appear in the
                     enclosing Module or AST object. In case it's
                     useful, the first Module object for a particular
                     module has a list containing continuations of
                     that module. You will probably not have any use
                     for this."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 definitions):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__definitions  = definitions
        self._continuations = []

    def accept(self, visitor): visitor.visitModule(self)

    def definitions(self):   return self.__definitions
    def continuations(self): return self._continuations


class Interface (Decl, DeclRepoId):
    """Interface declaration (Decl, DeclRepoId)

Functions:

  abstract()     -- boolean: true if the interface is declared abstract.
  inherits()     -- list of Interface objects from which this one
                    inherits.
  contents()     -- list of Decl objects for all items declared within
                    this interface.
  declarations() -- subset of contents() containing types, constants
                    and exceptions.
  callables()    -- subset of contents() containing Operations and
                    Attributes."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 abstract, inherits):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__abstract     = abstract
        self.__inherits     = inherits
        self.__contents     = []
        self.__declarations = []
        self.__callables    = []

    def _setContents(self, contents):
        self.__contents     = contents
        self.__declarations = filter(lambda c: not (isinstance(c, Attribute) or
                                                    isinstance(c, Operation)),
                                     contents)
        self.__callables    = filter(lambda c: (isinstance(c, Attribute) or
                                                isinstance(c, Operation)),
                                     contents)

    def accept(self, visitor): visitor.visitInterface(self)

    def abstract(self):     return self.__abstract
    def inherits(self):     return self.__inherits
    def contents(self):     return self.__contents
    def declarations(self): return self.__declarations
    def callables(self):    return self.__callables


class Forward (Decl, DeclRepoId):
    """Forward-declared interface (Decl, DeclRepoId)

Functions:

  abstract() -- boolean: true if the interface is declared abstract.
  fullDecl() -- Interface object corresponding to full interface
                declaration or None if there is no full declaration."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 abstract):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__abstract = abstract
        self._fullDecl  = None
        self._more      = []

    def accept(self, visitor): visitor.visitForward(self)

    def abstract(self): return self.__abstract
    def fullDecl(self): return self._fullDecl


class Const (Decl, DeclRepoId):
    """Constant declaration (Decl, DeclRepoId)

Functions:

  constType() -- IdlType.Type object of this constant. Aliases not
                 stripped.
  constKind() -- TypeCode kind of constant with aliases stripped.
  value()     -- value of the constant. Either an integer or an
                 Enumerator object."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 constType, constKind, value):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__constType = constType
        self.__constKind = constKind
        self.__value     = value
        #print line, "Const init:", constType, identifier, value

    def accept(self, visitor): visitor.visitConst(self)

    def constType(self):  return self.__constType
    def constKind(self):  return self.__constKind
    def value(self):      return self.__value


class Declarator (Decl, DeclRepoId):
    """Declarator used in typedefs, struct members, etc. (Decl, DeclRepoId)

Functions:

  sizes() -- list of array sizes, or None if this is a simple
             declarator.
  alias() -- Typedef object for this declarator if this is a typedef
             declarator. None otherwise."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 sizes):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__sizes = sizes
        self.__alias = None

    def _setAlias(self, alias): self.__alias = alias

    def accept(self, visitor): visitor.visitDeclarator(self)

    def sizes(self): return self.__sizes
    def alias(self): return self.__alias


class Typedef (Decl):
    """Typedef (Decl)

Functions:

  aliasType()   -- IdlType.Type object that this is an alias to.
  constrType()  -- boolean: true if the alias type was constructed
                   within this typedef declaration.
  declarators() -- list of Declarator objects."""

    def __init__(self, file, line, mainFile, pragmas,
                 aliasType, constrType, declarators):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__aliasType   = aliasType
        self.__constrType  = constrType
        self.__declarators = declarators

    def accept(self, visitor): visitor.visitTypedef(self)

    def aliasType(self):   return self.__aliasType
    def constrType(self):  return self.__constrType
    def declarators(self): return self.__declarators


class Member (Decl):
    """Member of a struct or exception (Decl)

Functions:

  memberType()  -- IdlType.Type object for the type of this member.
  constrType()  -- boolean: true if the member type was constructed
                   within the member declaration.
  declarators() -- list of Declarator objects."""

    def __init__(self, file, line, mainFile, pragmas,
                 memberType, constrType, declarators):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__memberType  = memberType
        self.__constrType  = constrType
        self.__declarators = declarators

    def accept(self, visitor): visitor.visitMember(self)

    def memberType(self):  return self.__memberType
    def constrType(self):  return self.__constrType
    def declarators(self): return self.__declarators


class Struct (Decl, DeclRepoId):
    """Struct declaration (Decl, DeclRepoId)

Functions:

  members()   -- list of Member objects for the struct contents.
  recursive() -- boolean: true if the struct is recursive."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 recursive):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__recursive = recursive

    def _setMembers(self, members):
        self.__members = members

    def accept(self, visitor): visitor.visitStruct(self)

    def members(self):    return self.__members
    def recursive(self):  return self.__recursive


class Exception (Decl, DeclRepoId):
    """Exception declaration (Decl, DeclRepoId)

Function:

  members() -- list of Member objects for the exception contents."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 members):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__members    = members
        #print line, "Exception init:", identifier, members

    def accept(self, visitor): visitor.visitException(self)

    def members(self):    return self.__members


class CaseLabel (Decl):
    """Case label within a union (Decl)

Functions:

  default()   -- boolean: true is this is the default label.
  value()     -- label value. Either an integer or an Enumerator
                 object. If default() is true, returns a value used by
                 none of the other union labels.
  labelKind() -- TypeCode kind of label."""

    def __init__(self, file, line, mainFile, pragmas,
                 default, value, labelKind):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__default   = default
        self.__value     = value
        self.__labelKind = labelKind

    def accept(self, visitor): visitor.visitCaseLabel(self)

    def default(self): return self.__default
    def value(self): return self.__value
    def labelKind(self): return self.__labelKind


class UnionCase (Decl):
    """One case within a union (Decl)

Functions:

  labels()     -- list of CaseLabel objects.
  caseType()   -- IdlType.Type object for the case type.
  constrType() -- boolean: true if the case type was constructed
                  within the case.
  declarator() -- Declarator object"""

    def __init__(self, file, line, mainFile, pragmas,
                 labels, caseType, constrType, declarator):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__labels     = labels
        self.__caseType   = caseType
        self.__constrType = constrType
        self.__declarator = declarator

    def accept(self, visitor): visitor.visitUnionCase(self)

    def labels(self):     return self.__labels
    def caseType(self):   return self.__caseType
    def constrType(self): return self.__constrType
    def declarator(self): return self.__declarator


class Union (Decl, DeclRepoId):
    """Union declaration (Decl, DeclRepoId)

Functions:

  switchType() -- IdlType.Type object corresponding to the switch type.
  constrType() -- boolean: true if the switch type was declared within
                  the switch statement. Only possible for Enums.
  cases()      -- list of UnionCase objects.
  recursive()  -- boolean: true if the union is recursive."""
  

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 switchType, constrType, recursive):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__switchType = switchType
        self.__constrType = constrType
        self.__recursive  = recursive

    def _setCases(self, cases):
        self.__cases = cases

    def accept(self, visitor): visitor.visitUnion(self)

    def switchType(self): return self.__switchType
    def constrType(self): return self.__constrType
    def cases(self):      return self.__cases
    def recursive(self):  return self.__recursive


class Enumerator (Decl, DeclRepoId):
    """Enumerator of an Enum (Decl, DeclRepoId)

No non-inherited functions."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

    def accept(self, visitor): visitor.visitEnumerator(self)


class Enum (Decl, DeclRepoId):
    """Enum declaration (Decl, DeclRepoId)

Function:

  enumerators() -- list of Enumerator objects."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 enumerators):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__enumerators = enumerators

    def accept(self, visitor): visitor.visitEnum(self)

    def enumerators(self): return self.__enumerators


class Attribute (Decl):
    """Attribute declaration (Decl)

Functions:

  readonly()    -- boolean: true if the attribute is read only.
  attrType()    -- IdlType.Type object for the attribute's type.
  identifiers() -- list of strings containing the attribute identifiers."""

    def __init__(self, file, line, mainFile, pragmas,
                 readonly, attrType, identifiers):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__readonly = readonly
        self.__attrType = attrType
        self.__identifiers = identifiers
        #print line, "Attribute init:", readonly, identifiers

    def accept(self, visitor): visitor.visitAttribute(self)

    def readonly(self):    return self.__readonly
    def attrType(self):    return self.__attrType
    def identifiers(self): return self.__identifiers


class Parameter (Decl):
    """A Parameter of an operation or factory specifier (Decl)

Functions:

  direction()  -- integer: 0 == in, 1 == out, 2 == inout.
  is_in()      -- boolean: true if in or inout.
  is_out()     -- boolean: true if out or inout.
  paramType()  -- IdlType.Type object for the parameter type.
  identifier() -- string of parameter identifier."""

    def __init__(self, file, line, mainFile, pragmas,
                 direction, paramType, identifier):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__direction  = direction
        self.__is_in      = (direction == 0 or direction == 2)
        self.__is_out     = (direction == 1 or direction == 2)
        self.__paramType  = paramType
        self.__identifier = identifier
        #print line, "Parameter init:", identifier

    def accept(self, visitor): visitor.visitParameter(self)

    def direction(self):  return self.__direction
    def is_in(self):      return self.__is_in
    def is_out(self):     return self.__is_out
    def paramType(self):  return self.__paramType
    def identifier(self): return self.__identifier


class Operation (Decl):
    """Operation declaration (Decl)

Functions:

  oneway()     -- boolean: true if operation is one way.
  returnType() -- IdlType.Type object for return type.
  identifier() -- string of operation identifier.
  parameters() -- list of Parameter objects.
  raises()     -- list of Exception objects.
  contexts()   -- list of strings for context expressions."""

    def __init__(self, file, line, mainFile, pragmas,
                 oneway, returnType, identifier,
                 parameters, raises, contexts):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__oneway     = oneway
        self.__returnType = returnType
        self.__identifier = identifier
        self.__parameters = parameters
        self.__raises     = raises
        self.__contexts   = contexts
        #print line, "Operation init:", identifier, raises, contexts

    def accept(self, visitor): visitor.visitOperation(self)

    def oneway(self):     return self.__oneway
    def returnType(self): return self.__returnType
    def identifier(self): return self.__identifier
    def parameters(self): return self.__parameters
    def raises(self):     return self.__raises
    def contexts(self):   return self.__contexts


class Native (Decl, DeclRepoId):
    """Native declaration (Decl, DeclRepoId)

Native should not be used in normal IDL.

No non-inherited functions."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

    def accept(self, visitor): visitor.visitNative(self)


class StateMember (Decl):
    """State member of a valuetype (Decl)

Functions:

  memberAccess() -- integer: 0 == public, 1 == private.
  memberType()   -- IdlType.Type object for member type.
  constrType()   -- boolean: true if member type is declared within
                    the StateMember.
  declarators()  -- list of Declarator objects."""

    def __init__(self, file, line, mainFile, pragmas,
                 memberAccess, memberType, constrType, declarators):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__memberAccess = memberAccess
        self.__memberType   = memberType
        self.__constrType   = constrType
        self.__declarators  = declarators

    def accept(self, visitor): visitor.visitStateMember(self)

    # Access specifier: 0 for public, 1 for private
    def memberAccess(self): return self.__memberAccess
    def memberType(self):   return self.__memberType
    def constrType(self):   return self.__constrType
    def declarators(self):  return self.__declarators
    

class Factory (Decl):
    """Factory method of valuetype (Decl)

Functions:

  identifier() -- string.
  parameters() -- list of Parameter objects."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, parameters):

        Decl.__init__(self, file, line, mainFile, pragmas)

        self.__identifier = identifier
        self.__parameters = parameters

    def accept(self, visitor): visitor.visitFactory(self)

    def identifier(): return self.__identifier
    def parameters(): return self.__parameters


class ValueForward (Decl, DeclRepoId):
    """Forward declared valuetype (Decl, DeclRepoId)

Function:

  abstract() -- boolean: true if declared abstract.
  fullDecl() -- Value or ValueAbs object corresponding to the full
                valuetype declaration or None if there is no full
                declaration."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 abstract):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__abstract = abstract
        self._fullDecl  = None
        self._more      = []

    def accept(self, visitor): visitor.visitValueForward(self)

    def abstract(self): return self.__abstract
    def fullDecl(self): return self._fullDecl


class ValueBox (Decl, DeclRepoId):
    """ValueBox declaration (Decl, DeclRepoId)

Functions:

  boxedType()  -- IdlType.Type object for boxed type.
  constrType() -- boolean: true if boxed type is declared inside the
                  ValueBox declaration."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 boxedType, constrType):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__boxedType  = boxedType
        self.__constrType = constrType

    def accept(self, visitor): visitor.visitValueBox(self)

    def boxedType(self):  return self.__boxedType
    def constrType(self): return self.__constrType


class ValueAbs (Decl, DeclRepoId):
    """Abstract valuetype declaration (Decl, DeclRepoId)

Functions:

  inherits()     -- list of ValueAbs objects from which this inherits.
  supports()     -- list of Interface object which this supports.
  contents()     -- list of Decl objects for declarations within this
                    valuetype.
  declarations() -- subset of contents() containing types, constants and
                    exceptions.
  callables()    -- subset of contents() containing Operations and
                    Attributes."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 inherits, supports):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__inherits     = inherits
        self.__supports     = supports
        self.__contents     = []
        self.__declarations = []
        self.__callables    = []

    def _setContents(self, contents):
        self.__contents     = contents
        self.__declarations = filter(lambda c: not \
                                     (isinstance(c, Attribute) or
                                      isinstance(c, Operation)),
                                     contents)
        self.__callables    = filter(lambda c: \
                                     (isinstance(c, Attribute) or
                                      isinstance(c, Operation)),
                                     contents)

    def accept(self, visitor): visitor.visitValueAbs(self)

    def inherits(self):     return self.__inherits
    def supports(self):     return self.__supports
    def contents(self):     return self.__contents
    def declarations(self): return self.__declarations
    def callables(self):    return self.__callables


class Value (Decl, DeclRepoId):
    """valuetype declaration (Decl, DeclRepoId)

Functions:

  custom()       -- boolean: true if declared custom.
  inherits()     -- list of valuetypes from which this inherits. The
                    first may be a Value object or a ValueAbs object;
                    any others will be ValueAbs objects.
  truncatable()  -- boolean: true if the inherited Value is declared
                    truncatable.
  supports()     -- list of Interface object which this supports.
  contents()     -- list of Decl objects for all items declared within
                    this valuetype.
  declarations() -- subset of contents() containing types, constants
                    and exceptions.
  callables()    -- subset of contents() containing Operations,
                    Attributes, StateMembers and Factorys."""

    def __init__(self, file, line, mainFile, pragmas,
                 identifier, scopedName, repoId,
                 custom, inherits, truncatable, supports):

        Decl.__init__(self, file, line, mainFile, pragmas)
        DeclRepoId.__init__(self, identifier, scopedName, repoId)

        self.__custom       = custom
        self.__inherits     = inherits
        self.__truncatable  = truncatable
        self.__supports     = supports
        self.__contents     = []
        self.__declarations = []
        self.__callables    = []

    def _setContents(self, contents):
        self.__contents     = contents
        self.__declarations = filter(lambda c: not \
                                     (isinstance(c, Attribute) or
                                      isinstance(c, Operation) or
                                      isinstance(c, StateMember) or
                                      isinstance(c, Factory)),
                                     contents)
        self.__callables    = filter(lambda c: \
                                     (isinstance(c, Attribute) or
                                      isinstance(c, Operation) or
                                      isinstance(c, StateMember) or
                                      isinstance(c, Factory)),
                                     contents)

    def accept(self, visitor): visitor.visitValueAbs(self)

    def custom(self):       return self.__custom
    def inherits(self):     return self.__inherits
    def truncatable(self):  return self.__truncatable
    def supports(self):     return self.__supports
    def contents(self):     return self.__contents
    def declarations(self): return self.__declarations
    def callables(self):    return self.__callables



# Map of Decl objects, indexed by stringified scoped name, and
# functions to access it

declMap = {}

def registerDecl(scopedName, decl):
    """Private function"""
    sname = idlutil.slashName(scopedName)
    if declMap.has_key(sname):

        rdecl = declMap[sname]

        if (isinstance(decl, Interface) and isinstance(rdecl, Forward)) or \
           ((isinstance(decl, ValueAbs) or isinstance(decl, Value)) and \
            isinstance(rdecl, ValueForward)):

            # resolving a forward declaration
            rdecl._fullDecl = decl
            for f in rdecl._more: f._fullDecl = decl
            declMap[sname] = decl

        elif (isinstance(decl, Forward) and isinstance(rdecl, Forward)) or \
             (isinstance(decl, ValueForward) and \
              isinstance(rdecl, ValueForward)):
            
            # repeat forward declaration
            rdecl._more.append(decl)

        elif isinstance(decl, Module) and \
             isinstance(rdecl, Module):
            # continued module
            rdecl._continuations.append(decl)

        else:
            print "***Warning: attempt to re-register", sname

        return

    declMap[sname] = decl


class DeclNotFound:

    """Exception to indicate that findDecl() could not find the
    requested Decl object."""

    def __init__(self, scopedName):
        self.__scopedName = scopedName

    def scopedName(self): return self.__scopedName


def findDecl(scopedName):
    """findDecl(scopedName) -> Decl

Find a Decl object given a fully scoped name represented as a list of
strings. Raises DeclNotFound if the name is not recognised."""

    sname = idlutil.slashName(scopedName)
    if not declMap.has_key(sname):
        raise DeclNotFound(scopedName)
    
    return declMap[sname]


# Declarations of non-basic `built-in' types

CORBAObject = Interface("<built in>", 0, 0, [],
                        "Object", ["CORBA", "Object"],
                        "IDL:omg.org/CORBA/Object:1.0",
                        0, [])
registerDecl(["CORBA", "Object"], CORBAObject)

CORBAModule = Module("<built in>", 0, 0, [], "CORBA", ["CORBA"],
                     "IDL:omg.org/CORBA:1.0", [CORBAObject])
registerDecl(["CORBA"], CORBAModule)
