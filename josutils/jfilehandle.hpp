// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JFILEHANDLE_HPP_HEADER_GUARD
#define JOSUTILS_JFILEHANDLE_HPP_HEADER_GUARD

#include "jbase/jstdint.hpp"
#include "jbase/juniqueptr.hpp"
#include <stdio.h> //for SEEK_CUR, etc.,

#ifdef _WIN32
    #include <windows.h>
#endif


namespace jjm
{

/*
This class is a thin wrapper on top of HANDLE for windows and int for POSIX. 
This class has shallow-copy semantics. 
*/

class FileHandle;
bool operator== (FileHandle x, FileHandle y); 
bool operator!= (FileHandle x, FileHandle y); 

class FileHandle
{
public: 
    #ifdef _WIN32
        typedef  void*  NativeHandle; 
    #else
        typedef  int  NativeHandle; 
    #endif

private:
    #ifdef _WIN32
        static inline NativeHandle invalidHandle() { return (NativeHandle)static_cast<ssize_t>(-1); } //has a static assert in cpp
    #else
        static inline NativeHandle invalidHandle() { return -1; }
    #endif

public:
    //Creates an invalid file handle
    FileHandle() : mhandle(invalidHandle()) {}

    //
    explicit FileHandle(NativeHandle handle_) : mhandle(handle_) {}

    //No-op. It is up to the user to explicitly call "close()" when needed. 
    //The user is free to use a UniquePtr or other RAII tools for that purpose. 
    ~FileHandle() {} 

    FileHandle(FileHandle const& x) : mhandle(x.mhandle) {}
    FileHandle& operator= (FileHandle const& x) { mhandle= x.mhandle; return *this; }

    void native(NativeHandle h) { mhandle = h; }
    NativeHandle native() const { return mhandle; }

    friend inline bool operator== (FileHandle x, FileHandle y) { return x.mhandle == y.mhandle; }
    friend inline bool operator!= (FileHandle x, FileHandle y) { return x.mhandle != y.mhandle; }

    
    /* Closes the handle, and sets this object to the invalid file handle. 
    close() is a no-op on the invalid file handle. 
    close() invokes JFATAL on errors. */
    void close(); 


    /* Closes the handle. 
    On success, it sets this object to the invalid file handle. 
    close2() is a no-op on the invalid file handle. 
    close2() returns 0 on success and -1 on errors. 
    To determine the cause of the error, you can use GetLastError() on Windows
    and errno on POSIX. */
    int close2(); 


    /* "whence" is defined according to POSIX. It is one of: 
    SEEK_SET: off is an offset from the start of the file
    SEEK_CUR: off is an offset from the current position of the file. 
    SEEK_END: off is an offset from the the end of the file, using the current size of the file. 
    
    On success, returns the current position of the file. 
    
    On failure, returns -1. 
    
    Implementations may not support this operation. Those implementations 
    return -1. 
    
    The cause of an error can be detected with GetLastError() for Windows
    and errno for POSIX. */
    int64_t seek2(int64_t off, int whence); 

    /* seek() is like seek2(), except on errors seek() throws a std::exception 
    which describes the error. */
    int64_t seek(int64_t off, int whence); 

    
    /* Reads some number of bytes, between 0 and the arg "bytes", inclusive. 

    This has a different API than the usual POSIX ::read() call. Normally with
    POSIX ::read(), EOF is detected with a return value of 0. However, it is
    desirable to create an API which would allow a subclass to return 0 when it
    gets EINTR which is incompatible with using 0 to report EOF. This interface
    has this particular API for this reason. 

    On success, return the number of bytes successfully read. 

    If an implementation cannot read the requested number of bytes because of 
    EOF, but it can read a number of bytes greater than 0, then it reads some
    allowed number of bytes, and returns the number of bytes successfully read. 

    If an implementation cannot read the requested number of bytes because of
    EOF, and if there are no more bytes left to be read, then it returns -1. 

    On other errors, return -2. 
    
    Subclasses should be written so that:
        1- the call fails and extracts zero bytes from the underlying source, or 
        2- the call succeeds and reads a certain number of bytes. 
    
    When subclasses get EINTR, they should return 0 rather than retry. This 
    allows the possibility of adding an interruption API on 
    BufferedInputStream and BufferedOutputStream. 
    
    The cause of an error can be detected with GetLastError() for Windows
    and errno for POSIX. */
    ssize_t read2(void * buf, std::size_t bytes);  

    /* read() is like read2(), except on errors read() throws a std::exception
    which descriibes the error. read() still returns -1 on EOF. */
    ssize_t read(void * buf, std::size_t bytes);  


    /* Writes some number of bytes, between 0 and the arg "bytes", inclusive. 
    
    On success, returns number of bytes successfully written. 
    
    On errors, returns -1. 
    
    Subclasses should be written so that:
        1- the call fails and writes zero bytes to the underlying target, or 
        2- the call succeeds and writes a certain number of bytes. 
        
    When subclasses get EINTR, they should return 0 rather than retry. This 
    allows the possibility of adding an interruption API on BufferedOutputStream. 
    
    The cause of an error can be detected with GetLastError() for Windows
    and errno for POSIX. */
    ssize_t write2(void const* buf, std::size_t bytes);  

    /* write() is like write2(), except on errors write() throws a std::exception
    which descriibes the error. */
    ssize_t write(void const* buf, std::size_t bytes);  
    
    /* writeComplete2() behaves like write2(), except it will use a loop to 
    output the entire string. 
    Upon an error, it will immediately break the loop and return the number of 
    bytes succesfully written. */ 
    ssize_t writeComplete2(void const* buf, std::size_t bytes); 

    /* writeComplete() behaves like write(), except it will use a loop to 
    output the entire string. 
    Upon an error, it will immediately break the loop and throw a 
    std::exception which describes the error.*/ 
    ssize_t writeComplete(void const* buf, std::size_t bytes); 



#ifndef _WIN32
    //returns 0 on success, -1 on errors
    int setCloseOnExec(bool b = true); 
#endif


    //utility functions to get the standard handles
#ifdef _WIN32
    static FileHandle getstdin () { return FileHandle(GetStdHandle(STD_INPUT_HANDLE )); }
    static FileHandle getstdout() { return FileHandle(GetStdHandle(STD_OUTPUT_HANDLE)); }
    static FileHandle getstderr() { return FileHandle(GetStdHandle(STD_ERROR_HANDLE )); }
#else
    static FileHandle getstdin () { return FileHandle(0); }
    static FileHandle getstdout() { return FileHandle(1); }
    static FileHandle getstderr() { return FileHandle(2); }
#endif


private:
    NativeHandle mhandle; 
};


class FileHandleCloser { public: void operator() (FileHandle h) const { h.close(); } };
typedef  jjm::UniquePtr<FileHandle, FileHandleCloser>  FileHandleOwner; 


} //namespace jjm

#endif
