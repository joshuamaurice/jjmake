// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "jbase/jinttostring.hpp"
#include "jbase/jnulltermiter.hpp"
#include "jbase/juniqueptr.hpp"
#include "jbase/jnulltermiter.hpp"
#include "junicode/jiconv.hpp"
#include "junicode/jutfstring.hpp"
#include "josutils/jenv.hpp"
#include "josutils/jstdstreams.hpp"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <typeinfo>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif


using namespace jjm;
using namespace std;


namespace 
{
    template <size_t N>
    void writeStringLiteral(FileHandle h, char const (&str)[N]) { h.writeComplete2(str, N-1); }
    void fatalHandler(char const * const filename, int linenum, int info_n, char const * info_cstr)
    {
        FileHandle x = FileHandle::getstderr();
        writeStringLiteral(x, "JFATAL(), file \""); 
        x.writeComplete2(filename, strlen(filename)); 
        writeStringLiteral(x, "\", line "); 

        string linenumString = toDecStr(linenum); //TODO change to version that does not allocate memory
        x.writeComplete2(linenumString.data(), linenumString.size()); 

        writeStringLiteral(x, ", info [");

        string info_n_string = toDecStr(info_n); //TODO change to version that does not allocate memory
        x.writeComplete2(info_n_string.data(), info_n_string.size()); 

        writeStringLiteral(x, "]");

        if (info_cstr)
        {   writeStringLiteral(x, ", \"");
            x.writeComplete2(info_cstr, strlen(info_cstr));
            writeStringLiteral(x, "\"");
        }

        writeStringLiteral(x, ".\n");
    }
    bool installFatalHandler = (jjmGetFatalHandler() = & fatalHandler, false); 

    template <size_t N>
    bool startsWith(string const& str, char const (&prefix)[N])
    {
        if (str.size() < N - 1)
            return false; 
        for (size_t i = 0; i < N - 1; ++i)
        {   if (str[i] != prefix[i])
                return false;
        }
        return true; 
    }
}


namespace jjm { int jjmakemain(vector<string> const& args); }
#ifdef _WIN32
    int wmain(int argc, wchar_t **argv)
    {
        try
        {   
            vector<string> args;
            try
            {   for (int x = 1; x < argc; ++x)
                {   Utf8String u8str = makeU8StrFromCpRange(makeCpRangeFromUtf16(makeNullTermRange(argv[x]))); 
                    if (startsWith(u8str, "--console-encoding="))
                    {   string encoding = u8str.substr(strlen("--console-encoding=")); 
                        jjm::setJinEncoding(encoding);
                        jjm::setJoutEncoding(encoding);
                        jjm::setJoutEncoding(encoding); 
                        continue; 
                    }
                    args.push_back(u8str);  
                }
            } catch (std::exception & e)
            {   string message; 
                message += "Failed to convert an argument of main() from UTF-16 to UTF-8. Cause:\n";
                message += e.what(); 
                throw std::runtime_error(message);
            }
            return jjm::jjmakemain(args);
        }
        catch (std::exception & e)
        {   string message; 
            message += typeid(e).name() ;
            message += ": ";
            message += e.what(); 
            message += "\n"; 
            jerr() << message << flush; 
        }
        return 1;

    }
#else
    int main(int argc, char** argv)
    {
        try
        {
            string argumentEncoding = jjm::getEncodingNameFrom_setlocale_LC_ALL_emptyString(); 

            vector<Utf8String> utf8args;
            for (int x = 1; x < argc; ++x)
            {
                string utf8arg;
                try
                {   utf8arg = jjm::convertEncoding("UTF-8", argumentEncoding, argv[x]); 
                } catch (std::exception & e)
                {   string message; 
                    message += "Failed to transcode an argument of main() to UTF-8. Cause:\n"; 
                    message += e.what();
                    throw std::runtime_error(message); 
                }
                if (startsWith(utf8arg, "--console-encoding="))
                {   string encoding = utf8arg.substr(strlen("--console-encoding=")); 
                    jjm::setJinEncoding(encoding);
                    jjm::setJoutEncoding(encoding);
                    jjm::setJoutEncoding(encoding); 
                    argumentEncoding = encoding; 
                    jjm::setEncodingOfEnvVars(encoding); 
                    continue; 
                }
                utf8args.push_back(utf8arg); 
            }
            return jjm::jjmakemain(utf8args);
        }
        catch (std::exception & e)
        {   string message; 
            message += typeid(e).name() ;
            message += ": ";
            message += e.what(); 
            message += "\n"; 
            jerr() << message << flush; 
        }
        return 1;
    }
#endif


namespace
{
    inline string escapeSingleQuote(string const& x)
    {
        string r;
        for (size_t i=0; i < x.size(); ++i)
        {   if (x[i] == '\'')
            {   r += '\'';
                r += '\\';
                r += '\'';
                r += '\'';
            }else
                r += x[i]; 
        }
        return r; 
    }

    void printHelpMessage()
    {
        BufferedOutputStream & s = jout(); 
        s << "--always-make\n";
        s << "        All goals are treated as out-of-date.\n"; 
        s << "\n";
        s << "--all-dependencies\n";
        s << "--all-dependents\n";
        s << "        Tells jjmake to also build all of the dependencies / dependents of\n";
        s << "        specified goals. Also tells jjmake to build all goals in\n";
        s << "        dependency / dependent order.\n";
        s << "        The default is --all-dependencies.\n"; 
        s << "\n"; 
        s << "--no-dependencies\n"; 
        s << "        Tells jjmake to build the goals in any order. Also tells jjmake\n"; 
        s << "        to not build goals unless they are explicitly specified on the\n"; 
        s << "        command line.\n"; 
        s << "\n";
        s << "--all-goals\n"; 
        s << "        Tell make to build all goals. This option is useful with -P.\n"; 
        s << "        This option is disabled by default.\n"; 
        s << "        When combined with --no-dependencies, jjmake will build all goals\n"; 
        s << "        in a random order; you probably do not want this except with -P.\n"; 
        s << "\n";

        s << "--console-encoding=<encoding>\n";
        s << "        Specifies the encoding of stdin, stdout, and stderr.\n";
#if defined(_WIN32)
        s << "\n"; 
        s << "        Native Windows Application rules:\n";
        s << "\n"; 
        s << "        If any of the standard handles (stdin, stdout, stderr) are directly\n";
        s << "        connected to a Windows terminal, then it will directly read / write\n";
        s << "        in UTF-16 via Windows-specific APIs.\n";
        s << "\n"; 
        s << "        Note: When a standard handle is redirected to/from a file or piped\n";
        s << "        to/from another program, this program will use the encoding\n";
        s << "        specified by --console-encoding.\n"; 
        s << "\n"; 
        s << "        Note: This program will use the encoding specified by\n";
        s << "        --console-encoding when directly read from / writing to a cygwin\n";
        s << "        terminal (because the cygwin terminals do not implement the\n";
        s << "        Windows-specific functions to count as a Windows-terminal).\n";
        s << "\n"; 
        s << "        Arguments of main() are accessed in UTF-16 via Windows-specific\n";
        s << "        APIs. It is the responsibility of the caller process to correctly\n";
        s << "        transcode when working with the Windows CreateProcess API.\n";
        s << "\n"; 
        s << "        Environmental variables are accessed in UTF-16 via Windows-specific\n";
        s << "        APIs. It is the responsibility of the caller process to correctly\n";
        s << "        transcode when working with the Windows CreateProcess API.\n";
        s << "\n"; 
        s << "        File paths are accessed in UTF-16 via Windows-specific APIs.\n";
#else
        s << "        Also specifies the encoding of all subsequent arguments of main().\n";
        s << "        Also specifies the encoding of environmental variables.\n";
        s << "        Also specifies the encoding of file paths.\n";
#endif
        s << "\n"; 
        s << "        Remember: The jjmake 'include' function always assumes UTF-8 file\n";
        s << "        contents.\n"; 

        s << "\n";
        s << "-D<var>=<val>\n";
        s << "-D <var>=<val>\n";
        s << "        Create a variable with the given name and\n"; 
        s << "        value in the root context.\n"; 
        s << "\n";
        s << "<goal>\n";
        s << "-G<goal>\n";
        s << "-G <goal>\n";
        s << "--goal=<goal>\n";
        s << "        Tell make to execute the goal.\n"; 
        s << "        this option is additive.\n"; 
        s << "\n";
        s << "-h\n";
        s << "-help\n";
        s << "--help\n";
        s << "        Display this help message.\n"; 
        s << "\n";
        s << "-I<file>\n";
        s << "-I <file>\n";
        s << "--include=<file>\n";
        s << "        Causes the specified file to be included in the root context.\n"; 
        s << "-K\n";
        s << "--keep-going\n";
        s << "        Continue as much as possible after a goal execution failure.\n";
        s << "        The default is to stop as soon as possible after a goal execution fails.\n";
        s << "\n";
        s << "-P\n";
        s << "--just-print\n";
        s << "        Instead of executing goals, print the names of goals when they\n";
        s << "        would be executed.\n";
        s << "\n";
        s << "-T<N>\n";
        s << "-T <N>\n";
        s << "--threads=<N>\n";
        s << "        Specify the number of goals to run concurrently.\n";
        s << "\n";
        s << "-v\n"; 
        s << "-V\n"; 
        s << "-version\n"; 
        s << "-Version\n"; 
        s << "--version\n"; 
        s << "--Version\n"; 
        s << "        Display version information.\n"; 
        s << flush; 
    }
}

int jjm::jjmakemain(vector<string> const& args)
{
    JjmakeContext::Arguments jjarguments; 
    bool hasInclude = false; 
    for (std::vector<string>::const_iterator arg = args.begin(); arg != args.end(); ++arg)
    {
        if (*arg == "--all-dependencies")
        {   jjarguments.dependencyMode = JjmakeContext::AllDependencies; 
            continue; 
        }
        if (*arg == "--all-dependents")
        {   jjarguments.dependencyMode = JjmakeContext::AllDependents; 
            continue; 
        }
        if (*arg == "--all-goals")
        {   jjarguments.allGoals = true; 
            continue; 
        }
        if (*arg == "--always-make")
        {   jjarguments.alwaysMake = true;
            continue; 
        }
        if (startsWith(*arg, "-D"))
        {   string x; 
            if (arg->size() == 2)
            {   ++arg; 
                if (arg == args.end())
                    throw std::runtime_error("Command line option \"-D\" without definition."); 
                x = *arg;
            }else
                x = arg->substr(2, string::npos); 
            if (x.find('=') == string::npos)
                throw std::runtime_error("Missing '=' in -D command line option \"" + *arg + "\"."); 
            string name = x.substr(0, x.find('=')); 
            string val =  x.substr(x.find('=') + 1, string::npos); 
            jjarguments.rootEvalText += "(set '" + escapeSingleQuote(name) + "' '" + escapeSingleQuote(val) + "')\n"; 
            continue; 
        }
        if (startsWith(*arg, "-G"))
        {   string x; 
            if (arg->size() == 2)
            {   ++arg; 
                if (arg == args.end())
                    throw std::runtime_error("Command line option \"-G\" without following goal."); 
                x = *arg;
            }else
                x = arg->substr(2, string::npos); 
            jjarguments.goals.push_back(x); 
            continue; 
        }
        if (startsWith(*arg, "--goal="))
        {   string x = arg->substr(strlen("--goal=")); 
            jjarguments.goals.push_back(x); 
            continue; 
        }
        if (   *arg == "-h" || *arg == "-help" || *arg == "--help")
        {   printHelpMessage(); 
            return 0; 
        }
        if (startsWith(*arg, "-I"))
        {   string x; 
            if (arg->size() == 2)
            {   ++arg; 
                if (arg == args.end())
                    throw std::runtime_error("Command line option \"-I\" without following path."); 
                x = *arg;
            }else
                x = arg->substr(2, string::npos); 
            hasInclude = true; 
            jjarguments.rootEvalText += "(include '" + escapeSingleQuote(x) + "')\n"; 
            continue; 
        }
        if (startsWith(*arg, "--include="))
        {   string x = arg->substr(strlen("--include=")); 
            hasInclude = true; 
            jjarguments.rootEvalText += "(include '" + escapeSingleQuote(x) + "')\n"; 
            continue; 
        }
        if (*arg == "-K" || *arg == "--keep-going")
        {   jjarguments.keepGoing = true; 
            continue; 
        }
        if (*arg == "--no-dependencies")
        {   jjarguments.dependencyMode = JjmakeContext::NoDependencies; 
            continue; 
        }
        if (*arg == "-P" || *arg == "--just-print")
        {   jjarguments.executionMode = JjmakeContext::PrintGoals; 
            continue; 
        }
        if (startsWith(*arg, "-T"))
        {   string x; 
            if (arg->size() == 2)
            {   ++arg; 
                if (arg == args.end())
                    throw std::runtime_error("Command line option \"-T\" without following number."); 
                x = *arg;
            }else
                x = arg->substr(2, string::npos); 
            int y = 0; 
            if (false == decStrToInteger(y, x))
                throw std::runtime_error("Not a valid number in -T command line option \"" + *arg + "\"."); 
            if (y < 1)
                throw std::runtime_error("Invalid number in -T command line option \"" + *arg + "\"."); 
            jjarguments.numThreads = y; 
            continue;
        }
        if (startsWith(*arg, "--threads="))
        {   string x = arg->substr(strlen("--threads=")); 
            int y = 0; 
            if (false == decStrToInteger(y, x))
                throw std::runtime_error("Not a valid number in --threads=<N> command line option \"" + *arg + "\"."); 
            if (y < 1)
                throw std::runtime_error("Invalid number in --threads=<N> command line option \"" + *arg + "\"."); 
            jjarguments.numThreads = y; 
            continue;
        }
        if (   *arg == "-v" || *arg == "-version" || *arg == "--version"
            || *arg == "-V" || *arg == "-Version" || *arg == "--Version")
        {
            //TODO print version information
            JFATAL(0, 0); 
        }
        if ( ! startsWith(*arg, "-"))
        {   jjarguments.goals.push_back(*arg); 
            continue;
        }
        string message;
        message += "Unrecognized command line option \"" + *arg + "\". ";
        message += "For usage information, >>--help<<."; 
        throw std::runtime_error(message); 
    }

    if (hasInclude == false)
        jjarguments.rootEvalText += "(include jjmake.txt)\n"; 

    JjmakeContext context(jjarguments); 
    context.execute(); 
    return 0; 
}
