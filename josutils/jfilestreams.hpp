// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JFILESTREAMS_HPP_HEADER_GUARD
#define JOSUTILS_JFILESTREAMS_HPP_HEADER_GUARD

#include "jfilehandle.hpp"
#include "jbase/jstreams.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jinttostring.hpp"

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

    //calls FileHandle::close on the previous handle (if any) and takes ownership
    void handle(FileHandle handle_); 

    FileHandle handle() const; 

    //release ownership over the internal FileHandle
    FileHandle release(); 

    virtual void close(); //same as FileHandle::close
    virtual std::int64_t seek(std::int64_t off, int whence); //same as FileHandle::seek2
    virtual ssize_t read(void * buf, std::size_t bytes); //same as FileHandle::read2
    virtual ssize_t write(void const* buf, std::size_t bytes); //same as FileHandle::write2
    virtual int flush() { return 0; }

    Utf8String getLastErrorDescription() const; 

private:
    FileStream(FileStream const& ); //not defined, not copyable
    FileStream& operator= (FileStream const& ); //not defined, not copyable

    FileHandle mhandle; 
    Utf8String lastErrorDescription; 
};




//**** **** **** ****
//** Private Implementation

inline FileStream::FileStream() : mhandle() {}

inline FileStream::FileStream(FileHandle handle_) : mhandle(handle_) {}

inline FileStream::~FileStream() { mhandle.close(); } 

inline void FileStream::handle(FileHandle handle_) 
{
    lastErrorDescription.clear(); 
    this->close(); 
    mhandle = handle_; 
}

inline FileHandle FileStream::handle() const { return mhandle; }

inline FileHandle FileStream::release() 
{
    lastErrorDescription.clear(); 
    FileHandle x = mhandle; 
    mhandle = FileHandle(); 
    return x; 
}

inline void FileStream::close() 
{ 
#ifdef _WIN32
    SetLastError(0);
#else
    errno = 0;
#endif
    mhandle.close(); 
#ifdef _WIN32
    int lastError = GetLastError();
#else
    int lastErrno = errno;
#endif
    lastErrorDescription.clear(); 
    lastErrorDescription += "FileStream::close() failed. Cause:\n";
    lastErrorDescription += "FileHandle::close() failed. Cause:\n";
#ifdef _WIN32
    lastErrorDescription += "GetLastError() " + toDecStr(lastError) + "."; 
#else
    lastErrorDescription += "errno " + toDecStr(lastErrno) + "."; 
#endif
}

inline std::int64_t FileStream::seek(std::int64_t off, int whence) 
{ 
    try
    {   return mhandle.seek(off, whence); 
    } catch (std::exception & e)
    {   lastErrorDescription.clear(); 
        lastErrorDescription += "FileStream::seek() failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }
} 

inline ssize_t FileStream::read(void * buf, std::size_t bytes) 
{
    try
    {   return mhandle.read(buf, bytes); 
    } catch (std::exception & e)
    {   lastErrorDescription.clear(); 
        lastErrorDescription += "FileStream::read() failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -2; 
    }
}

inline ssize_t FileStream::write(void const* buf, std::size_t bytes) 
{ 
    try
    {   return mhandle.write(buf, bytes); 
    } catch (std::exception & e)
    {   lastErrorDescription.clear(); 
        lastErrorDescription += "FileStream::write() failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }
}

inline Utf8String FileStream::getLastErrorDescription() const
{
    return lastErrorDescription; 
}

}//namespace jjm

#endif
