#include "pch.h"
#include <xlang/Foundation.Collections.h>
#include "catch.hpp"

using namespace xlang;
using namespace Foundation;
using namespace Foundation::Collections;

namespace
{
    template <typename A, typename B>
    void test_sequence(A const& container, std::initializer_list<B> expected)
    {
        bool const equal = std::equal(begin(container), end(container), begin(expected), end(expected));
        REQUIRE(equal);
    }

    template <typename A, typename B>
    void test_associative(A const& container, std::initializer_list<B> expected)
    {
        bool const equal = std::equal(begin(container), end(container), begin(expected), end(expected),
            [](auto&& left, auto&& right)
        {
            return left.Key() == right.first && left.Value() == right.second;
        });

        REQUIRE(equal);
    }

    struct simple_iterable :
        implements<simple_iterable, IIterable<int>>,
        iterable_base<simple_iterable, int>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        std::vector<int> m_values{ 1,2,3 };
    };

    struct simple_iterable_with_custom_container :
        implements<simple_iterable_with_custom_container, IIterable<int>>,
        iterable_base<simple_iterable_with_custom_container, int>
    {
        auto get_container() const noexcept
        {
            struct container
            {
                std::vector<int>::const_iterator first;
                std::vector<int>::const_iterator last;

                auto begin() const
                {
                    return first;
                }

                auto end() const
                {
                    return last;
                }
            };

            return container{ m_values.begin(), m_values.end() };
        }

        std::vector<int> m_values{ 1,2,3 };
    };

    struct simple_vector_view :
        implements<simple_vector_view, IVectorView<int>, IIterable<int>>,
        vector_view_base<simple_vector_view, int>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        std::vector<int> m_values{ 1,2,3 };
    };

    struct simple_vector_view_with_custom_container :
        implements<simple_vector_view_with_custom_container, IVectorView<int>, IIterable<int>>,
        vector_view_base<simple_vector_view_with_custom_container, int>
    {
        auto get_container() const noexcept
        {
            struct container
            {
                std::vector<int>::const_iterator first;
                std::vector<int>::const_iterator last;

                auto begin() const
                {
                    return first;
                }

                auto end() const
                {
                    return last;
                }
            };

            return container{ m_values.begin(), m_values.end() };
        }

        std::vector<int> m_values{ 1,2,3 };
    };

    struct simple_vector :
        implements<simple_vector, IVector<int>, IVectorView<int>, IIterable<int>>,
        vector_base<simple_vector, int>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        auto& get_container() noexcept
        {
            return m_values;
        }

        std::vector<int> m_values{ 1,2,3 };
    };

    struct simple_iterable_pair :
        implements<simple_iterable_pair, IIterable<IKeyValuePair<int, hstring>>>,
        iterable_base<simple_iterable_pair, IKeyValuePair<int, hstring>>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        std::map<int, hstring> m_values{ {1,u8"one"}, {2, u8"two"}, {3, u8"three"} };
    };

    struct simple_map_view :
        implements<simple_map_view, IMapView<int, hstring>, IIterable<IKeyValuePair<int, hstring>>>,
        map_view_base<simple_map_view, int, hstring>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        std::map<int, hstring> m_values{ { 1,u8"one" },{ 2, u8"two" },{ 3, u8"three" } };
    };

    struct simple_map :
        implements<simple_map, IMap<int, hstring>, IMapView<int, hstring>, IIterable<IKeyValuePair<int, hstring>>>,
        map_base<simple_map, int, hstring>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        auto& get_container() noexcept
        {
            return m_values;
        }

        std::map<int, hstring> m_values{ { 1,u8"one" },{ 2, u8"two" },{ 3, u8"three" } };
    };
}

TEST_CASE("simple_containers")
{
    std::initializer_list<int> sequence{ 1,2,3 };
    std::initializer_list<std::pair<int, hstring>> associative{ { 1,u8"one" },{ 2, u8"two" },{ 3, u8"three" } };

    test_sequence(make<simple_iterable>(), sequence);
    test_sequence(make<simple_iterable_with_custom_container>(), sequence);
    test_sequence(make<simple_vector_view>(), sequence);
    test_sequence(make<simple_vector_view_with_custom_container>(), sequence);
    test_sequence(make<simple_vector>(), sequence);

    test_associative(make<simple_iterable_pair>(), associative);
    test_associative(make<simple_map_view>(), associative);
    test_associative(make<simple_map>(), associative);
}

