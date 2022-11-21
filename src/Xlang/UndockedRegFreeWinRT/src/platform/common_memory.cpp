#include <stdlib.h>
#include "pal.h"

#ifdef _WIN32
#error "This file is for targeting platforms other than Windows"
#endif

extern "C"
{
    void* XLANG_CALL xlang_mem_alloc(size_t count) noexcept
    {
        if (count == 0)
        {
            count = 1;
        }
        return ::malloc(count);
    }

    void XLANG_CALL xlang_mem_free(void* ptr) noexcept
    {
        ::free(ptr);
    }
}
