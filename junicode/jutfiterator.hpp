// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUTFITERATOR_HPP_HEADER_GUARD
#define JUNICODE_JUTFITERATOR_HPP_HEADER_GUARD

#include "jutfutils.hpp"
#include "jbase/jstaticassert.hpp"
#include <iterator>


namespace jjm
{

template <typename Int8Iter>
class Utf8ToCpInputIterator;
template <typename Int16Iter>
class Utf16ToCpInputIterator;

template <typename Int8Iter>
class Utf8ToCpBidiIterator;
template <typename Int16Iter>
class Utf16ToCpBidiIterator;

template <typename Int8Iter>
bool operator== (Utf8ToCpInputIterator<Int8Iter> const& a, Utf8ToCpInputIterator<Int8Iter> const& b);
template <typename Int8Iter>
bool operator!= (Utf8ToCpInputIterator<Int8Iter> const& a, Utf8ToCpInputIterator<Int8Iter> const& b);

template <typename Int16Iter>
bool operator== (Utf16ToCpInputIterator<Int16Iter> const& a, Utf16ToCpInputIterator<Int16Iter> const& b);
template <typename Int16Iter>
bool operator!= (Utf16ToCpInputIterator<Int16Iter> const& a, Utf16ToCpInputIterator<Int16Iter> const& b);

template <typename Int8Iter>
bool operator== (Utf8ToCpBidiIterator<Int8Iter> const& a, Utf8ToCpBidiIterator<Int8Iter> const& b);
template <typename Int8Iter>
bool operator!= (Utf8ToCpBidiIterator<Int8Iter> const& a, Utf8ToCpBidiIterator<Int8Iter> const& b);

template <typename Int16Iter>
bool operator== (Utf16ToCpBidiIterator<Int16Iter> const& a, Utf16ToCpBidiIterator<Int16Iter> const& b);
template <typename Int16Iter>
bool operator!= (Utf16ToCpBidiIterator<Int16Iter> const& a, Utf16ToCpBidiIterator<Int16Iter> const& b);


//Requires that the default constructed Int8Iter() be a stable, single 
//value that can be used to indicate "one-past-the-end" value. 
template <typename Int8Iter>
class Utf8ToCpInputIterator
{
public:
    typedef std::input_iterator_tag     iterator_category;
    typedef CodePoint                   value_type;
    typedef typename std::iterator_traits<Int8Iter>::difference_type    difference_type;
    typedef CodePoint const*            pointer;
    typedef CodePoint const&            reference;
public:
    struct Proxy { CodePoint cp; CodePoint operator* () const { return cp; } };
public:
    Utf8ToCpInputIterator(); //creates a "one-past-the-end" value
    Utf8ToCpInputIterator(Int8Iter current, Int8Iter end); //"end" is used for bounds-checking
    Utf8ToCpInputIterator&      operator++ (); //prefix
    Proxy               operator++ (int); //postfix
    value_type          operator* (); //returns the next code-point
    Int8Iter    getIter() const { return current; }

    friend bool operator== <> (Utf8ToCpInputIterator<Int8Iter> const& a, Utf8ToCpInputIterator<Int8Iter> const& b);
    friend bool operator!= <> (Utf8ToCpInputIterator<Int8Iter> const& a, Utf8ToCpInputIterator<Int8Iter> const& b);

private:
    Int8Iter current;
    Int8Iter end;
    CodePoint cp;
};

template <typename Iter>
Utf8ToCpInputIterator<Iter> makeUtf8ToCpInputIterator(Iter current, Iter end)
{
    JSTATICASSERT(sizeof(*current) == 1); 
    return Utf8ToCpInputIterator<Iter>(current, end); 
}

template <typename Iter>
std::pair<Utf8ToCpInputIterator<Iter>, Utf8ToCpInputIterator<Iter> >
makeUtf8ToCpRange(Iter current, Iter end)
{
    JSTATICASSERT(sizeof(*current) == 1); 
    std::pair<Utf8ToCpInputIterator<Iter>, Utf8ToCpInputIterator<Iter> > x;
    x.first  = Utf8ToCpInputIterator<Iter>(current, end); 
    x.second = Utf8ToCpInputIterator<Iter>(end    , end); 
    return x; 
}

template <typename Iter>
std::pair<Utf8ToCpInputIterator<Iter>, Utf8ToCpInputIterator<Iter> >
makeUtf8ToCpRange(std::pair<Iter, Iter> & range)
{
    JSTATICASSERT(sizeof(*(range.first)) == 1); 
    std::pair<Utf8ToCpInputIterator<Iter>, Utf8ToCpInputIterator<Iter> > x;
    x.first  = Utf8ToCpInputIterator<Iter>(range.first,  range.second); 
    x.second = Utf8ToCpInputIterator<Iter>(range.second, range.second); 
    return x; 
}


//Requires that the default constructed Int16Iter() be a stable, single 
//value that can be used to indicate "one-past-the-end" value. 
template <typename Int16Iter>
class Utf16ToCpInputIterator
{
public:
    typedef std::input_iterator_tag     iterator_category;
    typedef CodePoint                   value_type;
    typedef typename std::iterator_traits<Int16Iter>::difference_type    difference_type;
    typedef CodePoint const*            pointer;
    typedef CodePoint const&            reference;
public:
    struct Proxy { CodePoint cp; CodePoint operator* () const { return cp; } };
public:
    Utf16ToCpInputIterator(); //creates a "one-past-the-end" value
    Utf16ToCpInputIterator(Int16Iter current, Int16Iter end); //"end" is used for bounds-checking
    Utf16ToCpInputIterator&     operator++ (); //prefix
    Proxy                   operator++ (int); //postfix
    value_type              operator* (); //returns the next code-point
    Int16Iter       getIter() const { return current; }

    friend bool operator== <> (Utf16ToCpInputIterator<Int16Iter> const& a, Utf16ToCpInputIterator<Int16Iter> const& b);
    friend bool operator!= <> (Utf16ToCpInputIterator<Int16Iter> const& a, Utf16ToCpInputIterator<Int16Iter> const& b);

private:
    Int16Iter current;
    Int16Iter end;
    CodePoint cp;
};

template <typename Iter>
Utf16ToCpInputIterator<Iter> makeUtf16ToCpInputIterator(Iter current, Iter end)
{
    JSTATICASSERT(sizeof(*current) == 2); 
    return Utf16ToCpInputIterator<Iter>(current, end); 
}

template <typename Iter>
std::pair<Utf16ToCpInputIterator<Iter>, Utf16ToCpInputIterator<Iter> >
makeUtf16ToCpRange(Iter current, Iter end)
{
    JSTATICASSERT(sizeof(*(current)) == 2); 
    std::pair<Utf16ToCpInputIterator<Iter>, Utf16ToCpInputIterator<Iter> > x;
    x.first  = Utf16ToCpInputIterator<Iter>(current, end); 
    x.second = Utf16ToCpInputIterator<Iter>(end    , end); 
    return x; 
}

template <typename Iter>
std::pair<Utf16ToCpInputIterator<Iter>, Utf16ToCpInputIterator<Iter> >
makeUtf16ToCpRange(std::pair<Iter, Iter> & range)
{
    JSTATICASSERT(sizeof(*(range.first)) == 2); 
    std::pair<Utf16ToCpInputIterator<Iter>, Utf16ToCpInputIterator<Iter> > x;
    x.first  = Utf16ToCpInputIterator<Iter>(range.first,  range.second); 
    x.second = Utf16ToCpInputIterator<Iter>(range.second, range.second); 
    return x; 
}


template <typename Int8Iter>
class Utf8ToCpBidiIterator
{
public:
    typedef std::bidirectional_iterator_tag  iterator_category;
    typedef CodePoint  value_type;
    typedef typename std::iterator_traits<Int8Iter>::difference_type  difference_type;
    typedef CodePoint const*    pointer;
    typedef CodePoint const&    reference;

public:
    Utf8ToCpBidiIterator(); 
    Utf8ToCpBidiIterator(Int8Iter begin, Int8Iter current, Int8Iter end); //"begin" and "end" are used for bounds-checking
    Utf8ToCpBidiIterator&   operator++ (); //prefix
    Utf8ToCpBidiIterator    operator++ (int); //postfix
    Utf8ToCpBidiIterator&   operator-- (); //prefix
    Utf8ToCpBidiIterator    operator-- (int); //postfix
    value_type          operator* () const;
    Int8Iter    getIter() const { return current; }

    friend bool operator== <> (Utf8ToCpBidiIterator<Int8Iter> const& a, Utf8ToCpBidiIterator<Int8Iter> const& b);
    friend bool operator!= <> (Utf8ToCpBidiIterator<Int8Iter> const& a, Utf8ToCpBidiIterator<Int8Iter> const& b);

private:
    Int8Iter begin;
    Int8Iter current;
    Int8Iter end;
};


template <typename Int16Iter>
class Utf16ToCpBidiIterator
{
public:
    typedef std::bidirectional_iterator_tag     iterator_category;
    typedef CodePoint                           value_type;
    typedef typename std::iterator_traits<Int16Iter>::difference_type    difference_type;
    typedef CodePoint const*                    pointer;
    typedef CodePoint const&                    reference;

public:
    Utf16ToCpBidiIterator();
    Utf16ToCpBidiIterator(Int16Iter begin, Int16Iter current, Int16Iter end);
    Utf16ToCpBidiIterator&  operator++ (); //prefix
    Utf16ToCpBidiIterator   operator++ (int); //postfix
    Utf16ToCpBidiIterator&  operator-- (); //prefix
    Utf16ToCpBidiIterator   operator-- (int); //postfix
    CodePoint           operator* () const;
    Int16Iter   getIter() const { return current; }

    friend bool operator== <> (Utf16ToCpBidiIterator<Int16Iter> const& a, Utf16ToCpBidiIterator<Int16Iter> const& b);
    friend bool operator!= <> (Utf16ToCpBidiIterator<Int16Iter> const& a, Utf16ToCpBidiIterator<Int16Iter> const& b);

private:
    Int16Iter begin;
    Int16Iter current;
    Int16Iter end;
};


// ---- ---- ---- ---- 
// impl: 

template <typename Int8Iter>
Utf8ToCpInputIterator<Int8Iter>::Utf8ToCpInputIterator()
    : current(), end(), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Int8Iter>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);

    JSTATICASSERT(sizeof(*current) == 1); 
}

template <typename Int8Iter>
Utf8ToCpInputIterator<Int8Iter>::Utf8ToCpInputIterator(Int8Iter current_, Int8Iter end_)
    : current(current_), end(end_), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Int8Iter>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);

    if (current == end)
    {   //convert this object to a "one-past-the-end" value
        current = Int8Iter();
        end = Int8Iter();
        cp = static_cast<CodePoint>(-1); 
    }else
    {   cp = jjm::readUtf8Forward(current, end);
    }
}

template <typename Int8Iter>
Utf8ToCpInputIterator<Int8Iter>& Utf8ToCpInputIterator<Int8Iter>::operator++ () //prefix
{
    if (current == end)
    {   //convert this object to a "one-past-the-end" value
        current = Int8Iter();
        end = Int8Iter();
        cp = static_cast<CodePoint>(-1); 
        return *this; 
    }

    cp = jjm::readUtf8Forward(current, end);
    return *this; 
}

template <typename Int8Iter>
typename Utf8ToCpInputIterator<Int8Iter>::Proxy Utf8ToCpInputIterator<Int8Iter>::operator++ (int) //postfix
{
    Proxy p = { this->operator*() };
    ++*this;
    return p;
}

template <typename Int8Iter>
typename Utf8ToCpInputIterator<Int8Iter>::value_type Utf8ToCpInputIterator<Int8Iter>::operator* ()
{
    return cp; 
}

template <typename Int8Iter>
bool operator== (Utf8ToCpInputIterator<Int8Iter> const& a, Utf8ToCpInputIterator<Int8Iter> const& b)
{
    return a.cp == static_cast<CodePoint>(-1) && b.cp == static_cast<CodePoint>(-1);
}

template <typename Int8Iter>
bool operator!= (Utf8ToCpInputIterator<Int8Iter> const& a, Utf8ToCpInputIterator<Int8Iter> const& b)
{
    return !(a == b);
}

template <typename Int8Iter>
Utf16ToCpInputIterator<Int8Iter>::Utf16ToCpInputIterator()
    : current(), end(), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Int8Iter>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);

    JSTATICASSERT(sizeof(*current) == 2); 
}

template <typename Int16Iter>
Utf16ToCpInputIterator<Int16Iter>::Utf16ToCpInputIterator(Int16Iter current_, Int16Iter end_)
    : current(current_), end(end_), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Int16Iter>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);
    
    if (current == end)
    {   //convert this object to a "one-past-the-end" value
        current = Int16Iter();
        end = Int16Iter();
    }else
    {   cp = jjm::readUtf16Forward(current, end);
    }
}

template <typename Int16Iter>
Utf16ToCpInputIterator<Int16Iter>& Utf16ToCpInputIterator<Int16Iter>::operator++ () //prefix
{
    if (current == end)
    {   //convert this object to a "one-past-the-end" value
        current = Int16Iter();
        end = Int16Iter();
        cp = static_cast<CodePoint>(-1); 
        return *this; 
    }

    cp = jjm::readUtf16Forward(current, end);
    return *this; 
}

template <typename Int8Iter>
typename Utf16ToCpInputIterator<Int8Iter>::Proxy Utf16ToCpInputIterator<Int8Iter>::operator++ (int) //postfix
{
    Proxy p = { **this };
    ++*this;
    return p;
}

template <typename Int8Iter>
typename Utf16ToCpInputIterator<Int8Iter>::value_type Utf16ToCpInputIterator<Int8Iter>::operator* ()
{
    return cp;
}

template <typename Int8Iter>
bool operator== (Utf16ToCpInputIterator<Int8Iter> const& a, Utf16ToCpInputIterator<Int8Iter> const& b)
{
    return a.cp == static_cast<CodePoint>(-1) && b.cp == static_cast<CodePoint>(-1);
}

template <typename Int8Iter>
bool operator!= (Utf16ToCpInputIterator<Int8Iter> const& a, Utf16ToCpInputIterator<Int8Iter> const& b)
{
    return !(a == b);
}

template <typename Int8Iter>
Utf8ToCpBidiIterator<Int8Iter>::Utf8ToCpBidiIterator()
    : begin(), current(), end() 
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Int8Iter>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Int8Iter>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Int8Iter>
Utf8ToCpBidiIterator<Int8Iter>::Utf8ToCpBidiIterator(
            Int8Iter begin_,
            Int8Iter current_,
            Int8Iter end_)
    : begin(begin_), current(current_), end(end_)
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Int8Iter>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Int8Iter>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Int8Iter>
Utf8ToCpBidiIterator<Int8Iter>& Utf8ToCpBidiIterator<Int8Iter>::operator++ () //prefix
{
    jjm::readUtf8Forward(current, end);
    return *this;
}

template <typename Int8Iter>
Utf8ToCpBidiIterator<Int8Iter> Utf8ToCpBidiIterator<Int8Iter>::operator++ (int) //postfix
{
    Utf8ToCpBidiIterator tmp(*this);
    ++*this;
    return tmp;
}

template <typename Int8Iter>
Utf8ToCpBidiIterator<Int8Iter>& Utf8ToCpBidiIterator<Int8Iter>::operator-- () //prefix
{
    jjm::readUtf8Backward(begin, current);
    return *this;
}

template <typename Int8Iter>
Utf8ToCpBidiIterator<Int8Iter> Utf8ToCpBidiIterator<Int8Iter>::operator-- (int) //postfix
{
    Utf8ToCpBidiIterator tmp(*this);
    --*this;
    return tmp;
}

template <typename Int8Iter>
typename Utf8ToCpBidiIterator<Int8Iter>::value_type Utf8ToCpBidiIterator<Int8Iter>::operator* () const
{
    Int8Iter tmp = current;
    return jjm::readUtf8Forward(tmp, end);
}

template <typename Int8Iter>
bool operator== (Utf8ToCpBidiIterator<Int8Iter> const& a, Utf8ToCpBidiIterator<Int8Iter> const& b)
{
    return a.getIter() == b.getIter(); 
}

template <typename Int8Iter>
bool operator!= (Utf8ToCpBidiIterator<Int8Iter> const& a, Utf8ToCpBidiIterator<Int8Iter> const& b)
{
    return !(a == b); 
}

template <typename Int8Iter>
Utf16ToCpBidiIterator<Int8Iter>::Utf16ToCpBidiIterator()
    : begin(), current(), end()
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Int8Iter>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Int8Iter>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Int16Iter>
Utf16ToCpBidiIterator<Int16Iter>::Utf16ToCpBidiIterator(
                Int16Iter begin_,
                Int16Iter current_,
                Int16Iter end_) 
    : begin(begin_), current(current_), end(end_)
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Int16Iter>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Int16Iter>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Int8Iter>
Utf16ToCpBidiIterator<Int8Iter>& Utf16ToCpBidiIterator<Int8Iter>::operator++ () //prefix
{
    jjm::readUtf16Forward(current, end);
    return *this;
}

template <typename Int8Iter>
Utf16ToCpBidiIterator<Int8Iter> Utf16ToCpBidiIterator<Int8Iter>::operator++ (int) //postfix
{
    Utf16ToCpBidiIterator tmp(*this);
    ++*this;
    return tmp;
}

template <typename Int8Iter>
Utf16ToCpBidiIterator<Int8Iter>& Utf16ToCpBidiIterator<Int8Iter>::operator-- () //prefix
{
    jjm::readUtf16Backward(begin, current);
    return *this;
}

template <typename Int8Iter>
Utf16ToCpBidiIterator<Int8Iter> Utf16ToCpBidiIterator<Int8Iter>::operator-- (int) //postfix
{
    Utf16ToCpBidiIterator tmp(*this);
    --*this;
    return tmp;
}

template <typename Int16Iter>
typename Utf16ToCpBidiIterator<Int16Iter>::value_type Utf16ToCpBidiIterator<Int16Iter>::operator* () const
{
    Int16Iter tmp = current;
    return jjm::readUtf16Forward(tmp, end);
}

template <typename Int8Iter>
bool operator== (Utf16ToCpBidiIterator<Int8Iter> const& a, Utf16ToCpBidiIterator<Int8Iter> const& b)
{
    return a.getIter() == b.getIter();
}

template <typename Int8Iter>
bool operator!= (Utf16ToCpBidiIterator<Int8Iter> const& a, Utf16ToCpBidiIterator<Int8Iter> const& b)
{
    return !(a == b); 
}

} //namespace jjm

#endif
