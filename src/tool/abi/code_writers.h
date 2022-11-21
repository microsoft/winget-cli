#pragma once

#include <optional>

#include "abi_writer.h"
#include "types.h"

inline std::string_view mangled_name_macro_format(writer& w)
{
    using namespace std::literals;
    switch (w.config().ns_prefix_state)
    {
    case ns_prefix::always:
        return "__x_ABI_C%"sv;

    default:
        return "__x_%"sv;
    }
}

inline std::string_view cpp_typename_format(writer& w)
{
    using namespace std::literals;
    switch (w.config().ns_prefix_state)
    {
    case ns_prefix::always:
        return "ABI::%"sv;

    case ns_prefix::optional:
        return "ABI_PARAMETER(%)"sv;

    default:
        return "%"sv;
    }
}

inline std::string_view c_typename_format(writer& w)
{
    using namespace std::literals;
    switch (w.config().ns_prefix_state)
    {
    case ns_prefix::always:
        return "__x_ABI_C%"sv;

    case ns_prefix::optional:
        return "C_ABI_PARAMETER(%)"sv;

    default:
        return "__x_%"sv;
    }
}

inline void write_cpp_fully_qualified_type(writer& w, std::string_view typeNamespace, std::string_view typeName)
{
    w.write(cpp_typename_format(w), [&](writer& w) { w.write("@::%", typeNamespace, typeName); });
}

// NOTE: xlang::bind captures arguments by reference in the lambda that it returns, however since all of these bind_*
//       helpers are intermediate functions, we can't have it capturing references to our stack arguments, hence the
//       reason all of these arguments are captured by reference
inline auto bind_cpp_fully_qualified_type(std::string_view const& typeNamespace, std::string_view const& typeName)
{
    return xlang::text::bind<write_cpp_fully_qualified_type>(typeNamespace, typeName);
}

inline void write_mangled_name_macro(writer& w, metadata_type const& type)
{
    w.write(mangled_name_macro_format(w), type.mangled_name());
}

inline void write_mangled_name_macro(writer& w, generic_inst const& type)
{
    w.write(type.mangled_name());
}

template <typename T>
inline auto bind_mangled_name_macro(T const& type)
{
    return [&](writer& w)
    {
        write_mangled_name_macro(w, type);
    };
}

template <typename T>
inline void write_iid_name(writer& w, T const& type)
{
    using namespace xlang::text;
    if (w.config().ns_prefix_state == ns_prefix::optional)
    {
        w.write("C_IID(%)", type.mangled_name());
    }
    else
    {
        w.write("IID_%", bind_mangled_name_macro(type));
    }
}

template <typename T>
inline auto bind_iid_name(T const& type)
{
    return [&](writer& w)
    {
        write_iid_name(w, type);
    };
}

template <typename T>
inline void write_c_type_name(writer& w, T const& type)
{
    write_c_type_name(w, type, "");
}

template <typename Suffix>
inline void write_c_type_name(writer& w, typedef_base const& type, Suffix&& suffix)
{
    w.write(c_typename_format(w), [&](writer& w) { w.write("%%", type.mangled_name(), suffix); });
}

template <typename Suffix>
inline void write_c_type_name(writer& w, generic_inst const& type, Suffix&& suffix)
{
    w.write("%%", type.mangled_name(), suffix);
}

template <typename T>
auto bind_c_type_name(T const& type)
{
    return [&](writer& w)
    {
        write_c_type_name(w, type);
    };
}

template <typename T, typename Suffix>
auto bind_c_type_name(T const& type, Suffix&& suffix)
{
    return [&](writer& w)
    {
        write_c_type_name(w, type, suffix);
    };
}

inline void write_contract_macro(writer& w, std::string_view contractNamespace, std::string_view contractTypeName)
{
    using namespace xlang::text;
    w.write("%_%_VERSION",
        bind_list<writer::write_uppercase>("_", namespace_range{ contractNamespace }),
        bind<writer::write_uppercase>(contractTypeName));
}

inline void write_deprecation_message(
    writer& w,
    deprecation_info const& info,
    std::size_t additionalIndentation = 0,
    std::string_view deprecationMacro = "DEPRECATED")
{
    using namespace xlang::text;
    XLANG_ASSERT(w.config().enable_header_deprecation);

    auto [ns, name] = decompose_type(info.contract_type);
    w.write(R"^-^(#if % >= %
%%("%")
#endif // % >= %
)^-^",
        bind<write_contract_macro>(ns, name), format_hex{ info.version },
        indent{ additionalIndentation }, deprecationMacro, info.message,
        bind<write_contract_macro>(ns, name), format_hex{ info.version });
}

inline void write_uuid(writer& w, xlang::meta::reader::TypeDef const& type)
{
    auto iidStr = type_iid(type);
    w.write(std::string_view{ iidStr.data(), iidStr.size() - 1 });
}

inline void write_uuid(writer& w, typedef_base const& type)
{
    write_uuid(w, type.type());
}

inline void write_generated_uuid(writer& w, metadata_type const& type)
{
    sha1 signatureHash;
    static constexpr std::uint8_t namespaceGuidBytes[] =
    {
        0x11, 0xf4, 0x7a, 0xd5,
        0x7b, 0x73,
        0x42, 0xc0,
        0xab, 0xae, 0x87, 0x8b, 0x1e, 0x16, 0xad, 0xee
    };
    signatureHash.append(namespaceGuidBytes, std::size(namespaceGuidBytes));
    type.append_signature(signatureHash);

    auto iidHash = signatureHash.finalize();
    iidHash[6] = (iidHash[6] & 0x0F) | 0x50;
    iidHash[8] = (iidHash[8] & 0x3F) | 0x80;
    w.write_printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        iidHash[0], iidHash[1], iidHash[2], iidHash[3],
        iidHash[4], iidHash[5],
        iidHash[6], iidHash[7],
        iidHash[8], iidHash[9],
        iidHash[10], iidHash[11], iidHash[12], iidHash[13], iidHash[14], iidHash[15]);
}

inline void write_uuid(writer& w, generic_inst const& type)
{
    write_generated_uuid(w, type);
}

template <typename T>
auto bind_uuid(T const& type)
{
    return [&](writer& w)
    {
        write_uuid(w, type);
    };
}
