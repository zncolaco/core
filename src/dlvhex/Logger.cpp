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
 * @file   Logger.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of logging facility.
 */

#include "dlvhex/Logger.hpp"

namespace
{
  Logger* instance = 0;
}

Logger& Logger::Instance()
{
  if( instance == 0 )
    instance = new Logger();
  return *instance;
}

void Logger::setPrintLevels(Levels levels)
{
  if( levels & ERROR != ERROR )
    out << "Logger warning: deactivated ERROR level" << std::endl;
  printlevels = levels;
}

void Logger::setPrintLevelWidth(int width)
{
  assert(width >= 0 );
  levelwidth = width;
}

