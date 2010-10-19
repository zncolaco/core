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
 * @file   TestDependencyGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test the dependency graph builder (and the graph)
 */

#include <boost/cstdint.hpp>
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"

#define BOOST_TEST_MODULE "TestDependencyGraph"
#include <boost/test/unit_test.hpp>

#include <iostream>

#define LOG_REGISTRY_PROGRAM(ctx) \
  ctx.registry->logContents(); \
	RawPrinter printer(std::cerr, ctx.registry); \
	LOG("edb"); \
	printer.printmany(ctx.edb,"\n"); \
	std::cerr << std::endl; \
	LOG("edb end"); \
	LOG("idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testDependencyGraphConstruction) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss <<
		// a <-(+)-> f(X) (head/head = disjunctive)
    "a v f(X)." << std::endl <<
	  // X(a) -(+)-> f(X) (unifying)
	  // f(b) -(+)-> f(X) (unifying)
	  // b -(+)-> X(a) (head/body = positive)
	  // b -(-)-> f(b) (head/nafbody = negative)
    "b :- X(a), not f(b)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry->ogatoms.getIDByString("a");
  ID idb = ctx.registry->ogatoms.getIDByString("b");
  ID idfb = ctx.registry->ogatoms.getIDByString("f(b)");
  BOOST_REQUIRE((ida | idb | idfb) != ID_FAIL);

  ID idfX = ctx.registry->onatoms.getIDByString("f(X)");
  ID idXa = ctx.registry->onatoms.getIDByString("X(a)");
  BOOST_REQUIRE((idfX | idXa) != ID_FAIL);

	// TODO: build graph and check graph
}