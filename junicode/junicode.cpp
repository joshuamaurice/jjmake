// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jutfiterator.hpp"
#include "jutfstring.hpp"
#include "jutfutils.hpp"

template void jjm::writeUtf8<char*>(UnicodeCodePoint cp, char *&);
template jjm::CodePoint jjm::readUtf8Forward<char*>(char *&);
template jjm::CodePoint jjm::readUtf8Backward<char*>(char *&);

template void jjm::writeUtf16<jjm::Utf16EncodingUnit*>(UnicodeCodePoint cp, jjm::Utf16EncodingUnit *&);
template jjm::CodePoint jjm::readUtf8Forward<jjm::Utf16EncodingUnit*>(jjm::Utf16EncodingUnit *&);
template jjm::CodePoint jjm::readUtf8Backward<jjm::Utf16EncodingUnit*>(jjm::Utf16EncodingUnit *&);

template class jjm::Utf8InputIterator<char*>;
template class jjm::Utf16InputIterator<jjm::Utf16EncodingUnit*>;
template class jjm::Utf8BidiIterator<char*>;
template class jjm::Utf16BidiIterator<jjm::Utf16EncodingUnit*>;

template class jjm::BasicUtf8String<std::allocator<char> >;
template class jjm::BasicUtf16String<std::allocator<jjm::Utf16EncodingUnit> >;
