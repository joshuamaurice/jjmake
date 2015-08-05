// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JICONV_HPP_HEADER_GUARD
#define JUNICODE_JICONV_HPP_HEADER_GUARD

#include "jbase/juniqueptr.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jstreams.hpp"

#include <string>
#include <cstddef>

#ifdef _WIN32
    #define USING_STATIC_LIBICONV 1
    #include "libiconv/include/iconv.h"
#else
    #include <iconv.h>
#endif

namespace jjm
{

//DeallocateIconv is private detail:
class DeallocateIconv { public: void operator() (iconv_t converter) const; };


//Uses iconv
std::string convertEncoding(
        std::string const& toEncoding, 
        std::string const& fromEncoding, 
        std::string const& text
        ); 


//Uses iconv
class EncodingConverterInputStream : public jjm::InputStream
{
public:
    EncodingConverterInputStream(); 

    //does not take ownership of the stream
    EncodingConverterInputStream(jjm::InputStream * st, std::string const& encoding, std::string const& underlyingStreamEncoding); 

    //does not take ownership of the stream
    void reset(jjm::InputStream * st, std::string const& encoding, std::string const& underlyingStreamEncoding); 
    jjm::InputStream * getStream() const { return stream; }

    virtual ~EncodingConverterInputStream(); 
    virtual void close(); 
    virtual std::int64_t seek(std::int64_t off, int whence); 
    virtual ssize_t read(void * buf, std::size_t bytes); 
    virtual jjm::Utf8String getLastErrorDescription() const; 

private:
    jjm::Utf8String lastErrorDescription; 

    jjm::InputStream * stream; 

    jjm::UniquePtr<iconv_t, DeallocateIconv> converter; 

    jjm::UniqueArray<char*> input; 
    std::size_t inputSize; 
    std::size_t inputCapacity; 
    
    bool foundEof; 
    bool needsFlush; 

    jjm::UniqueArray<char*> output; 
    std::size_t outputSize; 
    std::size_t outputCapacity; 
};


//Uses iconv
class EncodingConverterOutputStream : public jjm::OutputStream
{
public:
    EncodingConverterOutputStream(); 

    //does not take ownership of the stream
    EncodingConverterOutputStream(jjm::OutputStream * st, std::string const& toEncoding, std::string const& fromEncoding); 

    //does not take ownership of the stream
    void reset(jjm::OutputStream * st, std::string const& encoding, std::string const& underlyingStreamEncoding); 
    jjm::OutputStream * getStream() const { return stream; }

    virtual ~EncodingConverterOutputStream(); 
    virtual void close(); 
    virtual std::int64_t seek(std::int64_t off, int whence); 
    virtual ssize_t write(void const* buf, std::size_t bytes);  
    virtual int flush(); 
    virtual jjm::Utf8String getLastErrorDescription() const; 

private:
    jjm::Utf8String lastErrorDescription; 

    jjm::OutputStream * stream; 

    jjm::UniquePtr<iconv_t, DeallocateIconv> converter; 

    jjm::UniqueArray<char*> input; 
    std::size_t inputSize; 
    std::size_t inputCapacity; 

    bool foundEof; 

    jjm::UniqueArray<char*> output; 
    std::size_t outputSize; 
    std::size_t outputCapacity; 
};


}//namespace jjm

#endif
