// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JWARNINGPRAGMAS_HPP_HEADER_GUARD
#define JWARNINGPRAGMAS_HPP_HEADER_GUARD

#if defined(_WIN32) && defined(_MSC_VER)
    #pragma warning (disable : 4100) //unreferenced formal parameter
    #pragma warning (disable : 4127) //conditional expression is constant
    #pragma warning (disable : 4244) //conversion from 'XX' to 'XX', possible loss of data
    //#pragma warning (disable : 4390) 
    //#pragma warning (disable : 4503) //decorated name length exceed, name was truncated
    #pragma warning (disable : 4512) //assignment operator could not be generated
    //#pragma warning (disable : 4702) //unreachable code
    #pragma warning (disable : 4800) //forcing value to bool 'true' or 'false' (performance warning)
    //#pragma warning (disable : 4805)
    #pragma warning (disable : 4996) //Ex: 'std::equal': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#endif

#endif
