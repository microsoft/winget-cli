#include "pal_internal.h"
#include "pal_error.h"

#if !XLANG_PLATFORM_WINDOWS
#error "This file is only for targeting Windows"
#endif

#include <Windows.h>
#include <winerror.h>

namespace xlang::impl
{
    [[noreturn]] inline void throw_originated_last_error()
    {
        throw_result(xlang_result_from_hresult(HRESULT_FROM_WIN32(::GetLastError())));
    }

    template<typename T>
    void check_bool(T result)
    {
        if (!result)
        {
            throw_originated_last_error();
        }
    }
}