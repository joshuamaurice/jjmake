// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUNICODEBASE_HPP_HEADER_GUARD
#define JUNICODE_JUNICODEBASE_HPP_HEADER_GUARD

#include "jbase/jstdint.hpp"
#include "jbase/jtemplatemetaprogrammingutils.hpp"

#include <limits>
#include <limits.h>


namespace jjm
{

//**** **** **** **** 
//** Public typedefs

typedef  std::uint32_t  UnicodeCodePoint; 

//We're going with a potentially signed type "char" because std::string is so 
//commonly used and we might as well get some code reuse. 
typedef  char  Utf8EncodingUnit; 
static_assert(CHAR_BIT == 8 && sizeof(char) == 1, "ERROR"); 

//Utf16EncodingUnit is defined to always be 16 bits. 
//On windows, it's not defined in terms of std::uint16_t because wchar_t is
//signed on windows, and doing it this way allows easy interoperability with 
//the windows APIs. 
#ifdef _WIN32
    typedef  wchar_t  Utf16EncodingUnit; 
#else
    typedef  std::uint16_t  Utf16EncodingUnit; 
#endif
static_assert(CHAR_BIT == 8 && sizeof(Utf16EncodingUnit) == 2, "ERROR"); 
static_assert(std::numeric_limits<Utf16EncodingUnit>::is_signed == false, "ERROR"); 

inline bool isAsciiLetter(UnicodeCodePoint c)
{
    return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')); 
}



//This is used below to template-typedef and template-specialized 
//the BasicUtf-- classes into Utf8 and Utf16
enum UtfKind { Utf8Kind = 8, Utf16Kind = 16 }; 

namespace Internal 
{   //Private Implementation Details
    template <UtfKind utfKind> struct UtfEncodingUnit {};
    template <> struct UtfEncodingUnit <Utf8Kind> { typedef Utf8EncodingUnit T; };
    template <> struct UtfEncodingUnit <Utf16Kind> { typedef Utf16EncodingUnit T; };
}




/*
Will return the utf8 sequence which describes the input code point. 
For invalid code points, returnvalue.len == -1; 
*/
struct Utf8Sequence { int len; char seq[4]; }; 
inline Utf8Sequence writeUtf8(UnicodeCodePoint cp);

/*
Will return the utf16 sequence which describes the input code point. 
For invalid code points, returnvalue.len == -1; 
*/
struct Utf16Sequence { int len; Utf16EncodingUnit seq[2]; }; 
inline Utf16Sequence writeUtf16(UnicodeCodePoint cp);

/*
Reads the next codepoint from a stream of utf8 data. Will not read past 
argument "end". 

Returns the code point on valid input, and moves "current" to 
one-past-the-end of what was just read. 

If the input utf8 is invalid, -1 is returned. For bidi iterators, the argument
"current" is moved back to its starting position. For input iterators, the 
state of "current" is unspecified. 

If the end is reached before the end of an otherwise valid utf8 sequence, then
-1 is returned. For bidi iterators, the argument "current" is moved back to 
its starting position. For input iterators, "current" is equal to "end". 
*/
template <typename Int8Iter>
UnicodeCodePoint readUtf8Forward(Int8Iter & current, Int8Iter const end);

/*
Reads the next codepoint from a stream of utf16 data. Will not read past 
argument "end". 

Returns the code point on valid input, and moves "current" to 
one-past-the-end of what was just read. 

If the input utf16 is invalid, -1 is returned. For bidi iterators, the argument
"current" is moved back to its starting position. For input iterators, the 
state of "current" is unspecified. 

If the end is reached before the end of an otherwise valid utf16 sequence, then
-1 is returned. For bidi iterators, the argument "current" is moved back to 
its starting position. For input iterators, "current" is equal to "end". 
*/
template <typename Int16Iter>
UnicodeCodePoint readUtf16Forward(Int16Iter & current, Int16Iter const end);

/* 
Reads the codepoint that is described by utf8 just before the current 
position of argument "current". Will not read before argument "begin". 

Returns the code point on valid input, and moves "current" to point at the 
beginning of the utf8 segment that describes that code point. 

If the input utf8 is invalid, -1 is returned, and the argument "current" is 
moved back to its starting position. 

If the beginning is reached before the start of an otherwise valid utf8 
sequence, then -1 is returned. and "current" is equal to "begin". 

If "begin" is reached before the start of an otherwise valid utf8 sequence, 
then -1 is returned, and the argument "current" is moved back to its starting 
position. 
*/
template <typename Int8Iter>
UnicodeCodePoint readUtf8Backward(Int8Iter const begin, Int8Iter & current);

/* 
Reads the codepoint that is described by utf16 just before the current 
position of argument "current". Will not read before argument "begin". 

Returns the code point on valid input, and moves "current" to point at the 
beginning of the utf8 segment that describes that code point. 

If the input utf16 is invalid, -1 is returned, and the argument "current" is 
moved back to its starting position. 

If "begin" is reached before the start of an otherwise valid utf16 sequence, 
then -1 is returned, and the argument "current" is moved back to its starting 
position. 
*/
template <typename Int16Iter>
UnicodeCodePoint readUtf16Backward(Int16Iter const begin, Int16Iter & current);




//**** **** **** **** 
//** Private Implementation

inline Utf8Sequence writeUtf8(UnicodeCodePoint cp)
{
    Utf8Sequence result; 

    if (cp <= 0x007F)
    {   result.len = 1; 
        result.seq[0] = static_cast<char>(cp); 
        return result; 
    }
    if (cp <= 0x07FF)
    {   result.len = 2; 
        result.seq[0] = static_cast<char>(0xC0 |         (cp >> 6)); 
        result.seq[1] = static_cast<char>(0x80 | (0x3F &  cp)); 
        return result; 
    }
    if (0xD800 <= cp && cp <= 0xDFFF)
    {   result.len = -1; 
        return result; 
    }
    if (cp <= 0xFFFF)
    {   result.len = 3; 
        result.seq[0] = static_cast<char>(0xE0 |         (cp >> 12)); 
        result.seq[1] = static_cast<char>(0x80 | (0x3F & (cp >> 6))); 
        result.seq[2] = static_cast<char>(0x80 | (0x3F &  cp)); 
        return result;
    }
    if (cp <= 0x1FFFFF)
    {   result.len = 4; 
        result.seq[0] = static_cast<char>(0xF0 |         (cp >> 18)); 
        result.seq[1] = static_cast<char>(0x80 | (0x3F & (cp >> 12))); 
        result.seq[2] = static_cast<char>(0x80 | (0x3F & (cp >> 6))); 
        result.seq[3] = static_cast<char>(0x80 | (0x3F &  cp)); 
        return result;
    }
    result.len = -1; 
    return result; 
}

inline Utf16Sequence writeUtf16(UnicodeCodePoint cp)
{
    Utf16Sequence result; 
    if (cp < 0x10000)
    {   if (0x0D800 <= cp && cp <= 0xDFFF)
        {   result.len = -1;
            return result;
        }
        result.len = 1; 
        result.seq[0] = static_cast<Utf16EncodingUnit>(cp); 
        return result;
    }
    if (cp <= 0x10FFFF)
    {   UnicodeCodePoint cp2 = cp - 0x10000;
        result.len = 2; 
        result.seq[0] = 0xD800 | static_cast<std::uint16_t>(cp2 >> 10); 
        result.seq[1] = 0xDC00 | static_cast<std::uint16_t>(cp2 & 0x3FF); 
        return result; 
    }
    result.len = -1; 
    return result; 
}

namespace Internal
{
    inline bool isUtf8Encoding1_byte1(unsigned char c) { return (0x80 & c) == 0x00; }
    inline bool isUtf8Encoding2_byte1(unsigned char c) { return (0xE0 & c) == 0xC0; }
    inline bool isUtf8Encoding3_byte1(unsigned char c) { return (0xF0 & c) == 0xE0; }
    inline bool isUtf8Encoding4_byte1(unsigned char c) { return (0xF8 & c) == 0xF0; }
    inline bool isUtf8_byte2_plus    (unsigned char c) { return (0xC0 & c) == 0x80; }

    template <typename Int8Iter, typename IterTag>
    class ReadUtf8ForwardImpl
    {
    public:
        static_assert(sizeof(**(Int8Iter*)0) == sizeof(Utf8EncodingUnit), "Invalid Template Type Parameter"); 
        static_assert(
                IsConvertibleTo<
                        typename std::iterator_traits<Int8Iter>::iterator_category, 
                        std::bidirectional_iterator_tag
                        >::b,
                "Invalid Template Type Parameter");
        UnicodeCodePoint operator()(Int8Iter & current, Int8Iter const end) const; 
    };

    template <typename Int8Iter>
    class ReadUtf8ForwardImpl <Int8Iter, std::input_iterator_tag>
    {
    public:
        static_assert(sizeof(**(Int8Iter*)0) == sizeof(Utf8EncodingUnit), "Invalid Template Type Parameter"); 
        UnicodeCodePoint operator()(Int8Iter & current, Int8Iter const end) const; 
    };

    template <typename Int8Iter>
    UnicodeCodePoint ReadUtf8ForwardImpl<Int8Iter, std::input_iterator_tag>::operator()
        (Int8Iter & current, Int8Iter const end) const
    {
        if (current == end)
            return -1; 
        std::uint8_t eu = *current; ++current; 
        if (isUtf8Encoding1_byte1(eu))
            return eu; 
        if (isUtf8Encoding2_byte1(eu))
        {   UnicodeCodePoint cp = (0x1F & eu);
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if (!isUtf8_byte2_plus(eu))
                return -1; 
            cp <<= 6;
            cp |= 0x3F & eu;
            return cp;
        }
        if (isUtf8Encoding3_byte1(eu))
        {   UnicodeCodePoint cp = (0x0F & eu);
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if (!isUtf8_byte2_plus(eu))
                return -1; 
            cp <<= 6;
            cp |= 0x3F & eu;
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if (!isUtf8_byte2_plus(eu))
                return -1; 
            cp <<= 6;
            cp |= 0x3F & eu;
            return cp; 
        }
        if (isUtf8Encoding4_byte1(eu))
        {   UnicodeCodePoint cp = (0x07 & eu);
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if (!isUtf8_byte2_plus(eu))
                return -1; 
            cp <<= 6;
            cp |= 0x3F & eu;
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if (!isUtf8_byte2_plus(eu))
                return -1; 
            cp <<= 6;
            cp |= 0x3F & eu;
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if (!isUtf8_byte2_plus(eu))
                return -1; 
            cp <<= 6;
            cp |= 0x3F & eu;
            return cp; 
        }
        return -1; 
    }

    template <typename Int8Iter, typename IterTag>
    UnicodeCodePoint ReadUtf8ForwardImpl<Int8Iter, IterTag>::operator() 
        (Int8Iter & current, Int8Iter const end) const
    {
        Int8Iter const tmp = current;
        UnicodeCodePoint cp = ReadUtf8ForwardImpl<Int8Iter, std::input_iterator_tag>()(current, end); 
        if (cp == static_cast<UnicodeCodePoint>(-1))
            current = tmp;
        return cp; 
    }
}

template <typename Int8Iter>
UnicodeCodePoint readUtf8Forward(Int8Iter & current, Int8Iter const end)
{
    return Internal::ReadUtf8ForwardImpl<Int8Iter, typename std::iterator_traits<Int8Iter>::iterator_category>()(current, end); 
}

template <typename Int8Iter>
UnicodeCodePoint readUtf8Backward(Int8Iter const begin, Int8Iter & current)
{
    const int b = IsConvertibleTo<
            typename std::iterator_traits<Int8Iter>::iterator_category, 
            std::bidirectional_iterator_tag
            >::b;
    static_assert(b, "Invalid Template Type Parameter"); 

    Int8Iter const tmp = current; 

    UnicodeCodePoint cp = 0;
    UnicodeCodePoint shift = 0;
    if (current == begin)
    {   current = tmp; 
        return -1; 
    }
    std::uint8_t eu = *--current;

    if (Internal::isUtf8Encoding1_byte1(eu))
        return eu; 
    if ( ! Internal::isUtf8_byte2_plus(eu))
    {   current = tmp; 
        return -1; 
    }
    cp |= (0x3F & eu) << shift;
    shift += 6;
    if (current == begin)
    {   current = tmp; 
        return -1; 
    }
    eu = *--current;

    if (Internal::isUtf8Encoding2_byte1(eu))
    {   cp |= (0x1F & eu) << shift;
        return cp; 
    }
    if ( ! Internal::isUtf8_byte2_plus(eu))
    {   current = tmp; 
        return -1; 
    }
    cp |= (0x3F & eu) << shift;
    shift += 6;
    if (current == begin)
    {   current = tmp; 
        return -1; 
    }
    eu = *--current;

    if (Internal::isUtf8Encoding3_byte1(eu))
    {   cp |= (0x0F & eu) << shift;
        return cp; 
    }
    if ( ! Internal::isUtf8_byte2_plus(eu))
    {   current = tmp; 
        return -1; 
    }
    cp |= (0x3F & eu) << shift;
    shift += 6;
    if (current == begin)
    {   current = tmp; 
        return -1; 
    }
    eu = *--current;

    if (Internal::isUtf8Encoding4_byte1(eu))
    {   cp |= (0x07 & eu) << shift;
        return cp; 
    }
    current = tmp; 
    return -1; 
}

namespace Internal
{
    //          xxxx xxxx xxxx xxxx  ->                      xxxx xxxx xxxx xxxx
    //000u uuuu xxxx xxxx xxxx xxxx  ->  1101 10ww wwxx xxxx 1101 11xx xxxx xxxx
    //Note: wwww = uuuuu - 1
    inline bool isUtf16OneUnitEncoding(std::uint16_t c) { return (c <= 0xD7FF) ||                              0xE000 <= c; }
    inline bool isUtf16HighSurrogate(  std::uint16_t c) { return       0xD800 <= c && c <= 0xDBFF; }
    inline bool isUtf16LowSurrogate(   std::uint16_t c) { return                           0xDC00 <= c && c <= 0xDFFF; }

    template <typename Int16Iter, typename IterTag>
    class ReadUtf16ForwardImpl
    {
    public:
        static_assert(sizeof(**(Int16Iter*)0) == sizeof(Utf16EncodingUnit), "Invalid Template Type Parameter"); 
        static_assert(
                IsConvertibleTo<
                        typename std::iterator_traits<Int16Iter>::iterator_category, 
                        std::bidirectional_iterator_tag
                        >::b,
                "Invalid Template Type Parameter");
        UnicodeCodePoint operator()(Int16Iter & current, Int16Iter const end) const; 
    };

    template <typename Int16Iter>
    class ReadUtf16ForwardImpl <Int16Iter, std::input_iterator_tag>
    {
    public:
        static_assert(sizeof(**(Int16Iter*)0) == sizeof(Utf16EncodingUnit), "Invalid Template Type Parameter"); 
        UnicodeCodePoint operator()(Int16Iter & current, Int16Iter const end) const; 
    };

    template <typename Int16Iter>
    UnicodeCodePoint ReadUtf16ForwardImpl<Int16Iter, std::input_iterator_tag>::operator()
        (Int16Iter & current, Int16Iter const end) const
    {
        if (current == end)
            return -1; 
        std::uint16_t eu = *current; ++current;
        if (isUtf16OneUnitEncoding(eu))
            return eu; 
        if (isUtf16HighSurrogate(eu))
        {   UnicodeCodePoint cp = 0x3FF & eu;
            if (current == end)
                return -1; 
            eu = *current; ++current;
            if ( ! isUtf16LowSurrogate(eu))
                return -1; 
            cp <<= 10;
            cp |= 0x3FF & eu;
            cp += 0x10000;
            return cp; 
        }
        return -1; 
    }

    template <typename Int16Iter, typename IterTag>
    UnicodeCodePoint ReadUtf16ForwardImpl<Int16Iter, IterTag>::operator()
        (Int16Iter & current, Int16Iter const end) const
    {
        Int16Iter const tmp = current;
        UnicodeCodePoint cp = ReadUtf16ForwardImpl<Int16Iter, std::input_iterator_tag>()(current, end); 
        if (cp == static_cast<UnicodeCodePoint>(-1))
            current = tmp;
        return cp; 
    }
}

template <typename Int16Iter>
UnicodeCodePoint readUtf16Forward(Int16Iter & current, Int16Iter const end)
{
    return Internal::ReadUtf16ForwardImpl<Int16Iter, typename std::iterator_traits<Int16Iter>::iterator_category>()(current, end); 
}

template <typename Int16Iter>
UnicodeCodePoint readUtf16Backward(Int16Iter const begin, Int16Iter & current)
{
    const int b = IsConvertibleTo<
            typename std::iterator_traits<Int16Iter>::iterator_category, 
            std::bidirectional_iterator_tag
            >::b;
    static_assert(b, "Invalid Template Type Parameter"); 

    Int16Iter const tmp = current; 
    if (current == begin)
    {   current = tmp; 
        return -1; 
    }
    std::uint16_t eu = *--current;
    if (Internal::isUtf16OneUnitEncoding(eu))
        return eu; 
    if (Internal::isUtf16LowSurrogate(eu))
    {   UnicodeCodePoint cp = (0x3FF & eu) << 10;
        if (current == begin)
        {   current = tmp;
            return -1; 
        }
        eu = *--current;
        if ( ! Internal::isUtf16HighSurrogate(eu))
        {   current = tmp;
            return -1; 
        }
        cp |= 0x3FF & eu;
        cp += 0x10000;
        return cp; 
    }
    current = tmp;
    return -1; 
}



}//namespace jjm

#endif
