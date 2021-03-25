// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>
#include <atomic>
#include <functional>

namespace AppInstaller
{
    // Forward declaration
    struct ProgressCallback;
    struct IProgressCallback;

    namespace details
    {
        // For SetCancellationFunction return.
        inline void RemoveCancellationFunction(IProgressCallback* callback);
    }

    // The semantic meaning of the progress values.
    enum class ProgressType: uint32_t
    {
        // Progress will not be sent.
        None,
        Bytes,
        Percent,
    };

    // Interface that only receives progress, and does not participate in cancellation.
    // This allows a sink be simple, and let ProgressCallback handle the complications
    // of cancel state.
    struct IProgressSink
    {
        // Called as progress is made.
        // If maximum is 0, the maximum is unknown.
        virtual void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) = 0;

        // Called as progress begins.
        virtual void BeginProgress() = 0;

        // Called as progress ends.
        virtual void EndProgress(bool hideProgressWhenDone) = 0;
    };

    // Callback interface given to the worker to report to.
    // Also enables the caller to request cancellation.
    struct IProgressCallback : public IProgressSink
    {
        using CancelFunctionRemoval = wil::unique_any<IProgressCallback*, decltype(&details::RemoveCancellationFunction), details::RemoveCancellationFunction>;

        // Returns a value indicating if the future has been cancelled.
        virtual bool IsCancelled() = 0;

        // Sets a cancellation function that will be called when the operation is to be cancelled.
        [[nodiscard]] virtual CancelFunctionRemoval SetCancellationFunction(std::function<void()>&& f) = 0;
    };

    // Implementation of IProgressCallback.
    struct ProgressCallback : public IProgressCallback
    {
        ProgressCallback() = default;
        ProgressCallback(IProgressSink* sink) : m_sink(sink) {}

        void BeginProgress() override 
        {
            IProgressSink* sink = GetSink();
            if (sink)
            {
                sink->BeginProgress();
            }
        };

        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override
        {
            IProgressSink* sink = GetSink();
            if (sink)
            {
                sink->OnProgress(current, maximum, type);
            }
        }

        void EndProgress(bool hideProgressWhenDone) override
        {
            IProgressSink* sink = GetSink();
            if (sink)
            {
                sink->EndProgress(hideProgressWhenDone);
            }
        };

        bool IsCancelled() override
        {
            return m_cancelled.load();
        }

        [[nodiscard]] IProgressCallback::CancelFunctionRemoval SetCancellationFunction(std::function<void()>&& f) override
        {
            m_cancellationFunction = std::move(f);
            if (m_cancellationFunction)
            {
                return IProgressCallback::CancelFunctionRemoval(this);
            }
            else
            {
                return {};
            }
        }

        void Cancel()
        {
            m_cancelled = true;
            if (m_cancellationFunction)
            {
                m_cancellationFunction();
            }
        }

        IProgressSink* GetSink()
        {
            return m_sink.load();
        }

    private:
        std::atomic<IProgressSink*> m_sink = nullptr;
        std::atomic_bool m_cancelled = false;
        std::function<void()> m_cancellationFunction;
    };

    namespace details
    {
        inline void RemoveCancellationFunction(IProgressCallback* callback)
        {
            (void)callback->SetCancellationFunction(nullptr);
        }
    }
}
