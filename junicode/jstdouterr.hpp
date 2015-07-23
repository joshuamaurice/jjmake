// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JSTDOUTERR_HPP_HEADER_GUARD
#define JUNICODE_JSTDOUTERR_HPP_HEADER_GUARD

#include <cstddef>
#include <string>

namespace jjm
{

/*
On windows, it will convert the input to Utf16String, and then use std::wcout
to do its work. 

On POSIX systems, it will use setlocale(LC_ALL, "") and iconv to convert to 
the proper locale and encoding, and then write to stdout. 

Return 0 on success, -1 on failure. 
*/
int writeToStdOut(std::string const& utf8);
int writeToStdOut(char const* nullTermUtf8);
int writeToStdOut(char const* utf8, std::size_t bytes);

//returns 0 on success, -1 on failure
int flushStdOut(); 

}//namespace jjm

#endif

