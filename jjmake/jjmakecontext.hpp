// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_JJMAKECONTEXT_HPP_HEADER_GUARD
#define JJMAKE_JJMAKECONTEXT_HPP_HEADER_GUARD

#include "node.hpp"
#include "jbase/juniqueptr.hpp"
#include "josutils/jthreading.hpp"

#include <map>
#include <string>
#include <vector>


namespace jjm
{

class ParserContext;

class JjmakeContext
{
public:
    enum ExecutionMode { ExecuteGoals, PrintGoals }; 
    enum DependencyMode { NoDependencies, AllDependencies, AllDependents }; 

    class Arguments
    {
    public:
        Arguments() : 
                executionMode(ExecuteGoals), 
                dependencyMode(AllDependencies), 
                alwaysMake(false), 
                keepGoing(false), 
                numThreads(1) 
                {}
        ExecutionMode executionMode; 
        DependencyMode dependencyMode; 
        std::vector<std::string> goals; 
        bool alwaysMake; 
        bool keepGoing; 
        int numThreads; 
        std::string rootEvalText; 
    }; 

public:
    JjmakeContext(Arguments const& args);
    ~JjmakeContext(); 
    void execute(); 
    
public:

    //newNode() always takes ownership, 
    //is safe to call newNode() concurrently on the same JjmakeContext object
    void newNode(jjm::Node* node); 


private:
    JjmakeContext(JjmakeContext const& ); //not defined, not copyable
    JjmakeContext& operator= (JjmakeContext const& ); //not defined, not copyable

    //internal functions

    void phase1(); 
    class InitialParseNode; 

    void initPathMaps(); 
    void createImplicitDependencies(); 

    void activateSpecifiedGoals();
    void enableDependenciesDependents();
    void setNumOutstandingPrereqs(); 

    void phase2();
    class ExecuteGoalRunnable; 

    void setFailFlag(); 

    //data members

    ExecutionMode executionMode; 
    DependencyMode dependencyMode; 
    std::vector<std::string> specifiedGoals; 
    int numThreads; 
    std::string rootEvalText; 

    ThreadPool threadPool; 
    UniquePtr<ParserContext*> rootParserContext; 
    jjm::Mutex nodesMutex; //protects this->nodes
    std::map<std::string, jjm::Node*> nodes; //ownership
    std::map<std::string, std::vector<jjm::Node*> > inputPathMap; 
    std::map<std::string, jjm::Node*> outputPathMap; 
    bool keepGoing; 
    bool failFlag; 
};

}//namespace jjm

#endif
