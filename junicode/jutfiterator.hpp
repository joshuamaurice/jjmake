// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUTFITERATOR_HPP_HEADER_GUARD
#define JUNICODE_JUTFITERATOR_HPP_HEADER_GUARD

#include "junicodebase.hpp"

#include <iterator>
#include <stdexcept>


namespace jjm
{

//**** **** **** **** 
//** Public APIs - convert a range of utf to a range of UnicodeCodePoints

template <typename EncodingUnitIterator, UtfKind utfKind>
class BasicUtfToCpInputIterator;

template <typename EncodingUnitIterator, UtfKind utfKind>
class BasicUtfToCpBidiIterator;

template <typename Int8Iter>
using Utf8ToCpInputIterator = BasicUtfToCpInputIterator<Int8Iter, Utf8Kind>; 

template <typename Int8Iter>
using Utf8ToCpBidiIterator = BasicUtfToCpBidiIterator<Int8Iter, Utf8Kind>; 

template <typename Int16Iter>
using Utf16ToCpInputIterator = BasicUtfToCpInputIterator<Int16Iter, Utf16Kind>; 

template <typename Int16Iter>
using Utf16ToCpBidiIterator = BasicUtfToCpBidiIterator<Int16Iter, Utf16Kind>; 

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator== (BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& b);

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator== (BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& b);

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator!= (BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& b);

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator!= (BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& b);

template <typename EncodingUnitIterator, UtfKind utfKind>
class BasicUtfToCpInputIterator
{
public:
    static_assert(sizeof(**(EncodingUnitIterator*)0) == sizeof(typename Internal::UtfEncodingUnit<utfKind>::T), "Invalid Template Type Parameter"); 
public:
    typedef std::input_iterator_tag         iterator_category;
    typedef UnicodeCodePoint                value_type;
    typedef typename std::iterator_traits<UnicodeCodePoint*>::difference_type    difference_type;
    typedef UnicodeCodePoint const*         pointer;
    typedef UnicodeCodePoint const&         reference;
public:
    struct Proxy { UnicodeCodePoint cp; UnicodeCodePoint operator* () const { return cp; } };
public:
    BasicUtfToCpInputIterator(); //creates a "one-past-the-end" value
    BasicUtfToCpInputIterator(EncodingUnitIterator const& current, EncodingUnitIterator const& end); //"end" is used for bounds-checking
    BasicUtfToCpInputIterator&  operator++ (); //prefix
    Proxy                       operator++ (int); //postfix
    UnicodeCodePoint            operator* () const;  
    EncodingUnitIterator        getIter() const { return current; }

    friend bool operator== <> (BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& b);
    friend bool operator!= <> (BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& b);

private:
    EncodingUnitIterator current;
    EncodingUnitIterator end;
    UnicodeCodePoint cp;
};

template <typename EncodingUnitIterator, UtfKind utfKind>
class BasicUtfToCpBidiIterator
{
public:
    static_assert(sizeof(**(EncodingUnitIterator*)0) == sizeof(typename Internal::UtfEncodingUnit<utfKind>::T), "Invalid Template Type Parameter"); 
public:
    typedef std::bidirectional_iterator_tag     iterator_category;
    typedef UnicodeCodePoint                    value_type;
    typedef typename std::iterator_traits<UnicodeCodePoint*>::difference_type  difference_type;
    typedef UnicodeCodePoint const*             pointer;
    typedef UnicodeCodePoint const&             reference;
public:
    BasicUtfToCpBidiIterator(); 
    BasicUtfToCpBidiIterator(EncodingUnitIterator const& begin, EncodingUnitIterator const& current, EncodingUnitIterator const& end); //"begin" and "end" are used for bounds-checking
    BasicUtfToCpBidiIterator&   operator++ (); //prefix
    BasicUtfToCpBidiIterator    operator++ (int); //postfix
    BasicUtfToCpBidiIterator&   operator-- (); //prefix
    BasicUtfToCpBidiIterator    operator-- (int); //postfix
    UnicodeCodePoint            operator* () const;
    EncodingUnitIterator        getIter() const { return current; }

    friend bool operator== <> (BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& b);
    friend bool operator!= <> (BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& a, BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& b);

private:
    EncodingUnitIterator begin;
    EncodingUnitIterator current;
    EncodingUnitIterator end;
};

namespace Internal
{   //Private Implementation Details
    template <typename Int8Iter, typename IterTag> class MakeCpRangeFromUtf8Impl; 
    template <typename Int16Iter, typename IterTag> class MakeCpRangeFromUtf16Impl; 
}

template <typename Int8Iter>
auto makeCpRangeFromUtf8(std::pair<Int8Iter, Int8Iter> const& range) 
    -> decltype(Internal::MakeCpRangeFromUtf8Impl<Int8Iter, typename std::iterator_traits<Int8Iter>::iterator_category>()(range.first, range.second));

template <typename Int16Iter>
auto makeCpRangeFromUtf16(std::pair<Int16Iter, Int16Iter> const& range) 
    -> decltype(Internal::MakeCpRangeFromUtf16Impl<Int16Iter, typename std::iterator_traits<Int16Iter>::iterator_category>()(range.first, range.second));




//**** **** **** **** 
//** Public APIs - convert a range of UnicodeCodePoints to utf

template <typename CpIter, UtfKind utfKind>
class BasicCpToUtfInputIterator;

template <typename CpIter, UtfKind utfKind>
class BasicCpToUtfBidiIterator;

template <typename CpIter>
using CpToUtf8InputIterator = BasicCpToUtfInputIterator<CpIter, Utf8Kind>; 

template <typename CpIter>
using CpToUtf8BidiIterator = BasicCpToUtfBidiIterator<CpIter, Utf8Kind>; 

template <typename CpIter>
using CpToUtf16InputIterator = BasicCpToUtfInputIterator<CpIter, Utf16Kind>; 

template <typename CpIter>
using CpToUtf16BidiIterator = BasicCpToUtfBidiIterator<CpIter, Utf16Kind>; 

template <typename CpIter, UtfKind utfKind>
bool operator== (BasicCpToUtfInputIterator<CpIter, utfKind> const& a, BasicCpToUtfInputIterator<CpIter, utfKind> const& b);

template <typename CpIter, UtfKind utfKind>
bool operator== (BasicCpToUtfBidiIterator<CpIter, utfKind> const& a, BasicCpToUtfBidiIterator<CpIter, utfKind> const& b);

template <typename CpIter, UtfKind utfKind>
bool operator!= (BasicCpToUtfInputIterator<CpIter, utfKind> const& a, BasicCpToUtfInputIterator<CpIter, utfKind> const& b);

template <typename CpIter, UtfKind utfKind>
bool operator!= (BasicCpToUtfBidiIterator<CpIter, utfKind> const& a, BasicCpToUtfBidiIterator<CpIter, utfKind> const& b);

template <typename CpIter, UtfKind utfKind>
class BasicCpToUtfInputIterator
{
public:
    static_assert(sizeof(**(CpIter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
public:
    typedef std::input_iterator_tag                                 iterator_category;
    typedef typename Internal::UtfEncodingUnit<utfKind>::T          value_type; //Utf8EncodingUnit or Utf16EncodingUnit
    typedef typename std::iterator_traits<value_type*>::difference_type   difference_type;
    typedef value_type const*                                       pointer;
    typedef value_type const&                                       reference;
public:
    struct Proxy { value_type c; value_type operator* () const { return c; } };
public:
    BasicCpToUtfInputIterator(); //creates a "one-past-the-end" value
    BasicCpToUtfInputIterator(CpIter const& current, CpIter const& end); //"end" is used for bounds-checking
    BasicCpToUtfInputIterator&  operator++ (); //prefix
    Proxy                       operator++ (int); //postfix
    value_type                  operator* () const; 
    CpIter                      getIter() const { return current; }

    friend bool operator== <> (BasicCpToUtfInputIterator<CpIter, utfKind> const& a, BasicCpToUtfInputIterator<CpIter, utfKind> const& b);
    friend bool operator!= <> (BasicCpToUtfInputIterator<CpIter, utfKind> const& a, BasicCpToUtfInputIterator<CpIter, utfKind> const& b);

private:
    CpIter current;
    CpIter end;
    value_type buf[4];
    int buflen; 
    int bufpos; 
};

template <typename CpIter, UtfKind utfKind>
class BasicCpToUtfBidiIterator
{
public:
    static_assert(sizeof(**(CpIter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
public:
    typedef std::bidirectional_iterator_tag                             iterator_category;
    typedef typename Internal::UtfEncodingUnit<utfKind>::T              value_type; //Utf8EncodingUnit or Utf16EncodingUnit
    typedef typename std::iterator_traits<value_type*>::difference_type   difference_type;
    typedef value_type const*                                           pointer;
    typedef value_type const&                                           reference;
public:
    BasicCpToUtfBidiIterator();
    BasicCpToUtfBidiIterator(CpIter const& begin, CpIter const& current, CpIter const& end);
    BasicCpToUtfBidiIterator&  operator++ (); //prefix
    BasicCpToUtfBidiIterator   operator++ (int); //postfix
    BasicCpToUtfBidiIterator&  operator-- (); //prefix
    BasicCpToUtfBidiIterator   operator-- (int); //postfix
    value_type                 operator* () const;
    CpIter                     getIter() const { return current; }

    friend bool operator== <> (BasicCpToUtfBidiIterator<CpIter, utfKind> const& a, BasicCpToUtfBidiIterator<CpIter, utfKind> const& b);
    friend bool operator!= <> (BasicCpToUtfBidiIterator<CpIter, utfKind> const& a, BasicCpToUtfBidiIterator<CpIter, utfKind> const& b);

private:
    CpIter begin;
    CpIter current;
    CpIter end;
    value_type buf[4]; 
    int bufpos; 
    int buflen; 
};

namespace Internal
{   //Private Implementation Details
    template <typename Int8Iter, typename IterTag> class MakeUtf8RangeFromCpRangeImpl; 
    template <typename Int16Iter, typename IterTag> class MakeUtf16RangeFromCpRangeImpl; 
}

template <typename CpIter>
auto makeUtf8RangeFromCpRange(std::pair<CpIter, CpIter> const& range) 
    -> decltype(Internal::MakeUtf8RangeFromCpRangeImpl<CpIter, typename std::iterator_traits<CpIter>::iterator_category>()(range.first, range.second));

template <typename CpIter>
auto makeUtf16RangeFromCpRange(std::pair<CpIter, CpIter> const& range) 
    -> decltype(Internal::MakeUtf16RangeFromCpRangeImpl<CpIter, typename std::iterator_traits<CpIter>::iterator_category>()(range.first, range.second));




//**** **** **** **** 
//** Private Implementations

namespace Internal
{   
    template <typename Int8Iter, typename IterTag>
    class MakeCpRangeFromUtf8Impl 
    { 
    public:
        std::pair<Utf8ToCpBidiIterator<Int8Iter>, Utf8ToCpBidiIterator<Int8Iter> >
            operator() 
            (Int8Iter const& rangeFirst, Int8Iter const& rangeSecond) const
        {
            std::pair<Utf8ToCpBidiIterator<Int8Iter>, Utf8ToCpBidiIterator<Int8Iter> > x;
            x.first = Utf8ToCpBidiIterator<Int8Iter>(rangeFirst, rangeFirst, rangeSecond); 
            x.second = Utf8ToCpBidiIterator<Int8Iter>(rangeFirst, rangeSecond, rangeSecond); 
            return x; 
        }
    };

    template <typename Int8Iter>
    class MakeCpRangeFromUtf8Impl <Int8Iter, std::input_iterator_tag>
    { 
    public:
        std::pair<Utf8ToCpInputIterator<Int8Iter>, Utf8ToCpInputIterator<Int8Iter> >
            operator() 
            (Int8Iter const& rangeFirst, Int8Iter const& rangeSecond) const
        {
            std::pair<Utf8ToCpInputIterator<Int8Iter>, Utf8ToCpInputIterator<Int8Iter> > x;
            x.first = Utf8ToCpInputIterator<Int8Iter>(rangeFirst, rangeSecond); 
            x.second = Utf8ToCpInputIterator<Int8Iter>(rangeSecond, rangeSecond); 
            return x; 
        }
    };
}

template <typename Int8Iter>
auto makeCpRangeFromUtf8(std::pair<Int8Iter, Int8Iter> const& range) 
    -> decltype(Internal::MakeCpRangeFromUtf8Impl<Int8Iter, typename std::iterator_traits<Int8Iter>::iterator_category>()(range.first, range.second))
{
    static_assert(sizeof(**(Int8Iter*)0) == sizeof(char), "Invalid Template Type Parameter"); 
    return Internal::MakeCpRangeFromUtf8Impl<Int8Iter, typename std::iterator_traits<Int8Iter>::iterator_category>()(range.first, range.second); 
}

namespace Internal
{
    template <typename Int16Iter, typename IterTag>
    class MakeCpRangeFromUtf16Impl 
    { 
    public:
        std::pair<Utf16ToCpBidiIterator<Int16Iter>, Utf16ToCpBidiIterator<Int16Iter> >
            operator() 
            (Int16Iter const& rangeFirst, Int16Iter const& rangeSecond) const
        {
            std::pair<Utf16ToCpBidiIterator<Int16Iter>, Utf16ToCpBidiIterator<Int16Iter> > x;
            x.first = Utf16ToCpBidiIterator<Int16Iter>(rangeFirst, rangeFirst, rangeSecond); 
            x.second = Utf16ToCpBidiIterator<Int16Iter>(rangeFirst, rangeSecond, rangeSecond); 
            return x; 
        }
    };

    template <typename Int16Iter>
    class MakeCpRangeFromUtf16Impl <Int16Iter, std::input_iterator_tag>
    { 
    public:
        std::pair<Utf16ToCpInputIterator<Int16Iter>, Utf16ToCpInputIterator<Int16Iter> >
            operator() 
            (Int16Iter const& rangeFirst, Int16Iter const& rangeSecond) const
        {
            std::pair<Utf16ToCpInputIterator<Int16Iter>, Utf16ToCpInputIterator<Int16Iter> > x;
            x.first = Utf16ToCpInputIterator<Int16Iter>(rangeFirst, rangeSecond); 
            x.second = Utf16ToCpInputIterator<Int16Iter>(rangeSecond, rangeSecond); 
            return x; 
        }
    };
}

template <typename Int16Iter>
auto makeCpRangeFromUtf16(std::pair<Int16Iter, Int16Iter> const& range) 
    -> decltype(Internal::MakeCpRangeFromUtf16Impl<Int16Iter, typename std::iterator_traits<Int16Iter>::iterator_category>()(range.first, range.second))
{
    static_assert(sizeof(**(Int16Iter*)0) == sizeof(Utf16EncodingUnit), "Invalid Template Type Parameter"); 
    return Internal::MakeCpRangeFromUtf16Impl<Int16Iter, typename std::iterator_traits<Int16Iter>::iterator_category>()(range.first, range.second); 
}

namespace Internal
{
    template <typename CpIter, typename IterTag>
    class MakeUtf8RangeFromCpRangeImpl
    { 
    public:
        std::pair<CpToUtf8BidiIterator<CpIter>, CpToUtf8BidiIterator<CpIter> >
            operator() 
            (CpIter const& rangeFirst, CpIter const& rangeSecond) const
        {
            std::pair<CpToUtf8BidiIterator<CpIter>, CpToUtf8BidiIterator<CpIter> > x;
            x.first = CpToUtf8BidiIterator<CpIter>(rangeFirst, rangeFirst, rangeSecond); 
            x.second = CpToUtf8BidiIterator<CpIter>(rangeFirst, rangeSecond, rangeSecond); 
            return x; 
        }
    };

    template <typename CpIter>
    class MakeUtf8RangeFromCpRangeImpl <CpIter, std::input_iterator_tag>
    { 
    public:
        std::pair<CpToUtf8InputIterator<CpIter>, CpToUtf8InputIterator<CpIter> >
            operator() 
            (CpIter const& rangeFirst, CpIter const& rangeSecond) const
        {
            std::pair<CpToUtf8InputIterator<CpIter>, CpToUtf8InputIterator<CpIter> > x;
            x.first = CpToUtf8InputIterator<CpIter>(rangeFirst, rangeSecond); 
            x.second = CpToUtf8InputIterator<CpIter>(rangeSecond, rangeSecond); 
            return x; 
        }
    };
}

template <typename CpIter>
auto makeUtf8RangeFromCpRange(std::pair<CpIter, CpIter> const& range) 
    -> decltype(Internal::MakeUtf8RangeFromCpRangeImpl<CpIter, typename std::iterator_traits<CpIter>::iterator_category>()(range.first, range.second))
{
    static_assert(sizeof(**(CpIter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
    return Internal::MakeUtf8RangeFromCpRangeImpl<CpIter, typename std::iterator_traits<CpIter>::iterator_category>()(range.first, range.second); 
}

namespace Internal
{
    template <typename CpIter, typename IterTag>
    class MakeUtf16RangeFromCpRangeImpl
    { 
    public:
        std::pair<CpToUtf16BidiIterator<CpIter>, CpToUtf16BidiIterator<CpIter> >
            operator() 
            (CpIter const& rangeFirst, CpIter const& rangeSecond) const
        {
            std::pair<CpToUtf16BidiIterator<CpIter>, CpToUtf16BidiIterator<CpIter> > x;
            x.first = CpToUtf16BidiIterator<CpIter>(rangeFirst, rangeFirst, rangeSecond); 
            x.second = CpToUtf16BidiIterator<CpIter>(rangeFirst, rangeSecond, rangeSecond); 
            return x; 
        }
    };

    template <typename CpIter>
    class MakeUtf16RangeFromCpRangeImpl <CpIter, std::input_iterator_tag>
    { 
    public:
        std::pair<CpToUtf16InputIterator<CpIter>, CpToUtf16InputIterator<CpIter> >
            operator() 
            (CpIter const& rangeFirst, CpIter const& rangeSecond) const
        {
            std::pair<CpToUtf16InputIterator<CpIter>, CpToUtf16InputIterator<CpIter> > x;
            x.first = CpToUtf16InputIterator<CpIter>(rangeFirst, rangeSecond); 
            x.second = CpToUtf16InputIterator<CpIter>(rangeSecond, rangeSecond); 
            return x; 
        }
    };
}

template <typename CpIter>
auto makeUtf16RangeFromCpRange(std::pair<CpIter, CpIter> const& range) 
    -> decltype(Internal::MakeUtf16RangeFromCpRangeImpl<CpIter, typename std::iterator_traits<CpIter>::iterator_category>()(range.first, range.second))
{
    static_assert(sizeof(**(CpIter*)0) == sizeof(UnicodeCodePoint), "Invalid Template Type Parameter"); 
    return Internal::MakeUtf16RangeFromCpRangeImpl<CpIter, typename std::iterator_traits<CpIter>::iterator_category>()(range.first, range.second); 
}




namespace Internal
{
    template <typename Iter, UtfKind utfKind> 
    struct ReadUtfForwardUtil {};

    template <typename Iter> 
    struct ReadUtfForwardUtil <Iter, Utf8Kind>
    { UnicodeCodePoint operator() (Iter & current, Iter const end) const { return readUtf8Forward(current, end); } };

    template <typename Iter> 
    struct ReadUtfForwardUtil <Iter, Utf16Kind>
    { UnicodeCodePoint operator() (Iter & current, Iter const end) const { return readUtf16Forward(current, end); } };


    template <typename Iter, UtfKind utfKind> 
    struct ReadUtfBackwardUtil {};

    template <typename Iter> 
    struct ReadUtfBackwardUtil <Iter, Utf8Kind>
    { UnicodeCodePoint operator() (Iter const begin, Iter & current) const { return readUtf8Backward(begin, current); } };
    
    template <typename Iter> 
    struct ReadUtfBackwardUtil <Iter, Utf16Kind>
    { UnicodeCodePoint operator() (Iter const begin, Iter & current) const { return readUtf16Backward(begin, current); } };


    template <UtfKind utfKind> 
    struct WriteUtfUtil {};

    template <> 
    struct WriteUtfUtil <Utf8Kind>
    { Utf8Sequence operator() (UnicodeCodePoint cp) const { return writeUtf8(cp); } };

    template <> 
    struct WriteUtfUtil <Utf16Kind>
    { Utf16Sequence operator() (UnicodeCodePoint cp) const { return writeUtf16(cp); } };
}




template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind>::BasicUtfToCpInputIterator() 
    : current(), end(), cp(static_cast<UnicodeCodePoint>(-1)) 
{
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind>::BasicUtfToCpInputIterator(
    EncodingUnitIterator const& current_, EncodingUnitIterator const& end_) 
    : current(current_), end(end_), cp(static_cast<UnicodeCodePoint>(-1)) 
{
    cp = Internal::ReadUtfForwardUtil<EncodingUnitIterator, utfKind>()(current, end); 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> &  
    BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind>::operator++ () //prefix
{
    cp = Internal::ReadUtfForwardUtil<EncodingUnitIterator, utfKind>()(current, end); 
    return *this; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
typename BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind>::Proxy  
    BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind>::operator++ (int ) //postfix
{
    Proxy proxy = { cp }; 
    cp = Internal::ReadUtfForwardUtil<EncodingUnitIterator, utfKind>()(current, end); 
    return proxy; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
UnicodeCodePoint  BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind>::operator* () const
{
    return cp; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator== (
    BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& a, 
    BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& b)
{
    return a.cp == static_cast<UnicodeCodePoint>(-1) && b.cp == static_cast<UnicodeCodePoint>(-1);
}

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator!= (
    BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& a, 
    BasicUtfToCpInputIterator<EncodingUnitIterator, utfKind> const& b)
{
    return ! (a == b); 
}




template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::BasicUtfToCpBidiIterator()
    : begin(), current(), end()
{
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::BasicUtfToCpBidiIterator(
    EncodingUnitIterator const& begin_, EncodingUnitIterator const& current_, EncodingUnitIterator const& end_)
    : begin(begin_), current(current_), end(end_)
{
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> &  
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::operator++ () //prefix
{
    Internal::ReadUtfForwardUtil<EncodingUnitIterator, utfKind>()(current, end); 
    return *this; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>  
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::operator++ (int ) //postifx
{
    BasicUtfToCpBidiIterator tmp(*this); 
    ++*this; 
    return tmp; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> & 
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::operator-- () //prefix
{
    Internal::ReadUtfBackwardUtil<EncodingUnitIterator, utfKind>()(begin, current); 
    return *this; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> 
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::operator-- (int ) //postfix
{
    BasicUtfToCpBidiIterator tmp(*this); 
    --*this; 
    return tmp; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
UnicodeCodePoint  BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind>::operator* () const
{
    EncodingUnitIterator tmp = current;
    return Internal::ReadUtfForwardUtil<EncodingUnitIterator, utfKind>()(tmp, end); 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator== (
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& a, 
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& b)
{
    return a.current == b.current; 
}

template <typename EncodingUnitIterator, UtfKind utfKind>
bool operator!= (
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& a, 
    BasicUtfToCpBidiIterator<EncodingUnitIterator, utfKind> const& b)
{
    return ! (a == b); 
}




template <typename CpIter, UtfKind utfKind>
BasicCpToUtfInputIterator<CpIter, utfKind>::BasicCpToUtfInputIterator()
    : current(), end(), buflen(0), bufpos(0)
{
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfInputIterator<CpIter, utfKind>::BasicCpToUtfInputIterator(
    CpIter const& current_, CpIter const& end_)
    : current(current_), end(end_), buflen(0), bufpos(0)
{
    if (current == end)
    {   current = CpIter();
        end = CpIter();
        return; 
    }

    UnicodeCodePoint cp = *current; 
    ++current; 
    auto x = Internal::WriteUtfUtil<utfKind>()(cp); 
    if (x.len < 0)
        throw std::runtime_error("BasicCpToUtfInputIterator read invalid Unicode code point."); 
    buf[0] = buf[1] = buf[2] = buf[3] = 0; 
    for (int i = 0; i < x.len; ++i)
        buf[i] = x.seq[i]; 
    buflen = x.len; 
    bufpos = 0; 
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfInputIterator<CpIter, utfKind> & 
    BasicCpToUtfInputIterator<CpIter, utfKind>::operator++ () //prefix
{
    if (bufpos == buflen && current == end)
        abort(); 

    if (bufpos != buflen)
    {   ++bufpos; 
        if (bufpos != buflen)
            return *this; 
    }

    if (current == end)
    {   current = CpIter();
        end = CpIter();
        bufpos = 0;
        buflen = 0;
        return *this; 
    }

    UnicodeCodePoint cp = *current; 
    ++current; 
    auto x = Internal::WriteUtfUtil<utfKind>()(cp); 
    if (x.len < 0)
        throw std::runtime_error("BasicCpToUtfInputIterator read invalid Unicode code point."); 
    buf[0] = buf[1] = buf[2] = buf[3] = 0; 
    for (int i = 0; i < x.len; ++i)
        buf[i] = x.seq[i]; 
    buflen = x.len; 
    bufpos = 0; 

    return *this; 
}

template <typename CpIter, UtfKind utfKind>
typename BasicCpToUtfInputIterator<CpIter, utfKind>::value_type
    BasicCpToUtfInputIterator<CpIter, utfKind>::operator* () const
{
    return buf[bufpos]; 
}

template <typename CpIter, UtfKind utfKind>
bool operator== (
    BasicCpToUtfInputIterator<CpIter, utfKind> const& a, 
    BasicCpToUtfInputIterator<CpIter, utfKind> const& b)
{
    return a.current == CpIter() && b.current == CpIter() && a.bufpos == b.bufpos && a.buflen == b.buflen;
}

template <typename CpIter, UtfKind utfKind>
bool operator!= (
    BasicCpToUtfInputIterator<CpIter, utfKind> const& a, 
    BasicCpToUtfInputIterator<CpIter, utfKind> const& b)
{
    return ! (a == b); 
}




template <typename CpIter, UtfKind utfKind>
BasicCpToUtfBidiIterator<CpIter, utfKind>::BasicCpToUtfBidiIterator()
    : begin(), current(), end(), bufpos(0), buflen(0)
{
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfBidiIterator<CpIter, utfKind>::BasicCpToUtfBidiIterator(
    CpIter const& begin_, CpIter const& current_, CpIter const& end_)
    : begin(begin_), current(current_), end(end_), bufpos(0), buflen(0)
{
    if (current == end)
        return; 
    
    UnicodeCodePoint cp = *current; 
    ++current; 
    auto x = Internal::WriteUtfUtil<utfKind>()(cp); 
    if (x.len < 0)
        throw std::runtime_error("BasicCpToUtfBidiIterator read invalid Unicode code point."); 
    buf[0] = buf[1] = buf[2] = buf[3] = 0; 
    for (int i = 0; i < x.len; ++i)
        buf[i] = x.seq[i]; 
    buflen = x.len; 
    bufpos = 0; 
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfBidiIterator<CpIter, utfKind> & 
    BasicCpToUtfBidiIterator<CpIter, utfKind>::operator++ () //prefix
{
    if (bufpos == buflen && current == end)
        abort(); 

    if (bufpos != buflen)
    {   ++bufpos;
        if (bufpos != buflen)
            return *this;
    }
    
    if (current == end)
    {   bufpos = 0; 
        buflen = 0; 
        return *this; 
    }
    
    UnicodeCodePoint cp = *current; 
    ++current; 
    auto x = Internal::WriteUtfUtil<utfKind>()(cp); 
    if (x.len < 0)
        throw std::runtime_error("BasicCpToUtfBidiIterator read invalid Unicode code point."); 
    buf[0] = buf[1] = buf[2] = buf[3] = 0; 
    for (int i = 0; i < x.len; ++i)
        buf[i] = x.seq[i]; 
    buflen = x.len; 
    bufpos = 0; 
    return *this; 
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfBidiIterator<CpIter, utfKind>
    BasicCpToUtfBidiIterator<CpIter, utfKind>::operator++ (int ) //postfix
{
    BasicCpToUtfBidiIterator tmp = *this;
    ++*this;
    return tmp; 
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfBidiIterator<CpIter, utfKind> & 
    BasicCpToUtfBidiIterator<CpIter, utfKind>::operator-- () //prefix
{
    if (bufpos == 0 && begin == current)
        abort(); 
    if (bufpos > 0)
    {   --bufpos;
        return *this; 
    }
    --current; 
    UnicodeCodePoint cp = *current; 
    auto x = Internal::WriteUtfUtil<utfKind>()(cp); 
    if (x.len < 0)
        throw std::runtime_error("BasicCpToUtfBidiIterator read invalid Unicode code point."); 
    buf[0] = buf[1] = buf[2] = buf[3] = 0; 
    for (int i = 0; i < x.len; ++i)
        buf[i] = x.seq[i]; 
    buflen = x.len; 
    bufpos = x.len - 1; 
}

template <typename CpIter, UtfKind utfKind>
BasicCpToUtfBidiIterator<CpIter, utfKind>
    BasicCpToUtfBidiIterator<CpIter, utfKind>::operator-- (int ) //postfix
{
    BasicCpToUtfBidiIterator tmp = *this;
    --*this;
    return tmp; 
}

template <typename CpIter, UtfKind utfKind>
typename BasicCpToUtfBidiIterator<CpIter, utfKind>::value_type
    BasicCpToUtfBidiIterator<CpIter, utfKind>::operator* () const
{
    return buf[bufpos]; 
}

template <typename CpIter, UtfKind utfKind>
bool operator== (
    BasicCpToUtfBidiIterator<CpIter, utfKind> const& a, 
    BasicCpToUtfBidiIterator<CpIter, utfKind> const& b)
{
    return a.current == b.current && a.bufpos == b.bufpos && a.buflen == b.buflen; 
}

template <typename CpIter, UtfKind utfKind>
bool operator!= (
    BasicCpToUtfBidiIterator<CpIter, utfKind> const& a, 
    BasicCpToUtfBidiIterator<CpIter, utfKind> const& b)
{
    return ! (a == b); 
}


} //namespace jjm

#endif
