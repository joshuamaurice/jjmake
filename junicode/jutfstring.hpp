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

//Helper typedefs
typedef  Utf8String   U8String;
typedef  Utf16String  U16String;

//

template <typename Iter>
Utf8String  & appendUtf8 (Utf8String  & u8str , std::pair<Iter, Iter> const& utf8Range); 
template <typename Iter>
Utf16String & appendUtf16(Utf16String & u16str, std::pair<Iter, Iter> const& utf16Range); 

template <typename Iter>
Utf8String  & appendCp(Utf8String  & u8str , std::pair<Iter, Iter> const& cpRange); 
template <typename Iter>
Utf16String & appendCp(Utf16String & u16str, std::pair<Iter, Iter> const& cpRange); 

inline Utf8String  & appendCp(Utf8String  & u8str , UnicodeCodePoint cp); 
inline Utf16String & appendCp(Utf16String & u16str, UnicodeCodePoint cp); 

inline Utf8String makeU8Str(Utf16String const& u16str);
inline Utf16String makeU16Str(Utf8String const& u8str);

inline std::pair<Utf8String ::const_iterator, Utf8String ::const_iterator> makeUtf8Range (Utf8String  const& u8str);
inline std::pair<Utf16String::const_iterator, Utf16String::const_iterator> makeUtf16Range(Utf16String const& u16str);

template <typename Iter>
Utf8String makeU8StrFromUtf8(std::pair<Iter, Iter> const& range);
template <typename Iter>
Utf16String makeU16StrFromUtf16(std::pair<Iter, Iter> const& range);

template <typename Iter> 
Utf8String makeU8StrFromCpRange(std::pair<Iter, Iter> const& cpRange);
template <typename Iter> 
Utf16String makeU16StrFromCpRange(std::pair<Iter, Iter> const& cpRange);

inline std::pair<Utf8ToCpBidiIterator<Utf8String::const_iterator>, Utf8ToCpBidiIterator<Utf8String::const_iterator> >
    makeCpRange(Utf8String const& ustr);
inline std::pair<Utf8ToCpBidiIterator<Utf8String::const_iterator>, Utf8ToCpBidiIterator<Utf8String::const_iterator> >
    makeCpRangeFromUtf8(Utf8String const& ustr);

inline std::pair<Utf16ToCpBidiIterator<Utf16String::const_iterator>, Utf16ToCpBidiIterator<Utf16String::const_iterator> >
    makeCpRange(Utf16String const& ustr);
inline std::pair<Utf16ToCpBidiIterator<Utf16String::const_iterator>, Utf16ToCpBidiIterator<Utf16String::const_iterator> >
    makeCpRangeFromUtf16(Utf16String const& ustr);

}//namespace jjm

//Utility to allow passing Utf16String to JFATAL
inline char const* JjmFatalHandlerUtil(jjm::Utf16String const& info_str) { return jjm::makeU8Str(info_str).c_str(); }

namespace jjm 
{


//**** **** **** **** 
//** Private Implementation

template <typename Iter>
Utf8String & appendUtf8(Utf8String & u8str, std::pair<Iter, Iter> const& utf8Range)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(Utf8EncodingUnit), "Invalid Template Type Parameter"); 
    u8str.append(utf8Range.first, utf8Range.second); 
    return u8str; 
}

template <typename Iter>
Utf16String & appendUtf16(Utf16String & u16str, std::pair<Iter, Iter> const& utf16Range)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(Utf16EncodingUnit), "Invalid Template Type Parameter"); 
    u16str.append(utf16Range.first, utf16Range.second); 
    return u16str; 
}

template <typename Iter>
Utf8String & appendCp(Utf8String & u8str, std::pair<Iter, Iter> const& cpRange)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
    auto y = makeUtf8RangeFromCpRange(cpRange); 
    u8str.append(y.first, y.second); 
    return u8str; 
}

template <typename Iter>
Utf16String & appendCp(Utf16String & u16str, std::pair<Iter, Iter> const& cpRange)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
    auto y = makeUtf8RangeFromCpRange(cpRange); 
    u16str.append(y.first, y.second); 
    return u16str; 
}

inline Utf8String & appendCp(Utf8String & u8str, UnicodeCodePoint cp)
{
    UnicodeCodePoint x[1] = { cp }; 
    auto y = makeUtf8RangeFromCpRange(std::make_pair(x + 0, x + 1)); 
    u8str.append(y.first, y.second); 
    return u8str; 
}

inline Utf16String & appendCp(Utf16String & u16str, UnicodeCodePoint cp)
{
    UnicodeCodePoint x[1] = { cp }; 
    auto y = makeUtf16RangeFromCpRange(std::make_pair(x + 0, x + 1)); 
    u16str.append(y.first, y.second); 
    return u16str; 
}

inline Utf8String makeU8Str(Utf16String const& u16str)
{
    return makeU8StrFromCpRange(makeCpRange(u16str)); 
}

inline Utf16String makeU16Str(Utf8String const& u8str)
{
    return makeU16StrFromCpRange(makeCpRange(u8str)); 
}

inline std::pair<Utf8String::const_iterator, Utf8String::const_iterator> 
    makeUtf8Range(Utf8String const& u8str)
{
    std::pair<Utf8String::const_iterator, Utf8String::const_iterator> x;
    x.first = u8str.begin();
    x.second = u8str.end(); 
    return x; 
}

inline std::pair<Utf16String::const_iterator, Utf16String::const_iterator> 
    makeUtf16Range(Utf16String const& u16str)
{
    std::pair<Utf16String::const_iterator, Utf16String::const_iterator> x; 
    x.first = u16str.begin();
    x.second = u16str.end(); 
    return x; 
}

template <typename Iter>
Utf8String makeU8StrFromUtf8(std::pair<Iter, Iter> const& range)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(Utf8EncodingUnit), "Invalid Template Type Parameter"); 
    return Utf8String(range.first, range.second); 
}

template <typename Iter>
Utf16String makeU16StrFromUtf16(std::pair<Iter, Iter> const& range)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(Utf16EncodingUnit), "Invalid Template Type Parameter"); 
    return Utf16String(range.first, range.second); 
}

template <typename Iter> 
Utf8String makeU8StrFromCpRange(std::pair<Iter, Iter> const& cpRange)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
    auto utfrange = makeUtf8RangeFromCpRange(cpRange); 
    Utf8String x; 
    x.insert(x.end(), utfrange.first, utfrange.second); 
    return x; 
}

template <typename Iter> 
Utf16String makeU16StrFromCpRange(std::pair<Iter, Iter> const& cpRange)
{
    static_assert(sizeof(**(Iter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
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
