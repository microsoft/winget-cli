
namespace xlang::impl
{
    template <typename T, typename Container>
    struct input_vector :
        implements<input_vector<T, Container>, fc::IVector<T>, fc::IVectorView<T>, fc::IIterable<T>>,
        vector_base<input_vector<T, Container>, T>
    {
        static_assert(std::is_same_v<Container, std::remove_reference_t<Container>>, "Must be constructed with rvalue.");

        explicit input_vector(Container&& values) : m_values(std::forward<Container>(values))
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
}

namespace xlang::param
{
    template <typename T>
    struct vector
    {
        using value_type = T;
        using interface_type = Foundation::Collections::IVector<value_type>;

        vector(std::nullptr_t) noexcept
        {
        }

        vector(vector const& values) = delete;
        vector& operator=(vector const& values) = delete;

        vector(interface_type const& values) noexcept : m_owned(false)
        {
            attach_abi(m_interface, xlang::get_abi(values));
        }

        template <typename Collection, std::enable_if_t<std::is_convertible_v<Collection, interface_type>>* = nullptr>
        vector(Collection const& values) noexcept
        {
            m_interface = values;
        }

        template <typename Allocator>
        vector(std::vector<value_type, Allocator>&& values) :
            m_interface(make<impl::input_vector<value_type, std::vector<value_type, Allocator>>>(std::move(values)))
        {
        }

        vector(std::initializer_list<value_type> values) :
            m_interface(make<impl::input_vector<value_type, std::vector<value_type>>>(values))
        {
        }

        ~vector() noexcept
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
    auto get_abi(vector<T> const& object) noexcept
    {
        return *(void**)(&object);
    }
}
