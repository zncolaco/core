/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* -*- C++ -*- */

/**
 * @file ASPsolverTest.cpp
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the ASP solver class.
 *
 *
 *
 */

#include "testsuite/dlvhex/ASPsolverTest.h"
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/AtomFactory.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(ASPsolverTest);

void
ASPsolverTest::setUp()
{
    solver = new ASPsolver();

    Globals::Instance()->setOption("NoPredicate", 0);
}

void
ASPsolverTest::tearDown() 
{
    delete solver;

    AtomFactory::Instance()->reset();
}

void
ASPsolverTest::testExecution()
{
    //
    // fatal error:
    //
    std::string prg("p.q");
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), FatalError);
    prg = "p(X):-q.";
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), FatalError);
    prg = "p(a,b).p(c).";
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), FatalError);

    //
    // TODO: if we parametrize the solver-executable later, test
    // also the solver existence!
    //
}

void
ASPsolverTest::testResult()
{
    std::string prg;
    AtomSet *as;

    //
    // no model
    //
    prg = "-a.a.";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    
    //
    // empty model
    //
    prg = "a:-b.";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 0);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    
    //
    // single model
    //
    prg = "a.b:-a.";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 2);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    
    //
    // two models
    //
    prg = "p(X):-not q(X),s(X).q(X):-not p(X),s(X).s(a).";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 2);
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 2);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);

    //
    // strings
    //
    std::string str("\"quoted string, includes some (nasty) special-characters!+#'*[]{}\"");
    prg = "e(" + str + ") :- b. b.";

    //
    // now calling with noEDB=1, b should not be in the result then!
    //
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg, 1));
    
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 1);
    CPPUNIT_ASSERT((*(as->begin())).getArgument(1).getString() == str);

    //
    // nothing left
    //
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
}
