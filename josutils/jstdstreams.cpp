// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstdstreams.hpp"

#include "junicode/jiconv.hpp"
#include "junicode/jutfstring.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
#include <stdlib.h>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

using namespace jjm;
using namespace std;


#ifdef _WIN32
namespace
{
    bool isConnectedToWin32Console(FileHandle handle)
    {
        //get the handle type
        SetLastError(0); 
        DWORD const handleType = GetFileType(handle.native()); 
        if (handleType == FILE_TYPE_UNKNOWN)
        {   DWORD const lastError = GetLastError();
            if (lastError == NO_ERROR)
            {   string message; 
                message += "GetFileType(<handle>) returned FILE_TYPE_UNKNOWN and GetLastError() returned 0."; 
                throw std::runtime_error(message); 
            }
            string message; 
            message += "GetFileType(<handle>) failed. Cause:\n"; 
            message += "GetLastError() " + toDecStr(lastError) + "."; 
            throw std::runtime_error(message); 
        }

        //do our stuff based on the handle type
        switch (handleType)
        {
        case FILE_TYPE_CHAR: return true; 
        case FILE_TYPE_DISK: return false; 
        case FILE_TYPE_PIPE: return false; 
        default: ; 
        }
        string message; 
        message += "GetFileType(<handle>) returned an unexpected value " + toDecStr(handleType) + "."; 
        throw std::runtime_error(message); 
    }
}
#endif

namespace
{
    class StandardInputStream : public jjm::InputStream
    {
    public:

        //does not take ownership of filehandle
        StandardInputStream(FileHandle handle); 

        ~StandardInputStream() {}

        //function is no-op if it detects that this is directly connected to a win32 terminal
        void resetEncoding(string const& encoding); 

        virtual void close() {} //no-op
        virtual std::int64_t seek(std::int64_t off, int whence) { return -1; } //cannot seek on stdin
        virtual ssize_t read(void * buf, std::size_t bytes); 

        virtual Utf8String getLastErrorDescription() const { return lastErrorDescription; }

    private:
        StandardInputStream(StandardInputStream const& ); //not defined, not copyable
        StandardInputStream& operator= (StandardInputStream const& ); //not defined, not copyable

        FileHandle handle; 
        Utf8String lastErrorDescription; 

        jjm::IconvConverter converter; 

    #ifdef _WIN32
        bool isConsole; 
        vector<char> remaining; 
    #endif
    };
}

StandardInputStream::StandardInputStream(FileHandle handle_)
    : handle(handle_)
#ifdef _WIN32
    , isConsole(false) 
#endif
{
    try
    {
#ifdef _WIN32
        isConsole = isConnectedToWin32Console(handle); 
#endif
    }
    catch (std::exception & e)
    {   string message; 
        message += "StandardInputStream::StandardInputStream() failed. Cause:\n"; 
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}

void StandardInputStream::resetEncoding(string const& encoding)
{
#ifdef _WIN32
    if (isConsole)
        return; 
#endif
    try
    {   converter.init("UTF-8", encoding); 
    }catch (std::exception & e)
    {   string message;
        message += "jjm's StandardInputStream::resetEncoding(\"" + encoding + "\") failed. Cause:\n";
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}

ssize_t StandardInputStream::read(void * const argumentBuffer, size_t const argumentBufferBytes)
{
    try
    {   
#ifdef _WIN32
        //if it's a console, read UTF-16 using the console-specific function ReadConsoleW
        if (isConsole)
        {   
            if (remaining.size() == 0)
            {   DWORD numCharsToRead = 0;
                if (argumentBufferBytes < std::numeric_limits<DWORD>::max())
                    numCharsToRead = (DWORD) argumentBufferBytes;
                else
                    numCharsToRead = std::numeric_limits<DWORD>::max(); 
                
                size_t const bufsize = numCharsToRead; 
                UniqueArray<wchar_t*> buf(new wchar_t[bufsize]); 

                DWORD numCharsRead = 0; 

                SetLastError(0); 
                if (0 == ReadConsoleW(handle.native(), buf.get(), numCharsToRead, & numCharsRead, 0))
                {   DWORD const lastError = GetLastError(); 
                    throw std::runtime_error(string() + "ReadConsoleW() failed. Cause:\n" + "GetLastError() " + toDecStr(lastError) + "."); 
                }

                auto x = makeUtf8RangeFromCpRange(makeCpRangeFromUtf16(make_pair(buf.get(), buf.get() + numCharsRead))); 
                remaining.insert(remaining.end(), x.first, x.second); 
            }
            if (remaining.size() > 0)
            {   ssize_t toCopy = std::min<ssize_t>(remaining.size(), argumentBufferBytes); 
                memcpy(argumentBuffer, &remaining[0], toCopy); 
                memmove(&remaining[0], &remaining[0] + toCopy, remaining.size() - toCopy); 
                remaining.resize(remaining.size() - toCopy); 
                return toCopy; 
            }
            return 0; 
        }
#endif
        //otherwise, read it in the encoding of the specified encoding
        bool eof = false; 
        if (converter.outputSize == 0 && converter.inputSize < converter.inputCapacity)
        {   ssize_t x = handle.read(converter.input.get() + converter.inputSize, converter.inputCapacity - converter.inputSize); 
            if (x == -1)
            {   eof = true; 
                x = 0; 
            }
            converter.inputSize += x; 
        }
        converter.convert(); 
        if (eof && converter.outputSize == 0 && converter.inputSize == 0)
            return -1; 
        if (eof && converter.outputSize == 0 && converter.inputSize != 0)
        {   throw std::runtime_error("Incomplete partial encoding sequence found just before end-of-input."); 
            return -2;
        }

        ssize_t toCopy = std::min<ssize_t>(argumentBufferBytes, converter.outputSize); 
        memcpy(argumentBuffer, converter.output.get(), toCopy); 
        memmove(converter.output.get(), converter.output.get() + toCopy, converter.outputSize - toCopy); 
        converter.outputSize -= toCopy; 
        return toCopy; 
    }catch (std::exception & e)
    {   lastErrorDescription.clear();
        lastErrorDescription += "StandardInputStream::read failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -2; 
    }
}


namespace
{
    class StandardOutputStream : public jjm::OutputStream
    {
    public:

        //does not take ownership of filehandle
        StandardOutputStream(FileHandle handle); 

        ~StandardOutputStream() {}

        //function is no-op if it detects that this is directly connected to a win32 terminal
        void resetEncoding(string const& encoding); 

        virtual void close() {} //no-op
        virtual std::int64_t seek(std::int64_t off, int whence) { return -1; } //cannot seek on stdout nor stderr
        virtual ssize_t write(void const* buf, std::size_t bytes); //takes utf8 input
        virtual int flush() { return 0; } //no-op... //TODO ? Is it? 

        virtual Utf8String getLastErrorDescription() const { return lastErrorDescription; }

    private:
        StandardOutputStream(StandardOutputStream const& ); //not defined, not copyable
        StandardOutputStream& operator= (StandardOutputStream const& ); //not defined, not copyable

        FileHandle handle; 
        Utf8String lastErrorDescription; 

        jjm::IconvConverter converter; 

    #ifdef _WIN32
        bool isConsole; 
    #endif
    };
}

StandardOutputStream::StandardOutputStream(FileHandle handle_)
    : handle(handle_)
#ifdef _WIN32
    , isConsole(false)
#endif
{
#ifdef _WIN32
    isConsole = ::isConnectedToWin32Console(handle); 
#endif
}

void StandardOutputStream::resetEncoding(string const& encoding)
{
    try
    {   converter.init(encoding, "UTF-8"); 
    }catch (std::exception & e)
    {   string message;
        message += "jjm's StandardOutputStream::StandardOutputStream(\"" + encoding + "\") failed. Cause:\n";
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}

ssize_t  StandardOutputStream::write(void const * const argumentBuffer, std::size_t const argumentBufferBytes)
{
#ifdef _WIN32
    //if it's a console, write UTF-16 using the console-specific function WriteConsoleW
    if (isConsole)
    {  
        pair<char const*, char const*> u8range;
        u8range.first  = static_cast<char const*>(argumentBuffer);
        u8range.second = static_cast<char const*>(argumentBuffer) + argumentBufferBytes;
        Utf16String u16str = makeU16StrFromCpRange(makeCpRangeFromUtf8(u8range)); 

        wchar_t const * begin = u16str.data();
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
                lastErrorDescription += "StandardOutputStream::write failed. Cause:\n";
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
        lastErrorDescription += "StandardOutputStream::write failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }
}

#ifdef _WIN32
    string jjm::getWindowsOemEncodingName()
    {
        //setlocale returns the Windows-ANSI codepage encoding, not the 
        //Windows-OEM encoding, which is what we want for command line 
        //applications. 
        SetLastError(0);
        size_t const oemCodePageBufferSize = 6; 
        wchar_t oemCodePageBuffer[oemCodePageBufferSize + 1]; 
        int const getLocaleInfoExResult = 
                GetLocaleInfoEx(
                        LOCALE_NAME_USER_DEFAULT, 
                        LOCALE_IDEFAULTCODEPAGE, 
                        oemCodePageBuffer, 
                        oemCodePageBufferSize
                        );
        if (getLocaleInfoExResult == 0)
        {   DWORD const lastError = GetLastError(); 
            string message;
            message += "GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IDEFAULTCODEPAGE, ...) failed. Cause:\n"; 
            message += "GetLastError() " + toDecStr(lastError) + "."; 
            throw std::runtime_error(message);
        }
        oemCodePageBuffer[getLocaleInfoExResult] = 0; 

        string encoding = "CP" + makeU8StrFromCpRange(makeCpRangeFromUtf16(
                make_pair(oemCodePageBuffer + 0, oemCodePageBuffer + getLocaleInfoExResult))); 
        return encoding; 
    }
#else
    string jjm::getEncodingNameFrom_setlocale_LC_ALL_emptyString()
    {
        char const * localeName1 = setlocale(LC_ALL, ""); 
        if (localeName1 == 0)
        {   string message;
            message += "jjm's StandardOutputStream::StandardOutputStream() failed. Cause:\n";
            message += "setlocale(LC_ALL, \"\") returned NULL."; 
            throw std::runtime_error(message); 
        }
        string encoding = localeName1; 
        if (encoding.find('.') == string::npos)
        {   string message;
            message += "jjm's StandardOutputStream::StandardOutputStream() failed. Cause:\n";
            message += string() + "The return value of setlocale(LC_ALL, \"\") -> \"" + localeName1 + "\" does not end in an encoding."; 
            throw std::runtime_error(message); 
        }
        encoding.erase(0, encoding.find('.') + 1); 
        return encoding; 
    }
#endif

void jjm::setJinEncoding (std::string const& encoding)
{
    dynamic_cast<StandardInputStream*>(jin().inputStream())->resetEncoding(encoding); 
}

void jjm::setJoutEncoding(std::string const& encoding)
{
    dynamic_cast<StandardOutputStream*>(jout().outputStream())->resetEncoding(encoding); 
}

void jjm::setJerrEncoding(std::string const& encoding)
{
    dynamic_cast<StandardOutputStream*>(jerr().outputStream())->resetEncoding(encoding); 
    dynamic_cast<StandardOutputStream*>(jlog().outputStream())->resetEncoding(encoding); 
}




jjm::BufferedInputStream  *  jjm::Internal::createJin()
{
    try
    {   
#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        StandardInputStream * x = new StandardInputStream(FileHandle::getstdin()); 
        x->resetEncoding(encoding); 
        BufferedInputStream * y = new BufferedInputStream(x); 
        return y; 
    }
    catch (std::exception & e)
    {   char const message[] = "Failed to initialize jjm::jin(). Cause:\n"; 
        char const * message2 = e.what(); 
        FileHandle::getstderr().writeComplete2(message, sizeof(message) - 1); 
        FileHandle::getstderr().writeComplete2(message2, strlen(message2)); 
        _exit(1); 
    }
}

jjm::BufferedOutputStream *  jjm::Internal::createJout()
{
    try
    {   
#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        StandardOutputStream * x = new StandardOutputStream(FileHandle::getstdout()); 
        x->resetEncoding(encoding); 
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
    {   
#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        StandardOutputStream * x = new StandardOutputStream(FileHandle::getstderr()); 
        x->resetEncoding(encoding); 
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
    {   
#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        StandardOutputStream * x = new StandardOutputStream(FileHandle::getstderr()); 
        x->resetEncoding(encoding); 
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
                message += "StandardOutputStream::StandardOutputStream() failed. Cause:\n";
                message += "GetLocaleInfoEx(<locale-name>, ... ) failed. Cause:\n";
                message += "GetLastError() " + toDecStr(lastError) + "."; 
                throw std::runtime_error(message); 
    }
#endif

