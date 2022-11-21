#include "pal.h"
#include <objbase.h>

#if !XLANG_PLATFORM_WINDOWS
#error "This file is only for targeting Windows"
#endif

extern "C"
{
    void* XLANG_CALL xlang_mem_alloc(size_t count) XLANG_NOEXCEPT
    {
        return ::CoTaskMemAlloc(count);
    }

    void XLANG_CALL xlang_mem_free(void* ptr) XLANG_NOEXCEPT
    {
        return ::CoTaskMemFree(ptr);
    }
}
