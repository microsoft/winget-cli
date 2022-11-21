#pragma once

#include "abi_writer.h"
#include "code_writers.h"
#include "types.h"

inline void write_contract_version(writer& w, unsigned int value)
{
    auto versionHigh = static_cast<int>((value & 0xFFFF0000) >> 16);
    auto versionLow = static_cast<int>(value & 0x0000FFFF);
    w.write("%.%", versionHigh, versionLow);
}

inline void write_type_banner_version_info(writer& w, xlang::meta::reader::TypeDef const& type)
{
    if (auto contractInfo = get_contract_history(type))
    {
        w.write(R"^-^( *
 * Introduced to % in version %
)^-^",
            contractInfo->current_contract.type_name,
            xlang::text::bind<write_contract_version>(contractInfo->current_contract.version));
    }

    if (is_experimental(type))
    {
        w.write(R"^-^( *
 * Type is for evaluation purposes and is subject to change or removal in future updates.
)^-^");
    }
}

inline void write_type_banner(writer& w, enum_type const& type)
{
    // NOTE: MIDL writes 'Struct' for enums, so we will, too
    w.write(R"^-^(/*
 *
 * Struct %
)^-^", type.clr_full_name());

    write_type_banner_version_info(w, type.type());

    w.write(R"^-^( *
 */
)^-^");
}

inline void write_type_banner(writer& w, struct_type const& type)
{
    w.write(R"^-^(/*
 *
 * Struct %
)^-^", type.clr_full_name());

    write_type_banner_version_info(w, type.type());

    w.write(R"^-^( *
 */
)^-^");
}

inline void write_type_banner(writer& w, delegate_type const& type)
{
    w.write(R"^-^(/*
 *
 * Delegate %
)^-^", type.clr_full_name());

    write_type_banner_version_info(w, type.type());

    w.write(R"^-^( *
 */
)^-^");
}

inline void write_type_banner(writer& w, interface_type const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;

    w.write(R"^-^(/*
 *
 * Interface %
)^-^", type.clr_full_name());

    write_type_banner_version_info(w, type.type());

    if (auto exclusiveAttr = get_attribute(type.type(), metadata_namespace, "ExclusiveToAttribute"sv))
    {
        auto sig = exclusiveAttr.Value();
        auto const& fixedArgs = sig.FixedArgs();
        XLANG_ASSERT(fixedArgs.size() == 1);
        auto sysType = std::get<ElemSig::SystemType>(std::get<ElemSig>(fixedArgs[0].value).value);

        w.write(R"^-^( *
 * Interface is a part of the implementation of type %
)^-^", sysType.name);
    }

    if (!type.required_interfaces.empty())
    {
        w.write(R"^-^( *
 * Any object which implements this interface must also implement the following interfaces:
)^-^");

        for (auto iface : type.required_interfaces)
        {
            w.write(" *     %\n", iface->clr_full_name());
        }
    }

    w.write(R"^-^( *
 */
)^-^");
}

inline void write_type_banner(writer& w, class_type const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;
    using namespace xlang::text;

    w.write(R"^-^(/*
 *
 * Class %
)^-^", type.clr_full_name());

    write_type_banner_version_info(w, type.type());

    for_each_attribute(type.type(), metadata_namespace, "ActivatableAttribute"sv, [&](bool first, CustomAttribute const& attr)
    {
        if (first)
        {
            w.write(" *\n * RuntimeClass can be activated.\n");
        }

        // There are 6 constructors for the ActivatableAttribute; we only care about the two that give us contracts
        auto sig = attr.Value();
        auto const& fixedArgs = sig.FixedArgs();
        if (fixedArgs.size() == 2)
        {
            auto const& elem0 = std::get<ElemSig>(fixedArgs[0].value);
            auto const& elem1 = std::get<ElemSig>(fixedArgs[1].value);
            if (!std::holds_alternative<std::uint32_t>(elem0.value) ||
                !std::holds_alternative<std::string_view>(elem1.value))
            {
                return;
            }

            w.write(" *   Type can be activated via RoActivateInstance starting with version % of the % API contract\n",
                bind<write_contract_version>(std::get<std::uint32_t>(elem0.value)),
                std::get<std::string_view>(elem1.value));
        }
        else if (fixedArgs.size() == 3)
        {
            auto const& elem0 = std::get<ElemSig>(fixedArgs[0].value);
            auto const& elem1 = std::get<ElemSig>(fixedArgs[1].value);
            auto const& elem2 = std::get<ElemSig>(fixedArgs[2].value);
            if (!std::holds_alternative<ElemSig::SystemType>(elem0.value) ||
                !std::holds_alternative<std::uint32_t>(elem1.value) ||
                !std::holds_alternative<std::string_view>(elem2.value))
            {
                return;
            }

            w.write(" *   Type can be activated via the % interface starting with version % of the % API contract\n",
                std::get<ElemSig::SystemType>(elem0.value).name,
                bind<write_contract_version>(std::get<std::uint32_t>(elem1.value)),
                std::get<std::string_view>(elem2.value));
        }
    });

    for_each_attribute(type.type(), metadata_namespace, "StaticAttribute"sv, [&](bool first, CustomAttribute const& attr)
    {
        if (first)
        {
            w.write(" *\n * RuntimeClass contains static methods.\n");
        }

        // There are 3 constructors for the ActivatableAttribute; we only care about one
        auto sig = attr.Value();
        auto const& fixedArgs = sig.FixedArgs();
        if (fixedArgs.size() != 3)
        {
            return;
        }

        auto const& contractElem = std::get<ElemSig>(fixedArgs[2].value);
        if (!std::holds_alternative<std::string_view>(contractElem.value))
        {
            return;
        }

        w.write(" *   Static Methods exist on the % interface starting with version % of the % API contract\n",
            std::get<ElemSig::SystemType>(std::get<ElemSig>(fixedArgs[0].value).value).name,
            bind<write_contract_version>(std::get<std::uint32_t>(std::get<ElemSig>(fixedArgs[1].value).value)),
            std::get<std::string_view>(contractElem.value));
    });

    if (!type.required_interfaces.empty())
    {
        w.write(R"^-^( *
 * Class implements the following interfaces:
)^-^");

        for (auto iface : type.required_interfaces)
        {
            auto suffix = iface == type.default_interface ? " ** Default Interface **" : ""sv;
            w.write(" *    %%\n", iface->clr_full_name(), suffix);
        }
    }

    if (auto attr = get_attribute(type.type(), metadata_namespace, "ThreadingAttribute"sv))
    {
        // There's only one constructor for ThreadingAttribute
        auto sig = attr.Value();
        auto const& fixedArgs = sig.FixedArgs();
        XLANG_ASSERT(fixedArgs.size() == 1);

        auto const& enumValue = std::get<ElemSig::EnumValue>(std::get<ElemSig>(fixedArgs[0].value).value);

        std::string_view msg = "";
        switch (std::get<std::int32_t>(enumValue.value))
        {
        case 1: msg = "Single Threaded Apartment"sv; break;
        case 2: msg = "Multi Threaded Apartment"sv; break;
        case 3: msg = "Both Single and Multi Threaded Apartment"sv; break;
        }

        if (!msg.empty())
        {
            w.write(" *\n * Class Threading Model:  %\n", msg);
        }
    }

    if (auto attr = get_attribute(type.type(), metadata_namespace, "MarshalingBehaviorAttribute"sv))
    {
        // There's only one constructor for ThreadingAttribute
        auto sig = attr.Value();
        auto const& fixedArgs = sig.FixedArgs();
        XLANG_ASSERT(fixedArgs.size() == 1);

        auto const& enumValue = std::get<ElemSig::EnumValue>(std::get<ElemSig>(fixedArgs[0].value).value);

        std::string_view msg = "";
        switch (std::get<std::int32_t>(enumValue.value))
        {
        case 1: msg = "None - Class cannot be marshaled"sv; break;
        case 2: msg = "Agile - Class is agile"sv; break;
        case 3: msg = "Standard - Class marshals using the standard marshaler"sv; break;
        }

        if (!msg.empty())
        {
            w.write(" *\n * Class Marshaling Behavior:  %\n", msg);
        }
    }

    w.write(R"^-^( *
 */
)^-^");
}
