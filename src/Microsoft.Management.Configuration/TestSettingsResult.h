// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestSettingsResult : winrt::implements<TestSettingsResult, ITestSettingsResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;

        TestSettingsResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Unit(const ConfigurationUnit& value);
        void TestResult(ConfigurationTestResult value);
        void ResultInformation(const IConfigurationUnitResultInformation& value);
#endif

        ConfigurationUnit Unit();
        ConfigurationTestResult TestResult();
        IConfigurationUnitResultInformation ResultInformation();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        ConfigurationTestResult m_testResult = ConfigurationTestResult::Unknown;
        IConfigurationUnitResultInformation m_resultInformation;
#endif
    };
}
