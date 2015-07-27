// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JFILESTREAMS_HPP_HEADER_GUARD
#define JOSUTILS_JFILESTREAMS_HPP_HEADER_GUARD

#include "jfilehandle.hpp"
#include "jbase/jstreams.hpp"
#include "jbase/jstdint.hpp"

#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace jjm
{

class FileStream : public jjm::InputStream, public jjm::OutputStream
{
public:
    FileStream(); 
    explicit FileStream(FileHandle handle_); 
    ~FileStream(); 

    void handle(FileHandle handle_); //calls FileHandle::close on the previous handle (if any)
    FileHandle handle() const; 
    FileHandle release(); //release ownership over the internal FileHandle

    virtual void close(); //same as FileHandle::close
    virtual std::int64_t seek(std::int64_t off, int whence); //same as FileHandle::seek2
    virtual ssize_t read(void * buf, std::size_t bytes); //same as FileHandle::read2
    virtual ssize_t write(void const* buf, std::size_t bytes); //same as FileHandle::write2
    virtual int flush() { return 0; }

    //This class saves the last error value of GetLastError() / errno 
    //to ease use of BufferedInputStream and BufferedOutputStream. 
    std::int32_t getLastError() const { return mLastError; }

private:
    FileStream(FileStream const& ); //not defined, not copyable
    FileStream& operator= (FileStream const& ); //not defined, not copyable

    FileHandle mhandle; 
    std::int32_t mLastError; 
};




//**** **** **** ****
//** Private Implementation

FileStream::FileStream() : mhandle(), mLastError(0) {}

FileStream::FileStream(FileHandle handle_) : mhandle(handle_), mLastError(0) {}

FileStream::~FileStream() { mhandle.close(); } 

void FileStream::handle(FileHandle handle_) 
{
    this->close(); 
    mhandle = handle_; 
}

FileHandle FileStream::handle() const { return mhandle; }

FileHandle FileStream::release() 
{ 
    FileHandle x = mhandle; 
    mhandle = FileHandle(); 
    return x; 
}

void FileStream::close() 
{ 
#ifdef _WIN32
    SetLastError(0);
#else
    errno = 0;
#endif
    mhandle.close(); 
#ifdef _WIN32
    mLastError = GetLastError();
#else
    mLastError = errno;
#endif
}

std::int64_t FileStream::seek(std::int64_t off, int whence) 
{ 
#ifdef _WIN32
    SetLastError(0);
#else
    errno = 0;
#endif
    return mhandle.seek(off, whence); 
#ifdef _WIN32
    mLastError = GetLastError();
#else
    mLastError = errno;
#endif
} 

ssize_t FileStream::read(void * buf, std::size_t bytes) 
{ 
#ifdef _WIN32
    SetLastError(0);
#else
    errno = 0;
#endif
    return mhandle.read(buf, bytes); 
#ifdef _WIN32
    mLastError = GetLastError();
#else
    mLastError = errno;
#endif
}

ssize_t FileStream::write(void const* buf, std::size_t bytes) 
{ 
#ifdef _WIN32
    SetLastError(0);
#else
    errno = 0;
#endif
    return mhandle.write(buf, bytes); 
#ifdef _WIN32
    mLastError = GetLastError();
#else
    mLastError = errno;
#endif
}

}//namespace jjm

#endif
