/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file   ASPSolver.tcc
 * @author Thomas Krennwallner
 * @date   Thu Feb 21 09:20:32 CET 2008
 * 
 * @brief  Definition of ASP solver class.
 * 
 */

#if !defined(_DLVHEX_ASPSOLVER_TCC)
#define _DLVHEX_ASPSOLVER_TCC


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/Error.h"
#include "dlvhex/Program.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/globals.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN


template<typename Builder, typename Parser>
ASPSolver<Builder,Parser>::ASPSolver(Process& p)
  : proc(p)
{ }


template<typename Builder, typename Parser>
void
ASPSolver<Builder,Parser>::solve(const Program& prg,
				 const AtomSet& facts,
				 std::vector<AtomSet>& as)
  throw (FatalError)
{
  int retcode;
  std::string error;
  
  try
    {
      DEBUG_START_TIMER;

      proc.spawn();

      // send program and facts
      try
        {
	  Builder builder(proc.getOutput());
	  const_cast<Program&>(prg).accept(builder);
	  const_cast<AtomSet&>(facts).accept(builder);
	}
      catch (std::ios_base::failure&)
        {
	  throw FatalError("Received an error while sending the program to the external LP solver.");
        }
      
      proc.endoffile(); // send EOF to process

      // parse result
      Parser parser;
      parser.parse(proc.getInput(), as, error);
      
      // get exit code of process
      retcode = proc.close();

      //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
      DEBUG_STOP_TIMER("Calling LP solver and parsing its result     ");
    }
  catch (FatalError&)
    {
      throw;
    }
  catch (GeneralError& e)
    {
      throw FatalError(e.getErrorMsg());
    }
  catch (std::exception& e)
    {
      throw FatalError(e.what());
    }

  // check for errors
  if (retcode == 127)
    {
      throw FatalError("LP solver command not found!");
    }
  else if (retcode != 0) // other problem
    {
      std::stringstream errstr;

      errstr << "LP solver failure (" << retcode << "):" << std::endl;
      errstr << error;

      throw FatalError(errstr.str());
    }
}

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_TCC

// Local Variables:
// mode: C++
// End: