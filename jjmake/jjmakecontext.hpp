// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_JJMAKECONTEXT_HPP_HEADER_GUARD
#define JJMAKE_JJMAKECONTEXT_HPP_HEADER_GUARD

#include "parsercontext.hpp"
#include "task.hpp"
#include "taskdag.hpp"
#include <string>
#include <vector>


namespace jjm
{

class JjmakeContext
{
public:
    enum ExecutionMode { ExecuteGoals, PrintGoals }; 
    enum DependencyMode { NoDependencies, AllDependencies, AllDependants }; 

public:
    JjmakeContext(
            ExecutionMode executionMode, 
            DependencyMode dependencyMode, 
            std::vector<std::string> const& goals
            ); 
    ~JjmakeContext(); 
    void execute(std::string const& firstFileContents); 

private:
    JjmakeContext(JjmakeContext const& ); //not defined, not copyable
    JjmakeContext& operator= (JjmakeContext const& ); //not defined, not copyable

    ExecutionMode executionMode; 
    DependencyMode dependencyMode; 
    std::vector<std::string> goals; 

    ParserContext * rootParserContext; 
    TaskDag taskDag; 
};

}//namespace jjm

#endif
