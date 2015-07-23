// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "parsercontext.hpp"
#include "junicode/jstdouterr.hpp"
#include "josutils/jpath.hpp"

using namespace std;

jjm::JjmakeContext::JjmakeContext(Arguments const& args)
    : 
    executionMode(args.executionMode), 
    dependencyMode(args.dependencyMode), 
    specifiedGoals(args.goals),
    numThreads(args.numThreads),
    rootEvalText(args.rootEvalText),
    threadPool(args.numThreads),
    keepGoing(args.keepGoing), 
    failFlag(false)
{
    rootParserContext.reset(ParserContext::newRoot(this)); 
    if (numThreads < 1)
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
            jjm::writeToStdOut(message); 
            jjm::flushStdOut(); 
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
    if (failFlag && keepGoing)
        throw std::runtime_error("Execution failed. See previous error messages."); 
}

void jjm::JjmakeContext::phase1()
{
    rootParserContext->setValue(".PWD", Path(".").getRealPath().toStdString()); 
    UniquePtr<InitialParseNode*> initialNode(new InitialParseNode); 
    initialNode->jjmakeContext = this;
    initialNode->parserContext = this->rootParserContext.get();
    initialNode->rootEvalText = rootEvalText; 
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
            string inputPathString = inputPath->toStdString(); 
            inputPathMap[inputPathString].push_back(node->second); 
        }
        
        for (vector<Path>::const_iterator outputPath = node->second->outputPaths.begin();
                    outputPath != node->second->outputPaths.end(); 
                    ++outputPath)
        {   string outputPathString = outputPath->toStdString(); 
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
    if (dependencyMode == AllDependencies)
    {   for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
        {   if (node->second->active)
                node->second->numOutstandingPrereqs = node->second->dependencies.size(); 
        }
    }
    if (dependencyMode == AllDependents)
    {   for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
        {   if (node->second->active)
                node->second->numOutstandingPrereqs = node->second->dependents.size(); 
        }
    }
}

void jjm::JjmakeContext::activateSpecifiedGoals()
{
    for (vector<string>::const_iterator g = specifiedGoals.begin(); g != specifiedGoals.end(); ++g)
    {
        map<string, Node*>::const_iterator node = nodes.find(*g); 
        if (node != nodes.end())
        {   node->second->active = true; 
            continue; 
        }

        Path const p1 = Path(*g).getAbsolutePath(); 

        if ( ! p1.isEmpty())
        {   node = nodes.find(p1.toStdString());
            if (node != nodes.end())
            {   node->second->active = true; 
                continue; 
            }
        }

        if ( ! p1.isEmpty())
        {   map<string, Node*>::iterator n = outputPathMap.find(p1.toStdString());
            if (n != outputPathMap.end())
            {   node->second->active = true;
                continue; 
            }
        }

        throw std::runtime_error("Cannot find matching node for goal \"" + *g + "\"."); 
    }
}

void jjm::JjmakeContext::enableDependenciesDependents()
{
    if (dependencyMode == NoDependencies)
        return; 
    vector<Node*> pending; 
    for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
    {   if (node->second->active)
            pending.push_back(node->second);
    }
    if (dependencyMode == AllDependencies)
    {   for ( ; pending.size(); )
        {   Node * node = pending.back();
            pending.pop_back();
            for (set<Node*>::iterator d = node->dependencies.begin(); d != node->dependencies.end(); ++d)
            {   if ( ! (**d).active)
                {   (**d).active = true;
                    pending.push_back(*d); 
                }
            }
        }
    }
    if (dependencyMode == AllDependents)
    {   for ( ; pending.size(); )
        {   Node * node = pending.back();
            pending.pop_back();
            for (set<Node*>::iterator d = node->dependents.begin(); d != node->dependents.end(); ++d)
            {   if ( ! (**d).active)
                {   (**d).active = true;
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
        {   if (context->failFlag && ! context->keepGoing)
                return; 

            if (context->executionMode == JjmakeContext::ExecuteGoals)
            {   jjm::writeToStdOut("[jjmake] Executing goal: " + node->goalName + "\n");
                jjm::flushStdOut(); 
                node->execute(); 
            }else if (context->executionMode == JjmakeContext::PrintGoals)
            {   jjm::writeToStdOut("[jjmake] Goal: " + node->goalName + "\n");
            }else
                JFATAL(context->executionMode, 0); 

            set<Node*> * downstream = 0;
            if (context->dependencyMode == JjmakeContext::AllDependencies)
                downstream = & node->dependents;
            if (context->dependencyMode == JjmakeContext::AllDependents)
                downstream = & node->dependencies;
            if (downstream)
            {   for (set<Node*>::iterator d = downstream->begin(); d != downstream->end(); ++d)
                {   if ( ! (**d).active)
                        continue; 
                    --(**d).numOutstandingPrereqs;
                    if ((**d).numOutstandingPrereqs < 0)
                        JFATAL(0, 0);
                    if ((**d).numOutstandingPrereqs == 0)
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
            jjm::writeToStdOut(message); 
            jjm::flushStdOut(); 
        }
    }
};

void jjm::JjmakeContext::phase2()
{
    for (map<string, Node*>::iterator node = nodes.begin(); node != nodes.end(); ++node)
    {   if (node->second->active && node->second->numOutstandingPrereqs == 0)
        {   UniquePtr<ExecuteGoalRunnable*> newRunnable(new ExecuteGoalRunnable);
            newRunnable.get()->context = this;
            newRunnable.get()->node = node->second;
            threadPool.addTask(newRunnable.release()); 
        }
    }
    threadPool.waitUntilIdle(); 
}

void jjm::JjmakeContext::setFailFlag()
{
    failFlag = true; 
    if ( ! keepGoing)
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
