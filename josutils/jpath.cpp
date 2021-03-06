// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jpath.hpp"

#include "jfilehandle.hpp"
#include "jopen.hpp"
#include "jstat.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"
#include "jbase/juniqueptr.hpp"
#include <string>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
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
    char const preferredSeparator = '\\';
#else
    char const preferredSeparator = '/';
#endif

    bool isSeparator(UnicodeCodePoint c)
    {
        if (c == '/')
            return true;
#ifdef _WIN32
        if (c == '\\')
            return true;
#endif
        return false; 
    }

    void pathInit(Utf8String & result, Utf8String const& input) 
    {
        if (input.size() == 0)
            return; 
        char const * const begin = &input[0];
        char const * const end = begin + input.size(); 

        char const * p = begin; 

#ifdef _WIN32
        if (input.size() >= 4 && p[0] == '\\' && p[1] == '\\' && p[2] == '.' && p[3] == '\\')
        {   throw std::runtime_error(""
                    "jjm::Path::Path() failed. "
                    "Paths that start with \"\\\\.\\\" name devices. "
                    "jjm::Path does not support paths like this. "
                    "This is a purposeful design decision because the author does not have sufficient understanding to deal with it.");
        }

        if (input.size() >= 4 && p[0] == '\\' && p[1] == '\\' && p[2] == '?' && p[3] == '\\')
        {   throw std::runtime_error(""
                    "jjm::Path::Path() failed. "
                    "Paths that start with \"\\\\?\\\" are passed without modification to the underlying filesystem. "
                    "jjm::Path does not support paths like this. "
                    "This is a purposeful design decision because the author does not have sufficient understanding to deal with it.");
        }

        if (input.size() >= 2 && p[0] == '\\' && p[1] == '\\')
        {   throw std::runtime_error(""
                    "jjm::Path::Path() failed. "
                    "Paths that start with \"\\\\\" have special meaning to the Windows APIs. "
                    "jjm::Path does not support paths like this. "
                    "This is a purposeful design decision because the author does not have sufficient understanding to deal with it.");
        }

        if (input.size() >= 2 && isAsciiLetter(p[0]) && p[1] == ':')
        {   result.append(p, p + 2);
            p += 2; 
        }
#endif

        //is-absolute
        bool const isAbs = isSeparator(*p);
        if (isAbs)
            result.push_back(preferredSeparator); 

        //get components
        size_t numComponents = 0; 
        bool needsSeparator = false; 
        for (;;)
        {   //get start of component (will also skip trailing separators)
            for ( ; p != end && isSeparator(*p); ++p)
                ;

            if (p == end)
                break; 
            char const * const firstNonSeparator = p; 

            for ( ; p != end && ! isSeparator(*p); ++p)
                ; 
            char const * const nextSeparatorOrEnd = p; 

            if (needsSeparator)
                result.push_back(preferredSeparator); 
            result.append(firstNonSeparator, nextSeparatorOrEnd); 
            needsSeparator = true; 
            ++numComponents;
        }

        if (isAbs == false && numComponents == 0)
        {   //this path does not name a file nor directory.
            //this path is an empty path
            //remove any prefix
            result.clear(); 
        }
    }


}

jjm::Path::Path(Utf8String const& input) { pathInit(path, input); }

#ifdef _WIN32
    jjm::Path  jjm::Path::win32device(Utf8String const& name)
    {
        Path path; 
        path.path += "\\\\.\\"; 
        path.path += name; 
        return path; 
    }
#endif

namespace
{
    pair<size_t, size_t> getPrefix(char const * const data, size_t endPos)
    {
    #ifdef _WIN32
        if (endPos >= 2 && isAsciiLetter(data[0]) && data[1] == ':')
            return std::pair<size_t, size_t>(0, 2); 
        return std::pair<size_t, size_t>(0, 0); 
    #else
        return std::pair<size_t, size_t>(0, 0); //TODO
    #endif
    }
    pair<size_t, size_t> getPrefix(Utf8String const& path)
    {
        return getPrefix(path.data(), path.size()); 
    }

    inline pair<size_t, size_t> getPrevComponent(Utf8String const& path, size_t const pos0)
    {
        pair<size_t, size_t> result(0, pos0); 

        pair<size_t, size_t> prefix = ::getPrefix(path); 
        result.first += prefix.second; 

        //otherwise empty path
        if (result.first == result.second)
            return result; 

        //do we have a trailing separator? skip it
        --result.second; 
        if (path.data()[result.second] == preferredSeparator)
        {   if (result.first == result.second)
            {   //str is a single separator char, return empty range
                result.first = result.second = pos0; 
                return result; 
            }
            --result.second; 
        }
        if (path.data()[result.second] == preferredSeparator)
            JFATAL(0, path); 

        ++result.second; //now points at one-past-end or a separator char

        //find the next separator to complete the component
        size_t pos = result.second - 1; 
        for (;;)
        {   if (result.first == pos)
                return result; 
            --pos;
            if (path.data()[pos] == preferredSeparator)
            {   result.first = pos + 1;
                return result; 
            }
        }
    }

    inline pair<size_t, size_t> getNextComponent(char const * const data, size_t const size)
    {
        pair<size_t, size_t> result(0, size); 

        //nothing left
        if (result.first == result.second)
            return result; 

        //do we have a leading separator? skip it
        if (data[result.first] == preferredSeparator)
        {   ++result.first; 
            if (result.first == result.second)
                return result; 
        }
        if (data[result.first] == preferredSeparator)
            JFATAL(0, string(data, data + size)); 

        //find the next separator to complete the component
        size_t pos = result.first; 
        for (;;)
        {   if (pos == result.second)
                return result; 
            ++pos;
            if (data[pos] == preferredSeparator)
            {   result.second = pos;
                return result; 
            }
        }
    }
    inline pair<size_t, size_t> getNextComponent(Utf8String const& path, size_t const pos0)
    {   
        pair<size_t, size_t> result = getNextComponent(path.data() + pos0, path.size() - pos0); 
        result.first += pos0; 
        result.second += pos0; 
        return result; 
    }
}

jjm::Utf8String  jjm::Path::getPrefix() const
{
    pair<size_t, size_t> prefix = ::getPrefix(path); 
    return Utf8String(path.data() + prefix.second, path.data() + path.size()); 
}

bool jjm::Path::isEmpty() const
{
    return path.size() == 0; 
}

bool jjm::Path::isAbsolute() const
{
    pair<size_t, size_t> prefix = ::getPrefix(path); 
    return path.size() >= prefix.second + 1 && path.data()[prefix.second] == preferredSeparator; 
}

bool jjm::Path::isRootPath() const
{
    pair<size_t, size_t> prefix = ::getPrefix(path); 
    return path.size() == prefix.second + 1 && path.data()[prefix.second] == preferredSeparator; 
}

jjm::Path  jjm::Path::getAbsoluteVersion() const
{
    if (isAbsolute())
        return *this; 
    pair<size_t, size_t> prefix = ::getPrefix(path); 
    Path result; 
    result.path.append(path.data(), path.data() + prefix.second);
    result.path.push_back(preferredSeparator);
    result.path.append(path.data() + prefix.second, path.data() + path.size());
    return result; 
}

jjm::Path& jjm::Path::append(Utf8String const& component)
{
    if ( ! isRootPath() && path.size() > 0 && path.data()[path.size() - 1] != preferredSeparator)
        path.push_back(preferredSeparator); 
    path.append(component.data(), component.data() + component.size()); 
    return *this; 
}

jjm::Utf8String jjm::Path::getFileName() const
{
    Utf8String result; 
    pair<size_t, size_t> const lastComponent = getPrevComponent(path, path.size()); 
    result.append(path.data() + lastComponent.first, path.data() + lastComponent.second); 
    return result; 
}

jjm::Path jjm::Path::getParent() const
{
    Path result; 

    pair<size_t, size_t> const lastComponent = getPrevComponent(path, path.size()); 
    if (lastComponent.first == lastComponent.second) //if no components
        return result; //return empty path

    pair<size_t, size_t> const secondToLastComponent = getPrevComponent(path, lastComponent.first); 
    if (secondToLastComponent.first == secondToLastComponent.second) //if only one component
    {   if (isAbsolute() == false) //if only one component, and relative path
            return result; //return empty path
        //here, the path has exactly one component, and absolute path
        pair<size_t, size_t> const prefix = ::getPrefix(path); 
        result.path.append(path.data() + prefix.first, path.data() + prefix.second); 
        result.path.push_back(preferredSeparator); 
        return result; //return a root path
    }

    //here, path has two or more components, 
    //just remove the last component
    result.path.append(path.begin(), path.begin() + secondToLastComponent.second); 
    return result; 
}

jjm::Path  jjm::Path::resolve(Path const& root) const
{
    pair<size_t, size_t> const thisPrefix = ::getPrefix(this->path);
    pair<size_t, size_t> const rootPrefix = ::getPrefix(root.path);

    //Do nothing if *this and root have different non-empty prefixes
    if (thisPrefix.first != thisPrefix.second 
            && rootPrefix.first != rootPrefix.second 
            && ! std::equal(this->path.data() + thisPrefix.first, this->path.data() + thisPrefix.second, root.path.data()))
    {
        return *this; 
    }

    Path result; 

    //append the non-empty prefix of this and root
    if (thisPrefix.first != thisPrefix.second)
        result.path.append(this->path.data() + thisPrefix.first, this->path.data() + thisPrefix.second);
    else if (rootPrefix.first != rootPrefix.second)
        result.path.append(root.path.data() + rootPrefix.first, root.path.data() + rootPrefix.second);

    if ( ! isAbsolute())
    {   //if this is not absolute, then prepend the components of root, and the is-abs of root
        result.path.append(root.path.data() + rootPrefix.second, root.path.data() + root.path.size());
        //then append the components of *this
        if (thisPrefix.second != path.size())
        {   result.path.push_back(preferredSeparator); 
            result.path.append(this->path.data() + thisPrefix.second, this->path.data() + this->path.size()); 
        }
    }else
    {   result.path.append(this->path.data() + thisPrefix.second, this->path.data() + this->path.size()); 
    }

    return result; 
}


#ifdef _WIN32
namespace
{
    inline Utf8String invokeGetFullPathW(Utf8String const& input)
    {
        Utf16String input2 = makeU16Str(input); 

        size_t const output1Size = 260; 
        wchar_t output1[output1Size]; 

        SetLastError(0); 
        DWORD const return1 = GetFullPathNameW(input2.c_str(), output1Size - 1, output1, 0); 
        if (return1 < output1Size)
        {   //normal success
            if (output1[return1] != 0)
                JFATAL(0, 0); //should be null terminated by the win32 API
            return makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(output1 + 0, output1 + return1)));
        }
        if (return1 == 0)
        {   //unexpected error
            DWORD const lastError = GetLastError(); 
            throw std::runtime_error("invokeGetFullPathW() failed. "
                    "GetFullPathNameW failed. GetLastError() " + toDecStr(lastError) + ".");
        }

        //otherwise, the given buffer was not big enough, try again with a bigger buffer
        DWORD const outputSize2 = return1; 
        jjm::UniqueArray<wchar_t*> output2(new wchar_t[outputSize2]); 

        SetLastError(0); 
        DWORD const return2 = GetFullPathNameW(input2.c_str(), outputSize2 - 1, output2.get(), 0); 
        if (return2 < outputSize2)
        {   //success
            if (output2.get()[return2] != 0)
                JFATAL(0, 0); //should be null terminated by the win32 API
            return makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(output2.get(), output2.get() + return2)));
        }
        if (return2 == 0)
        {   //unexpected error
            DWORD const lastError = GetLastError(); 
            throw std::runtime_error("invokeGetFullPathW() failed. "
                    "GetFullPathNameW failed. GetLastError() " + toDecStr(lastError) + ".");
        }
        //To get here, GetFullPathNameW needs to have failed a second time, 
        //complaining that the buffer wasn't big enough, when we gave a big enough buffer. 
        JFATAL(0, 0); 
        return Utf8String(); 
    }
}
#endif

jjm::Path  jjm::Path::getAbsolutePath() const
{
#ifdef _WIN32
    if (isEmpty())
        return *this; 

    pair<size_t, size_t> prefix = ::getPrefix(path); 
    bool const hasPrefix = prefix.first != prefix.second; 
    bool const isAbs = isAbsolute(); 

    if (hasPrefix && isAbs)
        return *this; 

    Utf8String root;
    if (hasPrefix)
        root.append(path.data() + prefix.first, path.data() + prefix.second); 
    if (isAbs)
        root.push_back(preferredSeparator); 
    else
        root.push_back('.'); 

    Utf8String root2 = invokeGetFullPathW(root); 
    Path root3(root2); 
    Path result = resolve(root3); 

    pair<size_t, size_t> resultPrefix = ::getPrefix(result.path); 
    if (resultPrefix.first == resultPrefix.second)
        JFATAL(0, path); 
    if ( ! result.isAbsolute())
        JFATAL(0, path); 
    if (result.isRootPath())
        return result; 

    vector<pair<size_t, size_t> > components; 
    for (pair<size_t, size_t> component(resultPrefix.second, resultPrefix.second); ; )
    {   component = ::getNextComponent(result.path, component.second); 
        if (component.first == component.second)
            break; 
        if (1 == component.second - component.first && result.path.data()[component.first] == '.')
            continue; 
        components.push_back(component); 
    }
    for (size_t i = 0; i < components.size(); )
    {   if (2 == components[i].second - components[i].first 
                && result.path.data()[components[i].first] == '.'
                && result.path.data()[components[i].first + 1] == '.')
        {   if (i == 0)
                return Path(); 
            components.erase(components.begin() + i - 1, components.begin() + i + 1); 
            --i; 
        }else
        {   ++i; 
        }
    }

    Path result2; 
    result2.path.append(result.path.data() + resultPrefix.first, result.path.data() + resultPrefix.second); 
    if (components.size() == 0)
        result2.path.push_back(preferredSeparator); 
    for (size_t i = 0; i < components.size(); ++i)
    {   result2.path.push_back(preferredSeparator); 
        result2.path.append(result.path.data() + components[i].first, result.path.data() + components[i].second); 
    }
    return result2; 
#else
    if (isEmpty())
        return *this; 

    pair<size_t, size_t> prefix = ::getPrefix(path); 
    if (prefix.first != prefix.second)
        throw std::runtime_error("jjm::Path::makeAbsolute() on POSIX systems does not support prefixes"); 
    if (isAbsolute() == false)
    {
        errno = 0; 
        UniquePtr<char*, jjm::InvokeFree> currentWorkingDirectory(::getcwd(0, 0)); 
        int const lastErrno = errno;

        if (currentWorkingDirectory.get() == 0)
        {   string msg = string() + "jjm::Path::getAbsolutePath() failed. "
                    + "::getcwd(0, 0) failed. "; 
            //TODO throw runtime_error(msg + "errno " + toDecStr(lastErrno) + " " + getErrnoName(lastErrno) + ".");
            throw runtime_error(msg + "errno " + toDecStr(lastErrno) + ".");
        }
        return resolve(Path(currentWorkingDirectory.get()));
    }
    return *this; 
#endif
}


jjm::Path  jjm::Path::getRealPath() const
{
#ifdef _WIN32
    FileHandleOwner file(FileOpener().readOnly().openExistingOnly().win32_fileFlagBackupSemantics().open2(*this)); 
    if (file.get() == FileHandle())
    {   DWORD const lastError = GetLastError(); 
        string msg = string() + "jjm::Path::getRealPath() failed. "
                + "CreateFileW(\"" + path.c_str() 
                + "\", <access flags>, 0, 0, <creation flags>, <normal file attributes>) failed. ";
        if (lastError == ERROR_PATH_NOT_FOUND)
            throw runtime_error(msg + "GetLastError() ERROR_PATH_NOT_FOUND: The system cannot find the path specified.");
        if (lastError == ERROR_SHARING_VIOLATION)
            throw runtime_error(msg + "GetLastError() ERROR_SHARING_VIOLATION: The process cannot access the file because it is being used by another process.");
        if (lastError == ERROR_FILE_NOT_FOUND)
            throw runtime_error(msg + "GetLastError() ERROR_FILE_NOT_FOUND: The system cannot find the file specified.");
        if (lastError == ERROR_ACCESS_DENIED)
            throw runtime_error(msg + "GetLastError() ERROR_ACCESS_DENIED: Access is denied.");
        throw runtime_error(msg + "GetLastError() " + toDecStr(lastError) + ".");
    }

    DWORD const buffer1Size = 260; 
    wchar_t buffer1[buffer1Size]; 

    SetLastError(0);
    DWORD const return1 = GetFinalPathNameByHandleW(file.get().native(), buffer1, buffer1Size - 1, FILE_NAME_NORMALIZED); 
    if (return1 < buffer1Size)
    {   //success

        bool const expectedPrefix = return1 >= 4 && buffer1[0] == '\\' && buffer1[1] == '\\' && buffer1[2] == '?' && buffer1[3] == '\\'; 
        if ( ! expectedPrefix)
            JFATAL(0, Utf16String(buffer1 + 0, buffer1 + return1)); 

        Utf8String result1 = makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(buffer1 + 4, buffer1 + return1)));
        Path result2(result1);
        return result2; 
    }
    if (return1 == 0)
    {   //unexpected error
        DWORD const lastError = GetLastError(); 
        throw std::runtime_error("jjm::Path::getRealPath() failed. "
                "GetFinalPathNameByHandleW failed. GetLastError() " + toDecStr(lastError) + ".");
    }

    DWORD const buffer2Size = return1; 
    UniqueArray<wchar_t*> buffer2(new wchar_t[buffer2Size]);

    SetLastError(0);
    DWORD const return2 = GetFinalPathNameByHandleW(file.get().native(), buffer2.get(), buffer2Size - 1, FILE_NAME_NORMALIZED); 
    if (return2 < buffer2Size)
    {   //success

        bool const expectedPrefix = return2 >= 4 && buffer2[0] == '\\' && buffer2[1] == '\\' && buffer2[2] == '?' && buffer2[3] == '\\'; 
        if ( ! expectedPrefix)
            JFATAL(0, Utf16String(buffer2.get(), buffer2.get() + return2)); 

        Utf8String result1 = makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(buffer2.get() + 4, buffer2.get() + return2)));
        Path result2(result1);
        return result2; 
    }
    if (return1 == 0)
    {   //unexpected error
        DWORD const lastError = GetLastError(); 
        throw std::runtime_error("jjm::Path::getRealPath() failed. "
                "GetFinalPathNameByHandleW failed. GetLastError() " + toDecStr(lastError) + ".");
    }
    //To get here, GetFinalPathNameByHandleW needs to have failed a second time, 
    //complaining that the buffer wasn't big enough, when we gave a big enough buffer. 
    JFATAL(0, 0); 
#else
    string initialUtf8PathString = getAbsolutePath().getStringRep(); 

    string localizedInput = initialUtf8PathString; //TODO

    errno = 0; 
    UniquePtr<char*, jjm::InvokeFree> localizedResult(::realpath(localizedInput.c_str(), 0)); 
    int const lastErrno = errno; 

    if (localizedResult.get() == 0)
    {   if (lastErrno == ENOENT)
        {   throw std::runtime_error("jjm::Path::getRealPath() failed. "
                    "UTF8 path before localization \"" + initialUtf8PathString + "\". Cause:\n"
                    "::realpath failed. Cause:\n" "errno ENOENT, the path does not name a file on the filesystem.");
        }
        if (lastErrno == ENOTDIR)
        {   throw std::runtime_error("jjm::Path::getRealPath() failed. "
                    "UTF8 path before localization \"" + initialUtf8PathString + "\". Cause:\n"
                    "::realpath failed. Case: \n" "Cause: errno ENOTDIR, the path does not name a file on the filesystem.");
        }
        throw std::runtime_error("jjm::Path::getRealPath() failed. "
                "UTF8 path before localization \"" + initialUtf8PathString + "\". Cause:\n"
                "::realpath failed. Cause:\n" "errno " + toDecStr(lastErrno) + ".");
    }

    string utf8Result(localizedResult.get()); //TODO
    return Path(utf8Result); 

#endif
}

jjm::Path  jjm::Path::getRealPath2() const
{
#ifdef _WIN32
    FileHandleOwner file(FileOpener().readOnly().openExistingOnly().win32_fileFlagBackupSemantics().open2(*this)); 
    if (file.get() == FileHandle())
    {   DWORD const lastError = GetLastError(); 
        if (lastError == ERROR_PATH_NOT_FOUND)
            return Path(); 
        if (lastError == ERROR_FILE_NOT_FOUND)
            return Path(); 

        string msg = string() + "jjm::Path::getRealPath2() failed. "
                + "CreateFileW(\"" + path.c_str() 
                + "\", <access flags>, 0, 0, <creation flags>, <normal file attributes>) failed. ";
        if (lastError == ERROR_SHARING_VIOLATION)
            throw runtime_error(msg + "GetLastError() ERROR_SHARING_VIOLATION: The process cannot access the file because it is being used by another process.");
        if (lastError == ERROR_ACCESS_DENIED)
            throw runtime_error(msg + "GetLastError() ERROR_ACCESS_DENIED: Access is denied.");
        throw runtime_error(msg + "GetLastError() " + toDecStr(lastError) + ".");
    }

    DWORD const buffer1Size = 260; 
    wchar_t buffer1[buffer1Size]; 

    SetLastError(0);
    DWORD const return1 = GetFinalPathNameByHandleW(file.get().native(), buffer1, buffer1Size - 1, FILE_NAME_NORMALIZED); 
    if (return1 < buffer1Size)
    {   //success

        bool const expectedPrefix = return1 >= 4 && buffer1[0] == '\\' && buffer1[1] == '\\' && buffer1[2] == '?' && buffer1[3] == '\\'; 
        if ( ! expectedPrefix)
            JFATAL(0, Utf16String(buffer1 + 0, buffer1 + return1)); 

        Utf8String result1 = makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(buffer1 + 4, buffer1 + return1)));
        Path result2(result1);
        return result2; 
    }
    if (return1 == 0)
    {   //unexpected error
        DWORD const lastError = GetLastError(); 
        throw std::runtime_error("jjm::Path::getRealPath2() failed. "
                "GetFinalPathNameByHandleW failed. GetLastError() " + toDecStr(lastError) + ".");
    }

    DWORD const buffer2Size = return1; 
    UniqueArray<wchar_t*> buffer2(new wchar_t[buffer2Size]);

    SetLastError(0);
    DWORD const return2 = GetFinalPathNameByHandleW(file.get().native(), buffer2.get(), buffer2Size - 1, FILE_NAME_NORMALIZED); 
    if (return2 < buffer2Size)
    {   //success

        bool const expectedPrefix = return2 >= 4 && buffer2[0] == '\\' && buffer2[1] == '\\' && buffer2[2] == '?' && buffer2[3] == '\\'; 
        if ( ! expectedPrefix)
            JFATAL(0, Utf16String(buffer2.get() + 0, buffer2.get() + return2)); 

        Utf8String result1 = makeU8StrFromCpRange(makeCpRangeFromUtf16(make_pair(buffer2.get() + 4, buffer2.get() + return2)));
        Path result2(result1);
        return result2; 
    }
    if (return1 == 0)
    {   //unexpected error
        DWORD const lastError = GetLastError(); 
        throw std::runtime_error("jjm::Path::getRealPath2() failed. "
                "GetFinalPathNameByHandleW failed. GetLastError() " + toDecStr(lastError) + ".");
    }
    //To get here, GetFinalPathNameByHandleW needs to have failed a second time, 
    //complaining that the buffer wasn't big enough, when we gave a big enough buffer. 
    JFATAL(0, 0); 
#else
    string initialUtf8PathString = getAbsolutePath().getStringRep(); 

    string localizedInput = initialUtf8PathString; //TODO

    errno = 0; 
    UniquePtr<char*, jjm::InvokeFree> localizedResult(::realpath(localizedInput.c_str(), 0)); 
    int const lastErrno = errno; 

    if (localizedResult.get() == 0)
    {   if (lastErrno == ENOENT)
            return Path(); 
        if (lastErrno == ENOTDIR)
            return Path(); 
        throw std::runtime_error("jjm::Path::getRealPath2() failed. "
                "UTF8 path before localization \"" + initialUtf8PathString + "\". Cause:\n"
                "::realpath failed. Cause:\n" "errno " + toDecStr(lastErrno) + ".");
    }

    string utf8Result(localizedResult.get()); //TODO
    return Path(utf8Result); 
#endif
}


#ifndef _WIN32
    string jjm::Path::getLocalizedString() const
    {
        //TODO
        return getStringRep();  
    }

    jjm::Path jjm::Path::fromLocalizedString(string const& localizedString)
    {
        //TODO
        return Path(localizedString); 
    }
#endif
