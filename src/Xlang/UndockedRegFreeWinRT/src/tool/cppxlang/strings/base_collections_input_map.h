
namespace xlang::impl
{
    template <typename K, typename V, typename Container>
    struct input_map :
        implements<input_map<K, V, Container>, fc::IMap<K, V>, fc::IMapView<K, V>, fc::IIterable<fc::IKeyValuePair<K, V>>>,
        map_base<input_map<K, V, Container>, K, V>
    {
        static_assert(std::is_same_v<Container, std::remove_reference_t<Container>>, "Must be constructed with rvalue.");

        explicit input_map(Container&& values) : m_values(std::forward<Container>(values))
        {
        }

        auto& get_container() noexcept
        {
            return m_values;
        }

        auto& get_container() const noexcept
        {
            return m_values;
        }

    private:

        Container m_values;
    };

    template <typename K, typename V, typename Container>
    auto make_input_map(Container&& values)
    {
        return make<input_map<K, V, Container>>(std::forward<Container>(values));
    }
}

namespace xlang::param
{
    template <typename K, typename V>
    struct map
    {
        using value_type = Foundation::Collections::IKeyValuePair<K, V>;
        using interface_type = Foundation::Collections::IMap<K, V>;

        map(std::nullptr_t) noexcept
        {
        }

        map(map const& values) = delete;
        map& operator=(map const& values) = delete;

        map(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_interface, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        map(Collection const& values) noexcept
        {
            m_interface = values;
        }

        template <typename Compare, typename Allocator>
        map(std::map<K, V, Compare, Allocator>&& values) :
            m_interface(impl::make_input_map<K, V>(std::move(values)))
        {
        }

        template <typename Hash, typename KeyEqual, typename Allocator>
        map(std::unordered_map<K, V, Hash, KeyEqual, Allocator>&& values) :
            m_interface(impl::make_input_map<K, V>(std::move(values)))
        {
        }

        map(std::initializer_list<std::pair<K const, V>> values) :
            m_interface(impl::make_input_map<K, V>(std::map<K, V>(values)))
        {
        }

        ~map() noexcept
        {
            if (!m_owned)
            {
                detach_abi(m_interface);
            }
        }

        operator interface_type const& () const noexcept
        {
            return m_interface;
        }

    private:

        interface_type m_interface;
        bool m_owned{ true };
    };

    template <typename K, typename V>
    auto get_abi(map<K, V> const& object) noexcept
    {
        return *(void**)(&object);
    }
}
