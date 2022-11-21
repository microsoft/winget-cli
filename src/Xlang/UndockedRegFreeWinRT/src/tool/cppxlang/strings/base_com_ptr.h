
namespace xlang
{
    template <typename T>
    struct com_ptr
    {
        using type = impl::abi_t<T>;

        com_ptr(std::nullptr_t = nullptr) noexcept {}

        com_ptr(void* ptr, take_ownership_from_abi_t) noexcept : m_ptr(static_cast<type*>(ptr))
        {
        }

        com_ptr(com_ptr const& other) noexcept : m_ptr(other.m_ptr)
        {
            add_ref();
        }

        template <typename U>
        com_ptr(com_ptr<U> const& other) noexcept : m_ptr(other.m_ptr)
        {
            add_ref();
        }

        template <typename U>
        com_ptr(com_ptr<U>&& other) noexcept : m_ptr(std::exchange(other.m_ptr, {}))
        {
        }

        ~com_ptr() noexcept
        {
            release_ref();
        }

        com_ptr& operator=(com_ptr const& other) noexcept
        {
            copy_ref(other.m_ptr);
            return*this;
        }

        com_ptr& operator=(com_ptr&& other) noexcept
        {
            if (this != &other)
            {
                release_ref();
                m_ptr = std::exchange(other.m_ptr, {});
            }

            return*this;
        }

        template <typename U>
        com_ptr& operator=(com_ptr<U> const& other) noexcept
        {
            copy_ref(other.m_ptr);
            return*this;
        }

        template <typename U>
        com_ptr& operator=(com_ptr<U>&& other) noexcept
        {
            release_ref();
            m_ptr = std::exchange(other.m_ptr, {});
            return*this;
        }

        explicit operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }

        auto operator->() const noexcept
        {
            return m_ptr;
        }

        T& operator*() const noexcept
        {
            return *m_ptr;
        }

        type* get() const noexcept
        {
            return m_ptr;
        }

        type** put() noexcept
        {
            XLANG_ASSERT(m_ptr == nullptr);
            return &m_ptr;
        }

        void** put_void() noexcept
        {
            return reinterpret_cast<void**>(put());
        }

        void attach(type* value) noexcept
        {
            release_ref();
            *put() = value;
        }

        type* detach() noexcept
        {
            return std::exchange(m_ptr, {});
        }

        friend void swap(com_ptr& left, com_ptr& right) noexcept
        {
            std::swap(left.m_ptr, right.m_ptr);
        }

        template <typename To>
        auto as() const
        {
            return impl::as<To>(m_ptr);
        }

        template <typename To>
        auto try_as() const noexcept
        {
            return impl::try_as<To>(m_ptr);
        }

        template <typename To>
        void as(To& to) const
        {
            to = as<impl::wrapped_type_t<To>>();
        }

        template <typename To>
        bool try_as(To& to) const noexcept
        {
            to = try_as<impl::wrapped_type_t<To>>();
            return static_cast<bool>(to);
        }

        com_interop_result as(guid const& id, void** result) const noexcept
        {
            return m_ptr->QueryInterface(id, result);
        }

        void copy_from(type* other) noexcept
        {
            copy_ref(other);
        }

        void copy_to(type** other) const noexcept
        {
            add_ref();
            *other = m_ptr;
        }

        template <typename F, typename...Args>
        void capture(F function, Args&&...args)
        {
            check_xlang_error(function(args..., guid_of<T>(), put_void()));
        }

        template <typename O, typename M, typename...Args>
        void capture(com_ptr<O> const& object, M method, Args&&...args)
        {
            check_xlang_error((object.get()->*(method))(args..., guid_of<T>(), put_void()));
        }

    private:

        void copy_ref(type* other) noexcept
        {
            if (m_ptr != other)
            {
                release_ref();
                m_ptr = other;
                add_ref();
            }
        }

        void add_ref() const noexcept
        {
            if (m_ptr)
            {
                const_cast<std::remove_const_t<type>*>(m_ptr)->AddRef();
            }
        }

        void release_ref() noexcept
        {
            if (m_ptr)
            {
                unconditional_release_ref();
            }
        }

        XLANG_NOINLINE void unconditional_release_ref() noexcept
        {
            std::exchange(m_ptr, {})->Release();
        }

        template <typename U>
        friend struct com_ptr;

        type* m_ptr{};
    };

    template <typename T, typename F, typename...Args>
    impl::com_ref<T> capture(F function, Args&& ...args)
    {
        void* result{};
        check_xlang_error(function(args..., guid_of<T>(), &result));
        return { result, take_ownership_from_abi };
    }
    template <typename T, typename O, typename M, typename...Args>
    impl::com_ref<T> capture(com_ptr<O> const& object, M method, Args && ...args)
    {
        void* result{};
        check_xlang_error((object.get()->*(method))(args..., guid_of<T>(), &result));
        return { result, take_ownership_from_abi };
    }

    template <typename T>
    auto get_abi(com_ptr<T> const& object) noexcept
    {
        return object.get();
    }

    template <typename T>
    auto put_abi(com_ptr<T>& object) noexcept
    {
        return object.put_void();
    }

    template <typename T>
    void attach_abi(com_ptr<T>& object, impl::abi_t<T>* value) noexcept
    {
        object.attach(value);
    }

    template <typename T>
    auto detach_abi(com_ptr<T>& object) noexcept
    {
        return object.detach();
    }

    template <typename T>
    bool operator==(com_ptr<T> const& left, com_ptr<T> const& right) noexcept
    {
        return get_abi(left) == get_abi(right);
    }

    template <typename T>
    bool operator==(com_ptr<T> const& left, std::nullptr_t) noexcept
    {
        return get_abi(left) == nullptr;
    }

    template <typename T>
    bool operator==(std::nullptr_t, com_ptr<T> const& right) noexcept
    {
        return nullptr == get_abi(right);
    }

    template <typename T>
    bool operator!=(com_ptr<T> const& left, com_ptr<T> const& right) noexcept
    {
        return !(left == right);
    }

    template <typename T>
    bool operator!=(com_ptr<T> const& left, std::nullptr_t) noexcept
    {
        return !(left == nullptr);
    }

    template <typename T>
    bool operator!=(std::nullptr_t, com_ptr<T> const& right) noexcept
    {
        return !(nullptr == right);
    }

    template <typename T>
    bool operator<(com_ptr<T> const& left, com_ptr<T> const& right) noexcept
    {
        return get_abi(left) < get_abi(right);
    }

    template <typename T>
    bool operator>(com_ptr<T> const& left, com_ptr<T> const& right) noexcept
    {
        return right < left;
    }

    template <typename T>
    bool operator<=(com_ptr<T> const& left, com_ptr<T> const& right) noexcept
    {
        return !(right < left);
    }

    template <typename T>
    bool operator>=(com_ptr<T> const& left, com_ptr<T> const& right) noexcept
    {
        return !(left < right);
    }
}

template <typename T>
void** IID_PPV_ARGS_Helper(xlang::com_ptr<T>* ptr) noexcept
{
    return xlang::put_abi(*ptr);
}
