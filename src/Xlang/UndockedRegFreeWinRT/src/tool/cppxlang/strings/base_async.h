
namespace xlang::impl
{
    template <typename Async>
    void blocking_suspend(Async const& async)
    {
        std::mutex m;
        std::condition_variable cv;
        bool completed = false;
        async.Completed([&](auto && ...)
            {
                {
                    std::lock_guard const guard(m);
                    completed = true;
                }
                cv.notify_one();
            });

        std::unique_lock guard(m);
        cv.wait(m, [&] { return completed; });
    }
}
