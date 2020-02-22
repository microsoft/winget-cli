// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <atomic>
#include <future>

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

        virtual void OnCanceled() = 0;

        virtual void OnCompleted() = 0;
    };

    // Future wrapper that enables progress to be hooked up by caller.
    template <typename Result, typename... Args>
    struct Future
    {
        using Task = std::packaged_task<Result, Args...>;

        Future(Task&& task, FutureProgressType type = FutureProgressType::None);

        Future(const Future&) = delete;
        Future& operator=(const Future&) = delete;

        Future(Future&&) = default;
        Future& operator=(Future&&) = default;

        // Cancel the processing of the future, if possible.
        void Cancel();

        // Gets the result, waiting as required.
        // If cancelled, result depends on promise keeper.
        T Get();

        // Gets the progress type.
        FutureProgressType GetProgressType() const { return m_progressType; }

        // Sets the progress receiver.
        // If the future has already started, the OnStarted function will be called synchronously.
        void SetProgressReceiver(IFutureProgress* receiver);

    private:
        Task m_task;
        std::shared_future m_future;
        FutureProgressType m_progressType;
        std::atomic_bool m_hasStarted;
        std::atomic_bool m_canceled;
    };
}
