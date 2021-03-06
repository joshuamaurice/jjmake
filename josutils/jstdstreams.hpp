// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JSTDSTREAMS_HPP_HEADER_GUARD
#define JOSUTILS_JSTDSTREAMS_HPP_HEADER_GUARD

#include "jfilehandle.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jstreams.hpp"
#include "jbase/jinttostring.hpp"
#include "junicode/jutfstring.hpp"
#include <string>
#include <cstring>

#ifdef _WIN32
    typedef  unsigned long  DWORD; 
#endif

namespace jjm
{
    
//**** **** **** **** 
//** Private Implementation
namespace Internal
{
    BufferedInputStream  *  createJin(); 
    BufferedOutputStream *  createJout(); 
    BufferedOutputStream *  createJerr(); 
    BufferedOutputStream *  createJlog(); 
}

//**** **** **** **** 
//** Public APIs


/*
These are meant as replacements to std::in, std::out, std::err
to work with Unicode and localization. 

The APIs are in terms of UTF-8 strings. jin returns UTF-8 strings. jout, jerr,
and jlog take input UTF-8 strings. These objects do The Right Thing regarding
encodings for dealing with the outside world. 

..

Doing The Right Thing on POSIX: 

It follows the convention of assuming the encoding obtained from 
setlocale(LC_ALL, "") is the encoding of the outside world. It uses iconv to 
convert to/from UTF-8 and this encoding of the outside world. 

Note: This implementation will invoke setlocale(LC_ALL, "") before main() 
starts. 

..

Doing The Right Thing on Windows: 

If the std handle is connected to a terminal, then it uses ReadConsoleW /
WriteConsoleW to read and write in UTF-16. If the file is redirected to a file
or piped to another program, then this rule does not apply. 

Otherwise, we match the convention used on Windows console programs which is 
to use the current Windows-OEM encoding, a thing which is distinct from the 
current Windows-ANSI encoding. setlocale(LC_ALL, "") will return the current 
Windows-ANSI encoding, which is not what we want. Instead, we obtain the 
current Windows-OEM encoding by calling: 
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IDEFAULTCODEPAGE, ...);

..

TODO details buffering and automatic flushing
*/
inline BufferedInputStream  &  jin()  { static BufferedInputStream  * x = 0; if (x == 0) x = Internal::createJin();  return *x; }
inline BufferedOutputStream &  jout() { static BufferedOutputStream * x = 0; if (x == 0) x = Internal::createJout(); return *x; }
inline BufferedOutputStream &  jerr() { static BufferedOutputStream * x = 0; if (x == 0) x = Internal::createJerr(); return *x; }
inline BufferedOutputStream &  jlog() { static BufferedOutputStream * x = 0; if (x == 0) x = Internal::createJlog(); return *x; }
namespace { bool jjmForceInitJin  = (jin(),  false); }
namespace { bool jjmForceInitJout = (jout(), false); }
namespace { bool jjmForceInitJerr = (jerr(), false); }
namespace { bool jjmForceInitJlog = (jlog(), false); }

/* setJinEncoding(), setJoutEncoding(), setJerrEncoding() 

These functions are no-ops if it is detected that the relevant std handle 
(stdin, stdout, stderr) is directedly connected to a Windows console. In that 
case, WriteConsoleW is always used. 

Note that no synchronization is done. It is not safe to call these functions
when jin(), jout(), jerr(), jlog() might be in use by another thread. It is 
suggested that these should be called only by main before any threads are 
created. */
void setJinEncoding (std::string const& encoding);
void setJoutEncoding(std::string const& encoding); 
void setJerrEncoding(std::string const& encoding); //also sets the encoding of jlog


inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, Utf8String const& str) { return out.write(str.data(), str.size());  }
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, Utf16String const& str) { return out << makeU8Str(str);  }
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, char const * t) { return out.write(t, std::strlen(t)); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, char c) { char buf[2] = { c, 0 }; return out.write(buf, 1); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, unsigned char c) { char buf[2] = { (char)c, 0 }; return out.write(buf, 1); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, signed char c) { char buf[2] = { (char)c, 0 }; return out.write(buf, 1); } 
template <size_t N> BufferedOutputStream &  operator<< (BufferedOutputStream & out, char const (&str)[N]) { return out.write(str, N-1); }

inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, BufferedOutputStream& (*f)(BufferedOutputStream& ) ) { return f(out); } 

inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, short x) { return out << jjm::toDecStr(x); } //TODO could be more performant
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, unsigned short x) { return out << jjm::toDecStr(x); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, int x) { return out << jjm::toDecStr(x); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, unsigned int x) { return out << jjm::toDecStr(x); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, long x) { return out << jjm::toDecStr(x); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, unsigned long x) { return out << jjm::toDecStr(x); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, long long x) { return out << jjm::toDecStr(x); } 
inline BufferedOutputStream &  operator<< (BufferedOutputStream & out, unsigned long long x) { return out << jjm::toDecStr(x); } 
//TODO float, double, long double

inline BufferedOutputStream &  endl(BufferedOutputStream & out)
{
    out.write("\n", 1); 
    out.flush(); 
    return out; 
}

inline BufferedOutputStream &  flush(BufferedOutputStream & out)
{
    out.flush(); 
    return out; 
}

inline BufferedInputStream &  getline(BufferedInputStream & in, Utf8String & str)
{
    for (;;)
    {   char c[1]; 
        in.read(c, 1);
        if (!in)
            return in; 
        if (*c == '\n')
            return in; 
        str.push_back(*c); 
    }
}


//**** **** **** **** 
//** Private Implementation
namespace Internal
{
//try to flush jout, jerr, jlog at the end of the process
//Uses logic similar to Schwarz counters e.g. nifty counters
class ForceStdOutFlush { public: ForceStdOutFlush(); ~ForceStdOutFlush(); };
namespace { ForceStdOutFlush forceStdOutFlush; }
class ForceStdErrFlush { public: ForceStdErrFlush(); ~ForceStdErrFlush(); };
namespace { ForceStdErrFlush forceStdErrFlush; }
class ForceStdLogFlush { public: ForceStdLogFlush(); ~ForceStdLogFlush(); };
namespace { ForceStdLogFlush forceStdLogFlush; }
}//namespace Internal


}//namespace jjm

#endif
