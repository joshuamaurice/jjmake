// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jstat.hpp"

#include "jfilehandle.hpp"
#include "jopen.hpp"
#include "jbase/jfatal.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <sys/time.h>
    #include <sys/stat.h>
    #include <unistd.h>
    //#include <sys/types.h> 
#endif

#include <algorithm>
#include <cerrno>
#include <sstream>

using namespace jjm;
using namespace std;




#ifdef _WIN32
    namespace
    {
        inline FileType getFileType(
                    DWORD const dwFileAttributes, 
                    DWORD const ReparseTag, 
                    jjm::Path const& path
                    )
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DEVICE)    return FileType::Other; 
            if (dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)   return FileType::Other; 
            if (dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) return FileType::Other; 
            if (dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    return FileType::Other; 
            if (dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) return FileType::Other; 
            if ((dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && ReparseTag == IO_REPARSE_TAG_SYMLINK) return FileType::Symlink;
            if (dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) return FileType::Other;
            if (dwFileAttributes & FILE_ATTRIBUTE_NORMAL)      return FileType::RegularFile;
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return FileType::Directory;
            JFATAL(dwFileAttributes, path.getStringRep()); 
        }
    }
    
    template <typename StatT>
    inline void init(
                StatT * const st, 
                bool const resolveSymlinks, 
                bool const throwExceptionOnError, 
                jjm::Path const& path, 
                FileHandle const file, 
                char const * const stname
                ) 
    {
        FILE_ATTRIBUTE_TAG_INFO fileAttributeInfo; 
        SetLastError(0);
        errno = 0; 
        BOOL const x1 = GetFileInformationByHandleEx(
                file.native(), FileAttributeTagInfo, & fileAttributeInfo, sizeof(fileAttributeInfo)); 
        if (0 == x1)
        {   if ( ! throwExceptionOnError)
                return; 
            DWORD const lastError = GetLastError(); 
            string msg = string() + stname + " failed. "
                    + "GetFileInformationByHandleEx() on \"" + path.getStringRep().c_str() 
                    + "\" failed. ";  
            throw runtime_error(msg + "GetLastError returned " + toDecStr(lastError) + ".");
        }

        FILE_BASIC_INFO fileBasicInfo; 
        SetLastError(0);
        errno = 0; 
        BOOL const x2 = GetFileInformationByHandleEx(
                file.native(), FileStandardInfo, & fileBasicInfo, sizeof(fileBasicInfo)); 
        if (0 == x2)
        {   if ( ! throwExceptionOnError)
                return; 
            DWORD const lastError = GetLastError(); 
            string msg = string() + stname + " failed. "
                    + "GetFileInformationByHandleEx() on \"" + path.getStringRep().c_str() 
                    + "\" failed. ";  
            throw runtime_error(msg + "GetLastError returned " + toDecStr(lastError) + ".");
        }

        st->type = getFileType(fileAttributeInfo.FileAttributes, fileAttributeInfo.ReparseTag, path); 

        st->lastWriteTimeNanoSec = fileBasicInfo.LastWriteTime.QuadPart; 
        st->lastWriteTimeNanoSec *= static_cast<std::int64_t>(100); //multiply by 100 to convert to nano-seconds
        
        st->lastChangeTimeNanoSec = fileBasicInfo.ChangeTime.QuadPart; 
        st->lastChangeTimeNanoSec *= static_cast<std::int64_t>(100); //multiply by 100 to convert to nano-seconds
    }

    template <typename StatT>
    inline void init(
                StatT * const st, 
                bool const resolveSymlinks, 
                bool const throwExceptionOnError, 
                jjm::Path const& path, 
                char const * const stname
                ) 
    {   
        //Get a file handle, and get the attributes from the file handle. 
        //We're doing it this way, because it appears to be the simplest way 
        //to resolve and not-resolve symbolic links as needed. 
        FileOpener fileOpener = FileOpener().readOnly().openExistingOnly()
                .win32_fileFlagBackupSemantics(); 
        if (resolveSymlinks == false)
            fileOpener.win32_doNotResolveSymlink(); 
        FileHandleOwner fd(fileOpener.open2(path)); 
        if (fd.get() == FileHandle())
        {   DWORD const lastError = GetLastError(); 
            if (lastError == ERROR_PATH_NOT_FOUND || lastError == ERROR_FILE_NOT_FOUND)
            {   st->type = FileType::NoExist;
                return; 
            }
            if ( ! throwExceptionOnError)
            {   st->type = FileType::Invalid;
                return; 
            }
            string msg = string() + stname + " failed. "
                    + "CreateFileW(\"" + path.getStringRep().c_str() 
                    + "\", <access flags>, 0, 0, <creation flags>, <normal file attributes>) failed. ";
            if (lastError == ERROR_SHARING_VIOLATION)
                throw runtime_error(msg + "GetLastError returned ERROR_SHARING_VIOLATION: The process cannot access the file because it is being used by another process.");
            if (lastError == ERROR_ACCESS_DENIED)
                throw runtime_error(msg + "GetLastError returned ERROR_ACCESS_DENIED: Access is denied.");
            throw runtime_error(msg + "GetLastError returned " + toDecStr(lastError) + ".");
        }

        init(st, resolveSymlinks, throwExceptionOnError, path, fd.get(), stname); 
    }

    jjm::Stat jjm::Stat::stat  (Path const& path) { Stat st; init(&st, true, true, path, "jjm::Stat::stat"); return st; }
    jjm::Stat jjm::Stat::stat2 (Path const& path) { Stat st; init(&st, true, false, path, "jjm::Stat::stat2"); return st; }
    jjm::Stat jjm::Stat::lstat (Path const& path) { Stat st; init(&st, false, true, path, "jjm::Stat::lstat"); return st; }
    jjm::Stat jjm::Stat::lstat2(Path const& path) { Stat st; init(&st, false, false, path, "jjm::Stat::lstat2"); return st; }
    jjm::Stat jjm::Stat::get2  (FileHandle file)  { Stat st; init(&st, false, false, Path(), file, "jjm::Stat::lstat2"); return st; }

#else

    template <typename StatT>
    inline void init(
                StatT * const s, 
                bool const resolveSymlinks, 
                bool const throwExceptionOnError, 
                jjm::Path const& path, 
                char const * const stname
                ) 
    {
        string localizedInput = path.getLocalizedString(); 

        errno = 0; 
        struct stat st;
        int const statResult = resolveSymlinks
                ? ::stat(localizedInput.c_str(), & st)
                : ::lstat(localizedInput.c_str(), & st);
        int const lastErrno = errno; 

        if (statResult != 0)
        {   if (ENOENT == lastErrno || ENOTDIR == lastErrno)
            {   s->type = FileType::NoExist;
                return;
            }
            if ( ! throwExceptionOnError)
            {   s->type = FileType::Invalid;
                return; 
            }
            string message;
            message += string() + stname + " failed. UTF8 path before localization \"" + path.toStdString() + "\". Cause:\n";
            message += string() + (resolveSymlinks ? "::stat" : "::lstat") + "(<path>) failed. Cause:\n";
            if (EACCES == lastErrno)
                throw runtime_error(message + "errno EACCES, search permission is denied on a component of the path prefix, or write permission is denied on the parent directory of the directory to be created.");
            throw runtime_error(message + "errno " + toDecStr(lastErrno) + ".");
        }

        if (     S_ISREG(st.st_mode)) s->type = FileType::RegularFile;
        else if (S_ISDIR(st.st_mode)) s->type = FileType::Directory;
        else if (S_ISLNK(st.st_mode)) s->type = FileType::Symlink;
        else if (S_ISBLK(st.st_mode)) s->type = FileType::Other;
        else if (S_ISCHR(st.st_mode)) s->type = FileType::Other;
        else if (S_ISFIFO(st.st_mode)) s->type = FileType::Other;
        else                           JFATAL(0, path.toStdString());

        if (resolveSymlinks && s->type == FileType::Symlink)
            JFATAL(0, path.toStdString()); 

        s->lastWriteTimeNanoSec = 0;
        s->lastWriteTimeNanoSec += (std::uint64_t)(st.st_mtim.tv_nsec);
        s->lastWriteTimeNanoSec += (std::uint64_t)(st.st_mtim.tv_sec) * (std::uint64_t)1000 * (std::uint64_t)1000 * (std::uint64_t)1000;

        s->lastChangeTimeNanoSec = 0;
        s->lastChangeTimeNanoSec += (std::uint64_t)(st.st_ctim.tv_nsec);
        s->lastChangeTimeNanoSec += (std::uint64_t)(st.st_ctim.tv_sec) * (std::uint64_t)1000 * (std::uint64_t)1000 * (std::uint64_t)1000;
    }

    jjm::Stat jjm::Stat::stat  (Path const& path) { Stat st; init(&st, true, true, path, "jjm::Stat::stat"); return st; }
    jjm::Stat jjm::Stat::stat2 (Path const& path) { Stat st; init(&st, true, false, path, "jjm::Stat::stat2"); return st; }
    jjm::Stat jjm::Stat::lstat (Path const& path) { Stat st; init(&st, false, true, path, "jjm::Stat::lstat"); return st; }
    jjm::Stat jjm::Stat::lstat2(Path const& path) { Stat st; init(&st, false, false, path, "jjm::Stat::lstat2"); return st; }

#endif
