#include "pal_internal.h"
#include "platform_activation.h"
#include "string_convert.h"
#include "pal_error.h"
#include <string>
#include <dlfcn.h>

namespace xlang::impl
{
    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<char16_t> module_namespace)
    {
        auto const length = get_converted_length(module_namespace);
        auto converted_name = std::make_unique<xlang_char8[]>(length);
        uint32_t converted_length = convert_string(module_namespace, converted_name.get(), length);

        // Note: common convert_string is incorrectly return zero on success. Using length as a 
        //       temporary workaround. Tracked by GitHub issue #180
        return try_get_activation_func({ converted_name.get(), length });
    }

    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<xlang_char8> module_namespace)
    {
        void* module{};

        std::string module_name{};
        module_name.reserve(module_namespace.size() + 6); // 6 == len("lib") + len(".so")
        module_name += "lib";
        module_name += module_namespace;
        module_name += ".so";

        module = dlopen(module_name.c_str(), RTLD_LAZY);

        if (module)
        {
            return reinterpret_cast<xlang_pfn_lib_get_activation_factory>(dlsym(module, activation_fn_name.data()));
        }

        return nullptr;
    }
}