#include "win32_pal_internal.h"
#include "platform_activation.h"
#include "string_convert.h"
#include <memory>
#include <string>

using namespace std::string_view_literals;

namespace xlang::impl
{
    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<wchar_t> module_namespace)
    {
        HMODULE module{};

        constexpr auto file_ext{ L".dll"sv };
        if (module_namespace.size() + file_ext.size() < MAX_PATH)
        {
            wchar_t module_name[MAX_PATH];
            wchar_t* out = std::copy(module_namespace.begin(), module_namespace.end(), module_name);
            out = std::copy(file_ext.begin(), file_ext.end(), out);
            *out = L'\0';
            module = ::LoadLibraryW(module_name);
        }
        else
        {
            std::wstring module_name;
            module_name.reserve(module_namespace.size() + file_ext.size());
            module_name = module_namespace;
            module_name += file_ext;
            module = ::LoadLibraryW(module_name.data());
        }

        if (module)
        {
            return reinterpret_cast<xlang_pfn_lib_get_activation_factory>(::GetProcAddress(module, activation_fn_name.data()));
        }
        return nullptr;
    }

    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<char16_t> module_namespace)
    {
        static_assert(sizeof(char16_t) == sizeof(wchar_t));
        return try_get_activation_func(
            std::basic_string_view<wchar_t>
        {
            reinterpret_cast<wchar_t const*>(module_namespace.data()), module_namespace.size()
        });
    }

    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<xlang_char8> module_namespace)
    {
        auto const length = get_converted_length(module_namespace);
        if (length < MAX_PATH)
        {
            char16_t converted_name[MAX_PATH];
            uint32_t converted_length = convert_string(module_namespace, converted_name, MAX_PATH);
            return try_get_activation_func({ converted_name, converted_length });
        }
        else
        {
            auto converted_name = std::make_unique<char16_t[]>(length);
            uint32_t converted_length = convert_string(module_namespace, converted_name.get(), MAX_PATH);
            return try_get_activation_func({ converted_name.get(), converted_length });
        }
    }
}
