// Copyright (c) 2010-2011, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JFILESTREAMS_HPP_HEADER_GUARD
#define JOSUTILS_JFILESTREAMS_HPP_HEADER_GUARD

#include "jfilehandle.hpp"
#include "jbase/jstreams.hpp"

namespace jjm
{

class FileStream : public jjm::InputStream, public jjm::OutputStream
{
public:
    FileStream() : mhandle() {}
    explicit FileStream(FileHandle handle_) : mhandle(handle_) {} //takes ownership
    ~FileStream() { mhandle.close(); } 

    void handle(FileHandle handle_) { close(); mhandle = handle_; } //takes ownership
    FileHandle handle() const { return mhandle; }
    FileHandle release() { FileHandle x = mhandle; mhandle = FileHandle(); return x; }

    virtual void close() { mhandle.close(); }
    virtual int64_t seek(int64_t off, int whence) { return mhandle.seek(off, whence); } 
    virtual ssize_t read(void * buf, std::size_t bytes) { return mhandle.read(buf, bytes); }
    virtual ssize_t write(void const* buf, std::size_t bytes) { return mhandle.write(buf, bytes); }
    virtual int flush() { return 0; }

private:
    FileStream(FileStream const& ); //not defined, not copyable
    FileStream& operator= (FileStream const& ); //not defined, not copyable

    FileHandle mhandle; 
};

}//namespace jjm

#endif
