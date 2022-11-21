
namespace std::experimental
{
    template <typename TResult, typename... Args>
    struct coroutine_traits<xlang::Foundation::IAsyncOperation<TResult>, Args...>
    {
        struct promise_type final : xlang::impl::promise_base<promise_type,
            xlang::Foundation::IAsyncOperation<TResult>,
            xlang::Foundation::AsyncOperationCompletedHandler<TResult>>
        {
            TResult get_return_value() noexcept
            {
                return std::move(m_result);
            }

            void return_value(TResult&& value) noexcept
            {
                m_result = std::move(value);
            }

            void return_value(TResult const& value) noexcept
            {
                m_result = value;
            }

            TResult m_result{ xlang::impl::empty_value<TResult>() };
        };
    };
}
