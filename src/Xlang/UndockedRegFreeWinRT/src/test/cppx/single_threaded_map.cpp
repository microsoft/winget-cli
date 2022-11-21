#include "pch.h"
#include "catch.hpp"
#include <deque>

using namespace xlang;
using namespace Foundation::Collections;

namespace
{
    void compare(IMap<int, int> const & left, std::map<int, int> && right)
    {
        std::map<int, int> copy;

        for (auto pair : left)
        {
            copy[pair.Key()] = pair.Value();
        }

        REQUIRE(copy == right);
    }

    template <typename Change>
    void test_invalidation(IMap<int, int> const & values, Change change)
    {
        std::array<IKeyValuePair<int, int>, 3> array;

        values.Insert(1,10);
        values.Insert(2,20);
        values.Insert(3,30);
        IIterator<IKeyValuePair<int, int>> first = values.First();
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

    void test_map(IMap<int, int> const & values)
    {
        compare(values, {});

        REQUIRE(!values.Insert(1, 10));
        compare(values, {{1,10}});
        REQUIRE(values.Insert(1, 100));
        compare(values, { { 1,100 } });
        REQUIRE(!values.Insert(2, 20));
        compare(values, { { 1,100 }, {2,20} });

        values.Remove(3);
        compare(values, { { 1,100 },{ 2,20 } });
        values.Remove(2);
        compare(values, { { 1,100 } });
        values.Remove(1);
        compare(values, { });

        values.Insert(1,10);
        compare(values, { { 1,10 } });
        values.Clear();
        compare(values, {});

        test_invalidation(values, [&] { values.Clear(); });
        test_invalidation(values, [&] { values.Remove(10); });
        test_invalidation(values, [&] { values.Insert(1,10); });
    }
}

TEST_CASE("single_threaded_map - construction")
{
    IMap<int, int> values;

    values = single_threaded_map<int, int>();
    REQUIRE(values.Size() == 0);

    values = single_threaded_map<int, int>(std::map<int, int>{ { 1,10 },{ 2,20 },{ 3,30 } });
    compare(values, { { 1,10 },{ 2,20 },{ 3,30 } });

    values = single_threaded_map<int, int>(std::unordered_map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });
    compare(values, { { 1,10 },{ 2,20 },{ 3,30 } });
}

TEST_CASE("test_single_threaded_map")
{
    test_map(single_threaded_map<int, int>());
}
