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
#include "ConfigurationSetApplyProcessor.h"
#include "TestConfigurationSetResult.h"
#include "TestConfigurationUnitResult.h"
#include "ConfigurationUnitResultInformation.h"
#include "GetConfigurationUnitSettingsResult.h"
#include "ExceptionResultHelpers.h"
#include "ConfigurationSetChangeData.h"
#include "GetConfigurationUnitDetailsResult.h"
#include "GetConfigurationSetDetailsResult.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
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

        // Specifies the set of intents that should execute during a Test request
        bool ShouldTestDuringTest(ConfigurationUnitIntent intent)
        {
            return (intent == ConfigurationUnitIntent::Assert || intent == ConfigurationUnitIntent::Apply);
        }
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

    event_token ConfigurationProcessor::ConfigurationChange(const Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>& handler)
    {
        return m_configurationChange.add(handler);
    }

    void ConfigurationProcessor::ConfigurationChange(const event_token& token) noexcept
    {
        m_configurationChange.remove(token);
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet> ConfigurationProcessor::GetConfigurationHistory()
    {
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet>> ConfigurationProcessor::GetConfigurationHistoryAsync()
    {
        co_return GetConfigurationHistory();
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
            Windows::Storage::Streams::InputStreamOptions readOptions = 
                Windows::Storage::Streams::InputStreamOptions::Partial | Windows::Storage::Streams::InputStreamOptions::ReadAhead;
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
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field(), parser->Value(), parser->Line(), parser->Column());
                co_return *result;
            }

            auto configurationSet = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
            configurationSet->Initialize(parser->GetConfigurationUnits());
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field(), parser->Value(), parser->Line(), parser->Column());
                co_return *result;
            }
            configurationSet->SchemaVersion(parser->GetSchemaVersion());

            PropagateLifetimeWatcher(configurationSet.as<Windows::Foundation::IUnknown>());

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
        const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        UNREFERENCED_PARAMETER(configurationSets);
        UNREFERENCED_PARAMETER(includeConfigurationHistory);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVector<ConfigurationConflict>> ConfigurationProcessor::CheckForConflictsAsync(
        const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        co_return CheckForConflicts(configurationSets, includeConfigurationHistory);
    }

    Configuration::GetConfigurationSetDetailsResult ConfigurationProcessor::GetSetDetails(const ConfigurationSet& configurationSet, ConfigurationUnitDetailFlags detailFlags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return GetSetDetailsImpl(configurationSet, detailFlags);
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::GetConfigurationSetDetailsResult, Configuration::GetConfigurationUnitDetailsResult> ConfigurationProcessor::GetSetDetailsAsync(const ConfigurationSet& configurationSet, ConfigurationUnitDetailFlags detailFlags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        ConfigurationSet localSet = configurationSet;

        co_await winrt::resume_background();

        co_return GetSetDetailsImpl(localSet, detailFlags, { co_await winrt::get_progress_token(), co_await winrt::get_cancellation_token()});
    }

    Configuration::GetConfigurationSetDetailsResult ConfigurationProcessor::GetSetDetailsImpl(
        const ConfigurationSet& configurationSet,
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

    Configuration::ApplyConfigurationSetResult ConfigurationProcessor::ApplySet(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return ApplySetImpl(configurationSet, flags);
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::ApplyConfigurationSetResult, Configuration::ConfigurationSetChangeData> ConfigurationProcessor::ApplySetAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        ConfigurationSet localSet = configurationSet;

        co_await winrt::resume_background();

        co_return ApplySetImpl(localSet, flags, { co_await winrt::get_progress_token(), co_await winrt::get_cancellation_token() });
    }

    Configuration::ApplyConfigurationSetResult ConfigurationProcessor::ApplySetImpl(
        const ConfigurationSet& configurationSet,
        ApplyConfigurationSetFlags flags,
        AppInstaller::WinRT::AsyncProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> progress)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        bool consistencyCheckOnly = WI_IsFlagSet(flags, ApplyConfigurationSetFlags::PerformConsistencyCheckOnly);
        IConfigurationSetProcessor setProcessor;

        if (!consistencyCheckOnly)
        {
            setProcessor = m_factory.CreateSetProcessor(configurationSet);
        }

        ConfigurationSetApplyProcessor applyProcessor{ configurationSet, m_threadGlobals.GetTelemetryLogger(), std::move(setProcessor), std::move(progress) };
        applyProcessor.Process(consistencyCheckOnly);

        return applyProcessor.Result();
    }

    Configuration::TestConfigurationSetResult ConfigurationProcessor::TestSet(const ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);
        return TestSetImpl(configurationSet);
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::TestConfigurationSetResult, Configuration::TestConfigurationUnitResult> ConfigurationProcessor::TestSetAsync(const ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        auto strong_this{ get_strong() };
        ConfigurationSet localSet = configurationSet;

        co_await winrt::resume_background();

        co_return TestSetImpl(localSet, { co_await winrt::get_progress_token(), co_await winrt::get_cancellation_token() });
    }

    Configuration::TestConfigurationSetResult ConfigurationProcessor::TestSetImpl(
        const ConfigurationSet& configurationSet,
        AppInstaller::WinRT::AsyncProgress<TestConfigurationSetResult, TestConfigurationUnitResult> progress)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(configurationSet);
        auto result = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationSetResult>>();
        result->TestResult(ConfigurationTestResult::NotRun);
        progress.Result(*result);

        try
        {
            for (const auto& unit : configurationSet.Units())
            {
                AICLI_LOG(Config, Info, << "Testing configuration unit: " << AppInstaller::Utility::ConvertToUTF8(unit.Type()));

                auto testResult = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationUnitResult>>();
                auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
                testResult->Initialize(unit, *unitResult);

                if (ShouldTestDuringTest(unit.Intent()))
                {
                    progress.ThrowIfCancelled();

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

                    progress.ThrowIfCancelled();

                    if (unitProcessor)
                    {
                        try
                        {
                            ITestSettingsResult settingsResult = unitProcessor.TestSettings();
                            testResult->TestResult(settingsResult.TestResult());
                            testResult->ResultInformation(settingsResult.ResultInformation());
                        }
                        catch (...)
                        {
                            ExtractUnitResultInformation(std::current_exception(), unitResult);
                        }

                        m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(
                            configurationSet.InstanceIdentifier(),
                            unit,
                            ConfigurationUnitIntent::Assert,
                            TelemetryTraceLogger::TestAction,
                            testResult->ResultInformation());
                    }
                }
                else
                {
                    testResult->TestResult(ConfigurationTestResult::NotRun);
                }

                if (FAILED(unitResult->ResultCode()))
                {
                    testResult->TestResult(ConfigurationTestResult::Failed);
                }

                result->AppendUnitResult(*testResult);

                progress.Progress(*testResult);
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
            // TODO: Directives overlay to prevent running elevated for get
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
