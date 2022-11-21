
namespace xlang
{
    struct fire_and_forget {};
}

namespace std::experimental
{
    template <typename... Args>
    struct coroutine_traits<xlang::fire_and_forget, Args...>
    {
        struct promise_type
        {
            xlang::fire_and_forget get_return_object() const noexcept
            {
                return{};
            }

            void return_void() const noexcept
            {
            }

            suspend_never initial_suspend() const noexcept
            {
                return{};
            }

            suspend_never final_suspend() const noexcept
            {
                return{};
            }

            void unhandled_exception() const noexcept
            {
                std::terminate();
            }
        };
    };
}
