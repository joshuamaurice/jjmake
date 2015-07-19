// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jpath.hpp"

#include "jbase/juniqueptr.hpp"
#include <string>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace jjm; 




namespace
{
#ifdef _WIN32
    bool forceCreationOfMbcsEnv = (getenv("FOO"), false); 
    bool forceCreationOfWideEnv = (_wgetenv(L"FOO"), false); 
#endif

#ifdef _WIN32
    CodePoint const preferredSeparator = '\\';
#else
    CodePoint const preferredSeparator = '/';
#endif

    bool isSeparator(CodePoint c)
    {
        if (c == '/')
            return true;
#ifdef _WIN32
        if (c == '\\')
            return true;
#endif
        return false; 
    }

    bool isLatinLetter(CodePoint c)
    {
        if ('a' <= c && c <= 'z')
            return true;
        if ('A' <= c && c <= 'Z')
            return true; 
        return false; 
    }

    void pathInitUtf8(
                Path * path, 
                char const * arg //may be null for empty, will be null-terminated, 
                )
    {
        path->setAbsolute(false); 

        if (arg == 0)
            return; 

#ifdef _WIN32
        //determine if there's a prefix, 
        if (isLatinLetter(arg[0]) && arg[1] == ':')
        {   path->setPrefix(U8Str::utf8(arg, arg + 2)); 
            arg += 2; 
        }
#endif

        //determine if absolute
        path->setAbsolute(isSeparator(*arg)); 

        //get components
        for (;;)
        {   //get start of component (will also skip trailing separators)
            for ( ; isSeparator(*arg); ++arg)
                ;

            char const * firstNonSeparator = arg; 
            if (*firstNonSeparator == 0)
                break; 

            for ( ; ! isSeparator(*arg); ++arg)
                arg; 
            char const * nextSeparatorOrNull  = arg; 

            path->append(U8Str::utf8(firstNonSeparator, nextSeparatorOrNull)); 
        }
    }

#ifdef _WIN32
    char getCurrentWorkingDrive()
    {
        size_t const output1Size = 260; 
        wchar_t output1[output1Size]; 
        SetLastError(0); 
        DWORD const return1 = GetFullPathNameW(L"\\", output1Size, output1, 0); 
        if (return1 < output1Size)
        {   //normal success
            if (output1[return1] != 0)
                JFATAL(0, 0); //should be null terminated by the win32 API

            //expected that it looks like "A:\\" or "C:\\", etc.
            if (return1 != 3)
                JFATAL(0, 0); 
            if ( ! isLatinLetter(output1[0]))
                JFATAL(0, 0); 
            if (output1[1] != ':')
                JFATAL(0, 0); 
            if (output1[2] != '\\')
                JFATAL(0, 0); 

            return static_cast<char>(output1[0]); 
        }
        if (return1 == 0)
        {   //unexpected error
            DWORD error = GetLastError(); 
            throw std::runtime_error("getCurrentWorkingDrive() failed. "
                    "GetFullPathNameW failed. GetLastError() " + toDecStr(error) + ".");
        }
        //otherwise, the given buffer was not big enough, try again with a bigger buffer
        DWORD const outputSize2 = return1 + 1; 
        jjm::UniqueArray<wchar_t*> output2(new wchar_t[outputSize2]); 
        SetLastError(0); 
        DWORD const return2 = GetFullPathNameW(L"\\", outputSize2, output2.get(), 0); 
        if (return2 < outputSize2)
        {   //success
            if (output2.get()[return2] != 0)
                JFATAL(0, 0); //should be null terminated by the win32 API

            //expected that it looks like "A:\\" or "C:\\", etc.
            if (return1 != 3)
                JFATAL(0, 0); 
            if ( ! isLatinLetter(output1[0]))
                JFATAL(0, 0); 
            if (output1[1] != ':')
                JFATAL(0, 0); 
            if (output1[2] != '\\')
                JFATAL(0, 0); 

            return static_cast<char>(output2[0]); 
        }
        if (return2 == 0)
        {   //unexpected error
            DWORD error = GetLastError(); 
            throw std::runtime_error("getCurrentWorkingDrive() failed. "
                    "GetFullPathNameW failed. GetLastError() " + toDecStr(error) + ".");
        }
        //To get here, GetFullPathNameW needs to have failed a second time, 
        //complaining that the buffer wasn't big enough, when we gave a big enough buffer. 
        JFATAL(0, 0); 
        return 0; 
    }
    U8Str getCurrentWorkingDirectory(char drive)
    {
        wchar_t const input[4] = { drive, ':', '.', '\0' }; 

        size_t const output1Size = 260; 
        wchar_t output1[output1Size]; 
        SetLastError(0); 
        DWORD const return1 = GetFullPathNameW(input, output1Size, output1, 0); 
        if (return1 < output1Size)
        {   //normal success
            if (output1[return1] != 0)
                JFATAL(0, 0); //should be null terminated by the win32 API
            return U8Str::cp(U16Str::utf16(&output1[0], output1 + return1)); //TODO can remove one copy
        }
        if (return1 == 0)
        {   //unexpected error
            DWORD error = GetLastError(); 
            throw std::runtime_error("getCurrentWorkingDirectory(drive) failed. "
                    "GetFullPathNameW failed. GetLastError() " + toDecStr(error) + ".");
        }
        //otherwise, the given buffer was not big enough, try again with a bigger buffer
        DWORD const outputSize2 = return1 + 1; 
        jjm::UniqueArray<wchar_t*> output2(new wchar_t[outputSize2]); 
        SetLastError(0); 
        DWORD const return2 = GetFullPathNameW(input, outputSize2, output2.get(), 0); 
        if (return2 < outputSize2)
        {   //success
            if (output2.get()[return2] != 0)
                JFATAL(0, 0); //should be null terminated by the win32 API
            return U8Str::cp(U16Str::utf16(&output1[0], output1 + return1)); //TODO can remove one copy
        }
        if (return2 == 0)
        {   //unexpected error
            DWORD error = GetLastError(); 
            throw std::runtime_error("getCurrentWorkingDirectory(drive) failed. "
                    "GetFullPathNameW failed. GetLastError() " + toDecStr(error) + ".");
        }
        //To get here, GetFullPathNameW needs to have failed a second time, 
        //complaining that the buffer wasn't big enough, when we gave a big enough buffer. 
        JFATAL(0, 0); 
        return U8Str(); 
    }
#endif
}

jjm::Path::Path(std::string const& utf8path) { pathInitUtf8(this, utf8path.c_str()); }
jjm::Path::Path(Utf8String const& path) { pathInitUtf8(this, path.c_str()); }
jjm::Path::Path(Utf16String const& path) { pathInitUtf8(this, U8Str::cp(path).c_str()); }


jjm::Path jjm::Path::getParent() const
{
    Path result(*this); 
    for ( ; result.components.size(); )
    {   if (result.components.back().sizeEU() == 1 && result.components.back().data()[0] == '.')
            result.components.pop_back(); 
    }
    if (result.components.size() == 0)
    {   result.absolute = false; 
        return result; 
    }
    if (result.components.size() == 1 && result.isAbsolute())
    {   result.components.clear();
        return result; 
    }
    if (result.components.size() == 1 && ! result.isAbsolute())
    {   result.components[0] = a2u8(".");
        return result; 
    }
    result.components.pop_back();
    return result; 
}


jjm::Utf8String jjm::Path::toString() const
{
    U8Str result; 
    result.append(prefix); 
    if (absolute)
        result.appendCP(preferredSeparator);
    for (size_t i=0; i < components.size(); )
    {   result.append(components[i]); 
        ++i; 
        if (i < components.size())
            result.appendCP(preferredSeparator); 
    }
    return result; 
}


jjm::Path &  jjm::Path::resolve(Path const& root)
{
    //Do nothing if *this and root have non-empty prefixes and the prefixes
    //are different
    if (this->prefix.sizeBytes() != 0 && root.prefix.sizeBytes() != 0 && this->prefix != root.prefix)
        return *this; 

    //otherwise: 

    //if prefix is unset, copy from root, 
    if (this->getPrefix().sizeBytes() == 0)
        this->prefix = root.prefix; 

    //if not absolute, then copy isAbsolute from root, and prepend the 
    //components from root, 
    if (this->absolute == false)
    {   this->absolute = root.absolute; 
        this->components.insert(
                this->components.begin(), 
                root.components.begin(), 
                root.components.end()
                ); 
    }

    return *this; 
}


jjm::Path &  jjm::Path::makeAbsolute()
{
#ifdef _WIN32
    if (prefix.sizeBytes() == 0)
    {   char c = getCurrentWorkingDrive(); 
        prefix.appendCP(c);
        prefix.appendCP(':'); 
    }
    if (absolute == false)
    {   char drive = prefix.data()[0]; 
        U8Str currentWorkingDirectory = getCurrentWorkingDirectory(drive); 
        Path root(currentWorkingDirectory);
        resolve(root); 
    }
#else
    if (prefix.sizeBytes() > 0)
        throw std::runtime_error("jjm::Path::makeAbsolute() on POSIX systems does not support prefixes"); 
    if (absolute == false)
        resolve(getCurrentWorkingDirectory();
#endif
    return *this; 
}






#if 0
{
    template <typename String, typename Iter>
    inline String getFileParentImpl(String const& input)
    {
        Iter const begin = pathUtilsGetBeginIterator(input); 
        Iter const end   = pathUtilsGetEndIterator(  input); 
        Iter const afterDriveDesignator = gotoEndOfDriveDesignator(begin, end);

        Iter endOfLastComponent   = gotoEndOfLastComponent(afterDriveDesignator, end);
        Iter beginOfLastComponent = gotoBeginOfLastComponent(afterDriveDesignator, endOfLastComponent);

        //if no components
        if (beginOfLastComponent == endOfLastComponent)
            return String(); 

        //if one component, relative path
        if (afterDriveDesignator == beginOfLastComponent)
            return pathUtilsSubstring(input, begin, afterDriveDesignator) + pathUtilsDot(input); 

        Iter endOfSecondLastComponent = beginOfLastComponent;
        --endOfSecondLastComponent;
        endOfSecondLastComponent = gotoEndOfLastComponent(afterDriveDesignator, endOfSecondLastComponent);

        //if one component, absolute path
        Iter x = endOfSecondLastComponent;
        if (x == afterDriveDesignator || isSeparator(*--x))
            return pathUtilsSubstring(input, begin, ++Iter(afterDriveDesignator)); 

        //two or more components
        return pathUtilsSubstring(input, begin, endOfSecondLastComponent); 
    }
}
std::string jjm::getFileParent(std::string const& path) { return getFileParentImpl<std::string, std::string::const_iterator>(path); }
Utf8String  jjm::getFileParent(Utf8String const& path) { return getFileParentImpl<U8Str, U8Str::EuIterator>(path); }
Utf16String jjm::getFileParent(Utf16String const& path) { return getFileParentImpl<U16Str, U16Str::EuIterator>(path); }


namespace
{
    template <typename String, typename Iter>
    inline bool isAbsolutePathImpl(String const& input)
    {
        Iter const begin = pathUtilsGetBeginIterator(input); 
        Iter const end   = pathUtilsGetEndIterator(  input); 
        Iter const afterDriveDesignator = gotoEndOfDriveDesignator(begin, end);

        //if the path is empty, or if the path only has a drive designator, 
        if (afterDriveDesignator == end)
            return false;

        //if it starts with a separator, then it's an absolute path
        return isSeparator(*afterDriveDesignator); 
    }
}
bool jjm::isAbsolutePath(std::string const& path) { return isAbsolutePathImpl<std::string, std::string::const_iterator>(path); }
bool jjm::isAbsolutePath(Utf8String const& path) { return isAbsolutePathImpl<U8Str, U8Str::EuIterator>(path); }
bool jjm::isAbsolutePath(Utf16String const& path) { return isAbsolutePathImpl<U16Str, U16Str::EuIterator>(path); }



std::string jjm::getAbsolutePath(std::string const& input)
{
#ifdef _WIN32
    Utf8ToCpBidiIterator<char const*> a1(input.c_str(), input.c_str()               , input.c_str() + input.size()); 
    Utf8ToCpBidiIterator<char const*> a2(input.c_str(), input.c_str() + input.size(), input.c_str() + input.size()); 
    U16Str c = getAbsolutePath(U16Str::cp(a1, a2)); 
    std::string d; 
    for (U16Str::CpIterator ci = c.beginCP(); ci != c.endCP(); ++ci)
    {   CodePoint cp = *ci; 
        char buf[4]; 
        char* buf2 = buf; 
        writeUtf8(cp, buf2);
        d.insert(d.end(), buf, buf2); 
    }
    return d; 
#else
    JFATAL(0, 0); //TODO impl
#endif
}
Utf8String  jjm::getAbsolutePath(Utf8String  const& input)
{
#ifdef _WIN32
    U16Str x = U16Str::cp(input); 
    U16Str y = getAbsolutePath(x); 
    return U8Str::cp(y); 
#else
    JFATAL(0, 0); //TODO impl
#endif
}
Utf16String jjm::getAbsolutePath(Utf16String const& input)
{
#ifdef _WIN32
    size_t const outputSize = 260; 
    wchar_t output[outputSize]; 
    SetLastError(0); 
    DWORD const return1 = GetFullPathNameW(input.c_str(), outputSize, output, 0); 
    if (return1 < outputSize)
    {   //normal success
        if (output[return1] != 0)
            JFATAL(0, 0); //should be null terminated by the win32 API
        return U16Str::utf16(&output[0], output + return1); 
    }
    if (return1 == 0)
    {   //unexpected error
        DWORD error = GetLastError(); 
        throw std::runtime_error("jjm::getAbsolutePath(std::string const& ) failed. "
                "GetFullPathNameW failed. GetLastError() " + toDecStr(error) + ".");
    }
    //otherwise, the given buffer was not big enough, try again with a bigger buffer
    DWORD const outputSize2 = return1 + 1; 
    jjm::UniqueArray<wchar_t*> output2(new wchar_t[outputSize2]); 
    SetLastError(0); 
    DWORD const return2 = GetFullPathNameW(input.c_str(), outputSize2, output2.get(), 0); 
    if (return2 < outputSize2)
    {   //success
        if (output2.get()[return2] != 0)
            JFATAL(0, 0); //should be null terminated by the win32 API
        return U16Str::utf16(output2.get(), output2.get() + return2); 
    }
    if (return2 == 0)
    {   //unexpected error
        DWORD error = GetLastError(); 
        throw std::runtime_error("jjm::getAbsolutePath(std::string const& ) failed. "
                "GetFullPathNameW failed. GetLastError() " + toDecStr(error) + ".");
    }
    //To get here, GetFullPathNameW needs to have failed a second time, 
    //complaining that the buffer wasn't big enough, when we gave a big enough buffer. 
    JFATAL(0, 0); 
#else
    U8Str x = U8Str::cp(input); 
    U8Str y = getAbsolutePath(x); 
    return U16Str::cp(y); 
#endif
}


#if 0

#define ASSERT_EQUALS(x, y) \
    if (x != y) \
    cerr << "Failed test at " << __FILE__ << " " << __LINE__ << ". [" << x << "], [" << y << "]" << endl;

namespace
{
    void tests()
    {
        //getFileName
        ASSERT_EQUALS(string("foo"), getFileName(string("C:/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C://foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:/foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C://foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:/foo//"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("C:foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:foo//"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("C:/bar/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C://bar/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:/bar//foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:/bar/foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:/bar/foo//"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("C:bar/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:bar//foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:bar/foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("C:bar/foo//"))); 

        ASSERT_EQUALS(string(""), getFileName(string("C:"))); 
        ASSERT_EQUALS(string(""), getFileName(string("C:/"))); 
        ASSERT_EQUALS(string(""), getFileName(string("C://"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("//foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("/foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("//foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("/foo//"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("foo//"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("/bar/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("//bar/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("/bar//foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("/bar/foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("/bar/foo//"))); 

        ASSERT_EQUALS(string("foo"), getFileName(string("bar/foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("bar//foo"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("bar/foo/"))); 
        ASSERT_EQUALS(string("foo"), getFileName(string("bar/foo//"))); 

        ASSERT_EQUALS(string(""), getFileName(string(""))); 
        ASSERT_EQUALS(string(""), getFileName(string("/"))); 
        ASSERT_EQUALS(string(""), getFileName(string("//"))); 

        //getFileParent
        ASSERT_EQUALS(string("C:/"), getFileParent(string("C:/foo"))); 
        ASSERT_EQUALS(string("C:/"), getFileParent(string("C://foo"))); 
        ASSERT_EQUALS(string("C:/"), getFileParent(string("C:/foo/"))); 
        ASSERT_EQUALS(string("C:/"), getFileParent(string("C://foo/"))); 
        ASSERT_EQUALS(string("C:/"), getFileParent(string("C:/foo//"))); 

        ASSERT_EQUALS(string("C:."), getFileParent(string("C:foo"))); 
        ASSERT_EQUALS(string("C:."), getFileParent(string("C:foo/"))); 
        ASSERT_EQUALS(string("C:."), getFileParent(string("C:foo//"))); 

        ASSERT_EQUALS(string("C:/bar"),  getFileParent(string("C:/bar/foo"))); 
        ASSERT_EQUALS(string("C://bar"), getFileParent(string("C://bar/foo"))); 
        ASSERT_EQUALS(string("C:/bar"),  getFileParent(string("C:/bar//foo"))); 
        ASSERT_EQUALS(string("C:/bar"),  getFileParent(string("C:/bar/foo/"))); 
        ASSERT_EQUALS(string("C:/bar"),  getFileParent(string("C:/bar/foo//"))); 

        ASSERT_EQUALS(string("C:bar"), getFileParent(string("C:bar/foo"))); 
        ASSERT_EQUALS(string("C:bar"), getFileParent(string("C:bar//foo"))); 
        ASSERT_EQUALS(string("C:bar"), getFileParent(string("C:bar/foo/"))); 
        ASSERT_EQUALS(string("C:bar"), getFileParent(string("C:bar/foo//"))); 

        ASSERT_EQUALS(string(""), getFileParent(string("C:"))); 
        ASSERT_EQUALS(string(""), getFileParent(string("C:/"))); 
        ASSERT_EQUALS(string(""), getFileParent(string("C://"))); 

        ASSERT_EQUALS(string("/"), getFileParent(string("/foo"))); 
        ASSERT_EQUALS(string("/"), getFileParent(string("//foo"))); 
        ASSERT_EQUALS(string("/"), getFileParent(string("/foo/"))); 
        ASSERT_EQUALS(string("/"), getFileParent(string("//foo/"))); 
        ASSERT_EQUALS(string("/"), getFileParent(string("/foo//"))); 

        ASSERT_EQUALS(string("."), getFileParent(string("foo"))); 
        ASSERT_EQUALS(string("."), getFileParent(string("foo/"))); 
        ASSERT_EQUALS(string("."), getFileParent(string("foo//"))); 

        ASSERT_EQUALS(string("/bar"),  getFileParent(string("/bar/foo"))); 
        ASSERT_EQUALS(string("//bar"), getFileParent(string("//bar/foo"))); 
        ASSERT_EQUALS(string("/bar"),  getFileParent(string("/bar//foo"))); 
        ASSERT_EQUALS(string("/bar"),  getFileParent(string("/bar/foo/"))); 
        ASSERT_EQUALS(string("/bar"),  getFileParent(string("/bar/foo//"))); 

        ASSERT_EQUALS(string("bar"), getFileParent(string("bar/foo"))); 
        ASSERT_EQUALS(string("bar"), getFileParent(string("bar//foo"))); 
        ASSERT_EQUALS(string("bar"), getFileParent(string("bar/foo/"))); 
        ASSERT_EQUALS(string("bar"), getFileParent(string("bar/foo//"))); 

        ASSERT_EQUALS(string(""), getFileParent(string(""))); 
        ASSERT_EQUALS(string(""), getFileParent(string("/"))); 
        ASSERT_EQUALS(string(""), getFileParent(string("//"))); 
    }
}

#endif

#endif
