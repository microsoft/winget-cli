
namespace std::experimental
{
    template <typename... Args>
    struct coroutine_traits<xlang::Foundation::IAsyncAction, Args...>
    {
        struct promise_type final : xlang::impl::promise_base<promise_type,
            xlang::Foundation::IAsyncAction,
            xlang::Foundation::AsyncActionCompletedHandler>
        {
            void return_void() const noexcept
            {
            }
        };
    };
}
