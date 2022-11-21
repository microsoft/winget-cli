
#ifdef _DEBUG

#define XLANG_ASSERT assert
#define XLANG_VERIFY XLANG_ASSERT
#define XLANG_VERIFY_(result, expression) XLANG_ASSERT(result == expression)

#else

#define XLANG_ASSERT(expression) ((void)0)
#define XLANG_VERIFY(expression) (void)(expression)
#define XLANG_VERIFY_(result, expression) (void)(expression)

#endif

#if defined(_MSC_VER)
#define XLANG_EBO __declspec(empty_bases)
#define XLANG_NOVTABLE __declspec(novtable)
#define XLANG_CALL __stdcall
#define XLANG_NOINLINE  __declspec(noinline)
#else
#define XLANG_EBO
#define XLANG_NOVTABLE
#define XLANG_CALL
#define XLANG_NOINLINE
#endif

#define XLANG_SHIM(...) (*(abi_t<__VA_ARGS__>**)&static_cast<__VA_ARGS__ const&>(static_cast<D const&>(*this)))

#ifndef XLANG_EXTERNAL_CATCH_CLAUSE
#define XLANG_EXTERNAL_CATCH_CLAUSE
#endif

#if defined(_MSC_VER)
#ifdef _M_HYBRID
#define XLANG_LINK(function, count) __pragma(comment(linker, "/alternatename:#XLANG_" #function "@" #count "=#" #function "@" #count))
#elif _M_IX86
#define XLANG_LINK(function, count) __pragma(comment(linker, "/alternatename:_XLANG_" #function "@" #count "=_" #function "@" #count))
#else
#define XLANG_LINK(function, count) __pragma(comment(linker, "/alternatename:XLANG_" #function "=" #function))
#endif
#else
#define XLANG_LINK(function, count)
#endif

// Note: this is a workaround for a false-positive warning produced by the Visual C++ 15.9 compiler.
#pragma warning(disable : 5046)
