// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>
#include <atomic>
#include <functional>
#include <string_view>

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

        // Sets a message for the current progress state.
        virtual void SetProgressMessage(std::string_view message) = 0;

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
        ProgressCallback(IProgressSink* sink);

        void BeginProgress() override;

        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;

        void SetProgressMessage(std::string_view message) override;

        void EndProgress(bool hideProgressWhenDone) override;

        bool IsCancelled() override;

        [[nodiscard]] IProgressCallback::CancelFunctionRemoval SetCancellationFunction(std::function<void()>&& f) override;

        void Cancel();

        IProgressSink* GetSink();

    private:
        std::atomic<IProgressSink*> m_sink = nullptr;
        std::atomic_bool m_cancelled = false;
        std::function<void()> m_cancellationFunction;
    };

    // A progress callback that reports its progress as a partial range of percentage to its base progress callback
    struct PartialPercentProgressCallback : public ProgressCallback
    {
        PartialPercentProgressCallback(IProgressCallback& baseCallback, uint64_t globalMax);

        void BeginProgress() override;

        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;

        void SetProgressMessage(std::string_view message) override;

        void EndProgress(bool hideProgressWhenDone) override;

        [[nodiscard]] IProgressCallback::CancelFunctionRemoval SetCancellationFunction(std::function<void()>&& f) override;

        void SetRange(uint64_t rangeMin, uint64_t rangeMax);

    private:
        IProgressCallback& m_baseCallback;
        uint64_t m_rangeMin = 0;
        uint64_t m_rangeMax = 0;
        uint64_t m_globalMax = 0;
    };

    namespace details
    {
        inline void RemoveCancellationFunction(IProgressCallback* callback)
        {
            (void)callback->SetCancellationFunction(nullptr);
        }
    }
}
