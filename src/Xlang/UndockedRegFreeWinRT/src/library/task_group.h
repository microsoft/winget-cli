#pragma once

#include "impl/base.h"

namespace xlang
{
    struct task_group
    {
        task_group(task_group const&) = delete;
        task_group& operator=(task_group const&) = delete;

        task_group() noexcept = default;

        ~task_group() noexcept
        {
            for (auto&& task : m_tasks)
            {
                task.wait();
            }
        }

        template <typename T>
        void add(T&& callback)
        {
#if defined(XLANG_DEBUG)
            callback();
#else
            m_tasks.push_back(std::async(std::forward<T>(callback)));
#endif
        }

        void get()
        {
            auto tasks = std::move(m_tasks);

            for (auto&& task : tasks)
            {
                task.wait();
            }

            for (auto&& task : tasks)
            {
                task.get();
            }
        }

    private:

        std::vector<std::future<void>> m_tasks;
    };
}
