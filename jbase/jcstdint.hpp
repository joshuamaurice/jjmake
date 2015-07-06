// Copyright (c) 2010-2011, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JCSTDINT_HPP_HEADER_GUARD
#define JCSTDINT_HPP_HEADER_GUARD

#include "jwarningpragmas.hpp"
#include "jstaticassert.hpp"

#include <climits>
#include <cstdlib>

#ifdef _WIN32
    #ifdef _WIN64
        namespace std
        {   
            typedef unsigned char   uint8_t;
            typedef signed char      int8_t;
            typedef unsigned short  uint16_t;
            typedef signed short     int16_t;
            typedef unsigned int    uint32_t;
            typedef signed int       int32_t;
            typedef unsigned long long  uint64_t;
            typedef signed long long    int64_t;

            typedef unsigned long long  size_t; //done to assert that the ssize_t is the proper typedef
            typedef          long long  ssize_t;
        }
    #else
        namespace std
        {   
            typedef unsigned char   uint8_t;
            typedef signed char      int8_t;
            typedef unsigned short  uint16_t;
            typedef signed short     int16_t;
            typedef unsigned int    uint32_t;
            typedef signed int       int32_t;
            typedef unsigned long long  uint64_t;
            typedef signed long long     int64_t;

            typedef unsigned int   size_t; //done to assert that the ssize_t is the proper typedef
            typedef          int   ssize_t;
        }
    #endif

    JSTATICASSERT(CHAR_BIT == 8)
    JSTATICASSERT(sizeof(std::uint8_t) == 1)
    JSTATICASSERT(sizeof(std:: int8_t) == 1)
    JSTATICASSERT(sizeof(std::uint16_t) == 2)
    JSTATICASSERT(sizeof(std:: int16_t) == 2)
    JSTATICASSERT(sizeof(std::uint32_t) == 4)
    JSTATICASSERT(sizeof(std:: int32_t) == 4)
    JSTATICASSERT(sizeof(std::uint64_t) == 8)
    JSTATICASSERT(sizeof(std:: int64_t) == 8)
#else
    #include <cstdint>
#endif

#endif
