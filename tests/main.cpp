// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "junicode/jutfstring.hpp"
#include "josutils/jpath.hpp"
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
    if (x != y) \
    { \
        cerr << "Failed test at " << __FILE__ << " " << __LINE__ << ". [" << x << "], [" << y << "]" << endl; \
        failed = true; \
    }

bool operator!= (Path const& a, Path const& b) { return a.getStringRep() != b.getStringRep(); } 
ostream& operator<< (ostream& out, Path const& p) { out << p.getStringRep(); return out; }

void junicodeTests()
{
    ASSERT_EQUALS(Utf8String("foo"), makeU8StrFromUtf8(makeNullTermRange("foo")));
    ASSERT_EQUALS(Utf8String("foo"), makeU8StrFromCpRange(makeCpRangeFromUtf8(makeNullTermRange("foo")))); 

    ASSERT_EQUALS(Utf8String("foo"), makeU8Str(makeU16StrFromCpRange(makeCpRangeFromUtf8(makeNullTermRange("foo"))))); 
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

    ASSERT_EQUALS(string("foo"), Path("C:foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:foo//").getFileName());

    ASSERT_EQUALS(string("foo"), Path("C:/bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C://bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/bar//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/bar/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:/bar/foo//").getFileName());

    ASSERT_EQUALS(string("foo"), Path("C:bar/foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:bar//foo").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:bar/foo/").getFileName());
    ASSERT_EQUALS(string("foo"), Path("C:bar/foo//").getFileName());

    ASSERT_EQUALS(string(""), Path("C:").getFileName());
    ASSERT_EQUALS(string(""), Path("C:/").getFileName());
    ASSERT_EQUALS(string(""), Path("C://").getFileName());

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
