// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JUSTRING_HPP_HEADER_GUARD
#define JUNICODE_JUSTRING_HPP_HEADER_GUARD

#include "jutfiterator.hpp"
#include "jbase/jboundscheckediterator.hpp"


namespace jjm
{
;

template <typename allocator_tye> 
class BasicUtf8String;
template <typename allocator_tye> 
class BasicUtf16String;




//Note that this class only uses the allocate() and deallocate() functions of allocator_type.
template <typename allocator_type = std::allocator<char> >
class BasicUtf8String
{
public:
    typedef allocator_type          AllocatorType;

    typedef  char                   EncodingUnit;
    typedef  unsigned char          UnsignedEncodingUnit;
    typedef  EncodingUnit const*    EuIterator;
    typedef  Utf8BidiIterator<BoundsCheckedIter<EuIterator> >    CpIterator;

    static BasicUtf8String bom()
    {
        static unsigned char const x[3] = { 0xEF, 0xBB, 0xBF };
        return utf(reinterpret_cast<char const*>(x), reinterpret_cast<char const*>(x)+3);
    }

public:
    /*The static creation functions (utf() and cp() create a copy.
    The utf functions do no validation.
    The cp functions do validate input (throws std::exception on invalid
    input).
    */
    static BasicUtf8String utf(std::string const& utf8str, allocator_type const& alloc = allocator_type());
    static BasicUtf8String utf(EncodingUnit const* nullTerminatedUtf8, allocator_type const& alloc = allocator_type());
    static BasicUtf8String utf(BoundsCheckedIter<EncodingUnit const*> nullTerminatedUtf8, allocator_type const& alloc = allocator_type());
    static BasicUtf8String utf(EncodingUnit const* firstUtf8, EncodingUnit const* lastUtf8, allocator_type const& alloc = allocator_type());
    static BasicUtf8String utf(BoundsCheckedIter<EncodingUnit const*> firstUtf8, BoundsCheckedIter<EncodingUnit const*> lastUtf8, allocator_type const& alloc = allocator_type());
    static BasicUtf8String utf(CpIterator first, CpIterator last, allocator_type const& alloc = allocator_type());
    static BasicUtf8String cp(CodePoint x, allocator_type const& alloc = allocator_type());
    template <typename CpRange> static BasicUtf8String cp(CpRange const& x, allocator_type const& alloc = allocator_type());
    template <typename CpIterator2> static BasicUtf8String cp(CpIterator2 first, CpIterator2 last, allocator_type const& alloc = allocator_type());

public:
    explicit BasicUtf8String(allocator_type const& alloc = allocator_type());
    BasicUtf8String(BasicUtf8String const& x);
    BasicUtf8String& operator= (BasicUtf8String const& x);
    ~BasicUtf8String();

    template <typename allocator_type2> BasicUtf8String(BasicUtf16String<allocator_type2> const& x, allocator_type const& alloc = allocator_type());

    //Non-empty strings guaranteed to return null-terminated string. 
    //Empty strings may return null-terminated empty string or null-pointer. 
    EncodingUnit const* c_str() const { return first; }
    EncodingUnit const* data() const { return first; }

    bool empty() const { return lastSize == first; }

    //To give fast c_str() and data(), and ensure always null terminated, 
    //it is guaranteed that: capacityEU() >= sizeEU() + 1 or capacityEU() == 0
    BasicUtf8String& reserveEU(std::size_t numEncodingUnits);

    std::size_t capacityEU() const { return lastCapacity - first; }

    void shrink_to_fit() { BasicUtf8String(*this).swap(*this); }

    std::size_t lengthBytes() const { return (lastSize - first) * sizeof(EncodingUnit); }
    std::size_t lengthEU() const { return lastSize - first; }
    std::size_t sizeBytes() const { return (lastSize - first) * sizeof(EncodingUnit); }
    std::size_t sizeEU() const { return lengthEU(); }

    void clear() { lastSize = first; }

    template <typename CpIterator2> BasicUtf8String& insert(CpIterator at, CpIterator2 first, CpIterator2 last);
    BasicUtf8String& insert(CpIterator at, BasicUtf8String const& str);

    void erase(CpIterator first, CpIterator last);

    BasicUtf8String& append(BasicUtf8String str);

    BasicUtf8String& appendCP(CodePoint cp);
    template <typename CpRange> BasicUtf8String& appendCP(CpRange r);
    template <typename CpIterator2> BasicUtf8String& appendCP(CpIterator2 first, CpIterator2 last);

    //Only add complete codepoints! Otherwise bad things may happen. 
    BasicUtf8String& appendEU(EncodingUnit const* first, EncodingUnit const* last);

    void swap(BasicUtf8String& x);

    CpIterator  beginCP() { return cbeginCP(); }
    CpIterator  beginCP() const { return cbeginCP(); }
    CpIterator cbeginCP() const;
    CpIterator  endCP() { return cendCP(); }
    CpIterator  endCP() const { return cendCP(); }
    CpIterator cendCP() const;

    EuIterator  beginEU() { return cbeginEU(); }
    EuIterator  beginEU() const { return cbeginEU(); }
    EuIterator cbeginEU() const { return first; }
    EuIterator  endEU() { return cendEU(); }
    EuIterator  endEU() const { return cendEU(); }
    EuIterator cendEU() const { return lastSize; }

    allocator_type get_allocator() const { return myalloc; }

    //Do not use this unless you know what you are doing, are familiar with 
    //utf8, and are familiar with the implementation of this class. 
    EncodingUnit* mutable_data() { return first; }

private:
    EncodingUnit* first;
    EncodingUnit* lastSize;
    EncodingUnit* lastCapacity;
    allocator_type myalloc;

    //Caller need to guarantee the invariant that everything in the range 
    //[lastSize, lastCapacity) is 0. 
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
    typedef  Utf16BidiIterator<BoundsCheckedIter<EuIterator> >    CpIterator;

    static BasicUtf16String bom()
    {
        static EncodingUnit const x = 0xFEFF;
        return utf(&x, &x + 1);
    }

public:
    /*The static creation functions (utf() and cp()) create a copy.
    The utf functions do no validation. */
    static BasicUtf16String utf(EncodingUnit const* nullTerminatedUtf16, allocator_type const& alloc = allocator_type());
    static BasicUtf16String utf(BoundsCheckedIter<EncodingUnit const*> nullTerminatedUtf16, allocator_type const& alloc = allocator_type());
    static BasicUtf16String utf(EncodingUnit const* firstUtf16, EncodingUnit const* lastUtf16, allocator_type const& alloc = allocator_type());
    static BasicUtf16String utf(BoundsCheckedIter<EncodingUnit const*> firstUtf16, BoundsCheckedIter<EncodingUnit const*> lastUtf16, allocator_type const& alloc = allocator_type());
    static BasicUtf16String utf(CpIterator first, CpIterator last, allocator_type const& alloc = allocator_type());
    static BasicUtf16String cp(CodePoint x, allocator_type const& alloc = allocator_type());
    template <typename CpRange> static BasicUtf16String cp(CpRange const& x, allocator_type const& alloc = allocator_type());
    template <typename CpIterator2> static BasicUtf16String cp(CpIterator2 first, CpIterator2 last, allocator_type const& alloc = allocator_type());

public:
    explicit BasicUtf16String(allocator_type const& alloc = allocator_type());
    BasicUtf16String(BasicUtf16String const& x);
    BasicUtf16String& operator= (BasicUtf16String const& x);
    ~BasicUtf16String();

    template <typename allocator_type2> BasicUtf16String(BasicUtf8String<allocator_type2> const& x, allocator_type const& alloc = allocator_type());
    
    //Non-empty strings guaranteed to return null-terminated string. 
    //Empty strings may return null-terminated empty string or null-pointer. 
    EncodingUnit const* c_str() const { return first; }
    EncodingUnit const* data() const { return first; }

    bool empty() const { return lastSize == first; }

    //To give fast c_str() and data(), and ensure always null terminated, 
    //it is guaranteed that: capacityEU() >= sizeEU() + 1 or capacityEU() == 0
    BasicUtf16String& reserveEU(std::size_t numEncodingUnits);

    std::size_t capacityEU() const { return lastCapacity - first; }

    void shrink_to_fit() { BasicUtf16String(*this).swap(*this); }

    std::size_t lengthBytes() const { return (lastSize - first) * sizeof(EncodingUnit); }
    std::size_t lengthEU() const { return lastSize - first; }
    std::size_t sizeBytes() const { return (lastSize - first) * sizeof(EncodingUnit); }
    std::size_t sizeEU() const { return lengthEU(); }

    void clear() { lastSize = first; }

    BasicUtf16String& insert(CpIterator at, BasicUtf16String const& str);
    template <typename CpIterator2> BasicUtf16String& insert(CpIterator at, CpIterator2 first, CpIterator2 last);

    void erase(CpIterator first, CpIterator last);

    BasicUtf16String& append(BasicUtf16String str);

    BasicUtf16String& appendCP(CodePoint cp);
    template <typename CpRange> BasicUtf16String& appendCP(CpRange r);
    template <typename CpIterator2> BasicUtf16String& appendCP(CpIterator2 first, CpIterator2 last);

    //Only add complete code point encodings! Otherwise bad things may happen. 
    BasicUtf16String& appendEU(EncodingUnit const* first, EncodingUnit const* last);

    void swap(BasicUtf16String& x);

    CpIterator  beginCP() { return cbeginCP(); }
    CpIterator  beginCP() const { return cbeginCP(); }
    CpIterator cbeginCP() const;
    CpIterator  endCP() { return cendCP(); }
    CpIterator  endCP() const { return cendCP(); }
    CpIterator cendCP() const;

    EuIterator  beginEU() { return cbeginEU(); }
    EuIterator  beginEU() const { return cbeginEU(); }
    EuIterator cbeginEU() const { return first; }
    EuIterator  endEU() { return cendEU(); }
    EuIterator  endEU() const { return cendEU(); }
    EuIterator cendEU() const { return lastSize; }

    allocator_type get_allocator() const { return myalloc; }

    //Do not use this unless you know what you are doing, are familiar with 
    //utf16, and are familiar with the implementation of this class. 
    EncodingUnit* mutable_data() { return first; }

private:
    EncodingUnit* first;
    EncodingUnit* lastSize;
    EncodingUnit* lastCapacity;
    allocator_type myalloc;

    //Caller need to guarantee the invariant that everything in the range 
    //[lastSize, lastCapacity) is 0. 
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


template <typename allocator_type>
BasicUtf8String<allocator_type> BasicUtf8String<allocator_type>::utf(
    std::string const& utf8str, allocator_type const& alloc)
{
    return utf(utf8str.c_str(), utf8str.c_str() + utf8str.size(), alloc);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf(
    EncodingUnit const* nullTerminatedUtf8, allocator_type const& alloc)
{
    return utf(
        nullTerminatedUtf8,
        nullTerminatedUtf8 + std::strlen(nullTerminatedUtf8),
        alloc);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf(
    BoundsCheckedIter<EncodingUnit const*> nullTerminatedUtf8, allocator_type const& alloc)
{
    return utf(nullTerminatedUtf8.current(), alloc);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf(
    EncodingUnit const* first, EncodingUnit const* last, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    str.reserveEUInternal((last - first) + 1);
    std::memcpy(str.first, first, (last - first) * sizeof(EncodingUnit));
    str.lastSize = str.first + (last - first);
    *str.lastSize = 0;
    return str;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::utf(
    BoundsCheckedIter<EncodingUnit const*> first, BoundsCheckedIter<EncodingUnit const*> last, allocator_type const& alloc)
{
    return utf(first.current(), last.current(), alloc);
}

template <typename allocator_type>
BasicUtf8String<allocator_type> BasicUtf8String<allocator_type>::utf(
    CpIterator first, CpIterator last, allocator_type const& alloc)
{
    return utf(first.euIter(), last.euIter(), alloc);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::cp(
    CodePoint x, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    str.appendCP(x);
    return str;
}

template <typename allocator_type>
template <typename CpRange>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::cp(
    CpRange const& r, allocator_type const& alloc)
{
    return cp(r.beginCP(), r.endCP(), alloc);
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf8String<allocator_type>  BasicUtf8String<allocator_type>::cp(
    CpIter first, CpIter last, allocator_type const& alloc)
{
    BasicUtf8String str(alloc);
    str.appendCP(first, last);
    return str;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>::BasicUtf8String(allocator_type const& alloc)
    : first(0), lastSize(0), lastCapacity(0), myalloc(alloc)
{}

template <typename allocator_type>
BasicUtf8String<allocator_type>::BasicUtf8String(BasicUtf8String const& x)
    : first(0), lastSize(0), lastCapacity(0), myalloc(x.myalloc)
{
    if (x.sizeEU())
    {
        reserveEUInternal(x.sizeEU() + 1);
        std::memcpy(first, x.first, x.sizeBytes());
        lastSize = first + x.sizeEU();
        *lastSize = 0;
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
    if (first)
        myalloc.deallocate(first, capacityEU());
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::reserveEU(std::size_t n)
{
    if (capacityEU() < n)
    {
        reserveEUInternal(n);
        std::memset(first + sizeEU(), 0, (lastCapacity - lastSize) * sizeof(EncodingUnit));
    }
    return *this;
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::reserveEUInternal(std::size_t n)
{
    if (capacityEU() < n)
    {
        BasicUtf8String newstr(myalloc);
        newstr.first = newstr.myalloc.allocate(n);
        newstr.lastSize = newstr.first + sizeEU();
        newstr.lastCapacity = newstr.first + n;
        if (sizeEU())
            std::memcpy(newstr.first, first, sizeBytes());
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
        size_t const atEuIndex = at.euIter().current() - first;
        if (sizeEU() + str.sizeEU() + 1 > capacityEU())
        {
            BasicUtf8String newstr(myalloc);
            newstr.reserveEUInternal(sizeEU() + str.sizeEU() + 1);
            newstr.appendEU(first, first + atEuIndex);
            newstr.append(str);
            newstr.appendEU(first + atEuIndex, lastSize);
            *newstr.lastSize = 0;
            swap(newstr);
        }
        else
        {
            std::memmove(
                first + atEuIndex + str.sizeEU(),
                first + atEuIndex,
                (sizeEU() - atEuIndex) * sizeof(EncodingUnit));
            std::memcpy(
                first + atEuIndex,
                str.data(),
                str.sizeBytes());
            lastSize += str.sizeEU();
        }
    }
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf8String<allocator_type>&  BasicUtf8String<allocator_type>::insert(
    CpIterator at, CpIter firstArg, CpIter lastArg)
{
    //TODO is there a better implementation with strong exception safety?
    return insert(at, BasicUtf8String(firstArg, lastArg));
}

template <typename allocator_type>
void BasicUtf8String<allocator_type>::erase(CpIterator firstToErase, CpIterator lastToErase)
{
    EncodingUnit * firstToErase1 = first + (firstToErase.euIter().current() - first);
    EncodingUnit * lastToErase1 = first + (lastToErase.euIter().current() - first);
    std::memmove(firstToErase1, lastToErase1, (lastSize - lastToErase1) * sizeof(EncodingUnit));
    std::memset(lastSize - (lastToErase1 - firstToErase1), 0, (lastToErase1 - firstToErase1) * sizeof(EncodingUnit));
    lastSize -= (lastToErase1 - firstToErase1);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::append(BasicUtf8String str)
{
    return insert(endCP(), str);
}

template <typename allocator_type>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendCP(CodePoint const cp)
{
    int const len = ::jjm::utf8LengthOf(cp);
    if (sizeEU() + len + 1 >= capacityEU())
        reserveEU((capacityEU() >> 1) + capacityEU() + 16); //MAGIC NUMBERS, do 1.5 bigger plus 16
    EncodingUnit* tmp = first + sizeEU();
    ::jjm::writeUtf8(cp, tmp);
    lastSize += len;
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendCP(CpIter first, CpIter last)
{
    for (; first != last; ++first)
        appendCP(*first);
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
BasicUtf8String<allocator_type>& BasicUtf8String<allocator_type>::appendEU(
    EncodingUnit const* x, EncodingUnit const* y)
{
    if (y - x)
    {
        reserveEUInternal(sizeEU() + (y - x) + 1);
        std::memcpy(first + sizeEU(), x, (y - x) * sizeof(EncodingUnit));
        lastSize += (y - x);
        *lastSize = 0;
    }
    return *this;
}

template <typename allocator_type>
void BasicUtf8String<allocator_type>::swap(BasicUtf8String & str)
{
    using std::swap;
    swap(first, str.first);
    swap(lastSize, str.lastSize);
    swap(lastCapacity, str.lastCapacity);
    swap(myalloc, str.myalloc);
}

template <typename allocator_type>
typename BasicUtf8String<allocator_type>::CpIterator
BasicUtf8String<allocator_type>::cbeginCP() const
{
    return jjm::Utf8BidiIterator<jjm::BoundsCheckedIter<EuIterator> >(
        jjm::BoundsCheckedIter<EuIterator>(beginEU(), beginEU(), endEU()));
}

template <typename allocator_type>
typename BasicUtf8String<allocator_type>::CpIterator
BasicUtf8String<allocator_type>::cendCP() const
{
    return jjm::Utf8BidiIterator<jjm::BoundsCheckedIter<EuIterator> >(
        jjm::BoundsCheckedIter<EuIterator>(beginEU(), endEU(), endEU()));
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
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf(
    EncodingUnit const* nullTerminatedUtf16, allocator_type const& alloc)
{
    EncodingUnit const* x = nullTerminatedUtf16;
    for (; *x;)
        ++x;
    return utf(nullTerminatedUtf16, x, alloc);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf(
    BoundsCheckedIter<EncodingUnit const*> nullTerminatedUtf16, allocator_type const& alloc)
{
    return utf(nullTerminatedUtf16.current(), alloc);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf(
    EncodingUnit const* first, EncodingUnit const* last, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    str.reserveEUInternal((last - first) + 1);
    std::memcpy(str.first, first, (last - first) * sizeof(EncodingUnit));
    str.lastSize = str.first + (last - first);
    *str.lastSize = 0;
    return str;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::utf(
    BoundsCheckedIter<EncodingUnit const*> first, BoundsCheckedIter<EncodingUnit const*> last, allocator_type const& alloc)
{
    return utf(first.current(), last.current(), alloc);
}

template <typename allocator_type>
BasicUtf16String<allocator_type> BasicUtf16String<allocator_type>::utf(
    CpIterator first, CpIterator last, allocator_type const& alloc)
{
    return utf(first.euIter(), last.euIter(), alloc);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::cp(
    CodePoint x, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    str.appendCP(x);
    return str;
}

template <typename allocator_type>
template <typename CpRange>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::cp(
    CpRange const& r, allocator_type const& alloc)
{
    return cp(r.beginCP(), r.endCP(), alloc);
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf16String<allocator_type>  BasicUtf16String<allocator_type>::cp(
    CpIter first, CpIter last, allocator_type const& alloc)
{
    BasicUtf16String str(alloc);
    str.appendCP(first, last);
    return str;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>::BasicUtf16String(allocator_type const& alloc)
    : first(0), lastSize(0), lastCapacity(0), myalloc(alloc)
{}

template <typename allocator_type>
BasicUtf16String<allocator_type>::BasicUtf16String(BasicUtf16String const& x)
    : first(0), lastSize(0), lastCapacity(0), myalloc(x.myalloc)
{
    if (x.sizeEU())
    {
        reserveEUInternal(x.sizeEU() + 1);
        std::memcpy(first, x.first, x.sizeBytes());
        lastSize = first + x.sizeEU();
        *lastSize = 0;
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
    if (first)
        myalloc.deallocate(first, capacityEU());
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::reserveEU(std::size_t n)
{
    if (capacityEU() < n)
    {
        reserveEUInternal(n);
        std::memset(first + sizeEU(), 0, (lastCapacity - lastSize) * sizeof(EncodingUnit));
    }
    return *this;
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::reserveEUInternal(std::size_t n)
{
    if (capacityEU() < n)
    {
        BasicUtf16String newstr(myalloc);
        newstr.first = newstr.myalloc.allocate(n);
        newstr.lastSize = newstr.first + sizeEU();
        newstr.lastCapacity = newstr.first + n;
        if (sizeEU())
            std::memcpy(newstr.first, first, sizeBytes());
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
        size_t const atEuIndex = at.euIter().current() - first;
        if (sizeEU() + str.sizeEU() + 1 > capacityEU())
        {
            BasicUtf16String newstr(myalloc);
            newstr.reserveEUInternal(sizeEU() + str.sizeEU() + 1);
            newstr.appendEU(first, first + atEuIndex);
            newstr.append(str);
            newstr.appendEU(first + atEuIndex, lastSize);
            *newstr.lastSize = 0;
            swap(newstr);
        }
        else
        {
            std::memmove(
                first + atEuIndex + str.sizeEU(),
                first + atEuIndex,
                (sizeEU() - atEuIndex) * sizeof(EncodingUnit));
            std::memcpy(
                first + atEuIndex,
                str.data(),
                str.sizeBytes());
            lastSize += str.sizeEU();
        }
    }
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf16String<allocator_type>&  BasicUtf16String<allocator_type>::insert(
    CpIterator at, CpIter firstArg, CpIter lastArg)
{
    //TODO is there a better implementation with strong exception safety?
    return insert(at, BasicUtf16String(firstArg, lastArg));
}

template <typename allocator_type>
void BasicUtf16String<allocator_type>::erase(CpIterator firstToErase, CpIterator lastToErase)
{
    EncodingUnit * firstToErase1 = first + (firstToErase.euIter().current() - first);
    EncodingUnit * lastToErase1 = first + (lastToErase.euIter().current() - first);
    std::memmove(firstToErase1, lastToErase1, (lastSize - lastToErase1) * sizeof(EncodingUnit));
    std::memset(lastSize - (lastToErase1 - firstToErase1), 0, (lastToErase1 - firstToErase1) * sizeof(EncodingUnit));
    lastSize -= (lastToErase1 - firstToErase1);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::append(BasicUtf16String str)
{
    return insert(endCP(), str);
}

template <typename allocator_type>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendCP(CodePoint const cp)
{
    int const len = ::jjm::utf16LengthOf(cp);
    if (sizeEU() + len + 1 >= capacityEU())
        reserveEU((capacityEU() >> 1) + capacityEU() + 16); //MAGIC NUMBERS, do 1.5 bigger plus 16
    EncodingUnit* tmp = first + sizeEU();
    ::jjm::writeUtf16(cp, tmp);
    lastSize += len;
    return *this;
}

template <typename allocator_type>
template <typename CpIter>
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendCP(CpIter first, CpIter last)
{
    for (; first != last; ++first)
        appendCP(*first);
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
BasicUtf16String<allocator_type>& BasicUtf16String<allocator_type>::appendEU(
    EncodingUnit const* x, EncodingUnit const* y)
{
    if (y - x)
    {
        reserveEUInternal(sizeEU() + (y - x) + 1);
        std::memcpy(first + sizeEU(), x, (y - x) * sizeof(EncodingUnit));
        lastSize += (y - x);
        *lastSize = 0;
    }
    return *this;
}

template <typename allocator_type>
void BasicUtf16String<allocator_type>::swap(BasicUtf16String & str)
{
    using std::swap;
    swap(first, str.first);
    swap(lastSize, str.lastSize);
    swap(lastCapacity, str.lastCapacity);
    swap(myalloc, str.myalloc);
}

template <typename allocator_type>
typename BasicUtf16String<allocator_type>::CpIterator
BasicUtf16String<allocator_type>::cbeginCP() const
{
    return jjm::Utf16BidiIterator<jjm::BoundsCheckedIter<EuIterator> >(
        jjm::BoundsCheckedIter<EuIterator>(beginEU(), beginEU(), endEU()));
}

template <typename allocator_type>
typename BasicUtf16String<allocator_type>::CpIterator
BasicUtf16String<allocator_type>::cendCP() const
{
    return jjm::Utf16BidiIterator<jjm::BoundsCheckedIter<EuIterator> >(
        jjm::BoundsCheckedIter<EuIterator>(beginEU(), endEU(), endEU()));
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
    return Utf8String::utf(ascii);
}

inline Utf8String a2u8(std::string const& ascii)
{
    return Utf8String::utf(ascii.data(), ascii.data() + ascii.size());
}

template <std::size_t n>
inline Utf8String a2u8(char const (&ascii)[n])
{
    return Utf8String::utf(ascii, ascii + n - 1);
}

inline Utf16String a2u16(char const * ascii)
{
    return Utf16String(a2u8(ascii));
}

inline Utf16String a2u16(std::string const& ascii)
{
    return Utf16String(a2u8(ascii));
}

template <std::size_t n>
Utf16String a2u16(char const (&ascii)[n])
{
    return Utf16String(a2u8(ascii));
}




}//namespace jjm

#endif
