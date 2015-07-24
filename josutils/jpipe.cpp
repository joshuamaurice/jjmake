// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jpipe.hpp"

#include "jbase/jinttostring.hpp"
#include "jbase/jfatal.hpp"

#include <cerrno>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    //#include <sys/time.h>
    //#include <sys/types.h>
    //#include <sys/stat.h>
#endif


jjm::Pipe jjm::Pipe::create()
{
#ifdef _WIN32
    HANDLE handles[2];
    {
        SECURITY_ATTRIBUTES pipeSecurityAttributes; 
        ZeroMemory(&pipeSecurityAttributes, sizeof(SECURITY_ATTRIBUTES));
        pipeSecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES); 
        pipeSecurityAttributes.bInheritHandle = FALSE; 

        SetLastError(0); 
        errno = 0; 
        if ( ! CreatePipe(&handles[0], &handles[1], &pipeSecurityAttributes, 0))
        {   DWORD const lastError = GetLastError(); 
            throw std::runtime_error("jjm::Pipe::create() failed. GetLastError() " + toDecStr(lastError) + "."); 
        }
    }
    Pipe p;
    p.readable  = FileHandle(handles[0]);
    p.writeable = FileHandle(handles[1]);
    return p;
#else
    int handles[2];

    errno = 0;
    #if ((defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L) || defined(__CYGWIN__))
        int const x = ::pipe2(handles, O_CLOEXEC); 
    #else
        #warning You should upgrade your POSIX system to support O_CLOEXEC. 
        int const x = ::pipe(handles); 
    #endif

    if (x != 0)
    {   int lastErrno = errno; 
        throw std::runtime_error("jjm::Pipe::create() failed. GetLastError() " + toDecStr(lastErrno) + "."); 
    }

    Pipe p;
    p.readable  = FileHandle(handles[0]);
    p.writeable = FileHandle(handles[1]);
#if ((defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L) || defined(__CYGWIN__))
#else
    p.readable .setCloseOnExec(); 
    p.writeable.setCloseOnExec(); 
#endif
    return p;
#endif
}
