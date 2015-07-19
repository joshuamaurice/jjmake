// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_PLUGIN_HPP_HEADER_GUARD
#define JJMAKE_PLUGIN_HPP_HEADER_GUARD

namespace jjm
{

class Plugin
{
public:
    virtual ~Plugin() {}
private:
    Plugin(Plugin const& ); //not defined, not copyable
    Plugin& operator= (Plugin const& ); //not defined, not copyable
};

} //namespace jjm

#endif
