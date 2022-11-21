
namespace xlang::impl
{
    template <typename T, typename Container>
    struct input_iterable :
        implements<input_iterable<T, Container>, non_agile, no_weak_ref, fc::IIterable<T>>,
        iterable_base<input_iterable<T, Container>, T>
    {
        static_assert(std::is_same_v<Container, std::remove_reference_t<Container>>, "Must be constructed with rvalue.");

        explicit input_iterable(Container&& values) : m_values(std::forward<Container>(values))
        {
        }

        auto& get_container() const noexcept
        {
            return m_values;
        }

    private:

        Container const m_values;
    };

    template <typename T, typename InputIt>
    struct scoped_input_iterable :
        input_scope,
        implements<scoped_input_iterable<T, InputIt>, non_agile, no_weak_ref, fc::IIterable<T>>,
        iterable_base<scoped_input_iterable<T, InputIt>, T>
    {
        void abi_enter() const
        {
            check_scope();
        }

        scoped_input_iterable(InputIt first, InputIt last) : m_begin(first), m_end(last)
        {
        }

        auto get_container() const noexcept
        {
            return range_container<InputIt>{ m_begin, m_end };
        }

#ifdef _DEBUG
        void use_make_function_to_create_this_object() final
        {
        }
#endif

    private:

        InputIt const m_begin;
        InputIt const m_end;
    };

    template <typename T, typename Container>
    auto make_input_iterable(Container&& values)
    {
        return make<input_iterable<T, Container>>(std::forward<Container>(values));
    }

    template <typename T, typename InputIt>
    auto make_scoped_input_iterable(InputIt first, InputIt last)
    {
        using interface_type = fc::IIterable<T>;
        std::pair<interface_type, input_scope*> result;
        auto ptr = new scoped_input_iterable<T, InputIt>(first, last);
        *put_abi(result.first) = to_abi<interface_type>(ptr);
        result.second = ptr;
        return result;
    }
}

namespace xlang::param
{
    template <typename T>
    struct iterable
    {
        using value_type = T;
        using interface_type = Foundation::Collections::IIterable<value_type>;

        iterable(std::nullptr_t) noexcept
        {
        }

        iterable(iterable const& values) = delete;
        iterable& operator=(iterable const& values) = delete;

        iterable(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_pair.first, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        iterable(Collection const& values) noexcept
        {
            m_pair.first = values;
        }

        template <typename Allocator>
        iterable(std::vector<value_type, Allocator>&& values) : m_pair(impl::make_input_iterable<value_type>(std::move(values)), nullptr)
        {
        }

        template <typename Allocator>
        iterable(std::vector<value_type, Allocator> const& values) : m_pair(impl::make_scoped_input_iterable<value_type>(values.begin(), values.end()))
        {
        }

        iterable(std::initializer_list<value_type> values) : m_pair(impl::make_scoped_input_iterable<value_type>(values.begin(), values.end()))
        {
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, value_type>>* = nullptr>
        iterable(std::initializer_list<U> values) : m_pair(impl::make_scoped_input_iterable<value_type>(values.begin(), values.end()))
        {
        }

        template<class InputIt>
        iterable(InputIt first, InputIt last) : m_pair(impl::make_scoped_input_iterable<value_type>(first, last))
        {
        }

        ~iterable() noexcept
        {
            if (m_pair.second)
            {
                m_pair.second->invalidate_scope();
            }

            if (!m_owned)
            {
                detach_abi(m_pair.first);
            }
        }

        operator interface_type const& () const noexcept
        {
            return m_pair.first;
        }

    private:

        std::pair<interface_type, impl::input_scope*> m_pair;
        bool m_owned{ true };
    };

    template <typename K, typename V>
    struct iterable<Foundation::Collections::IKeyValuePair<K, V>>
    {
        using value_type = Foundation::Collections::IKeyValuePair<K, V>;
        using interface_type = Foundation::Collections::IIterable<value_type>;

        iterable(std::nullptr_t) noexcept
        {
        }

        iterable(iterable const& values) = delete;
        iterable& operator=(iterable const& values) = delete;

        iterable(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_pair.first, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        iterable(Collection const& values) noexcept
        {
            m_pair.first = values;
        }

        template <typename Compare, typename Allocator>
        iterable(std::map<K, V, Compare, Allocator>&& values) : m_pair(impl::make_input_iterable<value_type>(std::move(values)), nullptr)
        {
        }

        template <typename Compare, typename Allocator>
        iterable(std::map<K, V, Compare, Allocator> const& values) : m_pair(impl::make_scoped_input_iterable<value_type>(values.begin(), values.end()))
        {
        }

        template <typename Hash, typename KeyEqual, typename Allocator>
        iterable(std::unordered_map<K, V, Hash, KeyEqual, Allocator>&& values) : m_pair(impl::make_input_iterable<value_type>(std::move(values)), nullptr)
        {
        }

        template <typename Hash, typename KeyEqual, typename Allocator>
        iterable(std::unordered_map<K, V, Hash, KeyEqual, Allocator> const& values) : m_pair(impl::make_scoped_input_iterable<value_type>(values.begin(), values.end()))
        {
        }

        iterable(std::initializer_list<std::pair<K const, V>> values) : m_pair(impl::make_scoped_input_iterable<value_type>(values.begin(), values.end()))
        {
        }

        template<class InputIt>
        iterable(InputIt first, InputIt last) : m_pair(impl::make_scoped_input_iterable<value_type>(first, last))
        {
        }

        ~iterable() noexcept
        {
            if (m_pair.second)
            {
                m_pair.second->invalidate_scope();
            }

            if (!m_owned)
            {
                detach_abi(m_pair.first);
            }
        }

        operator interface_type const& () const noexcept
        {
            return m_pair.first;
        }

    private:

        std::pair<interface_type, impl::input_scope*> m_pair;
        bool m_owned{ true };
    };

    template <typename T>
    auto get_abi(iterable<T> const& object) noexcept
    {
        return *(void**)(&object);
    }

    template <typename T>
    struct async_iterable
    {
        using value_type = T;
        using interface_type = Foundation::Collections::IIterable<value_type>;

        async_iterable(std::nullptr_t) noexcept
        {
        }

        async_iterable(async_iterable const& values) = delete;
        async_iterable& operator=(async_iterable const& values) = delete;

        async_iterable(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_interface, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        async_iterable(Collection const& values) noexcept
        {
            m_interface = values;
        }

        template <typename Allocator>
        async_iterable(std::vector<value_type, Allocator>&& values) :
            m_interface(impl::make_input_iterable<value_type>(std::move(values)))
        {
        }

        async_iterable(std::initializer_list<value_type> values) :
            m_interface(impl::make_input_iterable<value_type>(std::vector<value_type>(values)))
        {
        }

        ~async_iterable() noexcept
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
    struct async_iterable<Foundation::Collections::IKeyValuePair<K, V>>
    {
        using value_type = Foundation::Collections::IKeyValuePair<K, V>;
        using interface_type = Foundation::Collections::IIterable<value_type>;

        async_iterable(std::nullptr_t) noexcept
        {
        }

        async_iterable(async_iterable const& values) = delete;
        async_iterable& operator=(async_iterable const& values) = delete;

        async_iterable(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_interface, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        async_iterable(Collection const& values) noexcept
        {
            m_interface = values;
        }

        template <typename Compare, typename Allocator>
        async_iterable(std::map<K, V, Compare, Allocator>&& values) :
            m_interface(impl::make_input_iterable<value_type>(std::move(values)))
        {
        }

        template <typename Hash, typename KeyEqual, typename Allocator>
        async_iterable(std::unordered_map<K, V, Hash, KeyEqual, Allocator>&& values) :
            m_interface(impl::make_input_iterable<value_type>(std::move(values)))
        {
        }

        async_iterable(std::initializer_list<std::pair<K const, V>> values) :
            m_interface(impl::make_input_iterable<value_type>(std::map<K, V>(values)))
        {
        }

        ~async_iterable() noexcept
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

    template <typename T>
    auto get_abi(async_iterable<T> const& object) noexcept
    {
        return *(void**)(&object);
    }
}
