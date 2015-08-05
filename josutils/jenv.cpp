// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jenv.hpp"

#include "junicode/jiconv.hpp"
#include "junicode/jutfstring.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"

#include <stdlib.h>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
#endif

#ifndef _WIN32
    extern char ** environ;
#endif

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
    namespace
    {
        string & getEncodingOfEnvVars()
        {
            static string * x = 0;
            if (x == 0)
                x = new Utf8String(jjm::getEncodingNameFrom_setlocale_LC_ALL_emptyString()); 
            return *x; 
        }
    }

    void jjm::setEncodingOfEnvVars(string const& encoding)
    {
        getEncodingOfEnvVars() = encoding; 
    }

    map<jjm::Utf8String, jjm::Utf8String>  jjm::getEnvMapUtf8()
    {
        string const& encoding = getEncodingOfEnvVars(); 
        map<Utf8String, Utf8String> result;
        for (char const * const * x = environ; *x; ++x)
        {   char const * const y1 = *x;
            char const * y2 = y1;
            for ( ; *y2 != '\0' && *y2 != '='; ++y2)
                ;
            if (*y2 == '\0')
                JFATAL(0, y1);
            Utf8String transcodedValue;
            try
            {   transcodedValue = jjm::convertEncoding("UTF-8", encoding, string(y2 + 1)); 
            } catch (std::exception & e)
            {   string message;
                message += "jjm::getEnvMapUtf8() failed. Cause:\n";
                message += "Failed to transcode value of variable to UTF-8. Variable name \"" + string(y1, y2) + "\". Cause:\n";
                message += e.what();
                throw std::runtime_error(message);
            }
            result[Utf8String(y1, y2)] = transcodedValue; 
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
            result[Utf8String(y1, y2)] = Utf8String(y2+1); 
        }
        return result;
    }
    
    Utf8String jjm::getEnvVarUtf8(Utf8String const& name)
    {
        string const& encoding = getEncodingOfEnvVars(); 
        Utf8String result; 
        char * v = getenv(name.c_str()); 
        if (v != 0)
        {   try
            {   result = jjm::convertEncoding("UTF-8", encoding, string(v)); 
            } catch (std::exception & e)
            {   string message;
                message += "jjm::getEnvVarUtf8(\"" + name + "\") failed. Cause:\n";
                message += "Failed to transcode value of variable to UTF-8. Cause:\n";
                message += e.what();
                throw std::runtime_error(message);
            }
        }
        return result; 
    }
    
    string jjm::getEnvVarLocal(Utf8String const& name)
    {
        string result; 
        char * v = getenv(name.c_str()); 
        if (v != 0)
            result = v; 
        return result; 
    }

#endif

    
string jjm::getEncodingNameFrom_setlocale_LC_ALL_emptyString()
{
    char const * localeName1 = setlocale(LC_ALL, ""); 
    if (localeName1 == 0)
    {   string message;
        message += "jjm's StandardOutputStream::StandardOutputStream() failed. Cause:\n";
        message += "setlocale(LC_ALL, \"\") returned NULL."; 
        throw std::runtime_error(message); 
    }
    string encoding = localeName1; 
    if (encoding.find('.') == string::npos)
    {   string message;
        message += "jjm's StandardOutputStream::StandardOutputStream() failed. Cause:\n";
        message += string() + "The return value of setlocale(LC_ALL, \"\") -> \"" + localeName1 + "\" does not end in an encoding."; 
        throw std::runtime_error(message); 
    }
    encoding.erase(0, encoding.find('.') + 1); 
    return encoding; 
}
    
#ifdef _WIN32
    string jjm::getWindowsOemEncodingName()
    {
        //setlocale returns the Windows-ANSI codepage encoding, not the 
        //Windows-OEM encoding, which is what we want for command line 
        //applications. 
        SetLastError(0);
        size_t const oemCodePageBufferSize = 6; 
        wchar_t oemCodePageBuffer[oemCodePageBufferSize + 1]; 
        int const getLocaleInfoExResult = 
                GetLocaleInfoEx(
                        LOCALE_NAME_USER_DEFAULT, 
                        LOCALE_IDEFAULTCODEPAGE, 
                        oemCodePageBuffer, 
                        oemCodePageBufferSize
                        );
        if (getLocaleInfoExResult == 0)
        {   DWORD const lastError = GetLastError(); 
            string message;
            message += "GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IDEFAULTCODEPAGE, ...) failed. Cause:\n"; 
            message += "GetLastError() " + toDecStr(lastError) + "."; 
            throw std::runtime_error(message);
        }
        oemCodePageBuffer[getLocaleInfoExResult] = 0; 

        string encoding = "CP" + makeU8StrFromCpRange(makeCpRangeFromUtf16(
                make_pair(oemCodePageBuffer + 0, oemCodePageBuffer + getLocaleInfoExResult))); 
        return encoding; 
    }
#endif
