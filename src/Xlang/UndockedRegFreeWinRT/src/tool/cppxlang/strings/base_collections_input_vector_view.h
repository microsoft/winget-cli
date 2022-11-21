
namespace xlang::impl
{
    template <typename T, typename Container>
    struct input_vector_view :
        implements<input_vector_view<T, Container>, non_agile, no_weak_ref, fc::IVectorView<T>, fc::IIterable<T>>,
        vector_view_base<input_vector_view<T, Container>, T>
    {
        static_assert(std::is_same_v<Container, std::remove_reference_t<Container>>, "Must be constructed with rvalue.");

        explicit input_vector_view(Container&& values) : m_values(std::forward<Container>(values))
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
    struct scoped_input_vector_view :
        input_scope,
        implements<scoped_input_vector_view<T, InputIt>, non_agile, no_weak_ref, fc::IVectorView<T>, fc::IIterable<T>>,
        vector_view_base<scoped_input_vector_view<T, InputIt>, T>
    {
        void abi_enter() const
        {
            check_scope();
        }

        scoped_input_vector_view(InputIt first, InputIt last) : m_begin(first), m_end(last)
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

    template <typename T, typename InputIt>
    auto make_scoped_input_vector_view(InputIt first, InputIt last)
    {
        using interface_type = fc::IVectorView<T>;
        std::pair<interface_type, input_scope*> result;
        auto ptr = new scoped_input_vector_view<T, InputIt>(first, last);
        *put_abi(result.first) = to_abi<interface_type>(ptr);
        result.second = ptr;
        return result;
    }
}

namespace xlang::param
{
    template <typename T>
    struct vector_view
    {
        using value_type = T;
        using interface_type = Foundation::Collections::IVectorView<value_type>;

        vector_view(std::nullptr_t) noexcept
        {
        }

        vector_view(vector_view const& values) = delete;
        vector_view& operator=(vector_view const& values) = delete;

        vector_view(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_pair.first, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        vector_view(Collection const& values) noexcept
        {
            m_pair.first = values;
        }

        template <typename Allocator>
        vector_view(std::vector<value_type, Allocator>&& values) : m_pair(make<impl::input_vector_view<value_type, std::vector<value_type, Allocator>>>(std::move(values)), nullptr)
        {
        }

        template <typename Allocator>
        vector_view(std::vector<value_type, Allocator> const& values) : m_pair(impl::make_scoped_input_vector_view<value_type>(values.begin(), values.end()))
        {
        }

        vector_view(std::initializer_list<value_type> values) : m_pair(impl::make_scoped_input_vector_view<value_type>(values.begin(), values.end()))
        {
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, value_type>>* = nullptr>
        vector_view(std::initializer_list<U> values) : m_pair(impl::make_scoped_input_vector_view<value_type>(values.begin(), values.end()))
        {
        }

        template<class InputIt>
        vector_view(InputIt first, InputIt last) : m_pair(impl::make_scoped_input_vector_view<value_type>(first, last))
        {
        }

        ~vector_view() noexcept
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
    auto get_abi(vector_view<T> const& object) noexcept
    {
        return *(void**)(&object);
    }

    template <typename T>
    struct async_vector_view
    {
        using value_type = T;
        using interface_type = Foundation::Collections::IVectorView<value_type>;

        async_vector_view(std::nullptr_t) noexcept
        {
        }

        async_vector_view(async_vector_view const& values) = delete;
        async_vector_view& operator=(async_vector_view const& values) = delete;

        async_vector_view(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_interface, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        async_vector_view(Collection const& values) noexcept
        {
            m_interface = values;
        }

        template <typename Allocator>
        async_vector_view(std::vector<value_type, Allocator>&& values) :
            m_interface(make<impl::input_vector_view<value_type, std::vector<value_type, Allocator>>>(std::move(values)))
        {
        }

        async_vector_view(std::initializer_list<value_type> values) :
            m_interface(make<impl::input_vector_view<value_type, std::vector<value_type>>>(values))
        {
        }

        ~async_vector_view() noexcept
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
    auto get_abi(async_vector_view<T> const& object) noexcept
    {
        return *(void**)(&object);
    }
}
