// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "jbase/jnulltermiter.hpp"
#include "jbase/juniqueptr.hpp"
#include "junicode/jutfstring.hpp"
#include "junicode/jstdouterr.hpp"
#include "jbase/jnulltermiter.hpp"
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif


using namespace jjm;
using namespace std;


namespace 
{
    void fatalHandler(char const * const filename, int linenum, int info_n, char const * info_cstr)
    {
        string message; 
        message += string() + "JFATAL(), file \"" + filename + "\", line " + toDecStr(linenum) + ", info [" + toDecStr(info_n) + "]"; 
        if (info_cstr)
            message += string() + ", \"" + info_cstr + "\"";
        message += ".\n"; 
        jjm::writeToStdOut(message); 
        jjm::flushStdOut(); 
    }
    bool installFatalHandler = (jjmGetFatalHandler() = & fatalHandler, false); 
}


namespace jjm { int jjmakemain(vector<string> const& args); }
#ifdef _WIN32
    int wmain(int argc, wchar_t **argv)
    {
        vector<string> args;
        for (int x = 1; x < argc; ++x)
        {   auto utf16range = makeNullTermRange(argv[x]);
            U8Str u8str = U8Str::utf16(utf16range); 
            if (u8str.sizeBytes())
                args.push_back(std::string(u8str.c_str()));  
            else
                args.push_back(std::string());  
        }
        return jjm::jjmakemain(args);
    }
#else
    int main(int argc, char** argv)
    {
        //TODO use setlocale(LC_ALL, ""); and iconv
        JFATAL(0, 0); 
    }
#endif


namespace
{
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
        jjm::writeToStdOut("-A\n");
        jjm::writeToStdOut("--always-make\n");
        jjm::writeToStdOut("        All targets are treated as out-of-date.\n"); 
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-D<var>=<val>\n");
        jjm::writeToStdOut("-D <var>=<val>\n");
        jjm::writeToStdOut("        Create a variable with the given name and\n"); 
        jjm::writeToStdOut("        value in the root context.\n"); 
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-G<goal>\n");
        jjm::writeToStdOut("-G <goal>\n");
        jjm::writeToStdOut("--goal=<goal>\n");
        jjm::writeToStdOut("        Tell make to execute that goal.\n"); 
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-h\n");
        jjm::writeToStdOut("-help\n");
        jjm::writeToStdOut("--help\n");
        jjm::writeToStdOut("        Display this help message.\n"); 
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-I<file>\n");
        jjm::writeToStdOut("-I <file>\n");
        jjm::writeToStdOut("--include=<file>\n");
        jjm::writeToStdOut("        Include the given file in the root context.\n"); 
        jjm::writeToStdOut("-K\n");
        jjm::writeToStdOut("--keep-going\n");
        jjm::writeToStdOut("        Continue as much after an error during\n");
        jjm::writeToStdOut("        a target execution.\n");
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-P\n");
        jjm::writeToStdOut("--just-print\n");
        jjm::writeToStdOut("        Instead of executing goals, print what goals\n");
        jjm::writeToStdOut("        would be executed.\n");
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-T<N>\n");
        jjm::writeToStdOut("-T <N>\n");
        jjm::writeToStdOut("--threads=<N>\n");
        jjm::writeToStdOut("        Specify the number of goals to run\n");
        jjm::writeToStdOut("        concurrently.\n");
        jjm::writeToStdOut("\n");
        jjm::writeToStdOut("-v\n"); 
        jjm::writeToStdOut("-V\n"); 
        jjm::writeToStdOut("-version\n"); 
        jjm::writeToStdOut("-Version\n"); 
        jjm::writeToStdOut("--version\n"); 
        jjm::writeToStdOut("--Version\n"); 
        jjm::writeToStdOut("        Display version information.\n"); 
        jjm::flushStdOut(); 
    }
}

int jjm::jjmakemain(vector<string> const& args)
{
    try 
    {
        JjmakeContext::Arguments jjarguments; 
        bool hasInclude = false; 
        for (std::vector<string>::const_iterator arg = args.begin(); arg != args.end(); ++arg)
        {
            if (*arg == "-A" || *arg == "--always-make")
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
            {   string x = arg->substr(string("--goal=").size()); 
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
            {   string x = arg->substr(string("--include=").size()); 
                hasInclude = true; 
                jjarguments.rootEvalText += "(include '" + escapeSingleQuote(x) + "')\n"; 
                continue; 
            }
            if (*arg == "-K" || *arg == "--keep-going")
            {   jjarguments.keepGoing = true; 
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
            {   string x = arg->substr(string("--threads=").size()); 
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
            if (startsWith(*arg, "-"))
                throw std::runtime_error("Unrecognized command line option \"" + *arg + "\"."); 

            //goal
            jjarguments.goals.push_back(*arg); 
        }

        if (hasInclude == false)
            jjarguments.rootEvalText += "(include jjmake.txt)\n"; 

        JjmakeContext context(jjarguments); 
        context.execute(); 
    }
    catch (std::exception & e)
    {   string message; 
        message += typeid(e).name() ;
        message += ": ";
        message += e.what(); 
        message += "\n"; 
        jjm::writeToStdOut(message); 
        jjm::flushStdOut(); 
    }
    return 1;
}
