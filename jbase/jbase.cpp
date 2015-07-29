// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jwarningpragmas.hpp"

#include "jfatal.hpp"
#include "jinttostring.hpp"
#include "jnulltermiter.hpp"
#include "jstdint.hpp"
#include "jtemplatemetaprogrammingutils.hpp"
#include "juniqueptr.hpp"

//Just do a sanity check. The rest of the program assumes that we're compiling 
//for Windows, or for POSIX. 
#if defined(_WIN32)
#elif defined(__unix__) || defined(__unix)
    #include <unistd.h>
    #if defined(_POSIX_VERSION)
    #else
        #error
    #endif
#else
    #error
#endif
