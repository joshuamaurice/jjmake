// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_FILETYPE_HPP_HEADER_GUARD
#define JOSUTILS_FILETYPE_HPP_HEADER_GUARD


namespace jjm
{

class FileType
{   
private:
    enum E { eInvalid = 0, eNoExist = 1, eRegularFile = 2, eDirectory = 3, eSymlink = 4, eOther = 5 }; //DO NOT RELY ON THE NUMBERS
    explicit FileType(E x) : e(x) {}
    E e; 

public:
    FileType() : e(eNoExist) {}
    //default copy ctor, operator=, and dtor suffice
    friend inline bool operator== (FileType a, FileType b) { return a.e == b.e; }
    friend inline bool operator!= (FileType a, FileType b) { return a.e != b.e; }
    friend inline bool operator<= (FileType a, FileType b) { return a.e <= b.e; }
    friend inline bool operator<  (FileType a, FileType b) { return a.e <  b.e; }
    friend inline bool operator>= (FileType a, FileType b) { return a.e >= b.e; }
    friend inline bool operator>  (FileType a, FileType b) { return a.e >  b.e; }

    static FileType Invalid;
    static FileType NoExist;
    static FileType RegularFile;
    static FileType Directory;
    static FileType Symlink;
    static FileType Other; //includes POSIX S_ISBLK, S_ISCHR, S_ISFIFO, 

public:
    E toEnum() const { return e; } //for debugging only, and 
};

} //namespace jjm

#endif
