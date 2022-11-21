
#ifdef _WIN32
namespace xlang
{
    [[nodiscard]] inline auto resume_background() noexcept
    {
        struct awaitable
        {
            bool await_ready() const noexcept
            {
                return false;
            }

            void await_resume() const noexcept
            {
            }

            void await_suspend(std::experimental::coroutine_handle<> handle) const
            {
                if (!XLANG_TrySubmitThreadpoolCallback(callback, handle.address(), nullptr))
                {
                    throw_last_error();
                }
            }

        private:

            static void XLANG_CALL callback(void*, void* context) noexcept
            {
                std::experimental::coroutine_handle<>::from_address(context)();
            }
        };

        return awaitable{};
    }

    template <typename T>
    [[nodiscard]] auto resume_background(T const& context) noexcept
    {
        struct awaitable
        {
            awaitable(T const& context) : m_context(context)
            {
            }

            bool await_ready() const noexcept
            {
                return false;
            }

            void await_resume() const noexcept
            {
            }

            void await_suspend(std::experimental::coroutine_handle<> resume)
            {
                m_resume = resume;

                if (!XLANG_TrySubmitThreadpoolCallback(callback, this, nullptr))
                {
                    throw_last_error();
                }
            }

        private:

            static void XLANG_CALL callback(void*, void* context) noexcept
            {
                auto that = static_cast<awaitable*>(context);
                auto guard = that->m_context();
                that->m_resume();
            }

            T const& m_context;
            std::experimental::coroutine_handle<> m_resume{ nullptr };
        };

        return awaitable{ context };
    }

    struct apartment_context
    {
        apartment_context()
        {
            m_context.capture(XLANG_CoGetObjectContext);
        }

        bool await_ready() const noexcept
        {
            return false;
        }

        void await_resume() const noexcept
        {
        }

        void await_suspend(std::experimental::coroutine_handle<> handle) const
        {
            impl::com_callback_args args{};
            args.data = handle.address();
            check_xlang_error(m_context->ContextCallback(callback, &args, guid_of<impl::ICallbackWithNoReentrancyToApplicationSTA>(), 5, nullptr));
        }

    private:

        static int32_t XLANG_CALL callback(impl::com_callback_args* args) noexcept
        {
            std::experimental::coroutine_handle<>::from_address(args->data)();
            return impl::error_ok;
        }

        com_ptr<impl::IContextCallback> m_context;
    };

    [[nodiscard]] inline auto resume_after(Foundation::TimeSpan duration) noexcept
    {
        struct awaitable
        {
            explicit awaitable(Foundation::TimeSpan duration) noexcept :
                m_duration(duration)
            {
            }

            bool await_ready() const noexcept
            {
                return m_duration.count() <= 0;
            }

            void await_suspend(std::experimental::coroutine_handle<> handle)
            {
                m_timer.attach(check_pointer(XLANG_CreateThreadpoolTimer(callback, handle.address(), nullptr)));
                int64_t relative_count = -m_duration.count();
                XLANG_SetThreadpoolTimer(m_timer.get(), &relative_count, 0, 0);
            }

            void await_resume() const noexcept
            {
            }

        private:

            static void XLANG_CALL callback(void*, void* context, void*) noexcept
            {
                std::experimental::coroutine_handle<>::from_address(context)();
            }

            struct timer_traits
            {
                using type = impl::ptp_timer;

                static void close(type value) noexcept
                {
                    XLANG_CloseThreadpoolTimer(value);
                }

                static constexpr type invalid() noexcept
                {
                    return nullptr;
                }
            };

            handle_type<timer_traits> m_timer;
            Foundation::TimeSpan m_duration;
        };

        return awaitable{ duration };
    }

#ifdef _RESUMABLE_FUNCTIONS_SUPPORTED
    inline auto operator co_await(Foundation::TimeSpan duration)
    {
        return resume_after(duration);
    }
#endif

    [[nodiscard]] inline auto resume_on_signal(void* handle, Foundation::TimeSpan timeout = {}) noexcept
    {
        struct awaitable
        {
            awaitable(void* handle, Foundation::TimeSpan timeout) noexcept :
                m_timeout(timeout),
                m_handle(handle)
            {}

            bool await_ready() const noexcept
            {
                return XLANG_WaitForSingleObject(m_handle, 0) == 0;
            }

            void await_suspend(std::experimental::coroutine_handle<> resume)
            {
                m_resume = resume;
                m_wait.attach(check_pointer(XLANG_CreateThreadpoolWait(callback, this, nullptr)));
                int64_t relative_count = -m_timeout.count();
                int64_t* file_time = relative_count != 0 ? &relative_count : nullptr;
                XLANG_SetThreadpoolWait(m_wait.get(), m_handle, file_time);
            }

            bool await_resume() const noexcept
            {
                return m_result == 0;
            }

        private:

            static void XLANG_CALL callback(void*, void* context, void*, uint32_t result) noexcept
            {
                auto that = static_cast<awaitable*>(context);
                that->m_result = result;
                that->m_resume();
            }

            struct wait_traits
            {
                using type = impl::ptp_wait;

                static void close(type value) noexcept
                {
                    XLANG_CloseThreadpoolWait(value);
                }

                static constexpr type invalid() noexcept
                {
                    return nullptr;
                }
            };

            handle_type<wait_traits> m_wait;
            Foundation::TimeSpan m_timeout;
            void* m_handle;
            uint32_t m_result{};
            std::experimental::coroutine_handle<> m_resume{ nullptr };
        };

        return awaitable{ handle, timeout };
    }

    [[nodiscard]] inline auto resume_foreground(
        Windows::UI::Core::CoreDispatcher const& dispatcher,
        Windows::UI::Core::CoreDispatcherPriority const priority = Windows::UI::Core::CoreDispatcherPriority::Normal) noexcept
    {
        struct awaitable
        {
            awaitable(Windows::UI::Core::CoreDispatcher const& dispatcher, Windows::UI::Core::CoreDispatcherPriority const priority) noexcept :
                m_dispatcher(dispatcher),
                m_priority(priority)
            {
            }

            bool await_ready() const
            {
                return m_dispatcher.HasThreadAccess();
            }

            void await_resume() const noexcept
            {
            }

            void await_suspend(std::experimental::coroutine_handle<> handle) const
            {
                m_dispatcher.RunAsync(m_priority, [handle]
                    {
                        handle();
                    });
            }

        private:

            Windows::UI::Core::CoreDispatcher const& m_dispatcher;
            Windows::UI::Core::CoreDispatcherPriority const m_priority;
        };

        return awaitable{ dispatcher, priority };
    };

    [[nodiscard]] inline auto resume_foreground(
        Windows::System::DispatcherQueue const& dispatcher,
        Windows::System::DispatcherQueuePriority const priority = Windows::System::DispatcherQueuePriority::Normal) noexcept
    {
        struct awaitable
        {
            awaitable(Windows::System::DispatcherQueue const& dispatcher, Windows::System::DispatcherQueuePriority const priority) noexcept :
                m_dispatcher(dispatcher),
                m_priority(priority)
            {
            }

            bool await_ready() const noexcept
            {
                return false;
            }

            bool await_resume() const noexcept
            {
                return m_queued;
            }

            bool await_suspend(std::experimental::coroutine_handle<> handle)
            {
                m_queued = m_dispatcher.TryEnqueue(m_priority, [handle]
                    {
                        handle();
                    });

                return m_queued;
            }

        private:

            Windows::System::DispatcherQueue const& m_dispatcher;
            Windows::System::DispatcherQueuePriority const m_priority;
            bool m_queued{};
        };

        return awaitable{ dispatcher, priority };
    };
}

#endif
