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
    #define USING_STATIC_LIBICONV 1
    #include "libiconv/include/iconv.h"
#else
    #include <iconv.h>
#endif

namespace jjm
{


//TODO see if I can make a better interface. 
class IconvConverter
{
public:
    IconvConverter();
    ~IconvConverter();

    //safe to call init() several times. 
    //Calling init() nukes any contents of input and output. 
    void init(std::string const& toEncoding, std::string const& fromEncoding);

    //will overwrite any existing contents of this->output
    void convert(); 
    
    /* This function is meant to reset the state of the converter for any 
    stateful encoding. The user should always call this function after 
    finishing processing one stream of data and before starting another stream
    of data. 
    This function will overwrite any existing contents of this->input and 
    this->output. */
    void resetState(); 

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


//Utility function, it uses IconvConverter
std::string iconvConvert(
        std::string const& toEncoding, 
        std::string const& fromEncoding, 
        std::string const& text
        ); 


}//namespace jjm

#endif
