// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JBASE_JNullTermIterator_HPP_HEADER_GUARD
#define JBASE_JNullTermIterator_HPP_HEADER_GUARD

#include <iterator>

namespace jjm
{

template <typename Iter>
class NullTermIterator; 
template <typename Iter>
bool operator== (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b);
template <typename Iter>
bool operator!= (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b);


template <typename Iter>
class NullTermIterator
{
public:
    typedef std::input_iterator_tag                                 iterator_category;
    typedef typename std::iterator_traits<Iter>::value_type         value_type;
    typedef typename std::iterator_traits<Iter>::difference_type    difference_type;
    typedef typename std::iterator_traits<Iter>::pointer            pointer;
    typedef typename std::iterator_traits<Iter>::reference          reference;
public:
    struct Proxy { value_type v; value_type operator* () const { return v; } };
public: 
    NullTermIterator() : iter(), v() {} //creates a "one-past-the-end" value
    NullTermIterator(Iter iter_); 
    NullTermIterator&  operator++ (); //prefix
    Proxy          operator++ (int); //postfix
    value_type     operator* (); 
    Iter           getIter() const { return iter; }

    friend bool operator== <> (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b);
    friend bool operator!= <> (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b);

private:
    Iter iter; 
    value_type v; 
};

template <typename Iter>
std::pair<NullTermIterator<Iter>, NullTermIterator<Iter> >  
makeNullTermRange(Iter nullTermIter)
{
    std::pair<NullTermIterator<Iter>, NullTermIterator<Iter> > result; 
    result.first = NullTermIterator<Iter>(nullTermIter); 
    result.second = NullTermIterator<Iter>(); 
    return result; 
}

// ---- ---- ---- ---- 
// impl: 

template <typename Iter>
NullTermIterator<Iter>::NullTermIterator(Iter iter_)
    : iter(iter_)
{
    v = *iter; 
}

template <typename Iter>
NullTermIterator<Iter>&  NullTermIterator<Iter>::operator++ () //prefix
{
    ++iter; 
    v = *iter; 
    return *this; 
}

template <typename Iter>
typename NullTermIterator<Iter>::Proxy  NullTermIterator<Iter>::operator++ (int ) //postfix
{
    Proxy p = { v };
    ++*this;
    return p;
}

template <typename Iter>
typename NullTermIterator<Iter>::value_type  NullTermIterator<Iter>::operator* ()
{
    return v; 
}


template <typename Iter>
bool operator== (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b)
{
    return a.v == 0 && b.v == 0;
}

template <typename Iter>
bool operator!= (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b)
{
    return !(a == b); 
}

}//namespace jjm

#endif
