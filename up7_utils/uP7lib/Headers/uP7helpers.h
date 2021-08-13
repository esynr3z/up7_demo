////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// 2012-2021 (c) Baical                                                        /
//                                                                             /
// This library is free software; you can redistribute it and/or               /
// modify it under the terms of the GNU Lesser General Public                  /
// License as published by the Free Software Foundation; either                /
// version 3.0 of the License, or (at your option) any later version.          /
//                                                                             /
// This library is distributed in the hope that it will be useful,             /
// but WITHOUT ANY WARRANTY; without even the implied warranty of              /
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           /
// Lesser General Public License for more details.                             /
//                                                                             /
// You should have received a copy of the GNU Lesser General Public            /
// License along with this library.                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////
#ifndef UP7HELPERS_H
#define UP7HELPERS_H

#if defined(__GNUC__)
    #define ST_PACK_ENTER(x) 
    #define ST_PACK_EXIT(x) 
    #define ST_ATTR_PACK(x)   __attribute__ ((aligned(x), packed))

#elif defined(_MSC_VER)
    #define ST_PACK_ENTER(x)  __pragma(pack(push, x))
    #define ST_PACK_EXIT()    __pragma(pack(pop))
    #define ST_ATTR_PACK(x)
#endif

#if !defined(TRUE)
    #define TRUE              1
    #define FALSE             0
#endif

#if !defined(UNUSED_ARG)
    #define UNUSED_ARG(x)     (void)(x)
#endif

#if !defined(ST_ASSERT)
    #define ST_ASSERT(cond)   typedef int assert_type[(cond) ? 1 : -1]
#endif

#endif //UP7HELPERS_H
