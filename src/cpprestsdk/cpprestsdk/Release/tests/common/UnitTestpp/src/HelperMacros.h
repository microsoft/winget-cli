/***
 * This file is based on or incorporates material from the UnitTest++ r30 open source project.
 * Microsoft is not the original author of this code but has modified it and is licensing the code under
 * the MIT License. Microsoft reserves all other rights not expressly granted under the MIT License,
 * whether by implication, estoppel or otherwise.
 *
 * UnitTest++ r30
 *
 * Copyright (c) 2006 Noel Llopis and Charles Nicholson
 * Portions Copyright (c) Microsoft Corporation
 *
 * All Rights Reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ***/

#ifndef UNITTEST_HELPERMACROS_H
#define UNITTEST_HELPERMACROS_H

#include "../config.h"

#define UNITTEST_MULTILINE_MACRO_BEGIN                                                                                 \
    do                                                                                                                 \
    {
#ifdef UNITTEST_WIN32
#define UNITTEST_MULTILINE_MACRO_END                                                                                   \
    }                                                                                                                  \
    __pragma(warning(push)) __pragma(warning(disable : 4127)) while (0) __pragma(warning(pop))
#else
#define UNITTEST_MULTILINE_MACRO_END                                                                                   \
    }                                                                                                                  \
    while (0)
#endif

#ifdef UNITTEST_WIN32_DLL
#define UNITTEST_IMPORT __declspec(dllimport)
#define UNITTEST_EXPORT __declspec(dllexport)

#ifdef UNITTEST_DLL_EXPORT
#define UNITTEST_LINKAGE UNITTEST_EXPORT
#define UNITTEST_IMPEXP_TEMPLATE
#else
#define UNITTEST_LINKAGE UNITTEST_IMPORT
#define UNITTEST_IMPEXP_TEMPLATE extern
#endif

#define UNITTEST_STDVECTOR_LINKAGE(T)                                                                                  \
    __pragma(warning(push)) __pragma(warning(disable : 4231))                                                          \
        UNITTEST_IMPEXP_TEMPLATE template class UNITTEST_LINKAGE std::allocator<T>;                                    \
    UNITTEST_IMPEXP_TEMPLATE template class UNITTEST_LINKAGE std::vector<T>;                                           \
    __pragma(warning(pop))
#else
#define UNITTEST_IMPORT
#define UNITTEST_EXPORT
#define UNITTEST_LINKAGE
#define UNITTEST_IMPEXP_TEMPLATE
#define UNITTEST_STDVECTOR_LINKAGE(T)
#endif

#ifdef UNITTEST_WIN32
#define UNITTEST_JMPBUF jmp_buf
#define UNITTEST_SETJMP setjmp
#define UNITTEST_LONGJMP longjmp
#elif defined UNITTEST_POSIX
#define UNITTEST_JMPBUF std::jmp_buf
#define UNITTEST_SETJMP setjmp
#define UNITTEST_LONGJMP std::longjmp
#endif

#endif
