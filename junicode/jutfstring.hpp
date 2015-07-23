// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUSTRING_HPP_HEADER_GUARD
#define JUNICODE_JUSTRING_HPP_HEADER_GUARD

#include "jutfiterator.hpp"
#include "jbase/jnulltermiter.hpp"
#include "jbase/jtemplatemetaprogrammingutils.hpp"


namespace jjm
{

template <typename allocator_tye> 
class BasicUtf8String;
template <typename allocator_tye> 
class BasicUtf16String;




//Note that this class only uses the allocate() and deallocate() functions of allocator_type.
template <typename allocator_type = std::allocator<char> >
class BasicUtf8String
{
public:
    typedef  allocator_type         AllocatorType;

    typedef  char                   EncodingUnit;
    typedef  unsigned char          UnsignedEncodingUnit;
    typedef  EncodingUnit const*    EuIterator;
    typedef  Utf8ToCpBidiIterator<EuIterator>    CpIterator;

    static BasicUtf8String bom()
    {
        static unsigned char const x[3] = { 0xEF, 0xBB, 0xBF };
        return utf8(reinterpret_cast<char const*>(x), reinterpret_cast<char const*>(x)+3);
    }

public:
    /* The static creation functions utf8(), utf16(), and cp() create a copy.
    The utf8 functions do no validation.
    The utf16 and cp functions do validate input (throws std::exception on 
    invalid input). */

    static                           BasicUtf8String utf8(char const* nullTerm,               allocator_type const& alloc = allocator_type());
    template <typename Range> static BasicUtf8String utf8(Range const& range,                 allocator_type const& alloc = allocator_type());
    template <typename Iter>  static BasicUtf8String utf8(Iter const& begin, Iter const& end, allocator_type const& alloc = allocator_type());

    template <typename Range> static BasicUtf8String utf16(Range const& range,                 allocator_type const& alloc = allocator_type());
    template <typename Iter>  static BasicUtf8String utf16(Iter const& begin, Iter const& end, allocator_type const& alloc = allocator_type());

    static BasicUtf8String cp(CodePoint cp, allocator_type const& alloc = allocator_type());
    template <typename Range> static BasicUtf8String cp(Range const& range,                 allocator_type const& alloc = allocator_type());
    template <typename Iter>  static BasicUtf8String cp(Iter const& begin, Iter const& end, allocator_type const& alloc = allocator_type());

public:
    explicit BasicUtf8String(allocator_type const& alloc = allocator_type());
    BasicUtf8String(BasicUtf8String const& x);
    BasicUtf8String& operator= (BasicUtf8String const& x);
    ~BasicUtf8String();

    //Non-empty strings guaranteed to return null-terminated string. 
    //Empty strings may return null-terminated empty string or null-pointer. 
    EncodingUnit const* c_str() const { return m_begin; }
    EncodingUnit const* data() const { return m_begin; }

    bool empty() const { return m_endSize == m_begin; }

    //To give fast c_str() and data(), and ensure always null terminated, 
    //it is guaranteed that: capacityEU() >= sizeEU() + 1 or capacityEU() == 0
    BasicUtf8String& reserveEU(std::size_t numEncodingUnits);

    std::size_t capacityEU() const { return m_endCapacity - m_begin; }

    void shrink_to_fit() { BasicUtf8String(*this).swap(*this); }

    std::size_t lengthBytes() const { return (m_endSize - m_begin) * sizeof(EncodingUnit); }
    std::size_t lengthEU() const { return m_endSize - m_begin; }
    std::size_t sizeBytes() const { return (m_endSize - m_begin) * sizeof(EncodingUnit); }
    std::size_t sizeEU() const { return lengthEU(); }

    void clear() { m_endSize = m_begin; }

    template <typename CpIterator2> BasicUtf8String& insert(CpIterator at, CpIterator2 begin_, CpIterator2 end_);
    BasicUtf8String& insert(CpIterator at, BasicUtf8String const& str);

    void erase(CpIterator begin_, CpIterator end_);

    BasicUtf8String& append(BasicUtf8String str);

    BasicUtf8String& appendCP(CodePoint cp);
    template <typename CpRange> BasicUtf8String& appendCP(CpRange r);
    template <typename CpIterator2> BasicUtf8String& appendCP(CpIterator2 begin_, CpIterator2 end_);

    //Only add complete codepoints! Otherwise bad things may happen. 
    BasicUtf8String& appendEU(char x);
    BasicUtf8String& appendEU(EncodingUnit const* begin_, EncodingUnit const* end_);
    template <typename EuIter> BasicUtf8String& appendEU(EuIter begin_, EuIter end_);

    void swap(BasicUtf8String& x);

    CpIterator  beginCP() { return cbeginCP(); }
    CpIterator  beginCP() const { return cbeginCP(); }
    CpIterator cbeginCP() const;
    CpIterator  endCP() { return cendCP(); }
    CpIterator  endCP() const { return cendCP(); }
    CpIterator cendCP() const;

    EuIterator  beginEU() { return cbeginEU(); }
    EuIterator  beginEU() const { return cbeginEU(); }
    EuIterator cbeginEU() const { return m_begin; }
    EuIterator  endEU() { return cendEU(); }
    EuIterator  endEU() const { return cendEU(); }
    EuIterator cendEU() const { return m_endSize; }

    allocator_type get_allocator() const { return m_alloc; }

    //Do not use this unless you know what you are doing, are familiar with 
    //utf8, and are familiar with the implementation of this class. 
    EncodingUnit* mutable_data() { return m_begin; }

private:
    EncodingUnit* m_begin;
    EncodingUnit* m_endSize;
    EncodingUnit* m_endCapacity;
    allocator_type m_alloc;

    //Caller need to guarantee the invariant that everything in the range 
    //[m_endSize, m_endCapacity) is 0. 
    BasicUtf8String& reserveEUInternal(std::size_t numEncodingUnits);
};


template <typename allocator_type>
void swap(BasicUtf8String<allocator_type>& x, BasicUtf8String<allocator_type>& y) { x.swap(y); }

// !! Be warned that these comparison operators do not do proper collation. !!
//They do not do normalization. 
//They do binary content comparison, and a binary content lexicographic sorting. 
template <typename alloc1, typename alloc2> bool operator== (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator!= (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator<  (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator<= (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator>  (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator>= (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b);

typedef BasicUtf8String<std::allocator<char> >  Utf8String;
typedef BasicUtf8String<std::allocator<char> >  Utf8Str;
typedef BasicUtf8String<std::allocator<char> >  U8Str;

template <typename allocator_type>
BasicUtf8String<allocator_type>  operator+  (
        BasicUtf8String<allocator_type> const& a, BasicUtf8String<allocator_type> const& b);


template <typename allocator_type>
char const* JjmFatalHandlerUtil(BasicUtf8String<allocator_type> const& str) { return str.c_str(); }




//Note that this class only uses the allocate() and deallocate() functions of allocator_type.
template <typename allocator_type = std::allocator<Utf16EncodingUnit> >
class BasicUtf16String
{
public:
    typedef allocator_type          AllocatorType;

    typedef  Utf16EncodingUnit      EncodingUnit;
    typedef  Utf16EncodingUnit      UnsignedEncodingUnit;
    typedef  EncodingUnit const*    EuIterator;
    typedef  Utf16ToCpBidiIterator<EuIterator>    CpIterator;

    static BasicUtf16String bom()
    {
        static EncodingUnit const x = 0xFEFF;
        return utf16(&x, &x + 1);
    }

public:
    /* The static creation functions utf8(), utf16(), and cp() create a copy.
    The utf16 functions do no validation.
    The utf8 and cp functions do validate input (throws std::exception on 
    invalid input). */

    template <typename Range> static BasicUtf16String utf8(Range const& range,                 allocator_type const& alloc = allocator_type());
    template <typename Iter>  static BasicUtf16String utf8(Iter const& begin, Iter const& end, allocator_type const& alloc = allocator_type());

    static                           BasicUtf16String utf16(EncodingUnit const* nullTerm,       allocator_type const& alloc = allocator_type());
    template <typename Range> static BasicUtf16String utf16(Range const& range,                 allocator_type const& alloc = allocator_type());
    template <typename Iter>  static BasicUtf16String utf16(Iter const& begin, Iter const& end, allocator_type const& alloc = allocator_type());

    static BasicUtf16String cp(CodePoint cp, allocator_type const& alloc = allocator_type());
    template <typename Range> static BasicUtf16String cp(Range const& range,                 allocator_type const& alloc = allocator_type());
    template <typename Iter>  static BasicUtf16String cp(Iter const& begin, Iter const& end, allocator_type const& alloc = allocator_type());

public:
    explicit BasicUtf16String(allocator_type const& alloc = allocator_type());
    BasicUtf16String(BasicUtf16String const& x);
    BasicUtf16String& operator= (BasicUtf16String const& x);
    ~BasicUtf16String();
    
    //Non-empty strings guaranteed to return null-terminated string. 
    //Empty strings may return null-terminated empty string or null-pointer. 
    EncodingUnit const* c_str() const { return m_begin; }
    EncodingUnit const* data() const { return m_begin; }

    bool empty() const { return m_endSize == m_begin; }

    //To give fast c_str() and data(), and ensure always null terminated, 
    //it is guaranteed that: capacityEU() >= sizeEU() + 1 or capacityEU() == 0
    BasicUtf16String& reserveEU(std::size_t numEncodingUnits);

    std::size_t capacityEU() const { return m_endCapacity - m_begin; }

    void shrink_to_fit() { BasicUtf16String(*this).swap(*this); }

    std::size_t lengthBytes() const { return (m_endSize - m_begin) * sizeof(EncodingUnit); }
    std::size_t lengthEU() const { return m_endSize - m_begin; }
    std::size_t sizeBytes() const { return (m_endSize - m_begin) * sizeof(EncodingUnit); }
    std::size_t sizeEU() const { return lengthEU(); }

    void clear() { m_endSize = m_begin; }

    BasicUtf16String& insert(CpIterator at, BasicUtf16String const& str);
    template <typename CpIterator2> BasicUtf16String& insert(CpIterator at, CpIterator2 begin_, CpIterator2 end_);

    void erase(CpIterator begin_, CpIterator end_);

    BasicUtf16String& append(BasicUtf16String str);

    BasicUtf16String& appendCP(CodePoint cp);
    template <typename CpRange> BasicUtf16String& appendCP(CpRange r);
    template <typename CpIterator2> BasicUtf16String& appendCP(CpIterator2 begin_, CpIterator2 end_);

    //Only add complete code point encodings! Otherwise bad things may happen. 
    BasicUtf16String& appendEU(EncodingUnit x);
    BasicUtf16String& appendEU(EncodingUnit const* begin_, EncodingUnit const* end_);
    template <typename EuIter> BasicUtf16String& appendEU(EuIter begin_, EuIter end_);

    void swap(BasicUtf16String& x);

    CpIterator  beginCP() { return cbeginCP(); }
    CpIterator  beginCP() const { return cbeginCP(); }
    CpIterator cbeginCP() const;
    CpIterator  endCP() { return cendCP(); }
    CpIterator  endCP() const { return cendCP(); }
    CpIterator cendCP() const;

    EuIterator  beginEU() { return cbeginEU(); }
    EuIterator  beginEU() const { return cbeginEU(); }
    EuIterator cbeginEU() const { return m_begin; }
    EuIterator  endEU() { return cendEU(); }
    EuIterator  endEU() const { return cendEU(); }
    EuIterator cendEU() const { return m_endSize; }

    allocator_type get_allocator() const { return m_alloc; }

    //Do not use this unless you know what you are doing, are familiar with 
    //utf16, and are familiar with the implementation of this class. 
    EncodingUnit* mutable_data() { return m_begin; }

private:
    EncodingUnit* m_begin;
    EncodingUnit* m_endSize;
    EncodingUnit* m_endCapacity;
    allocator_type m_alloc;

    //Caller need to guarantee the invariant that everything in the range 
    //[m_endSize, m_endCapacity) is 0. 
    BasicUtf16String& reserveEUInternal(std::size_t numEncodingUnits);
};

template <typename allocator_type>
void swap(BasicUtf16String<allocator_type>& x, BasicUtf16String<allocator_type>& y) { x.swap(y); }

// !! Be warned that these comparison operators do not do proper collation. !!
//They do not do normalization. 
//They do binary content comparison, and a binary content lexicographic sorting. 
template <typename alloc1, typename alloc2> bool operator== (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator!= (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator<  (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator<= (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator>  (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b);
template <typename alloc1, typename alloc2> bool operator>= (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b);

typedef BasicUtf16String<std::allocator<Utf16EncodingUnit> >  Utf16String;
typedef BasicUtf16String<std::allocator<Utf16EncodingUnit> >  Utf16Str;
typedef BasicUtf16String<std::allocator<Utf16EncodingUnit> >  U16Str;

template <typename allocator_type>
BasicUtf16String<allocator_type>  operator+  (
        BasicUtf16String<allocator_type> const& a, BasicUtf16String<allocator_type> const& b);




/* It is assumed that only 7-bit ASCII (not extended ASCII, not Latin-1) strings are
passed to these functions. It is not checked.
(See: jstringliteral.hpp, which asserts at compile time that "ASCII character"
string literals are encoded in ASCII.)
*/
inline Utf8String  a2u8(char const * ascii);
inline Utf8String  a2u8(std::string const& ascii);
template <std::size_t n>  Utf8String  a2u8(char const (&ascii)[n]);
inline Utf16String  a2u16(char const * ascii);
inline Utf16String  a2u16(std::string const& ascii);
template <std::size_t n>  Utf16String  a2u16(char const (&ascii)[n]);




// ---- ---- ---- ---- 
// impl: 

namespace Internal
{
    inline char const* getUtf8EuRangeBegin(std::string const& range) { return range.c_str(); }
    inline char const* getUtf8EuRangeEnd(std::string const& range) { return range.c_str() + range.size(); }

    inline U8Str::EuIterator getUtf8EuRangeBegin(U8Str const& range) { return range.beginEU(); }
    inline U8Str::EuIterator getUtf8EuRangeEnd(U8Str const& range) { return range.endEU(); }

    inline U16Str::EuIterator getUtf16EuRangeBegin(U16Str const& range) { return range.beginEU(); }
    inline U16Str::EuIterator getUtf16EuRangeEnd(U16Str const& range) { return range.endEU(); }

    template <typename Iter>
    Iter getUtf16EuRangeBegin(std::pair<Iter, Iter> const& range) { return range.first; }
    template <typename Iter>
    Iter getUtf16EuRangeEnd  (std::pair<Iter, Iter> const& range) { return range.second; }

    inline U8Str::CpIterator getCpRangeBegin(U8Str const& range) { return range.beginCP(); }
    inline U8Str::CpIterator getCpRangeEnd  (U8Str const& range) { return range.endCP  (); }

    inline U16Str::CpIterator getCpRangeBegin(U16Str const& range) { return range.beginCP(); }
    inline U16Str::CpIterator getCpRangeEnd  (U16Str const& range) { return range.endCP  (); }

    template <typename Iter> Iter getCpRangeBegin(std::pair<Iter, Iter> range) { return range.first; }
    template <typename Iter> Iter getCpRangeEnd  (std::pair<Iter, Iter> range) { return range.second; }

    template <typename Iter, typename IterTag>
    struct JustringIterDiff
    {   inline typename std::iterator_traits<Iter>::difference_type operator() (Iter end, Iter begin) const { return 0; }
    };
    template <typename Iter>
    struct JustringIterDiff<Iter, std::random_access_iterator_tag>
    {   inline typename std::iterator_traits<Iter>::difference_type operator() (Iter end, Iter begin) const { return end - begin; }
    };
}





template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf8(
    char const* nullTerm, allocator_type const& alloc)
{
    BasicUtf8String str(alloc); 
    auto range = jjm::makeNullTermRange(nullTerm); 
    str.appendEU(range.first, range.second); 
    return str; 
}

template <typename allocator_type>
template <typename Range>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf8(
    Range const& range, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    using jjm::Internal::getUtf8EuRangeBegin; 
    using jjm::Internal::getUtf8EuRangeEnd; 
    JSTATICASSERT(sizeof( * getUtf8EuRangeBegin(range)) == 1); 
    str.appendEU(getUtf8EuRangeBegin(range), getUtf8EuRangeEnd(range)); 
    return str; 
}

template <typename allocator_type>
template <typename Iter>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf8(
    Iter const& begin_, Iter const& end_, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    JSTATICASSERT(sizeof( * begin_) == 1); 
    str.appendEU(begin_, end_); 
    return str; 
}

template <typename allocator_type>
template <typename Range>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf16(
    Range const& range, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    using jjm::Internal::getUtf16EuRangeBegin; 
    using jjm::Internal::getUtf16EuRangeEnd; 
    JSTATICASSERT(sizeof( * getUtf16EuRangeBegin(range)) == 2); 
    str.appendCP(
            makeUtf16ToCpInputIterator(getUtf16EuRangeBegin(range), getUtf16EuRangeEnd(range)), 
            makeUtf16ToCpInputIterator(getUtf16EuRangeEnd(  range), getUtf16EuRangeEnd(range))
            ); 
    return str; 
}

template <typename allocator_type>
template <typename Iter>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf16(
    Iter const& begin_, Iter const& end_, allocator_type const& alloc)
{
    JSTATICASSERT(sizeof( * begin_) == 2); 
    BasicUtf8String str(alloc);
    Utf16ToCpInputIterator<Iter> a1(begin_, end_); 
    Utf16ToCpInputIterator<Iter> a2(end_,   end_); 
    str.appendCP(a1, a2); 
    return str; 
}

template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::cp(
    CodePoint cp, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    str.appendCP(cp); 
    return str; 
}

template <typename allocator_type>
template <typename Range>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::cp(
    Range const& range, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    using jjm::Internal::getCpRangeBegin; 
    using jjm::Internal::getCpRangeEnd; 
    JSTATICASSERT(sizeof( * getCpRangeBegin(range)) == 4); 
    str.appendCP(getCpRangeBegin(range), getCpRangeEnd(range)); 
    return str; 
}

template <typename allocator_type>
template <typename Iter>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::cp(
    Iter const& begin_, Iter const& end_, allocator_type const& alloc)
{
    return cp(std::pair<Iter, Iter>(begin_, end_)); 
}


template <typename allocator_type>
BasicUtf8String<allocator_type>::BasicUtf8String(allocator_type const& alloc)
    : m_begin(0), m_endSize(0), m_endCapacity(0), m_alloc(alloc)
{}

template <typename allocator_type>
BasicUtf8String<allocator_type>::BasicUtf8String(BasicUtf8String const& x)
    : m_begin(0), m_endSize(0), m_endCapacity(0), m_alloc(x.m_alloc)
{
    if (x.sizeEU())
    {
        reserveEUInternal(x.sizeEU() + 1);
        std::memcpy(m_begin, x.m_begin, x.sizeBytes());
        m_endSize = m_begin + x.sizeEU();
        *m_endSize = 0;
    }
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::operator= (BasicUtf8String const& x)
{
    BasicUtf8String(x).swap(*this);
    return *this;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>::~BasicUtf8String()
{
    if (m_begin)
        m_alloc.deallocate(m_begin, capacityEU());
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::reserveEU(std::size_t n)
{
    if (capacityEU() < n)
    {
        reserveEUInternal(n);
        std::memset(m_begin + sizeEU(), 0, (m_endCapacity - m_endSize) * sizeof(EncodingUnit));
    }
    return *this;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::reserveEUInternal(std::size_t n)
{
    if (capacityEU() < n)
    {
        BasicUtf8String newstr(m_alloc);
        newstr.m_begin = newstr.m_alloc.allocate(n);
        newstr.m_endSize = newstr.m_begin + sizeEU();
        newstr.m_endCapacity = newstr.m_begin + n;
        if (sizeEU())
            std::memcpy(newstr.m_begin, m_begin, sizeBytes());
        swap(newstr);
    }
    return *this;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>&
BasicUtf8String<allocator_type>::insert(CpIterator at, BasicUtf8String const& str)
{
    if (str.sizeEU() > 0)
    {
        size_t const atEuIndex = at.getIter() - m_begin;
        if (sizeEU() + str.sizeEU() + 1 > capacityEU())
        {
            BasicUtf8String newstr(m_alloc);
            newstr.reserveEUInternal(sizeEU() + str.sizeEU() + 1);
            newstr.appendEU(m_begin, m_begin + atEuIndex);
            newstr.append(str);
            newstr.appendEU(m_begin + atEuIndex, m_endSize);
            *newstr.m_endSize = 0;
            swap(newstr);
        }
        else
        {
            std::memmove(
                m_begin + atEuIndex + str.sizeEU(),
                m_begin + atEuIndex,
                (sizeEU() - atEuIndex) * sizeof(EncodingUnit));
            std::memcpy(
                m_begin + atEuIndex,
                str.data(),
                str.sizeBytes());
            m_endSize += str.sizeEU();
        }
    }
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf8String<allocator_type>&  BasicUtf8String<allocator_type>::insert(
    CpIterator at, CpIter begin_, CpIter last_)
{
    //TODO is there a better implementation with strong exception safety?
    return insert(at, BasicUtf8String(begin_, last_));
}

template <typename allocator_type>
void BasicUtf8String<allocator_type>::erase(CpIterator beginToErase, CpIterator endToErase)
{
    EncodingUnit * beginToErase1 = m_begin + (beginToErase.getIter() - m_begin);
    EncodingUnit * endToErase1 = m_begin + (endToErase.getIter() - m_begin);
    std::memmove(beginToErase1, endToErase1, (m_endSize - endToErase1) * sizeof(EncodingUnit));
    std::memset(m_endSize - (endToErase1 - beginToErase1), 0, (endToErase1 - beginToErase1) * sizeof(EncodingUnit));
    m_endSize -= (endToErase1 - beginToErase1);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::append(BasicUtf8String str)
{
    return insert(endCP(), str);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendCP(CodePoint const cp)
{
    int const len = jjm::utf8LengthOf(cp);
    if (sizeEU() + len + 1 >= capacityEU())
        reserveEU((capacityEU() >> 1) + capacityEU() + 16); //MAGIC NUMBERS, do 1.5 bigger plus 16
    EncodingUnit* tmp = m_begin + sizeEU();
    jjm::writeUtf8(cp, tmp);
    m_endSize += len;
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendCP(CpIter begin_, CpIter end_)
{
    for (; begin_ != end_; ++begin_)
        appendCP(*begin_);
    return *this;
}

namespace Internal
{
    template <typename allocator_type, typename CpRange, bool isNumeric>
    struct AppendUtf8CpRangeHelper;

    template <typename allocator_type, typename CpRange>
    struct AppendUtf8CpRangeHelper<allocator_type, CpRange, true>
    {
        void operator() (BasicUtf8String<allocator_type> & str, CpRange r)
        {
            str.appendCP(static_cast<CodePoint>(r));
        }
    };

    template <typename allocator_type, typename CpRange>
    struct AppendUtf8CpRangeHelper<allocator_type, CpRange, false>
    {
        void operator() (BasicUtf8String<allocator_type> & str, CpRange r)
        {
            str.appendCP(r.beginCP(), r.endCP());
        }
    };
}

template <typename allocator_type>
template <typename CpRange>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendCP(CpRange r)
{
    Internal::AppendUtf8CpRangeHelper<
        allocator_type, CpRange, std::numeric_limits<CpRange>::is_specialized>
        ()(*this, r);
    return *this;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendEU(EncodingUnit x)
{
    EncodingUnit y[2] = { x, 0 };
    appendEU(y, y + 1); 
    return *this;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendEU(
    EncodingUnit const* x, EncodingUnit const* y)
{
    if (y - x)
    {
        reserveEUInternal(sizeEU() + (y - x) + 1);
        std::memcpy(m_begin + sizeEU(), x, (y - x) * sizeof(EncodingUnit));
        m_endSize += (y - x);
        *m_endSize = 0;
    }
    return *this;
}

template <typename allocator_type>
template <typename EuIter>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendEU(
    EuIter x, EuIter y)
{
    int const isRandomAccessIter = IsConvertibleTo<std::iterator_traits<EuIter>::iterator_category*, std::random_access_iterator_tag*>::b; 
    if (isRandomAccessIter)
    {   typename std::iterator_traits<EuIter>::difference_type inputRangeLength = 
                Internal::JustringIterDiff<EuIter, std::iterator_traits<EuIter>::iterator_category>()(y, x);
        reserveEUInternal(sizeEU() + inputRangeLength + 1);
        for ( ; x != y; )
        {   *m_endSize = *x; 
            ++x;
            ++m_endSize; 
        }
        *m_endSize = 0;
    }else
    {   for ( ; x != y; )
        {   if (sizeEU() + 2 >= capacityEU())
                reserveEU((capacityEU() >> 1) + capacityEU() + 16); //MAGIC NUMBERS, do 1.5 bigger plus 16
            *m_endSize = *x; 
            ++x;
            ++m_endSize; 
        }
        *m_endSize = 0;
    }
    return *this;
}

template <typename allocator_type>
void BasicUtf8String<allocator_type>::swap(BasicUtf8String & str)
{
    using std::swap;
    swap(m_begin, str.m_begin);
    swap(m_endSize, str.m_endSize);
    swap(m_endCapacity, str.m_endCapacity);
    swap(m_alloc, str.m_alloc);
}

template <typename allocator_type>
typename BasicUtf8String<allocator_type>::CpIterator
BasicUtf8String<allocator_type>::cbeginCP() const
{
    return jjm::Utf8ToCpBidiIterator<EuIterator>(beginEU(), beginEU(), endEU()); 
}

template <typename allocator_type>
typename BasicUtf8String<allocator_type>::CpIterator
BasicUtf8String<allocator_type>::cendCP() const
{
    return jjm::Utf8ToCpBidiIterator<EuIterator>(beginEU(), endEU(), endEU());
}

template <typename allocator_type>
inline BasicUtf8String<allocator_type>  operator+  (
    BasicUtf8String<allocator_type> const& a, BasicUtf8String<allocator_type> const& b)
{
    BasicUtf8String<allocator_type> x(a);
    x.append(b);
    return x;
}

template <typename alloc1, typename alloc2>
inline bool operator== (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b)
{
    return a.sizeEU() == b.sizeEU() && std::equal(a.data(), a.data() + a.sizeEU(), b.data());
}

template <typename alloc1, typename alloc2>
inline bool operator!= (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b)
{
    return !(a == b);
}

template <typename alloc1, typename alloc2>
inline bool operator<  (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b)
{
    return std::lexicographical_compare(a.data(), a.data() + a.sizeEU(), b.data(), b.data() + b.sizeEU());
}

template <typename alloc1, typename alloc2>
inline bool operator<= (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b)
{
    return !(b<a);
}

template <typename alloc1, typename alloc2>
inline bool operator>(BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b)
{
    return b<a;
}

template <typename alloc1, typename alloc2>
inline bool operator>= (BasicUtf8String<alloc1> const& a, BasicUtf8String<alloc2> const& b)
{
    return !(a<b);
}





template <typename allocator_type>
template <typename Range>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf8(
    Range const& range, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    using jjm::Internal::getUtf8RangeBegin; 
    using jjm::Internal::getUtf8RangeEnd; 
    Utf8ToCpInputIterator<Iter> a1(getUtf8RangeBegin(range), getUtf8RangeEnd(range)); 
    Utf8ToCpInputIterator<Iter> a2(getUtf8RangeEnd(  range), getUtf8RangeEnd(range)); 
    JSTATICASSERT(sizeof( * getUtf8RangeBegin(range)) == 1); 
    str.appendCP(a1, a2); 
    return str; 
}

template <typename allocator_type>
template <typename Iter>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf8(
    Iter const& begin_, Iter const& end_, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    Utf8ToCpInputIterator<Iter> a1(begin_, end_); 
    Utf8ToCpInputIterator<Iter> a2(end_,   end_); 
    JSTATICASSERT(sizeof( * begin_) == 1); 
    str.appendCP(a1, a2); 
    return str; 
}

template <typename allocator_type>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf16(
    EncodingUnit const* nullTerm, allocator_type const& alloc)
{
    BasicUtf16String str(alloc); 
    auto range = jjm::makeNullTermRange(nullTerm); 
    JSTATICASSERT(sizeof( * range.first) == 2); 
    str.appendEU(range.first, range.second); 
    return str; 
}

template <typename allocator_type>
template <typename Range>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf16(
    Range const& range, allocator_type const& alloc)
{
    BasicUtf16String str(alloc); 
    using jjm::Internal::getUtf16EuRangeBegin; 
    using jjm::Internal::getUtf16EuRangeEnd; 
    JSTATICASSERT(sizeof( * getUtf16EuRangeBegin(range)) == 2); 
    str.appendEU(getUtf16EuRangeBegin(range), getUtf16EuRangeEnd(range)); 
    return str; 
}

template <typename allocator_type>
template <typename Iter>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf16(
    Iter const& begin_, Iter const& end_, allocator_type const& alloc)
{
    BasicUtf16String str(alloc); 
    JSTATICASSERT(sizeof( * begin_) == 2); 
    str.appendEU(begin_, end_); 
    return str; 
}

template <typename allocator_type>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::cp(
    CodePoint cp, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    str.appendCP(cp); 
    return str; 
}

template <typename allocator_type>
template <typename Range>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::cp(
    Range const& range, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    using jjm::Internal::getCpRangeBegin; 
    using jjm::Internal::getCpRangeEnd;  
    JSTATICASSERT(sizeof( * getCpRangeBegin(range)) == 4); 
    str.appendCP(getCpRangeBegin(range), getCpRangeEnd(range));  
    return str; 
}

template <typename allocator_type>
template <typename Iter>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::cp(
    Iter const& begin_, Iter const& end_, allocator_type const& alloc)
{
    return cp(std::pair<Iter, Iter>(begin_, end_)); 
}

template <typename allocator_type>
BasicUtf16String<allocator_type>::BasicUtf16String(allocator_type const& alloc)
    : m_begin(0), m_endSize(0), m_endCapacity(0), m_alloc(alloc)
{}

template <typename allocator_type>
BasicUtf16String<allocator_type>::BasicUtf16String(BasicUtf16String const& x)
    : m_begin(0), m_endSize(0), m_endCapacity(0), m_alloc(x.m_alloc)
{
    if (x.sizeEU())
    {
        reserveEUInternal(x.sizeEU() + 1);
        std::memcpy(m_begin, x.m_begin, x.sizeBytes());
        m_endSize = m_begin + x.sizeEU();
        *m_endSize = 0;
    }
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::operator= (BasicUtf16String const& x)
{
    BasicUtf16String(x).swap(*this);
    return *this;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>::~BasicUtf16String()
{
    if (m_begin)
        m_alloc.deallocate(m_begin, capacityEU());
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::reserveEU(std::size_t n)
{
    if (capacityEU() < n)
    {
        reserveEUInternal(n);
        std::memset(m_begin + sizeEU(), 0, (m_endCapacity - m_endSize) * sizeof(EncodingUnit));
    }
    return *this;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::reserveEUInternal(std::size_t n)
{
    if (capacityEU() < n)
    {
        BasicUtf16String newstr(m_alloc);
        newstr.m_begin = newstr.m_alloc.allocate(n);
        newstr.m_endSize = newstr.m_begin + sizeEU();
        newstr.m_endCapacity = newstr.m_begin + n;
        if (sizeEU())
            std::memcpy(newstr.m_begin, m_begin, sizeBytes());
        swap(newstr);
    }
    return *this;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>&
BasicUtf16String<allocator_type>::insert(CpIterator at, BasicUtf16String const& str)
{
    if (str.sizeEU() > 0)
    {
        size_t const atEuIndex = at.getIter() - m_begin;
        if (sizeEU() + str.sizeEU() + 1 > capacityEU())
        {
            BasicUtf16String newstr(m_alloc);
            newstr.reserveEUInternal(sizeEU() + str.sizeEU() + 1);
            newstr.appendEU(m_begin, m_begin + atEuIndex);
            newstr.append(str);
            newstr.appendEU(m_begin + atEuIndex, m_endSize);
            *newstr.m_endSize = 0;
            swap(newstr);
        }
        else
        {
            std::memmove(
                m_begin + atEuIndex + str.sizeEU(),
                m_begin + atEuIndex,
                (sizeEU() - atEuIndex) * sizeof(EncodingUnit));
            std::memcpy(
                m_begin + atEuIndex,
                str.data(),
                str.sizeBytes());
            m_endSize += str.sizeEU();
        }
    }
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf16String<allocator_type>&  BasicUtf16String<allocator_type>::insert(
    CpIterator at, CpIter begin_, CpIter last_)
{
    //TODO is there a better implementation with strong exception safety?
    return insert(at, BasicUtf16String(begin_, last_));
}

template <typename allocator_type>
void BasicUtf16String<allocator_type>::erase(CpIterator beginToErase, CpIterator endToErase)
{
    EncodingUnit * beginToErase1 = m_begin + (beginToErase.getIter() - m_begin);
    EncodingUnit * endToErase1 = m_begin + (endToErase.getIter() - m_begin);
    std::memmove(beginToErase1, endToErase1, (m_endSize - endToErase1) * sizeof(EncodingUnit));
    std::memset(m_endSize - (endToErase1 - beginToErase1), 0, (endToErase1 - beginToErase1) * sizeof(EncodingUnit));
    m_endSize -= (endToErase1 - beginToErase1);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::append(BasicUtf16String str)
{
    return insert(endCP(), str);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendCP(CodePoint const cp)
{
    int const len = jjm::utf16LengthOf(cp);
    if (sizeEU() + len + 1 >= capacityEU())
        reserveEU((capacityEU() >> 1) + capacityEU() + 16); //MAGIC NUMBERS, do 1.5 bigger plus 16
    EncodingUnit* tmp = m_begin + sizeEU();
    jjm::writeUtf16(cp, tmp);
    m_endSize += len;
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendCP(CpIter begin_, CpIter end_)
{
    for (; begin_ != end_; ++begin_)
        appendCP(*begin_);
    return *this;
}

namespace Internal
{
    template <typename allocator_type, typename CpRange, bool isNumeric>
    struct AppendUtf16CpRangeHelper;

    template <typename allocator_type, typename CpRange>
    struct AppendUtf16CpRangeHelper<allocator_type, CpRange, true>
    {
        void operator() (BasicUtf16String<allocator_type> & str, CpRange r)
        {
            str.appendCP(static_cast<CodePoint>(r));
        }
    };

    template <typename allocator_type, typename CpRange>
    struct AppendUtf16CpRangeHelper<allocator_type, CpRange, false>
    {
        void operator() (BasicUtf16String<allocator_type> & str, CpRange r)
        {
            str.appendCP(r.beginCP(), r.endCP());
        }
    };
}

template <typename allocator_type>
template <typename CpRange>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendCP(CpRange r)
{
    Internal::AppendUtf16CpRangeHelper<
        allocator_type, CpRange, std::numeric_limits<CpRange>::is_specialized>
        ()(*this, r);
    return *this;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendEU(EncodingUnit x)
{
    EncodingUnit y[2] = { x, 0};
    appendEU(y, y + 1);
    return *this;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendEU(
    EncodingUnit const* x, EncodingUnit const* y)
{
    if (y - x)
    {   reserveEUInternal(sizeEU() + (y - x) + 1);
        std::memcpy(m_begin + sizeEU(), x, (y - x) * sizeof(EncodingUnit));
        m_endSize += (y - x);
        *m_endSize = 0;
    }
    return *this;
}

template <typename allocator_type>
template <typename EuIter>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendEU(
    EuIter x, EuIter y)
{
    int const isRandomAccessIter = IsConvertibleTo<std::iterator_traits<EuIter>::iterator_category*, std::random_access_iterator_tag*>::b; 
    if (isRandomAccessIter)
    {   typename std::iterator_traits<EuIter>::difference_type inputRangeLength = 
                Internal::JustringIterDiff<EuIter, std::iterator_traits<EuIter>::iterator_category>()(y, x);
        reserveEUInternal(sizeEU() + inputRangeLength + 1);
        for ( ; x != y; )
        {   *m_endSize = *x; 
            ++x;
            ++m_endSize; 
        }
        *m_endSize = 0;
    }else
    {   for ( ; x != y; )
        {   if (sizeEU() + 2 >= capacityEU())
                reserveEU((capacityEU() >> 1) + capacityEU() + 16); //MAGIC NUMBERS, do 1.5 bigger plus 16
            *m_endSize = *x; 
            ++x;
            ++m_endSize; 
        }
        *m_endSize = 0;
    }
    return *this;
}

template <typename allocator_type>
void BasicUtf16String<allocator_type>::swap(BasicUtf16String & str)
{
    using std::swap;
    swap(m_begin, str.m_begin);
    swap(m_endSize, str.m_endSize);
    swap(m_endCapacity, str.m_endCapacity);
    swap(m_alloc, str.m_alloc);
}

template <typename allocator_type>
typename BasicUtf16String<allocator_type>::CpIterator
BasicUtf16String<allocator_type>::cbeginCP() const
{
    return jjm::Utf16ToCpBidiIterator<EuIterator>(beginEU(), beginEU(), endEU());
}

template <typename allocator_type>
typename BasicUtf16String<allocator_type>::CpIterator
BasicUtf16String<allocator_type>::cendCP() const
{
    return jjm::Utf16ToCpBidiIterator<EuIterator>(beginEU(), endEU(), endEU());
}

template <typename allocator_type>
inline BasicUtf16String<allocator_type>  operator+  (
    BasicUtf16String<allocator_type> const& a, BasicUtf16String<allocator_type> const& b)
{
    BasicUtf16String<allocator_type> x(a);
    x.append(b);
    return x;
}

template <typename alloc1, typename alloc2>
inline bool operator== (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b)
{
    return a.sizeEU() == b.sizeEU() && std::equal(a.data(), a.data() + a.sizeEU(), b.data());
}

template <typename alloc1, typename alloc2>
inline bool operator!= (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b)
{
    return !(a == b);
}

template <typename alloc1, typename alloc2>
inline bool operator<  (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b)
{
    return std::lexicographical_compare(a.data(), a.data() + a.sizeEU(), b.data(), b.data() + b.sizeEU());
}

template <typename alloc1, typename alloc2>
inline bool operator<= (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b)
{
    return !(b<a);
}

template <typename alloc1, typename alloc2>
inline bool operator>(BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b)
{
    return b<a;
}

template <typename alloc1, typename alloc2>
inline bool operator>= (BasicUtf16String<alloc1> const& a, BasicUtf16String<alloc2> const& b)
{
    return !(a<b);
}





inline Utf8String a2u8(char const * ascii)
{
    return U8Str::utf8(ascii);
}

inline Utf8String a2u8(std::string const& ascii)
{
    return U8Str::utf8(ascii.data(), ascii.data() + ascii.size());
}

template <std::size_t n>
inline Utf8String a2u8(char const (&ascii)[n])
{
    return U8Str::utf8(ascii, ascii + n - 1);
}

inline Utf16String a2u16(char const * ascii)
{
    return U16Str::cp(a2u8(ascii));
}

inline Utf16String a2u16(std::string const& ascii)
{
    return U16Str::cp(a2u8(ascii));
}

template <std::size_t n>
Utf16String a2u16(char const (&ascii)[n])
{
    return U16Str::cp(a2u8(ascii));
}




}//namespace jjm

#endif
