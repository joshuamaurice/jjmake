// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstdinouterr.hpp"
#include "junicode/jutfstring.hpp"
#include "junicode/jwinlocale.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <iconv.h>
    #include <unistd.h>
#endif

using namespace jjm;
using namespace std;



#ifdef _WIN32
    class WideToMultibyteConverter
    {
    public:
        WideToMultibyteConverter() {}
        ~WideToMultibyteConverter() {}
        vector<wchar_t> input; 
        vector<char> output; 

        //throws std::exception on errors
        void convert()
        {
        }
    };

    class MultibyteToWideConverter
    {
    public:
        MultibyteToWideConverter() {}
        ~MultibyteToWideConverter() {}
        vector<char> input; 
        vector<wchar_t> output; 

        //throws std::exception on errors
        void convert()
        {
            for (;;)
            {
            }
        }
    };
#endif

/*
StandardOutputErrorStream docs: 

On POSIX and Windows, it does not do newline conversions. 

On POSIX and Windows, you can get a proper locale by calling: 
    setlocale(LC_ALL, "")
which sets the process locale to the proper locale. 

On POSIX systems, this class will use the locale and iconv to convert input 
UTF-8 strings to the encoding of the specified locale, and then it writes the
localized string to the specified handle. 

On Windows, life gets more interesting. 

It checks if the filehandle is a windows console, and if yes, it converts input
UTF-8 strings to UTF-16, and then writes the UTF-16 strings to the filehandle. 
The console will behave properly. 

Otherwise, it uses the specified locale and the Windows functions 
MultiByteToWideChar() and WideCharToMultiByte() to convert from UTF-8 to
the desired encoding, and then it writes the localized string to the 
filehandle. 

This class breaks the contract of jjm::OutputStream slightly. In particular, 
in error scenarios, it may have written some number of bytes. 
*/
namespace
{
    class StandardOutputErrorStream : public jjm::OutputStream
    {
    public:
        StandardOutputErrorStream(); 
        StandardOutputErrorStream(FileHandle handle); //does not take ownership of filehandle
        ~StandardOutputErrorStream(); 
        virtual void close() {} //no-op
        virtual std::int64_t seek(std::int64_t off, int whence) { return -1; } //cannot seek on stdout nor stderr
        virtual ssize_t write(void const* buf, std::size_t bytes); //takes utf8 input
        virtual int flush() { return 0; } //no-op... //TODO ? Is it? 

        Utf8String getLastErrorDescription() const { return lastErrorDescription; }

    private:
        StandardOutputErrorStream(StandardOutputErrorStream const& ); //not defined, not copyable
        StandardOutputErrorStream& operator= (StandardOutputErrorStream const& ); //not defined, not copyable

        FileHandle handle; 
        Utf8String lastErrorDescription; 

    #ifdef _WIN32
        bool isConsole; 
    #else
        vector<char> remainingInput;  
        vector<char> remainingOutput;  
        iconv_t converter; 
    #endif
    };
}

StandardOutputErrorStream::StandardOutputErrorStream()
#ifdef _WIN32
    : isConsole(false)
#else
    : converter((iconv_t)-1)
#endif
{
}

StandardOutputErrorStream::StandardOutputErrorStream(FileHandle handle_)
    : handle(handle_)
#ifdef _WIN32
    , isConsole(false)
#else
    , converter((iconv_t)-1)
#endif
{
#ifdef _WIN32
    //get the handle type
    SetLastError(0); 
    DWORD const handleType = GetFileType(handle.native()); 
    if (handleType == FILE_TYPE_UNKNOWN)
    {   DWORD const lastError = GetLastError();
        if (lastError == NO_ERROR)
        {   string message; 
            message += "StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
            message += "GetFileType(<handle>) returned FILE_TYPE_UNKNOWN and GetLastError() returned 0."; 
            throw std::runtime_error(message); 
        }
        string message; 
        message += "StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
        message += "GetFileType(<handle>) failed. Cause:\n"; 
        message += "GetLastError() " + toDecStr(lastError) + "."; 
        throw std::runtime_error(message); 
    }

    //do our stuff based on the handle type
    switch (handleType)
    {
    case FILE_TYPE_CHAR: isConsole = true; break;
    case FILE_TYPE_DISK: break; 
    case FILE_TYPE_PIPE: break; 
    default: 
        {
            string message; 
            message += "StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
            message += "GetFileType(<handle>) returned an unrecognized value " + toDecStr(handleType) + "."; 
            throw std::runtime_error(message); 
        }
    }
#else
    char const * const localeName1 = setlocale(LC_ALL, 0); 
    if (localeName1 == 0)
        JFATAL(0, 0); 
    string tocode = localeName1; 
    if (tocode.find('.') == string::npos)
        JFATAL(0, tocode); 
    tocode.erase(0, tocode.find('.') + 1); 

    errno = 0; 
    converter = iconv_open("CP437", "UTF-8");
    if (converter == (iconv_t)-1)
    {   int const lastErrno = errno; 
        string message; 
        message += "StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
        message += "iconv_open(\"" + tocode + "\", \"UTF-8\"). failed. Cause:\n";
        message += "errno " + toDecStr(lastErrno) + "."; 
        throw std::runtime_error(message); 
    }
#endif
}

StandardOutputErrorStream::~StandardOutputErrorStream()
{
#ifndef _WIN32
    if (converter != (iconv_t)-1)
    {   errno = 0;
        if (0 != iconv_close(converter))
            JFATAL(0, 0); 
    }
#endif
}

ssize_t  StandardOutputErrorStream::write(void const * const argumentBuffer, std::size_t const argumentBufferBytes)
{
#ifdef _WIN32
    pair<char const*, char const*> u8range;
    u8range.first  = static_cast<char const*>(argumentBuffer);
    u8range.second = static_cast<char const*>(argumentBuffer) + argumentBufferBytes;
    Utf16String u16str = makeU16StrFromCpRange(makeCpRangeFromUtf8(u8range)); 

    //if it's a console, write UTF-16 using the console-specific function WriteConsoleW
    if (isConsole)
    {   wchar_t const * begin = u16str.data();
        wchar_t const * const end   = u16str.data() + u16str.size();
        for ( ; begin != end; )
        {   ssize_t remaining = end - begin; 
            DWORD numWritten = 0; 
            SetLastError(0); 
            BOOL const writeConsoleWReturn = 
                    WriteConsoleW(
                            handle.native(), 
                            begin, 
                            ((remaining > std::numeric_limits<DWORD>::max()) ? std::numeric_limits<DWORD>::max() : remaining), 
                            & numWritten, 
                            0
                            );
            if (0 == writeConsoleWReturn)
            {   DWORD const lastError = GetLastError(); 
                lastErrorDescription.clear();
                lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
                lastErrorDescription += "WriteConsoleW() failed. Cause:\n";
                lastErrorDescription += "GetLastError() " + toDecStr(lastError) + "."; 
                return -1; 
            }
            begin += numWritten; 
        }
        return argumentBufferBytes; 
    }

    //otherwise, write it in the encoding of the specified locale
    
    //get the required size
    SetLastError(0); 
    int const wideCharToMultiByteReturn1 = 
            WideCharToMultiByte(
                    CP_OEMCP, 
                    WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS, 
                    u16str.c_str(), 
                    -1, 
                    0,
                    0, 
                    0, 
                    0
                    ); 
    if (wideCharToMultiByteReturn1 == 0)
    {   DWORD const lastError = GetLastError();
        lastErrorDescription.clear();
        lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
        lastErrorDescription += "Call to WideCharToMultiByte() to get the buffer size failed. Cause:\n";
        lastErrorDescription += "GetLastError() " + toDecStr(lastError) + "."; 
        return -1; 
    }

    //do the conversion
    UniqueArray<char*> outputBuffer(new char[wideCharToMultiByteReturn1]);
    SetLastError(0); 
    int const wideCharToMultiByteReturn2 = 
            WideCharToMultiByte(
                    CP_OEMCP, 
                    WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS, 
                    u16str.c_str(), 
                    -1, 
                    outputBuffer.get(), 
                    wideCharToMultiByteReturn1, 
                    0, //use default replacement-char
                    0 //don't care if a replacement-char was used
                    ); 
    if (wideCharToMultiByteReturn2 == 0)
    {   DWORD const lastError = GetLastError();
        lastErrorDescription.clear();
        lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
        lastErrorDescription += "Call to WideCharToMultiByte() to do the conversion failed. Cause:\n";
        lastErrorDescription += "GetLastError() " + toDecStr(lastError) + "."; 
    }

    if (outputBuffer.get()[wideCharToMultiByteReturn1 - 1] != 0)
        JFATAL(0, 0); 

    try
    {   handle.writeComplete(outputBuffer.get(), wideCharToMultiByteReturn1 - 1); 
    } catch (std::exception & e)
    {   lastErrorDescription.clear();
        lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }

    return argumentBufferBytes;
#else
    size_t const magicNumber = 1024; 

    remainingInput.insert(
            remainingInput.end(), 
            reinterpret_cast<char const*>(argumentBuffer), 
            reinterpret_cast<char const*>(argumentBuffer) + argumentBufferBytes); 
    for (;;)
    {   char * inbuf = &remainingInput[0]; 
        size_t inbytes = remainingInput.size(); 

        remainingOutput.resize(remainingOutput.size() + magicNumber); 
        char * outbuf = &remainingOutput[0]; 
        size_t outbytes = magicNumber; 

        errno = 0; 
        size_t const iconvReturn = iconv(converter, & inbuf, & inbytes, & outbuf, & outbytes); 
        int lastErrno = errno; 

        //drop the stuff that was just converted
        remainingInput.erase(remainingInput.begin(), remainingInput.begin() + (inbuf - &remainingInput[0])); 

        //trim the output vec to contain data instead of "capacity"
        remainingOutput.erase(remainingOutput.begin() + (outbuf - &remainingOutput[0]), remainingOutput.end()); 

        if (iconvReturn == (size_t)-1)
        {   if (lastErrno == EINVAL)
            {   //input sequence is in the middle of a character, we're good
            }else if (lastErrno == E2BIG)
            {   //output buffer doesn't have enough space left, we're good, 
            }else
            {   lastErrorDescription.clear();
                lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
                lastErrorDescription += "iconv() failed. Cause:\n";
                lastErrorDescription += "errno " + toDecStr(lastErrno) + "."; 
                return -1;  
            }
        }
        if (remainingOutput.size() == 0)
            return argumentBufferBytes; 

        try
        {   handle.writeComplete(&remainingOutput[0], remainingOutput.size()); 
        } catch (std::exception & e)
        {   lastErrorDescription.clear();
            lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
            lastErrorDescription += e.what(); 
            return -1; 
        }
    }
    JFATAL(0, 0); 
#endif
}




jjm::BufferedInputStream  *  jjm::Internal::createJin()
{
    return 0; 
}

jjm::BufferedOutputStream *  jjm::Internal::createJout()
{
    try
    {   char const * localeName1 = setlocale(LC_ALL, ""); 
        if (localeName1 == 0)
        {   string message;
            message += "Failed to initialize jjm::jout(). Cause:\n";
            message += "setlocale(LC_ALL, \"\") returned null."; 
            FileHandle::getstderr().writeComplete2(message.c_str(), message.size()); 
            _exit(1); 
        }

        StandardOutputErrorStream * x = new StandardOutputErrorStream(FileHandle::getstdout()); 
        BufferedOutputStream * y = new BufferedOutputStream(x); 
        return y; 
    }
    catch (std::exception & e)
    {   char const message[] = "Failed to initialize jjm::jout(). Cause:\n"; 
        char const * message2 = e.what(); 
        FileHandle::getstderr().writeComplete2(message, sizeof(message) - 1); 
        FileHandle::getstderr().writeComplete2(message2, strlen(message2)); 
        _exit(1); 
    }
}

jjm::BufferedOutputStream *  jjm::Internal::createJerr()
{
    try
    {   char const * localeName1 = setlocale(LC_ALL, ""); 
        if (localeName1 == 0)
        {   string message;
            message += "Failed to initialize jjm::jerr(). Cause:\n";
            message += "setlocale(LC_ALL, \"\") returned null."; 
            FileHandle::getstderr().writeComplete2(message.c_str(), message.size()); 
            _exit(1); 
        }

        StandardOutputErrorStream * x = new StandardOutputErrorStream(FileHandle::getstderr()); 
        BufferedOutputStream * y = new BufferedOutputStream(x); 
        return y; 
    }
    catch (std::exception & e)
    {   char const message[] = "Failed to initialize jjm::jerr(). Cause:\n"; 
        char const * message2 = e.what(); 
        FileHandle::getstderr().writeComplete2(message, sizeof(message) - 1); 
        FileHandle::getstderr().writeComplete2(message2, strlen(message2)); 
        _exit(1); 
    }
}

jjm::BufferedOutputStream *  jjm::Internal::createJlog()
{
    try
    {   char const * localeName1 = setlocale(LC_ALL, ""); 
        if (localeName1 == 0)
        {   string message;
            message += "Failed to initialize jjm::jlog(). Cause:\n";
            message += "setlocale(LC_ALL, \"\") returned null."; 
            FileHandle::getstderr().writeComplete2(message.c_str(), message.size()); 
            _exit(1); 
        }

        StandardOutputErrorStream * x = new StandardOutputErrorStream(FileHandle::getstderr()); 
        BufferedOutputStream * y = new BufferedOutputStream(x); 
        return y; 
    }
    catch (std::exception & e)
    {   char const message[] = "Failed to initialize jjm::jlog(). Cause:\n"; 
        char const * message2 = e.what(); 
        FileHandle::getstderr().writeComplete2(message, sizeof(message) - 1); 
        FileHandle::getstderr().writeComplete2(message2, strlen(message2)); 
        _exit(1); 
    }
}


namespace { std::int64_t forceStdOutFlushCount; }
jjm::Internal::ForceStdOutFlush::ForceStdOutFlush()
{
    ++forceStdOutFlushCount; 
}
jjm::Internal::ForceStdOutFlush::~ForceStdOutFlush()
{
    if (0 == --forceStdOutFlushCount)
        jout() << flush; 
}

namespace { std::int64_t forceStdErrFlushCount; }
jjm::Internal::ForceStdErrFlush::ForceStdErrFlush()
{
    ++forceStdErrFlushCount; 
}
jjm::Internal::ForceStdErrFlush::~ForceStdErrFlush()
{
    if (0 == --forceStdErrFlushCount)
        jerr() << flush; 
}

namespace { std::int64_t forceStdLogFlushCount; }
jjm::Internal::ForceStdLogFlush::ForceStdLogFlush()
{
    ++forceStdLogFlushCount; 
}
jjm::Internal::ForceStdLogFlush::~ForceStdLogFlush()
{
    if (0 == --forceStdLogFlushCount)
        jlog() << flush; 
}



    //localeName = "en-US"; 

    //get the code page id for that locale string
#if 0
    SetLastError(0); 
    int const getLocaleInfoExReturn = 
            GetLocaleInfoEx(
                    makeU16Str(localeName).c_str(), 
                    LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER, 
                    reinterpret_cast<wchar_t *>( & this->codePageId), 
                    6
                    ); 
    if (getLocaleInfoExReturn == 0)
    {   DWORD const lastError = GetLastError();
        string message; 
                message += "StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
                message += "GetLocaleInfoEx(<locale-name>, ... ) failed. Cause:\n";
                message += "GetLastError() " + toDecStr(lastError) + "."; 
                throw std::runtime_error(message); 
    }
#endif
