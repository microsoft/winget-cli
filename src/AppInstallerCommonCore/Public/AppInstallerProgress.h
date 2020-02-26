// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <atomic>

namespace AppInstaller
{
    // The semantic meaning of the progress values.
    enum class ProgressType
    {
        // Progress will not be sent.
        None,
        Bytes,
        Percent,
    };

    // Callback interface given to the worker to report to.
    // Also enables the caller to request cancellation.
    struct IProgressCallback
    {
        // Called as progress is made.
        // If maximum is 0, the maximum is unknown.
        virtual void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) = 0;

        // Returns a value indicating if the future has been cancelled.
        virtual bool IsCancelled() = 0;
    };

    // Implementation of IProgressCallback.
    struct ProgressCallback : public IProgressCallback
    {
        ProgressCallback() = default;
        ProgressCallback(IProgressCallback* callback) : m_callback(callback) {}

        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override
        {
            IProgressCallback* callback = GetCallback();
            if (callback)
            {
                callback->OnProgress(current, maximum, type);
            }
        }

        bool IsCancelled() override
        {
            return m_cancelled.load();
        }

        void Cancel()
        {
            m_cancelled = true;
        }

        IProgressCallback* GetCallback()
        {
            return m_callback.load();
        }

    private:
        std::atomic<IProgressCallback*> m_callback = nullptr;
        std::atomic_bool m_cancelled = false;
    };
}
