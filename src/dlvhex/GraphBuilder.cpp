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


/**
 * @file GraphBuilder.cpp
 * @author Roman Schindlauer
 * @date Wed Jan 18 17:43:14 CET 2006
 *
 * @brief Abstract strategy class for finding the dependency edges of a program.
 *
 *
 */

#include "dlvhex/GraphBuilder.h"
#include "dlvhex/Component.h"
#include "dlvhex/globals.h"
#include "dlvhex/Registry.h"
#include "dlvhex/Atom.h"
#include "dlvhex/PluginContainer.h"

DLVHEX_NAMESPACE_BEGIN

void
GraphBuilder::run(const Program& program, NodeGraph& nodegraph, PluginContainer& container)
{
	//
	// in this multimap, we will store the input arguments of type PREDICATE
	// of the external atoms. see below.
	//
	std::multimap<Term, AtomNodePtr> extinputs;


	//
	// empty the NodeGraph
	//
	nodegraph.reset();


	// we start with rule-id 1, 0 is reserved for EXTERNAL and UNIFYING
//	   unsigned ruleID = 1;

	//
	// go through all rules of the given program
	//
	for (Program::iterator r = program.begin();
		 r != program.end();
		 ++r)
	{
		//
		// all nodes of the current rule's head
		//
		std::vector<AtomNodePtr> currentHeadNodes;

		//
		// go through entire head disjunction
		//
		const RuleHead_t& head = (*r)->getHead();

		for (RuleHead_t::const_iterator hi = head.begin();
			 hi != head.end();
			 ++hi)
		{
			//
			// add a head atom node. This function will take care of also adding
			// the appropriate unifying dependency for all existing nodes.
			//
			AtomNodePtr hn = nodegraph.addUniqueHeadNode(*hi);

			//
			// go through all head atoms that were alreay created for this rule
			//
			for (std::vector<AtomNodePtr>::iterator currhead = currentHeadNodes.begin();
				 currhead != currentHeadNodes.end();
				 ++currhead)
			{
				//
				// and add disjunctive dependency
				//
				Dependency::addDep(*r, hn, *currhead, Dependency::DISJUNCTIVE);
				Dependency::addDep(*r, *currhead, hn, Dependency::DISJUNCTIVE);
			}

			//
			// add this atom to current head
			//
			currentHeadNodes.push_back(hn);
		}

		//
		// constraint: create virtual head node to keep the rule
		//
		if (head.size() == 0)
		{
			AtomPtr va = Registry::Instance()->storeAtom(new boolAtom);
			
			AtomNodePtr vn = nodegraph.addUniqueHeadNode(va);

			currentHeadNodes.push_back(vn);
		}


		//std::vector<AtomNodePtr> currentOrdinaryBodyNodes;
		std::vector<AtomNodePtr> currentBodyNodes;
		std::vector<AtomNodePtr> currentExternalBodyNodes;

		//
		// go through rule body
		//

		const RuleBody_t& body = (*r)->getBody();

		for (RuleBody_t::const_iterator li = body.begin();
				li != body.end();
				++li)
		{
			//
			// add a body atom node. This function will take care of also adding the appropriate
			// unifying dependency for all existing nodes.
			//
			AtomNodePtr bn = nodegraph.addUniqueBodyNode((*li)->getAtom());

			//
			// save all atoms and - separately - external atoms of this body -
			// after we are through the entire body, we might have to update
			// EXTERNAL dependencies inside the rule and build auxiliary rules!
			//
			//if ((typeid(*((*li)->getAtom())) == typeid(Atom)) &&
			if (!(*li)->isNAF())
				currentBodyNodes.push_back(bn);

			if (typeid(*((*li)->getAtom())) == typeid(ExternalAtom))
			{
				// not yet:
				//assert(!(*li)->isNAF());

				currentExternalBodyNodes.push_back(bn);
			}

			//
			// add dependency from this body atom to each head atom
			//
			for (std::vector<AtomNodePtr>::iterator currhead = currentHeadNodes.begin();
				 currhead != currentHeadNodes.end();
				 ++currhead)
			{
				if ((*li)->isNAF())
					Dependency::addDep(*r, bn, *currhead, Dependency::NEG_PRECEDING);
				else
					Dependency::addDep(*r, bn, *currhead, Dependency::PRECEDING);
			}

			//
			// if this body-atom is an external atom, we have to take care of the
			// external dependencies - between its arguments (but only those of type
			// PREDICATE) and any other atom in the program that matches this argument.
			//
			// What we will do here is to build a multimap, which stores each input
			// predicate symbol together with the AtomNode of this external atom.
			// If we are through all rules, we will go through the complete set
			// of AtomNodes and search for matches with this multimap.
			//
			if (typeid(*((*li)->getAtom())) == typeid(ExternalAtom))
			{
				ExternalAtom* ext = dynamic_cast<ExternalAtom*>((*li)->getAtom().get());

				boost::shared_ptr<PluginAtom> pluginAtom = container.getAtom(ext->getFunctionName());

				const std::vector<PluginAtom::InputType>& inputTypes = pluginAtom->getInputTypes();
				const Tuple& input = ext->getInputTerms();
				Tuple::const_iterator iit = input.begin();


				//
				// go through all input terms of this external atom
				//
				for (std::vector<PluginAtom::InputType>::const_iterator it = inputTypes.begin();
				     it != inputTypes.end();
				     ++it, ++iit)
				{
					//
					// consider only PREDICATE input terms (naturally, for constant
					// input terms we won't have any dependencies!)
					//
					if (*it == PluginAtom::PREDICATE)
					{
						//
						// store the AtomNode of this external atom together will
						// all the predicate input terms
						//
						// e.g., if we had an external atom '&ext[a,b](X)', where
						// 'a' is of type PREDICATE, and the atom was stored in Node n1,
						// then the map will get an entry <'a', n1>. Below, we will
						// then search for those AtomNodes with a Predicate 'a' - those
						// will be assigned a dependency relation with n1!
						//
						extinputs.insert(std::pair<Term, AtomNodePtr>(*iit, bn));
					}
				}
			}
		} // body finished

		//
		// now we go through the ordinary and external atoms of the body again
		// and see if we have to add any EXTERNAL_AUX dependencies.
		// An EXTERNAL_AUX dependency arises, if an external atom has variable
		// input arguments, which makes it necessary to create an auxiliary
		// rule.
		//
		for (std::vector<AtomNodePtr>::iterator currextbody = currentExternalBodyNodes.begin();
			 currextbody != currentExternalBodyNodes.end();
			 ++currextbody)
		{
			ExternalAtom* ext = dynamic_cast<ExternalAtom*>((*currextbody)->getAtom().get());

			//
			// does this external atom have any variable input parameters?
			//
			if (!ext->pureGroundInput())
			{
				//
				// ok, this extatom needs an auxiliary rule.
				// get the parameters, i.e., the input terms
				//
				Tuple extinput = ext->getInputTerms();

				//
				// make a new atom with the ext-parameters as arguments, this will be
				// the head of the auxiliary rule; add this atom to the
				// global atom store
				//
				AtomPtr auxheadatom = Registry::Instance()->storeAtom(new Atom(ext->getAuxPredicate(), extinput));

				//
				// add a new head node for the graph with this atom
				//
				AtomNodePtr auxheadnode = nodegraph.addUniqueHeadNode(auxheadatom);

				//
				// now we create the auxiliary rule
				//
				RuleHead_t auxhead;
				RuleBody_t auxbody;

				auxhead.insert(auxheadatom);

				//
				// the body is still empty here, but we can use the ausrule
				// pointer to populate it later, see below
				//
				Rule* auxrule = new Rule(auxhead, auxbody);

				Registry::Instance()->storeObject(auxrule);

				//
				// add aux dependency from this new head to the external atom
				// node
				//
				Dependency::addDep(0, auxheadnode, *currextbody, Dependency::EXTERNAL_AUX);

				//
				// the body of the auxiliary rule are all body literals
				// (ordinary or external) that have variables with the aux_head
				// in common and that are not weakly negated!
				//
				std::vector<AtomNodePtr> allbodynodes;

				for (std::vector<AtomNodePtr>::iterator currbody = currentBodyNodes.begin();
					 currbody != currentBodyNodes.end();
					 ++currbody)
				{
					//
					// don't consider the external atom itself, the input must
					// be bound by other atoms
					//
					if (*currextbody == *currbody)
						continue;

					bool thisAtomIsRelevant = false;

					Tuple currentAtomArguments = (*currbody)->getAtom()->getArguments();

					//
					// go through all variables of the external atom
					//
					Tuple::const_iterator inb = extinput.begin(), ine = extinput.end();

					while (inb != ine)
					{
						//
						// now see if any of the current ordinary body atom
						// arguments has a common variable with the external
						// atom
						//
						Tuple::const_iterator bodb = currentAtomArguments.begin();
						Tuple::const_iterator bode = currentAtomArguments.end();

						while (bodb != bode)
						{
							if (*(bodb++) == *inb)
							{
								thisAtomIsRelevant = true;
							}
						}

						inb++;
					}

					//
					// should this atom be in the auxiliary rule body?
					// (i.e., does one of its arguments occur in the input list
					// of the external atom?)
					//
					if (thisAtomIsRelevant)
					{
						//
						// make new literals with the (ordinary) body atoms of the current rule
						//
						Literal* l = new Literal((*currbody)->getAtom());

						Registry::Instance()->storeObject(l);

						// add the literal to the auxiliary rule body
						auxrule->addBody(l);
					
						//std::cout << std::endl << "adding: " << *(l->getAtom()) << std::endl;
						//
						// make a node for each of these new atoms
						//
						AtomNodePtr auxbodynode = nodegraph.addUniqueBodyNode(l->getAtom());
						
						//
						// add the usual body->head dependency
						//
						Dependency::addDep(auxrule, auxbodynode, auxheadnode, Dependency::PRECEDING);
					}
				} // ordinary body-lit loop

			} // if (!ext->pureGroundInput())
		}
	}

	//
	// Now we will build the EXTERNAL dependencies:
	//
	typedef std::multimap<Term, AtomNodePtr>::iterator mi;

	//
	// Go through all AtomNodes
	//
	for (std::vector<AtomNodePtr>::const_iterator node = nodegraph.getNodes().begin();
		 node != nodegraph.getNodes().end();
		 ++node)
	{
		//
		// do this only for ordinary atoms, external atoms can't be in the input
		// list!
		//
		if (typeid(*(*node)->getAtom()) != typeid(ExternalAtom))
		{
			//
			// For this AtomNode: take the predicate term of its atom and extract all
			// entries in the multimap that match this predicate. Those entries contain
			// now the AtomNodes of the external atoms that have such an input predicate.
			//
			std::pair<mi, mi> range = extinputs.equal_range((*node)->getAtom()->getPredicate());

			//
			// add dependency: from this node to the external atom (second in the pair of the
			// multimap)
			//
			for (mi i = range.first; i != range.second; ++i)
			{
				Dependency::addDep(0, *node, i->second, Dependency::EXTERNAL);
			}
		}
	}

}


void
GraphBuilder::dumpGraph(const NodeGraph& nodegraph, std::ostream& out) const
{
	out << "Dependency graph - Program Nodes:" << std::endl;

	for (std::vector<AtomNodePtr>::const_iterator node = nodegraph.getNodes().begin();
		 node != nodegraph.getNodes().end();
		 ++node)
	{
		out << **node << std::endl;
	}

	out << std::endl;
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
