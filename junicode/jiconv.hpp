// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JUNICODE_JICONV_HPP_HEADER_GUARD
#define JUNICODE_JICONV_HPP_HEADER_GUARD

#include "jbase/juniqueptr.hpp"

#include <string>
#include <cstddef>

#ifdef _WIN32
    #include "libiconv/include/iconv.h"
#else
    #include <iconv.hpp>
#endif

namespace jjm
{

//TODO see if I can make a better interface. 
class IconvConverter
{
public:
    IconvConverter();
    ~IconvConverter();

    void init(std::string const& toEncoding, std::string const& fromEncoding);

    //will overwrite any existing contents of this->output
    void convert(); 

    //can't use std::vector because of potential alignment issues

    UniqueArray<char*> input;
    size_t inputSize; 
    size_t inputCapacity; 

    UniqueArray<char*> output;
    size_t outputSize; 
    size_t outputCapacity; 

private:
    IconvConverter(IconvConverter const& ); //not defined, not copyable
    IconvConverter& operator= (IconvConverter const& ); //not defined, not copyable

    iconv_t converter; 
};

}//namespace jjm

#endif
