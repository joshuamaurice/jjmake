// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstdstreams.hpp"

#include "jenv.hpp"
#include "jfilestreams.hpp"
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

    class Win32ConsoleUtf16InputStream : public jjm::InputStream
    {
    public:
        //does not take ownership of filehandle
        Win32ConsoleUtf16InputStream(FileHandle handle); 

        ~Win32ConsoleUtf16InputStream() {}

        virtual void close() {} //no-op
        virtual std::int64_t seek(std::int64_t off, int whence) { return -1; } //cannot seek on stdin
        virtual ssize_t read(void * buf, std::size_t bytes); 

        virtual Utf8String getLastErrorDescription() const { return lastErrorDescription; }

    private:
        Utf8String lastErrorDescription; 
        FileHandle handle; 
        vector<char> remaining; 
    };

    class Win32ConsoleUtf16OutputStream : public jjm::OutputStream
    {
    public:

        //does not take ownership of filehandle
        Win32ConsoleUtf16OutputStream(FileHandle handle); 

        ~Win32ConsoleUtf16OutputStream() {}

        virtual void close() {} //no-op
        virtual std::int64_t seek(std::int64_t off, int whence) { return -1; } //cannot seek on stdout nor stderr
        virtual ssize_t write(void const* buf, std::size_t bytes); //takes utf8 input
        virtual int flush() { return 0; } //no-op... //TODO ? Is it? 

        virtual Utf8String getLastErrorDescription() const { return lastErrorDescription; }

    private:
        FileHandle handle; 
        Utf8String lastErrorDescription; 
    };
}

Win32ConsoleUtf16InputStream::Win32ConsoleUtf16InputStream(FileHandle handle_)
    : handle(handle_)
{
}

ssize_t Win32ConsoleUtf16InputStream::read(void * const argumentBuffer, size_t const argumentBufferBytes)
{
    try
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

    }catch (std::exception & e)
    {   lastErrorDescription.clear();
        lastErrorDescription += "StandardInputStream::read failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -2; 
    }
}

Win32ConsoleUtf16OutputStream::Win32ConsoleUtf16OutputStream(FileHandle handle_)
    : handle(handle_)
{
}

ssize_t  Win32ConsoleUtf16OutputStream::write(void const * const argumentBuffer, std::size_t const argumentBufferBytes)
{
    try
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

    }catch (std::exception & e)
    {   lastErrorDescription.clear();
        lastErrorDescription += "StandardOutputStream::write failed. Cause:\n";
        lastErrorDescription += e.what(); 
        return -1; 
    }
}
#endif




void jjm::setJinEncoding (std::string const& encoding)
{
    EncodingConverterInputStream * x = 
            dynamic_cast<EncodingConverterInputStream *>(jin().inputStream());
    if (x)
        x->reset(x->getStream(), "UTF-8", encoding); 
}

void jjm::setJoutEncoding(std::string const& encoding)
{
    EncodingConverterOutputStream * x = 
            dynamic_cast<EncodingConverterOutputStream *>(jout().outputStream());
    if (x)
        x->reset(x->getStream(), "UTF-8", encoding); 
}

void jjm::setJerrEncoding(std::string const& encoding)
{
    EncodingConverterOutputStream * x = 
            dynamic_cast<EncodingConverterOutputStream *>(jerr().outputStream());
    if (x)
        x->reset(x->getStream(), "UTF-8", encoding); 
    x = 
            dynamic_cast<EncodingConverterOutputStream *>(jlog().outputStream());
    if (x)
        x->reset(x->getStream(), "UTF-8", encoding); 
}




jjm::BufferedInputStream *  jjm::Internal::createJin()
{
    try
    {   
#ifdef _WIN32
        if (isConnectedToWin32Console(FileHandle::getstdin()))
        {   Win32ConsoleUtf16InputStream * x = new Win32ConsoleUtf16InputStream(FileHandle::getstdin()); 
            BufferedInputStream * y = new BufferedInputStream(x); 
            return y; 
        }
#endif

#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        InputStream * x = 0;
        x = new jjm::FileStream(FileHandle::getstdin()); 
        x = new jjm::EncodingConverterInputStream(x, "UTF-8", encoding); 
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
        if (isConnectedToWin32Console(FileHandle::getstdout()))
        {   Win32ConsoleUtf16OutputStream * x = new Win32ConsoleUtf16OutputStream(FileHandle::getstdout()); 
            BufferedOutputStream * y = new BufferedOutputStream(x); 
            return y; 
        }
#endif

#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        OutputStream * x = 0;
        x = new jjm::FileStream(FileHandle::getstdout()); 
        x = new jjm::EncodingConverterOutputStream(x, "UTF-8", encoding); 
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
        if (isConnectedToWin32Console(FileHandle::getstderr()))
        {   Win32ConsoleUtf16OutputStream * x = new Win32ConsoleUtf16OutputStream(FileHandle::getstderr()); 
            BufferedOutputStream * y = new BufferedOutputStream(x); 
            return y; 
        }
#endif

#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        OutputStream * x = 0;
        x = new jjm::FileStream(FileHandle::getstderr()); 
        x = new jjm::EncodingConverterOutputStream(x, "UTF-8", encoding); 
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
        if (isConnectedToWin32Console(FileHandle::getstderr()))
        {   Win32ConsoleUtf16OutputStream * x = new Win32ConsoleUtf16OutputStream(FileHandle::getstderr()); 
            BufferedOutputStream * y = new BufferedOutputStream(x); 
            return y; 
        }
#endif

#ifdef _WIN32
        string encoding = getWindowsOemEncodingName(); 
#else
        string encoding = getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 
#endif
        OutputStream * x = 0;
        x = new jjm::FileStream(FileHandle::getstderr()); 
        x = new jjm::EncodingConverterOutputStream(x, "UTF-8", encoding); 
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

