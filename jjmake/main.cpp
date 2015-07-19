// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jjmakecontext.hpp"

#include "jbase/jnulltermiter.hpp"
#include "jbase/juniqueptr.hpp"
#include "josutils/jpath.hpp"
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


namespace jjm { int jjmakemain(vector<string> const& args); }
#ifdef _WIN32
    int wmain(int argc, wchar_t **argv)
    {
        vector<string> args;
        for (int x = 1; x < argc; ++x)
        {   U8Str u8str = U8Str::utf16(makeNullTermRange(argv[x])); 
            args.push_back(std::string(u8str.c_str()));  
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
        for (size_t i=0; i < N; ++i)
        {   if (i >= N)
                return false; 
            if (str[i] != prefix[i])
                false;
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
}

int jjm::jjmakemain(vector<string> const& args)
{
    try 
    {
        string effectiveFileContents; 
        bool hasInclude = false; 
        vector<string> goals; 
        for (std::vector<string>::const_iterator arg = args.begin(); arg != args.end(); ++arg)
        {
            if (startsWith(*arg, "-I"))
            {   if (arg->size() == 2)
                {   ++arg; 
                    if (arg == args.end())
                        throw std::runtime_error("Command line option \"-I\" without following path."); 
                }
                hasInclude = true; 
                effectiveFileContents += "(include '" + escapeSingleQuote(*arg) + "')\n"; 
                continue; 
            }
            if (startsWith(*arg, "-D"))
            {   if (arg->size() == 2)
                {   ++arg; 
                    if (arg == args.end())
                        throw std::runtime_error("Command line option \"-D\" without following path."); 
                }
                string x = *arg; 
                if (x.find('=') == string::npos)
                    throw std::runtime_error("Missing '=' in command line option \"" + x + "\"."); 
                string name = x.substr(0, x.find('=')); 
                string val =  x.substr(0, x.find('=') + 1); 
                effectiveFileContents += "(set '" + escapeSingleQuote(name) + "' '" + escapeSingleQuote(val) + "')\n"; 
                continue; 
            }
            if (   *arg == "-v" || *arg == "-version" || *arg == "--version"
                || *arg == "-V" || *arg == "-Version" || *arg == "--Version")
            {
                //TODO print version information
                JFATAL(0, 0); 
            }
            if (   *arg == "-h" || *arg == "-help" || *arg == "--help")
            {
                //TODO print help information
                JFATAL(0, 0); 
            }
            throw std::runtime_error("Unrecognized command line option \"" + *arg + "\"."); 
        }

        if (hasInclude == false)
            effectiveFileContents += "(include jjmake.txt)\n"; 

        JjmakeContext context(JjmakeContext::ExecuteGoals, JjmakeContext::AllDependencies, goals);
        context.execute(effectiveFileContents); 
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
