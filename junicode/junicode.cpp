// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstringliteral.hpp"
#include "jutfiterator.hpp"
#include "junicodebase.hpp"
#include "jutfstring.hpp"


template jjm::Utf8String  jjm::makeU8StrFromCpRange <jjm::UnicodeCodePoint*>(std::pair<jjm::UnicodeCodePoint*, jjm::UnicodeCodePoint*> const& ); 
template jjm::Utf16String jjm::makeU16StrFromCpRange<jjm::UnicodeCodePoint*>(std::pair<jjm::UnicodeCodePoint*, jjm::UnicodeCodePoint*> const& ); 
