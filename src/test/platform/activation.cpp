#include "pch.h"

TEST_CASE("Simple activation")
{
    xlang_error_info* result{};

    // TODO: Update cpp projection to emit char16_t names
    std::u16string_view class_name{ u"AbiComponent.Widget" };
    xlang_string_header str_header{};
    xlang_string str{};

    result = xlang_create_string_reference_utf16(class_name.data(), static_cast<uint32_t>(class_name.size()), &str_header, &str);
    REQUIRE(result == nullptr);

    // TODO: use xplat com_ptr when available (GitHub issue #185)
    xlang_unknown* factory{};
    result = xlang_get_activation_factory(str, xlang_unknown_guid, reinterpret_cast<void**>(&factory));
    REQUIRE(result == nullptr);

    // TODO: Actually call an activation method once we've converged our type system.
    // For now, simply check that we got a valid object back.
    REQUIRE(factory != nullptr);

    if (factory)
    {
        factory->Release();
        factory = nullptr;
    }
}
