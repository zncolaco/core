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
 * @file   PluginInterface.cpp
 * @author Roman Schindlauer
 * @date   Mon Oct 17 14:37:07 CEST 2005
 * 
 * @brief Definition of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 * 
 *      
 */     

#include "dlvhex/PluginInterface.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Term.hpp"
#include "dlvhex/ID.hpp"

DLVHEX_NAMESPACE_BEGIN

#if 0
bool PluginAtom::Query::operator<(const Query& other) const
{
	/*
  return
    ( interpretation < other.interpretation ) ||
    ( interpretation == other.interpretation &&
      input < other.input ) ||
    ( interpretation == other.interpretation &&
      input == other.input &&
      pattern < other.pattern );
			*/
}
        
bool PluginAtom::Query::operator==(const Query& other) const
{
  return
      interpretation == other.interpretation &&
      input == other.input &&
      pattern == other.pattern;
}
#endif
        
PluginAtom::Answer::Answer():
  output(new std::vector<Tuple>)
{
}

void
PluginAtom::addInputPredicate()
{
	// throw error if last input term was tuple
	if (inputType.size() > 0)
		if (inputType.back() == TUPLE)
			throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(PREDICATE);
}


void
PluginAtom::addInputConstant()
{
	// throw error if last input term was tuple
	if (inputType.size() > 0)
		if (inputType.back() == TUPLE)
			throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(CONSTANT);
}


void
PluginAtom::addInputTuple()
{
    inputType.push_back(TUPLE);
}


bool
PluginAtom::checkInputArity(const unsigned arity) const
{
  bool ret = (inputType.size() == arity);

  if (!inputType.empty())
    {
      return inputType.back() == TUPLE ? true : ret;
    }
  else
    {
      return ret;
    }
}


void
PluginAtom::setOutputArity(const unsigned arity)
{
    outputSize = arity;
}


bool
PluginAtom::checkOutputArity(const unsigned arity) const
{
    return arity == outputSize;
}


void PluginAtom::retrieveCached(const Query& query, Answer& answer) throw (PluginError)
{
  // Cache answer for queries which were already done once:
  //
  // The most efficient way would be:
  // * use cache for same inputSet + same *inputi + more specific pattern
  // * store new cache for new inputSet/*inputi combination or unrelated (does not unify) pattern
  // * replace cache for existing inputSet/*inputi combination and less specific (unifies in one direction) pattern
  // 
  // The currently implemented "poor (wo)man's version" is:
  // * store answers in cache with queries as keys, disregard relations between patterns
  ///@todo: efficiency could be increased for certain programs by considering pattern relationships as indicated above

#if 0
  #include "dlvhex/PrintVisitor.h"
  #include <iostream>
  std::cerr << "cache:" << std::endl;
  for( QueryAnswerCache::const_iterator i = queryAnswerCache.begin(); i != queryAnswerCache.end(); ++i)
  {
	  std::cerr << "  query: <";
	  RawPrintVisitor visitor(std::cerr);
	  i->first.getInterpretation().accept(visitor);
	  std::cerr << "|" << i->first.getInputTuple() << "|" << i->first.getPatternTuple() << ">" << std::endl;
  }


	  std::cerr << "retrieving query: <";
	  RawPrintVisitor visitor(std::cerr);
	  query.getInterpretation().accept(visitor);
	  std::cerr << "|" << query.getInputTuple() << "|" << query.getPatternTuple() << ">";
#endif

  #if 0
  QueryAnswerCache::const_iterator it = queryAnswerCache.find(query);
  if( it != queryAnswerCache.end() )
  {
    answer = it->second;
  }
  else
  {
    retrieve(query, answer);

    // store in cache
    queryAnswerCache.insert(std::make_pair(query, answer));
  }
  #endif

  retrieve(query, answer);
}


const std::vector<PluginAtom::InputType>&
PluginAtom::getInputTypes() const
{
  return inputType;
}

PluginAtom::InputType
PluginAtom::getInputType(const unsigned index) const
{
	if (index >= inputType.size())
	{
		assert(inputType.back() == TUPLE);
		return inputType.back();
	}

    return inputType[index];
}

void PluginAtom::setRegistry(RegistryPtr reg)
{
  // i think we really don't want to change registry during the lifetime,
  // it would invalidate the cache and more bad things would happen
  assert(registry == 0);
  assert(reg != 0);
  registry = reg;
  predicateID = registry->terms.getIDByString(predicate);
  if( predicateID == ID_FAIL )
  {
    Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, predicate);
    predicateID = registry->terms.storeAndGetID(t);
  }
  assert(predicateID != ID_FAIL);
}

// fail if registry does not exists
// calculate and register replacementPredicateID if not existing
// return replacementPredicateID
ID PluginAtom::getReplacementPredicateID()
{
  if( replacementPredicateID == ID_FAIL )
  {
    assert(registry != 0);
    std::stringstream s;
    s << "aux_ext_" << predicate;
    Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT | ID::PROPERTY_TERM_AUX, s.str());
    ID tmp = registry->terms.getIDByString(t.symbol);
    // we do not want this to exist!
    assert(tmp == ID_FAIL);
    replacementPredicateID = registry->terms.storeAndGetID(t);
  }
  assert(replacementPredicateID != ID_FAIL);
  return replacementPredicateID;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
