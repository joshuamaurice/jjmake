// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jenv.hpp"

#include "jbase/jfatal.hpp"
#include "junicode/jutfstring.hpp"

#include <stdlib.h>

using namespace jjm;
using namespace std;


#ifdef _WIN32
    namespace { bool initMultiByteEnviron = ( getenv("FOO"), false); }
    namespace { bool initWideCharEnviron = ( _wgetenv(L"FOO"), false); }
#endif

#ifdef _WIN32
    map<jjm::Utf8String, jjm::Utf8String>  jjm::getEnvMapUtf8()
    {
        map<Utf8String, Utf8String> result;
        for (wchar_t const * const * x = _wenviron; *x; ++x)
        {   wchar_t const * const y1 = *x;
            wchar_t const * y2 = y1;
            for ( ; *y2 != '\0' && *y2 != '='; ++y2)
                ;
            if (*y2 == '\0')
                JFATAL(0, makeU16StrFromUtf16(make_pair(y1, y2))); 
            
            Utf8String name = makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(y1, y2))); 
            Utf8String value = makeU8StrFromCpRange(makeCpRangeFromUtf16(makeNullTermRange(y2+1))); 
            result[name] = value; 
        }
        return result;
    }

    Utf8String jjm::getEnvVarUtf8(Utf8String const& name)
    {
        wchar_t * v = _wgetenv(makeU16Str(name).c_str()); 
        Utf8String result; 
        if (v != 0)
            result = makeU8StrFromCpRange(makeCpRangeFromUtf16(makeNullTermRange(v))); 
        return result; 
    }
    
#else

    extern char ** environ;

    map<jjm::Utf8String, jjm::Utf8String>  jjm::getEnvMapUtf8()
    {
        map<Utf8String, Utf8String> result;
        for (char const * const * x = environ; *x; ++x)
        {   char const * const y1 = *x;
            char const * y2 = y1;
            for ( ; *y2 != '\0' && *y2 != '='; ++y2)
                ;
            if (*y2 == '\0')
                JFATAL(0, y1);
            //TODO encoding
            result[Utf8String(y1, y2)] = Utf8String(y2+1); 
        }
        return result;
    }
    
    map<jjm::Utf8String, jjm::Utf8String>  jjm::getEnvMapLocal()
    {
        map<Utf8String, Utf8String> result;
        for (char const * const * x = environ; *x; ++x)
        {   char const * const y1 = *x;
            char const * y2 = y1;
            for ( ; *y2 != '\0' && *y2 != '='; ++y2)
                ;
            if (*y2 == '\0')
                JFATAL(0, y1);
            //TODO encoding
            result[Utf8String(y1, y2)] = Utf8String(y2+1); 
        }
        return result;
    }
    
    Utf8String jjm::getEnvVarUtf8(Utf8String const& name)
    {
        Utf8String result; 
        char * v = getenv(name.c_str()); 
        if (v != 0)
        {   //TODO encoding
            result = v; 
        }
        return result; 
    }
    
    string jjm::getEnvVarLocal(Utf8String const& name)
    {
        string result; 
        char * v = getenv(name.c_str()); 
        if (v != 0)
        {   //TODO encoding
            result = v; 
        }
        return result; 
    }

#endif
