#pragma once

#include "pal_internal.h"
#include <xlang/base.h>

namespace xlang
{
    [[noreturn]] inline void throw_result(xlang_result result, xlang_char8 const* const message = nullptr)
    {
        hstring error_message = to_hstring(message);
        throw xlang_originate_error(result, get_abi(error_message));
    }

    [[noreturn]] inline void throw_result(xlang_error_info* result)
    {
        throw result;
    }

    // TODO(defaultryan) Replace with xlang projection methods
    inline xlang_error_info* to_result() noexcept
    {
        try
        {
            throw;
        }
        catch (xlang_error_info* e)
        {
            return e;
        }
        catch (std::bad_alloc const&)
        {
            return xlang_originate_error(xlang_result::out_of_memory);
        }
        catch (...)
        {
            std::terminate();
        }
    }
}