// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jprocess.hpp"

#include "jenv.hpp"
#include "jfilehandle.hpp"
#include "jthreading.hpp"
#include "jopen.hpp"
#include "jpipe.hpp"

#include "jbase/jfatal.hpp"
#include "jbase/juniqueptr.hpp"
#include "jbase/jinttostring.hpp"
#include "junicode/jutfstring.hpp"

#ifdef _WIN32
    #include <wchar.h>
    #include <windows.h>
#else
    #include <dirent.h>
    #include <fcntl.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <streambuf>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace jjm;


#ifdef _WIN32
    unsigned long long jjm::getPid() { return GetCurrentProcessId(); }
#else
    unsigned long long jjm::getPid() { return ::getpid(); }
#endif


namespace
{
    
    //guaranteed by c standard. 
    static_assert('1' - '0' == 1, "ERROR");
    static_assert('2' - '0' == 2, "ERROR");
    static_assert('3' - '0' == 3, "ERROR");
    static_assert('4' - '0' == 4, "ERROR");
    static_assert('5' - '0' == 5, "ERROR");
    static_assert('6' - '0' == 6, "ERROR");
    static_assert('7' - '0' == 7, "ERROR");
    static_assert('8' - '0' == 8, "ERROR");
    static_assert('9' - '0' == 9, "ERROR");

    inline bool weakIsDigit(char c)
    {
        switch(c)
        {
        case '0': return true;
        case '1': return true;
        case '2': return true;
        case '3': return true;
        case '4': return true;
        case '5': return true;
        case '6': return true;
        case '7': return true;
        case '8': return true;
        case '9': return true;
        default: return false;
        }
    }

    /* Returns -1 on error or empty input. 
    Handles only strings made of digits. No leading negative sign. No decimals.
    No hex prefix. No oct prefix. Etc. */
    inline int weakAtoi(char const* x)
    {
        if (*x == 0)
            return -1;
        int ret = 0;
        for ( ; *x; ++x)
        {   if ( ! weakIsDigit(*x))
                return -1;
            ret *= 10;
            ret += *x - '0';
        }
        return ret;
    }

    class Win32CreateProcessEnvWrapper
    {
    public:
        Win32CreateProcessEnvWrapper(bool doInit, map<Utf8String, Utf8String> const& x)
        {   
            if (!doInit)
                return;
            Utf16String str;
            for (map<Utf8String, Utf8String>::const_iterator y = x.begin(); y != x.end(); ++y)
            {   appendCp(str, makeCpRange(y->first)); 
                appendCp(str, '='); 
                appendCp(str, makeCpRange(y->second)); 
                appendCp(str, 0); 
            }
            appendCp(str, 0); 
            appendCp(str, 0); 

            envarray.reset(new wchar_t[str.size()]);
            memcpy(envarray.get(), str.data(), str.size() * 2);
        }
        jjm::UniqueArray<wchar_t*> envarray;
    };

    class PosixEnvironWrapper
    {
    public:
        PosixEnvironWrapper() : env(0) {}
        ~PosixEnvironWrapper() { clear(); }
        void init(map<string, string> const& x)
        {   clear();
            env = new char* [x.size() + 1];
            for (size_t i=0; i<x.size() + 1; ++i)
                env[i] = 0;

            map<string, string>::const_iterator y = x.begin(); 
            size_t i=0;
            for ( ; y != x.end(); ++y, ++i)
            {   size_t const size = y->first.size() + 1 + y->second.size() + 1;
                env[i] = new char[size];
                memcpy(env[i], y->first.data(), y->first.size());
                env[i][y->first.size()] = '=';
                memcpy(env[i] + y->first.size() + 1, y->second.data(), y->second.size());
                env[i][y->first.size() + 1 + y->second.size()] = 0;
            }
        }
        void clear()
        {   if (!env) return;
            for (char const * const * x = env; *x; ++x)
                delete[] *x;
            delete[] env;
            env = 0;
        }
        char ** env; 
    };
}

#ifdef _WIN32
    void jjm::Process::join()
    {   
        for (;;)
        {   if ( ! GetExitCodeProcess(processHandle, & exitCode))
                JFATAL(0, 0);
            if (STILL_ACTIVE != exitCode)
                break;
            DWORD const waitResult = WaitForSingleObject(processHandle, INFINITE);
            if (WAIT_OBJECT_0 != waitResult && WAIT_TIMEOUT != waitResult)
                JFATAL(waitResult, 0);
        }
    }

    int jjm::Process::getExitCode() { return exitCode; }

    jjm::Process::Process() : processHandle(0) {}

    jjm::Process::~Process()
    {   
        if (processHandle && !CloseHandle(processHandle)) 
            JFATAL(0, 0);
    }

    namespace
    {   
        void makeProperlyQuotedWindowsCMainArgumentString(
                    jjm::UniqueArray<wchar_t*> & processArguments, 
                    vector<jjm::Utf8String> const& cmd)
        {
            Utf16String processArgumentsStr;
            for (vector<Utf8String>::const_iterator arg = cmd.begin(); arg != cmd.end(); ++arg)
            {   
                bool containsWhitespace = false;
                auto cpRange = makeCpRange(*arg); 
                for (auto c = cpRange.first; c != cpRange.second; ++c)
                    containsWhitespace = containsWhitespace | static_cast<bool>(iswspace(*c));

                if (processArgumentsStr.size())
                    appendCp(processArgumentsStr, ' '); 
                if (containsWhitespace)
                    appendCp(processArgumentsStr, '\"'); 

                size_t backslashCount = 0;
                for (auto c = cpRange.first; c != cpRange.second; ++c)
                {   if (*c == '\"')
                    {   for (size_t i = 0; i < backslashCount; ++i)
                            appendCp(processArgumentsStr, '\\'); 
                        appendCp(processArgumentsStr, '\\');
                        appendCp(processArgumentsStr, *c);                        
                    }else
                        appendCp(processArgumentsStr, *c);
                    if (*c == '\\')
                        ++backslashCount;
                    else
                        backslashCount = 0;
                }

                if (containsWhitespace)
                {   for (size_t i=0; i<backslashCount; ++i)
                        appendCp(processArgumentsStr, '\\'); 
                    appendCp(processArgumentsStr, '\"');
                }
            }
            processArguments.reset(new wchar_t[processArgumentsStr.size() + 1]);
            memcpy(processArguments.get(), processArgumentsStr.data(), processArgumentsStr.size() * 2);
            * (processArguments.get() + processArgumentsStr.size()) = 0;
        }

        void createProcessArgumentsString(
                    jjm::UniqueArray<wchar_t*> & processArguments, 
                    ProcessBuilder::WindowsArgumentQuotingConvention argumentQuoting,
                    vector<Utf8String> cmd,
                    Utf8String const& newCmd0 //empty means unset
                    )
        {
            if (newCmd0.size())
                cmd[0] = newCmd0; 
            if (ProcessBuilder::msvc_c_main_convention == argumentQuoting)
                makeProperlyQuotedWindowsCMainArgumentString(processArguments, cmd);
            else
                JFATAL(argumentQuoting, 0);
        }

        void createPipes(
                ProcessBuilder const& pb,
                FileHandleOwner & pipeInReadEnd, FileHandleOwner & pipeInWriteEnd, 
                FileHandleOwner & pipeOutReadEnd, FileHandleOwner & pipeOutWriteEnd, 
                FileHandleOwner & pipeErrReadEnd, FileHandleOwner & pipeErrWriteEnd)
        {
            if (pb.getPipeIn())
            {   Pipe x = Pipe::create();
                pipeInReadEnd.reset(x.readable); 
                pipeInWriteEnd.reset(x.writeable); 
            }else
                pipeInReadEnd.reset(FileOpener().createOrOpen().readOnly().open(Path::win32device("NUL")));
            if (pb.getPipeOut())
            {   Pipe x = Pipe::create();
                pipeOutReadEnd.reset(x.readable); 
                pipeOutWriteEnd.reset(x.writeable); 
            }else
                pipeOutWriteEnd.reset(FileOpener().createOrOpen().writeOnly().open(Path::win32device("NUL")));
            if (pb.getPipeErr())
            {   Pipe x = Pipe::create();
                pipeErrReadEnd.reset(x.readable); 
                pipeErrWriteEnd.reset(x.writeable); 
            }else if (pb.getErrToOut() && pb.getPipeOut())
            {   HANDLE outDup;
                if ( ! DuplicateHandle(
                        GetCurrentProcess(), 
                        pipeOutWriteEnd.get().native(), 
                        GetCurrentProcess(), 
                        &outDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
                {   JFATAL(0, 0);
                }
                pipeErrWriteEnd.reset(FileHandle(outDup));
            }else
                pipeErrWriteEnd.reset(FileOpener().createOrOpen().writeOnly().open(Path::win32device("NUL")));
        }

        //requires windows vista or better
        void createAttributeList(
                        jjm::UniqueArray<HANDLE*> & handlesList, 
                        jjm::UniqueArray<unsigned char*> & memoryBackingAttributeList, 
                        PPROC_THREAD_ATTRIBUTE_LIST & attribList,
                        vector<jjm::FileHandle> const& inheritableHandles)
        {   
            if (inheritableHandles.size() == 0)
                JFATAL(0, 0);
            handlesList.reset(new HANDLE[inheritableHandles.size()]);
            for (size_t i=0; i<inheritableHandles.size(); ++i)
                handlesList[i] = inheritableHandles[i].native();

            SIZE_T requiredSize = 0;
            InitializeProcThreadAttributeList(0, 1, 0, & requiredSize);
            memoryBackingAttributeList.reset(new unsigned char[requiredSize]);
            attribList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(memoryBackingAttributeList.get());
            if ( ! InitializeProcThreadAttributeList(attribList, 1, 0, & requiredSize))
                JFATAL(0, 0);

            if ( ! UpdateProcThreadAttribute(attribList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, 
                            handlesList.get(), sizeof(HANDLE) * inheritableHandles.size(),
                            0, 0))
            {   JFATAL(0, 0);
            }
        }
    }

    jjm::Process* jjm::ProcessBuilder::spawn() const
    {   
        if (m_cmd.size() == 0)
            throw runtime_error("ProcessBuilder : Spawn failed. cmd empty.");
        if (m_cmd[0].size() == 0)
            throw runtime_error("ProcessBuilder : Spawn failed. cmd[0] empty.");

        Win32CreateProcessEnvWrapper envWrapper(m_hasCustomEnv, m_env);

        //cmd[0] is the name / path to the executable. 
        //If it contains a file separator character, then it should be 
        //interpreted as a path, which is precisely what giving to 
        //CreateProcessW as the lpApplicationName arg will do. 
        Utf16String applicationNameArg; 
        Utf8String newCmd0; 
        Path cmd0Path(m_cmd[0]); 
        if ( ! cmd0Path.getParent().isEmpty())
        {   applicationNameArg = makeU16Str(cmd0Path.getStringRep()); 
            newCmd0 = makeU8Str(applicationNameArg); 
        }
        wchar_t const * const applicationNameArgPointer = 
                applicationNameArg.size() ? applicationNameArg.c_str() : 0; 

        //Need to properly quote the command line for CreateProcess to parse
        //it because it's retarded and doesn't have argv, just a single
        //command arg string.
        jjm::UniqueArray<wchar_t*>  processArguments;
        createProcessArgumentsString(processArguments, m_argumentQuoting, m_cmd, newCmd0);

        FileHandleOwner pipeInReadEnd, pipeInWriteEnd;
        FileHandleOwner pipeOutReadEnd, pipeOutWriteEnd;
        FileHandleOwner pipeErrReadEnd, pipeErrWriteEnd;
        createPipes(*this, pipeInReadEnd, pipeInWriteEnd, pipeOutReadEnd, pipeOutWriteEnd, pipeErrReadEnd, pipeErrWriteEnd);

        vector<FileHandle> inheritableHandles;
        inheritableHandles.push_back(pipeInReadEnd.get());
        inheritableHandles.push_back(pipeOutWriteEnd.get());
        inheritableHandles.push_back(pipeErrWriteEnd.get());


        STARTUPINFOEXW startUpInfoEx;
        ZeroMemory(&startUpInfoEx, sizeof(STARTUPINFOEXW));
        startUpInfoEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
        STARTUPINFOW* startUpInfoPtr = & startUpInfoEx.StartupInfo;
        DWORD const creationFlags = CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT;
        UniqueArray<HANDLE*> handlesList(0);
        UniqueArray<unsigned char*> memoryBackingAttributeList(0);
        struct FreeAttribList
        {   FreeAttribList() : x(0) {}
            PPROC_THREAD_ATTRIBUTE_LIST x;
            ~FreeAttribList() { if (x) DeleteProcThreadAttributeList(x); }
        } attribList;
        createAttributeList(handlesList, memoryBackingAttributeList, attribList.x, inheritableHandles);
        startUpInfoEx.lpAttributeList = attribList.x;

        startUpInfoPtr->dwFlags = STARTF_USESTDHANDLES;
        startUpInfoPtr->hStdInput  = pipeInReadEnd.get().native();
        startUpInfoPtr->hStdOutput = pipeOutWriteEnd.get().native();
        startUpInfoPtr->hStdError  = pipeErrWriteEnd.get().native();


        PROCESS_INFORMATION processInformation;
        ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));

        Utf16String const u16dir = makeU16Str(m_dir.getStringRep());

        /* Make the handles that will become the child's stdin, stdout, and 
        stderr inheritable. When we do this, it's possible another thread can 
        call CreateProcess and inadvertently inherit these handles if they're
        not using the startUpInfoEx.lpAttributeList option to control which
        handles are inherited. We want to make this window of time where this
        is possible as short as possible. */
        for (size_t i=0; i<inheritableHandles.size(); ++i)
        {   if ( ! SetHandleInformation(inheritableHandles[i].native(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
                JFATAL(0, 0);
        }

        //allocate the process, and assign it an owner
        SetLastError(0); 
        BOOL const createProcessResult = ::CreateProcessW(
                    applicationNameArgPointer, 
                    processArguments.get(),
                    0, //security attributes, use default
                    0, //thread attributes, use default
                    TRUE,
                    creationFlags,
                    m_hasCustomEnv ? envWrapper.envarray.get() : 0, //0 is default env
                    u16dir.c_str(), //working dir for new process
                    startUpInfoPtr,
                    & processInformation
                    );
        DWORD const lastError = GetLastError();

        /* Close the inheritable handles that are child's stdin, stdout, and 
        stderr. The handles are inheritable, and it's possible another thread
        can call CreateProcess and inadvertently inherit these handles if
        they're not using the startUpInfoEx.lpAttributeList option to control 
        which handles are inherited. We want to make this window of time where 
        this is possible as short as possible. */
        pipeInReadEnd.reset(); 
        pipeOutWriteEnd.reset();
        pipeErrWriteEnd.reset();

        if ( ! createProcessResult)
        {   stringstream ss;
            ss << "ProcessBuilder : CreateProcessW(\n";
            if (applicationNameArgPointer == 0)
                ss << "NULL";
            else
                ss << "\"" << makeU8Str(applicationNameArg) << "\",\n";
            ss << "\"" << makeU8Str(Utf16String(processArguments.get())) << "\",\n"; 
            ss << "...) failed. Cause:\n";

            if (lastError == ERROR_FILE_NOT_FOUND)
                ss << "GetLastError() ERROR_FILE_NOT_FOUND, The system cannot find the file specified.";
            else if (lastError == ERROR_DIRECTORY)
                ss << "GetLastError() ERROR_DIRECTORY, directory is not valid [" << m_dir.getStringRep() << "].";
            else if (lastError == ERROR_ACCESS_DENIED)
                ss << "GetLastError() ERROR_ACCESS_DENIED, access is denied.";
            else if (lastError == ERROR_INVALID_PARAMETER)
                ss << "GetLastError() ERROR_INVALID_PARAMETER, The parameter is incorrect.";
            else
                ss << "GetLastError() " << lastError << "."; 
            throw std::runtime_error(ss.str());
        }

        //Gotta close this handle on a successful CreateProcess call, 
        //or we have a leak; quirk of the CreateProcess API.
        if ( ! CloseHandle(processInformation.hThread)) 
            JFATAL(0, 0);

        auto_ptr<Process> process(new Process);
        process->processHandle = processInformation.hProcess;
        process->processId = processInformation.dwProcessId;
        if (m_pipeIn)
            process->writeEndToChildsStdin.reset(pipeInWriteEnd.release());
        if (m_pipeOut)
            process->readEndFromChildsStdout.reset(pipeOutReadEnd.release());
        if (m_pipeErr)
            process->readEndFromChildsStderr.reset(pipeErrReadEnd.release());
        return process.release();
    }

#else
    void jjm::Process::join()
    {   for (;;)
        {   int x = waitpid(pid, & childStatus, 0);
            if (-1 != x)
                return;
            if (EINTR == errno)
                continue;
            JFATAL(0, 0);
        }
    }

    //TODO make this right, fix up the Process API to distinguish between 
    //terminated from _exit(), and terminated from signal
    int jjm::Process::getExitCode()
    {   if (WIFEXITED(childStatus))
            return WEXITSTATUS(childStatus);
        if (WIFSIGNALED(childStatus))
            return 260;
        if (WIFSTOPPED(childStatus))
            return 261;
    #ifdef WIFCONTINUED     /* Not all implementations support this */
        if (WIFCONTINUED(childStatus))
            return 262;
    #endif
        JFATAL(childStatus, 0);
    }

    jjm::Process::Process() : pid(-1) {}

    jjm::Process::~Process() {}

    namespace
    {
        inline int moveFileDescToNOrHigher(int & fd, int const n)
        {   
            errno = 0; 
            if (fd < n)
            {   int const new_fd = fcntl(fd, F_DUPFD, n);
                if (new_fd == -1)
                    return -1;
                fd = new_fd; 
            }
            return 0;
        }

        inline int moveFileDesc(int & source, int target)
        {   
            errno = 0; 
            int new_fd = fcntl(source, F_DUPFD, target);
            if (new_fd == -1)
                return -1;
            source = new_fd;
            return 0;
        }

        inline int clearCloseOnExec(int const fd)
        {   
            errno = 0; 
            int flags = fcntl(fd, F_GETFD);
            if (flags == -1)
                return -1;
            flags &= (~ FD_CLOEXEC);
            errno = 0; 
            if (fcntl(fd, F_SETFD, flags) == -1)
                return -1;
            return 0;
        }
        
        inline void asyncSignalSafeWriteCstr(int fd, char const * const str)
        {
            ssize_t i = 0; 
            for (char const * str2 = str; *str2; ++str2)
                ++i;

            for (char const * str2 = str; i; )
            {   ssize_t x = FileHandle(fd).write2(str2, i); 
                if (x < 0)
                    return; 
                str2 += x;
                i -= x; 
            }
        }
    
        inline void asyncSignalSafeWriteInt(int fd, int toPrint)
        {
            if (toPrint < 0)
            {   toPrint = - toPrint; 
                if (toPrint < 0)
                {   asyncSignalSafeWriteCstr(fd, "INT_MIN");
                    return; 
                }
                asyncSignalSafeWriteCstr(fd, "-");
            }
            if (toPrint == 0)
            {   asyncSignalSafeWriteCstr(fd, "0");
                return; 
            }

            int log10 = 0; 
            for (int x = toPrint; x; x /= 10)
                ++log10; 
            for (--log10; log10 >= 0; --log10)
            {   
                int nextDigit = toPrint; 
                for (int log10b = log10; log10b; --log10b)
                    nextDigit /= 10; 
                switch (nextDigit)
                {
                case 0: asyncSignalSafeWriteCstr(fd, "0"); break; 
                case 1: asyncSignalSafeWriteCstr(fd, "1"); break; 
                case 2: asyncSignalSafeWriteCstr(fd, "2"); break; 
                case 3: asyncSignalSafeWriteCstr(fd, "3"); break; 
                case 4: asyncSignalSafeWriteCstr(fd, "4"); break; 
                case 5: asyncSignalSafeWriteCstr(fd, "5"); break; 
                case 6: asyncSignalSafeWriteCstr(fd, "6"); break; 
                case 7: asyncSignalSafeWriteCstr(fd, "7"); break; 
                case 8: asyncSignalSafeWriteCstr(fd, "8"); break; 
                case 9: asyncSignalSafeWriteCstr(fd, "9"); break; 
                default: asyncSignalSafeWriteCstr(fd, "?"); return; 
                }
                for (int log10b = log10; log10b; --log10b)
                    nextDigit *= 10; 
                toPrint -= nextDigit; 
            }
        }

        inline void jforkFatal(char const * const file, int const line, int fd)
        {
            int const lastErrno = errno; 
            asyncSignalSafeWriteCstr(fd, "jjm::ProcessBuilder::spawn() failed in child process before exec(). Cause:\n");
            asyncSignalSafeWriteCstr(fd, "File \"");
            asyncSignalSafeWriteCstr(fd, file); 
            asyncSignalSafeWriteCstr(fd, "\", line ");
            asyncSignalSafeWriteInt(fd, line); 
            asyncSignalSafeWriteCstr(fd, ", errno "); 
            asyncSignalSafeWriteInt(fd, lastErrno); 
            asyncSignalSafeWriteCstr(fd, "."); 
            _exit(1); 
        }

        //This closes all open file descriptors greater than or equal to lowfd. 
        //lowfd must be 0 or bigger. 
        void myCloseFrom(int const lowfd, int const errorChannel)
        {
    #if (defined(__gnu_linux__) || defined (__CYGWIN__))
            //opendir may use a file descriptor.
            //We hope it uses the lowest available fd, which will be lowfd. 
            //We will not close lowfd in the loop, and only close it after
            //the opendir handle has been closed. 
            for (;;)
            {   int const toCloseBufferSize = 1024; 
                int toCloseBuffer[toCloseBufferSize];
                int* toCloseEnd = toCloseBuffer;
                errno = 0; 
                if (::close(lowfd) && errno != EBADF) 
                    jforkFatal(__FILE__, __LINE__, errorChannel); 
                errno = 0; 
                DIR* const opendirHandle = ::opendir("/proc/self/fd");
                if (0 == opendirHandle) 
                    jforkFatal(__FILE__, __LINE__, errorChannel); 
                for (;;)
                {   errno = 0;
                    dirent* const readdirHandle = ::readdir(opendirHandle);
                    if (0 == readdirHandle)
                    {   int const readdirErrno = readdirErrno; 
                        if (0 == readdirErrno)
                        {   errno = 0; 
                            if (-1 == ::closedir(opendirHandle)) 
                                jforkFatal(__FILE__, __LINE__, errorChannel); 
                            errno = 0; 
                            if (::close(lowfd) && readdirErrno != EBADF) 
                                jforkFatal(__FILE__, __LINE__, errorChannel); 
                            for (int* x = toCloseBuffer; x != toCloseEnd; ++x)
                            {   errno = 0; 
                                if (::close(*x)) 
                                    jforkFatal(__FILE__, __LINE__, errorChannel); 
                            }
                            return;
                        }
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                    }
                    if (0 == strcmp(".", readdirHandle->d_name)) 
                        continue;
                    if (0 == strcmp("..", readdirHandle->d_name)) 
                        continue;
                    int const entryAsInt = weakAtoi(readdirHandle->d_name); 
                    if (entryAsInt == -1) 
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                    if (entryAsInt <= lowfd) 
                        continue;
                    *(toCloseEnd++) = entryAsInt;
                    if (toCloseEnd == toCloseBuffer + toCloseBufferSize)
                    {   errno = 0;
                        if (-1 == ::closedir(opendirHandle)) 
                            jforkFatal(__FILE__, __LINE__, errorChannel); 
                        break;
                    }
                }
                for (int* x = toCloseBuffer; x != toCloseEnd; ++x)
                {   errno = 0; 
                    if (::close(*x)) 
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }
            }
    #else
        #error Not yet implemented. 
    #endif
        }
    }

    jjm::Process* jjm::ProcessBuilder::spawn() const
    {   
        if (m_cmd.size() == 0)
            throw runtime_error("ProcessBuilder : Spawn failed. cmd empty.");

        auto_ptr<Process> p(new Process);
        
        struct ArgvOwner
        {   ArgvOwner(size_t size_) : size(size_)
            {   argv = new char* [size];
                memset(argv, 0, size);
            }
            ~ArgvOwner()
            {   for (size_t i=0; i<size; ++i)
                    delete[] argv[i];
                delete[] argv;
            }
            char** argv;
            size_t const size;
        };
        ArgvOwner argv(m_cmd.size() + 1);
        for (size_t i=0; i<m_cmd.size(); ++i)
        {   argv.argv[i] = new char[m_cmd[i].size() + 1];
            strcpy(argv.argv[i], m_cmd[i].c_str());
        }
        argv.argv[m_cmd.size()] = 0;

        char const * const dir_c_str = m_dir.getStringRep().c_str();
        char const * const cmd0_c_str = m_cmd[0].c_str();

        PosixEnvironWrapper envWrapper;
        if (m_hasCustomEnv)
            envWrapper.init(m_env);

        FileHandleOwner inReadEnd, inWriteEnd;
        if (m_pipeIn)
        {   Pipe x = Pipe::create();
            inReadEnd.reset(x.readable);
            inWriteEnd.reset(x.writeable);
        }
        FileHandleOwner outReadEnd, outWriteEnd;
        if (m_pipeOut)
        {   Pipe x = Pipe::create();
            outReadEnd.reset(x.readable);
            outWriteEnd.reset(x.writeable);
        }
        FileHandleOwner errReadEnd, errWriteEnd;
        if (m_pipeErr)
        {   Pipe x = Pipe::create();
            errReadEnd.reset(x.readable);
            errWriteEnd.reset(x.writeable);
        }
        
        //Ok, we're going to open up one more pipe, to communicate possible errors from the child to the parent, 
        //such as executable file not found, etc., 
        FileHandleOwner childExecErrorsReadEnd, childExecErrorsWriteEnd;
        {
            Pipe x = Pipe::create();
            childExecErrorsReadEnd.reset(x.readable);
            childExecErrorsWriteEnd.reset(x.writeable);
        }

        switch (p->pid = fork())
        {
        case -1: JFATAL(0, 0); //error
        case 0: //child
            {
                int childsIn = inReadEnd.get().native();
                int childsOut = outWriteEnd.get().native();
                int childsErr = errWriteEnd.get().native();
                int errorChannel = childExecErrorsWriteEnd.get().native();

                //move any open pipes we care about to file descriptors 5 or higher
                if (m_pipeIn  && -1 == moveFileDescToNOrHigher(childsIn, 5)) jforkFatal(__FILE__, __LINE__, errorChannel); 
                if (m_pipeOut && -1 == moveFileDescToNOrHigher(childsOut, 5)) jforkFatal(__FILE__, __LINE__, errorChannel); 
                if (m_pipeErr && -1 == moveFileDescToNOrHigher(childsErr, 5)) jforkFatal(__FILE__, __LINE__, errorChannel); 
                if (-1 == moveFileDescToNOrHigher(errorChannel, 5)) jforkFatal(__FILE__, __LINE__, errorChannel); 
                
                //Now that the handles we care about are 5 or higher, close 0 
                //through 4, as we'll need that space for later. 
                errno = 0; if (::close(0) && errno != EBADF) jforkFatal(__FILE__, __LINE__, errorChannel); 
                errno = 0; if (::close(1) && errno != EBADF) jforkFatal(__FILE__, __LINE__, errorChannel); 
                errno = 0; if (::close(2) && errno != EBADF) jforkFatal(__FILE__, __LINE__, errorChannel); 
                errno = 0; if (::close(3) && errno != EBADF) jforkFatal(__FILE__, __LINE__, errorChannel); 
                errno = 0; if (::close(4) && errno != EBADF) jforkFatal(__FILE__, __LINE__, errorChannel); 

                /* Move our handles into position for the exec (and the error
                channel to 3 in preparation for myCloseFrom). */
                if (m_pipeIn)
                {   if (-1 == moveFileDesc(childsIn, 0))
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                    if (-1 == clearCloseOnExec(0))
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }else
                {   errno = 0; 
                    int x = ::open("/dev/null", O_RDONLY); //open it to nowhere
                    if (x != 0)
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }
                if (m_pipeOut)
                {   if (-1 == moveFileDesc(childsOut, 1))
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                    if (-1 == clearCloseOnExec(1))
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }else
                {   errno = 0; 
                    int x = ::open("/dev/null", O_WRONLY); //open it to nowhere
                    if (x != 1)
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }
                if (m_pipeErr)
                {   if (-1 == moveFileDesc(childsErr, 2))
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                    if (-1 == clearCloseOnExec(2))
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }else if (m_errToOut && m_pipeOut)
                {   errno = 0; 
                    int x = ::fcntl(1, F_DUPFD, 2); //make err be a dup of out
                    if (x != 2)
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }else
                {   errno = 0; 
                    int x = ::open("/dev/null", O_WRONLY); //open it to nowhere
                    if (x != 2)
                        jforkFatal(__FILE__, __LINE__, errorChannel); 
                }
                if (-1 == moveFileDesc(errorChannel, 3))
                    jforkFatal(__FILE__, __LINE__, errorChannel); 

                myCloseFrom(4, errorChannel);

                //change current dir
                if (chdir(dir_c_str))
                {   int const lastErrno = errno;
                    asyncSignalSafeWriteCstr(errorChannel, "chdir(\"");
                    asyncSignalSafeWriteCstr(errorChannel, dir_c_str);
                    asyncSignalSafeWriteCstr(errorChannel, "\") failed. CAuse:\n");
                    asyncSignalSafeWriteCstr(errorChannel, "Errno ");
                    asyncSignalSafeWriteInt(errorChannel, lastErrno );
                    asyncSignalSafeWriteCstr(errorChannel, ".");
                    asyncSignalSafeWriteCstr(errorChannel, ".");
                    _exit(1);
                }
        
                //and exec
                if (m_hasCustomEnv)
                {   if (-1 == execve(cmd0_c_str, argv.argv, envWrapper.env))
                    {   int const lastErrno = errno;
                        asyncSignalSafeWriteCstr(errorChannel, "execve(...) failed. Cause:\n");
                        asyncSignalSafeWriteCstr(errorChannel, "Errno ");
                        asyncSignalSafeWriteInt(errorChannel, lastErrno);
                        asyncSignalSafeWriteCstr(errorChannel, ".");
                        _exit(1);
                    }
                }else
                {   if (-1 == execvp(cmd0_c_str, argv.argv))
                    {   int const lastErrno = errno;
                        asyncSignalSafeWriteCstr(errorChannel, "execvp(...) failed. Cause:\n");
                        asyncSignalSafeWriteCstr(errorChannel, "Errno ");
                        asyncSignalSafeWriteInt(errorChannel, lastErrno);
                        asyncSignalSafeWriteCstr(errorChannel, ".");
                        _exit(1);
                    }
                }
                jforkFatal(__FILE__, __LINE__, errorChannel); 
            }
            _exit(1); 
        default: //parent
            {
                //close the ends of the pipes which we don't care about, 
                inReadEnd.reset();
                outWriteEnd.reset();
                errWriteEnd.reset();
                childExecErrorsWriteEnd.reset();

                //assign the ends that we might care about to the Process object
                p->writeEndToChildsStdin.reset(inWriteEnd.release());
                p->readEndFromChildsStdout.reset(outReadEnd.release());
                p->readEndFromChildsStderr.reset(errReadEnd.release());

                string asciiErrorString; 
                for (;;)
                {   char buf[100]; 
                    ssize_t x = childExecErrorsReadEnd.get().read2(buf, 100); 
                    int const lastErrno = errno; 
                    if (x == -1)
                        break;
                    if (x < 0)
                    {   throw std::runtime_error(string() 
                                + "jjm::ProcessBuilder::spawn() failed. Cause:\n"
                                + "An internal pipe was created to receive errors from the child process between fork() and exec(). "
                                + "That pipe unexpectedly closed. Errno " + toDecStr(lastErrno) + "."); 
                    }
                    asciiErrorString.append(buf + 0, buf + x);
                }
                if (asciiErrorString.size() > 0)
                {   throw std::runtime_error(string()
                            + "jjm::ProcessBuilder::spawn() failed. Cause:\n"
                            + "The child process failed between the fork() and exec(). Cause:\n"
                            + asciiErrorString); 
                }
            }
            break;
        }
        return p.release();
    }
#endif
    

jjm::FileHandle jjm::Process::releaseWriteEndToChildsStdin()
{   
    return writeEndToChildsStdin.release();
}

jjm::FileHandle jjm::Process::releaseReadEndFromChildsStdout()
{   
    return readEndFromChildsStdout.release();
}

jjm::FileHandle jjm::Process::releaseReadEndFromChildsStderr()
{   
    return readEndFromChildsStderr.release();
}


namespace
{
    bool writeToHandle(char const* const data, size_t const dataLen, jjm::FileHandle fd)
    {   char const* const end = data + dataLen;
        for (char const * p = data; p != end; )
        {   ssize_t const n = fd.write2(p, end - p);
            if (n < 0)
                return true;
            p += n;
        }
        return false;
    }

    class ReadFromHandle
    {
    public:
        ReadFromHandle(std::string& str_, jjm::FileHandle fd_, bool & failFlag_)
                : str(str_), fd(fd_), failFlag(failFlag_) {}
        string& str;
        jjm::FileHandle fd;
        bool & failFlag;
        void operator() ()
        {   size_t const fetchSize = 512; //magic number
            for (;;)
            {   size_t const oldSize = str.size();
                str.resize(str.size() + fetchSize);
                
                //Assumes std::string uses contiguous storage. 
                //Not gauranteed by C++03. It is guaranteed by C++11. 
                //True of all commercial implementations. 
                ssize_t const n = fd.read2( & str[0] + oldSize, fetchSize);
                if (n == -1)
                {   str.resize(oldSize);
                    break;
                }
                if (n < 0)
                {   str.resize(oldSize);
                    failFlag = true;
                    break;
                }
                str.resize(oldSize + n);
            }
        }
    };
}

namespace
{
    void syncexec(
                jjm::SyncExec& jSyncExec, 
                jjm::ProcessBuilder const& pb, 
                char const* const stdinData, 
                size_t const stdinDataLen)
    {   
        int numThreadsNeeded = 0;
        if (pb.getPipeIn() && stdinDataLen) ++numThreadsNeeded;
        if (pb.getPipeOut()) ++numThreadsNeeded;
        if (pb.getPipeErr()) ++numThreadsNeeded;

        UniquePtr<Process*> process(pb.spawn());

        FileHandleOwner childsStdIn( pb.getPipeIn()  ? process->releaseWriteEndToChildsStdin()   : FileHandle());
        FileHandleOwner childsStdOut(pb.getPipeOut() ? process->releaseReadEndFromChildsStdout() : FileHandle());
        FileHandleOwner childsStdErr(pb.getPipeErr() ? process->releaseReadEndFromChildsStderr() : FileHandle());

        try
        {   bool stdInFailFlag = false;
            bool stdOutFailFlag = false;
            bool stdErrFailFlag = false;
            {
                UniquePtr<Thread*> stdOutThread;
                UniquePtr<Thread*> stdErrThread;

                if (pb.getPipeIn() && 0 == stdinDataLen)
                    childsStdIn.reset(); //nothing to send, 

                if (pb.getPipeOut())
                {   ReadFromHandle callable(jSyncExec.out, childsStdOut.get(), stdOutFailFlag);
                    if (numThreadsNeeded == 1)
                        callable();//do processing in-thread
                    else
                    {   //do processing in a separate thread
                        stdOutThread.reset(new Thread(callable, Thread::JoinInDtor));
                    }
                    --numThreadsNeeded;
                }
                if (pb.getPipeErr())
                {   ReadFromHandle callable(jSyncExec.err, childsStdErr.get(), stdErrFailFlag);
                    if (numThreadsNeeded == 1)
                        callable();//do processing in-thread
                    else
                    {   //do processing in a separate thread
                        stdErrThread.reset(new Thread(callable, Thread::JoinInDtor));
                    }
                    --numThreadsNeeded;
                }

                if (pb.getPipeIn() && stdinDataLen)
                {   //do processing in-thread
                    if (writeToHandle(stdinData, stdinDataLen, childsStdIn.get()))
                        stdInFailFlag = true;
                    childsStdIn.reset();
                }
            }
            if (stdInFailFlag)
                throw std::runtime_error("jjm::SyncExec failed. Cause:\nFailure when writing to child's stdin.");
            if (stdOutFailFlag)
                throw std::runtime_error("jjm::SyncExec failed. Cause:\nFailure when reading from child's stdout.");
            if (stdErrFailFlag)
                throw std::runtime_error("jjm::SyncExec failed. Cause:\nFailure when reading from child's stderr.");
        } catch (...)
        {   childsStdIn.reset();
            childsStdOut.reset();
            childsStdErr.reset();
            process->join();
            throw;
        }
        childsStdIn.reset();
        childsStdOut.reset();
        childsStdErr.reset();
        process->join();
        jSyncExec.exitcode = process->getExitCode();
    }
}

jjm::SyncExec::SyncExec(ProcessBuilder const& pb, string const& stdinData) 
{ 
    syncexec(*this, pb, stdinData.data(), stdinData.size()); 
}
