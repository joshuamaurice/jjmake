// Copyright (c) 2010-2011, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstdouterr.hpp"

#include "jbase/jnulltermiter.hpp"
#include "junicode/jutfstring.hpp"
#include <iostream>

using namespace jjm;
using namespace std;


int jjm::writeToStdOut(char const* utf8, size_t bytes)
{
#ifdef _WIN32
    auto byteRange = make_pair(utf8, utf8 + bytes); 
    auto cpRange = makeUtf8ToCpRange(byteRange.first, byteRange.second); 
    U16Str r3 = U16Str::cp(cpRange); 
    std::wcout.write(r3.data(), r3.sizeEU()); 
    return (! std::wcout) ? -1 : 0; 
#else
    JFATAL(0, 0); //TODO
#endif
}

int jjm::writeToStdOut(char const* utf8)
{
#ifdef _WIN32
    auto byteRange = jjm::makeNullTermRange(utf8);
    auto cpRange = make_pair(
            jjm::makeUtf8ToCpInputIterator(byteRange.first,  byteRange.second), 
            jjm::makeUtf8ToCpInputIterator(byteRange.second, byteRange.second)
            ); 
    U16Str r4 = U16Str::cp(cpRange); 
    std::wcout.write(r4.data(), r4.sizeEU()); 
    return (! std::wcout) ? -1 : 0; 
#else
    JFATAL(0, 0); //TODO
#endif
}

int jjm::writeToStdOut(std::string const& utf8)
{
    return jjm::writeToStdOut(utf8.data(), utf8.size()); 
}

int jjm::flushStdOut()
{
#ifdef _WIN32
    std::wcout << flush; 
    return (! std::wcout) ? -1 : 0; 
#else
    JFATAL(0, 0); //TODO
#endif
}
