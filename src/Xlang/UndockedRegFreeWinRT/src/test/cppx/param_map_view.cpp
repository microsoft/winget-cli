#include "pch.h"
#include "catch.hpp"

using namespace xlang;
using namespace Foundation::Collections;

namespace
{
    template <typename T>
    auto make_copy(T const & param)
    {
        typename T::interface_type copy;
        copy_from_abi(copy, get_abi(param));
        return copy;
    }

    void test_empty_map_view(param::map_view<int, int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values.Size() == 0);
    }

    void test_null_map_view(param::map_view<int, int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values == nullptr);
    }

    IMapView<int, int> test_map_view_scope(param::map_view<int, int> const& param)
    {
        return make_copy(param);
    }

    void test_map_view(param::map_view<int, int> const& param)
    {
        IMapView<int, int> values = make_copy(param);

        REQUIRE(3 == values.Size());

        REQUIRE(10 == values.Lookup(1));
        REQUIRE(20 == values.Lookup(2));
        REQUIRE(30 == values.Lookup(3));
        REQUIRE_THROWS_AS(values.Lookup(4), out_of_bounds_error);

        REQUIRE(10 == values.TryLookup(1).value());
        REQUIRE(20 == values.TryLookup(2).value());
        REQUIRE(30 == values.TryLookup(3).value());
        REQUIRE(!values.TryLookup(4));

        REQUIRE(values.HasKey(1));
        REQUIRE(values.HasKey(2));
        REQUIRE(values.HasKey(3));
        REQUIRE(!values.HasKey(4));

        IMapView<int, int> left, right;
        values.Split(left, right);
        REQUIRE(left == nullptr);
        REQUIRE(right == nullptr);
    }

    struct viewable
    {
        IMapView<int, int> view{ single_threaded_map<int, int>(std::map<int, int>{ { 1,10 },{ 2,20 },{ 3,30 } }).GetView() };

        operator IMapView<int, int>() const
        {
            return view;
        }
    };
}

TEST_CASE("test_map_view")
{
    test_empty_map_view({});
    test_null_map_view(nullptr);

    // initializer_list
    test_map_view({ { 1,10 },{ 2,20 },{ 3,30 } });

    // std::map/unordered_map rvalue
    test_map_view(std::map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });
    test_map_view(std::unordered_map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });

    // std::map/unordered_map lvalue
    std::map<int, int> local_map{ { 1, 10 },{ 2,20 },{ 3,30 } };
    test_map_view(local_map);
    std::unordered_map<int, int> local_unordered_map{ { 1, 10 },{ 2,20 },{ 3,30 } };
    test_map_view(local_unordered_map);

    // WinRT interface
    IMapView<int, int> view = single_threaded_map<int, int>(std::map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } }).GetView();
    test_map_view(view);

    // Convertible WinRT interface
    test_map_view(viewable());
}

TEST_CASE("test_map_view_scope")
{
    std::map<int, int> local{ { 1, 10 },{ 2,20 },{ 3,30 } };
    IMapView<int, int> a = test_map_view_scope(local);
    REQUIRE_THROWS_AS(a.First(), invalid_state_error);
}

