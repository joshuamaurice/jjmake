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
    Environmental variables API. 

    ...

    On Windows, these functions access the environment via: 
        ::_wgetenv()
        extern char ** ::_environ

    Environmental variables on Windows are always stored in UTF-16, and so 
    there is no localization nor encoding problems. 

    This API has a hook (via a constructor of a namespace scope function) to 
    invoke getenv("foo") and _wgetenv(L"foo") at the start of the process to 
    ensure that the _environ and _wenviron are initialized (as per the 
    Microsoft documentation). 

    ...

    On POSIX, these functions access the environment via: 
        ::getenv()
        extern char ** ::environ

    On POSIX, the convention is that the environmental variables are encoded
    according to the encoding of setlocale(LC_ALL, ""). This API has a hook
    (via a constructor of a namespace scope function) to obtain the encoding
    of setlocale(LC_ALL, "") by calling setlocale(LC_ALL, ""). This encoding 
    is internally stored, and it is used to convert the environmental variables
    to UTF-8 as needed. 

    This encoding can later be changed with jjm::setEncodingOfEnvVars(). Note
    that no synchronization protects the internal encoding stored by this API. 
    
    ...

    Due to the lack of synchronization and possible threading concerns, good
    practice is to change environmental variables only at the start of the 
    process inside main(), or when creating a new process via the exec() API 
    and CreateProcessW() API. 
    */

    std::map<Utf8String, Utf8String> getEnvMapUtf8(); 
    Utf8String getEnvVarUtf8(Utf8String const& name); 
#ifndef _WIN32
    void setEncodingOfEnvVars(std::string const& encoding); //used for getEnvMapUtf8 and getEnvVarUtf8
    std::map<std::string, std::string> getEnvMapLocal(); 
    std::string getEnvVarLocal(std::string const& name); 
#endif




    /* Get the encoding of the result of setlocale(LC_ALL, ""). 
    On Windows, this returns the encoding of the current Windows-ANSI code page,
    not the encoding of the current Windows-OEM code page. 
    */
    std::string getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 

#ifdef _WIN32
    /* Get the encoding of the Windows-OEM code page in a format that iconv will
    hopefully understand. */
    std::string getWindowsOemEncodingName(); 
#endif

}

#endif
