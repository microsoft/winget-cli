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

    void test_empty_map(param::map<int, int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values.Size() == 0);
    }

    void test_null_map(param::map<int, int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values == nullptr);
    }

    void test_map(param::map<int, int> const& param)
    {
        IMap<int, int> values = make_copy(param);

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
    }

    struct viewable
    {
        IMap<int, int> view{ single_threaded_map<int, int>(std::map<int, int>{ { 1,10 },{ 2,20 },{ 3,30 } }) };

        operator IMap<int, int>() const
        {
            return view;
        }
    };
}

TEST_CASE("test_map")
{
    test_empty_map({});
    test_null_map(nullptr);

    // initializer_list
    test_map({ { 1,10 },{ 2,20 },{ 3,30 } });

    // std::map/unordered_map rvalue
    test_map(std::map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });
    test_map(std::unordered_map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });

    // WinRT interface
    IMap<int, int> view = single_threaded_map<int, int>(std::map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });
    test_map(view);

    // Convertible WinRT interface
    test_map(viewable());
}
