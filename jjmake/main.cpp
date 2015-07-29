// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "jbase/jinttostring.hpp"
#include "jbase/jnulltermiter.hpp"
#include "jbase/juniqueptr.hpp"
#include "jbase/jnulltermiter.hpp"
#include "junicode/jutfstring.hpp"
#include "josutils/jstdinouterr.hpp"
#include <iostream>
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
        cerr << message << flush; 
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
            Utf8String u8str = makeU8StrFromCpRange(makeCpRangeFromUtf16(utf16range)); 
            if (u8str.size())
                args.push_back(u8str);  
            else
                args.push_back(std::string());  
        }
        return jjm::jjmakemain(args);
    }
#else
    int main(int argc, char** argv)
    {
        //TODO use setlocale(LC_ALL, ""); and iconv
        vector<string> args;
        for (int x = 1; x < argc; ++x)
            args.push_back(string(argv[x])); 
        return jjm::jjmakemain(args);
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
        jout() << "-A\n";
        jout() << "--always-make\n";
        jout() << "        All targets are treated as out-of-date.\n"; 
        jout() << "\n";
        jout() << "-D<var>=<val>\n";
        jout() << "-D <var>=<val>\n";
        jout() << "        Create a variable with the given name and\n"; 
        jout() << "        value in the root context.\n"; 
        jout() << "\n";
        jout() << "-G<goal>\n";
        jout() << "-G <goal>\n";
        jout() << "--goal=<goal>\n";
        jout() << "        Tell make to execute that goal.\n"; 
        jout() << "\n";
        jout() << "-h\n";
        jout() << "-help\n";
        jout() << "--help\n";
        jout() << "        Display this help message.\n"; 
        jout() << "\n";
        jout() << "-I<file>\n";
        jout() << "-I <file>\n";
        jout() << "--include=<file>\n";
        jout() << "        Include the given file in the root context.\n"; 
        jout() << "-K\n";
        jout() << "--keep-going\n";
        jout() << "        Continue as much after an error during\n";
        jout() << "        a target execution.\n";
        jout() << "\n";
        jout() << "-P\n";
        jout() << "--just-print\n";
        jout() << "        Instead of executing goals, print what goals\n";
        jout() << "        would be executed.\n";
        jout() << "\n";
        jout() << "-T<N>\n";
        jout() << "-T <N>\n";
        jout() << "--threads=<N>\n";
        jout() << "        Specify the number of goals to run\n";
        jout() << "        concurrently.\n";
        jout() << "\n";
        jout() << "-v\n"; 
        jout() << "-V\n"; 
        jout() << "-version\n"; 
        jout() << "-Version\n"; 
        jout() << "--version\n"; 
        jout() << "--Version\n"; 
        jout() << "        Display version information.\n"; 
        jout() << flush; 
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
            if (startsWith(*arg, "--stdin-encoding="))
            {   string encoding = arg->substr(string("--stdin-encoding=").size()); 
                jjm::setJinEncoding(encoding); 
                continue; 
            }
            if (startsWith(*arg, "--stdout-encoding="))
            {   string encoding = arg->substr(string("--stdout-encoding=").size()); 
                jjm::setJoutEncoding(encoding); 
                continue; 
            }
            if (startsWith(*arg, "--stderr-encoding="))
            {   string encoding = arg->substr(string("--stderr-encoding=").size()); 
                jjm::setJerrEncoding(encoding); 
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
        jerr() << message << flush; 
    }
    return 1;
}
