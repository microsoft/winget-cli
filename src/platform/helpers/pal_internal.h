#pragma once

#include <pal.h>
#include <new>
#include <algorithm>

#ifdef _DEBUG

#if XLANG_COMPILER_MSVC
#define XLANG_ASSERT _ASSERTE
#else
#include <cassert>
#define XLANG_ASSERT assert
#endif
#define XLANG_VERIFY XLANG_ASSERT
#define XLANG_VERIFY_(result, expression) XLANG_ASSERT(result == expression)

#else

#define XLANG_ASSERT(expression) ((void)0)
#define XLANG_VERIFY(expression) (void)(expression)
#define XLANG_VERIFY_(result, expression) (void)(expression)

#endif
