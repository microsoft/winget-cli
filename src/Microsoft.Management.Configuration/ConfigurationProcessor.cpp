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

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
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

            void WriteDirect(std::string_view message) noexcept override try
            {
                m_processor.Diagnostics(DiagnosticLevel::Informational, message);
            }
            catch (...) {}

        private:
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

        bool ShouldTestDuringTest(ConfigurationUnitIntent intent)
        {
            return (intent == ConfigurationUnitIntent::Assert || intent == ConfigurationUnitIntent::Apply);
        }
    }

    ConfigurationProcessor::ConfigurationProcessor(const IConfigurationProcessorFactory& factory) : m_factory(factory)
    {
        AppInstaller::Logging::DiagnosticLogger& logger = m_threadGlobals.GetDiagnosticLogger();
        logger.EnableChannel(AppInstaller::Logging::Channel::All);
        logger.SetLevel(AppInstaller::Logging::Level::Verbose);
        logger.AddLogger(std::make_unique<ConfigurationProcessorDiagnosticsLogger>(*this));
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

    Configuration::OpenConfigurationSetResult ConfigurationProcessor::OpenConfigurationSet(Windows::Storage::Streams::IInputStream stream)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        auto result = make_self<wil::details::module_count_wrapper<OpenConfigurationSetResult>>();

        if (!stream)
        {
            result->Initialize(E_POINTER, {});
            return *result;
        }

        try
        {
            std::unique_ptr<ConfigurationSetParser> parser = ConfigurationSetParser::Create(stream);
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field());
                return *result;
            }

            auto configurationSet = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
            configurationSet->Initialize(parser->GetConfigurationUnits());
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field());
                return *result;
            }

            result->Initialize(*configurationSet);
        }
        catch(const wil::ResultException& resultException)
        {
            result->Initialize(resultException.GetErrorCode());
        }
        catch (...)
        {
            result->Initialize(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE);
            LOG_CAUGHT_EXCEPTION();
        }

        return *result;
    }

    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<ConfigurationConflict>> ConfigurationProcessor::CheckForConflictsAsync(
        const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        co_return CheckForConflicts(configurationSets, includeConfigurationHistory);
    }

    Windows::Foundation::IAsyncAction ConfigurationProcessor::GetSetDetailsAsync(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel)
    {
        co_return GetSetDetails(configurationSet, detailLevel);
    }

    Windows::Foundation::IAsyncAction ConfigurationProcessor::GetUnitDetailsAsync(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel)
    {
        co_return GetUnitDetails(unit, detailLevel);
    }

    Windows::Foundation::IAsyncOperation<Configuration::ApplyConfigurationSetResult> ConfigurationProcessor::ApplyAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        co_return Apply(configurationSet, flags);
    }

    Windows::Foundation::IAsyncOperation<Configuration::TestConfigurationSetResult> ConfigurationProcessor::TestAsync(const ConfigurationSet& configurationSet)
    {
        co_return Test(configurationSet);
    }

    Windows::Foundation::IAsyncOperation<Configuration::GetConfigurationUnitSettingsResult> ConfigurationProcessor::GetSettingsAsync(const ConfigurationUnit& unit)
    {
        co_return GetSettings(unit);
    }

    void ConfigurationProcessor::Diagnostics(DiagnosticLevel level, std::string_view message)
    {
        auto diagnostics = make_self<wil::details::module_count_wrapper<implementation::DiagnosticInformation>>();
        diagnostics->Initialize(level, AppInstaller::Utility::ConvertToUTF16(message));
        m_diagnostics(*this, *diagnostics);
    }

    Windows::Foundation::Collections::IVectorView<ConfigurationConflict> ConfigurationProcessor::CheckForConflicts(
        const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        UNREFERENCED_PARAMETER(configurationSets);
        UNREFERENCED_PARAMETER(includeConfigurationHistory);
        THROW_HR(E_NOTIMPL);
    }

    void ConfigurationProcessor::GetSetDetails(const ConfigurationSet& configurationSet, ConfigurationUnitDetailLevel detailLevel)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(configurationSet);

        for (const auto& unit : configurationSet.ConfigurationUnits())
        {
            IConfigurationUnitProcessorDetails details = setProcessor.GetUnitProcessorDetails(unit, detailLevel);
            get_self<implementation::ConfigurationUnit>(unit)->Details(std::move(details));
        }
    }

    void ConfigurationProcessor::GetUnitDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);
        IConfigurationUnitProcessorDetails details = setProcessor.GetUnitProcessorDetails(unit, detailLevel);
        get_self<implementation::ConfigurationUnit>(unit)->Details(std::move(details));
    }

    Configuration::ApplyConfigurationSetResult ConfigurationProcessor::Apply(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        // TODO: Not needed until we have history implemented
        UNREFERENCED_PARAMETER(flags);

        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        ConfigurationSetApplyProcessor applyProcessor{ configurationSet, m_factory.CreateSetProcessor(configurationSet) };
        applyProcessor.Process();

        auto result = make_self<wil::details::module_count_wrapper<implementation::ApplyConfigurationSetResult>>();
        result->Initialize(applyProcessor.GetUnitResults(), applyProcessor.ResultCode());
        return *result;
    }

    Configuration::TestConfigurationSetResult ConfigurationProcessor::Test(const ConfigurationSet& configurationSet)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(configurationSet);
        auto result = make_self<wil::details::module_count_wrapper<implementation::TestConfigurationSetResult>>();
        bool overallResult = true;

        for (const auto& unit : configurationSet.ConfigurationUnits())
        {
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
                    ExtractUnitResultInformation(std::current_exception(), unitResult, setProcessor);
                }

                if (unitProcessor)
                {
                    try
                    {
                        bool unitTestResult = unitProcessor.TestSettings();
                        overallResult = overallResult && unitTestResult;
                        testResult->TestResult(unitTestResult);
                    }
                    catch (...)
                    {
                        ExtractUnitResultInformation(std::current_exception(), unitResult, unitProcessor);
                    }
                }
            }
            else
            {
                unitResult->ResultCode(S_FALSE);
            }

            result->AppendUnitResult(*testResult);
        }

        result->TestResult(overallResult);
        return *result;
    }

    Configuration::GetConfigurationUnitSettingsResult ConfigurationProcessor::GetSettings(const ConfigurationUnit& unit)
    {
        auto threadGlobals = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_NOT_VALID_STATE, !m_factory);

        IConfigurationSetProcessor setProcessor = m_factory.CreateSetProcessor(nullptr);
        auto result = make_self<wil::details::module_count_wrapper<implementation::GetConfigurationUnitSettingsResult>>();
        auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
        result->Initialize(*unitResult);

        IConfigurationUnitProcessor unitProcessor;

        try
        {
            // TODO: Directives overlay to prevent running elevated for get
            unitProcessor = setProcessor.CreateUnitProcessor(unit, {});
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitResult, setProcessor);
        }

        if (unitProcessor)
        {
            try
            {
                result->Settings(unitProcessor.GetSettings());
            }
            catch (...)
            {
                ExtractUnitResultInformation(std::current_exception(), unitResult, unitProcessor);
            }
        }

        return *result;
    }
}
