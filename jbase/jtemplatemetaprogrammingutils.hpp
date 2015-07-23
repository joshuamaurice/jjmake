// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JTEMPLATEMETAPROGRAMMING_HPP_HEADER_GUARD
#define JTEMPLATEMETAPROGRAMMING_HPP_HEADER_GUARD

#include "jstaticassert.hpp"
#include <iterator>

namespace jjm
{
    // ****
    // not intended for users
    namespace Internal
    {   struct True { char c[16]; };
        struct False { char c[1]; };
        JSTATICASSERT(sizeof(True) != sizeof(False));
    }

    // ****
    // for users: 

    //is-same-type tester
    template <typename A, typename B> struct IsSameType { static int const b = 0; };
    template <typename A> struct IsSameType<A,A> { static int const b = 1; };

    //is-convertible-to tester
    template <typename Source, typename Target> 
    struct IsConvertibleTo
    {
    private:
        static Internal::True helper(int , Target const& ) { return Internal::True(); }
        static Internal::False helper(int , ... ) { return Internal::False(); }
    public:
        static int const b = (sizeof(Internal::True) == sizeof(helper(1, Source())) ? 1 : 0);
    };
}

#endif
