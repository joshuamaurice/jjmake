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

    //The logic of determining up-to-date should be delayed until this function 
    //is called. 
    //If the node is already up to date, then this function should merely exit
    //successfully. 
    virtual void execute() = 0; 

protected: 

    //Paths given to this constructor should be absolute 
    //and include the drive prefix for windows paths. 
    //Otherwise implicit dependency detection will fail
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

    //the following fields should remain fixed once phase2 goal execution has begun: 
    std::string goalName; 
    std::vector<jjm::Path> inputPaths; 
    std::vector<jjm::Path> outputPaths; 
    std::set<Node*> dependencies; 
    std::set<Node*> dependents; 
    bool activated; 

    Mutex mutex; 

    //The following fields may be modified during phase2 goal execution. 
    //Once phase2 goal execution has begun, access to this field should be
    //protected by this->mutex. 
    ssize_t numOutstandingPrereqs; 
};

} //namespace jjm

#endif
