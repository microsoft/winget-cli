#pragma once

#include <cctype>
#include <string_view>
#include <utility>

#include "meta_reader.h"
#include "namespace_iterator.h"

constexpr std::string_view system_namespace = "System";
constexpr std::string_view foundation_namespace = "Windows.Foundation";
constexpr std::string_view collections_namespace = "Windows.Foundation.Collections";
constexpr std::string_view metadata_namespace = "Windows.Foundation.Metadata";

enum class ns_prefix
{
    always,
    never,
    optional,
};

struct abi_configuration
{
    bool verbose = false;
    ns_prefix ns_prefix_state = ns_prefix::always;
    bool enum_class = false;
    bool lowercase_include_guard = false;
    bool enable_header_deprecation = false;

    std::string output_directory;
};

namespace xlang
{
    template <typename... T> visit_overload(T...) -> visit_overload<T...>;
}

template <typename Visitor>
inline auto visit(xlang::meta::reader::coded_index<xlang::meta::reader::TypeDefOrRef> const& type, Visitor&& visitor)
{
    using namespace xlang::meta::reader;
    switch (type.type())
    {
    case TypeDefOrRef::TypeDef: return visitor(type.TypeDef()); break;
    case TypeDefOrRef::TypeRef: return visitor(type.TypeRef()); break;
    case TypeDefOrRef::TypeSpec: return visitor(type.TypeSpec().Signature().GenericTypeInst()); break;
    }
}

inline constexpr std::pair<std::string_view, std::string_view> decompose_type(std::string_view typeName) noexcept
{
    auto pos = typeName.find('<');
    pos = typeName.rfind('.', pos);
    if (pos == std::string_view::npos)
    {
        // No namespace
        XLANG_ASSERT(false);
        return { std::string_view{}, typeName };
    }

    return { typeName.substr(0, pos), typeName.substr(pos + 1) };
}

inline bool is_generic(xlang::meta::reader::TypeDef const& type) noexcept
{
    return distance(type.GenericParam()) != 0;
}

inline xlang::meta::reader::ElementType underlying_enum_type(xlang::meta::reader::TypeDef const& type)
{
    return std::get<xlang::meta::reader::ElementType>(type.FieldList().first.Signature().Type().Type());
}

inline bool is_default(xlang::meta::reader::InterfaceImpl const& ifaceImpl)
{
    using namespace std::literals;
    return static_cast<bool>(get_attribute(ifaceImpl, metadata_namespace, "DefaultAttribute"sv));
}

inline xlang::meta::reader::coded_index<xlang::meta::reader::TypeDefOrRef> try_get_default_interface(xlang::meta::reader::TypeDef const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;
    XLANG_ASSERT(get_category(type) == category::class_type);

    for (auto const& iface : type.InterfaceImpl())
    {
        if (is_default(iface))
        {
            return iface.Interface();
        }
    }

    return {};
}

inline xlang::meta::reader::TypeDef try_get_base(xlang::meta::reader::TypeDef const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;

    // This could be System.Object, in which case we want to ignore
    auto extends = type.Extends();
    if (!extends)
    {
        return {};
    }
    else if (extends.type() != TypeDefOrRef::TypeRef)
    {
        return {};
    }

    auto ref = extends.TypeRef();
    if ((ref.TypeNamespace() == "System"sv) && (ref.TypeName() == "Object"sv))
    {
        return {};
    }

    return find_required(ref);
}

template <typename T, typename Func>
inline void for_each_attribute(
    T const& type,
    std::string_view namespaceFilter,
    std::string_view typeNameFilter,
    Func&& func)
{
    bool first = true;
    for (auto const& attr : type.CustomAttribute())
    {
        auto [ns, name] = attr.TypeNamespaceAndName();
        if ((ns == namespaceFilter) && (name == typeNameFilter))
        {
            func(first, attr);
            first = false;
        }
    }
}

namespace details
{
    template <typename T, typename... Types>
    struct variant_index;

    template <typename T, typename First, typename... Types>
    struct variant_index<T, First, Types...>
    {
        static constexpr bool found = std::is_same_v<T, First>;
        static constexpr std::size_t value = std::conditional_t<found,
            std::integral_constant<std::size_t, 0>,
            variant_index<T, Types...>>::value + (found ? 0 : 1);
    };
}

template <typename Variant, typename T>
struct variant_index;

template <typename... Types, typename T>
struct variant_index<std::variant<Types...>, T> : details::variant_index<T, Types...>
{
};

template <typename Variant, typename T>
constexpr std::size_t variant_index_v = variant_index<Variant, T>::value;

// Constructor arguments are not consistently encoded using the same type (e.g. using a signed 32-bit integer as an
// argument to a constructor that takes an unsigned 32-bit integer)
template <typename T, typename Variant>
T decode_integer(Variant const& value)
{
    switch (value.index())
    {
    case variant_index_v<Variant, std::int8_t>: return static_cast<T>(std::get<std::int8_t>(value));
    case variant_index_v<Variant, std::uint8_t>: return static_cast<T>(std::get<std::uint8_t>(value));
    case variant_index_v<Variant, std::int16_t>: return static_cast<T>(std::get<std::int16_t>(value));
    case variant_index_v<Variant, std::uint16_t>: return static_cast<T>(std::get<std::uint16_t>(value));
    case variant_index_v<Variant, std::int32_t>: return static_cast<T>(std::get<std::int32_t>(value));
    case variant_index_v<Variant, std::uint32_t>: return static_cast<T>(std::get<std::uint32_t>(value));
    case variant_index_v<Variant, std::int64_t>: return static_cast<T>(std::get<std::int64_t>(value));
    case variant_index_v<Variant, std::uint64_t>: return static_cast<T>(std::get<std::uint64_t>(value));
    case variant_index_v<Variant, char16_t>: return static_cast<T>(std::get<char16_t>(value));
    default: return std::get<T>(value); // This should throw, but that's intentional
    }
}

template <typename T>
struct type_identity
{
    using type = T;
};

template <typename T>
T decode_enum(xlang::meta::reader::ElemSig::EnumValue const& value)
{
    using integral_type = typename std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, type_identity<T>>::type;
    return static_cast<T>(decode_integer<integral_type>(value.value));
}

struct deprecation_info
{
    std::string_view contract_type;
    std::uint32_t version;

    std::string_view message;
};

template <typename T>
inline std::optional<deprecation_info> is_deprecated(T const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;

    auto attr = get_attribute(type, metadata_namespace, "DeprecatedAttribute"sv);
    if (!attr)
    {
        return std::nullopt;
    }

    auto sig = attr.Value();
    auto const& fixedArgs = sig.FixedArgs();

    // There are three DeprecatedAttribute constructors, two of which deal with version numbers which we don't care
    // about here. The third is relative to a contract version, which we _do_ care about
    if ((fixedArgs.size() != 4))
    {
        return std::nullopt;
    }

    auto const& contractSig = std::get<ElemSig>(fixedArgs[3].value);
    if (!std::holds_alternative<std::string_view>(contractSig.value))
    {
        return std::nullopt;
    }

    return deprecation_info
    {
        std::get<std::string_view>(contractSig.value),
        std::get<std::uint32_t>(std::get<ElemSig>(fixedArgs[2].value).value),
        std::get<std::string_view>(std::get<ElemSig>(fixedArgs[0].value).value)
    };
}

inline bool is_flags_enum(xlang::meta::reader::TypeDef const& type)
{
    using namespace std::literals;
    return static_cast<bool>(get_attribute(type, system_namespace, "FlagsAttribute"sv));
}

template <typename T>
bool is_experimental(T const& value)
{
    using namespace std::literals;
    return static_cast<bool>(get_attribute(value, metadata_namespace, "ExperimentalAttribute"sv));
}

inline bool is_overridable(xlang::meta::reader::InterfaceImpl const& iface)
{
    using namespace std::literals;
    return static_cast<bool>(get_attribute(iface, metadata_namespace, "OverridableAttribute"sv));
}

inline bool is_exclusiveto(xlang::meta::reader::TypeDef const& iface)
{
    using namespace std::literals;
    return static_cast<bool>(get_attribute(iface, metadata_namespace, "ExclusiveToAttribute"sv));
}

template <typename T>
bool is_enabled(T const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;
    if (auto attr = get_attribute(type, metadata_namespace, "FeatureAttribute"sv))
    {
        // There is a single constructor: .ctor(FeatureStage, bool), however we only care about the 'FeatureStage' value
        // since our behavior
        auto sig = attr.Value();
        auto const& args = sig.FixedArgs();
        XLANG_ASSERT(args.size() == 2);

        auto stage = decode_enum<std::int32_t>(std::get<ElemSig::EnumValue>(std::get<ElemSig>(args[0].value).value));
        return stage >= 2;
    }

    return true;
}

// NOTE: 37 characters for the null terminator; the actual string is 36 characters
inline std::array<char, 37> type_iid(xlang::meta::reader::TypeDef const& type)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;

    std::array<char, 37> result;

    auto attr = get_attribute(type, metadata_namespace, "GuidAttribute"sv);
    if (!attr)
    {
        xlang::throw_invalid("'Windows.Foundation.Metadata.GuidAttribute' attribute for type '", type.TypeNamespace(),
            ".", type.TypeName(), "' not found");
    }

    auto value = attr.Value();
    auto const& args = value.FixedArgs();
    // 966BE0A7-B765-451B-AAAB-C9C498ED2594
    std::snprintf(result.data(), result.size(), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        std::get<uint32_t>(std::get<ElemSig>(args[0].value).value),
        std::get<uint16_t>(std::get<ElemSig>(args[1].value).value),
        std::get<uint16_t>(std::get<ElemSig>(args[2].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[3].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[4].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[5].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[6].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[7].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[8].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[9].value).value),
        std::get<uint8_t>(std::get<ElemSig>(args[10].value).value));

    return result;
}
