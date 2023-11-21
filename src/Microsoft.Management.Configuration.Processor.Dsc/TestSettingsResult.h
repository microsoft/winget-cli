// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    struct TestSettingsResult : winrt::implements<TestSettingsResult, winrt::Microsoft::Management::Configuration::ITestSettingsResult>
    {
        TestSettingsResult(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit();

        winrt::Microsoft::Management::Configuration::ConfigurationTestResult TestResult();
        void TestResult(winrt::Microsoft::Management::Configuration::ConfigurationTestResult testResult);

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation();
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation resultInformation);

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        winrt::Microsoft::Management::Configuration::ConfigurationTestResult m_testResult = winrt::Microsoft::Management::Configuration::ConfigurationTestResult::NotRun;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}
