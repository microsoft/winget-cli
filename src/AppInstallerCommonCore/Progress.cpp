// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerProgress.h"

namespace AppInstaller
{
    ProgressCallback::ProgressCallback(IProgressSink* sink) : m_sink(sink)
    {
    }

    void ProgressCallback::BeginProgress()
    {
        IProgressSink* sink = GetSink();
        if (sink)
        {
            sink->BeginProgress();
        }
    }

    void ProgressCallback::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        IProgressSink* sink = GetSink();
        if (sink)
        {
            sink->OnProgress(current, maximum, type);
        }
    }

    void ProgressCallback::SetProgressMessage(std::string_view message)
    {
        IProgressSink* sink = GetSink();
        if (sink)
        {
            sink->SetProgressMessage(message);
        }
    }

    void ProgressCallback::EndProgress(bool hideProgressWhenDone)
    {
        IProgressSink* sink = GetSink();
        if (sink)
        {
            sink->EndProgress(hideProgressWhenDone);
        }
    };

    bool ProgressCallback::IsCancelledBy(CancelReason cancelReasons)
    {
        THROW_HR_IF(E_UNEXPECTED, cancelReasons == CancelReason::None);
        return WI_IsAnyFlagSet(cancelReasons, m_cancelReason);
    }

    [[nodiscard]] IProgressCallback::CancelFunctionRemoval ProgressCallback::SetCancellationFunction(std::function<void()>&& f)
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

    bool ProgressCallback::Wait(IProgressCallback& progress, std::chrono::milliseconds millisecondsToWait)
    {
        wil::unique_event calledEvent;
        calledEvent.create();

        auto cancellationFunc = progress.SetCancellationFunction([&calledEvent]()
            {
                calledEvent.SetEvent();
            });

        if (calledEvent.wait(static_cast<DWORD>(millisecondsToWait.count())))
        {
            return false;
        }

        return true;
    }

    void ProgressCallback::Cancel(CancelReason reason)
    {
        m_cancelReason = reason;
        if (m_cancellationFunction)
        {
            m_cancellationFunction();
        }
    }

    IProgressSink* ProgressCallback::GetSink()
    {
        return m_sink.load();
    }

    PartialPercentProgressCallback::PartialPercentProgressCallback(IProgressCallback& baseCallback, uint64_t globalMax) :
        m_baseCallback(baseCallback), m_globalMax(globalMax)
    {
    }

    void PartialPercentProgressCallback::BeginProgress()
    {
        THROW_HR(E_NOTIMPL);
    }

    void PartialPercentProgressCallback::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        THROW_HR_IF(E_UNEXPECTED, ProgressType::Percent != type);

        m_baseCallback.OnProgress(m_rangeMin + (m_rangeMax - m_rangeMin) * current / maximum, m_globalMax, type);
    }

    void PartialPercentProgressCallback::SetProgressMessage(std::string_view message)
    {
        m_baseCallback.SetProgressMessage(message);
    }

    void PartialPercentProgressCallback::EndProgress(bool)
    {
        THROW_HR(E_NOTIMPL);
    }

    IProgressCallback::CancelFunctionRemoval PartialPercentProgressCallback::SetCancellationFunction(std::function<void()>&& f)
    {
        return m_baseCallback.SetCancellationFunction(std::move(f));
    }

    void PartialPercentProgressCallback::SetRange(uint64_t rangeMin, uint64_t rangeMax)
    {
        THROW_HR_IF(E_INVALIDARG, rangeMin > rangeMax || rangeMax > m_globalMax);
        m_rangeMin = rangeMin;
        m_rangeMax = rangeMax;
    }
}
