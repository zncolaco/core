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
 * @file SafetyChecker.cpp
 * @author Roman Schindlauer
 * @date Mon Feb 27 15:08:46 CET 2006
 *
 * @brief Class for checking rule and program safety.
 *
 *
 */

#include <sstream>

#include "dlvhex/SafetyChecker.h"
#include "dlvhex/globals.h"
#include "dlvhex/AggregateAtom.h"


SafetyCheckerBase::SafetyCheckerBase()
{
}



SafetyChecker::SafetyChecker(const Program& program)
    : SafetyCheckerBase()
{
    testRules(program);
}


void
SafetyChecker::testRules(const Program& program) const throw (SyntaxError)
{
    if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
        Globals::Instance()->getVerboseStream() << std::endl << "Checking for rule safety." << std::endl;

    Program::const_iterator ruleit = program.begin();

    while (ruleit != program.end())
    {
        //
        // testing for simple rule safety:
        // * Each variable occurs in a positive ordinary atom.
        // * A variable occurs in the output list of an external atom and all
        //   input variables occur in a positive ordinary atom.
        //
        // -> 1) get all ordinary body atoms -> safeset
        //    2) look at extatoms: each input var must be in safeset
        //    3) if all is ok: add ext-atom arguments to safeset
        //    4) test if all head vars are in safeset
        //

        const RuleHead_t head = (*ruleit)->getHead();
        const RuleBody_t body = (*ruleit)->getBody();

        //
        // set of all variables in non-ext body atoms
        //
        std::set<Term> safevars;

        //
        // 1)
        // going through the rule body
        //

        RuleBody_t::const_iterator bb = body.begin(), be = body.end();

        while (bb != be)
        {
            //
            // only look at ordinary atoms
            // and aggregate terms
            //
            if ((typeid(*((*bb)->getAtom())) == typeid(Atom)) ||
                (typeid(*((*bb)->getAtom())) == typeid(BuiltinPredicate)) ||
                (typeid(*((*bb)->getAtom())) == typeid(AggregateAtom)))
            {
                //
                // look at predicate
                //
                Term pred = (*bb)->getAtom()->getPredicate();

                //
                // look at arguments
                //
                Tuple bodyarg = (*bb)->getAtom()->getArguments();

				//
				// in case of BuiltinPredicate: only equality with only one
				// variable is safe, just like in dlv
				//
				if (typeid(*((*bb)->getAtom())) == typeid(BuiltinPredicate))
				{
					if (pred == Term("="))
					{
						if (bodyarg[0].isVariable() && !bodyarg[1].isVariable())
							safevars.insert(bodyarg[0]);
						if (!bodyarg[0].isVariable() && bodyarg[1].isVariable())
							safevars.insert(bodyarg[1]);
					}
				}
				else
				{

					if (pred.isVariable())
						safevars.insert(pred);

					Tuple::const_iterator ordterm = bodyarg.begin();

					while (ordterm != bodyarg.end())
					{
						if ((*ordterm).isVariable())
							safevars.insert(*ordterm);

						ordterm++;
					}
				}
            }

            bb++;
        }
        

        //
        // 2)
        // going through the external atoms
        //
        
        const std::vector<ExternalAtom*> extatoms = (*ruleit)->getExternalAtoms();
        
        std::vector<ExternalAtom*>::const_iterator extit = extatoms.begin();

        while (extit != extatoms.end())
        {
            Tuple inp = (*extit)->getInputTerms();

            Tuple::const_iterator inpterm = inp.begin();

            while (inpterm != inp.end())
            {
                if ((*inpterm).isVariable())
                    if (safevars.find(*inpterm) == safevars.end())
                        throw SyntaxError("rule not safe", 
                                          (*ruleit)->getLine(),
                                          (*ruleit)->getFile());
                inpterm++;
            }

            //
            // 3)
            // this ext-atom is safe - we can add its arguments to the safe set
            //
            Tuple extarg = (*extit)->getArguments();

            Tuple::const_iterator extterm = extarg.begin();

            while (extterm != extarg.end())
            {
                if ((*extterm).isVariable())
                    safevars.insert(*extterm);
                extterm++;
            }

            extit++;
        }


        RuleHead_t::const_iterator hb = head.begin();

        //
        // 4)
        // going through the rule head
        //
        while (hb != head.end())
        {
            Tuple headarg = (*(hb++))->getArguments();

            Tuple::const_iterator headterm = headarg.begin();

            //
            // for each head atom: going through its arguments
            //
            while (headterm != headarg.end())
            {
                Term t(*(headterm++));

                //
                // does this variable occur in any positive body atom?
                //
                if (t.isVariable())
                {
                    if (find(safevars.begin(), safevars.end(), t) == safevars.end())
                    {
                        throw SyntaxError("rule not safe", 
                                          (*ruleit)->getLine(),
                                          (*ruleit)->getFile());
                    }
                }
            }
        }

        if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
        {
			Globals::Instance()->getVerboseStream() << "Rule in ";
            if (!(*ruleit)->getFile().empty())
                Globals::Instance()->getVerboseStream() << (*ruleit)->getFile() << ", ";
			Globals::Instance()->getVerboseStream() << "line " << (*ruleit)->getLine() << " is safe." << std::endl;
        }
        
        //
        // next rule
        //
        ++ruleit;
    }
}



StrongSafetyChecker::StrongSafetyChecker(const Program& program,
                                         const DependencyGraph* dg)
    : SafetyChecker(program)
{
    testStrongSafety(dg);
}


void
StrongSafetyChecker::testStrongSafety(const DependencyGraph* dg) const throw (SyntaxError)
{
    if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
        Globals::Instance()->getVerboseStream() << std::endl << "Checking for strong rule safety." << std::endl;

    //
    // testing for strong safety:
    //
    // A rule is strongly safe, if
    // * it is safe and
    // * if an external atom in the rule is part of a cycle, each variable in
    //   its output list occurs in a positive atom in the body, which does not
    //   belong to the cycle.
    //

    //
    // go through all program components
    // (a ProgramComponent is a SCC with external atom!)
    //
    const std::vector<Component*> components = dg->getComponents();

    std::vector<Component*>::const_iterator compit = components.begin();

    while (compit != components.end())
    {
        if (typeid(**compit) == typeid(ProgramComponent))
        {
            //
            // go through all rules of this component
            //
            ProgramComponent* progcomp = dynamic_cast<ProgramComponent*>(*compit);

            const Program rules = progcomp->getBottom();

            for (Program::const_iterator ruleit = rules.begin();
                 ruleit != rules.end();
                 ++ruleit)
            {
                const RuleHead_t head = (*ruleit)->getHead();
                const RuleBody_t body = (*ruleit)->getBody();

                //
                // for this rule: go through all ext-atoms
                //
                for (std::vector<ExternalAtom*>::const_iterator extit = (*ruleit)->getExternalAtoms().begin();
                     extit != (*ruleit)->getExternalAtoms().end();
                     ++extit)
                {
                    //
                    // is this atom also in the component?
                    // (not all the atoms in the bottom of a component are also
                    // in the component themselves!)
                    //
                    if (!progcomp->isInComponent(*extit))
                        continue;

                    //
                    // ok, this external atom is in the cycle of the component:
                    // now we have to check if each of its output arguments is
                    // strongly safe, i.e., if it occurs in another atom in the
                    // body, which is NOT part of the cycle
                    //
                    Tuple output = (*extit)->getArguments();

                    //
                    // look at all terms in its output list
                    //
                    for (Tuple::const_iterator outterm = output.begin();
                         outterm != output.end();
                         ++outterm)
                    {
                        bool argIsUnsafe = true;

                        RuleBody_t::const_iterator bodylit = body.begin();

                        while (bodylit != body.end())
                        {
                            //
                            // only look at atoms that are not part of the
                            // component!
                            // and only look at ordinary or external atoms
                            // builtins do not make a variable safe!
                            //
                            if ((typeid(*(*bodylit)->getAtom()) == typeid(Atom)) ||
                                (typeid(*(*bodylit)->getAtom()) == typeid(ExternalAtom)))
                            {
                                if (!(*compit)->isInComponent((*bodylit)->getAtom().get()))
                                {
                                    //
                                    // the arguments of this atom are safe
                                    //
                                    Tuple safeargs = (*bodylit)->getAtom()->getArguments();

                                    //
                                    // now see if the current
                                    // extatom-output-argument is one of those
                                    // safe vars
                                    //
                                    for (Tuple::const_iterator safeterm = safeargs.begin();
                                        safeterm != safeargs.end();
                                        ++safeterm)
                                    {
                                        if ((*safeterm).isVariable())
                                        {
                                            if (*safeterm == *outterm)
                                                argIsUnsafe = false;
                                        }
                                    }
                                }
                            }

                            bodylit++;
                        }

                        if (argIsUnsafe)
						{
							std::stringstream s;
							s << "rule not expansion-safe: " << **ruleit;
                            throw SyntaxError(s.str());
						}
                    }
                }
                
                if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
                {
					Globals::Instance()->getVerboseStream() << "Rule " << **ruleit
					                                        << " is expansion-safe." << std::endl;
                }
        
            } // rules-loop end
        }

        compit++;
    }
}
