// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JBOUNDSCHECKEDITERS_HPP_HEADER_GUARD
#define JBOUNDSCHECKEDITERS_HPP_HEADER_GUARD

#include "jtemplatemetaprogrammingutils.hpp"

#include <iterator>
#include <stdexcept>
#include <typeinfo>

namespace jjm
{

//This class will throw a std::exception on out-of-range increments and 
//dereferences. 
template <typename Iter>
class BoundsCheckedInputIter
{
public:
    typedef typename std::input_iterator_tag                        iterator_category;
    typedef typename std::iterator_traits<Iter>::value_type         value_type;
    typedef typename std::iterator_traits<Iter>::difference_type    difference_type;
    typedef typename std::iterator_traits<Iter>::pointer            pointer;
    typedef typename std::iterator_traits<Iter>::reference          reference;
public:
    struct Proxy { value_type v; value_type operator* () const { return v; } };
public:
    BoundsCheckedInputIter();
    BoundsCheckedInputIter(Iter current_arg, Iter last_arg);

    BoundsCheckedInputIter& operator++ (); //prefix
    Proxy                   operator++ (int); //postfix
    value_type operator* () const;

    Iter current() const { return current_; }
    Iter last() const { return last_; }

private:
    Iter current_, last_;
};
template <typename Iter> bool operator== (BoundsCheckedInputIter<Iter> const& x, BoundsCheckedInputIter<Iter> const& y);
template <typename Iter> bool operator!= (BoundsCheckedInputIter<Iter> const& x, BoundsCheckedInputIter<Iter> const& y);


//This class will throw a std::exception on out-of-range increments, 
//decrements, and dereferences. 
template <typename Iter>
class BoundsCheckedIter
{
private:
    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {}
public:
    typedef typename std::iterator_traits<Iter>::iterator_category  iterator_category;
    typedef typename std::iterator_traits<Iter>::value_type         value_type;
    typedef typename std::iterator_traits<Iter>::difference_type    difference_type;
    typedef typename std::iterator_traits<Iter>::pointer            pointer;
    typedef typename std::iterator_traits<Iter>::reference          reference;
public:
    BoundsCheckedIter();
    BoundsCheckedIter(Iter first_arg, Iter current_arg, Iter last_arg);

    BoundsCheckedIter& operator++ (); //prefix
    BoundsCheckedIter  operator++ (int); //postfix
    BoundsCheckedIter& operator-- (); //prefix
    BoundsCheckedIter  operator-- (int); //postfix
    BoundsCheckedIter& operator+= (typename BoundsCheckedIter<Iter>::difference_type offset);
    BoundsCheckedIter& operator-= (typename BoundsCheckedIter<Iter>::difference_type offset);
    value_type operator* () const;

    operator unspecified_bool_type () const { return current_ ? unspecified_bool_true : 0; }

    Iter first() const { return first_; }
    Iter current() const { return current_; }
    Iter last() const { return last_; }

private:
    Iter first_, current_, last_;
};
template <typename Iter> bool operator== (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y);
template <typename Iter> bool operator!= (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y);
template <typename Iter> bool operator<= (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y);
template <typename Iter> bool operator<  (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y);
template <typename Iter> bool operator>= (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y);
template <typename Iter> bool operator>  (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y);
template <typename Iter> BoundsCheckedIter<Iter> operator+ (BoundsCheckedIter<Iter> x, typename BoundsCheckedIter<Iter>::difference_type offset);
template <typename Iter> BoundsCheckedIter<Iter> operator+ (typename BoundsCheckedIter<Iter>::difference_type offset, BoundsCheckedIter<Iter> x);
template <typename Iter> typename BoundsCheckedIter<Iter>::difference_type  operator- (BoundsCheckedIter<Iter> x, BoundsCheckedIter<Iter> y);
template <typename Iter> BoundsCheckedIter<Iter> operator- (BoundsCheckedIter<Iter> x, typename BoundsCheckedIter<Iter>::difference_type offset);


// ---- ---- ---- ---- 
// impl: 

template <typename Iter>
BoundsCheckedInputIter<Iter>::BoundsCheckedInputIter() : current_(), last_() 
{
    const bool b = jjm::IsConvertibleTo<
            typename std::iterator_traits<Iter>::iterator_category, 
            std::input_iterator_tag
            >::b;
    JSTATICASSERT(b);
}

template <typename Iter>
BoundsCheckedInputIter<Iter>::BoundsCheckedInputIter(Iter current_arg, Iter last_arg)
        : current_(current_arg), last_(last_arg)
{
    const bool b = jjm::IsConvertibleTo<
            typename std::iterator_traits<Iter>::iterator_category, 
            std::input_iterator_tag
            >::b;
    JSTATICASSERT(b);
}

template <typename Iter>
BoundsCheckedInputIter<Iter>& BoundsCheckedInputIter<Iter>::operator++() //prefix
{
    if (current_ == last_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator++() out-of-range");
    ++current_;
    return *this;
}

template <typename Iter>
typename BoundsCheckedInputIter<Iter>::Proxy BoundsCheckedInputIter<Iter>::operator++(int) //postfix
{
    if (current_ == last_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator++(int) out-of-range");
    Proxy p = { *current_ };
    ++*this;
    return p;
}

template <typename Iter>
typename BoundsCheckedInputIter<Iter>::value_type BoundsCheckedInputIter<Iter>::operator*() const
{
    if (current_ == last_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator*() out-of-range");
    return *current_;
}

template <typename Iter>
bool operator== (BoundsCheckedInputIter<Iter> const& x, BoundsCheckedInputIter<Iter> const& y)
{
    return x.current() == y.current();
}

template <typename Iter> 
bool operator!= (BoundsCheckedInputIter<Iter> const& x, BoundsCheckedInputIter<Iter> const& y)
{
    return x.current() != y.current();
}

template <typename Iter>
BoundsCheckedIter<Iter>::BoundsCheckedIter() : first_(), current_(), last_()
{
    const bool b = jjm::IsSameType<std::output_iterator_tag, iterator_category>::b;
    JSTATICASSERT( ! b);
}

template <typename Iter>
BoundsCheckedIter<Iter>::BoundsCheckedIter(Iter first_arg, Iter current_arg, Iter last_arg)
    : first_(first_arg), current_(current_arg), last_(last_arg)
{
    const bool b = jjm::IsSameType<std::output_iterator_tag, iterator_category>::b;
    JSTATICASSERT( ! b);
}

template <typename Iter>
BoundsCheckedIter<Iter>& BoundsCheckedIter<Iter>::operator++ () //prefix
{
    if (current_ == last_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator++() out-of-range");
    ++current_;
    return *this;
}

template <typename Iter>
BoundsCheckedIter<Iter> BoundsCheckedIter<Iter>::operator++ (int) //postfix
{
    if (current_ == last_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator++(int) out-of-range");
    BoundsCheckedIter tmp(*this);
    ++*this;
    return tmp;
}

template <typename Iter>
BoundsCheckedIter<Iter>& BoundsCheckedIter<Iter>::operator-- () //prefix
{
    if (current_ == first_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator--() out-of-range");
    --current_;
    return *this;
}

template <typename Iter>
BoundsCheckedIter<Iter> BoundsCheckedIter<Iter>::operator-- (int) //postfix
{
    if (current_ == first_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator--(int) out-of-range");
    BoundsCheckedIter tmp(*this);
    --*this;
    return tmp;
}

template <typename Iter>
BoundsCheckedIter<Iter>& BoundsCheckedIter<Iter>::operator+= (typename BoundsCheckedIter<Iter>::difference_type offset)
{
    if (last_ - current_ < offset)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator+= out-of-range");
    current_ += offset;
    return *this;
}

template <typename Iter>
BoundsCheckedIter<Iter>& BoundsCheckedIter<Iter>::operator-= (typename BoundsCheckedIter<Iter>::difference_type offset)
{
    if (current_ - first_ < offset)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator-= out-of-range");
    current_ -= offset;
    return *this;
}

template <typename Iter>
typename BoundsCheckedIter<Iter>::value_type BoundsCheckedIter<Iter>::operator* () const
{
    if (current_ == last_)
        throw std::runtime_error(std::string() + typeid(*this).name() + "::operator*() out-of-range");
    return *current_;
}

template <typename Iter>
bool operator== (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y)
{
    return x.current() == y.current();
}

template <typename Iter>
bool operator!= (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y)
{
    return x.current() != y.current();
}

template <typename Iter>
bool operator< (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y)
{
    return x.current() < y.current();
}

template <typename Iter>
bool operator<= (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y)
{
    return x.current() <= y.current();
}

template <typename Iter>
bool operator> (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y)
{
    return x.current() > y.current();
}

template <typename Iter>
bool operator>= (BoundsCheckedIter<Iter> const& x, BoundsCheckedIter<Iter> const& y)
{
    return x.current() >= y.current();
}

template <typename Iter> 
BoundsCheckedIter<Iter> operator+ (BoundsCheckedIter<Iter> x, typename BoundsCheckedIter<Iter>::difference_type offset)
{
    x += offset;
    return x;
}

template <typename Iter> 
BoundsCheckedIter<Iter> operator+ (typename BoundsCheckedIter<Iter>::difference_type offset, BoundsCheckedIter<Iter> x)
{
    x += offset;
    return x;
}

template <typename Iter> 
typename BoundsCheckedIter<Iter>::difference_type operator- (BoundsCheckedIter<Iter> x, BoundsCheckedIter<Iter> y)
{
    return x.current() - y.current();
}

template <typename Iter> 
BoundsCheckedIter<Iter> operator- (BoundsCheckedIter<Iter> x, typename BoundsCheckedIter<Iter>::difference_type offset)
{
    x -= offset;
    return x;
}

}//namespace jjm

#endif
