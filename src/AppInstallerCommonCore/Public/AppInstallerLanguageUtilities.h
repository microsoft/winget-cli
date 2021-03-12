// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/result.h>

#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace AppInstaller
{
    // A helper type that resets itself when it is moved from.
    template <typename T>
    struct ResetWhenMovedFrom
    {
        ResetWhenMovedFrom() : m_var{} {}

        ResetWhenMovedFrom(T t) : m_var{ t } {}

        // Not copyable
        ResetWhenMovedFrom(const ResetWhenMovedFrom&) = delete;
        ResetWhenMovedFrom& operator=(const ResetWhenMovedFrom&) = delete;

        ResetWhenMovedFrom(ResetWhenMovedFrom&& other) noexcept :
            m_var(std::move(other.m_var))
        {
            other.m_var = T{};
        }

        ResetWhenMovedFrom& operator=(ResetWhenMovedFrom&& other) noexcept
        {
            m_var = std::move(other.m_var);
            other.m_var = T{};
            return *this;
        }

        operator T& () { return m_var; }
        operator const T& () const { return m_var; }

    private:
        T m_var;
    };

    // Enables a bool to be used as a destruction indicator.
    using DestructionToken = ResetWhenMovedFrom<bool>;

    // Enable use of folding to execute functions across parameter packs.
    struct FoldHelper
    {
        template <typename T>
        FoldHelper& operator,(T&&) { return *this; }
    };

    // Get the integral value for an enum.
    template <typename E>
    constexpr inline std::enable_if_t<std::is_enum_v<E>, std::underlying_type_t<E>> ToIntegral(E e)
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    // Get the enum value for an integral.
    template <typename E>
    constexpr inline std::enable_if_t<std::is_enum_v<E>, E> ToEnum(std::underlying_type_t<E> ut)
    {
        return static_cast<E>(ut);
    }

    // Enum based variant helper.
    // Enum must be an enum whose first member has the value 0, each subsequent member increases by 1, and the final member is named Max.
    // Mapping is a template type that takes one template parameter of type Enum, and whose members define value_t as the type for that enum value.
    template <typename Enum, template<Enum> typename Mapping>
    struct EnumBasedVariant
    {
    private:
        // Used to deduce the variant type; making a variant that includes std::monostate and all Mapping types.
        template <size_t... I>
        static inline auto Deduce(std::index_sequence<I...>) { return std::variant<std::monostate, typename Mapping<static_cast<Enum>(I)>::value_t...>{}; }

    public:
        // Holds data of any type listed in Mapping.
        using variant_t = decltype(Deduce(std::make_index_sequence<static_cast<size_t>(Enum::Max)>()));

        // Gets the index into the variant for the given Data.
        static constexpr inline size_t Index(Enum e) { return static_cast<size_t>(e) + 1; }
    };

    // Provides a map of the Enum to the mapped types.
    template <typename Enum, template<Enum> typename Mapping>
    struct EnumBasedVariantMap
    {
        using Variant = EnumBasedVariant<Enum, Mapping>;

        template <Enum E>
        using mapping_t = typename Mapping<E>::value_t;

        // Adds a value to the map, or overwrites an existing entry.
        // This must be used to create the initial data entry, but Get can be used to modify.
        template <Enum E>
        void Add(mapping_t<E>&& v)
        {
            m_data[E].emplace<Variant::Index(E)>(std::move(std::forward<mapping_t<E>>(v)));
        }

        template <Enum E>
        void Add(const mapping_t<E>& v)
        {
            m_data[E].emplace<Variant::Index(E)>(v);
        }

        // Return a value indicating whether the given enum is stored in the map.
        bool Contains(Enum e) const { return (m_data.find(e) != m_data.end()); }

        // Gets the value.
        template <Enum E>
        mapping_t<E>& Get()
        {
            return std::get<Variant::Index(E)>(GetVariant(E));
        }

        template <Enum E>
        const mapping_t<E>& Get() const
        {
            return std::get<Variant::Index(E)>(GetVariant(E));
        }

    private:
        typename Variant::variant_t& GetVariant(Enum e)
        {
            auto itr = m_data.find(e);
            THROW_HR_IF_MSG(E_NOT_SET, itr == m_data.end(), "GetVariant(%d)", e);
            return itr->second;
        }

        const typename Variant::variant_t& GetVariant(Enum e) const
        {
            auto itr = m_data.find(e);
            THROW_HR_IF_MSG(E_NOT_SET, itr == m_data.cend(), "GetVariant(%d)", e);
            return itr->second;
        }

        std::map<Enum, typename Variant::variant_t> m_data;
    };
}

// Enable enums to be output generically (as their integral value).
template <typename E>
std::enable_if_t<std::is_enum_v<E>, std::ostream&> operator<<(std::ostream& out, E e)
{
    return out << AppInstaller::ToIntegral(e);
}
