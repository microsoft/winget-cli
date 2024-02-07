// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestConfigurationUnitResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestConfigurationUnitResult : TestConfigurationUnitResultT<TestConfigurationUnitResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;

        TestConfigurationUnitResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ITestSettingsResult& result);

        void Unit(const ConfigurationUnit& unit);
        void ResultInformation(const IConfigurationUnitResultInformation& value);
        void TestResult(ConfigurationTestResult value);
#endif

        ConfigurationUnit Unit();
        IConfigurationUnitResultInformation ResultInformation();
        ConfigurationTestResult TestResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        IConfigurationUnitResultInformation m_resultInformation;
        ConfigurationTestResult m_testResult = ConfigurationTestResult::Unknown;
#endif
    };
}
