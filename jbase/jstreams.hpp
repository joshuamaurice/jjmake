// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JBASE_JSTREAMS_HPP_HEADER_GUARD
#define JBASE_JSTREAMS_HPP_HEADER_GUARD

#include "jstdint.hpp"
#include <stddef.h>
#include <stdio.h> //for SEEK_CUR, etc.,
#include <string.h>
#include <string>


namespace jjm 
{

/*
** Introduction

These classes are byte-focused. If you care about string encodings, then you
need to do additional work yourself. 

Most users should not operate with InputStreams and OutputStreams directly. 
Rather, most code should be written on top of BufferedInputStream and 
BufferedOutputStream (or possibly on top of other buffered wrappers). 

If you need to add a new kind of data source or data target, then that is the 
time when you would create a new subclass of InputStream or OutputStream. 


** Example Usage
int fd = //from somewhere
FileInputStream fileInputStream(fd);
BufferedInputStream bufferedInputStream( & fileInputStream);
bufferedInputStream.read ...
...


** Error Handling

The read() and write() of buffered-streams fail only when the underlying
InputStream or OutputStream returns failure. When the read() or write() of a 
buffered-stream fails, you can check the underlying InputStream or OutputStream
for further information (if any). 

It's guaranteed that once buffered-stream::isGood is false, any call to 
read(), write(), flush(), etc. will not process any bytes. This allows 
chaining together many read() and write() calls, and checking for errors only 
at the end. 


** Warnings to other programmers: 

BufferedInputStream and BufferedOutputStream do not inherit from 
InputStream and OutputStream for a reason! Read the comments and understand 
the design of the classes before you start doing things like adding virtual
functions, base classes, or derived classes to BufferedInputStream and 
BufferedOutputStream. 

These classes are byte-focused. These are analogous to Java's InputStream
and OutputStream. If you need to worry about encoding, think about how you
might do it in Java, particularly the OutputStreamWriter. 


** Design Notes

BufferedInputStream and BufferedOutputStream have no superclasses, no derived 
classes, and no virtual functions. The read() and write() member
functions of the buffered streams have very small bodies by containing only 
the fast-path, and they call out to read2() and write2() member functions for 
the uncommon path. The read() and write() member functions of the buffered 
streams also have inline definitions in the header. That should make the 
compiler expand-inline read() and write(), avoiding all function call 
overhead. Altogether, that should make BufferedInputStream and 
BufferedOutputStream about as fast as a hand-coded solution. 

In many important ways, the design is like C++ std iostreams. In other important 
ways, there are substantial differences. 
### Important similarities:
* BufferedInputStream is like std::istream. 
* BufferedOutputStream is like std::ostream.
* InputStream and OutputStream are like std::streambuf.
### Important differences: 
* PmFastStreams have actual readable and understandable documentation, 
especially concerning adding new kinds of data sources and data targets. 
* PmFastStreams don't deal with newline conversions ("\r\n" vs "\n"). It's 
an encoding issue IMHO. I decided specifically that PmFastStreams will not deal 
with encoding issues. You can handle it yourself with a wrapper on top of 
these classes. 
* PmFastStream don't have all of that locale and facet crap which kills 
performance and which no one uses anyway. When people do internationalization
and localization, the pithy and non-portable C++ standard library support isn't 
good enough. Real people use ICU, often with lots of customization. 
(Yes yes I know that some C++ implementations go through a lot of hoops to make
locales and facets not a huge performance killer, but do you know if all of 
them do?) 
*/

class InputStream;
class OutputStream;
class BufferedInputStream;
class BufferedOutputStream;


class BufferedInputStream
{
private:
    void safe_bool_type_func() {}
    typedef void (BufferedInputStream::*safe_bool_type)(); 

public:

    /* Does not take ownership over the inputStream. 
    Does not allocate an internal buffer until it is given a valid InputStream. 
    bufferSize==0 is a special flag to use an internal default for the buffer size. */
    BufferedInputStream(InputStream * inputStream = 0, std::size_t bufferSize = 0); 


    ~BufferedInputStream();


    /* Does not take ownership over the inputStream. 
    Resets isGood to true. 
    Resets isEof to false. 
    Resets gcount to 0. 
    Clears any internally buffered data. 
    bufferSize==0 is a special flag to use an internal default for the buffer size. */
    void resetInputStream(InputStream * inputStream = 0, std::size_t bufferSize = 0); 

    
    //Resets isGood to true. 
    //Resets isEof to false. 
    //Resets gcount to 0. 
    void resetState(); 

    
    InputStream * inputStream(); //getter
    InputStream const * inputStream() const; //getter

    
    bool isGood() const; //initially true
    bool isEof() const; //initially false
    std::size_t gcount() const; //initially 0

    
    /* Like the C++ std iostreams, this class has an implicit conversion 
    operator to test for isGood(), ex: such as for use in "if" conditions. 
    Google the "convert to safe bool" idiom, or simply "safe bool" idiom. 
    Ex:
    stream.read(buf, size);
    if (stream)
        onSuccess(); 
    if (stream.read(buf, size))
        onSuccess(); 
    if ( ! stream.read(buf, size))
        onFailure(); */
    operator safe_bool_type () const;

    
    /* This function will copy the requested number of bytes from the internal
    buffer and/or underlying InputStream to the argument buffer. 

    If isGood is false at the beginning of the call, then gcount is set to 0,
    and the call has no other effect. Otherwise, it reads some number of 
    bytes into the argument buffer and updates state as described below. 

    It sets gcount equal to the number of bytes copied into the argument 
    buffer from the internal buffer and/or the underlying InputStream. 

    If it can obtain the requested number of bytes from the internal buffer 
    and/or underlying InputStream, then it copies the requested number of bytes
    into the argument buffer. 

    If end-of-file prevents it from obtaining the requested number of bytes, 
    then it sets isGood to false. It sets isEof to true. It reads all of the 
    bytes before end-of-file, and copies all of the remaining bytes from the
    internal buffer and underlying InputStream into the argument buffer. 

    On other errors: It sets isGood to false. It copies some number of bytes 
    from the internal buffer and/or underlying InputStream, between 0 and arg 
    "bytes", inclusive. These bytes will not be repeated on subsequent calls. 
    (The user can determine how many bytes were read into the argument buffer 
    by calling gcount().)

    Errors are caused exclusively by the underlying InputStream. For details on
    cause of the error, check with the underlying InputStream. 

    This call returns *this. */
    BufferedInputStream & read(void * buf, std::size_t bytes);


    /*
    This function behaves like read(void* buf, size_t byes), except it appends
    bytes to str. If str already has data new data is appended. 
    */
    BufferedInputStream & read(std::string & str, std::size_t bytes);

    
    /* Utility. 
    Writes the binary contents of the plain-old-data type object. 
    This effectively does a std::memcpy on the argument. */
    template <typename PodT> 
    BufferedInputStream & readPod(PodT & pod)
    {
        return read( & pod, sizeof pod);
    }

    
    /* 
    If isGood is false at the beginning of the call, then this call has no 
    effect, and -1 is returned. Otherwise, the stream position is changed 
    according to the description below. 

    "whence" is defined according to POSIX. It is one of: 
    SEEK_SET: off is an offset from the start of the file
    SEEK_CUR: off is an offset from the current position of the file. 
    SEEK_END: off is an offset from the the end of the file, using the current size of the file. 

    On success, returns the current position of the stream. 

    On failure, returns -1. 

    The underlying InputStream may not support this operation. In that case,
    this function will simply return -1.
    
    Be warned: Because of the internal buffering of BufferedInputStream,
    results of this function can be different than calling seek on the 
    underlying InputStream. The implementation of BufferedInputStream::seek
    properly takes the internal buffering into account, and it will give the 
    proper results. */
    int64_t seek(int64_t off, int whence); 


private:
    InputStream * m_inputStream; //does not own
    bool m_isGood; 
    bool m_isEof; 
    std::size_t m_gcount; 

    //the internal buffer
    unsigned char * m_allocBegin; //owns
    unsigned char * m_allocEnd;
    unsigned char * m_dataBegin; 
    unsigned char * m_dataEnd;

    void read2(void * buf, std::size_t bytes);
    void read2(std::string & str, std::size_t bytes);

    BufferedInputStream(BufferedInputStream const& ); //not defined, not copyable
    BufferedInputStream& operator= (BufferedInputStream const& ); //not defined, not copyable
};


class BufferedOutputStream
{
private:
    void safe_bool_type_func() {}
    typedef void (BufferedOutputStream::*safe_bool_type)(); 

public:

    /* Does not take ownership over the outputStream. 
    Does not allocate an internal buffer until it is given a valid OutputStream. 
    bufferSize==0 is a special flag to use an internal default for the buffer size. */
    BufferedOutputStream(OutputStream * outputStream = 0, std::size_t bufferSize = 0); 

    
    /* The destructor will call flush(), and it will ignore any error that 
    happens. It is good practice to call flush explicitly yourself before 
    destroying this object in order to detect and handle possible errors. */
    ~BufferedOutputStream();

    
    /* Does not take ownership over the outputStream. 
    Resets isGood to true. 
    Resets gcount to 0. 
    Clears any internally buffered data without flushing. 
    bufferSize==0 is a special flag to use an internal default for the buffer size. */
    void resetOutputStream(OutputStream * outputStream = 0, std::size_t bufferSize = 0); 

    
    //Resets isGood to true. 
    //Resets gcount to 0. 
    void resetState(); 

    
    OutputStream * outputStream(); //getter
    OutputStream const * outputStream() const; //getter

    
    bool isGood() const; //initially true
    std::size_t gcount() const; //initially 0

    
    /* Like the C++ std iostreams, this class has an implicit conversion 
    operator to test for isGood(), ex: such as for use in "if" conditions. 
    Google the "convert to safe bool" idiom, or simply "safe bool" idiom. 
    Ex:
    stream.write(buf, size);
    if (stream)
        onSuccess(); 
    if (stream.write(buf, size))
        onSuccess(); 
    if ( ! stream.write(buf, size))
        onFailure(); */
    operator safe_bool_type () const;

    
    /* This function will write the requested number of bytes from the argument
    buffer into the internal buffer and/or the underlying OutputStream. 

    If isGood is false at the beginning of the call, then gcount is set to 0, 
    and this call has no other effect. Otherwise, it writes some number of 
    bytes from the argument buffer into the internal buffer and/or the 
    underlying OutputStream, and updates isGood and gcount as described below. 

    It sets gcount equal to the number of bytes copied from the argument 
    buffer into the internal buffer and/or the underlying OutputStream. 

    On success: It copies the requested number of bytes from the argument 
    buffer into this object's internal buffer and/or writes those bytes to the
    underlying OutputStream. 

    On errors: It sets isGood to false. It copies some number of bytes to the 
    internal buffer and/or the underlying OutputStream, between 0 and arg 
    "bytes", inclusive. (The user can determine how many bytes were copied from 
    the argument buffer by calling gcount().) 
    
    Errors are caused exclusively by the underlying OutputStream. For details on
    cause of the error, check with the underlying OutputStream. 

    This call returns *this. */
    BufferedOutputStream & write(void const * buf, std::size_t bytes);

    
    /* Utility. 
    Reads the binary contents of the plain-old-data type object. 
    This effectively does a std::memcpy from the argument. */
    template <typename PodT> 
    BufferedOutputStream & writePod(PodT const& pod)
    {
        return write( & pod, sizeof pod);
    }

    
    /* This will flush this object's internal buffer, and call flush on the
    underlying OutputStream. 

    gcount is unaffected. (gcount refers to the number of bytes copied from the
    buffer argument of write(), which has no applicability when moving bytes
    from the internal buffer to the underlying OutputStream.)

    If isGood() is false at the beginning of the call, then this call has no 
    effect. 

    On errors: It sets isGood to false. Some number of bytes may have been 
    moved from the internal buffer to the underlying OutputStream. */
    BufferedOutputStream & flush(); 

    
    /* This will flush this class's internal buffer. It will not call flush 
    on the underlying OutputStream. 

    gcount is unaffected. (gcount refers to the number of bytes copied from the
    buffer argument of write(), which has no applicability when moving bytes
    from the internal buffer to the underlying OutputStream.)

    If isGood() is false at the beginning of the call, then this call has no 
    effect. 

    On errors: It sets isGood to false. Some number of bytes may have been 
    moved from the internal buffer to the underlying OutputStream. */
    BufferedOutputStream & flushInternalBuffer(); 

    
    /* 
    If isGood is false at the beginning of the call, then this call has no 
    effect, and -1 is returned. Otherwise, the stream position is changed 
    according to the description below. 

    "whence" is defined according to POSIX. It is one of: 
    SEEK_SET: off is an offset from the start of the file
    SEEK_CUR: off is an offset from the current position of the file. 
    SEEK_END: off is an offset from the the end of the file, using the current size of the file. 

    On success, it flushes the internal buffer to the underlying OutputStream,
    and this call returns the current position of the stream. 

    On failure, it flushes some of the internal buffer (none, some, or all) 
    to the underlying OutputStream, and this call returns -1. 

    The underlying OutputStream may not support this operation. In that case,
    it flushes some of the internal buffer (none, some, or all) to the 
    underlying OutputStream, and this call returns -1.
    
    Be warned: Because of the internal buffering of BufferedOutputStream,
    results of this function can be different than calling seek on the 
    underlying OutputStream. The implementation of BufferedOutputStream::seek
    properly takes the internal buffering into account, and it will give the 
    proper results. */
    int64_t seek(int64_t off, int whence); 


private:
    OutputStream * m_outputStream; //does not own
    bool m_isGood; 
    std::size_t m_gcount; 

    //the internal buffer
    unsigned char * m_allocBegin; //owns
    unsigned char * m_allocEnd;
    unsigned char * m_dataBegin;
    unsigned char * m_dataEnd;

    void write2(void const * buf, std::size_t bytes);

    BufferedOutputStream(BufferedOutputStream const& ); //not defined, not copyable
    BufferedOutputStream& operator= (BufferedOutputStream const& ); //not defined, not copyable
};


class Stream
{
public:
    virtual ~Stream() {}

    virtual void close() = 0; 
    

    /* "whence" is defined according to POSIX. It is one of: 
    SEEK_SET: off is an offset from the start of the file
    SEEK_CUR: off is an offset from the current position of the file. 
    SEEK_END: off is an offset from the the end of the file, using the current size of the file. 
    
    On success, returns the current position of the file. 
    
    On failure, returns -1. 
    
    Implementations may not support this operation. Those implementations 
    should simply return -1. */
    virtual int64_t seek(int64_t off, int whence) = 0; 


protected:
    Stream() {}

private:
    Stream(Stream const& ); //not defined, not copyable
    Stream& operator= (Stream const& ); //not defined, not copyable
};


/* Subclasses of InputStream should be written to have good performance for 
bulk operations, and they should not add additional userspace buffering. Users 
who need userspace buffering should use BufferedInputStream or some other 
buffering wrapper. */
class InputStream : public virtual Stream
{
public:
    virtual ~InputStream() {}
    virtual void close() = 0; 
    virtual int64_t seek(int64_t off, int whence) = 0; 

    
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
    BufferedInputStream and BufferedOutputStream. */
    virtual ssize_t read(void * buf, std::size_t bytes) = 0;  


protected:
    InputStream() {}

private:
    InputStream(InputStream const& ); //not defined, not copyable
    InputStream& operator= (InputStream const& ); //not defined, not copyable
};


/* Subclasses of OutputStream should be written to have good performance for 
bulk operations, and they should not add additional userspace buffering. Users 
who need userspace buffering should use BufferedOutputStream or some other 
buffering wrapper. */
class OutputStream : public virtual Stream
{
public:
    //There is no requirement on subclasses to call flush in the destructor. 
    //It is the responsibility of the user to call flush. 
    virtual ~OutputStream() {} 

    
    //There is no requirement on subclasses to call flush in close. 
    //It is the responsibility of the user to call flush. 
    virtual void close() = 0; 

    
    virtual int64_t seek(int64_t off, int whence) = 0; 

    
    /* Writes some number of bytes, between 0 and the arg "bytes", inclusive. 
    
    On success, returns number of bytes successfully written. 
    
    On errors, returns -1. 
    
    Subclasses should be written so that:
        1- the call fails and writes zero bytes to the underlying target, or 
        2- the call succeeds and writes a certain number of bytes. 
        
    When subclasses get EINTR, they should return 0 rather than retry. This 
    allows the possibility of adding an interruption API on BufferedOutputStream. */
    virtual ssize_t write(void const* buf, std::size_t bytes) = 0;  

    
    /* Ensure that the data is moved to its final destination. Subclasses 
    should be implemented to avoid the use of additional userspace buffering, 
    but an explicit flush is still a useful API to have. 
    
    Returns 0 on success. 
    
    Return -1 on errors. */
    virtual int flush() = 0; 


protected:
    OutputStream() {}

private:
    OutputStream(OutputStream const& ); //not defined, not copyable
    OutputStream& operator= (OutputStream const& ); //not defined, not copyable
};


// **** **** **** **** 
// **** **** **** **** 
// No public APIs past this point. 
// The following is included in the header for compiler inlining only. 

inline OutputStream * BufferedOutputStream::outputStream()
{
    return m_outputStream;
}

inline OutputStream const * BufferedOutputStream::outputStream() const
{
    return m_outputStream;
}

inline InputStream * BufferedInputStream::inputStream()
{
    return m_inputStream;
}

inline InputStream const * BufferedInputStream::inputStream() const
{
    return m_inputStream;
}

inline bool BufferedInputStream::isGood() const 
{ 
    return m_isGood; 
}

inline bool BufferedOutputStream::isGood() const 
{ 
    return m_isGood; 
}

inline bool BufferedInputStream::isEof() const 
{ 
    return m_isEof; 
}

inline std::size_t BufferedInputStream::gcount() const 
{ 
    return m_gcount; 
}

inline std::size_t BufferedOutputStream::gcount() const 
{ 
    return m_gcount; 
}

inline BufferedInputStream::operator safe_bool_type() const 
{ 
    return m_isGood ? & BufferedInputStream::safe_bool_type_func : 0; 
}

inline BufferedOutputStream::operator safe_bool_type() const 
{ 
    return m_isGood ? & BufferedOutputStream::safe_bool_type_func : 0; 
}

inline BufferedInputStream & BufferedInputStream::read(void * buf, std::size_t bytes)
{
    m_gcount = 0; 
    if (!*this)
        return *this;

    //if the requested size can be satisfied with our internal buffer, do that. 
    if (bytes + m_dataBegin <= m_dataEnd)
    {   memcpy(buf, m_dataBegin, bytes);
        m_dataBegin += bytes; 
        m_gcount = bytes; 
        return *this;
    }
    read2(buf, bytes);
    return *this;
}

inline BufferedInputStream & BufferedInputStream::read(std::string & str, std::size_t bytes)
{
    m_gcount = 0; 
    if (!*this)
        return *this;

    //if the requested size can be satisfied with our internal buffer, do that. 
    if (bytes + m_dataBegin <= m_dataEnd)
    {   str.insert(str.end(), m_dataBegin, m_dataBegin + bytes);
        m_dataBegin += bytes; 
        m_gcount = bytes; 
        return *this;
    }
    read2(str, bytes);
    return *this;
}

inline BufferedOutputStream & BufferedOutputStream::write(void const * buf, std::size_t bytes)
{
    m_gcount = 0; 
    if (!*this)
        return *this;

    //if the requested size can fit in our internal buffer, then put it there. 
    if (bytes + m_dataEnd <= m_allocEnd)
    {   memcpy(m_dataEnd, buf, bytes);
        m_dataEnd += bytes; 
        m_gcount = bytes; 
        return *this;
    }
    write2(buf, bytes);
    return *this;
}

} //namespace jjm

#endif
