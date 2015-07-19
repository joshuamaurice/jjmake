// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include <iomanip>
#include <sstream>

namespace jjm
{

template <typename IntegerT>
std::string toDecStr(IntegerT const& x)
{
    std::stringstream ss;
    ss << std::dec << x;
    return ss.str(); 
}

template <typename IntegerT>
std::string toHexStr(IntegerT const& x)
{
    std::stringstream ss;
    ss << std::hex << x;
    return ss.str(); 
}

//returns true on success 
template <typename IntegerT>
bool decStrToInteger(IntegerT & result, std::string const& str)
{
    std::stringstream ss;
    ss << str; 
    ss >> std::dec >> result; 
    if (!ss)
        return false;
    ss.get(); 
    if (ss)
        return false;
    return true;
}

template <typename IntegerT>
bool decHexToInteger(IntegerT & result, std::string const& str)
{
    IntegerT result; 
    std::stringstream ss;
    ss << str; 
    ss >> std::hex >> result; 
    if (!ss)
        return false;
    ss.get(); 
    if (ss)
        return false;
    return true;
}

} //namespace
