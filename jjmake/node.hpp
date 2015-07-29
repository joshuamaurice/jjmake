// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_NODE_HPP_HEADER_GUARD
#define JJMAKE_NODE_HPP_HEADER_GUARD

#include "josutils/jthreading.hpp"
#include "jbase/jstdint.hpp"
#include "josutils/jpath.hpp"
#include <string>
#include <set>
#include <vector>

namespace jjm
{

class JjmakeContext; 
class ParserContext; 

class Node
{
public:
    virtual ~Node() {}

    //will be invoked by JjmakeContext
    virtual void execute() = 0; 

protected: 
    Node(   std::string const& goalName_, 
            std::vector<jjm::Path> const& inputPaths_, 
            std::vector<jjm::Path> const& outputPaths_
            ); 

private:
    Node(Node const& ); //not defined, not copyable
    Node& operator= (Node const& ); //not defined, not copyable

private:
    friend class jjm::JjmakeContext; 
    friend class jjm::ParserContext; 
    std::string goalName; 
    std::vector<jjm::Path> inputPaths; //guaranteed to be in realpath format
    std::vector<jjm::Path> outputPaths; //guaranteed to be in realpath format
    std::set<Node*> dependencies; 
    std::set<Node*> dependents; 
    ssize_t numOutstandingPrereqs; 
    bool activated; 
    Mutex mutex; 
};

} //namespace jjm

#endif
