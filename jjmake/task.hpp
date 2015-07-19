// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_TASKDAG_HPP_HEADER_GUARD
#define JJMAKE_TASKDAG_HPP_HEADER_GUARD

#include <string>

namespace jjm
{

class Plugin;

class Task
{
public:
    Task(); 
    ~Task(); 
    std::string name; 
    Plugin* pluginInstance; 
private:
    Task(Task const& ); //not defined, not copyable
    Task& operator= (Task const& ); //not defined, not copyable
};

} //namespace jjm

#endif
