// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include <iomanip>
#include <sstream>

namespace jjm
{

template <typename IntegerT>
std::string toHexStr(IntegerT const& x)
{
    std::stringstream ss;
    ss << std::hex << x;
    return ss.str(); 
}

} //namespace
