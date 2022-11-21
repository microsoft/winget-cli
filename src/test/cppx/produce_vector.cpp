#include "pch.h"
#include "catch.hpp"
#include <array>

//
// These tests cover the production of the various vector-related interfaces.
// Tests ensure that the ABI surface lines up on the consumer and producer sides and this is mainly done simply by calling
// the various inteface methods.
//

using namespace xlang;
using namespace Foundation;
using namespace Foundation::Collections;

//
// IVectorChangedEventArgs
//

// This producer tests that IVector may be specialized with a value type (int32_t).

TEST_CASE("produce_IVector_int32_t")
{
    IVector<int32_t> v = single_threaded_vector<int32_t>({ 1, 2, 3 });

    REQUIRE(v.GetAt(0) == 1);
    REQUIRE(v.GetAt(2) == 3);
    REQUIRE_THROWS_AS(v.GetAt(v.Size()), out_of_bounds_error);

    REQUIRE(v.Size() == 3);

    IVectorView<int32_t> view = v.GetView();
    REQUIRE(view.as<IVector<int32_t>>() == v);

    uint32_t index = 0;
    REQUIRE(v.IndexOf(2, index));
    REQUIRE(index == 1);
    REQUIRE(!v.IndexOf(4, index));

    v.SetAt(1, 20);
    REQUIRE(v.GetAt(1) == 20);
    REQUIRE_THROWS_AS(v.SetAt(v.Size(), 0), out_of_bounds_error);
    v.SetAt(1, 2);

    v.InsertAt(0, 0);

    v.InsertAt(v.Size(), 4);

    REQUIRE_THROWS_AS(v.InsertAt(v.Size() + 1, 0), out_of_bounds_error);
    REQUIRE(v.Size() == 5);
    REQUIRE(v.GetAt(0) == 0);
    REQUIRE(v.GetAt(1) == 1);
    REQUIRE(v.GetAt(2) == 2);
    REQUIRE(v.GetAt(3) == 3);
    REQUIRE(v.GetAt(4) == 4);

    v.RemoveAt(0);
    v.RemoveAt(v.Size() - 1);
    REQUIRE_THROWS_AS(v.RemoveAt(v.Size()), out_of_bounds_error);
    REQUIRE(v.Size() == 3);
    REQUIRE(v.GetAt(0) == 1);
    REQUIRE(v.GetAt(1) == 2);
    REQUIRE(v.GetAt(2) == 3);

    v.Append(4);
    REQUIRE(v.Size() == 4);
    REQUIRE(v.GetAt(0) == 1);
    REQUIRE(v.GetAt(1) == 2);
    REQUIRE(v.GetAt(2) == 3);
    REQUIRE(v.GetAt(3) == 4);

    v.RemoveAtEnd();
    REQUIRE(v.Size() == 3);
    REQUIRE(v.GetAt(0) == 1);
    REQUIRE(v.GetAt(1) == 2);
    REQUIRE(v.GetAt(2) == 3);

    REQUIRE(v.Size() == 3);
    v.Clear();
    REQUIRE(v.Size() == 0);
}

// This test covers the specifics of collection/array interaction

TEST_CASE("produce_IVector_array_int32_t")
{
    IVector<int32_t> v = single_threaded_vector<int32_t>({ 1, 2, 3 });

    {
        // Exact number of values.

        std::array<int32_t, 3> many {};
        REQUIRE(3 == v.GetMany(0, many));
        REQUIRE(many[0] == 1);
        REQUIRE(many[1] == 2);
        REQUIRE(many[2] == 3);
    }

    {
        // Array is larger than IVector.

        std::array<int32_t, 4> many{};
        REQUIRE(3 == v.GetMany(0, many));
        REQUIRE(many[0] == 1);
        REQUIRE(many[1] == 2);
        REQUIRE(many[2] == 3);
        REQUIRE(many[3] == 0);
    }

    {
        // Array is smaller than IVector.

        std::array<int32_t, 2> many{};
        REQUIRE(2 == v.GetMany(0, many));
        REQUIRE(many[0] == 1);
        REQUIRE(many[1] == 2);
    }

    {
        // Using a non-zero start index to end of collection.

        std::array<int32_t, 2> many{};
        REQUIRE(2 == v.GetMany(1, many));
        REQUIRE(many[0] == 2);
        REQUIRE(many[1] == 3);
    }

    {
        // Using a non-zero start index and array is not large enough for rest of collection.

        std::array<int32_t, 1> many{};
        REQUIRE(1 == v.GetMany(1, many));
        REQUIRE(many[0] == 2);
    }

    {
        // Start index doesn't include any values.

        std::array<int32_t, 2> many{};
        REQUIRE(0 == v.GetMany(v.Size(), many));
        REQUIRE(many[0] == 0);
        REQUIRE(many[1] == 0);
    }

    {
        std::array<int32_t, 2> many { 10, 20 };
        REQUIRE(v.Size() == 3);
        v.ReplaceAll(many);
        REQUIRE(v.Size() == 2);
        REQUIRE(v.GetAt(0) == 10);
        REQUIRE(v.GetAt(1) == 20);
    }
}

// This test covers the IVector's implementation of IIterable

TEST_CASE("produce_IVector_IIterable_int32_t")
{
    IVector<int32_t> v = single_threaded_vector<int32_t>({ 1, 2, 3 });

    IIterable<int> iterable = v;

    // Both should produce identical iterators but the iterators are unique objects.
    REQUIRE(iterable.First());
    REQUIRE(v.First());
    REQUIRE(iterable.First() != v.First());

    IIterator<int> i = v.First();
    REQUIRE(i.Current() == 1);
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE(i.Current() == 2);
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE(i.Current() == 3);
    REQUIRE(i.HasCurrent());
    REQUIRE(!i.MoveNext());

    std::array<int32_t, 4> many{};
    REQUIRE(0 == i.GetMany(many));

    // Reset iterator
    i = v.First();
    REQUIRE(3 == i.GetMany(many));
    REQUIRE(many[0] == 1);
    REQUIRE(many[1] == 2);
    REQUIRE(many[2] == 3);
    REQUIRE(many[3] == 0);
}

// This producer tests that IVector may be specialized with an hstring type.

TEST_CASE("produce_IVector_hstring")
{
    IVector<hstring> v = single_threaded_vector<hstring>({ L"1", L"2", L"3" });

    REQUIRE(v.GetAt(0) == L"1");
    REQUIRE(v.GetAt(2) == L"3");
    REQUIRE_THROWS_AS(v.GetAt(v.Size()), out_of_bounds_error);

    REQUIRE(v.Size() == 3);

    IVectorView<hstring> view = v.GetView();
    REQUIRE(view.as<IVector<hstring>>() == v);

    uint32_t index = 0;
    REQUIRE(v.IndexOf(L"2", index));
    REQUIRE(index == 1);
    REQUIRE(!v.IndexOf(L"4", index));

    v.SetAt(1, L"20");
    REQUIRE(v.GetAt(1) == L"20");
    REQUIRE_THROWS_AS(v.SetAt(v.Size(), L"0"), out_of_bounds_error);
    v.SetAt(1, L"2");

    v.InsertAt(0, L"0");
    v.InsertAt(v.Size(), L"4");
    REQUIRE_THROWS_AS(v.InsertAt(v.Size() + 1, L"0"), out_of_bounds_error);
    REQUIRE(v.Size() == 5);
    REQUIRE(v.GetAt(0) == L"0");
    REQUIRE(v.GetAt(1) == L"1");
    REQUIRE(v.GetAt(2) == L"2");
    REQUIRE(v.GetAt(3) == L"3");
    REQUIRE(v.GetAt(4) == L"4");

    v.RemoveAt(0);
    v.RemoveAt(v.Size() - 1);
    REQUIRE_THROWS_AS(v.RemoveAt(v.Size()), out_of_bounds_error);
    REQUIRE(v.Size() == 3);
    REQUIRE(v.GetAt(0) == L"1");
    REQUIRE(v.GetAt(1) == L"2");
    REQUIRE(v.GetAt(2) == L"3");

    v.Append(L"4");
    REQUIRE(v.Size() == 4);
    REQUIRE(v.GetAt(0) == L"1");
    REQUIRE(v.GetAt(1) == L"2");
    REQUIRE(v.GetAt(2) == L"3");
    REQUIRE(v.GetAt(3) == L"4");

    v.RemoveAtEnd();
    REQUIRE(v.Size() == 3);
    REQUIRE(v.GetAt(0) == L"1");
    REQUIRE(v.GetAt(1) == L"2");
    REQUIRE(v.GetAt(2) == L"3");

    REQUIRE(v.Size() == 3);
    v.Clear();
    REQUIRE(v.Size() == 0);
}

// This test covers the specifics of collection/array interaction

TEST_CASE("produce_IVector_array_hstring")
{
    IVector<hstring> v = single_threaded_vector<hstring>({ L"1", L"2", L"3" });

    {
        // Exact number of values.

        std::array<hstring, 3> many{};
        REQUIRE(3 == v.GetMany(0, many));
        REQUIRE(many[0] == L"1");
        REQUIRE(many[1] == L"2");
        REQUIRE(many[2] == L"3");
    }

    {
        // Array is larger than IVector.

        std::array<hstring, 4> many{};
        REQUIRE(3 == v.GetMany(0, many));
        REQUIRE(many[0] == L"1");
        REQUIRE(many[1] == L"2");
        REQUIRE(many[2] == L"3");
        REQUIRE(many[3] == L"");
    }

    {
        // Array is smaller than IVector.

        std::array<hstring, 2> many{};
        REQUIRE(2 == v.GetMany(0, many));
        REQUIRE(many[0] == L"1");
        REQUIRE(many[1] == L"2");
    }

    {
        // Start index doesn't include any values.

        std::array<hstring, 2> many{};
        REQUIRE(0 == v.GetMany(v.Size(), many));
        REQUIRE(many[0] == L"");
        REQUIRE(many[1] == L"");
    }

    {
        std::array<hstring, 2> many{ L"10", L"20" };
        REQUIRE(v.Size() == 3);
        v.ReplaceAll(many);
        REQUIRE(v.Size() == 2);
        REQUIRE(v.GetAt(0) == L"10");
        REQUIRE(v.GetAt(1) == L"20");
    }
}

// This test covers the IVector's implementation of IIterable

TEST_CASE("produce_IVector_IIterable_hstring")
{
    IVector<hstring> v = single_threaded_vector<hstring>({ L"1", L"2", L"3" });

    IIterable<hstring> iterable = v;

    // Both should produce identical iterators but the iterators are unique objects.
    REQUIRE(iterable.First());
    REQUIRE(v.First());
    REQUIRE(iterable.First() != v.First());

    IIterator<hstring> i = v.First();
    REQUIRE(i.Current() == L"1");
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE(i.Current() == L"2");
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE(i.Current() == L"3");
    REQUIRE(i.HasCurrent());
    REQUIRE(!i.MoveNext());

    std::array<hstring, 4> many{};
    REQUIRE(0 == i.GetMany(many));

    // Reset iterator
    i = v.First();
    REQUIRE(3 == i.GetMany(many));
    REQUIRE(many[0] == L"1");
    REQUIRE(many[1] == L"2");
    REQUIRE(many[2] == L"3");
    REQUIRE(many[3] == L"");
}
