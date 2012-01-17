/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * \file   ComfortPluginInterface.hpp
 * \author Peter Schüller
 *
 * \brief High-level interface for plugins.
 *
 * This interface is not as efficient as directly implementing
 * PluginInterface, but the plugin writer does not need to care about IDs
 * and the Registry.
 *
 * If you convert dlvhex 1.X plugins to dlvhex 2.X, you might want to use
 * this interface.
 */

#ifndef COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011
#define COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

#include <cctype>

DLVHEX_NAMESPACE_BEGIN

// use ModelCallback from PluginInterface.h
// use FinalCallback from PluginInterface.h
// use PluginConverter from PluginInterface.h
// TODO rewriter, optimizer?
// use the original PluginInterface, and simply register ComfortPluginAtoms instead of PluginAtoms

// you can stream out ComfortTerm objects, e.g., for debugging
struct ComfortTerm:
  public ostream_printable<ComfortTerm>
{
  enum Type { STR, INT };

  Type type;
  std::string strval;
  int intval;

  bool isConstant() const
    { return (type == STR) && (!isupper(strval[0])); }
  // that's how we represent variables
  bool isVariable() const
    { return (type == STR) && (isupper(strval[0])); }
  bool isInteger() const
    { return type == INT; }

  static ComfortTerm createVariable(const std::string& s)
    { assert(!s.empty() && isupper(s[0])); return ComfortTerm(STR, s, 0); }
  static ComfortTerm createConstant(const std::string& s)
    { assert(!s.empty() && !isupper(s[0])); return ComfortTerm(STR, s, 0); }
  static ComfortTerm createInteger(int i)
    { return ComfortTerm(INT, "", i); }

  // equality
  inline bool operator==(const ComfortTerm& other) const
    { return
        (type == other.type) &&
        (type == STR || intval == other.intval) &&
        (type == INT || strval == other.strval); }
  inline bool operator!=(const ComfortTerm& other) const
    { return !operator==(other); }
  // comparability (for putting ComfortTerm into sets)
  inline bool operator<(const ComfortTerm& other) const
    { return
        (type < other.type) ||
        (type == STR && other.type == STR && strval < other.strval) ||
        (type == INT && other.type == INT && intval < other.intval); }

  // non-virtual (see Printhelpers.hpp)
  std::ostream& print(std::ostream& o) const;

protected:
  // stupid constructor, use "create..." functions
  ComfortTerm(Type type, const std::string& strval, int intval):
    type(type), strval(strval), intval(intval) {}
};

typedef std::vector<ComfortTerm> ComfortTuple;

// you can stream out ComfortAtom objects, e.g., for debugging
struct ComfortAtom:
  public ostream_printable<ComfortAtom>
{
  #warning TODO strong negation as prefix of first constant!
  ComfortTuple tuple;

  inline const std::string& toString() const
    { if( strval.empty() ) calculateStrVal(); return strval; }

  // comparability (for putting ComfortAtom into sets)
  bool operator<(const ComfortAtom& other) const
    { return tuple < other.tuple; }

  // non-virtual (see Printhelpers.hpp)
  std::ostream& print(std::ostream& o) const;

  inline const std::string& getPredicate() const
    { assert(!tuple.empty() && !tuple.front().isInteger());
      return tuple.front().strval; }
  // TODO implement setArgument, setArguments, setPredicate, getArguments, getArgument, getArity, isStrongNegated

  bool unifiesWith(const ComfortAtom& other) const;

protected:
  mutable std::string strval;
  void calculateStrVal() const;
};

// this mimicks the old AtomSet
// you can stream out ComfortInterpretation objects, e.g., for debugging
struct ComfortInterpretation;
struct ComfortInterpretation:
  public std::set<ComfortAtom>,
  public ostream_printable<ComfortInterpretation>
{
  // adders

  // insert one atom
  void insert(const ComfortAtom&);

  // insert all atoms from other interpretation
  void insert(const ComfortInterpretation&);

  // removers

  // remove atoms whose predicate matches a string in the given set
  void remove(const std::set<std::string>& predicates);

  // remove atoms whose predicate does not match any string in the given set
  void keep(const std::set<std::string>& predicates);

  // remove negative atoms
  #warning todo strong negation
  void keepPos();

  // accessors/helpers

  bool isConsistent() const;

  // copy all atoms that match the specified predicate into destination interpretation
  void matchPredicate(const std::string& predicate, ComfortInterpretation& destination) const;

  // copy all atoms that unify with the specified predicate into destination interpretation
  void matchAtom(const ComfortAtom& atom, ComfortInterpretation& destination) const;

  // return set difference *this \ subtractThis
  ComfortInterpretation difference(const ComfortInterpretation& subtractThis) const;

  // non-virtual (see Printhelpers.hpp)
  std::ostream& print(std::ostream& o) const;
};

class ComfortPluginAtom:
  public PluginAtom
{
public:
  struct ComfortQuery
  {
    ComfortInterpretation interpretation;
    ComfortTuple input;
    ComfortTuple pattern;
  };

  typedef std::set<ComfortTuple>
    ComfortAnswer;

  // as in PluginAtom, your constructor must use addInput...() methods to
  // define inputs and must use setOutputArity().
  ComfortPluginAtom(const std::string& predicate, bool monotonic=false):
    PluginAtom(predicate, monotonic) {}

  virtual ~ComfortPluginAtom() {}

  // you have to implement this method
  virtual void retrieve(const ComfortQuery&, ComfortAnswer&) = 0;

protected:
  // we implemented the original retrieve here, so you don't have to take care of this anymore
  virtual void retrieve(const Query& q, Answer& a);
};

DLVHEX_NAMESPACE_END

#endif // COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011
