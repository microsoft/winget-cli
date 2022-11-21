#include "pch.h"
#include "catch.hpp"
#include <deque>

using namespace xlang;
using namespace Foundation::Collections;

namespace
{
    void compare(IVector<int> const & left, std::vector<int> && right)
    {
        std::vector<int> copy(begin(left), end(left));
        REQUIRE(copy == right);
    }

    template <typename Change>
    void test_invalidation(IVector<int> const & values, Change change)
    {
        std::array<int, 3> array;

        values.ReplaceAll({ 1,2,3 });
        IIterator<int> first = values.First();
        first.HasCurrent();
        first.Current();
        first.MoveNext();
        first.GetMany(array);

        change(); // <-- invalidate

        REQUIRE_THROWS_AS(first.HasCurrent(), invalid_state_error);
        REQUIRE_THROWS_AS(first.Current(), invalid_state_error);
        REQUIRE_THROWS_AS(first.MoveNext(), invalid_state_error);
        REQUIRE_THROWS_AS(first.GetMany(array), invalid_state_error);
    }

    void test_vector(IVector<int> const & values)
    {
        compare(values, {});

        REQUIRE_THROWS_AS(values.SetAt(0, 1), out_of_bounds_error);
        values.InsertAt(0, 1);
        compare(values, { 1 });
        values.SetAt(0, 2);
        compare(values, { 2 });

        values.Clear();
        compare(values, {});

        REQUIRE_THROWS_AS(values.InsertAt(1, 1), out_of_bounds_error);
        compare(values, {});
        values.InsertAt(0, 1);
        compare(values, {1});
        values.InsertAt(0, 2);
        compare(values, { 2, 1 });
        values.InsertAt(2, 0);
        compare(values, { 2, 1, 0 });

        REQUIRE_THROWS_AS(values.RemoveAt(3), out_of_bounds_error);
        values.RemoveAt(1);
        compare(values, { 2, 0 });
        values.RemoveAt(0);
        compare(values, { 0 });
        values.RemoveAt(0);
        compare(values, {});

        values.Append(1);
        compare(values, {1});
        values.Append(2);
        compare(values, { 1, 2 });
        values.Append(3);
        compare(values, { 1, 2, 3 });

        values.RemoveAtEnd();
        compare(values, { 1, 2 });
        values.RemoveAtEnd();
        compare(values, { 1 });
        values.RemoveAtEnd();
        compare(values, {  });
        REQUIRE_THROWS_AS(values.RemoveAtEnd(), out_of_bounds_error);

        values.ReplaceAll({1,2,3,4});
        compare(values, {1,2,3,4});

        values.ReplaceAll({});
        compare(values,{});

        test_invalidation(values, [&] { values.Clear(); });
        test_invalidation(values, [&] { values.SetAt(0, 0); });
        test_invalidation(values, [&] { values.InsertAt(0, 0); });
        test_invalidation(values, [&] { values.RemoveAt(0); });
        test_invalidation(values, [&] { values.Append(0); });
        test_invalidation(values, [&] { values.RemoveAtEnd(); });
        test_invalidation(values, [&] { values.ReplaceAll({}); });
    }
}

TEST_CASE("single_threaded_vector - construction")
{
    IVector<int> values;

    values = single_threaded_vector<int>();
    REQUIRE(values.Size() == 0);

    values = single_threaded_vector<int>({ 1,2,3 });
    compare(values, { 1,2,3 });
}

TEST_CASE("test_single_threaded_vector")
{
    test_vector(single_threaded_vector<int>());
}
