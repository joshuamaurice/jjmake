// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JPATH_HPP_HEADER_GUARD
#define JOSUTILS_JPATH_HPP_HEADER_GUARD

#include "junicode/jutfstring.hpp"

#include <algorithm> //for std::swap
#include <string>
#include <utility> //for std::swap
#include <vector>

namespace jjm
{

/*
The Path class changes behavior depending on the taget platform of the 
compiler. 


Definition: Separator, File Separator,

Certain characters are file separators. When compiled for POSIX systems, the 
only file-separator is slash '/'. When compiled for Windows systems, slash '/' 
and backslash '\\' are the file separator characters. 


Definition: Component

A component is a non-empty string. Separator-chars cannot be in
a component. A null char '\0' cannot be in a component. 

Windows filesystems have additional restrictions, but they are not enforced 
directly by this class (except for makeRealPath()). 


Definition: Path

A path is a single string which has the following parts in the following order: 

- Prefix. Prefixes are not used for POSIX file systems. Windows filesystems 
often make use of prefixes. For example, "C:" is a prefix. 

- Zero or more separator characters. A path with one or more separator chars
in this position is called absolute. Path which are not absolute are called 
relative. 

- Zero or more components, separated by one or more separator chars. 

- Zero or more separator chars. These separator chars are ignored. 
However, trailing separator chars for a path with zero components are 
significant. It's the difference between relative paths and absolute paths. 


Definition: Empty Path

An empty string is the empty path. It has zero components. It is relative. 
(It is not absolute because it does not have any separator chars.) 

A relative path with zero components may not have a prefix. Such a thing is an
invalid path. (If necessary, use the "." component, ex:  "C:."  .) 


Definition: Root Path

A root path is an absolute path with zero components. It may or may not have a 
prefix. 
*/

class Path
{
public:
    Path() {} 
    Path(Path const& rhs) : path(rhs.path) {}

    explicit Path(Utf8String const& path); 
    explicit Path(Utf16String const& path); 

    ~Path() {}

    Path& operator= (Path rhs) { this->swap(rhs); return *this; } 

    void swap(Path & rhs) { using std::swap; swap(path, rhs.path); } 
    friend void swap(Path & x, Path & y) { x.swap(y); }

    Utf8String const& getStringRep() const { return path; }

    bool isEmpty() const; 
    bool isAbsolute() const; 
    bool isRootPath() const; 

    //append(): Adds a component to the end of the list of components. returns *this. 
    //If component contains a separator character, std::exception will be thrown. 
    Path& append(Utf8String const& component); 
    
    Utf8String getPrefix() const; 

    //If *this has no components, then getFileName() return an empty string. 
    //Otherwise, these functions return the last component. 
    Utf8String getFileName() const; 


    /* getParent()
    The return value depends on the value of this according to the following 
    rules: 
    - zero components         --> return value: 0 components, is relative, empty prefix ... the empty path
    - one component, relative --> return value: 0 components, is relative, empty prefix ... the empty path
    - one component, absolute --> return value: 0 components, is absolute, same prefix ... a root path
    - two components          --> return value: removed last component, same absolute/relative, same prefix
    */
    Path getParent() const; 

    
    /* 
    resolve() has the following formal behavior: 

    If this and root have incompatible prefixes (both non-empty and unequal), 
    then resolve() returns a simple copy of this with no modification. 

    Otherwise, if this is absolute, then resolve() returns the following path: 
        - The non-empty prefix (if any) of this and root. 
        - The result is absolute.
        - The components of this. 

    Otherwise, this is relative, and resolve() returns the following path: 
        - The non-empty prefix (if any) of this and root. 
        - The result is absolute iff root is absolute
        - The components of root followed by the components of this. 
    
    Note: The components "." and ".." are not treated specially. If you want 
    to resolve these components, then call getRealPath(). 
    */
    Path resolve(Path const& root) const; 


    //Same behavior as: Path tmp(child); tmp.resolve(root); return tmp; 
    static Path join(Path const& root, Path const& child)
            { return child.resolve(root); }


    //If this path is already absolute, then this function returns a copy. 
    //Otherwise, this function returns an absolute path with the same prefix
    //and the same components. 
    Path getAbsoluteVersion() const; 


    /* 
    getAbsolutePath() consults the current working directory and returns
    an absolute path which names the same file or directory as *this path. 
    
    On POSIX, this call has the following effect: 
        if (isEmpty())
            return *this;
        this->resolve(getCurrentWorkingDirectory())

    On Windows, this call has the following behavior: 
        if (isEmpty())
            return *this;
        if (this->getPrefix().sizeBytes() == 0)
            this->setPrefix(getCurrentWorkingDrive() + ":");
        if (this->isAbsolute() == false)
            this->resolve(getCurrentWorkingDirectoryOfDrive(getPrefix() + ".");

    Finally, "." components and ".." components are handled in the following 
    way. 

    All "." components are removed. 

    If the path contains a ".." components with a preceding component which is
    not a ".." component, then remove both components; repeat this process for
    as long as you can. If a ".." component remains, then return the empty 
    path. 

    Posix Note: 

    When compiled for POSIX, when called on a path with a prefix, this function
    will throw a std::exception. 

    Windows Note: 

    On windows, the current-working-drive and the 
    current-working-directories are stored in a simple hidden unprotected, 
    unsynchronized hidden global variable in the process. 

    That means calling this function while another thread in the process 
    changes the current-working-dir is a data race which can cause memory 
    corruption. 

    We suggest that you should never modify the current-working-dirs during the
    process's life. Instead, changes to the current-working-dirs should be 
    done when constructing the process. (The same suggestion also applies to
    environmental variables.) 
    */
    Path getAbsolutePath() const; 


    /* 
    If the path does not name a file or directory, then getRealPath() throws
    std::exception. 
    TODO docs
    */
    Path getRealPath() const; 

    /* 
    As getRealPath2(), except if *this does not name a file or directory on the
    filesystem, then getRealPath() returns the empty path. 
    TODO docs
    */
    Path getRealPath2() const; 


#ifndef _WIN32
    //Uses iconv
    //Works together with another jjm utility to call setlocale(LC_ALL, ""). 
    std::string getLocalizedString() const; 

    //Uses iconv
    //Works together with another jjm utility to call setlocale(LC_ALL, ""). 
    static Path fromLocalizedString(std::string const& localizedString); 
#endif


private:
    //The path member variable is kept in a normalized form: 
    //No trailing separators (except for root paths). 
    //No adjacent separators. 
    //The only separator chars are the platform's preferred separator char. 
    Utf8String path; 
};

} //namespace jjm

#endif
