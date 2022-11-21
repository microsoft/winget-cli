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

    void test_empty_iterable(param::iterable<int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(!values.First().HasCurrent());
        REQUIRE(!values.First().MoveNext());
        REQUIRE_THROWS_AS(values.First().Current(), out_of_bounds_error);

        std::array<int, 3> copy;
        REQUIRE(0 == values.First().GetMany(copy));
    }

    void test_empty_iterable_pair(param::iterable<IKeyValuePair<int, int>> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(!values.First().HasCurrent());
        REQUIRE(!values.First().MoveNext());
        REQUIRE_THROWS_AS(values.First().Current(), out_of_bounds_error);

        std::array<IKeyValuePair<int, int>, 3> copy;
        REQUIRE(0 == values.First().GetMany(copy));
    }

    IIterable<int> test_iterable_scope(param::iterable<int> const& param)
    {
        return make_copy(param);
    }

    IIterable<IKeyValuePair<int, int>> test_iterable_pair_scope(param::iterable<IKeyValuePair<int, int>> const& param)
    {
        return make_copy(param);
    }

    void test_null_iterable(param::iterable<int> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values == nullptr);
    }

    void test_null_iterable_pair(param::iterable<IKeyValuePair<int, int>> const& param)
    {
        auto values = make_copy(param);
        REQUIRE(values == nullptr);
    }

    void test_iterable(param::iterable<int> const& param)
    {
        IIterable<int> values = make_copy(param);

        std::vector<int> vector(begin(values), end(values));
        REQUIRE(vector.size() == 3);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 2);
        REQUIRE(vector[2] == 3);

        std::array<int, 3> array;
        REQUIRE(3 == values.First().GetMany(array));
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 2);
        REQUIRE(array[2] == 3);

        std::array<int, 2> short_array;
        REQUIRE(2 == values.First().GetMany(short_array));
        REQUIRE(short_array[0] == 1);
        REQUIRE(short_array[1] == 2);

        IIterator<int> iterator = values.First();

        REQUIRE(iterator.HasCurrent());
        REQUIRE(iterator.Current() == 1);
        REQUIRE(iterator.MoveNext());

        REQUIRE(iterator.HasCurrent());
        REQUIRE(iterator.Current() == 2);
        REQUIRE(iterator.MoveNext());

        REQUIRE(iterator.HasCurrent());
        REQUIRE(iterator.Current() == 3);
        REQUIRE(!iterator.MoveNext());
    }

    void test_iterable_pair(param::iterable<IKeyValuePair<int, int>> const& param)
    {
        auto values = make_copy(param);

        std::map<int, int> map;

        for (auto pair : values)
        {
            map[pair.Key()] = pair.Value();
        }

        REQUIRE(map.size() == 3);
        REQUIRE(map[1] == 10);
        REQUIRE(map[2] == 20);
        REQUIRE(map[3] == 30);

        std::array<IKeyValuePair<int, int>, 3> array;
        REQUIRE(3 == values.First().GetMany(array));

        std::sort(array.begin(), array.end(), [](IKeyValuePair<int, int> const & left, IKeyValuePair<int, int> const & right)
        {
            return left.Key() < right.Key();
        });

        REQUIRE(array[0].Key() == 1);
        REQUIRE(array[0].Value() == 10);
        REQUIRE(array[1].Key() == 2);
        REQUIRE(array[1].Value() == 20);
        REQUIRE(array[2].Key() == 3);
        REQUIRE(array[2].Value() == 30);

        std::array<IKeyValuePair<int, int>, 2> short_array;
        REQUIRE(2 == values.First().GetMany(short_array));

        map.clear();

        IIterator<IKeyValuePair<int, int>> iterator = values.First();

        REQUIRE(iterator.HasCurrent());
        map[iterator.Current().Key()] = iterator.Current().Value();
        REQUIRE(iterator.MoveNext());

        REQUIRE(iterator.HasCurrent());
        map[iterator.Current().Key()] = iterator.Current().Value();
        REQUIRE(iterator.MoveNext());

        REQUIRE(iterator.HasCurrent());
        map[iterator.Current().Key()] = iterator.Current().Value();
        REQUIRE(!iterator.MoveNext());

        REQUIRE(map.size() == 3);
        REQUIRE(map[1] == 10);
        REQUIRE(map[2] == 20);
        REQUIRE(map[3] == 30);
    }
}

TEST_CASE("test_iterable")
{
    test_empty_iterable({});
    test_null_iterable(nullptr);

    // initializer_list
    test_iterable({1,2,3});

    // std::vector rvalue
    test_iterable(std::vector<int>{ 1,2,3 });

    // std::vector lvalue
    std::vector<int> local{ 1, 2, 3 };
    test_iterable(local);

    // any other range
    std::list<int> list{1,2,3};
    test_iterable({list.begin(), list.end()});

    // WinRT interface
    IIterable<int> iterable = single_threaded_vector<int>({ 1,2,3 });
    test_iterable(iterable);

    // Convertible WinRT interface
    test_iterable(single_threaded_vector<int>({ 1,2,3 }));

    // Other internal implementations of IIterable
    test_iterable(make<impl::input_vector_view<int, std::vector<int>>>(std::vector<int>{1,2,3}));
    test_iterable(impl::make_scoped_input_vector_view<int>(local.begin(), local.end()).first);
}

TEST_CASE("test_iterable_pair")
{
    test_empty_iterable_pair({});
    test_null_iterable_pair(nullptr);

    // initializer_list
    test_iterable_pair({{1,10}, {2,20}, {3,30}});

    // std::map/unordered_map rvalue
    test_iterable_pair(std::map<int, int>{ { 1,10 },{ 2,20 },{ 3,30 } });
    test_iterable_pair(std::unordered_map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 } });

    // std::map/unordered_map lvalue
    std::map<int, int> local_map{ { 1, 10 }, { 2,20 }, { 3,30 } };
    test_iterable_pair(local_map);
    std::unordered_map<int, int> local_unordered_map{ { 1, 10 },{ 2,20 },{ 3,30 } };
    test_iterable_pair(local_unordered_map);

    // WinRT interface
    IIterable<IKeyValuePair<int, int>> iterable = single_threaded_map<int, int>(std::map<int, int>{ { 1,10 },{ 2,20 },{ 3,30 } });
    test_iterable_pair(iterable);

    // Convertible WinRT interface
    test_iterable_pair(single_threaded_map<int, int>(std::map<int, int>{ { 1,10 },{ 2,20 },{ 3,30 } }));

    // Other internal implementations of IIterable
    test_iterable_pair(make<impl::input_map_view<int, int, std::map<int, int>>>(std::map<int, int>{ { 1, 10 }, { 2,20 }, { 3,30 }}));
    test_iterable_pair(impl::make_scoped_input_map_view<int, int, std::map<int, int>>(local_map).first);
}

TEST_CASE("test_iterable_scope")
{
    IIterable<int> a = test_iterable_scope({1,2,3});
    REQUIRE_THROWS_AS(a.First(), invalid_state_error);

    IIterable<IKeyValuePair<int, int>> b = test_iterable_pair_scope({ { 1, 10 },{ 2,20 },{ 3,30 }});
    REQUIRE_THROWS_AS(b.First(), invalid_state_error);
}

