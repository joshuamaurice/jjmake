// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "node.hpp"

#include "jbase/jfatal.hpp"

#include <stdlib.h>

using namespace jjm;
using namespace std;

jjm::Node::Node(
            std::string const& goalName_, 
            std::vector<jjm::Path> const& inputPaths_, 
            std::vector<jjm::Path> const& outputPaths_
            )
    : 
    goalName(goalName_),
    inputPaths(inputPaths_), 
    outputPaths(outputPaths_), 
    numOutstandingPrereqs(0),
    activated(false)
{
    for (size_t i = 0; i < inputPaths.size(); ++i)
    {   if ( ! inputPaths[i].isAbsolute())
            JFATAL(0, 0); 
    }
    for (size_t i=0; i < outputPaths.size(); ++i)
    {   if ( ! outputPaths[i].isAbsolute())
            JFATAL(0, 0); 
    }
}
