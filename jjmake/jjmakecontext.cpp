// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "junicode/jstdouterr.hpp"

using namespace std;

jjm::JjmakeContext::JjmakeContext(
            ExecutionMode executionMode_, 
            DependencyMode dependencyMode_, 
            vector<string> const& goals_,
            int numThreads_
            )
    : 
    executionMode(executionMode_), 
    dependencyMode(dependencyMode_), 
    goals(goals_),
    numThreads(numThreads_),
    rootParserContext(ParserContext::newRoot()),
    threadPool(numThreads_)
{
}

jjm::JjmakeContext::~JjmakeContext() 
{
}

void jjm::JjmakeContext::execute(string const& rootEvalText)
{
    class InitialTask : public Thread::Runnable
    {
    public: 
        JjmakeContext * jjmakeContext; 
        ParserContext * parserContext; 
        string rootEvalText; 
        virtual void run()
        {
            try
            {
                parserContext->eval(rootEvalText); 
            }
            catch (std::exception & e)
            {   string message; 
                message += typeid(e).name() ;
                message += ": ";
                message += e.what(); 
                message += "\n"; 
                jjm::writeToStdOut(message); 
                jjm::flushStdOut(); 
            }
        }
    };
    UniquePtr<InitialTask*> initialTask(new InitialTask); 
    initialTask->jjmakeContext = this;
    initialTask->parserContext = this->rootParserContext.get();
    initialTask->rootEvalText = rootEvalText; 

    //start phase 1
    threadPool.addTask(initialTask.release()); 
    threadPool.waitUntilIdle(); 
    //phase 1 complete
}
