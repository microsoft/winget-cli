
namespace xlang
{
    struct event_token
    {
        int64_t value{};

        explicit operator bool() const noexcept
        {
            return value != 0;
        }
    };

    inline bool operator==(event_token const& left, event_token const& right) noexcept
    {
        return left.value == right.value;
    }

    struct auto_revoke_t {};
    inline constexpr auto_revoke_t auto_revoke{};

    template <typename I>
    struct event_revoker
    {
        using method_type = int32_t(XLANG_CALL impl::abi_t<I>::*)(xlang::event_token);

        event_revoker() noexcept = default;
        event_revoker(event_revoker const&) = delete;
        event_revoker& operator=(event_revoker const&) = delete;
        event_revoker(event_revoker&&) noexcept = default;

        event_revoker& operator=(event_revoker&& other) noexcept
        {
            if (this != &other)
            {
                revoke();
                m_object = std::move(other.m_object);
                m_method = other.m_method;
                m_token = other.m_token;
            }

            return *this;
        }

        template <typename U>
        event_revoker(U&& object, method_type method, event_token token) :
            m_object(std::forward<U>(object)),
            m_method(method),
            m_token(token)
        {}

        ~event_revoker() noexcept
        {
            revoke();
        }

        void revoke() noexcept
        {
            if (I object = std::exchange(m_object, {}).get())
            {
                ((*reinterpret_cast<impl::abi_t<I>**>(&object))->*(m_method))(m_token);
            }
        }

        explicit operator bool() const noexcept
        {
            return m_object ? true : false;
        }

    private:

        weak_ref<I> m_object;
        method_type m_method{};
        event_token m_token{};
    };

    template <typename I>
    struct factory_event_revoker
    {
        using method_type = int32_t(XLANG_CALL impl::abi_t<I>::*)(xlang::event_token);

        factory_event_revoker() noexcept = default;
        factory_event_revoker(factory_event_revoker const&) = delete;
        factory_event_revoker& operator=(factory_event_revoker const&) = delete;
        factory_event_revoker(factory_event_revoker&&) noexcept = default;

        factory_event_revoker& operator=(factory_event_revoker&& other) noexcept
        {
            if (this != &other)
            {
                revoke();
                m_object = std::move(other.m_object);
                m_method = other.m_method;
                m_token = other.m_token;
            }

            return *this;
        }

        template <typename U>
        factory_event_revoker(U&& object, method_type method, event_token token) noexcept :
            m_object(std::forward<U>(object)),
            m_method(method),
            m_token(token)
        {}

        ~factory_event_revoker() noexcept
        {
            revoke();
        }

        void revoke() noexcept
        {
            if (auto object = std::exchange(m_object, {}))
            {
                ((*reinterpret_cast<impl::abi_t<I>**>(&object))->*(m_method))(m_token);
            }
        }

        explicit operator bool() const noexcept
        {
            return m_object ? true : false;
        }

    private:

        I m_object;
        method_type m_method{};
        event_token m_token{};
    };
}

namespace xlang::impl
{
    template <typename I, auto Method>
    struct event_revoker
    {
        event_revoker() noexcept = default;
        event_revoker(event_revoker const&) = delete;
        event_revoker& operator=(event_revoker const&) = delete;

        event_revoker(event_revoker&&) noexcept = default;
        event_revoker& operator=(event_revoker&& other) noexcept
        {
            event_revoker(std::move(other)).swap(*this);
            return *this;
        }

        event_revoker(I const& object, event_token token)
            : m_object(object)
            , m_token(token)
        {}

        operator xlang::event_revoker<I>() && noexcept
        {
            return { std::move(m_object), Method, m_token };
        }

        ~event_revoker() noexcept
        {
            if (m_object)
            {
                revoke_impl(m_object.get());
            }
        }

        void swap(event_revoker& other) noexcept
        {
            std::swap(m_object, other.m_object);
            std::swap(m_token, other.m_token);
        }

        void revoke() noexcept
        {
            revoke_impl(std::exchange(m_object, {}).get());
        }

        explicit operator bool() const noexcept
        {
            return bool{ m_object };
        }

    private:
        void revoke_impl(I object) noexcept
        {
            if (object)
            {
                ((*reinterpret_cast<impl::abi_t<I>**>(&object))->*(Method))(m_token);
            }
        }

        xlang::weak_ref<I> m_object{};
        event_token m_token{};
    };

    template <typename I, auto Method>
    struct factory_event_revoker
    {
        factory_event_revoker() noexcept = default;
        factory_event_revoker(factory_event_revoker const&) = delete;
        factory_event_revoker& operator=(factory_event_revoker const&) = delete;

        factory_event_revoker(factory_event_revoker&&) noexcept = default;
        factory_event_revoker& operator=(factory_event_revoker&& other) noexcept
        {
            factory_event_revoker(std::move(other)).swap(*this);
            return *this;
        }
        factory_event_revoker(I const& object, event_token token)
            : m_object(object)
            , m_token(token)
        {}

        operator xlang::factory_event_revoker<I>() && noexcept
        {
            return { std::move(m_object), Method, m_token };
        }

        ~factory_event_revoker() noexcept
        {
            if (m_object)
            {
                revoke_impl(m_object);
            }
        }

        void swap(factory_event_revoker& other) noexcept
        {
            std::swap(m_object, other.m_object);
            std::swap(m_token, other.m_token);
        }

        void revoke() noexcept
        {
            revoke_impl(std::exchange(m_object, {}));
        }

        explicit operator bool() const noexcept
        {
            return bool{ m_object };
        }

    private:
        void revoke_impl(I object) noexcept
        {
            if (object)
            {
                ((*reinterpret_cast<impl::abi_t<I>**>(&object))->*(Method))(m_token);
            }
        }
    private:
        I m_object;
        event_token m_token{};
    };

    template <typename D, typename Revoker, typename S>
    Revoker make_event_revoker(S source, event_token token)
    {
        return { static_cast<D const&>(*source), token };
    }

    template <typename T>
    struct event_array
    {
        using value_type = T;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator = value_type*;

        explicit event_array(uint32_t const count) noexcept : m_size(count)
        {
            std::uninitialized_fill_n(data(), count, value_type());
        }

        unsigned long AddRef() noexcept
        {
            return 1 + m_references.fetch_add(1, std::memory_order_relaxed);
        }

        unsigned long Release() noexcept
        {
            uint32_t const remaining = m_references.fetch_sub(1, std::memory_order_release) - 1;

            if (remaining == 0)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                this->~event_array();
                ::operator delete(static_cast<void*>(this));
            }

            return remaining;
        }

        reference back() noexcept
        {
            XLANG_ASSERT(m_size > 0);
            return*(data() + m_size - 1);
        }

        iterator begin() noexcept
        {
            return data();
        }

        iterator end() noexcept
        {
            return data() + m_size;
        }

        uint32_t size() const noexcept
        {
            return m_size;
        }

        ~event_array() noexcept
        {
            std::destroy(begin(), end());
        }

    private:

        pointer data() noexcept
        {
            return reinterpret_cast<pointer>(this + 1);
        }

        std::atomic<uint32_t> m_references{ 1 };
        uint32_t m_size{ 0 };
    };

    template <typename T>
    com_ptr<event_array<T>> make_event_array(uint32_t const capacity)
    {
        void* raw = ::operator new(sizeof(event_array<T>) + (sizeof(T)* capacity));
#pragma warning(suppress: 6386)
        return { new(raw) event_array<T>(capacity), take_ownership_from_abi };
    }
}

namespace xlang
{
    template <typename Delegate>
    struct event
    {
        using delegate_type = Delegate;

        event() = default;
        event(event<Delegate> const&) = delete;
        event<Delegate>& operator =(event<Delegate> const&) = delete;

        explicit operator bool() const noexcept
        {
            return m_targets != nullptr;
        }

        event_token add(delegate_type const& delegate)
        {
            event_token token{};

            // Extends life of old targets array to release delegates outside of lock.
            delegate_array temp_targets;

            {
                std::lock_guard const change_guard(m_change);
                delegate_array new_targets = impl::make_event_array<delegate_type>((!m_targets) ? 1 : m_targets->size() + 1);

                if (m_targets)
                {
                    std::copy_n(m_targets->begin(), m_targets->size(), new_targets->begin());
                }

                new_targets->back() = delegate;
                token = get_token(new_targets->back());

                std::lock_guard const swap_guard(m_swap);
                temp_targets = std::exchange(m_targets, std::move(new_targets));
            }

            return token;
        }

        void remove(event_token const token)
        {
            // Extends life of old targets array to release delegates outside of lock.
            delegate_array temp_targets;

            {
                std::lock_guard const change_guard(m_change);

                if (!m_targets)
                {
                    return;
                }

                uint32_t available_slots = m_targets->size() - 1;
                delegate_array new_targets;
                bool removed = false;

                if (available_slots == 0)
                {
                    if (get_token(*m_targets->begin()) == token)
                    {
                        removed = true;
                    }
                }
                else
                {
                    new_targets = impl::make_event_array<delegate_type>(available_slots);
                    auto new_iterator = new_targets->begin();

                    for (delegate_type const& element : *m_targets)
                    {
                        if (!removed && token == get_token(element))
                        {
                            removed = true;
                            continue;
                        }

                        if (available_slots == 0)
                        {
                            XLANG_ASSERT(!removed);
                            break;
                        }

                        *new_iterator = element;
                        ++new_iterator;
                        --available_slots;
                    }
                }

                if (removed)
                {
                    std::lock_guard const swap_guard(m_swap);
                    temp_targets = std::exchange(m_targets, std::move(new_targets));
                }
            }
        }

        template<typename...Arg>
        void operator()(Arg const&... args)
        {
            delegate_array temp_targets;

            {
                std::lock_guard const swap_guard(m_swap);
                temp_targets = m_targets;
            }

            if (temp_targets)
            {
                for (delegate_type const& element : *temp_targets)
                {
                    try
                    {
                        element(args...);
                    }
                    catch (xlang_error const& e)
                    {
                    }
                }
            }
        }

    private:

        event_token get_token(delegate_type const& delegate) const noexcept
        {
            return event_token{ reinterpret_cast<int64_t>(get_abi(delegate)) };
        }

        using delegate_array = com_ptr<impl::event_array<delegate_type>>;

        delegate_array m_targets;
        std::mutex m_swap;
        std::mutex m_change;
    };
}
