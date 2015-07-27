// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JENV_HPP_HEADER_GUARD
#define JENV_HPP_HEADER_GUARD

#include "jbase/jwarningpragmas.hpp"
#include "junicode/jutfstring.hpp"

#include <map>

namespace jjm
{
    /* 
    getEnvMapUtf8() and getEnvMapLocalized()

    These functions are thin wrappers on top of 
        extern char ** ::_environ   on win32, and
        extern char ** ::environ   on POSIX. 

    getEnvMapUtf8() facilitates the common convention(?) of interpreting the 
    names and values of env vars according to the current locale as specified 
    in LC_ALL and friends, as if obtained by setlocale(LC_ALL, ""). 

    getEnvMapLocal() does not interpret the name nor value. The values are
    forwarded as-is with no modification. This function is provided only for
    POSIX systems. Because the Window implementation uses the UTF16 _environ 
    variable, you should always use getEnvMapUtf8() on windows.

    Concurrent access to env vars is not protected on windows, and doing so
    will lead to crashes. 
    */
    std::map<Utf8String, Utf8String> getEnvMapUtf8(); 
#ifndef _WIN32
    std::map<std::string, std::string> getEnvMapLocal(); 
#endif


    /*
    getEnvVarUtf8() and getEnvVarLocal()

    These functions get the value of a single variable. In terms of encodings,
    they obey the same rules as getEnvMapUtf8() and getEnvMapLocal(). 
    */
    Utf8String getEnvVarUtf8(Utf8String const& name); 
#ifndef _WIN32
    std::string getEnvVarLocal(std::string const& name); 
#endif

    /*
    On Windows, initially only one of ::environ and ::_wenviron are created. 
    The other is created on the first call to a function like 
    getenv() / _wgetenv(). 
    
    Code in the lib associated with this header will invoke 
    getenv("FOO") and _wgetenv(L"FOO") in a namespace-scope initializor to 
    ensure that both are created and available to avoid potential threading
    data races. For further reading, research the static initialization 
    order fiasco. 

    In particular, creating multiple threads before main() and accessing the 
    environment is likely to cause a crash. 
    */
}

#endif
