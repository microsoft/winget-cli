
namespace xlang::impl
{
    template <typename T, typename H>
    struct implements_delegate : abi_t<T>, H
    {
        implements_delegate(H&& handler) : H(std::forward<H>(handler)) {}

        com_interop_result XLANG_CALL QueryInterface(guid const& id, void** result) noexcept final
        {
            if (is_guid_of<T>(id) || is_guid_of<Windows::Foundation::IUnknown>(id))
            {
                *result = static_cast<abi_t<T>*>(this);
                AddRef();
                return com_interop_result::success;
            }

            *result = nullptr;
            return com_interop_result::no_interface;
        }

        uint32_t XLANG_CALL AddRef() noexcept final
        {
            return 1 + m_references.fetch_add(1, std::memory_order_relaxed);
        }

        uint32_t XLANG_CALL Release() noexcept final
        {
            uint32_t const target = m_references.fetch_sub(1, std::memory_order_release) - 1;

            if (target == 0)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete static_cast<delegate_t<T, H>*>(this);
            }

            return target;
        }

    private:

        std::atomic<uint32_t> m_references{ 1 };
    };

    template <typename T, typename H>
    T make_delegate(H&& handler)
    {
        return { static_cast<void*>(static_cast<abi_t<T>*>(new delegate_t<T, H>(std::forward<H>(handler)))), take_ownership_from_abi };
    }

    template <typename... T>
    struct XLANG_NOVTABLE variadic_delegate_abi : unknown_abi
    {
        virtual void invoke(T const&...) = 0;
    };

    template <typename H, typename... T>
    struct variadic_delegate final : variadic_delegate_abi<T...>, H
    {
        variadic_delegate(H&& handler) : H(std::forward<H>(handler)) {}

        void invoke(T const&... args) final
        {
            (*this)(args...);
        }

        com_interop_result XLANG_CALL QueryInterface(guid const& id, void** result) noexcept final
        {
            if (is_guid_of<Windows::Foundation::IUnknown>(id))
            {
                *result = static_cast<unknown_abi*>(this);
                AddRef();
                return com_interop_result::success;
            }

            *result = nullptr;
            return com_interop_result::no_interface;
        }

        uint32_t XLANG_CALL AddRef() noexcept final
        {
            return 1 + m_references.fetch_add(1, std::memory_order_relaxed);
        }

        uint32_t XLANG_CALL Release() noexcept final
        {
            uint32_t const target = m_references.fetch_sub(1, std::memory_order_release) - 1;

            if (target == 0)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete this;
            }

            return target;
        }

    private:

        std::atomic<uint32_t> m_references{ 1 };
    };
}

namespace xlang
{
    template <typename... T>
    struct XLANG_EBO delegate : Windows::Foundation::IUnknown
    {
        delegate(std::nullptr_t = nullptr) noexcept {}
        delegate(void* ptr, take_ownership_from_abi_t) noexcept : IUnknown(ptr, take_ownership_from_abi) {}

        template <typename L>
        delegate(L handler) :
            delegate(make(std::forward<L>(handler)))
        {}

        template <typename F> delegate(F* handler) :
            delegate([=](auto&&... args) { handler(args...); })
        {}

        template <typename O, typename M> delegate(O* object, M method) :
            delegate([=](auto&&... args) { ((*object).*(method))(args...); })
        {}

        template <typename O, typename M> delegate(com_ptr<O>&& object, M method) :
            delegate([o = std::move(object), method](auto&&... args) { return ((*o).*(method))(args...); })
        {
        }

        template <typename O, typename M> delegate(weak_ref<O>&& object, M method) :
            delegate([o = std::move(object), method](auto&&... args) { if (auto s = o.get()) { ((*s).*(method))(args...); } })
        {
        }

        void operator()(T const&... args) const
        {
            (*(impl::variadic_delegate_abi<T...>**)this)->invoke(args...);
        }

    private:

        template <typename H>
        static xlang::delegate<T...> make(H&& handler)
        {
            return { static_cast<void*>(new impl::variadic_delegate<H, T...>(std::forward<H>(handler))), take_ownership_from_abi };
        }
    };
}
