// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationProcessor.h"
#include "ConfigurationProcessor.g.cpp"
#include "ConfigurationSet.h"
#include "OpenConfigurationSetResult.h"
#include "ConfigurationSetParser.h"
#include "DiagnosticInformationInstance.h"
#include "ApplyConfigurationSetResult.h"
#include "ApplyConfigurationUnitResult.h"
#include "TestConfigurationSetResult.h"
#include "TestConfigurationUnitResult.h"
#include "ConfigurationUnitResultInformation.h"
#include "GetConfigurationUnitSettingsResult.h"
#include "GetAllConfigurationUnitSettingsResult.h"
#include "ExceptionResultHelpers.h"
#include "ConfigurationSetChangeData.h"
#include "GetConfigurationUnitDetailsResult.h"
#include "GetConfigurationSetDetailsResult.h"
#include "DefaultSetGroupProcessor.h"
#include "ConfigurationSequencer.h"
#include "ConfigurationStatus.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include <winget/GroupPolicy.h>

using namespace std::chrono_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        AppInstaller::Logging::Level ConvertLevel(DiagnosticLevel level)
        {
            switch (level)
            {
            case DiagnosticLevel::Verbose: return AppInstaller::Logging::Level::Verbose;
            case DiagnosticLevel::Informational: return AppInstaller::Logging::Level::Info;
            case DiagnosticLevel::Warning: return AppInstaller::Logging::Level::Warning;
            case DiagnosticLevel::Error: return AppInstaller::Logging::Level::Error;
            case DiagnosticLevel::Critical: return AppInstaller::Logging::Level::Crit;
            default: return AppInstaller::Logging::Level::Warning;
            }
        }

        DiagnosticLevel ConvertLevel(AppInstaller::Logging::Level level)
        {
            switch (level)
            {
            case AppInstaller::Logging::Level::Verbose: return DiagnosticLevel::Verbose;
            case AppInstaller::Logging::Level::Info: return DiagnosticLevel::Informational;
            case AppInstaller::Logging::Level::Warning: return DiagnosticLevel::Warning;
            case AppInstaller::Logging::Level::Error: return DiagnosticLevel::Error;
            case AppInstaller::Logging::Level::Crit: return DiagnosticLevel::Critical;
            default: return DiagnosticLevel::Warning;
            }
        }

        // ILogger that sends data back to the Diagnostics event of the ConfigurationProcessor.
        struct ConfigurationProcessorDiagnosticsLogger : public AppInstaller::Logging::ILogger
        {
            ConfigurationProcessorDiagnosticsLogger(ConfigurationProcessor& processor) : m_processor(processor) {}

            std::string GetName() const override
            {
                return "ConfigurationProcessorDiagnosticsLogger";
            }

            void Write(AppInstaller::Logging::Channel channel, AppInstaller::Logging::Level level, std::string_view message) noexcept override try
            {
                std::ostringstream strstr;
                strstr << '[' << AppInstaller::Logging::GetChannelName(channel) << "] " << message;
                m_processor.SendDiagnostics(ConvertLevel(level), strstr.str());
            }
            catch (...) {}

            void WriteDirect(AppInstaller::Logging::Channel, AppInstaller::Logging::Level level, std::string_view message) noexcept override try
            {
                m_processor.SendDiagnostics(ConvertLevel(level), message);
            }
            catch (...) {}

        private:
            ConfigurationProcessor& m_processor;
        };

        // Helper to ensure a one-time callback attach
        struct AttachWilFailureCallback
        {
            AttachWilFailureCallback()
            {
                wil::SetResultLoggingCallback(wilResultLoggingCallback);
            }

            ~AttachWilFailureCallback() = default;

            static void __stdcall wilResultLoggingCallback(const wil::FailureInfo& info) noexcept
            {
                AICLI_LOG(Fail, Error, << [&]() {
                    wchar_t message[2048];
                    GetFailureLogString(message, ARRAYSIZE(message), info);
                    return AppInstaller::Utility::ConvertToUTF8(message);
                    }());
            }

            static void Ensure()
            {
                static AttachWilFailureCallback s_callbackAttach;
            }
        };
    }

    ConfigurationProcessor::ConfigurationProcessor()
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet));
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::Configuration));

        AppInstaller::Logging::DiagnosticLogger& logger = m_threadGlobals.GetDiagnosticLogger();
        logger.EnableChannel(AppInstaller::Logging::Channel::All);
        logger.SetLevel(AppInstaller::Logging::Level::Verbose);
        logger.AddLogger(std::make_unique<ConfigurationProcessorDiagnosticsLogger>(*this));
    }

    ConfigurationProcessor::ConfigurationProcessor(const IConfigurationSetProcessorFactory& factory) : ConfigurationProcessor()
    {
        ConfigurationSetProcessorFactory(factory);
    }

    event_token ConfigurationProcessor::Diagnostics(const Windows::Foundation::EventHandler<IDiagnosticInformation>& handler)
    {
        AttachWilFailureCallback::Ensure();
        return m_diagnostics.add(handler);
    }

    void ConfigurationProcessor::Diagnostics(const event_token& token) noexcept
    {
        m_diagnostics.remove(token);
    }

    DiagnosticLevel ConfigurationProcessor::MinimumLevel()
    {
        return m_minimumLevel;
    }

    void ConfigurationProcessor::MinimumLevel(DiagnosticLevel value)
    {
        m_minimumLevel = value;
        m_threadGlobals.GetDiagnosticLogger().SetLevel(ConvertLevel(value));
        if (m_factory)
        {
            m_factory.MinimumLevel(value);
        }
    }

    hstring ConfigurationProcessor::Caller() const
    {
        return hstring{ AppInstaller::Utility::ConvertToUTF16(m_threadGlobals.GetTelemetryLogger().GetCaller()) };
    }

    void ConfigurationProcessor::Caller(hstring value)
    {
        m_threadGlobals.GetTelemetryLogger().SetCaller(AppInstaller::Utility::ConvertToUTF8(value));
    }

    guid ConfigurationProcessor::ActivityIdentifier()
    {
        return *m_threadGlobals.GetTelemetryLogger().GetActivityId();
    }

    void ConfigurationProcessor::ActivityIdentifier(const guid& value)
    {
        m_threadGlobals.GetTelemetryLogger().SetActivityId(value);
    }

    bool ConfigurationProcessor::GenerateTelemetryEvents()
    {
        return m_threadGlobals.GetTelemetryLogger().IsEnabled();
    }

    void ConfigurationProcessor::GenerateTelemetryEvents(bool value)
    {
        std::ignore = m_threadGlobals.GetTelemetryLogger().EnableRuntime(value);
    }

    event_token ConfigurationProcessor::ConfigurationChange(const Windows::Foundation::TypedEventHandler<Configuration::ConfigurationSet, Configuration::ConfigurationChangeData>& handler)
    {
        if (!m_configurationChange)
        {
            auto status = ConfigurationStatus::Instance();
            std::atomic_store(&m_changeRegistration, status->RegisterForChange(*this));
        }

        return m_configurationChange.add(handler);
    }

    void ConfigurationProcessor::ConfigurationChange(const event_token& token) noexcept
    {
        m_configurationChange.remove(token);

        if (!m_configurationChange)
        {
            std::atomic_store(&m_changeRegistration, {});
        }
    }

    void ConfigurationProcessor::ConfigurationChange(const Configuration::ConfigurationSet& set, const Configuration::ConfigurationChangeData& data) try
    {
        m_configurationChange(set, data);
    }
    CATCH_LOG();

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet> ConfigurationProcessor::GetConfigurationHistory()
    {
        return GetConfigurationHistoryImpl();
    }

    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet>> ConfigurationProcessor::GetConfigurationHistoryAsync()
    {
        auto strong_this{ get_strong() };
        co_await winrt::resume_background();
        co_return GetConfigurationHistoryImpl({ co_await winrt::get_cancellation_token() });
    }

    Configuration::OpenConfigurationSetResult ConfigurationProcessor::OpenConfigurationSet(const Windows::Storage::Streams::IInputStream& stream)
    {
        return OpenConfigurationSetAsync(stream).get();
    }

    Windows::Foundation::IAsyncOperation<Configuration::OpenConfigurationSetResult> ConfigurationProcessor::OpenConfigurationSetAsync(const Windows::Storage::Streams::IInputStream& stream)
    {
        auto strong_this{ get_strong() };
        Windows::Storage::Streams::IInputStream localStream = stream;

        co_await winrt::resume_background();
        auto cancellation = co_await winrt::get_cancellation_token();

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();
        auto result = make_self<wil::details::module_count_wrapper<OpenConfigurationSetResult>>();

        if (!localStream)
        {
            result->Initialize(E_POINTER, {});
            co_return *result;
        }

        try
        {
            // Read the entire file into memory as we expect them to be small and
            // our YAML parser doesn't support streaming at this time.
            // This is done here to enable easy cancellation propagation to the stream reads.
            uint32_t bufferSize = 1 << 20;
            Windows::Storage::Streams::Buffer buffer(bufferSize);

            // Memory stream in mixed elevation does not support InputStreamOptions as flags.
            Windows::Storage::Streams::InputStreamOptions readOptions = Windows::Storage::Streams::InputStreamOptions::Partial;
            std::string inputString;

            for (;;)
            {
                auto asyncOperation = localStream.ReadAsync(buffer, bufferSize, readOptions);

                // Manually poll status and propagate cancellation to stay on this thread for thread globals
                while (asyncOperation.Status() == Windows::Foundation::AsyncStatus::Started)
                {
                    if (cancellation())
                    {
                        asyncOperation.Cancel();
                    }

                    std::this_thread::sleep_for(100ms);
                }

                Windows::Storage::Streams::IBuffer readBuffer = asyncOperation.GetResults();

                size_t readSize = static_cast<size_t>(readBuffer.Length());
                if (readSize)
                {
                    static_assert(sizeof(char) == sizeof(*readBuffer.data()));
                    inputString.append(reinterpret_cast<char*>(readBuffer.data()), readSize);
                }
                else
                {
                    break;
                }
            }

            std::unique_ptr<ConfigurationSetParser> parser = ConfigurationSetParser::Create(inputString);

            // Temporary block on parsing 0.3 schema while it is experimental.
            if (parser->GetSchemaVersion() == L"0.3" && !m_supportSchema03)
            {
                result->Initialize(APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED);
                co_return *result;
            }

            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field(), parser->Value(), parser->Line(), parser->Column());
                co_return *result;
            }

            parser->Parse();
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field(), parser->Value(), parser->Line(), parser->Column());
                co_return *result;
            }

            auto configurationSet = parser->GetConfigurationSet();
            PropagateLifetimeWatcher(configurationSet.as<Windows::Foundation::IUnknown>());
            configurationSet->SetInputHash(AppInstaller::Utility::SHA256::ConvertToString(AppInstaller::Utility::SHA256::ComputeHash(inputString)));

            result->Initialize(*configurationSet);
        }
        catch (const wil::ResultException& resultException)
        {
            result->Initialize(resultException.GetErrorCode());
        }
        catch (...)
        {
            result->Initialize(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE);
            LOG_CAUGHT_EXCEPTION();
        }

        co_return *result;
    }

    Windows::Foundation::Collections::IVector<ConfigurationConflict> ConfigurationProcessor::CheckForConflicts(
        const Windows::Foundation::Collections::IVectorView<Configuration::ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        UNREFERENCED_PARAMETER(configurationSets);
        UNREFERENCED_PARAMETER(includeConfigurationHistory);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<ConfigurationConflict>> ConfigurationProcessor::CheckForConflictsAsync(
        const Windows::Foundation::Collections::IVectorView<Configuration::ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        co_return CheckForConflicts(configurationSets, includeConfigurationHistory);
    }

    Configuration::GetConfigurationSetDetailsResult ConfigurationProcessor::GetSetDetails(const Configuration::ConfigurationSet& configurationSet, ConfigurationUnitDetailFlags detailFlags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return GetSetDetailsImpl(configurationSet, detailFlags);
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::GetConfigurationSetDetailsResult, Configuration::GetConfigurationUnitDetailsResult> ConfigurationProcessor::GetSetDetailsAsync(
        const Configuration::ConfigurationSet& configurationSet,
        ConfigurationUnitDetailFlags detailFlags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        Configuration::ConfigurationSet localSet = configurationSet;

        co_await winrt::resume_background();

        co_return GetSetDetailsImpl(localSet, detailFlags, { co_await winrt::get_progress_token(), co_await winrt::get_cancellation_token()});
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet> ConfigurationProcessor::GetConfigurationHistoryImpl(AppInstaller::WinRT::AsyncCancellation cancellation)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        m_database.EnsureOpened(false);
        cancellation.ThrowIfCancelled();

        std::vector<Configuration::ConfigurationSet> result;
        for (const auto& set : m_database.GetSetHistory())
        {
            PropagateLifetimeWatcher(*set);
            result.emplace_back(*set);
        }

        return multi_threaded_vector(std::move(result));
    }

    Configuration::GetConfigurationSetDetailsResult ConfigurationProcessor::GetSetDetailsImpl(
        const Configuration::ConfigurationSet& configurationSet,
        ConfigurationUnitDetailFlags detailFlags,
        AppInstaller::WinRT::AsyncProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> progress)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(configurationSet);

        auto result = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationSetDetailsResult>>();
        progress.Result(*result);

        for (const auto& unit : configurationSet.Units())
        {
            progress.ThrowIfCancelled();

            auto unitResult = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationUnitDetailsResult>>();
            auto unitResultInformation = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
            unitResult->Unit(unit);
            unitResult->ResultInformation(*unitResultInformation);

            try
            {
                IConfigurationUnitProcessorDetails details = setProcessor.GetUnitProcessorDetails(unit, detailFlags);
                unitResult->Details(details);
                get_self<implementation::ConfigurationUnit>(unit)->Details(std::move(details));
            }
            catch (...)
            {
                ExtractUnitResultInformation(std::current_exception(), unitResultInformation);
            }

            result->UnitResultsVector().Append(*unitResult);
            progress.Progress(*unitResult);
        }

        return *result;
    }

    Configuration::GetConfigurationUnitDetailsResult ConfigurationProcessor::GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return GetUnitDetailsImpl(unit, detailFlags);
    }

    Windows::Foundation::IAsyncOperation<Configuration::GetConfigurationUnitDetailsResult> ConfigurationProcessor::GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        ConfigurationUnit localUnit = unit;

        co_await winrt::resume_background();

        co_return GetUnitDetailsImpl(localUnit, detailFlags);
    }

    Configuration::GetConfigurationUnitDetailsResult ConfigurationProcessor::GetUnitDetailsImpl(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);

        auto unitResult = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationUnitDetailsResult>>();
        auto unitResultInformation = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
        unitResult->Unit(unit);
        unitResult->ResultInformation(*unitResultInformation);

        try
        {
            IConfigurationUnitProcessorDetails details = setProcessor.GetUnitProcessorDetails(unit, detailFlags);
            unitResult->Details(details);
            get_self<implementation::ConfigurationUnit>(unit)->Details(std::move(details));
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitResultInformation);
        }

        return *unitResult;
    }

    Configuration::ApplyConfigurationSetResult ConfigurationProcessor::ApplySet(const Configuration::ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return ApplySetImpl(configurationSet, flags);
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::ApplyConfigurationSetResult, Configuration::ConfigurationSetChangeData> ConfigurationProcessor::ApplySetAsync(
        const Configuration::ConfigurationSet& configurationSet,
        ApplyConfigurationSetFlags flags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        Configuration::ConfigurationSet localSet = configurationSet;

        co_await winrt::resume_background();

        co_return ApplySetImpl(localSet, flags, { co_await winrt::get_progress_token(), co_await winrt::get_cancellation_token() });
    }

    Configuration::ApplyConfigurationSetResult ConfigurationProcessor::ApplySetImpl(
        const Configuration::ConfigurationSet& configurationSet,
        ApplyConfigurationSetFlags flags,
        AppInstaller::WinRT::AsyncProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> progress)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationGroupProcessor groupProcessor;
        bool recordHistoryAndStatus = false;

        if (WI_IsFlagSet(flags, ApplyConfigurationSetFlags::PerformConsistencyCheckOnly))
        {
            // If performing a consistency check, always use the default processor and let it know as well
            auto defaultGroupProcessor = make_self<wil::details::module_count_wrapper<implementation::DefaultSetGroupProcessor>>();
            defaultGroupProcessor->Initialize(configurationSet, nullptr, m_threadGlobals, true);
            groupProcessor = *defaultGroupProcessor;
        }
        else
        {
            groupProcessor = GetSetGroupProcessor(configurationSet);

            // Write this set to the database history
            // This is a somewhat arbitrary time to write it, but it should not be done if PerformConsistencyCheckOnly is passed, so this is convenient.
            recordHistoryAndStatus = true;
            m_database.EnsureOpened();
            progress.ThrowIfCancelled();
            m_database.WriteSetHistory(configurationSet, WI_IsFlagSet(flags, ApplyConfigurationSetFlags::DoNotOverwriteMatchingOriginSet));
        }

        auto result = make_self<wil::details::module_count_wrapper<implementation::ApplyConfigurationSetResult>>();

        // Build out the unit results and a map to find them quickly
        using UnitResultType = decltype(make_self<wil::details::module_count_wrapper<implementation::ApplyConfigurationUnitResult>>());
        std::map<guid, UnitResultType> unitResultMap;

        std::function<void(const winrt::Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>&)> createUnitResults =
            [&](const winrt::Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& units)
            {
                for (const Configuration::ConfigurationUnit& unit : units)
                {
                    // Add to result
                    UnitResultType applyUnitResult = make_self<wil::details::module_count_wrapper<implementation::ApplyConfigurationUnitResult>>();
                    applyUnitResult->Unit(unit);
                    result->UnitResultsVector().Append(*applyUnitResult);

                    // Add to map
                    unitResultMap.emplace(unit.InstanceIdentifier(), applyUnitResult);

                    // Handle members if present
                    if (unit.IsGroup())
                    {
                        createUnitResults(unit.Units());
                    }
                }
            };

        createUnitResults(configurationSet.Units());

        progress.Result(*result);

        try
        {
            ConfigurationSequencer sequencer{ m_database };
            auto status = ConfigurationStatus::Instance();
            guid setInstanceIdentifier = configurationSet.InstanceIdentifier();
            auto updateState = [&](ConfigurationSetState state)
                {
                    try
                    {
                        progress.Progress(implementation::ConfigurationSetChangeData::Create(state));
                    }
                    CATCH_LOG();

                    if (recordHistoryAndStatus)
                    {
                        status->UpdateSetState(setInstanceIdentifier, state);
                    }
                };

            if (!WI_IsFlagSet(flags, ApplyConfigurationSetFlags::PerformConsistencyCheckOnly))
            {
                if (sequencer.Enqueue(configurationSet))
                {
                    updateState(ConfigurationSetState::Pending);
                    sequencer.Wait(progress);
                }
            }

            progress.ThrowIfCancelled();

            updateState(ConfigurationSetState::InProgress);

            // Forward unit result progress to caller
            auto applyOperation = groupProcessor.ApplyGroupSettingsAsync([&](const auto&, const IApplyGroupMemberSettingsResult& unitResult)
                {
                    auto itr = unitResultMap.find(unitResult.Unit().InstanceIdentifier());
                    if (itr != unitResultMap.end())
                    {
                        itr->second->Initialize(unitResult);
                    }

                    // Create progress object
                    auto applyResult = make_self<implementation::ConfigurationSetChangeData>();
                    applyResult->Initialize(unitResult);
                    progress.Progress(*applyResult);

                    if (recordHistoryAndStatus)
                    {
                        status->UpdateUnitState(setInstanceIdentifier, applyResult);
                    }
                });

            // Cancel the inner operation if we are cancelled
            progress.Callback([applyOperation]() { applyOperation.Cancel(); });

            IApplyGroupSettingsResult applyResult = applyOperation.get();

            // Place all results from the processor into our result
            if (applyResult.ResultInformation())
            {
                result->ResultCode(applyResult.ResultInformation().ResultCode());
            }

            for (const IApplyGroupMemberSettingsResult& unitResult : applyResult.UnitResults())
            {
                // Update overall result
                auto itr = unitResultMap.find(unitResult.Unit().InstanceIdentifier());
                if (itr == unitResultMap.end())
                {
                    continue;
                }

                itr->second->Initialize(unitResult);

                m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(
                    configurationSet.InstanceIdentifier(),
                    itr->second->Unit(),
                    ConfigurationUnitIntent::Apply,
                    TelemetryTraceLogger::ApplyAction,
                    itr->second->ResultInformation());
            }

            updateState(ConfigurationSetState::Completed);

            m_threadGlobals.GetTelemetryLogger().LogConfigProcessingSummaryForApply(*winrt::get_self<implementation::ConfigurationSet>(configurationSet), *result);
            return *result;
        }
        catch (...)
        {
            m_threadGlobals.GetTelemetryLogger().LogConfigProcessingSummaryForApplyException(
                *winrt::get_self<implementation::ConfigurationSet>(configurationSet),
                LOG_CAUGHT_EXCEPTION(),
                *result);
            throw;
        }
    }

    Configuration::TestConfigurationSetResult ConfigurationProcessor::TestSet(const Configuration::ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return TestSetImpl(configurationSet);
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::TestConfigurationSetResult, Configuration::TestConfigurationUnitResult> ConfigurationProcessor::TestSetAsync(const Configuration::ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        Configuration::ConfigurationSet localSet = configurationSet;

        co_await winrt::resume_background();

        co_return TestSetImpl(localSet, { co_await winrt::get_progress_token(), co_await winrt::get_cancellation_token() });
    }

    Configuration::TestConfigurationSetResult ConfigurationProcessor::TestSetImpl(
        const Configuration::ConfigurationSet& configurationSet,
        AppInstaller::WinRT::AsyncProgress<TestConfigurationSetResult, TestConfigurationUnitResult> progress)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationGroupProcessor groupProcessor = GetSetGroupProcessor(configurationSet);
        auto result = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationSetResult>>();
        result->TestResult(ConfigurationTestResult::NotRun);
        progress.Result(*result);

        try
        {
            // Forward unit result progress to caller
            auto testOperation = groupProcessor.TestGroupSettingsAsync([&](const auto&, const ITestSettingsResult& unitResult)
                {
                    auto testResult = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationUnitResult>>();
                    testResult->Initialize(unitResult);

                    result->AppendUnitResult(*testResult);
                    progress.Progress(*testResult);
                });

            // Cancel the inner operation if we are cancelled
            progress.Callback([testOperation]() { testOperation.Cancel(); });

            ITestGroupSettingsResult testResult = testOperation.get();

            // Send telemetry for all results
            for (const ITestSettingsResult& unitResult : testResult.UnitResults())
            {
                auto testUnitResult = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationUnitResult>>();
                testUnitResult->Initialize(unitResult);

                m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(
                    configurationSet.InstanceIdentifier(),
                    testUnitResult->Unit(),
                    ConfigurationUnitIntent::Assert,
                    TelemetryTraceLogger::TestAction,
                    testUnitResult->ResultInformation());
            }

            m_threadGlobals.GetTelemetryLogger().LogConfigProcessingSummaryForTest(*winrt::get_self<implementation::ConfigurationSet>(configurationSet), *result);
            return *result;
        }
        catch (...)
        {
            m_threadGlobals.GetTelemetryLogger().LogConfigProcessingSummaryForTestException(
                *winrt::get_self<implementation::ConfigurationSet>(configurationSet),
                LOG_CAUGHT_EXCEPTION(),
                *result);
            throw;
        }
    }

    Configuration::GetConfigurationUnitSettingsResult ConfigurationProcessor::GetUnitSettings(const ConfigurationUnit& unit)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return GetUnitSettingsImpl(unit);
    }

    Windows::Foundation::IAsyncOperation<Configuration::GetConfigurationUnitSettingsResult> ConfigurationProcessor::GetUnitSettingsAsync(const ConfigurationUnit& unit)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        ConfigurationUnit localUnit = unit;

        co_await winrt::resume_background();

        co_return GetUnitSettingsImpl(localUnit, { co_await winrt::get_cancellation_token() });
    }

    Configuration::GetConfigurationUnitSettingsResult ConfigurationProcessor::GetUnitSettingsImpl(
        const ConfigurationUnit& unit,
        AppInstaller::WinRT::AsyncCancellation cancellation)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);
        auto result = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationUnitSettingsResult>>();
        auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
        result->ResultInformation(*unitResult);

        cancellation.ThrowIfCancelled();

        IConfigurationUnitProcessor unitProcessor;

        try
        {
            unitProcessor = setProcessor.CreateUnitProcessor(unit);
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitResult);
        }

        cancellation.ThrowIfCancelled();

        if (unitProcessor)
        {
            try
            {
                IGetSettingsResult settingsResult = unitProcessor.GetSettings();
                result->Settings(settingsResult.Settings());
                result->ResultInformation(settingsResult.ResultInformation());
            }
            catch (...)
            {
                ExtractUnitResultInformation(std::current_exception(), unitResult);
            }

            m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(GUID_NULL, unit, ConfigurationUnitIntent::Inform, TelemetryTraceLogger::GetAction, result->ResultInformation());
        }

        return *result;
    }
    
    Configuration::GetAllConfigurationUnitSettingsResult ConfigurationProcessor::GetAllUnitSettings(const ConfigurationUnit& unit)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return GetAllUnitSettingsImpl(unit);
    }

    Windows::Foundation::IAsyncOperation<Configuration::GetAllConfigurationUnitSettingsResult> ConfigurationProcessor::GetAllUnitSettingsAsync(const ConfigurationUnit& unit)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        ConfigurationUnit localUnit = unit;

        co_await winrt::resume_background();

        co_return GetAllUnitSettingsImpl(localUnit, { co_await winrt::get_cancellation_token() });
    }

    Configuration::GetAllConfigurationUnitSettingsResult ConfigurationProcessor::GetAllUnitSettingsImpl(
        const ConfigurationUnit& unit,
        AppInstaller::WinRT::AsyncCancellation cancellation)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);
        auto result = make_self<wil::details::module_count_wrapper<implementation::GetAllConfigurationUnitSettingsResult>>();
        auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
        result->ResultInformation(*unitResult);

        cancellation.ThrowIfCancelled();

        IConfigurationUnitProcessor unitProcessor;

        try
        {
            // TODO: Directives overlay to prevent running elevated for test
            unitProcessor = setProcessor.CreateUnitProcessor(unit);
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitResult);
        }

        cancellation.ThrowIfCancelled();

        IGetAllSettingsConfigurationUnitProcessor getAllSettingsUnitProcessor;
        if (unitProcessor.try_as<IGetAllSettingsConfigurationUnitProcessor>(getAllSettingsUnitProcessor))
        {
            cancellation.ThrowIfCancelled();

            try
            {
                IGetAllSettingsResult allSettingsResult = getAllSettingsUnitProcessor.GetAllSettings();
                result->Settings(allSettingsResult.Settings());
                result->ResultInformation(allSettingsResult.ResultInformation());
            }
            catch (...)
            {
                ExtractUnitResultInformation(std::current_exception(), unitResult);
            }

            m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(GUID_NULL, unit, ConfigurationUnitIntent::Inform, TelemetryTraceLogger::ExportAction, result->ResultInformation());
        }
        else
        {
            AICLI_LOG(Config, Error, << "Unit Processor does not support GetAllSettings operation");
            unitResult->Initialize(WINGET_CONFIG_ERROR_NOT_SUPPORTED_BY_PROCESSOR, hstring{});
        }

        return *result;
    }

    IConfigurationGroupProcessor ConfigurationProcessor::GetSetGroupProcessor(const Configuration::ConfigurationSet& configurationSet)
    {
        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(configurationSet);

        IConfigurationGroupProcessor result = setProcessor.try_as<IConfigurationGroupProcessor>();
        if (!result)
        {
            auto groupProcessor = make_self<wil::details::module_count_wrapper<implementation::DefaultSetGroupProcessor>>();
            groupProcessor->Initialize(configurationSet, setProcessor, m_threadGlobals);
            result = *groupProcessor;
        }

        return result;
    }

    HRESULT STDMETHODCALLTYPE ConfigurationProcessor::SetLifetimeWatcher(IUnknown* watcher)
    {
        return AppInstaller::WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
    }

    void ConfigurationProcessor::ConfigurationSetProcessorFactory(const IConfigurationSetProcessorFactory& value)
    {
        m_factory = value;

        if (m_factory)
        {
            m_factoryDiagnosticsEventRevoker = m_factory.Diagnostics(winrt::auto_revoke,
                [weak_this{ get_weak() }](const IInspectable&, const IDiagnosticInformation& information)
                {
                    if (auto strong_this{ weak_this.get() })
                    {
                        strong_this->SendDiagnostics(information);
                    }
                });
        }
    }

    void ConfigurationProcessor::SendDiagnostics(DiagnosticLevel level, std::string_view message) try
    {
        if (level >= m_minimumLevel)
        {
            auto diagnostics = make_self<wil::details::module_count_wrapper<implementation::DiagnosticInformationInstance>>();
            diagnostics->Initialize(level, AppInstaller::Utility::ConvertToUTF16(message));
            SendDiagnosticsImpl(*diagnostics);
        }
    }
    // While diagnostics can be important, a failure to send them should not cause additional issues.
    catch (...) {}

    void ConfigurationProcessor::SendDiagnostics(const IDiagnosticInformation& information) try
    {
        if (information.Level() >= m_minimumLevel)
        {
            SendDiagnosticsImpl(information);
        }
    }
    // While diagnostics can be important, a failure to send them should not cause additional issues.
    catch (...) {}

    void ConfigurationProcessor::SetSupportsSchema03(bool value)
    {
        m_supportSchema03 = value;
    }

    void ConfigurationProcessor::SendDiagnosticsImpl(const IDiagnosticInformation& information)
    {
        std::lock_guard<std::recursive_mutex> lock{ m_diagnosticsMutex };

        // Prevent a winrt/wil error recursion here by detecting that this thread failed to send a previous message.
        if (m_isHandlingDiagnostics)
        {
            std::wstring debugMessage = L"An error occurred while trying to send a previous diagnostics message:\n";
            debugMessage.append(information.Message());
            OutputDebugStringW(debugMessage.c_str());
            return;
        }

        m_isHandlingDiagnostics = true;
        auto notHandling = wil::scope_exit([&] { m_isHandlingDiagnostics = false; });

        m_diagnostics(*this, information);
    }
}
