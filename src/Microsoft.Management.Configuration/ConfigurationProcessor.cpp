// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationProcessor.h"
#include "ConfigurationProcessor.g.cpp"
#include "ConfigurationSet.h"
#include "OpenConfigurationSetResult.h"
#include "ConfigurationSetParser.h"
#include "DiagnosticInformation.h"
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
                m_processor.Diagnostics(ConvertLevel(level), strstr.str());
            }
            catch (...) {}

            void WriteDirect(AppInstaller::Logging::Channel, AppInstaller::Logging::Level level, std::string_view message) noexcept override try
            {
                m_processor.Diagnostics(ConvertLevel(level), message);
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
        };

        // Specifies the set of intents that should execute during a Test request
        bool ShouldTestDuringTest(ConfigurationUnitIntent intent)
        {
            return (intent == ConfigurationUnitIntent::Assert || intent == ConfigurationUnitIntent::Apply);
        }
    }

    ConfigurationProcessor::ConfigurationProcessor(const IConfigurationSetProcessorFactory& factory) : m_factory(factory)
    {
        AppInstaller::Logging::DiagnosticLogger& logger = m_threadGlobals.GetDiagnosticLogger();
        logger.EnableChannel(AppInstaller::Logging::Channel::All);
        logger.SetLevel(AppInstaller::Logging::Level::Verbose);
        logger.AddLogger(std::make_unique<ConfigurationProcessorDiagnosticsLogger>(*this));

        if (m_factory)
        {
            m_factoryDiagnosticsEventRevoker = m_factory.Diagnostics(winrt::auto_revoke,
                [this](const IInspectable&, const DiagnosticInformation& information)
                {
                    m_diagnostics(*this, information);
                });
        }
    }

    event_token ConfigurationProcessor::Diagnostics(const Windows::Foundation::EventHandler<DiagnosticInformation>& handler)
    {
        static AttachWilFailureCallback s_callbackAttach;
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
        Windows::Storage::Streams::IInputStream localStream = stream;
        co_await winrt::resume_background();
        auto cancellation = co_await get_cancellation_token();
        cancellation.enable_propagation();

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();
        auto result = make_self<wil::details::module_count_wrapper<OpenConfigurationSetResult>>();

        if (!stream)
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
                Windows::Storage::Streams::IBuffer readBuffer = co_await localStream.ReadAsync(buffer, bufferSize, readOptions);

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
                result->Initialize(parser->Result(), parser->Field());
                co_return *result;
            }

            auto configurationSet = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
            configurationSet->Initialize(parser->GetConfigurationUnits());
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field());
                co_return *result;
            }

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

    Configuration::GetConfigurationSetDetailsResult ConfigurationProcessor::GetSetDetails(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel)
    {
        return GetSetDetailsAsync(configurationSet, detailLevel).get();
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::GetConfigurationSetDetailsResult, Configuration::GetConfigurationUnitDetailsResult> ConfigurationProcessor::GetSetDetailsAsync(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        ConfigurationSet localSet = configurationSet;
        co_await winrt::resume_background();

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(localSet);

        auto progress = co_await winrt::get_progress_token();
        auto result = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationSetDetailsResult>>();
        progress.set_result(*result);

        for (const auto& unit : localSet.ConfigurationUnits())
        {
            auto unitResult = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationUnitDetailsResult>>();
            auto unitResultInformation = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
            unitResult->Unit(unit);
            unitResult->ResultInformation(*unitResultInformation);

            try
            {
                IConfigurationUnitProcessorDetails details = setProcessor.GetUnitProcessorDetails(unit, detailLevel);
                get_self<implementation::ConfigurationUnit>(unit)->Details(std::move(details));
            }
            catch (...)
            {
                ExtractUnitResultInformation(std::current_exception(), unitResultInformation);
            }

            result->UnitResultsVector().Append(*unitResult);
            progress(*unitResult);
        }

        co_return *result;
    }

    void ConfigurationProcessor::GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel)
    {
        return GetUnitDetailsAsync(unit, detailLevel).get();
    }

    Windows::Foundation::IAsyncAction ConfigurationProcessor::GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        ConfigurationUnit localUnit = unit;
        co_await winrt::resume_background();

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);
        IConfigurationUnitProcessorDetails details = setProcessor.GetUnitProcessorDetails(localUnit, detailLevel);
        get_self<implementation::ConfigurationUnit>(localUnit)->Details(std::move(details));
    }

    Configuration::ApplyConfigurationSetResult ConfigurationProcessor::ApplySet(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        return ApplySetAsync(configurationSet, flags).get();
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::ApplyConfigurationSetResult, Configuration::ConfigurationSetChangeData> ConfigurationProcessor::ApplySetAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        ConfigurationSet localSet = configurationSet;
        co_await winrt::resume_background();
        auto progress = co_await winrt::get_progress_token();

        // TODO: Not needed until we have history implemented
        UNREFERENCED_PARAMETER(flags);

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        auto result = make_self<wil::details::module_count_wrapper<implementation::ApplyConfigurationSetResult>>();
        ConfigurationSetApplyProcessor applyProcessor{ localSet, m_threadGlobals.GetTelemetryLogger(), m_factory.CreateSetProcessor(localSet), result, progress};
        progress.set_result(*result);

        applyProcessor.Process();

        co_return *result;
    }

    Configuration::TestConfigurationSetResult ConfigurationProcessor::TestSet(const ConfigurationSet& configurationSet)
    {
        return TestSetAsync(configurationSet).get();
    }

    Windows::Foundation::IAsyncOperationWithProgress<Configuration::TestConfigurationSetResult, Configuration::TestConfigurationUnitResult> ConfigurationProcessor::TestSetAsync(const ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        ConfigurationSet localSet = configurationSet;
        co_await winrt::resume_background();
        auto progress = co_await winrt::get_progress_token();

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(localSet);
        auto result = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationSetResult>>();
        result->TestResult(ConfigurationTestResult::NotRun);
        progress.set_result(*result);

        for (const auto& unit : localSet.ConfigurationUnits())
        {
            AICLI_LOG(Config, Info, << "Testing configuration unit: " << AppInstaller::Utility::ConvertToUTF8(unit.UnitName()));

            auto testResult = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationUnitResult>>();
            auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
            testResult->Initialize(unit, *unitResult);

            if (ShouldTestDuringTest(unit.Intent()))
            {
                IConfigurationUnitProcessor unitProcessor;

                try
                {
                    // TODO: Directives overlay to prevent running elevated for test
                    unitProcessor = setProcessor.CreateUnitProcessor(unit, {});
                }
                catch (...)
                {
                    ExtractUnitResultInformation(std::current_exception(), unitResult);
                }

                if (unitProcessor)
                {
                    try
                    {
                        TestSettingsResult settingsResult = unitProcessor.TestSettings();
                        testResult->TestResult(settingsResult.TestResult());
                        testResult->ResultInformation(settingsResult.ResultInformation());
                    }
                    catch (...)
                    {
                        ExtractUnitResultInformation(std::current_exception(), unitResult);
                    }

                    m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(localSet.InstanceIdentifier(), unit, ConfigurationUnitIntent::Assert, TelemetryTraceLogger::TestAction, testResult->ResultInformation());
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

            progress(*testResult);
        }

        m_threadGlobals.GetTelemetryLogger().LogConfigProcessingSummaryForTest(*winrt::get_self<implementation::ConfigurationSet>(localSet), *result);
        co_return *result;
    }

    Configuration::GetConfigurationUnitSettingsResult ConfigurationProcessor::GetUnitSettings(const ConfigurationUnit& unit)
    {
        return GetUnitSettingsAsync(unit).get();
    }

    Windows::Foundation::IAsyncOperation<Configuration::GetConfigurationUnitSettingsResult> ConfigurationProcessor::GetUnitSettingsAsync(const ConfigurationUnit& unit)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        ConfigurationUnit localUnit = unit;
        co_await winrt::resume_background();

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);
        auto result = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationUnitSettingsResult>>();
        auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
        result->ResultInformation(*unitResult);

        IConfigurationUnitProcessor unitProcessor;

        try
        {
            // TODO: Directives overlay to prevent running elevated for get
            unitProcessor = setProcessor.CreateUnitProcessor(localUnit, {});
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitResult);
        }

        if (unitProcessor)
        {
            try
            {
                GetSettingsResult settingsResult = unitProcessor.GetSettings();
                result->Settings(settingsResult.Settings());
                result->ResultInformation(settingsResult.ResultInformation());
            }
            catch (...)
            {
                ExtractUnitResultInformation(std::current_exception(), unitResult);
            }

            m_threadGlobals.GetTelemetryLogger().LogConfigUnitRunIfAppropriate(GUID_NULL, localUnit, ConfigurationUnitIntent::Inform, TelemetryTraceLogger::GetAction, result->ResultInformation());
        }

        co_return *result;
    }

    void ConfigurationProcessor::Diagnostics(DiagnosticLevel level, std::string_view message)
    {
        if (level >= m_minimumLevel)
        {
            auto diagnostics = make_self<wil::details::module_count_wrapper<implementation::DiagnosticInformation>>();
            diagnostics->Initialize(level, AppInstaller::Utility::ConvertToUTF16(message));
            m_diagnostics(*this, *diagnostics);
        }
    }
}
