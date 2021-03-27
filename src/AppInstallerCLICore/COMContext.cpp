// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMContext.h"

namespace AppInstaller
{

    NullStream::NullStream()
    {
        m_nullOut.reset(new std::ostream(&m_nullStreamBuf));
        m_nullIn.reset(new std::istream(&m_nullStreamBuf));
    }

    void COMContext::BeginProgress()
    {
        m_comProgressCallback(ReportType::BeginProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::OnProgress(uint64_t current, uint64_t maximum, ProgressType progressType)
    {
        m_comProgressCallback(ReportType::Progressing, current, maximum, progressType, m_executionStage);
    }

    void COMContext::EndProgress(bool)
    {
        m_comProgressCallback(ReportType::EndProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::SetExecutionStage(CLI::Workflow::ExecutionStage executionStage, bool)
    {
        m_executionStage = executionStage;
        m_comProgressCallback(ReportType::ExecutionPhaseUpdate, 0, 0, ProgressType::None, m_executionStage);
    }
}