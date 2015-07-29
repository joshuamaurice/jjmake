// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JPROCESS_HPP_HEADER_GUARD
#define JOSUTILS_JPROCESS_HPP_HEADER_GUARD

#include "jfilehandle.hpp"
#include "jpath.hpp" 
#include "junicode/jutfstring.hpp" 

#include <map>
#include <vector>

#ifdef _WIN32
    typedef void* HANDLE;
    typedef unsigned long DWORD;
#endif


namespace jjm
{

class Process;
class ProcessBuilder;


//returns getpid() for unix-like, GetCurrentProcessId() for win32
unsigned long long getPid();


class ProcessBuilder
{
public:
    
    ProcessBuilder() //defaults
        : m_dir(Path(".")),
            m_hasCustomEnv(false), 
            m_pipeIn(false), m_pipeOut(false), m_pipeErr(false),
            m_errToOut(false)
#ifdef _WIN32
            , m_argumentQuoting(msvc_c_main_convention)
#endif
        {}

    //default copy ctor, operator=, and dtor suffice.

    //TODO document and/or fix how executables are found and whether dir() 
    //affects the lookup process. 

    ProcessBuilder&  cmd(std::vector<Utf8String> const& cmd_)  { m_cmd = cmd_; return *this; }
    ProcessBuilder&  arg(Utf8String const& arg)  { m_cmd.push_back(arg); return *this; }
    std::vector<Utf8String> const&  getCmd() const  { return m_cmd; }
    
    ProcessBuilder&  dir(Path const& dir_)  { m_dir = dir_; return *this; }
    Path const&  getDir() const { return m_dir; }

    ProcessBuilder&  env(std::map<Utf8String, Utf8String> const& env_)  { m_hasCustomEnv = true; m_env = env_; return *this; }
    std::pair<bool, std::map<Utf8String, Utf8String> const* >  getEnv() const  { return std::make_pair(m_hasCustomEnv, &m_env); }

    ProcessBuilder&  pipeIn(bool x = true)  { m_pipeIn = x; return *this; }
    ProcessBuilder&  pipeOut(bool x = true)  { m_pipeOut = x; return *this; }
    ProcessBuilder&  pipeErr(bool x = true)  { m_pipeErr = x; if (x) { m_errToOut = false; } return *this; }
    bool  getPipeIn() const  { return m_pipeIn; }
    bool  getPipeOut() const  { return m_pipeOut; }
    bool  getPipeErr() const  { return m_pipeErr; }

    ProcessBuilder&  errToOut(bool x = true)  { m_errToOut = x; if (x) { m_pipeErr = false; } return *this; }
    bool  getErrToOut() const  { return m_errToOut; }

#ifdef _WIN32
    /* Windows fails at life because its only process creation primitive, 
    CreateProcess, takes a single string for all of the arguments, leaving it
    up to each process to parse the argument string into separate arguments. 
    There are different conventions on windows on how to do it. For windows 
    systems, this option tells ProcessBuilder which convention to use to 
    generate the single argument string. 

    This option has no effect on non-windows systems. 

    msvc_c_main_convention : The default. The convention used by the code 
    implicitly provided by msvc to parse the argument string into the argc and 
    argv of "int main(int argc, char** argv);" for compiled C programs. 

    TODO convention for cmd.exe
    */
    enum WindowsArgumentQuotingConvention { msvc_c_main_convention };
    ProcessBuilder&  argumentQuoting(WindowsArgumentQuotingConvention argumentQuoting_)  { m_argumentQuoting = argumentQuoting_; return *this; }
    WindowsArgumentQuotingConvention  getArgumentQuoting() const  { return m_argumentQuoting; }
#endif

    /* Caller owns returned object and must delete it. 
    Throws std::exception on errors. 
    If any of the child's stdin, stdout, stderr are not explicitly piped or 
    redirected (ex: errToOut()), then spawn() will implicitly redirect the 
    handle to /dev/null for POSIX systems, and \\.\NUL for win32. */
    Process* spawn() const;

private:
    std::vector<Utf8String> m_cmd;
    Path m_dir;
    bool m_hasCustomEnv; 
    std::map<Utf8String, Utf8String> m_env;
    bool m_pipeIn;
    bool m_pipeOut;
    bool m_pipeErr;
    bool m_errToOut;
#ifdef _WIN32
    WindowsArgumentQuotingConvention m_argumentQuoting;
#endif
};




/* The SyncExec constructor will synchronously execute the specified process 
and capture its stdout and stderr (if piped).
if false == getPipeIn(), then stdinData is ignored. */
class SyncExec  //TODO handle encoding
{
public:

    SyncExec() {}

    //stdinData will be converted from UTF-8 and "\n" newline convention to 
    //the current locale's encoding and the platform's convention for newlines. 
    //TODO
    explicit SyncExec(ProcessBuilder const& pb, Utf8String const& stdinData = Utf8String()); 

    int exitcode;

    //this->out and this->err will be converted from the current locale's 
    //encoding and from the platform's newline convention to UTF-8 and 
    //"\n" newline convention. 
    //TODO
    Utf8String out; 
    Utf8String err; 
};




/* As usual, the child process may block if the input buffer associated with 
its stdin is full. 

As usual, the child process may block if the output buffers associated with 
its output streams (stdout and stderr) are full. 

Obviously, if you piped stdout and/or stderr, then you must read from both
until EOF, otherwise join() may block indefinitely. Also obviously, if the 
child's stdin is open, then join() may block indefinitely. (Close the handle
for the child's stdin after writing all of the data and flushing.) 

Obviously, if you have piped stdin, stdout, and/or stderr, then each must be 
processed in different threads, or processing must use select(), poll(), 
epoll(), or similar. Otherwise, there is a deadlock potential. */
class Process
{
public:
    //TODO background thread for posix systems to handle reaping children,
    //created by JProcessBuilder::spawn(), and not joined with 
    //JProcess::join(), and lost via JProcess::~JProcess(). 
    ~Process();

    //Caller owns the returned handles. 
    //Callers using the returned handles should be aware they are receiving 
    //the bytes directly, and it is up to the user to deal with newline
    //conventions and encoding conventions. 
    FileHandle releaseWriteEndToChildsStdin();
    FileHandle releaseReadEndFromChildsStdout();
    FileHandle releaseReadEndFromChildsStderr();

    //TODO kill

    void join();

    //May call getExitCode() only after join()
    int getExitCode();
    
private:
    Process();
    Process(Process const& ); //not defined, not copyable
    Process& operator= (Process ); //not defined, not copyable

    friend class ProcessBuilder;
    
    #ifdef _WIN32
        HANDLE processHandle;
        DWORD processId;
        DWORD exitCode;
    #else
        pid_t pid;
        int childStatus;
    #endif

    FileHandleOwner writeEndToChildsStdin;
    FileHandleOwner readEndFromChildsStdout;
    FileHandleOwner readEndFromChildsStderr;
};


} //namespace jjm

#endif
