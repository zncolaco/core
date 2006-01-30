/* -*- C++ -*- */

/**
 * @file GuessCheckModelGenerator.cpp
 * @author Roman Schindlauer
 * @date Fri Jan 27 23:18:22 CET 2006
 *
 * @brief Strategy class for computing the model of a subprogram by a guess&check
 * computation.
 *
 *
 */

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"
#include "dlvhex/Interpretation.h"



GuessCheckModelGenerator::GuessCheckModelGenerator()
{
   // serializedProgram.clear();
}


/*
void
FixpointModelGenerator::initialize(const Program& p)
{
    serializeProgram(p);
}
*/


/*
void
GuessCheckModelGenerator::serializeProgram(const Program& p)
{
    //
    // make a textual representation of the components' rules
    // (with external atoms replaced)
    //

    //
    // the rules will be in higher-order-syntax, if dlvhex was called in ho-mode
    //

    ///todo: remove modelgenerator instantiation from here. should be somewhere outside!
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    dlvprogram.buildProgram(p);

    serializedProgram = dlvprogram.getString();
}


const std::string&
FixpointModelGenerator::getSerializedProgram() const
{
    return serializedProgram;
}
*/

void
GuessCheckModelGenerator::compute(const Program& program,
                                  const GAtomSet &I,
                                  std::vector<GAtomSet> &models)
{
    models.clear();

    ASPsolver Solver;
    
    std::string EDBprogram, fixpointProgram;

    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    std::vector<ExternalAtom> extatoms(program.getExternalAtoms());

    std::cout << "*** guess and check not implemented yet! ***" << std::endl;

//    models.push_back(currentI.getAtomSet());
}
