// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUSTRING_HPP_HEADER_GUARD
#define JUNICODE_JUSTRING_HPP_HEADER_GUARD

#include "jutfiterator.hpp"
#include "jbase/jnulltermiter.hpp"
#include "jbase/jtemplatemetaprogrammingutils.hpp"

#include <stdexcept>
#include <string.h>


namespace jjm
{

//This program assumes that std::string contains utd8 unless otherwise 
//explicitly specified. 
typedef  std::string                            Utf8String; 
typedef  std::basic_string<char>                Utf8String; 
typedef  std::basic_string<Utf8EncodingUnit>    Utf8String; 

//Utf16String is std::wstring on windows. 
typedef  std::basic_string<Utf16EncodingUnit>   Utf16String; 
#ifdef _WIN32
    //ensure that std::wstring on windows is the same type as Utf16String
    typedef  std::wstring                       Utf16String; 
#endif

inline Utf8String makeU8Str(Utf16String const& u16str);
inline Utf16String makeU16Str(Utf8String const& u8str);

template <typename CpIter> 
Utf8String makeU8StrFromCpRange(std::pair<CpIter, CpIter> const& cpRange);
template <typename CpIter> 
Utf16String makeU16StrFromCpRange(std::pair<CpIter, CpIter> const& cpRange);

inline std::pair<Utf8ToCpBidiIterator<Utf8String::const_iterator>, Utf8ToCpBidiIterator<Utf8String::const_iterator> >
    makeCpRange(Utf8String const& ustr);
inline std::pair<Utf8ToCpBidiIterator<Utf8String::const_iterator>, Utf8ToCpBidiIterator<Utf8String::const_iterator> >
    makeCpRangeFromUtf8(Utf8String const& ustr);

inline std::pair<Utf16ToCpBidiIterator<Utf16String::const_iterator>, Utf16ToCpBidiIterator<Utf16String::const_iterator> >
    makeCpRange(Utf16String const& ustr);
inline std::pair<Utf16ToCpBidiIterator<Utf16String::const_iterator>, Utf16ToCpBidiIterator<Utf16String::const_iterator> >
    makeCpRangeFromUtf16(Utf16String const& ustr);
}

//Utility to allow passing Utf16String to JFATAL
inline char const* JjmFatalHandlerUtil(jjm::Utf16String const& info_str) { return jjm::makeU8Str(info_str).c_str(); }

namespace jjm 
{


//**** **** **** **** 
//** Private Implementation

inline Utf8String makeU8Str(Utf16String const& u16str)
{
    return makeU8StrFromCpRange(makeCpRange(u16str)); 
}

inline Utf16String makeU16Str(Utf8String const& u8str)
{
    return makeU16StrFromCpRange(makeCpRange(u8str)); 
}

template <typename CpIter> 
Utf8String makeU8StrFromCpRange(std::pair<CpIter, CpIter> const& cpRange)
{
    auto utfrange = makeUtf8RangeFromCpRange(cpRange); 
    Utf8String x; 
    x.insert(x.end(), utfrange.first, utfrange.second); 
    return x; 
}

template <typename CpIter> 
Utf16String makeU16StrFromCpRange(std::pair<CpIter, CpIter> const& cpRange)
{
    auto utfrange = makeUtf16RangeFromCpRange(cpRange); 
    Utf16String x; 
    x.insert(x.end(), utfrange.first, utfrange.second); 
    return x; 
}

inline std::pair<Utf8ToCpBidiIterator<Utf8String::const_iterator>, Utf8ToCpBidiIterator<Utf8String::const_iterator> >
    makeCpRange(Utf8String const& ustr)
{
    return makeCpRangeFromUtf8(std::make_pair(ustr.begin(), ustr.end())); 
}

inline std::pair<Utf8ToCpBidiIterator<Utf8String::const_iterator>, Utf8ToCpBidiIterator<Utf8String::const_iterator> >
    makeCpRangeFromUtf8(Utf8String const& ustr)
{
    return makeCpRangeFromUtf8(std::make_pair(ustr.begin(), ustr.end())); 
}

inline std::pair<Utf16ToCpBidiIterator<Utf16String::const_iterator>, Utf16ToCpBidiIterator<Utf16String::const_iterator> >
    makeCpRange(Utf16String const& ustr)
{
    return makeCpRangeFromUtf16(std::make_pair(ustr.begin(), ustr.end()));
}

inline std::pair<Utf16ToCpBidiIterator<Utf16String::const_iterator>, Utf16ToCpBidiIterator<Utf16String::const_iterator> >
    makeCpRangeFromUtf16(Utf16String const& ustr)
{
    return makeCpRangeFromUtf16(std::make_pair(ustr.begin(), ustr.end()));
}


}//namespace jjm

#endif
