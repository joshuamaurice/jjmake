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
    //#include <sys/time.h>
    //#include <sys/types.h>
    //#include <sys/stat.h>
#endif

#ifndef _WIN32
    namespace
    {   void setCloseOnExec(int const fd) //TODO refactor into FileHandle
        {   int flags = fcntl(fd, F_GETFD);
            if (flags == -1)
                abort();
            flags |= FD_CLOEXEC;
            if (fcntl(fd, F_SETFD, flags) == -1)
                abort();
        }
    }
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

    #if (defined(__gnu_linux__) && defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L)
        errno = 0;
        int const x = ::pipe2(handles, O_CLOEXEC); 
    #else
        #error Unsupported platform. 
    #endif

    if (x != 0)
    {   int lastErrno = errno; 
        throw std::runtime_error("jjm::Pipe::create() failed. GetLastError() " + toDecStr(lastErrno) + "."); 
    }

    Pipe p;
    p.readable  = FileHandle(handles[0]);
    p.writeable = FileHandle(handles[1]);
    return p;
#endif
}
