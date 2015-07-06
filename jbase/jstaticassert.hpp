// Copyright (c) 2010-2011, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JSTATICASSERT_HPP_HEADER_GUARD
#define JSTATICASSERT_HPP_HEADER_GUARD

#include "jwarningpragmas.hpp"

template <bool > class JjmStaticAssertClassTemplate {};
#define JSTATICASSERT(bool_const_expr) \
        typedef JjmStaticAssertClassTemplate<true> JjmStaticAssertTypedef; \
        typedef JjmStaticAssertClassTemplate<bool_const_expr> JjmStaticAssertTypedef;

#endif
