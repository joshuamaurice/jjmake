// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUTFUTILS_HPP_HEADER_GUARD
#define JUNICODE_JUTFUTILS_HPP_HEADER_GUARD

#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"
#include "jbase/jstaticassert.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jtemplatemetaprogrammingutils.hpp"

#include <memory>
#include <utility>


namespace jjm
{

// ---- ---- ---- ---- 
// stuff intended for use by users:

typedef  std::uint32_t  UnicodeCodePoint;
typedef  std::uint32_t  CodePoint;


/* Utf16EncodingUnit is defined to be a 16 bit unsigned integer type. 
On windows, it is also a typedef of wchar_t, to allow easy use of win32 APIs.*/
#ifdef _WIN32
    typedef  wchar_t  Utf16EncodingUnit;
#else
    typedef  std::uint16_t  Utf16EncodingUnit;
#endif
JSTATICASSERT(CHAR_BIT == 8 && sizeof(Utf16EncodingUnit) == 2); 
JSTATICASSERT(std::numeric_limits<Utf16EncodingUnit>::is_signed == false); 


//Returns 1, 2, 3, or 4. 
//Throws std::exception on invalid input. 
inline int utf8LengthOf(UnicodeCodePoint cp);

//Returns 1 or 2. 
//Throws std::exception on invalid input. 
inline int utf16LengthOf(UnicodeCodePoint cp);

//Will write the codepoint to the byte array pointed to by at. 
//Will update the iterator to point to past-the-end of what was just written. 
//It is the user's responsibility to ensure that sufficient space remains. 
//The user can check how much will be written by calling utf8LengthOf. 
//Throws std::exception on invalid codepoint input. 
template <typename Int8Iter>
void writeUtf8(UnicodeCodePoint cp, Int8Iter & at);

//Will write the codepoint to the byte array pointed to by at. 
//Will update the iterator to point to past-the-end of what was just written. 
//It is the user's responsibility to ensure that sufficient space remains. 
//The user can check how much will be written by calling utf16LengthOf. 
//Throws std::exception on invalid codepoint input. 
template <typename Int16Iter>
void writeUtf16(UnicodeCodePoint cp, Int16Iter & at);

//Reads the next codepoint from a stream of utf8 data. 
//Modifies the argument iter to point to the next piece of data. 
//On invalid input, it throws std::exception and the position of iter is unspecified. 
template <typename Int8Iter>
CodePoint readUtf8Forward(Int8Iter & current, Int8Iter const end);

//Reads the next codepoint from a stream of utf16 data. 
//Modifies the argument iter to point to the next piece of data. 
//On invalid input, it throws std::exception and the position of iter is unspecified. 
template <typename Int16Iter>
CodePoint readUtf16Forward(Int16Iter & iter, Int16Iter const end);

//Reads the previous codepoint from a bidi stream of utf8 data. 
//Modifies the argument it to point to the start of the code point segment. 
//On invalid input, it throws std::exception and the position of iter is unspecified. 
template <typename Int8Iter>
CodePoint readUtf8Backward(Int8Iter const begin, Int8Iter & iter);

//Reads the previous codepoint from a bidi stream of utf16 data. 
//Modifies the argument it to point to the start of the code point segment. 
//On invalid input, it throws std::exception and the position of iter is unspecified. 
template <typename Int16Iter>
CodePoint readUtf16Backward(Int16Iter const begin, Int16Iter & iter);


// ---- ---- ---- ---- 
// impl: 
inline int utf8LengthOf(UnicodeCodePoint cp)
{
    if (cp <= 0x007F) return 1;
    if (cp <= 0x07FF) return 2;
    if (0xD800 <= cp && cp <= 0xDFFF)
        throw std::runtime_error("utf8LengthOf() invalid code point input [0x" + toHexStr(cp) + "].");
    if (cp <= 0xFFFF) return 3;
    if (cp <= 0x1FFFFF) return 4;
    throw std::runtime_error("utf8LengthOf() invalid code point input [0x" + toHexStr(cp) + "].");
}

template <typename Int8Iter>
void writeUtf8(UnicodeCodePoint cp, Int8Iter & at)
{
    int const b = IsConvertibleTo<std::iterator_traits<Int8Iter>::iterator_category, std::output_iterator_tag>::b
            || IsConvertibleTo<std::iterator_traits<Int8Iter>::iterator_category, std::forward_iterator_tag>::b;
    JSTATICASSERT(b);

    if (cp <= 0x007F)
    {   *at = static_cast<std::uint8_t>(cp); ++at;
        return;
    }
    if (cp <= 0x07FF)
    {   *at = static_cast<std::uint8_t>(0xC0 |         (cp >> 6)); ++at;
        *at = static_cast<std::uint8_t>(0x80 | (0x3F &  cp)); ++at;
        return;
    }
    if (0xD800 <= cp && cp <= 0xDFFF)
        throw std::runtime_error("writeUtf8() invalid code point input [0x" + toHexStr(cp) + "].");
    if (cp <= 0xFFFF)
    {   *at = static_cast<std::uint8_t>(0xE0 |         (cp >> 12)); ++at;
        *at = static_cast<std::uint8_t>(0x80 | (0x3F & (cp >> 6))); ++at;
        *at = static_cast<std::uint8_t>(0x80 | (0x3F &  cp)); ++at;
        return;
    }
    if (cp <= 0x1FFFFF)
    {   *at = static_cast<std::uint8_t>(0xF0 |         (cp >> 18)); ++at;
        *at = static_cast<std::uint8_t>(0x80 | (0x3F & (cp >> 12))); ++at;
        *at = static_cast<std::uint8_t>(0x80 | (0x3F & (cp >> 6))); ++at;
        *at = static_cast<std::uint8_t>(0x80 | (0x3F &  cp)); ++at;
        return;
    }
    throw std::runtime_error("writeUtf8() invalid code point input [0x" + toHexStr(cp) + "].");
}

namespace Internal
{
    inline bool isUtf8Encoding1_byte1(unsigned char c) { return (0x80 & c) == 0x00; }
    inline bool isUtf8Encoding2_byte1(unsigned char c) { return (0xE0 & c) == 0xC0; }
    inline bool isUtf8Encoding3_byte1(unsigned char c) { return (0xF0 & c) == 0xE0; }
    inline bool isUtf8Encoding4_byte1(unsigned char c) { return (0xF8 & c) == 0xF0; }
    inline bool isUtf8_byte2_plus    (unsigned char c) { return (0xC0 & c) == 0x80; }
}

template <typename Int8Iter>
CodePoint readUtf8Forward(Int8Iter & at, Int8Iter const end)
{   
    const int b = IsConvertibleTo<
            typename std::iterator_traits<Int8Iter>::iterator_category, 
            std::input_iterator_tag
            >::b;
    JSTATICASSERT(b);

    using namespace Internal;
    if (at == end)
        throw std::runtime_error("readUtf8Forward() out of bounds.");
    std::uint8_t eu = *at; ++at; 
    if (isUtf8Encoding1_byte1(eu))
        return eu; 
    if (isUtf8Encoding2_byte1(eu))
    {   UnicodeCodePoint cp = (0x1F & eu);
        if (at == end)
            throw std::runtime_error("readUtf8Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf8_byte2_plus(eu))
            throw std::runtime_error("readUtf8Forward() invalid utf8 input data.");
        cp <<= 6;
        cp |= 0x3F & eu;
        return cp;
    }
    if (isUtf8Encoding3_byte1(eu))
    {   UnicodeCodePoint cp = (0x0F & eu);
        if (at == end)
            throw std::runtime_error("readUtf8Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf8_byte2_plus(eu))
            throw std::runtime_error("readUtf8Forward() invalid utf8 input data.");
        cp <<= 6;
        cp |= 0x3F & eu;
        if (at == end)
            throw std::runtime_error("readUtf8Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf8_byte2_plus(eu))
            throw std::runtime_error("readUtf8Forward() invalid utf8 input data.");
        cp <<= 6;
        cp |= 0x3F & eu;
        return cp; 
    }
    if (isUtf8Encoding4_byte1(eu))
    {   UnicodeCodePoint cp = (0x07 & eu);
        if (at == end)
            throw std::runtime_error("readUtf8Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf8_byte2_plus(eu))
            throw std::runtime_error("readUtf8Forward() invalid utf8 input data.");
        cp <<= 6;
        cp |= 0x3F & eu;
        if (at == end)
            throw std::runtime_error("readUtf8Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf8_byte2_plus(eu))
            throw std::runtime_error("readUtf8Forward() invalid utf8 input data.");
        cp <<= 6;
        cp |= 0x3F & eu;
        if (at == end)
            throw std::runtime_error("readUtf8Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf8_byte2_plus(eu))
            throw std::runtime_error("readUtf8Forward() invalid utf8 input data.");
        cp <<= 6;
        cp |= 0x3F & eu;
        return cp; 
    }
    throw std::runtime_error("readUtf8Forward() invalid utf8 input data and/or out of position iter.");
}

template <typename Int8Iter>
CodePoint readUtf8Backward(Int8Iter const begin, Int8Iter & at)
{
    const int b = IsConvertibleTo<std::iterator_traits<Int8Iter>::iterator_category, std::bidirectional_iterator_tag>::b;
    JSTATICASSERT(b);

    using namespace Internal;
    UnicodeCodePoint cp = 0;
    UnicodeCodePoint shift = 0;
    if (at == begin)
        throw std::runtime_error("readUtf8Backward() out of bounds.");
    std::uint8_t eu = *--at;

    if (isUtf8Encoding1_byte1(eu))
        return eu; 
    if (!isUtf8_byte2_plus(eu))
        throw std::runtime_error("readUtf8Backward() invalid utf8 input data.");
    cp |= (0x3F & eu) << shift;
    shift += 6;
    if (at == begin)
        throw std::runtime_error("readUtf8Backward() out of bounds.");
    eu = *--at;

    if (isUtf8Encoding2_byte1(eu))
    {   cp |= (0x1F & eu) << shift;
        return cp; 
    }
    if (!isUtf8_byte2_plus(eu))
        throw std::runtime_error("readUtf8Backward() invalid utf8 input data.");
    cp |= (0x3F & eu) << shift;
    shift += 6;
    if (at == begin)
        throw std::runtime_error("readUtf8Backward() out of bounds.");
    eu = *--at;

    if (isUtf8Encoding3_byte1(eu))
    {   cp |= (0x0F & eu) << shift;
        return cp; 
    }
    if (!isUtf8_byte2_plus(eu))
        throw std::runtime_error("readUtf8Backward() invalid utf8 input data.");
    cp |= (0x3F & eu) << shift;
    shift += 6;
    if (at == begin)
        throw std::runtime_error("readUtf8Backward() out of bounds.");
    eu = *--at;

    if (isUtf8Encoding4_byte1(eu))
    {   cp |= (0x07 & eu) << shift;
        return cp; 
    }
    throw std::runtime_error("readUtf8Backward() invalid utf8 input data.");
}

inline int utf16LengthOf(UnicodeCodePoint cp)
{
    if (cp < 0x10000) 
    {   if (0x0D800 <= cp && cp <= 0xDFFF) 
            throw std::runtime_error("utf16LengthOf() invalid code point input [0x" + toHexStr(cp) + "].");
        return 1;
    }
    if (cp <= 0x10FFFF) 
        return 2;
    throw std::runtime_error("utf16LengthOf() invalid code point input [0x" + toHexStr(cp) + "].");
}

template <typename Int16Iter>
void writeUtf16(UnicodeCodePoint cp, Int16Iter & at)
{
    int const b = IsConvertibleTo<std::iterator_traits<Int16Iter>::iterator_category, std::output_iterator_tag>::b
            || IsConvertibleTo<std::iterator_traits<Int16Iter>::iterator_category, std::forward_iterator_tag>::b;
    JSTATICASSERT(b);

    if (cp < 0x10000)
    {   if (0x0D800 <= cp && cp <= 0xDFFF)
            throw std::runtime_error("writeUtf16() invalid code point input [0x" + toHexStr(cp) + "].");
        *at = static_cast<std::uint16_t>(cp); ++at;
        return;
    }
    if (cp <= 0x10FFFF)
    {   CodePoint cp2 = cp - 0x10000;
        *at = 0xD800 | static_cast<std::uint16_t>(cp2 >> 10); ++at;
        *at = 0xDC00 | static_cast<std::uint16_t>(cp2 & 0x3FF); ++at;
        return;
    }
    throw std::runtime_error("writeUtf16() invalid code point input [0x" + toHexStr(cp) + "].");
}

namespace Internal
{
    //          xxxx xxxx xxxx xxxx  ->                      xxxx xxxx xxxx xxxx
    //000u uuuu xxxx xxxx xxxx xxxx  ->  1101 10ww wwxx xxxx 1101 11xx xxxx xxxx
    //Note: wwww = uuuuu - 1
    inline bool isUtf16OneUnitEncoding(std::uint16_t c) { return (c <= 0xD7FF) ||                              0xE000 <= c; }
    inline bool isUtf16HighSurrogate(  std::uint16_t c) { return       0xD800 <= c && c <= 0xDBFF; }
    inline bool isUtf16LowSurrogate(   std::uint16_t c) { return                           0xDC00 <= c && c <= 0xDFFF; }
}

template <typename Int16Iter>
CodePoint readUtf16Forward(Int16Iter & at, Int16Iter const end)
{   
    const int b = IsConvertibleTo<
            typename std::iterator_traits<Int16Iter>::iterator_category, 
            std::input_iterator_tag
            >::b;
    JSTATICASSERT(b);

    using namespace Internal;
    if (at == end)
        throw std::runtime_error("readUtf16Forward() out of bounds.");
    std::uint16_t eu = *at; ++at;
    if (isUtf16OneUnitEncoding(eu))
        return eu; 
    if (isUtf16HighSurrogate(eu))
    {   UnicodeCodePoint cp = 0x3FF & eu;
        if (at == end)
            throw std::runtime_error("readUtf16Forward() out of bounds.");
        eu = *at; ++at;
        if (!isUtf16LowSurrogate(eu))
            throw std::runtime_error("readUtf16Forward() invalid utf16 input data.");
        cp <<= 10;
        cp |= 0x3FF & eu;
        cp += 0x10000;
        return cp; 
    }
    throw std::runtime_error("readUtf16Forward() invalid utf16 input data and/or insufficient length of input range and/or out of position iter.");
}

template <typename Int16Iter>
CodePoint readUtf16Backward(Int16Iter const begin, Int16Iter & at)
{
    const int b = IsConvertibleTo<std::iterator_traits<Int16Iter>::iterator_category, std::bidirectional_iterator_tag>::b;
    JSTATICASSERT(b);

    using namespace Internal;
    if (at == begin)
        throw std::runtime_error("readUtf16Backward() out of bounds.");
    std::uint16_t eu = *--at;
    if (isUtf16OneUnitEncoding(eu))
        return eu; 
    if (isUtf16LowSurrogate(eu))
    {   UnicodeCodePoint cp = (0x3FF & eu) << 10;
        if (at == begin)
            throw std::runtime_error("readUtf16Backward() out of bounds.");
        eu = *--at;
        if (!isUtf16HighSurrogate(eu))
            throw std::runtime_error("readUtf16Backward() invalid utf16 input data.");
        cp |= 0x3FF & eu;
        cp += 0x10000;
        return cp; 
    }
    throw std::runtime_error("readUtf16Backward() invalid utf16 input data.");
}


} //namespace jjm

#endif
