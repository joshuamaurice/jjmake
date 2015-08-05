// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jiconv.hpp"

#include "jbase/juniqueptr.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jinttostring.hpp"
#include "jbase/jfatal.hpp"

#include <stdexcept>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#endif

using namespace jjm;
using namespace std;



namespace
{
    size_t const bufferSize = 4 * 1024; 

    inline iconv_t createIconvHandle(
                string const& toEncoding, 
                string const& fromEncoding
                )
    {
#ifdef _WIN32
        SetLastError(0); 
#endif
        errno = 0; 
        iconv_t converter = iconv_open(toEncoding.c_str(), fromEncoding.c_str()); 
        if (converter != (iconv_t)-1)
            return converter;

#ifdef _WIN32
        DWORD const lastError = GetLastError(); 
#endif
        int const lastErrno = errno; 
        string message; 
        message += "iconv_open(\"" + toEncoding + "\", \"" + fromEncoding + "\") failed. Cause:\n";
#ifdef _WIN32
        message += "GetLastError() " + toDecStr(lastError) + ". errno " + toDecStr(lastErrno) + "."; 
#else
        message += "errno " + toDecStr(lastErrno) + "."; 
#endif
        throw std::runtime_error(message); 
    }

    inline void invokeIconv(
                iconv_t converter, 
                char * const inputBufferBegin, 
                size_t & inputSize, 
                char * const outputBufferBegin, 
                size_t & outputSize, 
                size_t const outputCapacity
                )
    {
#ifdef _WIN32
        char const * inputPtr = inputBufferBegin; 
#else
        char * inputPtr = inputBufferBegin; 
#endif
        char * outputPtr = outputBufferBegin + outputSize; 
        size_t outputRemaining = outputCapacity - outputSize; 

#ifdef _WIN32
        SetLastError(0); 
#endif
        errno = 0; 
        size_t const iconvReturn = iconv(converter, & inputPtr, & inputSize, & outputPtr, & outputRemaining); 
#ifdef _WIN32
        DWORD const lastError = GetLastError(); 
#endif
        int const lastErrno = errno; 

        memmove(inputBufferBegin, inputPtr, inputSize); 
        outputSize = outputCapacity - outputRemaining; 

        if (iconvReturn != (size_t)-1)
        {   //success! 
            return; 
        }

        if (lastErrno == E2BIG)
        {   //out of space in the output buffer, but otherwise successful
            return; 
        }
        if (lastErrno == EINVAL)
        {   //input ends in an incomplete sequence, keep it there for next time, but otherwise successful
            return; 
        }
        string message; 
        message += "iconv(<iconv_handle>, <inbuf>, <n>, <outbuf>, <n>) failed. Cause:\n";
        if (lastErrno == EILSEQ)
        {   //Perhaps contrary to documentation, this is how my version of iconv seems to behave. 
            message += "errno EILSEQ, the input contains a sequence of bytes which are invalid in the input encoding, "
                    "or a valid character in the input cannot be mapped to a valid character in the output encoding."; 
            throw std::runtime_error(message); 
        }
#ifdef _WIN32
        message += "GetLastError() " + toDecStr(lastError) + ". errno " + toDecStr(lastErrno) + "."; 
#else
        message += "errno " + toDecStr(lastErrno) + "."; 
#endif
        throw std::runtime_error(message); 
    }

    inline void invokeIconvStateFlushReset(
                iconv_t converter, 
                char * const outputBufferBegin, 
                size_t & outputSize, 
                size_t const outputCapacty,
                bool & needsFlush
                )
    {
        char * outputPtr = outputBufferBegin + outputSize; 
        size_t outputRemaining = outputCapacty - outputSize; 
        
#ifdef _WIN32
        SetLastError(0); 
#endif
        errno = 0; 
        size_t const iconvReturn = iconv(converter, 0, 0, & outputPtr, & outputRemaining); 
#ifdef _WIN32
        DWORD const lastError = GetLastError(); 
#endif
        int const lastErrno = errno; 

        outputSize = outputCapacty - outputRemaining; 

        if (iconvReturn != (size_t)-1)
        {   //success! 
            needsFlush = false; 
            return; 
        }

        if (lastErrno == E2BIG)
        {   //out of space in the output buffer, return success, but keep needsFlush == true
            return; 
        }
        string message; 
        message += "iconv(<iconv_handle>, 0, 0, <outbuf>, <n>) failed. Cause:\n";
#ifdef _WIN32
        message += "GetLastError() " + toDecStr(lastError) + ". errno " + toDecStr(lastErrno) + "."; 
#else
        message += "errno " + toDecStr(lastErrno) + "."; 
#endif
        throw std::runtime_error(message); 
    }
}

void jjm::DeallocateIconv::operator() (iconv_t converter) const
{
    if (converter != (iconv_t)-1)
    {   
#ifdef _WIN32
        SetLastError(0); 
#endif
        errno = 0;
        if (0 != iconv_close(converter))
            JFATAL(0, 0); 
    }
}




string jjm::convertEncoding(string const& toEncoding, string const& fromEncoding, string const& text)
{
    UniquePtr<iconv_t, DeallocateIconv> converter(createIconvHandle(toEncoding, fromEncoding)); 

    string output; 
    size_t outputSize = 0; 

    char const * const inputBegin = &text[0];
    size_t inputRemaining = text.size(); 

    bool needsFlush = true; 
    for (;;)
    {   if (inputRemaining == 0 && needsFlush)
            invokeIconvStateFlushReset(converter.get(), &output[0], outputSize, output.size(), needsFlush);
        if (inputRemaining == 0 && ! needsFlush)
        {   output.resize(outputSize);
            return output; 
        }
        if (outputSize + bufferSize >= output.size())
            output.resize(output.size() + (output.size() >> 1) + bufferSize); 
        invokeIconv(
                converter.get(), 
                const_cast<char*>(inputBegin), inputRemaining,
                &output[0], outputSize, output.size()
                ); 
    }
}




jjm::EncodingConverterInputStream::EncodingConverterInputStream()
    : stream(0), 
    converter((iconv_t)-1), 
    inputSize(0), inputCapacity(0), 
    foundEof(false), needsFlush(false), 
    outputSize(0), outputCapacity(0)
{
}

jjm::EncodingConverterInputStream::EncodingConverterInputStream(
        InputStream * stream_, std::string const& encoding, std::string const& underlyingStreamEncoding)
    : stream(0), 
    converter((iconv_t)-1), 
    inputSize(0), inputCapacity(0), 
    foundEof(false), needsFlush(false), 
    outputSize(0), outputCapacity(0)
{
    try
    {   reset(stream_, encoding, underlyingStreamEncoding); 
    }catch (std::exception & e)
    {   string message;
        message += "jjm::EncodingConverterInputStream::EncodingConverterInputStream(<stream>, <encoding>, <encoding>) failed. Cause:\n"; 
        message += e.what(); 
        throw e; 
    }
}

void jjm::EncodingConverterInputStream::reset(
        InputStream * stream_, std::string const& encoding, std::string const& underlyingStreamEncoding)
{
    try
    {   
        stream = 0; 
        converter.reset(); 

        if (input.get() == 0)
        {   input.reset(new char[bufferSize]); 
            inputSize = 0; 
            inputCapacity = bufferSize; 
        }
        if (output.get() == 0)
        {   output.reset(new char[bufferSize]); 
            outputSize = 0; 
            outputCapacity = bufferSize; 
        }

        stream = stream_;  

        converter.reset(createIconvHandle(encoding, underlyingStreamEncoding)); 

    }catch (std::exception & e)
    {   string message;
        message += "jjm::EncodingConverterInputStream::reset(<encoding>, <encoding>) failed. Cause:\n";
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}

jjm::EncodingConverterInputStream::~EncodingConverterInputStream()
{
}

void jjm::EncodingConverterInputStream::close()
{
    if (stream)
    {   stream->close(); 
        stream = 0; 
    }
    converter.reset(); 
}

std::int64_t jjm::EncodingConverterInputStream::seek(std::int64_t off, int whence)
{
    lastErrorDescription = "jjm::EncodingConverterInputStream does not support seek."; 
    return -1;
}

ssize_t jjm::EncodingConverterInputStream::read(void * argumentBuffer, std::size_t argumentBufferBytes)
{
    try
    {
        if (inputSize < inputCapacity)
        {   
            ssize_t x = stream->read(input.get(), inputCapacity - inputSize); 
            if (x >= 0)
            {   inputSize += x; 
            }else if (x == -1)
            {   foundEof = true; 
                needsFlush = true; 
            }else
            {   lastErrorDescription.clear(); 
                lastErrorDescription += "jjm::EncodingConverterInputStream::read() failed. Cause:\n";
                lastErrorDescription += stream->getLastErrorDescription(); 
                return -2; 
            }
        }

        if (inputSize > 0 && outputSize < outputCapacity)
            invokeIconv(converter.get(), input.get(), inputSize, output.get(), outputSize, outputCapacity); 

        if (needsFlush)
            invokeIconvStateFlushReset(converter.get(), output.get(), outputSize, outputCapacity, needsFlush); 
    
        if (outputSize > 0)
        {   ssize_t toCopy = std::min<ssize_t>(argumentBufferBytes, outputSize); 
            memcpy(argumentBuffer, output.get(), toCopy); 
            memmove(output.get(), output.get() + toCopy, outputSize - toCopy); 
            outputSize -= toCopy; 
            return toCopy; 
        }

        if (foundEof && ! needsFlush)
            return -1; 

        return 0; 

    }catch(std::exception & e)
    {   lastErrorDescription.clear();
        lastErrorDescription += "jjm::EncodingConverterInputStream::read() failed. Cause:\n";
        lastErrorDescription += e.what();
        return -2; 
    }
}

jjm::Utf8String  jjm::EncodingConverterInputStream::getLastErrorDescription() const
{
    return lastErrorDescription; 
}




jjm::EncodingConverterOutputStream::EncodingConverterOutputStream()
    : stream(0), 
    converter((iconv_t)-1), 
    inputSize(0), inputCapacity(0), 
    foundEof(false), 
    outputSize(0), outputCapacity(0)
{
}

jjm::EncodingConverterOutputStream::EncodingConverterOutputStream(
        jjm::OutputStream * stream_, std::string const& encoding, std::string const& underlyingStreamEncoding)
    : stream(0), 
    converter((iconv_t)-1), 
    inputSize(0), inputCapacity(0), 
    foundEof(false), 
    outputSize(0), outputCapacity(0)
{
    try
    {   reset(stream_, encoding, underlyingStreamEncoding); 
    }catch (std::exception & e)
    {   string message;
        message += "jjm::EncodingConverterOutputStream::EncodingConverterOutputStream(<stream>, <encoding>, <encoding>) failed. Cause:\n"; 
        message += e.what(); 
        throw e; 
    }
}

void jjm::EncodingConverterOutputStream::reset(
        jjm::OutputStream * stream_, std::string const& encoding, std::string const& underlyingStreamEncoding)
{
    try
    {   
        stream = 0; 
        converter.reset(); 

        if (input.get() == 0)
        {   input.reset(new char[bufferSize]); 
            inputSize = 0; 
            inputCapacity = bufferSize; 
        }
        if (output.get() == 0)
        {   output.reset(new char[bufferSize]); 
            outputSize = 0; 
            outputCapacity = bufferSize; 
        }
    
        stream = stream_;  

        converter.reset(createIconvHandle(encoding, underlyingStreamEncoding)); 

    }catch (std::exception & e)
    {   string message;
        message += "jjm::EncodingConverterOutputStream::reset(<encoding>, <encoding>) failed. Cause:\n";
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}

jjm::EncodingConverterOutputStream::~EncodingConverterOutputStream()
{
}

void jjm::EncodingConverterOutputStream::close()
{
    if (stream)
    {   stream->close(); 
        stream = 0; 
    }
    converter.reset(); 
}

std::int64_t jjm::EncodingConverterOutputStream::seek(std::int64_t off, int whence)
{
    lastErrorDescription = "jjm::EncodingConverterOutputStream does not support seek."; 
    return -1;
}

ssize_t jjm::EncodingConverterOutputStream::write(void const* argumentBuffer, std::size_t argumentBufferBytes)
{
    if (outputSize > 0)
    {   ssize_t x = stream->write(output.get(), outputSize); 
        if (x < 0)
        {   lastErrorDescription.clear();
            lastErrorDescription += "jjm::EncodingConverterOutputStream::write() failed. Cause:\n"; 
            lastErrorDescription += stream->getLastErrorDescription(); 
            return -1;
        }
        memmove(output.get(), output.get() + x, outputSize - x); 
        outputSize -= x; 
    }

    if (inputSize > 0 && outputSize < outputCapacity)
    {   try
        {   invokeIconv(converter.get(), input.get(), inputSize, output.get(), outputSize, outputCapacity); 
        }catch (std::exception & e)
        {   lastErrorDescription.clear(); 
            lastErrorDescription += "jjm::EncodingConverterOutputStream::write() failed. Cause:\n";
            lastErrorDescription += e.what(); 
            return -1; 
        }
    }

    if (inputSize < inputCapacity)
    {   ssize_t toCopy = std::min<ssize_t>(argumentBufferBytes, inputCapacity - inputSize); 
        memcpy(input.get(), argumentBuffer, toCopy); 
        inputSize += toCopy; 
        return toCopy; 
    }

    return 0; 
}

int jjm::EncodingConverterOutputStream::flush()
{
    for (;;)
    {   if (inputSize == 0 && outputSize == 0)
            break;
        if (inputSize > 0 && outputSize < outputCapacity)
        {   try
            {   invokeIconv(converter.get(), input.get(), inputSize, output.get(), outputSize, outputCapacity); 
            }catch (std::exception & e)
            {   lastErrorDescription.clear(); 
                lastErrorDescription += "jjm::EncodingConverterOutputStream::flush() failed. Cause:\n";
                lastErrorDescription += e.what(); 
                return -1; 
            }
        }
        if (outputSize > 0)
        {   ssize_t x = stream->write(output.get(), outputSize); 
            if (x < 0)
            {   lastErrorDescription.clear(); 
                lastErrorDescription += "jjm::EncodingConverterOutputStream::flush() failed. Cause:\n";
                lastErrorDescription += stream->getLastErrorDescription(); 
                return -1;
            }
            memmove(output.get(), output.get() + x, outputSize - x); 
            outputSize -= x; 
        }
    }

    try
    {   bool needsFlush = true; 
        invokeIconvStateFlushReset(converter.get(), output.get(), outputSize, outputCapacity, needsFlush); 
        if (needsFlush)
            JFATAL(0, 0); 
        for (;;)
        {   if (outputSize == 0)
                break;
            ssize_t x = stream->write(output.get(), outputSize); 
            if (x < 0)
            {   lastErrorDescription.clear(); 
                lastErrorDescription += "jjm::EncodingConverterOutputStream::flush() failed. Cause:\n";
                lastErrorDescription += stream->getLastErrorDescription(); 
                return -1;
            }
            memmove(output.get(), output.get() + x, outputSize - x); 
            outputSize -= x; 
        }
    }catch (std::exception & e)
    {   lastErrorDescription.clear(); 
        lastErrorDescription += "jjm::EncodingConverterOutputStream::flush() failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }

    return 0; //success; 
}

jjm::Utf8String jjm::EncodingConverterOutputStream::getLastErrorDescription() const
{
    return lastErrorDescription; 
}
