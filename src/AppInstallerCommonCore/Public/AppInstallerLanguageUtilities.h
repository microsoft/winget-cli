// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace AppInstaller
{
    // A helper type that resets itself when it is moved from.
    template <typename T>
    struct ResetWhenMovedFrom
    {
        ResetWhenMovedFrom() = default;

        ResetWhenMovedFrom(T t) : m_var(t) {}

        // Not copyable
        ResetWhenMovedFrom(const ResetWhenMovedFrom&) = delete;
        ResetWhenMovedFrom& operator=(const ResetWhenMovedFrom&) = delete;

        ResetWhenMovedFrom(ResetWhenMovedFrom&& other) :
            m_var(std::move(other.m_var))
        {
            other.m_var = T{};
        }

        ResetWhenMovedFrom& operator=(ResetWhenMovedFrom&& other)
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

    // A helper to zip multiple iterators together.
    // Does not ensure anything about size, and can only be used once.
    // Ex.
    // for (auto values : ZipIterator{ { 1, 2 }, { "a", "b" } })
    template <typename... T>
    struct ZipIterator
    {
        using iterators_t = std::tuple<decltype(std::make_tuple(std::declval<T>().begin()...))>;
        using values_t = std::tuple<decltype(std::make_tuple((*((std::declval<T>()).begin()))...))>;

        struct itr_wrapper
        {
            iterators_t m_itrs;

            itr_wrapper& operator++()
            {
                op_inc_impl(std::make_integer_sequence<size_t, sizeof...(T)>{});
                return *this;
            }

            values_t operator*()
            {
                return op_star_impl(std::make_integer_sequence<size_t, sizeof...(T)>{});
            }

            bool operator!=(const itr_wrapper& other)
            {
                return op_noteq_impl(other, std::make_integer_sequence<size_t, sizeof...(T)>{});
            }

        private:
            template <typename... T>
            void ignore(T&&...) {}

            template <size_t... I>
            void op_inc_impl(std::integer_sequence<size_t, I...>)
            {
                ignore((++(std::get<I>(m_itrs)))...);
            }

            template <size_t... I>
            values_t op_star_impl(std::integer_sequence<size_t, I...>)
            {
                std::make_tuple((*(std::get<I>(m_itrs)))...);
            }

            template <size_t... I>
            bool op_noteq_impl(const itr_wrapper& other, std::integer_sequence<size_t, I...>)
            {
                return (((std::get<I>(m_itrs)) != (std::get<I>(other.m_itrs))) && ...);
            }
        };

        ZipIterator(const T&... iterables) :
            m_iterators(std::make_tuple(iterables.begin()...)),
            m_end(std::make_tuple(iterables.end()...))
        {}

        itr_wrapper begin() { return { m_iterators }; }
        itr_wrapper end() { return { m_end }; }

    private:
        iterators_t m_iterators;
        iterators_t m_end;
    };
}
