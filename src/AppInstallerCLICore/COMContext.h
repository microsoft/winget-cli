#pragma once
#include "pch.h"
#include "..\AppInstallerCommonCore\Public\AppInstallerProgress.h"
#include "ExecutionContext.h"

namespace AppInstaller
{
	using namespace AppInstaller::CLI;

    enum ReportType
    {
        BeginProgress,
        Progressing,
        EndProgress,
        ExecutionPhaseUpdate,
    };

	struct COMContext : IProgressSink, Execution::Context
	{
		COMContext(std::ostream& out, std::istream& in) : Execution::Context(out, in) 
        {
            Reporter.SetProgressSink(this);
        }

		~COMContext() {}

        // IProgressSink
        void BeginProgress() override
        {
            m_comProgressCallback((uint32_t)ReportType::BeginProgress, m_current, m_maximum, (uint32_t)m_type, m_executionPhase);
        };

        // IProgressSink
        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override
        {
            SetProgress(current, maximum, type);
            m_comProgressCallback((uint32_t)ReportType::Progressing, m_current, m_maximum, (uint32_t)m_type, m_executionPhase);
        }

        // IProgressSink
        void EndProgress() override
        {
            m_comProgressCallback((uint32_t)ReportType::EndProgress, m_current, m_maximum, (uint32_t)m_type, m_executionPhase);
        };

        // Execution::Context
        void SetExecutionStage(uint32_t executionPhase) override
        {
            SetProgress(0,0,ProgressType::None);
            m_executionPhase = executionPhase;
            m_comProgressCallback((uint32_t)ReportType::ExecutionPhaseUpdate, m_current, m_maximum, (uint32_t)m_type, executionPhase);
        }

        void SetProgressCallbackFunction(std::function<void(uint32_t reportType, uint64_t current, uint64_t maximum, uint32_t progressType, uint32_t executionPhase)>&& f)
        {
            m_comProgressCallback = std::move(f);
        }

	private:
        void SetProgress(uint64_t current, uint64_t maximum, ProgressType type)
        {
            m_current = current;
            m_maximum = maximum;
            m_type = type;
        }

		std::function<void(uint32_t reportType, uint64_t current, uint64_t maximum, uint32_t progressType, uint32_t executionPhase)> m_comProgressCallback;
        uint64_t m_current = 0;
        uint64_t m_maximum = 0;
        ProgressType m_type = ProgressType::None;
        uint32_t m_executionPhase = 0; // Instead add Worfklow::ExecutionStage::None and set that as default here
	};
}