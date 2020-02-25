// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>

#include <atomic>
#include <future>
#include <optional>
#include <type_traits>
#include <utility>

namespace AppInstaller
{
    // The semantic meaning of the progress values.
    enum class FutureProgressType
    {
        // Progress will not be sent.
        None,
        Bytes,
        Percent,
    };

    // Callback interface for receiving progress data.
    struct IFutureProgress
    {
        // Called when the future has started processing.
        virtual void OnStarted() = 0;

        // Called as progress is made.
        // If maximum is 0, the maximum is unknown.
        virtual void OnProgress(uint64_t current, uint64_t maximum, FutureProgressType type) = 0;

        // Called when the future is done processing.
        virtual void OnCompleted(bool cancelled) = 0;
    };

    // Callback interface given to the promise keeper to work with.
    struct IPromiseKeeperProgress
    {
        // Called as progress is made.
        // If maximum is 0, the maximum is unknown.
        virtual void OnProgress(uint64_t current, uint64_t maximum, FutureProgressType type) = 0;

        // Returns a value indicating if the future has been cancelled.
        virtual bool IsCancelled() = 0;
    };

    namespace details
    {
        struct PromiseKeeperProgress : public IPromiseKeeperProgress
        {
            void OnStarted()
            {
                IFutureProgress* receiver = GetFutureProgress();
                if (receiver)
                {
                    receiver->OnStarted();
                }
            }

            void OnProgress(uint64_t current, uint64_t maximum, FutureProgressType type) override
            {
                IFutureProgress* receiver = GetFutureProgress();
                if (receiver)
                {
                    receiver->OnProgress(current, maximum, type);
                }
            }

            void OnCompleted(bool cancelled)
            {
                IFutureProgress* receiver = GetFutureProgress();
                if (receiver)
                {
                    receiver->OnCompleted(cancelled);
                }
            }

            bool IsCancelled() override
            {
                return m_cancelled.load();
            }

            IFutureProgress* GetFutureProgress()
            {
                return m_receiver.load();
            }

            std::atomic<IFutureProgress*> m_receiver = nullptr;
            std::atomic_bool m_cancelled = false;
        };

        // Helper to determine template type info.
        template <typename T>
        struct TemplateDeduction
        {
            using t = T;
        };

        template <template<typename> typename T, typename U>
        struct TemplateDeduction<T<U>>
        {
            using t = T<void>;
        };

        template <typename T>
        using TemplateDeduction_t = typename TemplateDeduction<T>::t;
    }

    // Future wrapper that enables progress to be hooked up by caller.
    template <typename Result>
    struct Future
    {
        using Task = std::packaged_task<Result(IPromiseKeeperProgress*)>;
        // If the incoming Result type is void or std::optional, just use that as the return type for Get.
        // Otherwise, wrap it in std::optional.
        using GetResult = std::conditional_t<std::is_same_v<void, Result> || std::is_same_v<std::optional<void>, details::TemplateDeduction_t<Result>>, Result, std::optional<Result>>;

        Future(Task&& task) : m_task(std::move(task)) {}

        template <typename F>
        explicit Future(F&& f) : m_task(std::move(f)) {}

        Future(const Future&) = delete;
        Future& operator=(const Future&) = delete;

        Future(Future&&) = default;
        Future& operator=(Future&&) = default;

        // Cancel the processing of the future, if possible.
        void Cancel() { m_progress.m_cancelled = true; }

        // Gets the result, waiting as required.
        // If cancelled, result depends on promise keeper.
        GetResult Get()
        {
            m_progress.OnStarted();

            std::future future = m_task.get_future();
            m_task(&m_progress);

            auto scopeExit = wil::scope_exit([&]()
                {
                    if (m_progress.IsCancelled())
                    {
                        m_progress.OnCompleted(true);
                    }
                    else
                    {
                        m_progress.OnCompleted(false);
                    }
                });

            future.wait();

            if (m_progress.IsCancelled())
            {
                return HandleDefaultReturn<GetResult>::Return();
            }
            else
            {
                return future.get();
            }
        }

        // Sets the progress receiver.
        void SetProgressReceiver(IFutureProgress* receiver) { m_progress.m_receiver = receiver; }

    private:
        template <typename T>
        struct HandleDefaultReturn { static T Return() { return T{}; } };

        template <>
        struct HandleDefaultReturn<void> { static void Return() {} };

        Task m_task;
        details::PromiseKeeperProgress m_progress;
    };
}
