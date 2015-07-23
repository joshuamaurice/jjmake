// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jopen.hpp"

#include "jbase/jfatal.hpp"
#include "jbase/jnulltermiter.hpp"
#include "jbase/jstaticassert.hpp"

#include <sstream>
#include <stdexcept>

#include <cerrno>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <sys/stat.h>
#endif

using namespace jjm;
using namespace std;

    
namespace
{
#ifdef _WIN32
    inline DWORD makeWin32AccessFlags(int const accessFlags, bool const append)
    {
        if (accessFlags == 1) return GENERIC_READ;
        if (accessFlags == 2 && ! append) return GENERIC_WRITE;
        if (accessFlags == 2 &&   append) return FILE_APPEND_DATA | SYNCHRONIZE;
        if (accessFlags == 3 && ! append) return GENERIC_READ | GENERIC_WRITE;
        if (accessFlags == 3 &&   append) return GENERIC_READ | FILE_APPEND_DATA | SYNCHRONIZE;
        return 0;
    }

    inline DWORD makeWin32CreationFlags(int const createFlags, bool const truncate)
    {
        if (createFlags == 1 && ! truncate) return OPEN_ALWAYS;
        if (createFlags == 1 &&   truncate) return CREATE_ALWAYS;
        if (createFlags == 2) return CREATE_NEW;
        if (createFlags == 3 && ! truncate) return OPEN_EXISTING;
        if (createFlags == 3 &&  truncate) return TRUNCATE_EXISTING;
        return 0;
    }
#else
    inline int makePosixOpenFlags(int const accessMode, int const createMode, 
                bool const append, bool const truncate)
    {
        int posixopenflags = 0;
        if (accessMode == 1)
            posixopenflags = O_RDONLY;
        if (accessMode == 2 && ! append)
            posixopenflags = O_WRONLY;
        if (accessMode == 2 &&   append)
            posixopenflags = O_WRONLY | O_APPEND;
        if (accessMode == 3 && ! append)
            posixopenflags = O_RDWR;
        if (accessMode == 3 &&   append)
            posixopenflags = O_RDWR | O_APPEND;

        if (createMode == 1 && ! truncate)
            posixopenflags = O_CREAT;
        if (createMode == 1 &&   truncate)
            posixopenflags = O_CREAT | O_TRUNC;
        if (createMode == 2)
            posixopenflags = O_CREAT | O_EXCL;
        if (createMode == 3 && ! truncate)
            posixopenflags = 0;
        if (createMode == 3 &&   truncate)
            posixopenflags = O_TRUNC;

        return posixopenflags;
    }

    inline void setCloseOnExec(int const fd)
    {   
        int flags = fcntl(fd, F_GETFD);
        if (flags == -1)
            abort();
        flags |= FD_CLOEXEC;
        if (fcntl(fd, F_SETFD, flags) == -1)
            abort();
    }
#endif
}



#ifdef _WIN32
    jjm::FileHandle jjm::FileOpener::open(jjm::Path const& path) const
    {
        FileHandle const fd = open2(path);
        if (FileHandle() != fd)
            return fd;
        DWORD const lastError = GetLastError();
        string msg = string() + "jjm::FileOpener::open() failed. "
                + "CreateFileW(\"" + path.getStringRep().c_str() 
                + "\", <access flags>, 0, 0, <creation flags>, <normal file attributes>) failed. ";
        if (m_createMode == 2 && lastError == ERROR_FILE_EXISTS)
            throw runtime_error(msg + "GetLastError() ERROR_FILE_EXISTS: The file exists, and CREATE_NEW was specified.");
        if (lastError == ERROR_PATH_NOT_FOUND)
            throw runtime_error(msg + "GetLastError() ERROR_PATH_NOT_FOUND: The system cannot find the path specified.");
        if (lastError == ERROR_SHARING_VIOLATION)
            throw runtime_error(msg + "GetLastError() ERROR_SHARING_VIOLATION: The process cannot access the file because it is being used by another process.");
        if (lastError == ERROR_FILE_NOT_FOUND)
            throw runtime_error(msg + "GetLastError() ERROR_FILE_NOT_FOUND: The system cannot find the file specified.");
        if (lastError == ERROR_ACCESS_DENIED)
            throw runtime_error(msg + "GetLastError() ERROR_ACCESS_DENIED: Access is denied.");
        throw runtime_error(msg + "GetLastError returned " + toDecStr(lastError) + ".");
    }
#else
    jjm::FileHandle jjm::FileOpener::open(jjm::Path const& path) const
    {
        if (m_accessMode == 0)
            JFATAL(0, path.toStdString());
        if (m_createMode == 0)
            JFATAL(0, path.toStdString());

        errno = 0; 
        FileHandle const fd = open2(path); 
        int const lastErrno = errno;

        if (fd != FileHandle())
            return fd;

        string message;
        message += "jjm::FileOpener::open() failed. Path before localization \"" + path.toStdString() + "\". Cause:\n";
        message += "::open(<path>, <flags>, <flags>). failed. Cause:\n";
        if (EACCES == lastErrno)
            throw runtime_error(message + "errno EACCES, does not have correct permissions for file.");
        if (EISDIR == lastErrno)
            throw runtime_error(message + "errno EISDIR, cannot create file which is an existing directory.");
        if (ENOTDIR == lastErrno)
            throw runtime_error(message + "errno ENOTDIR, one of the parent directories does not exist.");
        if (EROFS == lastErrno)
            throw runtime_error(message + "errno EROFS, cannot modify read only file.");
        throw runtime_error(message + "errno " + toDecStr(lastErrno) + "."); //TODO redo getErrnoName
    }
#endif

    
#ifdef _WIN32
    jjm::FileHandle jjm::FileOpener::open2(jjm::Path const& path) const
    {
        U16Str utf16Path2; 
        for (auto cp = path.getStringRep().beginCP(); cp != path.getStringRep().endCP(); ++cp)
        {   if (*cp == '/')
                utf16Path2.appendCP('\\');
            else
                utf16Path2.appendCP(*cp);
        }

        if (m_accessMode == 0)
            JFATAL(0, U8Str::cp(utf16Path2).c_str());
        if (m_createMode == 0)
            JFATAL(0, U8Str::cp(utf16Path2).c_str());

        DWORD const win32AccessFlags = makeWin32AccessFlags(m_accessMode, m_append);
        DWORD const win32CreationFlags = makeWin32CreationFlags(m_createMode, m_truncate);
        DWORD const flagsAndAttributes = 
                FILE_ATTRIBUTE_NORMAL
                | ( m_win32_fileFlagBackupSemantics  ? (FILE_FLAG_BACKUP_SEMANTICS) : 0 )
                | ( m_win32_openSymlink ? (FILE_FLAG_OPEN_REPARSE_POINT) : 0 )
                ;
        DWORD const win32ShareMode = (m_accessMode == 1) ? FILE_SHARE_READ : 0; //allow read sharing in read-only mode

        SetLastError(0);
        errno = 0; 
        HANDLE const fileHandle = 
                CreateFileW(utf16Path2.c_str(), win32AccessFlags, win32ShareMode, 0, win32CreationFlags, flagsAndAttributes, 0);
        if (fileHandle == INVALID_HANDLE_VALUE)
            return FileHandle();
        return FileHandle(fileHandle);
    }
#else
    jjm::FileHandle jjm::FileOpener::open2(jjm::Path const& path) const
    {
        //TODO convert based on current locale and encoding
        if (m_accessMode == 0)
            JFATAL(0, path.toStdString());
        if (m_createMode == 0)
            JFATAL(0, path.toStdString());
        FileHandle const fd = open_asyncSignalSafe(path.getStringRep().c_str());
        return fd;
    }
#endif


#ifndef _WIN32
    FileHandle jjm::FileOpener::open_asyncSignalSafe(char const* path) const
    {
        errno = 0;
        if (m_accessMode == 0)
            return FileHandle();
        if (m_createMode == 0)
            return FileHandle();

        int posixopenflags = makePosixOpenFlags(m_accessMode, m_createMode, m_append, m_truncate);

        #if (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L)
            posixopenflags |= O_CLOEXEC;
        #else
            #warning TODO set close on exec flag on fd
        #endif
        
        for (;;)
        {   int const fd = ::open(path, posixopenflags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (-1 == fd)
            {   if (EINTR == errno)
                    continue;
                return FileHandle();
            }
            return FileHandle(fd);
        }
    }
#endif
