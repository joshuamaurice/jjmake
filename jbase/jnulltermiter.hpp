#ifndef JNULLTERMITER_HPP_HEADER_GUARD
#define JNULLTERMITER_HPP_HEADER_GUARD

#include <iterator>

namespace jjm
{

//**** **** **** **** 
//** Public APIs

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
    NullTermIterator(); //creates a "one-past-the-end" value
    NullTermIterator(Iter iter_); 
    NullTermIterator&  operator++ (); //prefix
    Proxy              operator++ (int); //postfix
    value_type         operator* () const;  
    Iter               getIter() const { return iter; }

    friend bool operator== <> (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b);
    friend bool operator!= <> (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b);

private:
    Iter iter; 
    value_type v; 
};

template <typename Iter>
std::pair<NullTermIterator<Iter>, NullTermIterator<Iter> >
    makeNullTermRange(Iter const& iter);




//**** **** **** **** 
//** Private Implementation

template <typename Iter>
std::pair<NullTermIterator<Iter>, NullTermIterator<Iter> >
    makeNullTermRange(Iter const& iter)
{
    std::pair<NullTermIterator<Iter>, NullTermIterator<Iter> > x;
    x.first = NullTermIterator<Iter>(iter);
    x.second = NullTermIterator<Iter>(); 
    return x; 
}

template <typename Type, size_t N>
std::pair<NullTermIterator<Type*>, NullTermIterator<Type*> >
    makeNullTermRange(Type (&arr)[N])
{
    std::pair<NullTermIterator<Type*>, NullTermIterator<Type*> > x;
    x.first = NullTermIterator<Type*>(arr + 0);
    x.second = NullTermIterator<Type*>(); 
    return x; 
}

template <typename Iter>
NullTermIterator<Iter>::NullTermIterator() 
    : iter(), v()
{
}

template <typename Iter>
NullTermIterator<Iter>::NullTermIterator(Iter iter_) 
    : iter(iter_), v()
{
    v = *iter;
    if (v == value_type())
        iter = Iter();
    else
        ++iter; 
}

template <typename Iter>
NullTermIterator<Iter> &  NullTermIterator<Iter>::operator++ () //prefix
{
    v = *iter;
    if (v == value_type())
        iter = Iter();
    else
        ++iter; 
    return *this; 
}

template <typename Iter>
typename NullTermIterator<Iter>::Proxy  NullTermIterator<Iter>::operator++ (int ) //postfix
{
    Proxy proxy = { v }; 
    ++*this;
    return proxy; 
}

template <typename Iter>
typename NullTermIterator<Iter>::value_type  NullTermIterator<Iter>::operator* () const
{
    return v; 
}

template <typename Iter>
bool operator== (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b)
{
    return a.v == typename NullTermIterator<Iter>::value_type() 
        && b.v == typename NullTermIterator<Iter>::value_type(); 
}

template <typename Iter>
bool operator!= (NullTermIterator<Iter> const& a, NullTermIterator<Iter> const& b)
{
    return ! (a == b);
}


}//namespace jjm

#endif
