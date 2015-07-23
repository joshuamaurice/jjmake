// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JSTAT_HPP_HEADER_GUARD
#define JSTAT_HPP_HEADER_GUARD

#include "jfiletype.hpp"
#include "jpath.hpp"
#include "jfilehandle.hpp"
#include "jbase/jstdint.hpp"
#include "junicode/jutfstring.hpp"

#include <string>

namespace jjm
{

class Stat
{
public:
    Stat() : type(FileType::Invalid), lastWriteTimeNanoSec(0), lastChangeTimeNanoSec(0) {}

    //When the path names a symbolic link, jjm::Stat returns information about
    //the target of the link. 
    //Throws std::exception on errors. 
    //(FileType::NoExist is not an error.) 
    static Stat stat(Path const& path); 

    //When the path names a symbolic link, jjm::Stat returns information about
    //the target of the link. 
    //On errors, returns a stat object with FileType::Invalid. 
    //(FileType::NoExist is not an error.) 
    static Stat stat2(Path const& path); 

    //When the path names a symbolic link, jjm::LStat returns information about
    //the link itself and not the target of the link. 
    //Throws std::exception on errors. 
    //(FileType::NoExist is not an error.) 
    static Stat lstat(Path const& path); 

    //When the path names a symbolic link, jjm::LStat returns information about
    //the link itself and not the target of the link. 
    //On errors, returns a stat object with FileType::Invalid. 
    //(FileType::NoExist is not an error.) 
    static Stat lstat2(Path const& path); 

#ifdef _WIN32
    static Stat get2(FileHandle file); //on errors, returns a stat object with FileType::Invalid
#endif

    FileType type;
    std::int64_t lastWriteTimeNanoSec; //time since unix epoch
    std::int64_t lastChangeTimeNanoSec; //time since unix epoch
};

} //namespace jjm

#endif
