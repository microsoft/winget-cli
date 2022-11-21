#include "pal_internal.h"
#include "opaque_string_wrapper.h"
#include "platform_activation.h"
#include "pal_error.h"

namespace xlang::impl
{
    template <typename char_type>
    xlang_error_info* get_activation_factory(
        xlang_string class_name,
        xlang_guid const& iid,
        void** factory)
    {
        for (auto current_namespace = enclosing_namespace(to_string_view<char_type>(class_name));
            !current_namespace.empty();
            current_namespace = enclosing_namespace(current_namespace))
        {
            xlang_pfn_lib_get_activation_factory pfn = try_get_activation_func(current_namespace);
            if (pfn)
            {
                xlang_result result = (*pfn)(class_name, iid, factory);
                if (result == xlang_result::success)
                {
                    return nullptr;
                }
                else if (result != xlang_result::type_load)
                {
                    throw_result(result);
                }
            }
        }
        return xlang_originate_error(xlang_result::type_load);
    }
}

using namespace xlang::impl;

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_get_activation_factory(
    xlang_string class_name,
    xlang_guid const& iid,
    void** factory
) noexcept
try
{
    if (!class_name)
    {
        xlang::throw_result(xlang_result::invalid_arg);
    }

    auto const encoding = xlang_get_string_encoding(class_name);
    if (encoding == (xlang_string_encoding::utf8 | xlang_string_encoding::utf16))
    {
        return get_activation_factory<filesystem_char_type>(class_name, iid, factory);
    }

    if (encoding == xlang_string_encoding::utf8)
    {
        return get_activation_factory<xlang_char8>(class_name, iid, factory);
    }

    return get_activation_factory<char16_t>(class_name, iid, factory);

}
catch (...)
{
    *factory = nullptr;
    return xlang::to_result();
}