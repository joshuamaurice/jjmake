// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_JJMAKECONTEXT_HPP_HEADER_GUARD
#define JJMAKE_JJMAKECONTEXT_HPP_HEADER_GUARD

#include "parsercontext.hpp"
#include "task.hpp"
#include "taskdag.hpp"

#include "jbase/juniqueptr.hpp"
#include "josutils/jthreading.hpp"

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
            std::vector<std::string> const& goals,
            int numThreads
            ); 
    ~JjmakeContext(); 
    void execute(std::string const& rootEvalText); 

private:
    JjmakeContext(JjmakeContext const& ); //not defined, not copyable
    JjmakeContext& operator= (JjmakeContext const& ); //not defined, not copyable

    ExecutionMode executionMode; 
    DependencyMode dependencyMode; 
    std::vector<std::string> goals; 
    int numThreads; 

    ThreadPool threadPool; 
    UniquePtr<ParserContext*> rootParserContext; 
    TaskDag taskDag; 
};

}//namespace jjm

#endif
