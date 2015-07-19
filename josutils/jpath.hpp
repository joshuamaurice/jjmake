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

Definition: file-separator. Certain characters are file separators. 
When compiled for POSIX systems, the only file-separator is slash '/'. 
When compiled for Windows systems, slash '/' and backslash '\\' are the 
file-separator characters. 

Definition: A path consists of the three members: 
- prefix: A string, possibly empty. (The drive designator for windows, i.e. "C:".) 
- isAbsolute. A boolean. 
- components. A list of strings, where each string has length 1 or more. 

Definition: A path with isAbsolute == false is called relative. 

Specific cases: 

Path: empty prefix, isAbsolute false, no components
This path does not name a file nor directory. This path is called empty. 

Path: empty prefix, isAbsolute true, no compnoents
This path names a directory. It names a root directory. For filesystems 
without prefixes (i.e. POSIX filesystems), there is a single unique root 
directory. 

Path: non-empty prefix, isAbsolute false, no components
This path does not name a file nor directory. If you wanted to name the current
working directory of a particular prefix, then specify the prefix, isAbsolute 
false, and a single component ".". 
*/

class Path
{
public:
    Path() : absolute(false) {}
    Path(Path const& rhs) : prefix(rhs.prefix), absolute(rhs.absolute), components(rhs.components) {}

    /* The string constructors create paths by parsing the string according to 
    the normal conventions. Trailing separators are ignored (except to 
    determine if the string is absolute). */
    explicit Path(std::string const& utf8path); 
    explicit Path(Utf8String const& path); 
    explicit Path(Utf16String const& path); 

    ~Path() {}

    Path& operator= (Path rhs) { this->swap(rhs); return *this; } 

    void swap(Path & rhs)
    {
        using std::swap; 
        swap(prefix, rhs.prefix); 
        swap(absolute, rhs.absolute); 
        swap(components, rhs.components); 
    }

    friend void swap(Path & x, Path & y) { x.swap(y); }


    Utf8String toString() const; 


    //setPrefix() Sets the prefix. Returns *this. 
    Path& setPrefix(std::string const& utf8prefix_) { prefix = U8Str::utf8(utf8prefix_); return *this; }
    Path& setPrefix(Utf8String const& prefix_) { prefix = prefix_; return *this; }
    Path& setPrefix(Utf16String const& prefix_) { prefix = U8Str::cp(prefix_); return *this; }

    Utf8String const& getPrefix() const { return prefix; }


    bool isAbsolute() const { return absolute; }
    Path& setAbsolute(bool absolute_) { absolute = absolute_; return *this; } 

    //append(): Adds a component to the end of the list of components. returns *this. 
    //If component contains a separator character, std::exception will be thrown. 
    Path& append(std::string const& utf8component) { components.push_back(U8Str::utf8(utf8component)); return *this; }
    Path& append(Utf8String const& component) { components.push_back(component); return *this; }
    Path& append(Utf16String const& component) { components.push_back(U8Str::cp(component)); return *this; }
    
    std::vector<Utf8String> const& getComponents() const { return components; }


    //If *this has no components, then it returns an empty string. 
    //Otherwise, it returns the last component. 
    Utf8String const& getFileName() const { if (components.size() == 0) return getEmptyString(); else return components.back(); }


    /* getParent()
    First it makes a copy. Then it removes all trailing "." components from 
    the copy. Then it applies the applicable applicable rule to the copy: 
    - zero components         --> clear components,         set relative,    same prefix
    - one component, absolute --> clear components,         set absolute,    same prefix
    - one component, relative --> reset to 1 component ".", set relative,    same prefix
    - two or more components  --> remove last component,    same isAbsolute, same prefix
    */
    Path getParent() const; 

    
    /* resolve() combines *this path and the root path. 
    In short, it modifies *this path by considering it as a path in the
    directory named by root. 

    resolve() has the following formal behavior: 
    - If this->prefix is no-empty, and root.prefix is non-empty, and 
        this->prefix != root.prefix, then the call has no effect.
    - Otherwise, the call has the following effect: 
        if (this->getPrefix().sizeBytes() == 0)
            this->setPrefix(root.getPrefix());
        if (this->isAbsolute() == false)
        {   this->setAbsolute(root.isAbsolute()); 
            this->components.prepend(root.getComponents()); 
        }

    Note: The components "." and ".." are preserved. If you want to resolve 
    "." and "..", then call getRealPath(). 

    In all cases, resolve() returns *this. 
    */
    Path& resolve(Path const& root); 


    //Same behavior as: Path tmp(child); tmp.resolve(root); return tmp; 
    static Path join(Path const& root, Path const& child)
            { Path tmp(child); tmp.resolve(root); return tmp; }


    /* makeAbsolute() changes *this path to be absolute.
    
    On POSIX, this call has the following effect: 
        this->resolve(getCurrentWorkingDirectory())

    On Windows, this call has the following behavior: 
        if (this->getPrefix().sizeBytes() == 0)
            this->setPrefix(getCurrentWorkingDrive() + ":");
        if (this->isAbsolute() == false)
            this->resolve(getCurrentWorkingDirectoryOfDrive(getPrefix() + ".");

    makeAbsolute() returns this. 

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
    Path& makeAbsolute(); 

    //Same behavior as: Path tmp(*this); tmp.makeAbsolute(); return tmp; 
    Path getAbsolutePath() const { Path tmp(*this); tmp.makeAbsolute(); return tmp; }


    //TODO
    Path getRealpath() const; 

private:
    static U8Str const& getEmptyString(); 

    U8Str prefix; 
    bool absolute; 
    std::vector<U8Str> components; 
};

} //namespace jjm

#endif
