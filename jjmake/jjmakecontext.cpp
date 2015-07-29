// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "parsercontext.hpp"
#include "josutils/jstdstreams.hpp"
#include "josutils/jpath.hpp"

using namespace std;

jjm::JjmakeContext::JjmakeContext(Arguments const& arguments_)
    : 
    arguments(arguments_), 
    threadPool(arguments_.numThreads),
    failFlag(false)

{
    rootParserContext.reset(ParserContext::newRoot(this)); 
    if (arguments.numThreads < 1)
        JFATAL(0, 0); 
}

jjm::JjmakeContext::~JjmakeContext() 
{
    for (map<std::string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
        delete node->second; 
    nodes.clear(); 
}

class jjm::JjmakeContext::InitialParseNode : public jjm::Thread::Runnable
{
public: 
    jjm::JjmakeContext * jjmakeContext; 
    jjm::ParserContext * parserContext; 
    string rootEvalText; 
    virtual void run()
    {
        try
        {
            parserContext->eval(rootEvalText); 
        }
        catch (std::exception & e)
        {   
            jjmakeContext->setFailFlag(); 

            string message; 
            message += typeid(e).name() ;
            message += ": ";
            message += e.what(); 
            message += "\n"; 
            jjmakeContext->toStdErr(message); 
        }
    }
};

void jjm::JjmakeContext::execute()
{
    phase1(); 

    initPathMaps();
    createImplicitDependencies(); 

    activateSpecifiedGoals();
    enableDependenciesDependents(); 
    setNumOutstandingPrereqs();
    
    phase2(); 
    if (failFlag && arguments.keepGoing)
        throw std::runtime_error("Execution failed. See previous error messages."); 
}

void jjm::JjmakeContext::phase1()
{
    rootParserContext->setValue(".PWD", Path(".").getRealPath().getStringRep()); 
    UniquePtr<InitialParseNode*> initialNode(new InitialParseNode); 
    initialNode->jjmakeContext = this;
    initialNode->parserContext = this->rootParserContext.get();
    initialNode->rootEvalText = arguments.rootEvalText; 
    threadPool.addTask(initialNode.release()); 
    threadPool.waitUntilIdle(); 
}

void jjm::JjmakeContext::initPathMaps()
{
    for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
    {   
        for (vector<Path>::const_iterator inputPath = node->second->inputPaths.begin();
                    inputPath != node->second->inputPaths.end(); 
                    ++inputPath)
        {
            string inputPathString = inputPath->getStringRep(); 
            inputPathMap[inputPathString].push_back(node->second); 
        }
        
        for (vector<Path>::const_iterator outputPath = node->second->outputPaths.begin();
                    outputPath != node->second->outputPaths.end(); 
                    ++outputPath)
        {   string outputPathString = outputPath->getStringRep(); 
            Node * & node2 = outputPathMap[outputPathString]; 
            if (node2 != 0)
            {   throw std::runtime_error(string()
                        + "Found two nodes with the same output path:\n"
                        + "Node 1: " + node->second->goalName + "\n"
                        + "Node 2: " + node2->goalName + "\n"
                        + "Path: " + outputPathString + "\n"
                        ); 
            }
            node2 = node->second; 
        }
    }

    //check for conflicting output paths
    map<string, jjm::Node*>::const_iterator prev = outputPathMap.begin(); 
    map<string, jjm::Node*>::const_iterator curr = prev; 
    if (curr == outputPathMap.end())
        return; 
    ++curr; 
    for ( ; curr != outputPathMap.end(); ++prev, ++curr)
    {
        //with lexicographic sort, with conflicting paths, the shorter path will be first
        if (prev->first.size() <= curr->first.size() 
                && std::equal(prev->first.begin(), prev->first.end(), curr->first.begin()))
        {
            throw std::runtime_error(string()
                    + "Found two nodes with conflicting output paths:\n"
                    + "Node 1: " + prev->second->goalName + "\n"
                    + "Path: " + prev->first + "\n"
                    + "Node 2: " + curr->second->goalName + "\n"
                    + "Path: " + curr->first + "\n"
                    ); 
        }
    }
}

namespace
{
    int comp(string const& x, string const& y)
    {
        for (size_t i=0; ; ++i)
        {   if (x.size() == i && y.size() == i)
                return 0;
            if (x.size() == i)
                return -1;
            if (y.size() == i)
                return 1; 
            if (x[i] < y[i])
                return -1;
            if (x[i] > y[i])
                return 1;
        }
    }
}

void jjm::JjmakeContext::createImplicitDependencies()
{
    map<string, vector<Node*> >::iterator input = inputPathMap.begin(); 
    map<string, Node*>::iterator output = outputPathMap.begin(); 
    for (;;)
    {   if (input == inputPathMap.end() || output == outputPathMap.end())
            break;
        string const& input2 = input->first; 
        string const& output2 = output->first; 
        int d = comp(input2, output2); 
        if (d == 0)
        {   for (vector<Node*>::iterator inputNode = input->second.begin(); inputNode != input->second.end(); ++inputNode)
            {   (**inputNode).dependencies.insert(output->second);
                output->second->dependents.insert(*inputNode); 
            }
            ++input;
            ++output;
        }
        if (d < 0)
            ++input; 
        if (d > 0)
            ++output; 
    }
}

void jjm::JjmakeContext::setNumOutstandingPrereqs()
{
    if (arguments.dependencyMode == AllDependencies)
    {   for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
        {   if (node->second->activated)
                node->second->numOutstandingPrereqs = node->second->dependencies.size(); 
        }
    }
    if (arguments.dependencyMode == AllDependents)
    {   for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
        {   if (node->second->activated)
                node->second->numOutstandingPrereqs = node->second->dependents.size(); 
        }
    }
}

void jjm::JjmakeContext::activateSpecifiedGoals()
{
    if (arguments.allGoals)
    {   for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
            node->second->activated = true; 
        return; 
    }

    for (vector<string>::const_iterator 
                g = arguments.goals.begin(); 
                g != arguments.goals.end(); 
                ++g)
    {
        map<string, Node*>::const_iterator node = nodes.find(*g); 
        if (node != nodes.end())
        {   node->second->activated = true; 
            continue; 
        }

        Path const p1 = Path(*g).getAbsolutePath(); 

        if ( ! p1.isEmpty())
        {   node = nodes.find(p1.getStringRep());
            if (node != nodes.end())
            {   node->second->activated = true; 
                continue; 
            }
        }

        if ( ! p1.isEmpty())
        {   map<string, Node*>::iterator n = outputPathMap.find(p1.getStringRep());
            if (n != outputPathMap.end())
            {   node->second->activated = true;
                continue; 
            }
        }

        throw std::runtime_error("Cannot find matching node for goal \"" + *g + "\"."); 
    }
}

void jjm::JjmakeContext::enableDependenciesDependents()
{
    if (arguments.dependencyMode == NoDependencies)
        return; 

    vector<Node*> pending; 
    for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
    {   if (node->second->activated)
            pending.push_back(node->second);
    }
    if (arguments.dependencyMode == AllDependencies)
    {   for ( ; pending.size(); )
        {   Node * node = pending.back();
            pending.pop_back();
            for (set<Node*>::iterator d = node->dependencies.begin(); d != node->dependencies.end(); ++d)
            {   if ( ! (**d).activated)
                {   (**d).activated = true;
                    pending.push_back(*d); 
                }
            }
        }
    }
    if (arguments.dependencyMode == AllDependents)
    {   for ( ; pending.size(); )
        {   Node * node = pending.back();
            pending.pop_back();
            for (set<Node*>::iterator d = node->dependents.begin(); d != node->dependents.end(); ++d)
            {   if ( ! (**d).activated)
                {   (**d).activated = true;
                    pending.push_back(*d); 
                }
            }
        }
    }
}

class jjm::JjmakeContext::ExecuteGoalRunnable : public jjm::Thread::Runnable
{
public:
    ExecuteGoalRunnable() : context(0), node(0) {}
    jjm::JjmakeContext * context; 
    jjm::Node * node; 
    virtual void run()
    {
        try
        {   if (context->failFlag && ! context->arguments.keepGoing)
                return; 

            if (context->arguments.executionMode == JjmakeContext::ExecuteGoals)
            {   context->toStdOut("[jjmake] Executing goal: " + node->goalName + "\n"); 
                node->execute(); 
            }else if (context->arguments.executionMode == JjmakeContext::PrintGoals)
            {   context->toStdOut("[jjmake] Goal: " + node->goalName + "\n"); 
            }else
                JFATAL(context->arguments.executionMode, 0); 

            set<Node*> * downstream = 0;
            if (context->arguments.dependencyMode == JjmakeContext::AllDependencies)
                downstream = & node->dependents;
            if (context->arguments.dependencyMode == JjmakeContext::AllDependents)
                downstream = & node->dependencies;
            if (downstream)
            {   for (set<Node*>::iterator d = downstream->begin(); d != downstream->end(); ++d)
                {   if ( ! (**d).activated)
                        continue; 
                    ssize_t numOutstandingPrereqsOfD; 
                    {
                        Lock lock((**d).mutex); 
                        numOutstandingPrereqsOfD  = --(**d).numOutstandingPrereqs;
                    }
                    if (numOutstandingPrereqsOfD < 0)
                        JFATAL(0, 0);
                    if (numOutstandingPrereqsOfD == 0)
                    {   UniquePtr<ExecuteGoalRunnable*> newRunnable(new ExecuteGoalRunnable);
                        newRunnable.get()->context = context;
                        newRunnable.get()->node = *d;
                        context->threadPool.addTask(newRunnable.release()); 
                    }
                }
            }
        }catch (std::exception & e)
        {   
            context->setFailFlag(); 

            string message; 
            message += "Failure during execution of node \"" + node->goalName + "\". Cause:\n"; 
            message += typeid(e).name() ;
            message += ": ";
            message += e.what(); 
            message += "\n"; 
            context->toStdErr(message); 
        }
    }
};

void jjm::JjmakeContext::phase2()
{
    //We collect the list of toExecute before creating and adding tasks to 
    //the threadpool to avoid a data race. 
    //The first loop accesses "numOutstandingPrereqs", which can be modified
    //when a task in the threadpool completes. 
    vector<Node*> toExecute; 
    for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
    {   if (node->second->activated && node->second->numOutstandingPrereqs == 0)
            toExecute.push_back(node->second); 
    }
    for (vector<Node*>::iterator node = toExecute.begin(); node != toExecute.end(); ++node)
    {   UniquePtr<ExecuteGoalRunnable*> newRunnable(new ExecuteGoalRunnable);
        newRunnable.get()->context = this;
        newRunnable.get()->node = *node;
        threadPool.addTask(newRunnable.release()); 
    }
    threadPool.waitUntilIdle(); 
}

void jjm::JjmakeContext::setFailFlag()
{
    failFlag = true; 
    if ( ! arguments.keepGoing)
        threadPool.setStopFlag(); 
}

void jjm::JjmakeContext::newNode(jjm::Node * node_)
{
    UniquePtr<Node*> node(node_); 
    if (node_->goalName.size() == 0)
        throw std::runtime_error("Adding node with empty name."); 

    {
        Lock lock(nodesMutex); 
        Node * & node2 = nodes[node_->goalName]; 
        if (node2 != 0)
            throw std::runtime_error("New node has same name as another node."); 
        node2 = node.release(); 
    }
}
