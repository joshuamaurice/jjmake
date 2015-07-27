// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JWINLOCALE_HPP_HEADER_GUARD
#define JUNICODE_JWINLOCALE_HPP_HEADER_GUARD

#include "jutfstring.hpp"
#include <vector>

namespace jjm
{

#if 0
class WinLocale
{
public:
    std::string localeShortName; 

    std::string languageShortName; 
    std::string languageLongName; 

    std::string countryShortName; 
    std::string countryLongName; 
};

//Creates this list once, then caches it. 
//The vector is created on-first call. 
//To avoid concurrency issues, it is also initialized from a namespace-scope constructor
std::vector<WinLocale> const &  getWinLocales(); 

//Returns null on no match. Caller does not own return. 
WinLocale const *  getWinLocale(std::string const& alias); 

#endif

}

#endif
