// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMContext.h"

namespace AppInstaller
{
    void COMContext::BeginProgress()
    {
        m_comProgressCallback(ReportType::BeginProgress, m_current, m_maximum, m_type, m_executionPhase);
    };

    void COMContext::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        SetProgress(current, maximum, type);
        m_comProgressCallback(ReportType::Progressing, m_current, m_maximum, m_type, m_executionPhase);
    }

    void COMContext::EndProgress(bool hideProgressWhenDone)
    {
        UNREFERENCED_PARAMETER(hideProgressWhenDone);
        m_comProgressCallback(ReportType::EndProgress, m_current, m_maximum, m_type, m_executionPhase);
    };

    void COMContext::SetExecutionStage(ExecutionStage executionPhase)
    {
        SetProgress(0, 0, ProgressType::None);
        m_executionPhase = executionPhase;
        m_comProgressCallback(ReportType::ExecutionPhaseUpdate, m_current, m_maximum, m_type, executionPhase);
    }

    void COMContext::SetProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        m_current = current;
        m_maximum = maximum;
        m_type = type;
    }
}