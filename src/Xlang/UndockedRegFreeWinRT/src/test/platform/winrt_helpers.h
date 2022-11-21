#pragma once

#include <pal.h>
#include <winrt/base.h>

// TODO: Unify C++/WinRT guid and xlang guid
inline xlang_guid to_guid(winrt::guid const& iid) noexcept
{
    static_assert(sizeof(xlang_guid) == sizeof(winrt::guid));
    xlang_guid result;
    std::memcpy(&result, &iid, sizeof(xlang_guid));
    return result;
}