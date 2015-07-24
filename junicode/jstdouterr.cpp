// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstdouterr.hpp"

#include "jbase/jnulltermiter.hpp"
#include "junicode/jutfstring.hpp"
#include <iostream>

using namespace jjm;
using namespace std;


int jjm::writeToStdOut(Utf8String const& str)
{
#ifdef _WIN32
    Utf16String p = jjm::makeU16Str(str); 
    std::wcout.write(p.data(), p.size()); 
    return ( ! std::wcout) ? -1 : 0; 
#else
    std::cout.write(str.c_str(), str.size()); //TODO localization
    return ( ! std::cout) ? -1 : 0; 
#endif
}

int jjm::flushStdOut()
{
#ifdef _WIN32
    std::wcout << flush; 
    return (! std::wcout) ? -1 : 0; 
#else
    std::cout << flush; //TODO
    return ( ! std::cout) ? -1 : 0; 
#endif
}
