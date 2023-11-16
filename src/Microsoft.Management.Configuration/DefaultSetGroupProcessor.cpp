// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DefaultSetGroupProcessor.h"
#include "ExceptionResultHelpers.h"
#include "TestGroupSettingsResult.h"
#include "TestSettingsResult.h"

#include <AppInstallerStrings.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        // Determines whether a configuration unit should be tested.
        bool ShouldTestDuringTest(const Configuration::ConfigurationUnit& unit)
        {
            return unit.IsActive() && unit.Intent() != ConfigurationUnitIntent::Inform;
        }
    }

    void DefaultSetGroupProcessor::Initialize(const ConfigurationSet& set, const IConfigurationSetProcessor& setProcessor, ConfigThreadGlobals& threadGlobals)
    {
        m_set = set;
        m_setProcessor = setProcessor;
        m_threadGlobals = &threadGlobals;
    }

    Windows::Foundation::IInspectable DefaultSetGroupProcessor::Group()
    {
        return m_set;
    }

    Windows::Foundation::IAsyncOperationWithProgress<ITestGroupSettingsResult, ITestSettingsResult> DefaultSetGroupProcessor::TestGroupSettingsAsync()
    {
        auto strongThis = get_strong();
        co_await resume_background();

        auto progress = co_await get_progress_token();
        auto cancellation = co_await get_cancellation_token();
        cancellation.enable_propagation();

        auto result = make_self<wil::details::module_count_wrapper<implementation::TestGroupSettingsResult>>();
        result->Group(m_set);
        progress.set_result(*result);

        try
        {
            for (const auto& unit : m_set.Units())
            {
                ThrowIf(cancellation());

                AICLI_LOG_DIRECT(m_threadGlobals->GetDiagnosticLogger(), Config, Info, << "Testing configuration unit: `" << AppInstaller::Utility::ConvertToUTF8(unit.Identifier()) << "` [" << AppInstaller::Utility::ConvertToUTF8(unit.Type()) << ']');

                ITestSettingsResult settingsResult;
                ConfigurationTestResult testResult = ConfigurationTestResult::Unknown;
                auto unitResult = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();

                if (ShouldTestDuringTest(unit))
                {
                    IConfigurationUnitProcessor unitProcessor;

                    try
                    {
                        unitProcessor = m_setProcessor.CreateUnitProcessor(unit);
                    }
                    catch (...)
                    {
                        ExtractUnitResultInformation(std::current_exception(), unitResult);
                    }

                    // Check again as creating the unit processor could take time
                    ThrowIf(cancellation());

                    if (unitProcessor)
                    {
                        IConfigurationGroupProcessor groupProcessor = unitProcessor.try_as<IConfigurationGroupProcessor>();

                        if (groupProcessor)
                        {
                            auto testOperation = groupProcessor.TestGroupSettingsAsync();

                            testOperation.Progress([&](const auto&, const ITestSettingsResult& unitResult)
                                {
                                    progress(unitResult);
                                });

                            ITestGroupSettingsResult groupResult = co_await testOperation;

                            // Put all of the group's unit results in our unit results
                            for (const auto& groupUnitResult : groupResult.UnitResults())
                            {
                                result->AppendUnitResult(groupUnitResult);
                            }

                            // Convert group result into a unit result for the group
                            auto testSettingsResult = make_self<wil::details::module_count_wrapper<implementation::TestSettingsResult>>();
                            testSettingsResult->Unit(unit);
                            testSettingsResult->TestResult(groupResult.TestResult());
                            testSettingsResult->ResultInformation(groupResult.ResultInformation());
                            settingsResult = *testSettingsResult;
                        }
                        else
                        {
                            try
                            {
                                settingsResult = unitProcessor.TestSettings();
                            }
                            catch (...)
                            {
                                ExtractUnitResultInformation(std::current_exception(), unitResult);
                            }
                        }
                    }
                }
                else
                {
                    testResult = ConfigurationTestResult::NotRun;
                }

                if (FAILED(unitResult->ResultCode()))
                {
                    testResult = ConfigurationTestResult::Failed;
                }

                // Check if we need to construct our own result object
                if (!settingsResult)
                {
                    auto testSettingsResult = make_self<wil::details::module_count_wrapper<implementation::TestSettingsResult>>();
                    testSettingsResult->Unit(unit);
                    testSettingsResult->TestResult(testResult);
                    testSettingsResult->ResultInformation(*unitResult);
                    settingsResult = *testSettingsResult;
                }

                result->AppendUnitResult(settingsResult);

                progress(settingsResult);
            }

            co_return *result;
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), result->ResultInformationInternal());
            throw;
        }
    }

    Windows::Foundation::IAsyncOperationWithProgress<IGetGroupSettingsResult, IGetSettingsResult> DefaultSetGroupProcessor::GetGroupSettingsAsync()
    {
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperationWithProgress<IApplyGroupSettingsResult, IApplySettingsResult> DefaultSetGroupProcessor::ApplyGroupSettingsAsync()
    {
        // Previous implementation...
        ConfigurationSetApplyProcessor applyProcessor{ configurationSet, m_threadGlobals.GetTelemetryLogger(), m_factory.CreateSetProcessor(configurationSet), std::move(progress) };
        applyProcessor.Process();

        return applyProcessor.Result();
    }

    void DefaultSetGroupProcessor::ThrowIf(bool cancellation)
    {
        if (cancellation)
        {
            AICLI_LOG_DIRECT(m_threadGlobals->GetDiagnosticLogger(), Config, Warning, << "Operation cancelled");
            throw winrt::hresult_canceled();
        }
    }
}
