// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef FATAL_HPP_INCLUDE_GUARD
#define FATAL_HPP_INCLUDE_GUARD

#include "jwarningpragmas.hpp"

#include <cstdlib>
#include <string>

/*JFATAL is a runtime assert in debug and release. It calls abort(). Use it for
sanity checks and developer assistance. All calls to JFATAL should mean a bug
exists and it should be fixed. End users of a program should not be able to
trigger a JFATAL.

info_str can be either a std::string or a c-string (a pointer to a
null-terminated char array). 

Do not use this in situations where only async-signal safe may be used. */
#define JFATAL(info_n, info_str) JFATAL_IMPL(__FILE__, __LINE__, info_n, info_str)


/*User needs to provide this. */
typedef void (*JjmFatalHandlerType)(char const * filename, int linenum, int info_n, char const* info_cstr);
inline JjmFatalHandlerType& jjmGetFatalHandler() { static JjmFatalHandlerType x = 0; return x; }

/*Utility functions so users of JFATAL() can use other arguments,
like std::string, as the second argument to JFATAL.*/
inline char const* JjmFatalHandlerUtil(char const* info_str) { return info_str; }
inline char const* JjmFatalHandlerUtil(std::string const& info_str) { return info_str.c_str(); }


#define JFATAL_IMPL(file, line, info_n, info_str) \
        {   try \
            {   using ::JjmFatalHandlerUtil; \
                JjmFatalHandlerType h = jjmGetFatalHandler(); \
                if (h) \
                    (*h)(file, line, info_n, JjmFatalHandlerUtil(info_str)); \
            } catch (...) {} \
            abort(); \
        }
        

#endif
