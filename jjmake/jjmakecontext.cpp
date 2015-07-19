// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

using namespace std;

jjm::JjmakeContext::JjmakeContext(
            ExecutionMode executionMode_, 
            DependencyMode dependencyMode_, 
            vector<string> const& goals_
            )
    : rootParserContext(0)
{
    executionMode = executionMode_;
    dependencyMode = dependencyMode_;
    goals = goals_; 
    rootParserContext = ParserContext::newRoot(); 
}

jjm::JjmakeContext::~JjmakeContext()
{
    delete rootParserContext; 
}

void jjm::JjmakeContext::execute(string const& firstFileContents)
{
    rootParserContext->eval(firstFileContents); 
}
