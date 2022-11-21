#include "pch.h"
#include "catch.hpp"
#include <array>

//
// These tests cover the production of the various map-related interfaces.
// Tests ensure that the ABI surface lines up on the consumer and producer sides and this is mainly done simply by calling
// the various inteface methods.
//

using namespace xlang;
using namespace Foundation;
using namespace Foundation::Collections;

namespace
{
    template <typename K, typename V>
    struct key_value_pair : implements<key_value_pair<K, V>, IKeyValuePair<K, V>>
    {
        key_value_pair(K key, V value) :
            m_key(std::move(key)),
            m_value(std::move(value))
        {
        }

        K Key() const
        {
            return m_key;
        }

        V Value() const
        {
            return m_value;
        }

    private:

        K m_key;
        V m_value;
    };
}

// This producer tests that IMap may be specialized with a value type key and hstring value.

TEST_CASE("produce_IMap_int32_t_hstring")
{
    IMap<int32_t, hstring> m = single_threaded_map<int32_t, hstring>();

    REQUIRE(!m.Insert(1, u8"one"));

    REQUIRE(m.Insert(1, u8"one"));

    REQUIRE(!m.Insert(2, u8"two"));

    REQUIRE(m.Insert(2, u8"two"));

    REQUIRE(m.Lookup(1) == u8"one");
    REQUIRE(m.Lookup(2) == u8"two");
    REQUIRE_THROWS_AS(m.Lookup(3), out_of_bounds_error);

    REQUIRE(m.TryLookup(1).value() == u8"one");
    REQUIRE(m.TryLookup(2).value() == u8"two");
    REQUIRE(!m.TryLookup(3));

    REQUIRE(m.Size() == 2);

    REQUIRE(m.HasKey(1));
    REQUIRE(!m.HasKey(3));

    IMapView<int32_t, hstring> view = m.GetView();
    const bool same = view.as<IMap<int32_t, hstring>>() == m;
    REQUIRE(same);

    REQUIRE(m.Size() == 2);
    m.Remove(1); // existing
    REQUIRE(m.Size() == 1);
    m.Remove(3); // not existing
    REQUIRE(m.Size() == 1);

    m.Clear();
    REQUIRE(m.Size() == 0);
}

// This test covers the IMap's specialization of IIterable

TEST_CASE("produce_IMap_IIterable_int32_t_hstring")
{
    IMap<int32_t, hstring> m = single_threaded_map<int32_t, hstring>();
    m.Insert(1, u8"one");
    m.Insert(2, u8"two");
    m.Insert(3, u8"three");

    IIterable<IKeyValuePair<int32_t, hstring>> iterable = m;

    // Both should produce identical iterators but the iterators are unique objects.
    REQUIRE(iterable.First());
    REQUIRE(m.First());
    REQUIRE(iterable.First() != m.First());

    IIterator<IKeyValuePair<int32_t, hstring>> i = m.First();
    REQUIRE((i.Current() == make<key_value_pair<int32_t, hstring>>(1, u8"one")));
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE((i.Current() == make<key_value_pair<int32_t, hstring>>(2, u8"two")));
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE((i.Current() == make<key_value_pair<int32_t, hstring>>(3, u8"three")));
    REQUIRE(i.HasCurrent());
    REQUIRE(!i.MoveNext());

    std::array<IKeyValuePair<int32_t, hstring>, 4> many{};
    REQUIRE(0 == i.GetMany(many));

    // Reset iterator
    i = m.First();
    REQUIRE(3 == i.GetMany(many));
    REQUIRE((many[0] == make<key_value_pair<int32_t, hstring>>(1, u8"one")));
    REQUIRE((many[1] == make<key_value_pair<int32_t, hstring>>(2, u8"two")));
    REQUIRE((many[2] == make<key_value_pair<int32_t, hstring>>(3, u8"three")));
    REQUIRE((!many[3]));
}

// This producer tests that IMap may be specialized with an hstring key and int32_t value.

TEST_CASE("produce_IMap_hstring_int32_t")
{
    IMap<hstring, int32_t> m = single_threaded_map<hstring, int32_t>();

    REQUIRE(!m.Insert(u8"one", 1));
    REQUIRE(m.Insert(u8"one", 1));

    REQUIRE(!m.Insert(u8"two", 2));
    REQUIRE(m.Insert(u8"two", 2));

    REQUIRE(m.Lookup(u8"one") == 1);
    REQUIRE(m.Lookup(u8"two") == 2);
    REQUIRE_THROWS_AS(m.Lookup(u8"three"), out_of_bounds_error);

    REQUIRE(m.TryLookup(u8"one").value() == 1);
    REQUIRE(m.TryLookup(u8"two").value() == 2);
    REQUIRE(!m.TryLookup(u8"three"));

    REQUIRE(m.Size() == 2);

    REQUIRE(m.HasKey(u8"one"));
    REQUIRE(!m.HasKey(u8"three"));

    IMapView<hstring, int32_t> view = m.GetView();
    const bool same = view.as<IMap<hstring, int32_t>>() == m;
    REQUIRE(same);

    REQUIRE(m.Size() == 2);
    m.Remove(u8"one"); // existing
    REQUIRE(m.Size() == 1);
    m.Remove(u8"three"); // not existing
    REQUIRE(m.Size() == 1);

    m.Clear();
    REQUIRE(m.Size() == 0);

    IMapView<hstring, int32_t> first;
    IMapView<hstring, int32_t> second;
    view.Split(first, second);
    REQUIRE(first == nullptr);
    REQUIRE(second == nullptr);
}

// This test covers the IMap's specialization of IIterable

TEST_CASE("produce_IMap_IIterable_hstring_int32_t")
{
    IMap<hstring, int32_t> m = single_threaded_map<hstring, int32_t>();
    m.Insert(u8"one", 1);
    m.Insert(u8"two", 2);
    m.Insert(u8"three", 3);

    IIterable<IKeyValuePair<hstring, int32_t>> iterable = m;

    // Both should produce identical iterators but the iterators are unique objects.
    REQUIRE(iterable.First());
    REQUIRE(m.First());
    REQUIRE(iterable.First() != m.First());

    IIterator<IKeyValuePair<hstring, int32_t>> i = m.First();
    REQUIRE((i.Current() == make<key_value_pair<hstring, int32_t>>(u8"one", 1)));
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE((i.Current() == make<key_value_pair<hstring, int32_t>>(u8"three", 3)));
    REQUIRE(i.HasCurrent());
    REQUIRE(i.MoveNext());
    REQUIRE((i.Current() == make<key_value_pair<hstring, int32_t>>(u8"two", 2)));
    REQUIRE(i.HasCurrent());
    REQUIRE(!i.MoveNext());

    std::array<IKeyValuePair<hstring, int32_t>, 4> many{};
    REQUIRE(0 == i.GetMany(many));

    // Reset iterator
    i = m.First();
    REQUIRE(3 == i.GetMany(many));
    REQUIRE((many[0] == make<key_value_pair<hstring, int32_t>>(u8"one", 1)));
    REQUIRE((many[1] == make<key_value_pair<hstring, int32_t>>(u8"three", 3)));
    REQUIRE((many[2] == make<key_value_pair<hstring, int32_t>>(u8"two", 2)));
    REQUIRE((!many[3]));
}
