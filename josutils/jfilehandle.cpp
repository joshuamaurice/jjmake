// Copyright (c) 2010-2011, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jfilehandle.hpp"

#include "jbase/jinttostring.hpp"
#include "jbase/jfatal.hpp"
#include <algorithm>
#include <errno.h>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif


int jjm::FileHandle::close2()
{
    if (mhandle == invalidHandle())
        return 0; 
#ifdef _WIN32
    SetLastError(0); 
    errno = 0; 
    if (0 == ::CloseHandle(mhandle))
        return -1; 
    mhandle = invalidHandle(); 
    return 0; 
#else
    errno = 0; 
    if (0 != ::close(mhandle))
        return -1; 
    mhandle = invalidHandle(); 
    return 0;
#endif
}

void jjm::FileHandle::close()
{
    if (mhandle == invalidHandle())
        return; 
#ifdef _WIN32
    SetLastError(0); 
    errno = 0; 
    if (0 == ::CloseHandle(mhandle))
        JFATAL(0, 0);
    mhandle = invalidHandle(); 
#else
    errno = 0; 
    if (0 != ::close(mhandle))
        JFATAL(0, 0);
    mhandle = invalidHandle(); 
#endif
}

int64_t jjm::FileHandle::seek2(int64_t off, int whence)
{
#ifdef _WIN32
    LARGE_INTEGER off2;
    JSTATICASSERT(sizeof(off2.QuadPart) == sizeof(int64_t)); 
    off2.QuadPart = off; 

    LARGE_INTEGER newPosition; 
    newPosition.QuadPart = 0; 

    DWORD whence2; 
    switch (whence)
    {
    case SEEK_SET: whence2 = FILE_BEGIN; break; 
    case SEEK_CUR: whence2 = FILE_CURRENT; break; 
    case SEEK_END: whence2 = FILE_END; break; 
    default: JFATAL(0, 0); 
    }

    SetLastError(0); 
    errno = 0; 
    DWORD const x = SetFilePointerEx(mhandle, off2, & newPosition, whence2);

    if (x == 0)
        return -1; 
    return newPosition.QuadPart; 
#else
    errno = 0; 
    return ::lseek(m_fd, off, whence); 
#endif
}

int64_t jjm::FileHandle::seek(int64_t off, int whence)
{
    int64_t const x = seek2(off, whence); 
    if (x >= 0)
        return x; 
#ifdef _WIN32
    DWORD const lastError = GetLastError(); 
    throw std::runtime_error("jjm::FileHandle::seek failed. GetLastError() " + toDecStr(lastError) + "."); 
#else
    int const lastErrno = errno; 
    throw std::runtime_error("jjm::FileHandle::seek failed. errno " + toDecStr(errno) + "."); 
#endif
}


ssize_t jjm::FileHandle::read2(void * buf, size_t bytes)
{
#ifdef _WIN32
    DWORD const bytes2 = std::min<size_t>(bytes, std::numeric_limits<DWORD>::max());
    DWORD numBytesReadIntoBuffer = 0; 

    SetLastError(0); 
    errno = 0; 
    DWORD const x = ReadFile(mhandle, buf, bytes2, & numBytesReadIntoBuffer, 0); 
    DWORD const lastError = GetLastError();

    if (x != 0)
    {   if (numBytesReadIntoBuffer > 0)
            return numBytesReadIntoBuffer; 
        if (bytes > 0)
            return -1; //eof
        return 0;
    }
    if (lastError ==  ERROR_BROKEN_PIPE)
        return -1; 
    return -2; 
#else
    errno = 0; 
    ssize_t const x = ::read(m_fd, buf, bytes);
    if (x > 0)
        return x; //success
    if (x == -1 && EINTR == errno)
        return 0;
    if (x == 0 && bytes != 0)
        return -1; //eof
    if (x == 0 && bytes == 0)
        return 0; 
    return -2; 
#endif
}

ssize_t jjm::FileHandle::read(void * buf, size_t bytes)
{
    ssize_t const x = read2(buf, bytes); 
    if (x >= 0)
        return x;
    if (x == -1)
        return -1; 
#ifdef _WIN32
    DWORD const lastError = GetLastError(); 
    throw std::runtime_error("jjm::FileHandle::read failed. GetLastError() " + toDecStr(lastError) + "."); 
#else
    int const lastErrno = errno; 
    throw std::runtime_error("jjm::FileHandle::read failed. errno " + toDecStr(errno) + "."); 
#endif
}

ssize_t jjm::FileHandle::write2(void const* buf, size_t bytes)
{
#ifdef _WIN32
    DWORD const bytes2 = std::min<size_t>(bytes, std::numeric_limits<DWORD>::max());
    DWORD numBytesWrittenToFile = 0; 

    SetLastError(0); 
    errno = 0; 
    DWORD const x = WriteFile(mhandle, buf, bytes2, & numBytesWrittenToFile, 0);
    DWORD const lastError = GetLastError(); 

    if (x != 0)
        return numBytesWrittenToFile; 
    return -1; 
#else
    errno = 0; 
    ssize_t const x = ::write(m_fd, buf, bytes);
    if (x >= 0)
        return x; //success
    if (x == -1 && EINTR == errno)
        return 0;
    return -1; 
#endif
}

ssize_t jjm::FileHandle::write(void const* buf, size_t bytes)
{
    ssize_t const x = write2(buf, bytes); 
    if (x >= 0)
        return x;
#ifdef _WIN32
    DWORD const lastError = GetLastError(); 
    throw std::runtime_error("jjm::FileHandle::write failed. GetLastError() " + toDecStr(lastError) + "."); 
#else
    int const lastErrno = errno; 
    throw std::runtime_error("jjm::FileHandle::write failed. errno " + toDecStr(errno) + "."); 
#endif
}
