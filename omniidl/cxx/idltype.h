// -*- c++ -*-
//                          Package   : omniidl
// idltype.h                Created on: 1999/10/18
//			    Author    : Duncan Grisby (dpg1)
//
//    Copyright (C) 1999 AT&T Laboratories Cambridge
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
//   Type objects

// $Id$
// $Log$
// Revision 1.1  1999/10/27 14:05:54  dpg1
// *** empty log message ***
//

#ifndef _idltype_h_
#define _idltype_h_

#include <idlutil.h>
#include <idlvisitor.h>
#include <idlscope.h>

// Note on Type object memory management
//
// All type objects which can be are statically allocated; all others
// are allocated on the heap. When Decl objects receive a pointer to a
// type object, they should call its shouldDelete() function to see if
// they are responsible for deleting the object. This only returns
// true for those heap-allocated objects which do not have an
// associated Decl object. Type objects created by Decl constructors
// must be deleted by the Decls' destructors.


class IdlType {
public:

  enum Kind {
    tk_null               = 0,
    tk_void               = 1,
    tk_short              = 2,
    tk_long               = 3,
    tk_ushort             = 4,
    tk_ulong              = 5,
    tk_float              = 6,
    tk_double             = 7,
    tk_boolean            = 8,
    tk_char	          = 9,
    tk_octet              = 10,
    tk_any	          = 11,
    tk_TypeCode           = 12,
    tk_Principal          = 13,
    tk_objref             = 14,
    tk_struct             = 15,
    tk_union              = 16,
    tk_enum	          = 17,
    tk_string             = 18,
    tk_sequence           = 19,
    tk_array              = 20,
    tk_alias              = 21,
    tk_except             = 22,
    tk_longlong           = 23,
    tk_ulonglong          = 24,
    tk_longdouble         = 25,
    tk_wchar              = 26,
    tk_wstring            = 27,
    tk_fixed              = 28,
    tk_value              = 29,
    tk_value_box          = 30,
    tk_native             = 31,
    tk_abstract_interface = 32
  };

  IdlType(Kind k) : kind_(k) { }
  virtual ~IdlType() {};

  Kind        kind()         const { return kind_; }
  const char* kindAsString() const;

  virtual _CORBA_Boolean shouldDelete() = 0;

  virtual void accept(TypeVisitor& visitor) = 0;

  // Find a type given a name. Marks the name used in current scope
  static IdlType* scopedNameToType(const char* file, int line,
				   const ScopedName* sn);

private:
  Kind kind_;
};


class BaseType : public IdlType {
public:
  BaseType(Kind k) : IdlType(k) {}
  virtual ~BaseType() {}

  _CORBA_Boolean shouldDelete() { return 0; }

  void accept(TypeVisitor& visitor) { visitor.visitBaseType(this); }

  // Static base type singletons
  static BaseType nullType;
  static BaseType voidType;
  static BaseType shortType;
  static BaseType longType;
  static BaseType ushortType;
  static BaseType ulongType;
  static BaseType floatType;
  static BaseType doubleType;
  static BaseType booleanType;
  static BaseType charType;
  static BaseType octetType;
  static BaseType anyType;
  static BaseType TypeCodeType;
  static BaseType PrincipalType;
  static BaseType longlongType;
  static BaseType ulonglongType;
  static BaseType longdoubleType;
  static BaseType wcharType;
};


//
// Strings can be used like base types without a declaration. eg:
//
//   void op(in string<10> s);
//
// therefore, the String type must include its bound here, rather than
// relying on looking at the corresponding declaration
//

class StringType : public IdlType {
public:

  StringType(_CORBA_ULong bound) : IdlType(tk_string), bound_(bound) { }
  virtual ~StringType() {}

  _CORBA_ULong   bound()        { return bound_; }
  _CORBA_Boolean shouldDelete() { return bound_ ? 1 : 0; }

  void accept(TypeVisitor& visitor) { visitor.visitStringType(this); }

  static StringType unboundedStringType;

private:
  _CORBA_ULong bound_;
};


class WStringType : public IdlType {
public:
  WStringType(_CORBA_ULong bound) : IdlType(tk_wstring), bound_(bound) { }
  virtual ~WStringType() {}

  _CORBA_ULong   bound()        { return bound_; }
  _CORBA_Boolean shouldDelete() { return bound_ ? 1 : 0; }

  void accept(TypeVisitor& visitor) { visitor.visitWStringType(this); }

  static WStringType unboundedWStringType;

private:
  _CORBA_ULong bound_;
};

//
// Sequences are never declared. They either appear as
//
//   typedef sequence <...> ...
//
// or inside a struct or union
//

// *** Element freeing
class SequenceType : public IdlType {
public:
  SequenceType(IdlType* seqType, _CORBA_ULong bound) :
    IdlType(tk_sequence), seqType_(seqType), bound_(bound) { }

  virtual ~SequenceType() {}

  IdlType*       seqType()      { return seqType_; }
  _CORBA_ULong   bound()        { return bound_; }
  _CORBA_Boolean shouldDelete() { return 1; }

  void accept(TypeVisitor& visitor) { visitor.visitSequenceType(this); }

private:
  IdlType*     seqType_;
  _CORBA_ULong bound_;
};

//
// Same goes for fixed
//

class FixedType : public IdlType {
public:
  FixedType(_CORBA_UShort digits, _CORBA_Short scale) :
    IdlType(tk_fixed), digits_(digits), scale_(scale) { }

  virtual ~FixedType() {}

  _CORBA_UShort  digits()       { return digits_; }
  _CORBA_Short   scale()        { return scale_; }
  _CORBA_Boolean shouldDelete() { return 1; }

  void accept(TypeVisitor& visitor) { visitor.visitFixedType(this); }

private:
  _CORBA_UShort digits_;
  _CORBA_Short  scale_;
};


//
// All other types must be declared, at least implicitly, so they have
// an associated declaration object
//

class Decl;

class DeclaredType : public IdlType {
public:
  DeclaredType(Kind k, Decl* decl) : IdlType(k), decl_(decl) {}

  virtual ~DeclaredType() {}

  Decl* decl() { return decl_; }

  _CORBA_Boolean shouldDelete() { return 0; }
  void accept(TypeVisitor& visitor) { visitor.visitDeclaredType(this); }

  static DeclaredType corbaObjectType;

private:
  Decl* decl_;
};


//
// TypeSpec class is used to distinguish simple_type_spec from
// constr_type_spec in the grammar
//

class TypeSpec {
public:
  TypeSpec(IdlType* type, _CORBA_Boolean constr)
    : type_(type), constr_(constr) {}
  ~TypeSpec() {}

  IdlType*       type()   const { return type_; }
  _CORBA_Boolean constr() const { return constr_; }

private:
  IdlType*       type_;
  _CORBA_Boolean constr_;
};


#endif // _idltype_h_
