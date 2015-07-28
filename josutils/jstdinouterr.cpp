// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstdinouterr.hpp"
#include "junicode/jiconv.hpp"
#include "junicode/jutfstring.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

using namespace jjm;
using namespace std;


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

        jjm::IconvConverter converter; 

    #ifdef _WIN32
        bool isConsole; 
    #endif
    };
}

StandardOutputErrorStream::StandardOutputErrorStream()
#ifdef _WIN32
    : isConsole(false)
#endif
{
}

StandardOutputErrorStream::StandardOutputErrorStream(FileHandle handle_)
    : handle(handle_)
#ifdef _WIN32
    , isConsole(false)
#endif
{
    char const * localeName1 = setlocale(LC_ALL, ""); 
    if (localeName1 == 0)
    {   string message;
        message += "jjm's StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
        message += "setlocale(LC_ALL, \"\") returned NULL."; 
        throw std::runtime_error(message); 
    }
    string encoding = localeName1; 
    if (encoding.find('.') == string::npos)
    {   string message;
        message += "jjm's StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
        message += string() + "The return value of setlocale(LC_ALL, \"\") -> \"" + localeName1 + "\" does not end in an encoding."; 
        throw std::runtime_error(message); 
    }
    encoding.erase(0, encoding.find('.') + 1); 
#ifdef _WIN32
    //TODO
    encoding = "CP" + encoding; 

    //TODO get the OEM codepage, not the win-ASCI codepage. 
    //setlocale returns the win-ASCI codepage, which is not what we want. 
#endif

    try
    {   converter.init(encoding, "UTF-8"); 
    }catch (std::exception & e)
    {   string message;
        message += "jjm's StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
        message += e.what(); 
        throw std::runtime_error(message); 
    }

#ifdef _WIN32
    //get the handle type
    SetLastError(0); 
    DWORD const handleType = GetFileType(handle.native()); 
    if (handleType == FILE_TYPE_UNKNOWN)
    {   DWORD const lastError = GetLastError();
        if (lastError == NO_ERROR)
        {   string message; 
            message += "jjm's StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
            message += "GetFileType(<handle>) returned FILE_TYPE_UNKNOWN and GetLastError() returned 0."; 
            throw std::runtime_error(message); 
        }
        string message; 
        message += "jjm's StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
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
            message += "jjm's StandardOutputErrorStream::StandardOutputErrorStream() failed. Cause:\n";
            message += "GetFileType(<handle>) returned an unrecognized value " + toDecStr(handleType) + "."; 
            throw std::runtime_error(message); 
        }
    }
#endif
}

StandardOutputErrorStream::~StandardOutputErrorStream() {}

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
#endif

    try
    {
        //otherwise, write it in the encoding of the specified locale
        char const * argumentBufferCurrent = reinterpret_cast<char const*>(argumentBuffer); 
        char const * argumentBufferEnd = reinterpret_cast<char const*>(argumentBuffer) + argumentBufferBytes; 
        for (;;)
        {
            if (argumentBufferCurrent == argumentBufferEnd)
                return argumentBufferBytes; 

            //copy some number of bytes to converter input
            {
                ssize_t toCopy = std::min<ssize_t>(argumentBufferEnd - argumentBufferCurrent, converter.inputCapacity - converter.inputSize); 
                std::memcpy(
                        converter.input.get() + converter.inputSize, 
                        argumentBufferCurrent, 
                        toCopy
                        ); 
                argumentBufferCurrent += toCopy; 
                converter.inputSize += toCopy; 
            }

            //convert
            converter.convert(); 

            //copy all of the output to the handle
            handle.writeComplete(converter.output.get(), converter.outputSize); 
        }
        JFATAL(0, 0); 
    }catch (std::exception & e)
    {   lastErrorDescription.clear();
        lastErrorDescription += "StandardOutputErrorStream::write failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }
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
