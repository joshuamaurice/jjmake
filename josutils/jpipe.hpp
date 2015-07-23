// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JPIPE_HPP_HEADER_GUARD
#define JPIPE_HPP_HEADER_GUARD

#include "jfilehandle.hpp"

namespace jjm
{

class Pipe
{
public:
    FileHandle readable;
    FileHandle writeable; 

    /* create() is a thin abstraction on type of pipe() on POSIX systems
    and CreatePipe() on Windows. 

    create() throws std::exception on errors. 

    It does not return "null" handles. 

    Both FileDes objects must be closed manually by the user, else resource 
    leak.

    Returned handles are not inheritable, ala O_CLOEXEC is given pipe() on 
    POSIX, and setting SECURITY_ATTRIBUTES::bInheritHandle = FALSE on 
    win32. */
    static Pipe create();
};

}

#endif
