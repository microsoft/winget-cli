#include "pch.h"
#include "catch.hpp"
#include <deque>

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

    void test_empty_vector(param::vector<int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values.Size() == 0);
        REQUIRE_THROWS_AS(values.GetAt(0), out_of_bounds_error);

        std::array<int, 3> copy;
        REQUIRE(0 == values.GetMany(0, copy));
    }

    IVector<int> test_vector_scope(param::vector<int> const& param)
    {
        return make_copy(param);
    }

    void test_null_vector(param::vector<int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values == nullptr);
    }

    void test_vector(param::vector<int> const& param)
    {
        IVector<int> values = make_copy(param);

        std::vector<int> vector(begin(values), end(values));
        REQUIRE(vector.size() == 3);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 2);
        REQUIRE(vector[2] == 3);

        std::array<int, 3> array;
        REQUIRE(3 == values.GetMany(0, array));
        REQUIRE(0 == values.GetMany(values.Size(), array));
        REQUIRE(0 == values.GetMany(values.Size(), array));
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 2);
        REQUIRE(array[2] == 3);
        REQUIRE(2 == values.GetMany(1, array));
        REQUIRE(array[0] == 2);
        REQUIRE(array[1] == 3);
        REQUIRE(array[2] == 3);

        std::array<int, 2> short_array;
        REQUIRE(2 == values.GetMany(0, short_array));
        REQUIRE(short_array[0] == 1);
        REQUIRE(short_array[1] == 2);

        uint32_t index = 100;
        REQUIRE(!values.IndexOf(4, index));

        REQUIRE(values.IndexOf(1, index));
        REQUIRE(index == 0);
        REQUIRE(values.IndexOf(2, index));
        REQUIRE(index == 1);
        REQUIRE(values.IndexOf(3, index));
        REQUIRE(index == 2);
    }

    struct viewable
    {
        IVector<int> view{ single_threaded_vector<int>({ 1,2,3 }) };

        operator IVector<int>() const
        {
            return view;
        }
    };
}

TEST_CASE("test_vector")
{
    test_empty_vector({});
    test_null_vector(nullptr);

    // initializer_list
    test_vector({1,2,3});

    // std::vector rvalue
    test_vector(std::vector<int>{ 1,2,3 });

    // WinRT interface
    IVector<int> view = single_threaded_vector<int>({ 1,2,3 });
    test_vector(view);

    // Convertible WinRT interface
    test_vector(viewable());
}

TEST_CASE("test_vector_scope")
{
    // No scoping is observed
    IVector<int> a = test_vector_scope({ 1,2,3 });
    REQUIRE(a.Size() == 3);
}

