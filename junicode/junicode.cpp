// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstringliteral.hpp"
#include "jutfiterator.hpp"
#include "jutfstring.hpp"
#include "jutfutils.hpp"

template void jjm::writeUtf8<char*>(UnicodeCodePoint cp, char*& );
template jjm::CodePoint jjm::readUtf8Forward<char*>(char*& , char* );
template jjm::CodePoint jjm::readUtf8Backward<char*>(char* , char*& );

template void jjm::writeUtf16<jjm::Utf16EncodingUnit*>(UnicodeCodePoint cp, jjm::Utf16EncodingUnit *&);
template jjm::CodePoint jjm::readUtf8Forward<jjm::Utf16EncodingUnit*>(jjm::Utf16EncodingUnit *& , jjm::Utf16EncodingUnit * );
template jjm::CodePoint jjm::readUtf8Backward<jjm::Utf16EncodingUnit*>(jjm::Utf16EncodingUnit * , jjm::Utf16EncodingUnit *& );

template class jjm::Utf8ToCpInputIterator<char*>;
template class jjm::Utf8ToCpBidiIterator<char*>;

template class jjm::Utf16ToCpInputIterator<jjm::Utf16EncodingUnit*>;
template class jjm::Utf16ToCpBidiIterator<jjm::Utf16EncodingUnit*>;

template class jjm::BasicUtf8String<std::allocator<char> >;
template class jjm::BasicUtf16String<std::allocator<jjm::Utf16EncodingUnit> >;
