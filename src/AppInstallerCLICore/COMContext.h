// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerProgress.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"

namespace AppInstaller::CLI::Execution
{
    enum class ReportType : uint32_t
    {
        ExecutionPhaseUpdate,
        BeginProgress,
        Progressing,
        EndProgress,
    };

    class NullStreamBuf : public std::streambuf {};

    struct NullStream
    {
        NullStream();

        ~NullStream() = default;

    protected:
        NullStreamBuf m_nullStreamBuf;
        std::unique_ptr<std::ostream> m_nullOut;
        std::unique_ptr<std::istream> m_nullIn;
    };

    typedef std::function<void(ReportType reportType, uint64_t current, uint64_t maximum, ProgressType progressType, CLI::Workflow::ExecutionStage executionPhase)> ProgressCallBackFunction;

    // NullStream constructs the Stream parameters for Context constructor
    // Hence, NullStream should always precede Context in base class order of COMContext's inheritance
    struct COMContext : IProgressSink, NullStream, CLI::Execution::Context
    {
        // When no Console streams need involvement, construct NullStreams instead to pass to Context
        COMContext() : NullStream(), CLI::Execution::Context(*m_nullOut, *m_nullIn)
        {
            Reporter.SetChannel(Reporter::Channel::Disabled);
            Reporter.SetProgressSink(this);
            SetFlags(CLI::Execution::ContextFlag::AgreementsAcceptedByCaller);
        }

        COMContext(std::ostream& out, std::istream& in) : CLI::Execution::Context(out, in)
        {
            Reporter.SetProgressSink(this);
            SetFlags(CLI::Execution::ContextFlag::AgreementsAcceptedByCaller);
        }

        ~COMContext() = default;

        // IProgressSink
        void BeginProgress() override;
        void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override;
        void EndProgress(bool) override;

        //Execution::Context
        void SetExecutionStage(CLI::Workflow::ExecutionStage executionPhase);

        CLI::Workflow::ExecutionStage GetExecutionStage() const { return m_executionStage; }

        void AddProgressCallbackFunction(ProgressCallBackFunction&& f);

        // Set Diagnostic and Telemetry loggers, Wil failure callback
        // This should be called only once per COM Server instance
        static void SetLoggers();

        // Set COM call context for diagnostic and telemetry loggers
        // This should be called for every COMContext object instance
        void SetContextLoggers(const std::wstring_view telemetryCorrelationJson, const std::string& caller);

        std::wstring_view GetCorrelationJson();

    private:
        void FireCallbacks(ReportType reportType, uint64_t current, uint64_t maximum, ProgressType progressType, ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase);

        CLI::Workflow::ExecutionStage m_executionStage = CLI::Workflow::ExecutionStage::Initial;
        std::vector<ProgressCallBackFunction> m_comProgressCallbacks;
        std::wstring m_correlationData = L"";
        std::mutex m_callbackLock;
    };
}
