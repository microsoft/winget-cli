// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <atomic>
#include <future>
#include <optional>

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
    struct IPromiseKeeper
    {
        // Called as progress is made.
        // If maximum is 0, the maximum is unknown.
        virtual void OnProgress(uint64_t current, uint64_t maximum, FutureProgressType type) = 0;

        // Returns a value indicating if the future has been cancelled.
        virtual bool IsCancelled() = 0;
    };

    namespace details
    {
        
    }

    // Future wrapper that enables progress to be hooked up by caller.
    template <typename Result>
    struct Future
    {
        using Task = std::packaged_task<Result(IPromiseKeeper&)>;

        Future(Task&& task) : m_task(std::move(task)) {}

        Future(const Future&) = delete;
        Future& operator=(const Future&) = delete;

        Future(Future&&) = default;
        Future& operator=(Future&&) = default;

        // Cancel the processing of the future, if possible.
        void Cancel() { m_canceled = true; }

        // Gets the result, waiting as required.
        // If cancelled, result depends on promise keeper.
        std::optional<Result> Get()
        {

        }

        // Sets the progress receiver.
        void SetProgressReceiver(IFutureProgress* receiver) { m_receiver = receiver; }

    private:
        Task m_task;
        std::future m_future;
        std::atomic<IFutureProgress*> m_receiver = nullptr;
        std::atomic_bool m_canceled = false;
    };
}
