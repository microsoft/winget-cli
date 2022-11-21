
namespace xlang
{
    template <typename Interface = Windows::Foundation::IActivationFactory>
    impl::com_ref<Interface> get_activation_factory(param::hstring const& name)
    {
        void* result;
        check_xlang_error(xlang_get_activation_factory(get_abi(name), guid_of<Interface>(), &result));

        return { result, take_ownership_from_abi };
    }
}

namespace xlang::impl
{
    template <typename Class, typename Interface>
    struct factory_cache_entry
    {
        template <typename F>
        auto call(F&& callback)
        {
            void* value = m_value.load(std::memory_order_acquire);
            if (value)
            {
                return callback(*reinterpret_cast<com_ref<Interface> const*>(&value));
            }

            auto object = get_activation_factory<Interface>(name_of<Class>());
            if (m_value.compare_exchange_strong(value, get_abi(object), std::memory_order_acq_rel))
            {
                value = detach_abi(object);
            }
            return callback(*reinterpret_cast<com_ref<Interface> const*>(&value));
        }

    private:
        std::atomic<void*> m_value{};
    };

    template <typename Class, typename Interface>
    struct factory_storage
    {
        static factory_cache_entry<Class, Interface> factory;
    };

    template <typename Class, typename Interface>
    factory_cache_entry<Class, Interface> factory_storage<Class, Interface>::factory;

    template <typename Class, typename Interface = Windows::Foundation::IActivationFactory, typename F>
    auto call_factory(F&& callback)
    {
        static_assert(std::is_standard_layout_v<factory_cache_entry<Class, Interface>>);

        return factory_storage<Class, Interface>::factory.call(callback);
    }

    template <typename Class, typename Interface = Windows::Foundation::IActivationFactory>
    impl::com_ref<Interface> try_get_activation_factory(xlang_error* exception = nullptr) noexcept
    {
        param::hstring const name{ name_of<Class>() };
        void* result;
        com_ptr<xlang_error_info> error_info{
            xlang_get_activation_factory(get_abi(name), guid_of<Interface>(), &result),
            take_ownership_from_abi
        };

        if (error_info != nullptr)
        {
            xlang_result error;
            error_info->GetError(&error);

            xlang_error local_exception{ error , error_info };
            if (exception)
            {
                // Optionally transfer ownership to the caller.
                *exception = std::move(local_exception);
            }
        }

        return { result, take_ownership_from_abi };
    }

    template <> struct abi<Windows::Foundation::IActivationFactory>
    {
        struct XLANG_NOVTABLE type : xlang_object_abi
        {
            virtual xlang_error_info* XLANG_CALL ActivateInstance(void** instance) noexcept = 0;
        };
    };

    template <> struct guid_storage<Windows::Foundation::IActivationFactory>
    {
        static constexpr guid value{ 0x00000035,0x0000,0x0000,{ 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 } };
    };

    template <typename D> struct produce<D, Windows::Foundation::IActivationFactory> : produce_base<D, Windows::Foundation::IActivationFactory>
    {
        xlang_error_info* XLANG_CALL ActivateInstance(void** instance) noexcept final try
        {
            *instance = nullptr;
            typename D::abi_guard guard(this->shim());
            *instance = detach_abi(this->shim().ActivateInstance());
            return nullptr;
        }
        catch (...) { return to_xlang_error(); }
    };
}

namespace xlang
{
    template <typename Class, typename Interface = Windows::Foundation::IActivationFactory>
    auto get_activation_factory()
    {
        // Normally, the callback avoids having to return a ref-counted object and the resulting AddRef/Release bump.
        // In this case we do want a unique reference, so we use the lambda to return one and thus produce an
        // AddRef'd object that is returned to the caller. 
        return impl::call_factory<Class, Interface>([](auto&& factory)
        {
            return factory;
        });
    }

    template <typename Class, typename Interface = Windows::Foundation::IActivationFactory>
    auto try_get_activation_factory() noexcept
    {
        return impl::try_get_activation_factory<Class, Interface>();
    }

    template <typename Class, typename Interface = Windows::Foundation::IActivationFactory>
    auto try_get_activation_factory(xlang_error& exception) noexcept
    {
        return impl::try_get_activation_factory<Class, Interface>(&exception);
    }

    namespace Windows::Foundation
    {
        struct IActivationFactory : IXlangObject
        {
            IActivationFactory(std::nullptr_t = nullptr) noexcept {}
            IActivationFactory(void* ptr, take_ownership_from_abi_t) noexcept : IXlangObject(ptr, take_ownership_from_abi) {}

            template <typename T>
            T ActivateInstance() const
            {
                IXlangObject instance;
                check_xlang_error((*(impl::abi_t<IActivationFactory>**)this)->ActivateInstance(put_abi(instance)));
                return instance.try_as<T>();
            }
        };
    }
}
