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

template <typename Integer8Iterator>
class Utf8InputIterator;
template <typename Integer16Iterator>
class Utf16InputIterator;

template <typename Integer8Iterator>
class Utf8BidiIterator;
template <typename Integer16Iterator>
class Utf16BidiIterator;

template <typename Integer8Iterator>
bool operator== (Utf8InputIterator<Integer8Iterator> const& a, Utf8InputIterator<Integer8Iterator> const& b);
template <typename Integer16Iterator>
bool operator== (Utf16InputIterator<Integer16Iterator> const& a, Utf16InputIterator<Integer16Iterator> const& b);

template <typename Integer8Iterator>
bool operator!= (Utf8InputIterator<Integer8Iterator> const& a, Utf8InputIterator<Integer8Iterator> const& b);
template <typename Integer16Iterator>
bool operator!= (Utf16InputIterator<Integer16Iterator> const& a, Utf16InputIterator<Integer16Iterator> const& b);


template <typename Integer8Iterator>
class Utf8InputIterator
{
public:
    typedef std::input_iterator_tag     iterator_category;
    typedef CodePoint                   value_type;
    typedef typename std::iterator_traits<Integer8Iterator>::difference_type    difference_type;
    typedef CodePoint const*            pointer;
    typedef CodePoint const&            reference;
public:
    struct Proxy { CodePoint cp; CodePoint operator* () const { return cp; } };
public:
    Utf8InputIterator();
    Utf8InputIterator(Integer8Iterator x);
    Utf8InputIterator&      operator++ (); //prefix
    Proxy               operator++ (int); //postfix
    value_type          operator* ();
    Integer8Iterator    euIter() const { return iter; }

    friend bool operator== <> (Utf8InputIterator<Integer8Iterator> const& a, Utf8InputIterator<Integer8Iterator> const& b);
    friend bool operator!= <> (Utf8InputIterator<Integer8Iterator> const& a, Utf8InputIterator<Integer8Iterator> const& b);

private:
    Integer8Iterator iter;
    CodePoint cp;
};


template <typename Integer16Iterator>
class Utf16InputIterator
{
public:
    typedef std::input_iterator_tag     iterator_category;
    typedef CodePoint                   value_type;
    typedef typename std::iterator_traits<Integer16Iterator>::difference_type    difference_type;
    typedef CodePoint const*            pointer;
    typedef CodePoint const&            reference;
public:
    struct Proxy { CodePoint cp; CodePoint operator* () const { return cp; } };
public:
    Utf16InputIterator();
    Utf16InputIterator(Integer16Iterator x);
    Utf16InputIterator&     operator++ (); //prefix
    Proxy                   operator++ (int); //postfix
    value_type              operator* ();
    Utf16InputIterator      euIter() const { return iter; }

    friend bool operator== <> (Utf16InputIterator<Integer16Iterator> const& a, Utf16InputIterator<Integer16Iterator> const& b);
    friend bool operator!= <> (Utf16InputIterator<Integer16Iterator> const& a, Utf16InputIterator<Integer16Iterator> const& b);

private:
    Integer16Iterator iter;
    CodePoint cp;
};


template <typename Integer8Iterator>
class Utf8BidiIterator
{
public:
    typedef std::bidirectional_iterator_tag  iterator_category;
    typedef CodePoint  value_type;
    typedef typename std::iterator_traits<Integer8Iterator>::difference_type  difference_type;
    typedef CodePoint const*    pointer;
    typedef CodePoint const&    reference;

public:
    Utf8BidiIterator();
    Utf8BidiIterator(Integer8Iterator x);
    Utf8BidiIterator&   operator++ (); //prefix
    Utf8BidiIterator    operator++ (int); //postfix
    Utf8BidiIterator&   operator-- (); //prefix
    Utf8BidiIterator    operator-- (int); //postfix
    value_type          operator* () const;
    Integer8Iterator    euIter() const { return iter; }
private:
    Integer8Iterator iter;
};


template <typename Integer16Iterator>
class Utf16BidiIterator
{
public:
    typedef std::bidirectional_iterator_tag     iterator_category;
    typedef CodePoint                           value_type;
    typedef typename std::iterator_traits<Integer16Iterator>::difference_type    difference_type;
    typedef CodePoint const*                    pointer;
    typedef CodePoint const&                    reference;

public:
    Utf16BidiIterator();
    Utf16BidiIterator(Integer16Iterator x);
    Utf16BidiIterator&  operator++ (); //prefix
    Utf16BidiIterator   operator++ (int); //postfix
    Utf16BidiIterator&  operator-- (); //prefix
    Utf16BidiIterator   operator-- (int); //postfix
    CodePoint           operator* () const;
    Integer16Iterator   euIter() const { return iter; }

    friend bool operator== <> (Utf16BidiIterator<Integer16Iterator> const& a, Utf16BidiIterator<Integer16Iterator> const& b);
    friend bool operator!= <> (Utf16BidiIterator<Integer16Iterator> const& a, Utf16BidiIterator<Integer16Iterator> const& b);

private:
    Integer16Iterator iter;
};


// ---- ---- ---- ---- 
// impl: 

template <typename Integer8Iterator>
Utf8InputIterator<Integer8Iterator>::Utf8InputIterator()
    : iter(), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Integer8Iterator>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);
}

template <typename Integer8Iterator>
Utf8InputIterator<Integer8Iterator>::Utf8InputIterator(Integer8Iterator x)
    : iter(x), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Integer8Iterator>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);
}

template <typename Integer8Iterator>
Utf8InputIterator<Integer8Iterator>& Utf8InputIterator<Integer8Iterator>::operator++ () //prefix
{
    if (cp != static_cast<CodePoint>(-1))
        cp = static_cast<CodePoint>(-1);
    return *this;
}

template <typename Integer8Iterator>
typename Utf8InputIterator<Integer8Iterator>::Proxy Utf8InputIterator<Integer8Iterator>::operator++ (int) //postfix
{
    Proxy p = { **this };
    ++*this;
    return p;
}

template <typename Integer8Iterator>
typename Utf8InputIterator<Integer8Iterator>::value_type Utf8InputIterator<Integer8Iterator>::operator* ()
{
    if (cp == static_cast<CodePoint>(-1))
        cp = jjm::readUtf8Forward(iter);
    return cp;
}

template <typename Integer8Iterator>
bool operator== (Utf8InputIterator<Integer8Iterator> const& a, Utf8InputIterator<Integer8Iterator> const& b)
{
    return a.cp == b.cp && a.iter == b.iter;
}

template <typename Integer8Iterator>
bool operator!= (Utf8InputIterator<Integer8Iterator> const& a, Utf8InputIterator<Integer8Iterator> const& b)
{
    return !(a == b);
}

template <typename Integer8Iterator>
Utf16InputIterator<Integer8Iterator>::Utf16InputIterator()
    : iter(), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Integer8Iterator>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);
}

template <typename Integer8Iterator>
Utf16InputIterator<Integer8Iterator>::Utf16InputIterator(Integer8Iterator x)
    : iter(x), cp(static_cast<CodePoint>(-1))
{
    const bool b = jjm::IsConvertibleTo<
        typename std::iterator_traits<Integer8Iterator>::iterator_category,
        std::input_iterator_tag
    >::b;
    JSTATICASSERT(b);
}

template <typename Integer8Iterator>
Utf16InputIterator<Integer8Iterator>& Utf16InputIterator<Integer8Iterator>::operator++ () //prefix
{
    if (cp != static_cast<CodePoint>(-1))
        cp = static_cast<CodePoint>(-1);
    return *this;
}

template <typename Integer8Iterator>
typename Utf16InputIterator<Integer8Iterator>::Proxy Utf16InputIterator<Integer8Iterator>::operator++ (int) //postfix
{
    Proxy p = { **this };
    ++*this;
    return p;
}

template <typename Integer8Iterator>
typename Utf16InputIterator<Integer8Iterator>::value_type Utf16InputIterator<Integer8Iterator>::operator* ()
{
    if (cp == static_cast<CodePoint>(-1))
        cp = jjm::readUtf16Forward(iter);
    return cp;
}

template <typename Integer8Iterator>
bool operator== (Utf16InputIterator<Integer8Iterator> const& a, Utf16InputIterator<Integer8Iterator> const& b)
{
    return a.cp == b.cp && a.iter == b.iter;
}

template <typename Integer8Iterator>
bool operator!= (Utf16InputIterator<Integer8Iterator> const& a, Utf16InputIterator<Integer8Iterator> const& b)
{
    return !(a == b);
}

template <typename Integer8Iterator>
Utf8BidiIterator<Integer8Iterator>::Utf8BidiIterator()
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Integer8Iterator>
Utf8BidiIterator<Integer8Iterator>::Utf8BidiIterator(Integer8Iterator x) : iter(x)
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Integer8Iterator>
Utf8BidiIterator<Integer8Iterator>& Utf8BidiIterator<Integer8Iterator>::operator++ () //prefix
{
    jjm::readUtf8Forward(iter);
    return *this;
}

template <typename Integer8Iterator>
Utf8BidiIterator<Integer8Iterator> Utf8BidiIterator<Integer8Iterator>::operator++ (int) //postfix
{
    Utf8BidiIterator tmp(*this);
    ++*this;
    return tmp;
}

template <typename Integer8Iterator>
Utf8BidiIterator<Integer8Iterator>& Utf8BidiIterator<Integer8Iterator>::operator-- () //prefix
{
    jjm::readUtf8Backward(iter);
    return *this;
}

template <typename Integer8Iterator>
Utf8BidiIterator<Integer8Iterator> Utf8BidiIterator<Integer8Iterator>::operator-- (int) //postfix
{
    Utf8BidiIterator tmp(*this);
    --*this;
    return tmp;
}

template <typename Integer8Iterator>
typename Utf8BidiIterator<Integer8Iterator>::value_type Utf8BidiIterator<Integer8Iterator>::operator* () const
{
    Integer8Iterator tmp = iter;
    return jjm::readUtf8Forward(tmp);
}

template <typename Integer8Iterator>
bool operator== (Utf8BidiIterator<Integer8Iterator> const& a, Utf8BidiIterator<Integer8Iterator> const& b)
{
    return a.euIter() == b.euIter();
}

template <typename Integer8Iterator>
bool operator!= (Utf8BidiIterator<Integer8Iterator> const& a, Utf8BidiIterator<Integer8Iterator> const& b)
{
    return a.euIter() != b.euIter();
}

template <typename Integer8Iterator>
Utf16BidiIterator<Integer8Iterator>::Utf16BidiIterator()
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Integer8Iterator>
Utf16BidiIterator<Integer8Iterator>::Utf16BidiIterator(Integer8Iterator x) : iter(x)
{
    const bool b1 = jjm::IsSameType<
        std::bidirectional_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    const bool b2 = jjm::IsSameType<
        std::random_access_iterator_tag,
        typename std::iterator_traits<Integer8Iterator>::iterator_category
    >::b;
    JSTATICASSERT(b1 || b2);
}

template <typename Integer8Iterator>
Utf16BidiIterator<Integer8Iterator>& Utf16BidiIterator<Integer8Iterator>::operator++ () //prefix
{
    jjm::readUtf16Forward(iter);
    return *this;
}

template <typename Integer8Iterator>
Utf16BidiIterator<Integer8Iterator> Utf16BidiIterator<Integer8Iterator>::operator++ (int) //postfix
{
    Utf16BidiIterator tmp(*this);
    ++*this;
    return tmp;
}

template <typename Integer8Iterator>
Utf16BidiIterator<Integer8Iterator>& Utf16BidiIterator<Integer8Iterator>::operator-- () //prefix
{
    jjm::readUtf16Backward(iter);
    return *this;
}

template <typename Integer8Iterator>
Utf16BidiIterator<Integer8Iterator> Utf16BidiIterator<Integer8Iterator>::operator-- (int) //postfix
{
    Utf16BidiIterator tmp(*this);
    --*this;
    return tmp;
}

template <typename Integer8Iterator>
typename Utf16BidiIterator<Integer8Iterator>::value_type Utf16BidiIterator<Integer8Iterator>::operator* () const
{
    Integer8Iterator tmp = iter;
    return jjm::readUtf16Forward(tmp);
}

template <typename Integer8Iterator>
bool operator== (Utf16BidiIterator<Integer8Iterator> const& a, Utf16BidiIterator<Integer8Iterator> const& b)
{
    return a.Integer8Iterator() == b.Integer8Iterator();
}

template <typename Integer8Iterator>
bool operator!= (Utf16BidiIterator<Integer8Iterator> const& a, Utf16BidiIterator<Integer8Iterator> const& b)
{
    return a.Integer8Iterator() != b.Integer8Iterator();
}

} //namespace

#endif
