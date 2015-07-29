// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jiconv.hpp"

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

jjm::IconvConverter::IconvConverter() : converter((iconv_t)-1) {}

void jjm::IconvConverter::init(std::string const& toEncoding, std::string const& fromEncoding)
{
    if (converter != (iconv_t)-1)
    {   errno = 0;
        if (0 != iconv_close(converter))
            JFATAL(0, 0); 
    }
    converter = (iconv_t)-1; 

    size_t const magicNumber = 4 * 1024; 

    input.reset(new char[magicNumber]); 
    inputSize = 0;
    inputCapacity = magicNumber; 

    output.reset(new char[magicNumber]); 
    outputSize = 0;
    outputCapacity = magicNumber; 

#ifdef _WIN32
    SetLastError(0); 
#endif
    errno = 0; 
    converter = iconv_open(toEncoding.c_str(), fromEncoding.c_str());
    if (converter == (iconv_t)-1)
    {   
#ifdef _WIN32
        DWORD const lastError = GetLastError(); 
#endif
        int const lastErrno = errno; 
        string message; 
        message += "jjm::IconvConverter::init() failed. Cause:\n";
        message += "iconv_open(\"" + toEncoding + "\", \"" + fromEncoding + "\") failed. Cause:\n";
#ifdef _WIN32
        message += "GetLastError() " + toDecStr(lastError) + ". errno " + toDecStr(lastErrno) + "."; 
#else
        message += "errno " + toDecStr(lastErrno) + "."; 
#endif
        throw std::runtime_error(message); 
    }
}

jjm::IconvConverter::~IconvConverter()
{
    if (converter != (iconv_t)-1)
    {   errno = 0;
        if (0 != iconv_close(converter))
            JFATAL(0, 0); 
    }
}

void jjm::IconvConverter::convert()
{
#ifdef _WIN32
    char const * inbuf = input.get(); 
#else
    char * inbuf = input.get(); 
#endif
    char * outbuf = output.get();
    size_t inputSizeLeft = inputSize; 
    size_t outputSizeLeft = outputCapacity; 

#ifdef _WIN32
    SetLastError(0); 
#endif
    errno = 0; 
    size_t const iconvReturn = iconv(converter, & inbuf, & inputSizeLeft, & outbuf, & outputSizeLeft); 
#ifdef _WIN32
    DWORD const lastError = GetLastError(); 
#endif
    int const lastErrno = errno; 

    memmove(input.get(), inbuf, inputSizeLeft); 
    inputSize = inputSizeLeft; 

    outputSize = outputCapacity - outputSizeLeft; 

    if (iconvReturn != (size_t)-1)
        return; 
    if (lastErrno == EILSEQ)
    {   string message; 
        message += "jjm::IconvConverter::convert() failed. Cause:\n";
        message += "iconv(...) failed. Cause:\n";
        message += "errno EILSEQ, the input contains a sequence of bytes which are invalid in the input encoding, "; 

        //Perhaps contrary to documentation, this is how my version of iconv seems to behave. 
        message += "or a valid character in the input cannot be mapped to a valid character in the output encoding."; 

        throw std::runtime_error(message); 
    }
    if (lastErrno == EINVAL)
    {   //input ends in an incomplete sequence, keep it there for next time, 
        return; 
    }
    if (lastErrno == E2BIG)
    {   //out of space in the output buffer, the buffer has good stuff, return success
        return; 
    }
    string message; 
    message += "jjm::IconvConverter::IconvConverter() failed. Cause:\n";
    message += "iconv(...) failed. Cause:\n";
#ifdef _WIN32
    message += "GetLastError() " + toDecStr(lastError) + ". errno " + toDecStr(lastErrno) + "."; 
#else
    message += "errno " + toDecStr(lastErrno) + "."; 
#endif
    throw std::runtime_error(message); 
}

void jjm::IconvConverter::resetState()
{
    inputSize = 0;
    outputSize = 0; 

#ifdef _WIN32
    SetLastError(0);
#endif
    errno = 0; 
    size_t const x = iconv(converter, 0, 0, 0, 0);
#ifdef _WIN32
    DWORD const lastError = GetLastError(); 
#endif
    int const lastErrno = errno; 

    if (x == (size_t)-1)
    {   string message; 
        message += "jjm::IconvConverter::resetState() failed. Cause:\n";
        message += "iconv(iconv_handle, 0, 0, 0, 0) failed. Cause:\n";
#ifdef _WIN32
        message += "GetLastError() " + toDecStr(lastError) + ". errno " + toDecStr(lastErrno) + "."; 
#else
        message += "errno " + toDecStr(lastErrno) + "."; 
#endif
        throw std::runtime_error(message); 
    }
}


string jjm::iconvConvert(
        std::string const& toEncoding, 
        std::string const& fromEncoding, 
        std::string const& text
        )
{
    try
    {
        string result; 
        IconvConverter converter;
        converter.init(toEncoding, fromEncoding); 
        if (text.size() == 0)
            return result; 
        char const * textbegin = &text[0]; 
        char const * const textend = &text[0] + text.size(); 
        for (;;)
        {   if (textbegin == textend)
            {   if (converter.inputSize == 0)
                    return result; 
                throw std::runtime_error("The input text ended with an incomplete encoding sequence.");
            }
            ssize_t toCopy = std::min<ssize_t>(textend - textbegin, converter.inputCapacity - converter.inputSize); 
            memcpy(converter.input.get() + converter.inputSize, textbegin, toCopy); 
            textbegin += toCopy; 
            converter.inputSize += toCopy; 
            converter.convert(); 
            result.append(converter.output.get(), converter.output.get() + converter.outputSize); 
        }
    } catch (std::exception & e)
    {   string message; 
        message += "jjm::iconvConvert() failed. Cause:\n"; 
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}
