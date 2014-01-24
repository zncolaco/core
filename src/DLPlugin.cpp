/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file DLPlugin.cpp
 * @author Daria Stepanova
 * @author Christoph Redl
 *
 * @brief Implements interface to DL-Lite using owlcpp.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/DLPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ExternalLearningHelper.h"
#include <iostream>
#include <string>
#include "boost/program_options.hpp"
#include "boost/range.hpp"
#include "boost/foreach.hpp"
#include "boost/filesystem.hpp"

#if defined(HAVE_OWLCPP)
#include "owlcpp/rdf/triple_store.hpp"
#include "owlcpp/io/input.hpp"
#include "owlcpp/io/catalog.hpp"
#include "owlcpp/terms/node_tags_owl.hpp"

#endif //HAVE_OWLCPP

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

// ============================== Class CachedOntology ==============================

DLPlugin::CachedOntology::CachedOntology() : loaded(false){
}

void DLPlugin::CachedOntology::operator=(CachedOntology& co){
	assert(false && "Copy constructor not supported");
}

void DLPlugin::CachedOntology::load(RegistryPtr reg, ID ontologyName){

	// load and prepare the ontology here
	owlcpp::Triple_store::result_b<0,0,0,0>::type r = store.find_triple(
			   	   owlcpp::any(),
				   owlcpp::any(),
				   owlcpp::any(),
		       owlcpp::any());

	try{
		load_file(reg->terms.getByID(ontologyName).getUnquotedString(), store);
	}catch(std::exception e){
		throw PluginError("Error while loading ontology " + reg->terms.getByID(ontologyName).getUnquotedString() + ": " + e.what());
	}

	loaded = true;
}

// ============================== Class DLPluginAtom ==============================

DLPlugin::DLPluginAtom::DLPluginAtom(std::string predName, ProgramCtx& ctx, std::vector<CachedOntology>& ontologies) : PluginAtom(predName, true), ctx(ctx), ontologies(ontologies), learnedSupportSets(false){
}

ID DLPlugin::DLPluginAtom::dlNeg(ID id){
	RegistryPtr reg = getRegistry();
	return reg->storeConstantTerm("\"-" + reg->terms.getByID(id).getUnquotedString() + "\"");
}

ID DLPlugin::DLPluginAtom::dlEx(ID id){
	RegistryPtr reg = getRegistry();
	return reg->storeConstantTerm("Ex" + reg->terms.getByID(id).getUnquotedString());
}

void DLPlugin::DLPluginAtom::constructClassificationProgram(){

	if (classificationIDB.size() > 0){
		DBGLOG(DBG, "Classification program was already constructed");
		return;
	}

	DBGLOG(DBG, "Constructing classification program");
	RegistryPtr reg = getRegistry();

	// prepare some terms and atoms
	subID = reg->storeConstantTerm("sub");
	opID = reg->storeConstantTerm("op");
	confID = reg->storeConstantTerm("conf");
	ID xID = reg->storeVariableTerm("X");
	ID yID = reg->storeVariableTerm("Y");
	ID x2ID = reg->storeVariableTerm("X2");
	ID y2ID = reg->storeVariableTerm("Y2");
	ID zID = reg->storeVariableTerm("Z");

	OrdinaryAtom subxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	subxy.tuple.push_back(subID);
	subxy.tuple.push_back(xID);
	subxy.tuple.push_back(yID);
	ID subxyID = reg->storeOrdinaryAtom(subxy);

	OrdinaryAtom subxz(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	subxz.tuple.push_back(subID);
	subxz.tuple.push_back(xID);
	subxz.tuple.push_back(zID);
	ID subxzID = reg->storeOrdinaryAtom(subxz);

	OrdinaryAtom subyz(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	subyz.tuple.push_back(subID);
	subyz.tuple.push_back(yID);
	subyz.tuple.push_back(zID);
	ID subyzID = reg->storeOrdinaryAtom(subyz);

	OrdinaryAtom opxx2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	opxx2.tuple.push_back(opID);
	opxx2.tuple.push_back(xID);
	opxx2.tuple.push_back(x2ID);
	ID opxx2ID = reg->storeOrdinaryAtom(opxx2);

	OrdinaryAtom opyy2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	opyy2.tuple.push_back(opID);
	opyy2.tuple.push_back(yID);
	opyy2.tuple.push_back(y2ID);
	ID opyy2ID = reg->storeOrdinaryAtom(opyy2);

	OrdinaryAtom suby2x2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	suby2x2.tuple.push_back(subID);
	suby2x2.tuple.push_back(y2ID);
	suby2x2.tuple.push_back(x2ID);
	ID suby2x2ID = reg->storeOrdinaryAtom(suby2x2);

	OrdinaryAtom confxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	confxy.tuple.push_back(confID);
	confxy.tuple.push_back(xID);
	confxy.tuple.push_back(yID);
	ID confxyID = reg->storeOrdinaryAtom(confxy);

	OrdinaryAtom opxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	opxy.tuple.push_back(opID);
	opxy.tuple.push_back(xID);
	opxy.tuple.push_back(yID);
	ID opxyID = reg->storeOrdinaryAtom(opxy);

	// Transitivity rule: sub(X,Z) :- sub(X,Y), sub(Y,Z)
	Rule trans(ID::MAINKIND_RULE);
	trans.body.push_back(subxyID);
	trans.body.push_back(subyzID);
	trans.head.push_back(subxzID);
	ID transID = reg->storeRule(trans);

	// Contraposition rule: sub(Y',X') :- op(X,X'), op(Y,Y'), sub(X,Y)
	Rule contra(ID::MAINKIND_RULE);
	contra.body.push_back(opxx2ID);
	contra.body.push_back(opyy2ID);
	contra.body.push_back(subxyID);
	trans.head.push_back(suby2x2ID);
	ID contraID = reg->storeRule(contra);

	// Conflict rule: conf(X,Y) :- op(X,Y), sub(X,Y)
	Rule conflict(ID::MAINKIND_RULE);
	conflict.body.push_back(opxyID);
	conflict.body.push_back(subxyID);
	conflict.body.push_back(confxyID);
	ID conflictID = reg->storeRule(conflict);

	// assemble program
	classificationIDB.push_back(transID);
	classificationIDB.push_back(contraID);
	classificationIDB.push_back(conflictID);
}

// computes the classification for a given ontology
InterpretationPtr DLPlugin::DLPluginAtom::computeClassification(ProgramCtx& ctx, CachedOntology& ontology){

	assert(!ontology->classification && "Classification for this ontology was already computed");
	constructClassificationProgram();

	DBGLOG(DBG, "Computing classification");
	RegistryPtr reg = getRegistry();

	// prepare data structures for the subprogram P
	ProgramCtx pc = ctx;
	pc.idb = classificationIDB;
	InterpretationPtr edb = InterpretationPtr(new Interpretation(reg));
	pc.edb = edb;
	pc.currentOptimum.clear();
	pc.config.setOption("NumberOfModels",0);

	// use the ontology to construct the EDB
	DBGLOG(DBG,"Ontology file was loaded");
	BOOST_FOREACH(owlcpp::Triple const& t, ontology.store.map_triple()) {
		DBGLOG(DBG, "Current triple: " << to_string(t.subj_, store) << " / " << to_string(t.pred_, ontology.store) << " / " << to_string(t.obj_, ontology.store));
		if (to_string(t.subj_, ontology.store) == "owl:Class" && to_string(t.pred_, ontology.store) == "rdf:type") {
			DBGLOG(DBG,"Construct facts of the form op(C,negC), sub(C,C) for this class.");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(opID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm(to_string(t.subj_, ontology.store))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);;
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}	
		if (to_string(t.obj_, ontology.store) == "owl:ObjectProperty" && to_string(t.pred_, ontology.store) == "rdf:type") {
			DBGLOG(DBG,"Construct facts of the form op(Subj,negSubj), sub(Subj,Subj), sub(exSubj,negexSubj), sub(exSubj,exSubj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(opID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm(to_string(t.subj_, ontology.store))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(dlEx(reg->storeConstantTerm(to_string(t.subj_, ontology.store))));
				fact.tuple.push_back(dlEx(dlEx(reg->storeConstantTerm(to_string(t.subj_, ontology.store)))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(dlEx(reg->storeConstantTerm(to_string(t.subj_, ontology.store))));
				fact.tuple.push_back(dlEx(reg->storeConstantTerm(to_string(t.subj_, ontology.store))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}

		if (to_string(t.pred_, ontology.store) == "owl:subClassOf")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.obj_, ontology.store)));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}

		if (to_string(t.pred_, ontology.store) == "owl:subPropertyOf")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.obj_, ontology.store)));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}

		if (to_string(t.pred_, ontology.store) == "owl:disjointWith")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,negObj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm(to_string(t.obj_, ontology.store))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}
		if (to_string(t.pred_, ontology.store) == "owl:propertyDisjointWith")
		{
			DBGLOG(DBG,to_string(t.subj_, ontology.store) << to_string(t.pred_, ontology.store)<< to_string(t.obj_, ontology.store));
			DBGLOG(DBG,"Construct facts of the form sub(Subj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm(to_string(t.obj_, ontology.store))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}
		if (to_string(t.pred_, ontology.store) == "rdfs:Domain")
		{
			DBGLOG(DBG,to_string(t.subj_, ontology.store) << to_string(t.pred_, ontology.store)<< to_string(t.obj_, ontology.store));
			DBGLOG(DBG,"Construct facts of the form sub(exSubj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(dlEx(reg->storeConstantTerm(to_string(t.subj_, ontology.store))));
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.obj_, ontology.store)));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}
	}
	DBGLOG(DBG, "EDB to classification program: " << *edb);

	// evaluate the subprogram and return its unique answer set
	std::vector<InterpretationPtr> answersets = ctx.evaluateSubprogram(pc, true);
	assert(answersets.size() == 1 && "Subprogram must have exactly one answer set");
	DBGLOG(DBG, "Classification: " << *answersets[0]);

	ontology.classification = answersets[0];
	assert(!!ontology->classification && "Could not compute classification");
}

DLPlugin::CachedOntology& DLPlugin::DLPluginAtom::prepareOntology(ProgramCtx& ctx, ID ontologyNameID){

	RegistryPtr reg = getRegistry();

	BOOST_FOREACH (CachedOntology o, ontologies){
		if (o.ontologyName == ontologyNameID){
			DBGLOG(DBG, "Accessing cached ontology " << reg->terms.getByID(ontologyNameID).getUnquotedString());
			return o;
		}
	}

	// ontology is not in the cache --> load it
	DBGLOG(DBG, "Loading ontology" << reg->terms.getByID(ontologyNameID).getUnquotedString());
	ontologies.push_back(CachedOntology());
	ontologies[ontologies.size() - 1].load(reg, ontologyNameID);
	computeClassification(ctx, ontologies[ontologies.size() - 1]);
	return ontologies[ontologies.size() - 1];
}

void DLPlugin::DLPluginAtom::guardSupportSet(bool& keep, Nogood& ng, const ID eaReplacement)
{
	assert(ng.isGround());

	RegistryPtr reg = getRegistry();

	// get the ontology name
	ID ontologyNameID = reg->ogatoms.getByID(eaReplacement).tuple[1];

	// find guard atom in the nogood
	BOOST_FOREACH (ID lit, ng){
		// since nogoods eliminate "unnecessary" property flags, we need to recover the original ID by retrieving it again
		ID litID = reg->ogatoms.getIDByAddress(lit.address);

		// check if it is a guard atom
		if (litID.isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(litID) == 'o'){
			const OrdinaryAtom& guardAtom = reg->ogatoms.getByID(litID);

			// recover the concept name and the individual
			ID conceptID = reg->getIDByAuxiliaryConstantSymbol(guardAtom.tuple[0]);	// let this be concept C
			ID individualID = guardAtom.tuple[1];	// let this be an individual a

#if defined(HAVE_OWLCPP)
			if ( true /* TODO: check here if C(a) holds */ ){
				// remove the guard atom
				Nogood restricted;
				BOOST_FOREACH (ID lit2, ng){
					if (lit2 != lit){
						restricted.insert(lit2);
					}
				}
				DBGLOG(DBG, "Keeping support set " << ng.getStringRepresentation(reg) << " with satisfied guard atom in form " << restricted.getStringRepresentation(reg));
				ng = restricted;
				keep = true;
			}else{
				DBGLOG(DBG, "Removing support set " << ng.getStringRepresentation(reg) << " because guard atom is unsatisfied");
				keep = false;
			}
#endif
		}
	}
	DBGLOG(DBG, "Keeping support set " << ng.getStringRepresentation(reg) << " without guard atom");
	keep = true;
}

void DLPlugin::DLPluginAtom::learnSupportSets(const Query& query, NogoodContainerPtr nogoods){

#if defined(HAVE_OWLCPP)
		DBGLOG(DBG, "Learning support sets");

		assert(!learnedSupportSets && "tried to learn support sets multiple times");

		// make sure that the ontology is in the cache and retrieve its classification
		InterpretationPtr classification = prepareOntology(ctx, query.input[0]).classification;

		RegistryPtr reg = getRegistry();

		// prepare output variable, tuple and negative output atom
		ID outvarID = reg->storeVariableTerm("O");
		Tuple outlist;
		outlist.push_back(outvarID);
		ID outlit = NogoodContainer::createLiteral(ExternalLearningHelper::getOutputAtom(query, outlist, true).address, false);

		// iterate over the maximum input
		bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
		bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

		ID qID = query.input[5];
		while (en < en_end){
			// check if it is c+, c-, r+ or r-

			const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);

			if (oatom.tuple[0] == query.input[1]){
				// c+
				assert(oatom.tuple.size() == 3 && "Second parameter must be a binary predicate");

				ID cID = oatom.tuple[1];

				// check if sub(C, Q) is true in the classification assignment
				OrdinaryAtom subcq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subcq.tuple.push_back(subID);
				subcq.tuple.push_back(cID);
				subcq.tuple.push_back(qID);
				ID subcqID = reg->storeOrdinaryAtom(subcq);

#ifdef DEBUG
				{
					std::stringstream ss;
					Printer printer(reg, ss);
					printer.print(subcqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subcqID.address)){
					OrdinaryAtom cpcx(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					cpcx.tuple.push_back(query.input[1]);
					cpcx.tuple.push_back(cID);
					cpcx.tuple.push_back(outvarID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcx)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}

				// check if conf(C, C) is true in the classification assignment
				OrdinaryAtom confcc(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subcq.tuple.push_back(confID);
				subcq.tuple.push_back(cID);
				subcq.tuple.push_back(cID);
				ID confccID = reg->storeOrdinaryAtom(confcc);
#ifdef DEBUG
				{
					std::stringstream ss;
					Printer printer(reg, ss);
					printer.print(confccID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(confccID.address)){
					OrdinaryAtom cpcx(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					cpcx.tuple.push_back(query.input[1]);
					cpcx.tuple.push_back(cID);
					cpcx.tuple.push_back(outvarID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcx)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}

				// check if sub(C, C') is true in the classification assignment (for some C')
				bm::bvector<>::enumerator en2 = classification->getStorage().first();
				bm::bvector<>::enumerator en2_end = classification->getStorage().end();
				while (en2 < en2_end){
					const OrdinaryAtom& cl = reg->ogatoms.getByAddress(*en2);
					if (cl.tuple[0] == cID){
						// add {cp(C, Y), negC'(Y)}
						Nogood supportset;

						OrdinaryAtom cpcy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
						cpcy.tuple.push_back(cID);
						cpcy.tuple.push_back(reg->storeVariableTerm("Y"));
						supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcy)));

						OrdinaryAtom negcp(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
						negcp.tuple.push_back(reg->getAuxiliaryConstantSymbol('o', dlNeg(cl.tuple[2])));
						negcp.tuple.push_back(reg->storeVariableTerm("Y"));
						supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(negcp)));

						supportset.insert(outlit);

						DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
						nogoods->addNogood(supportset);

						// check if cm(C', Y) occurs in the maximal interpretation
						bm::bvector<>::enumerator en3 = query.interpretation->getStorage().first();
						bm::bvector<>::enumerator en3_end = query.interpretation->getStorage().end();
						while (en3 < en3_end){
							const OrdinaryAtom& at = reg->ogatoms.getByAddress(*en3);
							if (at.tuple[0] == query.input[2]){
								Nogood supportset;

								// add { T cp(C,Y), T cm(C,Y) }
								OrdinaryAtom cpcy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
								cpcy.tuple.push_back(query.input[1]);
								cpcy.tuple.push_back(cID);
								cpcy.tuple.push_back(reg->storeVariableTerm("Y"));
								supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcy)));

								OrdinaryAtom cmcy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
								cpcy.tuple.push_back(query.input[2]);
								cpcy.tuple.push_back(cID);
								cpcy.tuple.push_back(reg->storeVariableTerm("Y"));
								supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcy)));

								supportset.insert(outlit);

								DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
								nogoods->addNogood(supportset);
							}
							en3++;
						}
					}
					en2++;
				}
			}else if (oatom.tuple[0] == query.input[2]){
				// c-
				assert(oatom.tuple.size() == 3 && "Third parameter must be a binary predicate");

				ID cID = oatom.tuple[1];

				// check if sub(negC, Q) is true in the classification assignment
				OrdinaryAtom subncq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subncq.tuple.push_back(subID);
				subncq.tuple.push_back(dlNeg(cID));
				subncq.tuple.push_back(qID);
				ID subncqID = reg->storeOrdinaryAtom(subncq);

#ifdef DEBUG
				{
					std::stringstream ss;
					Printer printer(reg, ss);
					printer.print(subncqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subncqID.address)){
					OrdinaryAtom cmcx(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					cmcx.tuple.push_back(query.input[2]);
					cmcx.tuple.push_back(cID);
					cmcx.tuple.push_back(outvarID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cmcx)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}
			}else if (oatom.tuple[0] == query.input[3]){
				// r+
				assert(oatom.tuple.size() == 4 && "Fourth parameter must be a ternary predicate");

				ID rID = oatom.tuple[1];

				// check if sub(negC, Q) is true in the classification assignment
				OrdinaryAtom subexrq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subexrq.tuple.push_back(subID);
				subexrq.tuple.push_back(dlEx(rID));
				subexrq.tuple.push_back(qID);
				ID subexrqID = reg->storeOrdinaryAtom(subexrq);

#ifdef DEBUG
				{
					std::stringstream ss;
					Printer printer(reg, ss);
					printer.print(subexrqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subexrqID.address)){
					OrdinaryAtom rprxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					rprxy.tuple.push_back(query.input[3]);
					rprxy.tuple.push_back(rID);
					rprxy.tuple.push_back(outvarID);
					rprxy.tuple.push_back(reg->storeVariableTerm("Y"));
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(rprxy)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}
			}else if (oatom.tuple[0] == query.input[4]){
				// r-
				assert(oatom.tuple.size() == 4 && "Fifth parameter must be a ternary predicate");

				ID rID = oatom.tuple[1];

				// check if sub(negC, Q) is true in the classification assignment
				OrdinaryAtom subnexrq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subnexrq.tuple.push_back(subID);
				subnexrq.tuple.push_back(dlNeg(dlEx(rID)));
				subnexrq.tuple.push_back(qID);
				ID subnexrqID = reg->storeOrdinaryAtom(subnexrq);

#ifdef DEBUG
				{
					std::stringstream ss;
					Printer printer(reg, ss);
					printer.print(subexrqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subnexrqID.address)){
					OrdinaryAtom rprxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					rprxy.tuple.push_back(query.input[4]);
					rprxy.tuple.push_back(rID);
					rprxy.tuple.push_back(outvarID);
					rprxy.tuple.push_back(reg->storeVariableTerm("Y"));
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(rprxy)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}
			}

			en++;
		}

		learnedSupportSets = true;
#else
		assert("No support for owlcpp compiled into this binary");
		throw PluginError("Error: No support for owlcpp compiled into this binary");
#endif
}

void DLPlugin::DLPluginAtom::retrieve(const Query& query, Answer& answer)
{
	assert(false && "this method should never be called since the learning-based method is present");
}

void DLPlugin::DLPluginAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods){

	// check if we want to learn support sets (but do this only once)
	if (!learnedSupportSets && !!nogoods && query.ctx->config.getOption("SupportSets")){
		learnSupportSets(query, nogoods);
	}
}

// ============================== Class CDLAtom ==============================

DLPlugin::CDLAtom::CDLAtom(ProgramCtx& ctx, std::vector<CachedOntology>& ontologies) : DLPluginAtom("cDL", ctx, ontologies)
{
	DBGLOG(DBG,"Constructor of cDL plugin is started");
	addInputConstant(); // the ontology
	addInputPredicate(); // the positive concept
	addInputPredicate(); // the negative concept
	addInputPredicate(); // the positive role
	addInputPredicate(); // the negative role
	addInputConstant(); // the query
	setOutputArity(1); // arity of the output list
}

void DLPlugin::CDLAtom::retrieve(const Query& query, Answer& answer)
{
	assert(false);
}

void DLPlugin::CDLAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
{
	// learn support sets (if enabled)
	DLPluginAtom::retrieve(query, answer, nogoods);

	// answer the query (TODO)
}

// ============================== Class RDLAtom ==============================

DLPlugin::RDLAtom::RDLAtom(ProgramCtx& ctx, std::vector<CachedOntology>& ontologies) : DLPluginAtom("rDL", ctx, ontologies)
{
	DBGLOG(DBG,"Constructor of cDL plugin is started");
	addInputConstant(); // the ontology
	addInputPredicate(); // the positive concept
	addInputPredicate(); // the negative concept
	addInputPredicate(); // the positive role
	addInputPredicate(); // the negative role
	addInputConstant(); // the query
	setOutputArity(2); // arity of the output list
}

void DLPlugin::RDLAtom::retrieve(const Query& query, Answer& answer)
{
	assert(false);
}

void DLPlugin::RDLAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
{
	// learn support sets (if enabled)
	DLPluginAtom::retrieve(query, answer, nogoods);

	// answer the query (TODO)
}

// ============================== Class DLPlugin ==============================

// Collect all types of external atoms 
DLPlugin::DLPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-DLplugin[internal]", 2, 0, 0);
}

DLPlugin::~DLPlugin()
{
}

// Define two external atoms: for the roles and for the concept queries
std::vector<DLPlugin::CachedOntology> ontologies;
std::vector<PluginAtomPtr> DLPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;
	ret.push_back(PluginAtomPtr(new CDLAtom(ctx, ontologies)));
	ret.push_back(PluginAtomPtr(new RDLAtom(ctx, ontologies)));
	return ret;
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
DLPlugin theDLPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theDLPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
