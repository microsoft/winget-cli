// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DefaultSetGroupProcessor.h"
#include "TestGroupSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    DefaultSetGroupProcessor::DefaultSetGroupProcessor(const ConfigurationSet& set, const IConfigurationSetProcessor& setProcessor) :
        m_set(set), m_setProcessor(setProcessor)
    {}

    Windows::Foundation::IInspectable DefaultSetGroupProcessor::Group()
    {
        return m_set;
    }

    Windows::Foundation::IAsyncOperationWithProgress<ITestGroupSettingsResult, ITestSettingsResult> DefaultSetGroupProcessor::TestGroupSettingsAsync()
    {
        auto strongThis = get_strong();
        co_await resume_background();

        auto progress = co_await get_progress_token();
        auto result = make_self<wil::details::module_count_wrapper<implementation::TestGroupSettingsResult>>();
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

    Windows::Foundation::IAsyncOperationWithProgress<IGetGroupSettingsResult, IGetSettingsResult> DefaultSetGroupProcessor::GetGroupSettingsAsync()
    {
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperationWithProgress<IApplyGroupSettingsResult, IApplySettingsResult> DefaultSetGroupProcessor::ApplyGroupSettingsAsync()
    {
        THROW_HR(E_NOTIMPL);
    }
}
