// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JSTATICASSERT_HPP_HEADER_GUARD
#define JSTATICASSERT_HPP_HEADER_GUARD

#include "jwarningpragmas.hpp"

#if defined(_WIN32)
    template <bool > class JjmStaticAssertClassTemplate {};
    #define JSTATICASSERT(bool_const_expr) \
            typedef JjmStaticAssertClassTemplate<true> JjmStaticAssertTypedef; \
            typedef JjmStaticAssertClassTemplate<bool_const_expr> JjmStaticAssertTypedef;
#else
    #define JSTATICASSERT(bool_const_expr) static_assert(bool_const_expr, ""); 
#endif

#endif
