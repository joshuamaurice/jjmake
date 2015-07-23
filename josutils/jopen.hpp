// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOPEN_HPP_HEADER_GUARD
#define JOPEN_HPP_HEADER_GUARD

#include "jpath.hpp"
#include "jfilehandle.hpp"
#include "junicode/jutfstring.hpp"

namespace jjm
{

/* This is a wrapper on top of open() on unix-like and CreateFile() on win32. 
It uses the usual builder idiom to select options. Example usage:
//FileDes fd = FileOpener().createOrOpen().readOnly().openU(path).c_str()); 

All of the functions of this class are async-signal-safe except where 
documented. 
*/
class FileOpener
{
public:

    FileOpener() : m_accessMode(0), m_createMode(0), 
            m_truncate(false), m_append(false), 
            m_win32_fileFlagBackupSemantics(false),
            m_win32_openSymlink(false)
            {}

    //The user must set one of these. 
    FileOpener& readOnly() { m_accessMode = 1; return *this; }
    FileOpener& writeOnly() { m_accessMode = 2; return *this; }
    FileOpener& readWrite() { m_accessMode = 3; return *this; }

    //The user must set one of these. 
    FileOpener& createOrOpen() { m_createMode = 1; return *this; }
    FileOpener& createNewOnly() { m_createMode = 2; return *this; }
    FileOpener& openExistingOnly() { m_createMode = 3; return *this; }

    //The user may optionally set truncate. 
    //Default ctor sets this to false. 
    FileOpener& truncate(bool b = true) { m_truncate = b; return *this; }

    /* The user may optionally set append. 
    Default ctor sets this to false. 
    
    The append flag guarantees that all writes go to the end of the file, 
    even when the FileDes is accessed concurrently in a single process, and 
    even when the underlying file is accessed concurrently by multiple
    processes. 
    This is accomplished on unix-like by specifying O_APPEND to open(). //TODO confirm
    This is accomplished on win32 by using FILE_APPEND_DATA | SYNCHRONIZE
    without any other write access rights. //TODO confirm
    See: 
        Dev Center - Desktop > Learn > Reference > Data Access and Storage > Local File Systems > File Management > File Management Reference > File Management Functions > CreateFile
        http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858%28v=vs.85%29.aspx

        Dev Center - Desktop > Learn > Reference > Data Access and Storage > Local File Systems > File Management > About File Management > File Security and Access Rights
        http://msdn.microsoft.com/en-us/library/windows/desktop/aa364399%28v=vs.85%29.aspx

        Windows Driver Kit / Device and Driver Technologies / Installable File System / Reference / IO Manager Routines / IoCreateFileSpecifyDeviceObject
        http://msdn.microsoft.com/en-us/library/ff548289.aspx
    */
    FileOpener& append(bool b = true) { m_append = b; return *this; }


#ifdef _WIN32
    /* Gives FILE_FLAG_BACKUP_SEMANTICS for dwFlagsAndAttributes argument to 
    CreateFileW. This is often used to open a directory as a file. 

    Default ctor sets this to false. */
    FileOpener& win32_fileFlagBackupSemantics(bool b = true) { m_win32_fileFlagBackupSemantics = b; return *this; }

    /* Gives FILE_FLAG_OPEN_REPARSE_POINT for dwFlagsAndAttributes argument to 
    CreateFileW. 

    Default ctor sets this to false. */
    FileOpener& win32_doNotResolveSymlink(bool b = true) { m_win32_openSymlink = b; return *this; }
#endif


    /* The open*() functions of this class are a thin wrapper no top of 
    CreateFileW for Windows and ::open() for POSIX systems. 

    Windows filesystems are always UTF-16. The given path will be converted to
    UTF-16 and given to CreateFileW. 

    On POSIX systems, the common convention is to convert a file path to a 
    multibyte string in the encoding of the current locale. 
    
    Returned handles are not inheritable to child processes. This is done by
    giving O_CLOEXEC to open() on unix-like, and passing 0 as 
    lpSecurityAttributes arg to CreateFile() on win32. 

    The user owns the returned file handle. It is the reponsibility of the user
    to close the handle to avoid a resource leak. 

    This function throws std::exception on errors. 
    
    FileOpener::open() is not async-signal-safe. 
    */
    FileHandle open(Path const& path) const; 


    /*
    FileOpener::open2() behaves like FileOpener::open(), except it returns an
    invalid-handle FileHandle object on errors. To determine the cause of the 
    error, use GetLastError() on Windows and errno on POSIX systems. 

    FileOpener::open2() is not async-signal-safe. 
    */
    FileHandle open2(Path const& path) const; 


#ifndef _WIN32
    /*
    FileOpener::openAsyncSignalSafe() behaves like open(), except as follows. 

    FileOpener::openAsyncSignalSafe() returns an invalid-handle FileHandle
    object on errors. To determine the cause of the error, use GetLastError() 
    on Windows and errno on POSIX systems. 

    FileOpener::openAsyncSignalSafe() does no manipulation nor transformation
    of the given path. It is passed directly to ::open(). 

    FileOpener::openAsyncSignalSafe() is async-signal-safe. 
    */
    FileHandle open_asyncSignalSafe(char const* path) const; 
#endif


private:
    int m_accessMode; //Impl notes: 0 unset, 1 readOnly, 2 writeOnly, 3 readWrite
    int m_createMode; //Impl notes: 0 unset, 1 createOrOpen, 2 createNewOnly, 3 openExistingOnly
    bool m_truncate;
    bool m_append; 
    bool m_win32_fileFlagBackupSemantics;
    bool m_win32_openSymlink; 
};

} //namespace jjm

#endif
