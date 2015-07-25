// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "junicode/jutfstring.hpp"
#include "josutils/jpath.hpp"
#include "jbase/jinttostring.hpp"
#include <iostream>

using namespace jjm;
using namespace std;

bool failed = false; 
void junicodeTests(); 
void jjmPathTests(); 

#ifdef _WIN32
int wmain()
#else
int main()
#endif
{
    junicodeTests(); 
    jjmPathTests(); 

    if (failed)
        return 1;
    cout << "Successful complete" << endl;
    return 0;
}


#define ASSERT_EQUALS(x, y) \
    if ((x) != (y)) \
    { \
        cerr << "Failed test at " << __FILE__ << " " << __LINE__ << ". [" << (x) << "], [" << (y) << "]" << endl; \
        failed = true; \
    }

bool operator!= (Path const& a, Path const& b) { return a.getStringRep() != b.getStringRep(); } 

template <typename Iter1, typename Iter2>
bool operator== (pair<Iter1, Iter1> const& range1, pair<Iter2, Iter2> const& range2)
{
    Iter1 i1 = range1.first;
    Iter2 i2 = range2.first; 
    for (;;)
    {   if (i1 == range1.second && i2 == range2.second)
            return true;
        if (i1 == range1.second || i2 == range2.second)
            return false;
        if ( ! (*i1 == *i2))
            return false; 
        ++i1;
        ++i2;
    }
}
template <typename Iter1, typename Iter2>
bool operator!= (pair<Iter1, Iter1> const& range1, pair<Iter2, Iter2> const& range2)
{
    return ! (range1 == range2); 
}

template <typename Iter> 
typename std::iterator_traits<Iter>::difference_type 
    size(std::pair<Iter, Iter> const& range) 
{
    typename std::iterator_traits<Iter>::difference_type d = 0; 
    for (Iter x = range.first; x != range.second; ++x)
        ++d;
    return d; 
}
size_t size(Utf8String const& x) { return x.size(); }
size_t size(Utf16String const& x) { return x.size(); }
size_t size(vector<UnicodeCodePoint> const& x) { return x.size(); }

pair<vector<UnicodeCodePoint>::const_iterator, vector<UnicodeCodePoint>::const_iterator> 
    makeRange(vector<UnicodeCodePoint> const& v) { return make_pair(v.begin(), v.end()); }

ostream& operator<< (ostream& out, Path const& p) { out << p.getStringRep(); return out; }

ostream& operator<< (ostream& out, Utf16String const& s) { out << makeU8Str(s); return out; }

ostream& operator<< (ostream& out, vector<UnicodeCodePoint> const& v) 
{ 
    out << "vector<UnicodeCodePoint> {";
    for (size_t i = 0; i < v.size(); ++i)
    {   if (i != 0)
            out << ", ";
        out << "0x" << toHexStr(v[i]);
    }
    out << "}"; 
    return out; 
}

template <typename Iter>
ostream& operator<< (ostream& out, pair<Iter, Iter> const& range) 
{ 
    out << "range {";
    bool first = true; 
    for (Iter i = range.first; i != range.second; ++i)
    {   if ( ! first)
            out << ", ";
        out << *i; 
        first = false; 
    }
    out << "}"; 
    return out; 
}


void junicodeTests()
{
    cout << "Running junicode tests" << endl;

    //input-iterators test
    Utf8String a1;
    appendCp(a1, 'x'); 
    ASSERT_EQUALS(a1.size(), 1);
    ASSERT_EQUALS(makeU16Str(a1).size(), 1);

    appendCp(a1, 0x03A3); 
    ASSERT_EQUALS(a1.size(), 1 +2);
    ASSERT_EQUALS(makeU16Str(a1).size(), 1 +1);

    appendCp(a1, 0xF901); 
    ASSERT_EQUALS(a1.size(), 1 +2 +3);
    ASSERT_EQUALS(makeU16Str(a1).size(), 1 +1 +1);

    appendCp(a1, 0x10401); 
    ASSERT_EQUALS(a1.size(), 1 +2 +3 +4);
    ASSERT_EQUALS(makeU16Str(a1).size(), 1 +1 +1 +2);

    Utf8String const u8str = a1; 
    Utf16String const u16str = makeU16Str(u8str); 
    vector<UnicodeCodePoint> const codepoints(makeCpRange(u8str).first, makeCpRange(u8str).second); 
    ASSERT_EQUALS(10, size(u8str));
    ASSERT_EQUALS(5, size(u16str));
    ASSERT_EQUALS(4, size(codepoints));

    vector<UnicodeCodePoint> nullTermCodePoints = codepoints;
    nullTermCodePoints.push_back(0); 

    //test the bidi iterator conversions
    ASSERT_EQUALS(makeRange(codepoints), makeCpRangeFromUtf8(makeUtf8Range(u8str)));
    ASSERT_EQUALS(makeRange(codepoints), makeCpRangeFromUtf16(makeUtf16Range(u16str)));

    ASSERT_EQUALS(u8str, makeU8StrFromUtf8(makeUtf8RangeFromCpRange(makeRange(codepoints)))); 
    ASSERT_EQUALS(u16str, makeU16StrFromUtf16(makeUtf16RangeFromCpRange(makeRange(codepoints)))); 

    //test the input iterator conversions
    ASSERT_EQUALS(makeRange(codepoints), makeCpRangeFromUtf8(makeNullTermRange(u8str.c_str())));
    ASSERT_EQUALS(makeRange(codepoints), makeCpRangeFromUtf16(makeNullTermRange(u16str.c_str())));

    ASSERT_EQUALS(u8str, makeU8StrFromUtf8(makeUtf8RangeFromCpRange(makeNullTermRange(&nullTermCodePoints[0]))));
    ASSERT_EQUALS(u16str, makeU16StrFromUtf16(makeUtf16RangeFromCpRange(makeNullTermRange(&nullTermCodePoints[0])))); 
}

void jjmPathTests()
{
    cout << "Running jjm::Path tests" << endl;
    //getFileName
    ASSERT_EQUALS(string("foo"), Path("C:/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C://foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C://foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/foo//").getFileName());

#ifdef _WIN32
    ASSERT_EQUALS(string("foo"), Path("C:foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:foo//").getFileName());
#else
    ASSERT_EQUALS(string("C:foo"), Path("C:foo").getFileName());
    ASSERT_EQUALS(string("C:foo"), Path("C:foo/").getFileName());
    ASSERT_EQUALS(string("C:foo"), Path("C:foo//").getFileName());
#endif

    ASSERT_EQUALS(string("foo"), Path("C:/bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C://bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/bar//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/bar/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/bar/foo//").getFileName());

    ASSERT_EQUALS(string("foo"), Path("C:bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:bar//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:bar/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:bar/foo//").getFileName());

#ifdef _WIN32
    ASSERT_EQUALS(string(""), Path("C:").getFileName());
    ASSERT_EQUALS(string(""), Path("C:/").getFileName());
    ASSERT_EQUALS(string(""), Path("C://").getFileName());
#else
    ASSERT_EQUALS(string("C:"), Path("C:").getFileName());
    ASSERT_EQUALS(string("C:"), Path("C:/").getFileName());
    ASSERT_EQUALS(string("C:"), Path("C://").getFileName());
#endif

    ASSERT_EQUALS(string("foo"), Path("/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("//foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("/foo//").getFileName());

    ASSERT_EQUALS(string("foo"), Path("foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("foo//").getFileName());

    ASSERT_EQUALS(string("foo"), Path("/bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("//bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("/bar//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("/bar/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("/bar/foo//").getFileName());

    ASSERT_EQUALS(string("foo"), Path("bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("bar//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("bar/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("bar/foo//").getFileName());

    ASSERT_EQUALS(string(""), Path("").getFileName());
    ASSERT_EQUALS(string(""), Path("/").getFileName());
    ASSERT_EQUALS(string(""), Path("//").getFileName());

    //getParent
    ASSERT_EQUALS(Path("C:/"), Path("C:/foo").getParent());
    ASSERT_EQUALS(Path("C:/"), Path("C://foo").getParent());
    ASSERT_EQUALS(Path("C:/"), Path("C:/foo/").getParent());
    ASSERT_EQUALS(Path("C:/"), Path("C://foo/").getParent());
    ASSERT_EQUALS(Path("C:/"), Path("C:/foo//").getParent());

    ASSERT_EQUALS(Path(""), Path("C:foo").getParent());
    ASSERT_EQUALS(Path(""), Path("C:foo/").getParent());
    ASSERT_EQUALS(Path(""), Path("C:foo//").getParent());

    ASSERT_EQUALS(Path("C:/bar"),  Path("C:/bar/foo").getParent());
    ASSERT_EQUALS(Path("C://bar"), Path("C://bar/foo").getParent());
    ASSERT_EQUALS(Path("C:/bar"),  Path("C:/bar//foo").getParent());
    ASSERT_EQUALS(Path("C:/bar"),  Path("C:/bar/foo/").getParent());
    ASSERT_EQUALS(Path("C:/bar"),  Path("C:/bar/foo//").getParent());

    ASSERT_EQUALS(Path("C:bar"), Path("C:bar/foo").getParent());
    ASSERT_EQUALS(Path("C:bar"), Path("C:bar//foo").getParent());
    ASSERT_EQUALS(Path("C:bar"), Path("C:bar/foo/").getParent());
    ASSERT_EQUALS(Path("C:bar"), Path("C:bar/foo//").getParent());

    ASSERT_EQUALS(Path(""), Path("C:").getParent());
    ASSERT_EQUALS(Path(""), Path("C:/").getParent());
    ASSERT_EQUALS(Path(""), Path("C://").getParent());

    ASSERT_EQUALS(Path("/"), Path("/foo").getParent());
    ASSERT_EQUALS(Path("/"), Path("//foo").getParent());
    ASSERT_EQUALS(Path("/"), Path("/foo/").getParent());
    ASSERT_EQUALS(Path("/"), Path("//foo/").getParent());
    ASSERT_EQUALS(Path("/"), Path("/foo//").getParent());

    ASSERT_EQUALS(Path(""), Path("foo").getParent());
    ASSERT_EQUALS(Path(""), Path("foo/").getParent());
    ASSERT_EQUALS(Path(""), Path("foo//").getParent());

    ASSERT_EQUALS(Path("/bar"),  Path("/bar/foo").getParent());
    ASSERT_EQUALS(Path("//bar"), Path("//bar/foo").getParent());
    ASSERT_EQUALS(Path("/bar"),  Path("/bar//foo").getParent());
    ASSERT_EQUALS(Path("/bar"),  Path("/bar/foo/").getParent());
    ASSERT_EQUALS(Path("/bar"),  Path("/bar/foo//").getParent());

    ASSERT_EQUALS(Path("bar"), Path("bar/foo").getParent());
    ASSERT_EQUALS(Path("bar"), Path("bar//foo").getParent());
    ASSERT_EQUALS(Path("bar"), Path("bar/foo/").getParent());
    ASSERT_EQUALS(Path("bar"), Path("bar/foo//").getParent());

    ASSERT_EQUALS(Path(""), Path("").getParent());
    ASSERT_EQUALS(Path(""), Path("/").getParent());
    ASSERT_EQUALS(Path(""), Path("//").getParent());
}
